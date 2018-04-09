/**
 * Removes lines consisting of only whitepaces, tabs from a text file. 
 * Adds a new line at the end of the last line if one didn't exist.
 *
 */
#include <iostream>
#include <fstream>
#include <string>


/**
 * @brief Removes empty lines from a text file.
 * @param input     Input text file name.
 * @param output    Output text file name.
 * @return          0 if ok, else -1.
 */
int no_empty_lines( const std::string& input, const std::string& output )
{
    std::ofstream out(output);
    if (!out.good()) {
        std::cerr << "Error: Cannot open file '" << output << "' for writing." 
                  << std::endl;
        return -1;
    }

    std::ifstream in(input);
    std::string line;

    while (in.good()) {
        std::getline(in, line);
        if (line.find_first_not_of(" \t\n") == std::string::npos) {
            continue;
        }
        out << line << std::endl;
    }

    in.close();
    out.close();

    return 0;
}



int main( int argc, char* argv[] )
{
    if (argc < 3) {
        std::cout << "Usage: no_empty_lines [input] [output]" << std::endl;
        return 0;
    }

    if (no_empty_lines( argv[1], argv[2] )) {
        return -1;
    }

    return 0;
}

