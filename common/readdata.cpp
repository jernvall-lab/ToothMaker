/**
 *  @file readdata.cpp
 *  @brief Reads model output.
 *
 */

#include <fstream>
#include <sstream>
#include <iostream>
#include <QString>
#include <QStringList>
#include <QDir>

#include <tooth.h>
#include <mesh.h>
#include <readdata.h>


namespace {

/**
 * @brief Eats away empty and comment lines from input stream.
 * @param in    Input stream.
 * @return
 */
void _eat_comments(std::istream& in) {
    char line[256];

    while (true) {
        if (!in.good()) return;
        // NOTE: std:ws eats newlines too!
        in >> std::ws;

        int pos = in.tellg();
        in.getline(line, 256);
        if (strncmp(line,"comment", 7) && strncmp(line, "#", 1)) {
            in.seekg(pos, std::ios_base::beg);
            break;
        }
    }
}


/**
 * @brief Read vertex properties in PLY files.
 * @param in        Input stream.
 * @param n_dim     Number of dimensions detected.
 * @param n_colors  Number of colors detected.
 */
void _read_ply_vertex(std::istream& in, int& n_dim, int& n_colors)
{
    std::string s[3] = {""};
    std::string trash;
    n_dim = 0;
    n_colors = 0;

    while (true) {
        if (!in.good()) return;
        int pos = in.tellg();
        _eat_comments(in);
        in >> s[0]; in >> s[1]; in >> s[2];
        std::getline(in, trash);
        if (!s[0].compare("property") && (!s[1].compare("float") || !s[1].compare("double"))) {
            if (!s[2].compare("x") || !s[2].compare("y") || !s[2].compare("z")) {
                n_dim = n_dim+1;
            }
        }
        else if (!s[0].compare("property") && (!s[1].compare("uchar"))) {
            if (!s[2].compare("red") || !s[2].compare("green") || !s[2].compare("blue") ||
                !s[2].compare("alpha")) {
                n_colors = n_colors+1;
            }
        }
        else {
            in.seekg(pos, std::ios_base::beg);
            break;
        }
    }
}


/**
 * @brief Read concentration properties in PLY files.
 * @param in        Input stream.
 * @param list      List of detected morphogen names.
 */
void _read_ply_concentrations(std::istream& in, std::vector<std::string>& list)
{
    std::string s[3] = {""};
    std::string trash;

    while (true) {
        if (!in.good()) return;
        int pos = in.tellg();
        _eat_comments(in);
        in >> s[0]; in >> s[1]; in >> s[2];
        std::getline(in, trash);
        if (!s[0].compare("property") && (!s[1].compare("float") || !s[1].compare("double"))) {
            list.push_back(s[2]);
        }
        else {
            in.seekg(pos, std::ios_base::beg);
            break;
        }
    }
}

}



/**
 * @brief Read MxN matrix from binary file.
 *
 * Note: We assume rather strictly that integers are of size 4 bytes and
 * floats 4 bytes.
 *
 * TODO:
 * - Add support for multiple data files.
 *
 * @param fname     File name.
 * @param tooth     Tooth object to store the data.
 * @return          EXIT_SUCCESS, EXIT_FAILURE.
 */
int morphomaker::Read_BIN_matrix( const std::string& fname, Tooth& tooth )
{
    std::ifstream in(fname);
    in.unsetf(std::ios::skipws);

    // Test if there's something to read.
    // First 8 bits code the matrix dimensions, followed by the data.
    in.seekg(0, std::ios::end);
    int size = in.tellg();
    if (size <= 8) {
        return EXIT_FAILURE;
    }
    in.seekg(0, std::ios::beg);

    // Read matrix dimensions.
    uint32_t m, n;
    char bytes[4];
    in.read(bytes, 4);
    memcpy(&m, bytes, 4);
    in.read(bytes, 4);
    memcpy(&n, bytes, 4);

    if ((uint32_t)size < m*n*4 + 8) {       // Incomplete data file.
        return EXIT_FAILURE;
    }

    // Read the actual data as chars.
    std::vector<char> vec;
    vec.reserve( size - 8 );
    vec.insert( vec.begin(),
                std::istream_iterator<char>(in),
                std::istream_iterator<char>() );

    // Converting to float vector and uploading.
    // TODO: Can this be done more efficiently?
    float* floats = reinterpret_cast<float*>( &vec[0] );
    std::vector<float> data;
    data.assign( floats, floats + m*n );

    tooth.set_domain_dim( m, n );
    tooth.add_cell_data( data );

    return EXIT_SUCCESS;
}



