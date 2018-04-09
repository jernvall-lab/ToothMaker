#ifndef DAD_TO_POLYGONS_H
#define DAD_TO_POLYGONS_H

/**
 *  Container for vertex data, neighbour lists.
 */
template <typename T>
using Vector = std::vector<std::vector<T>>;


/**
 *  Reads a line from input stream, splits into a vector.
 */
template <typename T>
void line_to_vector( std::ifstream& in, std::vector<T>& data ) 
{
    in >> std::ws;
    std::string line;
    std::getline(in, line);
    std::stringstream ss(line);

    T val;
    while (ss >> val)
        data.push_back(val);
}


/**
 *  Returns set difference of two vectors.
 */
template <typename T>
std::vector<T> set_diff( std::vector<T> set1, std::vector<T> set2 )
{
    std::vector<T> diff;
    std::set_difference( set1.begin(), set1.end(), set2.begin(), set2.end(),
                         std::inserter(diff, diff.begin()) );
    return diff;
}


/**
 *  Returns set intersection of two vector.
 */
template <typename T>
std::vector<T> set_intersect( std::vector<T> set1, std::vector<T> set2 )
{
    std::sort( set1.begin(), set1.end() );
    std::sort( set2.begin(), set2.end() );
    std::vector<T> intersect;
    std::set_intersection( set1.begin(), set1.end(), set2.begin(), set2.end(),
                           std::inserter(intersect, intersect.begin()) );
    return intersect;
}

#endif

