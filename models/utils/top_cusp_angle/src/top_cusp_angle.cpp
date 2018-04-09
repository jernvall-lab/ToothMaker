//
// Computes top cusp angles (cusps A,B,C) using local maxima data of
// triconodont-like tooth objects. As a preprocessing step, averages nearby
// maxima and checks for the cascade rule to extract real cusps.
//
// Writes top_cusp_angles.txt. Overwrites the existing file if present.
//
// Usage: Execute top_cusp_angle in the folder containing local_maxima.txt.
// No arguments.
//

#define _USE_MATH_DEFINES
#include <cmath>
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <sstream>
#include <iterator>

#include "top_cusp_angle.h"

#define INFILE  "local_maxima.txt"      // Input file name
#define OUTFILE "top_cusp_angles.txt"   // Output file name



/**
 *  Reads local maxima data for any number of tooth objects.
 */
int read_local_maxima( std::string infile, std::vector<std::string>& labels,
                       Vector<double>& data )
{
    std::ifstream in(infile);
    if (!in.good()) {
        std::cerr << "Error: Cannot open file '" << infile << "' for reading."
                  << std::endl;
        return EXIT_FAILURE;
    }

    std::vector<std::string> header;
    line_to_vector(in, header);
    if (header.at(0) != "ID" || header.size() != 4) {
        std::cerr << "Error: Cannot recognize input file format." << std::endl;
        in.close();
        return EXIT_FAILURE;
    }

    while (in.good()) {
        std::vector<std::string> line;
        line_to_vector(in, line);
        if (line.size() != 4) continue;
        labels.push_back( line.at(0) );
        data.push_back( {std::stod(line.at(1)), std::stod(line.at(2)), 
                         std::stod(line.at(3))} );
    }

    in.close();
    return EXIT_SUCCESS;
}



/**
 *  Gets unique cusp positions, cusp A position.
 *  - Assumes teeth as essentially 2D, so y-component (depth) can be ignored.
 *  - Cusp data is assumed to be ordered.
 */
int get_individual_cusps( const Vector<double>& data, Vector<double>& cusps )
{
    if (data.size() == 0) return 0;

    // Distance below which two maxima are considered to be part of the same
    // cusp, when ignoring the y component.
    // NOTE: This is not a 'scientific' value. Replace it as needed.
    double cusp_limit = 0.1;

    cusps.push_back( data.at(0) );
    std::vector<double> sum( data.at(0) );  // Accumulator for computing means
    int n = 1;
    for (auto cusp : data) {
        double x = cusp.at(0) - cusps.back().at(0);
        double z = cusp.at(2) - cusps.back().at(2);
        if (x*x + z*z < cusp_limit) {
            sum[0] = sum[0] + cusp.at(0);
            sum[1] = sum[1] + cusp.at(1);
            sum[2] = sum[2] + cusp.at(2);
            n++;
        }
        else {
            // Replace the initial cusp with the mean of joined maxima
            cusps.back() = {sum[0]/n, sum[1]/n, sum[2]/n};
            // Add the new cusp to the array for distance computations
            cusps.push_back( cusp );
            sum = cusp;
            n = 1;
        }
    }

    // Find cusp A (closest to origin).
    int cuspA = 0;
    for (uint16_t i=0; i<cusps.size(); i++) {
        double x1 = cusps.at(i).at(0);
        double y1 = cusps.at(i).at(1);
        double x2 = cusps.at(cuspA).at(0);
        double y2 = cusps.at(cuspA).at(1);
        
        if (x1*x1 + y1*y1 < x2*x2 + y2*y2) {
            cuspA = i;
        }
    }    

    return cuspA;
}



/**
 *  Gets cusps that satisfy the inhibitory cascade rule for cusp heights.
 */
int get_real_cusps( Vector<double>& data, int cuspA )
{
    if ((int)data.size() < cuspA+1) return 0;

    // Check cusps to the left of cusp A
    Vector<double> left_cusps;
    for (uint16_t i=0; i<cuspA; i++) {
        if (left_cusps.size() == 0 || data.at(i).at(2) < left_cusps.back().at(2)) {
            left_cusps.push_back( data.at(i) );
        }
    }

    left_cusps.push_back( data.at(cuspA) );
    int new_cuspA = left_cusps.size()-1;

    Vector<double> right_cusps;
    for (uint16_t i=data.size()-1; i>cuspA; i--) {
        if (right_cusps.size() == 0 || data.at(i).at(2) < right_cusps.back().at(2)) {
            right_cusps.push_back( data.at(i) );
        }
    }
    
    data.clear();
    data.reserve( left_cusps.size()+right_cusps.size() );
    std::copy( left_cusps.begin(), left_cusps.end(), 
               std::back_inserter(data) );
    std::reverse_copy( right_cusps.begin(), right_cusps.end(), 
                       std::back_inserter(data) );

    return new_cuspA;
}



double get_angle( int cuspA, const Vector<double>& data )
{
    // vectors
    auto p1 = data.at(cuspA-1);
    auto p2 = data.at(cuspA);
    auto p3 = data.at(cuspA+1);
    double v1[] = { p1.at(0)-p2.at(0), p1.at(2)-p2.at(2) };
    double v2[] = { p3.at(0)-p2.at(0), p3.at(2)-p2.at(2) };

    // vector norms
    double n1 = std::sqrt( v1[0]*v1[0] + v1[1]*v1[1] );
    double n2 = std::sqrt( v2[0]*v2[0] + v2[1]*v2[1] );

    double sigma = (v1[0]*v2[0] + v1[1]*v2[1]) / (n1*n2);

    return std::acos(sigma);
}



int main()
{
    // Create the output file with header line
    std::ofstream out(OUTFILE);
    if (!out.good()) {
        std::cerr << "Error: Cannot open " << OUTFILE << " for writing."
                  << std::endl;
        return -1;
    }
    out << "ID\tRADIANS\tDEGREES\tNOTES" << std::endl;

    // Read input file
    std::vector<std::string> labels;
    Vector<double> local_maxima;
    if (read_local_maxima( INFILE, labels, local_maxima )) {
        return -1;
    }

    // Get list of unique labels
    std::vector<std::string> ulabels(labels);
    std::sort( ulabels.begin(), ulabels.end() );
    auto last = std::unique( ulabels.begin(), ulabels.end() );
    ulabels.erase( last, ulabels.end() );

    // Process the cusps of each label separately
    for (auto label : ulabels) {
        // Get local maxima
        Vector<double> maxima;
        for (uint16_t i=0; i<labels.size(); i++) {
            if (labels.at(i) == label) {
                maxima.push_back( local_maxima.at(i) );
            }
        }    

        // Get cusps satisfying the cascade rule
        Vector<double> cusps;
        int cuspA = get_individual_cusps( maxima, cusps ); 
        cuspA = get_real_cusps( cusps, cuspA );

        // Compute angle, write output
        if (cuspA < 1 || cuspA > (int)cusps.size()-2) {
            out << label << "\tN/A\tN/A\tMissing B and/or C cusp" << std::endl;
            continue;
        }
        double angle = get_angle( cuspA, cusps );
        out << label << "\t" << angle << "\t" << angle/(2*M_PI)*360
            << "\t" << cusps.size() << " cusps" << std::endl;
    }

    out.close();

    return 0;
}