/**
 * @brief Read object data in OFF file format.
 * - Detects the number of dimensions, colors (RGB or RGBA supported).
 * - Polygon data optional.
 * - Morphogen concentrations optional.
 * - Comments accepted anywhere.
 *
 * Limitations:
 * - Elements must be followed by correct properties.
 * - No support for fancy elements/properties.
 * - Not much fault tolerance in general.
 *
 * @param fname     File name.
 * @param Tooth     Tooth object to store the object data.
 * @return          EXIT_SUCCESS, EXIT_FAILURE.
 */
int morphomaker::Read_PLY_file(const std::string& fname, Tooth& tooth)
{
    std::ifstream in(fname);
    std::string s[3] = {""};
    std::string trash;

    const int maxVar = 7;

    // File recognition.
    _eat_comments(in);
    in >> s[0];
    std::getline(in, trash);
    if (s[0].compare("ply")) {
        std::cerr << "Invalid data file format. Aborting." << std::endl;
        return EXIT_FAILURE;
    }
    _eat_comments(in);
    in >> s[0]; in >> s[1]; in >> s[2];
    std::getline(in, trash);
    if (s[0].compare("format") || s[1].compare("ascii") || atoi(s[2].c_str())!=1.0) {
        std::cerr << "Unknown format \"" << s[1] << "\". Only ascii supported. Aborting."
                  << std::endl;
        return EXIT_FAILURE;
    }

    // Read elements.
    int n_vert=0, n_face=0;
    int n_dim=0, n_colors=0;
    std::vector<std::string> morphogens;

    while (true) {
        if (!in.good()) return EXIT_FAILURE;
        _eat_comments(in);
        in >> s[0];
        if (!s[0].compare("end_header")) {
            break;
        }
        in >> s[1]; in >> s[2];
        std::getline(in, trash);

        if (!s[0].compare("element")) {
            if (!s[1].compare("vertex")) {
                n_vert = atoi(s[2].c_str());
                _read_ply_vertex(in, n_dim, n_colors);
            }
            if (!s[1].compare("face")) {
                n_face = atoi(s[2].c_str());
                // The next line should declare the vertex_indices, not interested.
                _eat_comments(in);
                std::getline(in, trash);
            }
            if (!s[1].compare("concentrations")) {
                // This must equal to n_vert.
                if (n_vert != atoi(s[2].c_str())) {
                    std::cerr << "Invalid number of concentrations. Aborting." << std::endl;
                    return EXIT_FAILURE;
                }
                _read_ply_concentrations(in, morphogens);
            }
        }
        else {
        }
    }

    Mesh mesh( n_vert, n_face );

    // Read n_dim vertices + n_colors colors per line.
    // NOTE: Unrecognised entries ignored.
    float p[maxVar] = {0.0};
    int i,j;

    for (i=0; i<n_vert; i++) {
        if (!in.good()) return EXIT_FAILURE;

        for (j=0; j<n_dim+n_colors; j++) {
            if (!in.good()) return EXIT_FAILURE;
            in >> p[j];
            if (j>=n_dim) {
                p[j] = (float)p[j]/255.0;
            }
        }
        std::getline(in, trash);

        vertex_color color;
        if ( n_colors >= 3 ) {
            color.r = p[n_dim];
            color.g = p[n_dim+1];
            color.b = p[n_dim+2];
            color.a = 1.0;
            if ( n_colors == 4 ) {
                color.a = p[n_dim+3];
            }
        }

        mesh.add_vertex( p[0], p[1], p[2] );
        mesh.set_vertex_color( i, color );
    }

    // Store a copy of current object colors to avoid losing them later when
    // manipulating vertex colors from the interface.
    mesh.set_alt_colors( mesh.get_vertex_colors() );

    // If there's polygons, they're expected next.
    int n, v;

    for (i=0; i<n_face; i++) {
        if (!in.good()) return EXIT_FAILURE;
        in >> n;

        std::vector<uint32_t> polygon;
        for (j=0; j<n; j++) {
            if (!in.good()) return EXIT_FAILURE;
            in >> v;
            polygon.push_back( v );
        }
        std::getline(in, trash);
        mesh.add_polygon( polygon );
    }

    // Concentrations, if present.
    for (i=0; i<n_vert; i++) {
        if (!in.good()) return EXIT_FAILURE;
        property prop;

        for (j=0; j<(int)morphogens.size(); j++) {
            if (!in.good()) return EXIT_FAILURE;
            double v;
            in >> v;
            if (v > 1.0) {       // Presume the values are in scale 0-255.
                v = v / 255.0;
            }
            vertex_color c;
            c.r = v; c.g = v; c.b = v; c.a = 1.0;
            prop.push_back( v );
            mesh.set_vertex_color( i, c );
        }
        mesh.set_property( prop );
    }

    tooth.add_mesh( mesh );

    return EXIT_SUCCESS;
}



