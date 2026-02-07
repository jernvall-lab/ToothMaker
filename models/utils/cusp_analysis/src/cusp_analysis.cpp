/**
 * @file cusp_analysis.cpp
 * @brief Cusp analysis result parser for tooth mesh OFF/DAD files.
 *
 * Reads the last-step COFF file and DAD file from a data folder and produces:
 *   1) local_maxima.txt    - Positions of local maxima (cusps), sorted by X.
 *   2) cuspA_baseline.txt  - Position of the border cell closest to X=0.
 *   3) top_cusp_angles.txt - Cusp A angle (between cusps B and C).
 *
 * All output files use append mode so results accumulate across scan items.
 *
 * Usage: cusp_analysis <par_id> <data_folder>
 *   - Run with CWD set to the output directory (where results are written).
 *   - par_id: Parameter set identifier (used as label in output).
 *   - data_folder: Directory containing .off and .dad files from model output.
 */

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define MAX_POLYGON_SIZE 5


struct Vertex {
    double x, y, z;
};

struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<std::vector<int>> faces;
};

struct Cusp {
    double x, y, z;
};



/**
 * @brief Parse a COFF file.
 */
int parse_off(const std::string& filename, Mesh& mesh)
{
    std::ifstream in(filename);
    if (!in.is_open()) {
        std::cerr << "Error: Cannot open " << filename << std::endl;
        return -1;
    }

    // Skip comment lines (starting with '#') before the OFF/COFF header.
    std::string header;
    while (std::getline(in, header)) {
        if (!header.empty() && header[0] != '#')
            break;
    }
    if (header.find("OFF") == std::string::npos) {
        std::cerr << "Error: Not an OFF file: " << filename << std::endl;
        return -1;
    }

    int nVertices = 0, nFaces = 0, nEdges = 0;
    in >> nVertices >> nFaces >> nEdges;
    if (nVertices <= 0 || nFaces <= 0) {
        return -1;
    }

    mesh.vertices.resize(nVertices);
    for (int i = 0; i < nVertices; i++) {
        in >> mesh.vertices[i].x >> mesh.vertices[i].y >> mesh.vertices[i].z;
        // Skip remaining values on the line (COFF color data: r g b a).
        std::string rest;
        std::getline(in, rest);
    }

    mesh.faces.reserve(nFaces);
    for (int i = 0; i < nFaces; i++) {
        int n;
        in >> n;
        std::vector<int> face(n);
        for (int j = 0; j < n; j++) {
            in >> face[j];
        }
        mesh.faces.push_back(face);
    }

    return 0;
}



/**
 * @brief Build vertex adjacency from face list.
 */
void build_adjacency(const Mesh& mesh,
                     std::vector<std::set<int>>& neighbors)
{
    neighbors.resize(mesh.vertices.size());

    for (auto& face : mesh.faces) {
        for (size_t j = 0; j < face.size(); j++) {
            for (size_t k = j + 1; k < face.size(); k++) {
                neighbors[face[j]].insert(face[k]);
                neighbors[face[k]].insert(face[j]);
            }
        }
    }
}



/**
 * @brief Find the DAD file with the highest iteration number in a directory.
 */
std::string find_last_dad_file(const std::string& dir)
{
    std::string best_file;
    int best_iter = -1;

    std::string cmd = "ls " + dir + "/*.dad 2>/dev/null";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "";

    char buf[1024];
    while (fgets(buf, sizeof(buf), pipe)) {
        std::string path(buf);
        if (!path.empty() && path.back() == '\n')
            path.pop_back();

        size_t pos = path.rfind('/');
        std::string fname = (pos != std::string::npos)
                            ? path.substr(pos + 1) : path;

        int iter = 0;
        size_t i = 0;
        while (i < fname.size() && fname[i] >= '0' && fname[i] <= '9') {
            iter = iter * 10 + (fname[i] - '0');
            i++;
        }

        if (i > 0 && iter > best_iter) {
            best_iter = iter;
            best_file = path;
        }
    }
    pclose(pipe);

    return best_file;
}



/**
 * @brief Parse cell shapes (Voronoi polygon vertices) from a DAD file.
 *
 * The DAD file's margins section contains lines like:
 *   <count> cell shape
 *   <x> <y> <z>
 *   ...
 * Each "cell shape" marker starts a new cell's boundary vertices.
 */
