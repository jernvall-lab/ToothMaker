#pragma once

#include <string>

namespace colormap {

struct Color {
    int r,g,b;
};

int Map_value( double, double, Color*, std::string type );

}
