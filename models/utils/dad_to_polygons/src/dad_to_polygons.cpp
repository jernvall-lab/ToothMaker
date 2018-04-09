//
// Constructs triangular mesh using the Humppa's output .off and .dad files.
//
// - Takes vertex data from the .off file, and cell connections from the .dad
//   file for constructing the triangles.
//
// - Prints each triangle with both orientations, as we don't have the surface
//   orientation information. Consequently, the total number of polygons in the
//   output is the double the real number.
//
// - Requires that the input file name is of the form produced by Humppa. 
//   Output file name is constructed from the input file name such that 
//   MorphoMaker will recognize it. 
//

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <iterator>
#include <functional>

#include "dad_to_polygons.h"    // templates

#define TOOTH_COLOR 0.5         // Default tooth color.
#define TOOTH_WHITE 1.0         // Color for differentiated cells & knots.



/**
 *  Read vertex data from .off file.
 */
int read_off_vertices( std::string off, Vector<double>& vertex_data )
{
    std::ifstream in(off);

    std::string s;
    in >> s;
    if (s.find("COFF")) {
        in.close();
        return EXIT_FAILURE;
    }

    int nvert, nfaces, nedges;
    in >> nvert; in >> nfaces; in >> nedges;

    for (int i=0; i<nvert; i++) {
        if (!in.good()) return EXIT_FAILURE;
        std::vector<double> data;
        line_to_vector(in, data);
        vertex_data.push_back(data);
    }

    in.close();
    return EXIT_SUCCESS;
}



/**
 *  Read cell connections from .dad file.
 */
int read_dad_nlist( std::string dad, int nvert, Vector<int>& nlist )
{
    std::ifstream in(dad);

    while (true) {
        if (!in.good()) return EXIT_FAILURE;
        std::vector<double> data;
        line_to_vector(in, data);
        if (data.size() < 2) break;
    }

    for (int i=0; i<nvert; i++) {
        if (!in.good()) return EXIT_FAILURE;

        std::vector<int> data;
        line_to_vector(in, data);
        nlist.push_back(data);

        std::string line;
        std::getline(in, line);     // ignore every other line
    }

    in.close();
    return EXIT_SUCCESS;
}



/**
 *  Shifts node indices minus one, removes the fake node.
 */
void replace_nlist_indices( Vector<int>& nlist )
{
    for (auto& list : nlist) {
        // Remove the fake node
        auto it = std::remove_if( list.begin(), list.end(), 
                                  [&](uint32_t v){ return v>nlist.size(); } );
        list.erase( it, list.end() );
        // Shift the rest one down
        std::transform( list.begin(), list.end(), list.begin(),
                        std::bind2nd(std::minus<int>(), 1) );        
    }
}



/**
 *  Constructs triangles and quads from cell connections data.
 */
void construct_triangles_quads( const Vector<int>& nlist, 
                                Vector<int>& tris, Vector<int>& quads )
{
    for (int i=0; i<(int)nlist.size(); i++) {
        // triangles
        for (auto j : nlist.at(i)) {
            for (auto k : set_intersect( nlist.at(j), nlist.at(i) )) {
                tris.push_back( {i, j, k} );
            }
        }

        // quads
        for (auto j : nlist.at(i)) {
            for (auto k : set_diff( nlist.at(j), nlist.at(i) )) {
                if (k == i) continue;

                for (auto w : set_intersect( nlist.at(k), nlist.at(i) )) {
                    if (w == j) continue;

                    // w is our candidate fourth node for a quad.
                    // Make sure the quad is not crossed by triangles:
                    auto c = set_intersect( nlist.at(w), nlist.at(j) );
                    auto diff_c = set_diff( c, {i,k} );
                    if (diff_c.size() > 0) continue;

                    c = set_intersect( nlist.at(j), {w} );
                    if (c.size() > 0) continue;

                    quads.push_back( {i, j, k, w} );
                }
            }
        }
    }
}



/**
 *  Get unique data rows.
 *  Two rows are considered equal if they are equal sets.
 */
void unique_rows( Vector<int>& data )
{
    // Returns true if a and b are equal sets.
    auto equal = []( std::vector<int> a, std::vector<int> b ) 
    {
          std::sort( a.begin(), a.end() );
          std::sort( b.begin(), b.end() );
          return std::equal( a.begin(), a.begin()+a.size(), b.begin() );
    };

    // Returns true if sorted a < sorted b.
    auto less_than = []( std::vector<int> a, std::vector<int> b ) 
    {
          std::sort( a.begin(), a.end() );
          std::sort( b.begin(), b.end() );
          return a < b;
    };

    std::sort( data.begin(), data.end(), less_than );
    auto last = std::unique( data.begin(), data.end(), equal );
    data.erase( last, data.end() );

    std::sort( data.begin(), data.end() );
}



/**
 *  Splits quads to triangles.
 */