int parse_dad_cell_shapes(const std::string& filename,
                          std::vector<std::vector<Vertex>>& cellShapes)
{
    std::ifstream in(filename);
    if (!in.is_open()) {
        std::cerr << "Error: Cannot open " << filename << std::endl;
        return -1;
    }

    // Scan for the cell shapes section (lines containing "cell shape").
    bool inShapes = false;
    int cellIndex = -1;
    std::string line;

    while (std::getline(in, line)) {
        if (line.find("cell shape") != std::string::npos) {
            inShapes = true;
            cellIndex++;
            cellShapes.push_back(std::vector<Vertex>());
            continue;
        }

        if (inShapes) {
            std::istringstream ss(line);
            double x, y, z;
            if (ss >> x >> y >> z) {
                // Check if this is still a vertex line (3 values).
                std::string rest;
                ss >> rest;
                if (rest.empty()) {
                    cellShapes[cellIndex].push_back({x, y, z});
                } else {
                    // Not a vertex line; end of shapes section.
                    break;
                }
            } else {
                // Not a vertex line; end of shapes section.
                break;
            }
        }
    }

    return cellShapes.empty() ? -1 : 0;
}



/**
 * @brief Gets list of cells that contain the given Voronoi node.
 *
 * Ported from writedata.cpp get_cells_with_node_().
 * Checks all cells to find which ones share the specified Voronoi node
 * (within epsilon tolerance).
 *
 * @return Array of MAX_POLYGON_SIZE cell indices (-1 = unused slot).
 */
int *get_cells_with_node(const std::vector<std::vector<Vertex>>& shapes,
                         int nodeCell, int vertIndex, int nVertices)
{
    float epsilon = 0.0001;

    int *cellsWithNode = (int*)malloc(MAX_POLYGON_SIZE * sizeof(int));
    for (int i = 0; i < MAX_POLYGON_SIZE; i++) {
        cellsWithNode[i] = -1;
    }

    int cellsFound = 0;
    auto& node_shape = shapes.at(nodeCell);

    for (int i = 0; i < nVertices; i++) {
        auto& shape = shapes.at(i);

        for (size_t j = 0; j < shape.size(); j++) {
            if (fabs(shape.at(j).x - node_shape.at(vertIndex).x) < epsilon &&
                fabs(shape.at(j).y - node_shape.at(vertIndex).y) < epsilon &&
                fabs(shape.at(j).z - node_shape.at(vertIndex).z) < epsilon) {

                cellsWithNode[cellsFound] = i;
                cellsFound++;
            }
        }
        if (cellsFound == MAX_POLYGON_SIZE) break;
    }

    return cellsWithNode;
}



/**
 * @brief Checks if a cell is a border cell using its Voronoi node sharing.
 *
 * Ported from writedata.cpp is_border_cell_().
 * A cell is on the border if any of its Voronoi nodes is shared by fewer
 * than 3 cells.
 *
 * @return Index of the border node, or -1 if not a border cell.
 */
int is_border_cell(const std::vector<std::vector<Vertex>>& shapes,
                   int i, int nVertices)
{
    for (size_t j = 0; j < shapes.at(i).size(); j++) {
        int *cellsWithNode = get_cells_with_node(shapes, i, j, nVertices);
        if (cellsWithNode[2] == -1) {
            free(cellsWithNode);
            return j;
        }
        free(cellsWithNode);
    }

    return -1;
}



/**
 * @brief Find the OFF file with the highest iteration number in a directory.
 */
std::string find_last_off_file(const std::string& dir)
{
    std::string best_file;
    int best_iter = -1;

    std::string cmd = "ls " + dir + "/*.off 2>/dev/null";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "";

    char buf[1024];
    while (fgets(buf, sizeof(buf), pipe)) {
        std::string path(buf);
        if (!path.empty() && path.back() == '\n')
            path.pop_back();

        size_t pos = path.rfind('/');
        std::string fname = (pos != std::string::npos)
                            ? path.substr(pos + 1) : path;

        int iter = 0;
        size_t i = 0;
        while (i < fname.size() && fname[i] >= '0' && fname[i] <= '9') {
            iter = iter * 10 + (fname[i] - '0');
            i++;
        }

        if (i > 0 && iter > best_iter) {
            best_iter = iter;
            best_file = path;
        }
    }
    pclose(pipe);

    return best_file;
}



/**
 * @brief Open a file for appending. Writes header if the file is new.
 */
std::ofstream open_append(const std::string& path, const std::string& header)
{
    bool exists = std::ifstream(path).good();
    std::ofstream out(path, std::ios::app);
    if (out.is_open() && !exists) {
        out << header << std::endl;
    }
    return out;
}



/**
 * @brief Find local minima (cusp tips) in the mesh.
 *
 * In the tooth model, cusps grow downward from the epithelial sheet,
 * so cusp tips are local minima in Z. The variable name "isMaxima"
 * is kept for consistency with the original writedata.cpp code.
 *
 * @return Vector of cusps sorted by X position.
 */
