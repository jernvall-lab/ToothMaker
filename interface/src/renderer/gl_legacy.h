#pragma once

#include "glcore.h"

// Default tooth color for Humppa. 0.5 is middle gray.
#define DEFAULT_TOOTH_COL 0.5

namespace glcore {

void Vertex_color_RENDER_HUMPPA( int, GLObject& );

void PaintGL_RENDER_HUMPPA( GLObject&, int, int );

void PaintGL_2D( GLObject&, GLdouble );

}
