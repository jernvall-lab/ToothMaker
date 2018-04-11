/**
 *  @file writedata.cpp
 *  @brief Parses and writes additional model data (local maxima etc.) to files.
 *
 *  TODO: Currenly contains Humppa-specific stuff solely. Should be externalized
 *  into output/result parsers.
 */

#include <cmath>
#include <vector>

#include "utils/writedata.h"
#include "morphomaker.h"

namespace {

/**
 * @brief Gets vertices directly connected to the vertex cell.
 * @param tooth                 Tooth object.
 * @param cellIndex             Vertex.
 * @param connectedCells_clean  Vertex neighbours.
 */
void get_connected_cells_( Tooth& tooth, int cellIndex,
                           std::vector<int> *connectedCells_clean )
{
    uint16_t i;
    std::vector<int> connectedCells;

    auto& mesh = tooth.get_mesh();
    auto& polygons = mesh.get_polygons();

    // Three or four cells per face. If i is the cell, then i-1 & i+1 are
    // neighbors.
    for ( auto& p : polygons ) {
        if ( p.at(0) == (uint16_t)cellIndex ) {
            connectedCells.push_back( p.at(1) );
        }
        if ( p.at(1) == (uint16_t)cellIndex ) {
            connectedCells.push_back( p.at(0) );
            connectedCells.push_back( p.at(2) );
        }
        if ( p.at(2) == (uint16_t)cellIndex ) {
            connectedCells.push_back( p.at(1) );
        }
        if ( p.size() > 3 ) {
            if ( p.at(2) == (uint16_t)cellIndex ) {
                connectedCells.push_back( p.at(3) );
            }
            if ( p.at(3) == (uint16_t)cellIndex ) {
                connectedCells.push_back( p.at(2) );
            }
        }
    }

    // At this point there's lot's of duplicates; get rid of them:
    uint16_t j;
    if ( connectedCells.size() > 0 ) {
        connectedCells_clean->push_back(connectedCells.at(0));
    }
    else return;

    for (i=1; i<connectedCells.size(); i++) {
        int isthere=0;
        for (j=0; j<connectedCells_clean->size(); j++) {
            if (connectedCells_clean->at(j)==connectedCells.at(i)) {
                isthere++;
                break;
            }
        }
        if (!isthere) {
            connectedCells_clean->push_back(connectedCells.at(i));
        }
    }
}


/**
 * @brief Gets list of cells that contain the given node in their domain.
 * - Each cell shape node belongs to the cell it surrounds; this gives us one
 *   the two cells whose connection is crossed between two shape nodes. To find
 *   the other cell, take one of the two nodes: There's two cells connected to
 *   the center cell that also have this node as a part of their domain; get
 *   those two cells figure out which one is closer to the other node.
 *
 * @param tooth         Tooth object.
 * @param nodeCell      Cell containing the node.
 * @param vertIndex     Vertex containing the node.
 * @return              Indices of the cells containing the node.
 */
int *get_cells_with_node_(Tooth& tooth, int nodeCell, int vertIndex)
{
    float epsilon = 0.0001;         // for comparing floating point values.

    int *cellsWithNode = (int*)malloc(MAX_POLYGON_SIZE*sizeof(int));
    for (int i=0; i<MAX_POLYGON_SIZE; i++) {
        cellsWithNode[i]=-1;
    }

    int nVertices = tooth.get_mesh().get_vertices().size();
    auto& shapes = tooth.get_cell_shapes();

    int cellsFound = 0;
    auto& node_shape = shapes.at( nodeCell );

    for (int i=0; i<nVertices; i++) {
        auto& shape = shapes.at(i);

        for (size_t j=0; j<shape.size(); j++) {
            if (fabs(shape.at(j).x - node_shape.at(vertIndex).x) < epsilon &&
                fabs(shape.at(j).y - node_shape.at(vertIndex).y) < epsilon &&
                fabs(shape.at(j).z - node_shape.at(vertIndex).z) < epsilon) {

                cellsWithNode[cellsFound]=i;
                cellsFound++;
            }
        }
        if (cellsFound==MAX_POLYGON_SIZE) break;
    }

    return cellsWithNode;
}



/**
 * @brief Checks if current cell is a border cell.
 *
 * @param tooth     Tooth object.
 * @param i         Cell index.
 * @return          1 if border cell, else 0.
 */
int _is_border_cell(Tooth& tooth, int i)
{
    auto& shapes = tooth.get_cell_shapes();

    for (uint16_t j=0; j<shapes.at(i).size(); j++) {
        int *cellsWithNode = get_cells_with_node_(tooth, i,j);
        if (cellsWithNode[2]==-1) {
            free(cellsWithNode);
            return j;
        }
        free(cellsWithNode);
    }

    return -1;
}

}