std::vector<Cusp> find_local_maxima(
    const Mesh& mesh,
    const std::vector<std::set<int>>& neighbors)
{
    const double epsilon = 0.0001;
    std::vector<Cusp> cusps;

    for (size_t i = 0; i < mesh.vertices.size(); i++) {
        double cellZ = mesh.vertices[i].z;
        int nEqualZ = 0;
        bool isMaxima = true;

        for (int nb : neighbors[i]) {
            double diff = mesh.vertices[nb].z - cellZ;
            if (std::fabs(diff) < epsilon) {
                nEqualZ++;
            } else if (diff < 0) {
                isMaxima = false;
                break;
            }
        }

        if (nEqualZ < 2 && isMaxima && neighbors[i].size() > 2) {
            cusps.push_back({mesh.vertices[i].x,
                             mesh.vertices[i].y,
                             mesh.vertices[i].z});
        }
    }

    std::sort(cusps.begin(), cusps.end(),
              [](const Cusp& a, const Cusp& b) { return a.x < b.x; });

    return cusps;
}



/**
 * @brief Write local maxima positions to file.
 */
void write_local_maxima(const std::vector<Cusp>& cusps,
                        const std::string& parId,
                        const std::string& outfile)
{
    auto out = open_append(outfile, "ID X Y Z");
    if (!out.is_open()) {
        std::cerr << "Error: Cannot write to " << outfile << std::endl;
        return;
    }

    for (auto& c : cusps) {
        char line[256];
        snprintf(line, sizeof(line), "%s %lf %lf %lf",
                 parId.c_str(), c.x, c.y, c.z);
        out << line << std::endl;
    }
}



/**
 * @brief Write cusp A baseline position to file.
 *
 * Ported from writedata.cpp Export_main_cusp_baseline().
 * Uses cell shapes from the DAD file to detect border cells, then finds
 * the border cell closest to X=0.
 */
void write_cuspa_baseline(const Mesh& mesh,
                          const std::vector<std::vector<Vertex>>& cellShapes,
                          const std::string& parId,
                          const std::string& outfile)
{
    auto out = open_append(outfile, "ID X Y Z");
    if (!out.is_open()) {
        std::cerr << "Error: Cannot write to " << outfile << std::endl;
        return;
    }

    // If no cell shapes available, output N/A.
    if (cellShapes.empty()) {
        out << parId << " N/A N/A N/A" << std::endl;
        return;
    }

    double minDist = 10000.0;
    int minDistCell = -1;
    int nVertices = mesh.vertices.size();

    for (int i = 0; i < nVertices; i++) {
        if (is_border_cell(cellShapes, i, nVertices) > -1) {
            double cellX = mesh.vertices[i].x;
            if (minDist > (cellX * cellX)) {
                minDist = (cellX * cellX);
                minDistCell = i;
            }
        }
    }

    if (minDistCell == -1) {
        out << parId << " N/A N/A N/A" << std::endl;
    } else {
        char line[256];
        snprintf(line, sizeof(line), "%s %lf %lf %lf",
                 parId.c_str(),
                 mesh.vertices[minDistCell].x,
                 mesh.vertices[minDistCell].y,
                 mesh.vertices[minDistCell].z);
        out << line << std::endl;
    }
}



/**
 * @brief Average nearby maxima into distinct cusps, find cusp A (closest
 *        to origin in the x-y plane).
 * @return Index of cusp A in the output vector, or -1 if empty.
 */
int get_individual_cusps(const std::vector<Cusp>& maxima,
                         std::vector<Cusp>& cusps)
{
    if (maxima.empty()) return -1;

    // Distance threshold below which two maxima are considered part of the
    // same cusp (ignoring the y component).
    const double cusp_limit = 0.1;

    cusps.push_back(maxima[0]);
    double sumX = maxima[0].x, sumY = maxima[0].y, sumZ = maxima[0].z;
    int n = 1;

    for (size_t i = 1; i < maxima.size(); i++) {
        double dx = maxima[i].x - cusps.back().x;
        double dz = maxima[i].z - cusps.back().z;
        if (dx * dx + dz * dz < cusp_limit) {
            sumX += maxima[i].x;
            sumY += maxima[i].y;
            sumZ += maxima[i].z;
            n++;
        } else {
            cusps.back() = {sumX / n, sumY / n, sumZ / n};
            cusps.push_back(maxima[i]);
            sumX = maxima[i].x;
            sumY = maxima[i].y;
            sumZ = maxima[i].z;
            n = 1;
        }
    }
    cusps.back() = {sumX / n, sumY / n, sumZ / n};

    // Find cusp A (closest to origin in x-y plane).
    int cuspA = 0;
    for (size_t i = 1; i < cusps.size(); i++) {
        double d1 = cusps[i].x * cusps[i].x + cusps[i].y * cusps[i].y;
        double d2 = cusps[cuspA].x * cusps[cuspA].x
                   + cusps[cuspA].y * cusps[cuspA].y;
        if (d1 < d2) {
            cuspA = i;
        }
    }

    return cuspA;
}