/**
 * @brief Read object data in OFF file format.
 *
 * Limitations:
 * - Data lines must not have any trailing whitespaces.
 *
 * @param fname     File name.
 * @param tooth     Tooth object to store the object data.
 * @return          EXIT_SUCCESS, EXIT_FAILURE.
 */
int morphomaker::Read_OFF_file(const std::string& fname, Tooth& tooth)
{
    std::ifstream in(fname);
    if (!in.good()) {
        std::cerr << "Error: Cannot open file '" << fname << "' for reading."
                  << std::endl;
        return EXIT_FAILURE;
    }

    // Must find 'OFF' or 'COFF' tag on the first non-comment line.
    _eat_comments(in);
    std::string s;
    in >> s;
    if (s.find("OFF") && s.find("COFF")) {
        std::cerr << "Error: Invalid header in " << fname << ". Expecting "
                  << "to find 'OFF' or 'COFF'." << std::endl;
        return EXIT_FAILURE;
    }

    // The next line should contain the vertices, faces counts.
    _eat_comments(in);
    int nvertices=0, nfaces=0, nedges=0;
    in >> nvertices; in >> nfaces; in >> nedges;

    Mesh mesh( nvertices, nfaces );

    // Maximum number of variables. For now, either 3 vertices,
    // or 3 vertices + 4 colors.
    const int maxVar = 7;
    float p[maxVar] = {0.0};
    int t[4] = {0};

    // Read nvertices lines of vertex data.
    int i,j;
    for (i=0; i<nvertices; i++) {
        if (!in.good()) return EXIT_FAILURE;
        _eat_comments(in);

        // Read node coordinates and potentially vertex color information.
        for (j=0; j<maxVar; j++) {
            if (in.peek() == '\n') {
                break;
            }
            in >> p[j];
        }
        mesh.add_vertex( p[0], p[1], p[2] );
        vertex_color c = { 0.0, 0.0, 0.0, 0.0 };
        if ( j == 7 ) {     // Only acccept RGBA colors, hence must be 7 cols.
            c = { p[3], p[4], p[5], p[6] };
        }
        mesh.set_vertex_color( i, c );
    }

    // Store a copy of current object colors to avoid losing them later when
    // manipulating vertex colors from the interface.
    mesh.set_alt_colors( mesh.get_vertex_colors() );

    // Read nfaces lines of polygon data.
    for (i=0; i<nfaces; i++) {
        if (!in.good()) return EXIT_FAILURE;
        _eat_comments(in);

        in >> t[0];
        if (t[0]<3 || t[0]>4) {
            std::cerr << "Error: Unsupported polygon size " << t[0] << "."
                      << " Only triangles and quads supported." << std::endl;
            return EXIT_FAILURE;
        }

        // TODO: Split quads to triangles.
        std::vector<uint32_t> polygon;
        for (j=1; j<=t[0]; j++) {
            in >> t[j];
            polygon.push_back(t[j]);
        }
        mesh.add_polygon( polygon );
    }

    tooth.add_mesh( mesh );

    return EXIT_SUCCESS;
}