/**
 * @brief Deduces the vertices of local maxima in 3D data, writes locaations
 *        to file.
 *
 * - If the output file already exists, appends to it.
 *
 * @param tooth         Tooth object.
 * @param outfile       Output file name.
 * @param id            Parameter ID.
 */
void morphomaker::Export_local_maxima( Tooth& tooth, std::string outfile,
                                       std::string id )
{
    float epsilon = 0.0001;         // for comparing floating point values.

    // Check the existence of output file.
    std::string output_flag = "w";
    FILE* input = fopen(outfile.c_str(), "r");
    if (input != NULL) {
        output_flag = "a";
        fclose(input);
    }

    // If the output file exists, open for appending; else writing.
    FILE* output = fopen(outfile.c_str(), output_flag.c_str());
    if (output == NULL) {
        fprintf(stderr, "Error: Can't open file '%s' for writing.\n",
                outfile.c_str());
        return;
    }
    if (output_flag == "w") {
        fprintf(output, "ID X Y Z\n");
    }

    auto& vertices = tooth.get_mesh().get_vertices();
    if (vertices.size() == 0) {
        fclose(output);
        return;
    }

    std::vector<int> connected;
    std::vector<double> cusps;

    for (uint16_t i=0; i<vertices.size(); i++) {
        int nEqualCellZ=0;
        int isMaxima=1;
        double cellZ = vertices.at(i).z;

        connected.clear();
        get_connected_cells_(tooth, i, &connected);

        for (uint16_t j=0; j<connected.size(); j++) {
            if (std::fabs(vertices.at( connected.at(j) ).z - cellZ) < epsilon) {
                nEqualCellZ++;
            }
            else if (vertices.at( connected.at(j) ).z < cellZ) {
                isMaxima=0;
                break;  // Ok, some cell is 'higher up' than our cell.. done.
            }
            else {}
        }

        // Must be one equal Z cell at max; must a local maximum;
        // must be at least 3 connections to other cells.
        if ((nEqualCellZ<2) && isMaxima && connected.size()>2) {
            cusps.push_back( vertices.at(i).x );
            cusps.push_back( vertices.at(i).y );
            cusps.push_back( vertices.at(i).z );
        }
    }

    // Write cusps to file sorted by X position.
    std::vector<int> indices;
    int indexTmp;
    for (uint16_t i=0; i<cusps.size()/3; i++) {
        indices.push_back(i);
    }

    for (uint16_t i=0; i<indices.size(); i++) {
        for (uint16_t j=i; j<indices.size(); j++) {
            if (cusps.at(indices.at(i)*3) > cusps.at(indices.at(j)*3)) {
                indexTmp = indices.at(i);
                indices.at(i) = indices.at(j);
                indices.at(j) = indexTmp;
            }
        }
    }

    for (uint16_t i=0; i<indices.size(); i++) {
        fprintf(output, "%s %lf %lf %lf\n", id.c_str(), cusps.at(indices.at(i)*3),
                cusps.at(indices.at(i)*3+1), cusps.at(indices.at(i)*3+2));
    }

    fclose(output);
}



/**
 * @brief Deduces the tooth main cusp base coordinates, writes to file.
 *
 * - If the output file already exists, appends to it.
 *
 * @param tooth         Tooth object.
 * @param outfile       Output file name.
 * @param id            Parameter ID.
 */
void morphomaker::Export_main_cusp_baseline( Tooth& tooth, std::string outfile,
                                             std::string id )
{
    // Check the existence of output file.
    std::string output_flag = "w";
    FILE* input = fopen(outfile.c_str(), "r");
    if (input != NULL) {
        output_flag = "a";
        fclose(input);
    }

    // If the output file exists, open for appending; else writing.
    FILE* output = fopen(outfile.c_str(), output_flag.c_str());
    if (output == NULL) {
        fprintf(stderr, "Error: Can't open file '%s' for writing.\n",
                outfile.c_str());
        return;
    }
    if (output_flag == "w") {
        fprintf(output, "ID X Y Z\n");
    }

    auto& vertices = tooth.get_mesh().get_vertices();
    if (vertices.size() == 0) {
        fclose(output);
        return;
    }

    double minDist = 10000.0;
    int minDistCell = -1;
    double cellX = 0.0;

    for (uint16_t i=0; i<vertices.size(); i++) {
        if (_is_border_cell(tooth, i)>-1) {
            cellX = vertices.at(i).x;
            if (minDist>(cellX*cellX)) {
                minDist=(cellX*cellX);
                minDistCell=i;
            }
        }
    }

    if (minDistCell==-1) {
        fprintf(output, "%s N/A N/A N/A\n", id.c_str());
    }
    else {
        fprintf( output, "%s %lf %lf %lf\n", id.c_str(),
                 vertices.at(minDistCell).x, vertices.at(minDistCell).y,
                 vertices.at(minDistCell).z );
    }

    fclose(output);
}