/**
 * @brief Filter cusps using the inhibitory cascade rule for cusp heights.
 *        Keeps only cusps where height decreases outward from cusp A.
 * @return New index of cusp A in the filtered vector.
 */
int apply_cascade_rule(std::vector<Cusp>& cusps, int cuspA)
{
    if (cuspA < 0 || cuspA >= (int)cusps.size()) return -1;

    // Left side: keep cusps where height decreases toward the left.
    std::vector<Cusp> left;
    for (int i = 0; i < cuspA; i++) {
        if (left.empty() || cusps[i].z < left.back().z) {
            left.push_back(cusps[i]);
        }
    }

    left.push_back(cusps[cuspA]);
    int newCuspA = left.size() - 1;

    // Right side: keep cusps where height decreases toward the right.
    std::vector<Cusp> right;
    for (int i = (int)cusps.size() - 1; i > cuspA; i--) {
        if (right.empty() || cusps[i].z < right.back().z) {
            right.push_back(cusps[i]);
        }
    }

    cusps.clear();
    cusps.insert(cusps.end(), left.begin(), left.end());
    cusps.insert(cusps.end(), right.rbegin(), right.rend());

    return newCuspA;
}



/**
 * @brief Compute the angle at cusp A between vectors to cusps B and C.
 *        Uses the x-z plane (ignoring depth/y).
 */
double compute_cusp_angle(const std::vector<Cusp>& cusps, int cuspA)
{
    auto& pB = cusps[cuspA - 1];
    auto& pA = cusps[cuspA];
    auto& pC = cusps[cuspA + 1];

    double v1x = pB.x - pA.x, v1z = pB.z - pA.z;
    double v2x = pC.x - pA.x, v2z = pC.z - pA.z;

    double n1 = std::sqrt(v1x * v1x + v1z * v1z);
    double n2 = std::sqrt(v2x * v2x + v2z * v2z);

    double cosAngle = (v1x * v2x + v1z * v2z) / (n1 * n2);
    return std::acos(cosAngle);
}



/**
 * @brief Compute and write top cusp angle for one parameter set.
 */
void write_cusp_angle(const std::vector<Cusp>& maxima,
                      const std::string& parId,
                      const std::string& outfile)
{
    auto out = open_append(outfile, "ID\tRADIANS\tDEGREES\tNOTES");
    if (!out.is_open()) {
        std::cerr << "Error: Cannot write to " << outfile << std::endl;
        return;
    }

    std::vector<Cusp> cusps;
    int cuspA = get_individual_cusps(maxima, cusps);

    if (cuspA >= 0) {
        cuspA = apply_cascade_rule(cusps, cuspA);
    }

    if (cuspA < 1 || cuspA > (int)cusps.size() - 2) {
        out << parId << "\tN/A\tN/A\tMissing B and/or C cusp" << std::endl;
        return;
    }

    double angle = compute_cusp_angle(cusps, cuspA);
    out << parId << "\t" << angle << "\t" << angle / (2 * M_PI) * 360
        << "\t" << cusps.size() << " cusps" << std::endl;
}



int main(int argc, char* argv[])
{
    if (argc < 3) {
        std::cerr << "Usage: cusp_analysis <par_id> <data_folder>" << std::endl;
        return -1;
    }

    std::string parId = argv[1];
    std::string dataFolder = argv[2];

    // Find the last-step OFF file in the data folder.
    std::string offFile = find_last_off_file(dataFolder);
    if (offFile.empty()) {
        std::cerr << "Error: No .off files found in " << dataFolder << std::endl;
        return -1;
    }

    // Parse the OFF file.
    Mesh mesh;
    if (parse_off(offFile, mesh)) {
        return -1;
    }

    if (mesh.vertices.empty()) {
        return 0;
    }

    // Build adjacency for local maxima detection.
    std::vector<std::set<int>> neighbors;
    build_adjacency(mesh, neighbors);

    // Find local maxima.
    std::vector<Cusp> maxima = find_local_maxima(mesh, neighbors);

    // Parse cell shapes from the DAD file for border detection.
    std::vector<std::vector<Vertex>> cellShapes;
    std::string dadFile = find_last_dad_file(dataFolder);
    if (!dadFile.empty()) {
        parse_dad_cell_shapes(dadFile, cellShapes);
    }

    // Write all output files to CWD.
    write_local_maxima(maxima, parId, "local_maxima.txt");
    write_cuspa_baseline(mesh, cellShapes, parId, "cuspA_baseline.txt");
    write_cusp_angle(maxima, parId, "top_cusp_angles.txt");

    return 0;
}
