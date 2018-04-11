#pragma once

#include <string>
#include "mesh.h"
#include "glcore.h"


namespace glcore {

std::string Read_shader_file( std::string );

void Shader_log( const std::string&, GLuint& );

void Set_color_data( Mesh*, std::vector<mesh::vertex_color>& );

void Set_vertex_data( Mesh*, std::vector<GLfloat>&, std::vector<GLuint>& );

void Draw_mesh( GLObject&, int, int );

}