/**
 * @brief Reads Hummpa .dad file.
 *
 * TODO: Currently assumes fixed length field separators. Using 64bit Humppa binaries
 *       increases the length of the field separator on some platforms, causing this
 *       function to fail, resulting in the loss of morphogen/vertex color data.
 *
 * @param step      Current step
 * @param stepsize  Step size
 * @param run_id    Model run id
 * @param tooth     Tooth object for storing the data
 * @return          -1 File reading failed. 0 OK.
 */
int morphomaker::Read_Humppa_DAD_file( int step, int stepsize, int run_id,
                                       Tooth& tooth)
{
    // Find the exact file name.
    QString target = QString::number(step*stepsize) + "*"
                     + QString::number(run_id) + "*.dad";
    QStringList filter;
    filter << target;
    QDir qdir("./");
    QFileInfoList files = qdir.entryInfoList( filter, QDir::Files );
    if (files.size() == 0) {
        return -1;
    }

    char dadFileName[1024];
    strcpy( dadFileName, files.at(0).fileName().toStdString().c_str() );

    FILE* input = fopen(dadFileName, "r");
    if (input==NULL) {
        if (DEBUG_MODE) fprintf(stderr, "%s(): Can't open file '%s'. Aborted.\n",
                                __FUNCTION__, dadFileName);
        return -1;
    }

    char dataBegin[64];
    int nvertices = tooth.get_mesh().get_vertices().size();
    sprintf(dataBegin, " %d %d\n", stepsize, nvertices);

    uint32_t i=0, j=0;
    int filePos=0, cnt=0;
    QStringList list;
    char tmp[256];

    while (!feof(input)) {
        if (fgets(tmp, 255, input) == NULL) {
            fclose(input);
            return -1;
        }

        if (!strcmp(tmp, dataBegin) && filePos==4) {  // Found concentrations.
            filePos=4;
            cnt = 0;
            break;
        }
        if (!strcmp(tmp, dataBegin) && filePos==3) {
            filePos=4;
            sprintf(dataBegin, " 4 %d\n", nvertices);
            continue;
        }
        if (!strcmp(tmp, dataBegin) && filePos==2) {
            filePos=3;
            i=0;
            sprintf(dataBegin, " 5 %d\n", nvertices);
            continue;
        }
        if (!strcmp(tmp, dataBegin) && filePos==1) {  // Found begin of cell shapes.
            filePos=2;
            i=0;
            continue;
        }
        if (!strcmp(tmp, dataBegin) && filePos==0) {
            filePos=1;
            continue;
        }

        if (filePos==2) {  // Read cell shapes.
            QString str = QString(tmp);
            list = str.split(" ");
            if (!list[2].compare("cell")) {
                j=0;
                i++;
                continue;
            }

            vertex vert = { list[1].toFloat(), list[2].toFloat(), list[3].toFloat() };
            tooth.add_cell_shape( i-1, vert );
            j++;
        }

        if (filePos==4) {  // Read cell data (concentrations).
            char *pch;
            j=0;
            std::vector<float> data;

            pch = strtok(tmp, " ");
            while (pch != NULL && j<5) {
                data.push_back( atof(pch) );
                pch = strtok(NULL, " ");
                j++;
            }

            // Humppa's DAD file contains concentrations for the epith. cells + stack of
            // 3 mesench. cells below each epith. cell. We only want to read the epith.
            // concentrations.
            if ( cnt%4 == 0 ) {
                tooth.add_cell_data( data );
                cnt = 0;
            }
            cnt++;
            i++;
        }
    }

    fclose(input);

    return 0;
}