Vector<int> quads_to_tris( const Vector<int>& quads )
{
    Vector<int> tris;
    for (auto q : quads) {
        tris.push_back( {q.at(0), q.at(1), q.at(2)} );
        tris.push_back( {q.at(0), q.at(2), q.at(3)} );
    }
    return tris;
}



/**
 *  Writes .off file.
 *  Each triangle is written with both orientations (twice).
 */
int write_off( const std::string fname, const Vector<double>& vertex_data,
               const Vector<int>& tris )
{
    std::ofstream out(fname);
    if (!out.good()) {        
        return EXIT_FAILURE;
    }

    out << "# Generated by dad_to_polygons. Vertex data from Humppa's .off file,"
        << std::endl << "# polygons parsed from .dad file." << std::endl;
    out << "COFF" << std::endl;
    out << vertex_data.size() << " " << 2*tris.size() << " " << vertex_data.size()
        << std::endl;
    
    for (auto line : vertex_data) {
        out << std::fixed << line.at(0) << " " << line.at(1) << " " << line.at(2);

        if (line.at(6) < 0.6) {             // Differentiated
            out << " " << TOOTH_WHITE << " " << TOOTH_WHITE << " "
                << TOOTH_WHITE << " 1.0";
        }
        else if  (line.at(6) > 0.999) {     // Knot
            out << " 1.0 1.0 0.0 1.0";
        }
        else {
            out << " " << TOOTH_COLOR << " " << TOOTH_COLOR << " "
                << TOOTH_COLOR << " 1.0";
        }
        out << std::endl;
    }

    for (auto line : tris) {
        out << "3 " << line.at(0) << " " << line.at(1) << " " << line.at(2);
        out << std::endl;
        out << "3 " << line.at(0) << " " << line.at(2) << " " << line.at(1);
        out << std::endl;
    }

    out.close();
    return EXIT_SUCCESS;
}



/**
 *  Constructs MorphoMaker-style output file name from the input file name.
 *  Assumes the input file is of form xyz_dsa__.off. If not, returns an empty
 *  string.
 */
std::string get_output_name( std::string infile )
{
    // Strip path from file name, if applicable.
    size_t idx = infile.find_last_of("/");
    std::string out = infile;
    if (idx != std::string::npos) {
        out = infile.substr(idx, infile.length());    
    }

    std::stringstream ss(out);
    std::vector<std::string> pieces;
    std::string item;
    while (std::getline(ss, item, '_')) {
        if (!item.empty()) {
            pieces.push_back(item);
        }
    }

    if (pieces.size() != 3) {
        return "";
    }
    out = pieces.at(0) + "_" + pieces.at(1) + pieces.at(2);

    // Put path back to the file name.
    if (idx != std::string::npos) {
        out = infile.substr(0, idx) + out;    
    }

    return out;
}



int main( int argc, char* argv[] )
{
    if (argc < 2) {
        std::cout << "Usage: dad_to_polygons [input.off]" 
                  << std::endl;
        return 0;
    }

    // Input file names. Expecting .off file given, and the presence of a .dad
    // file with the same file name body.
    std::string off( argv[1] );
    size_t idx = off.find_last_of(".");
    if (idx == std::string::npos) {
        return -1;   
    }
    std::string ext = off.substr(idx, off.length());
    if (ext.compare(".off")) {
        return -1;
    }
    std::string dad = off.substr(0, idx) + ".dad";

    // Constrcut output file name from the input .off file name.
    std::string out = get_output_name( off );
    if (out.empty()) {
        std::cerr << "Error: Input file name '" << off << "' not recognized."
                  << "Should be of form 'xyz__zyx__.off', with '_' as sepators."
                  << std::endl;
        return -1;
    }    

    // If the output file exits & appears to be written by this parser, exit.
    std::ifstream in(out);
    if (in.good()) {
        std::string s;
        in >> s;
        if (s.find("# Generated by dad_to_polygons")) {
            return 0;
        }
    }

    Vector<double> vertex_data;
    if (read_off_vertices( off, vertex_data )) {
        std::cerr << "Error: Cannot read .off file '" << off << "'."
                  << std::endl;
        return -1;
    }

    Vector<int> nlist;
    if (read_dad_nlist( dad, vertex_data.size(), nlist )) {
        std::cerr << "Error: Cannot read .dad file '" << off << "'."
                  << std::endl;
        return -1;
    }

    replace_nlist_indices( nlist );

    Vector<int> tris, quads;
    construct_triangles_quads( nlist, tris, quads );

    unique_rows(tris);
    unique_rows(quads);
    auto quad_tris = quads_to_tris(quads);
    tris.insert( tris.end(), quad_tris.begin(), quad_tris.end() );
    unique_rows(tris);

    if (write_off( out, vertex_data, tris )) {
        std::cerr << "Error: Cannot open file '" << out << "' for writing." 
                  << std::endl;
        return -1;
    }

    return 0;
}
