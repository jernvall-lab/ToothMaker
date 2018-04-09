/**
 *  @file gl_modern.cpp
 *  @brief Functionality for drawing 3D meshes.
 *
 *  Uses VBOs & programmable pipeline, with all features compatible with core
 *  OpenGL 3/4, except for shaders (v120) which still require compatibility
 *  profile.
 */

#include <iostream>
#include <fstream>
#include <string>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "glcore.h"             // PAN_SENSITIVITY
#include "gl_modern.h"
#include "morphomaker.h"        // SQUARE_WIN_SIZE



/**
 * @brief Reads shaders from files.
 * @param file      Full file name with path.
 * @return          Shader as a string, or an empty string if could not read.
 */
std::string glcore::Read_shader_file(std::string file)
{
    std::ifstream fstream(file, std::ios::in);
    if (!fstream.is_open()) {
        std::cerr << "Error: Cannot read file " << file << std::endl;
        return "";
    }

    std::string line, data = "";
    while (!fstream.eof()) {
        std::getline(fstream, line);
        data.append(line + "\n");
    }
    fstream.close();

    return data;
}



/**
 * @brief Writes shader compiler output to stdout.
 * @param name      Shader name.
 * @param shader    Shader object.
 */
void glcore::Shader_log( const std::string& name, GLuint& shader )
{
    GLint status;
    glGetShaderiv( shader, GL_COMPILE_STATUS, &status );
    GLint log_size;
    glGetShaderiv( shader, GL_INFO_LOG_LENGTH, &log_size );
    GLchar* log = new GLchar[log_size + 1];
    strcpy(log, "");
    glGetShaderInfoLog( shader, log_size, NULL, log );

    if (status == true) {
        std::cout << name << " shader compile success." << std::endl;
    }
    else {
        std::cerr << name << " shader compile failure." << std::endl;
    }
    if (strlen(log) > 0) {
        std::cout << std::endl << "Log: " << log << std::endl;
    }
}



/**
 * @brief Construct vertex color array.
 *        The array structure corresponds to the vertex data returned by
 *        _set_vertex_data().
 * @param mesh
 * @param tri_color_data
 */
void glcore::Set_color_data( Mesh* mesh, std::vector<vertex_color>& tri_color_data )
{
    auto& colors = mesh->get_vertex_colors();
    auto& tris = mesh->get_triangle_indices();

    tri_color_data.reserve( tris.size()*3 );

    for ( uint32_t i=0; i<tris.size(); i=i+3 ) {
        uint32_t nodes[] = { tris.at(i), tris.at(i+1), tris.at(i+2) };

        for ( uint8_t j=0; j<3; j++ ) {
            tri_color_data.push_back( colors.at(nodes[j]) );
        }
    }
}



/**
 * @brief Computes face normals and assigns them to the vertices of the triangles.
 *        The constructed data is intended for flat shading.
 * @param mesh          Mesh object.
 * @param tri_data      Array for storing triangle vertices and normals.
 * @param tri_indices   Array for storing triangle vertex indices.
 */
void glcore::Set_vertex_data( Mesh* mesh, std::vector<GLfloat>& tri_data,
                              std::vector<GLuint>& tri_indices )
{
    // To get flat shading we need to send each vertex multiple times to the GPU:
    // Once for each polygon it is associated with.
    // Face normals are also assigned separately for each of the face vertices.

    auto& tris = mesh->get_triangle_indices();
    auto& vertices = mesh->get_vertices();

    tri_data.reserve( tris.size()*3*6 );
    tri_indices.reserve( tris.size()*3 );

    for ( uint32_t i=0; i<tris.size(); i=i+3 ) {
        uint32_t nodes[] = { tris.at(i), tris.at(i+1), tris.at(i+2) };

        vertex p[] = { vertices.at(nodes[0]),
                       vertices.at(nodes[1]),
                       vertices.at(nodes[2]) };
        vertex v1 = p[0]-p[2];
        vertex v2 = p[1]-p[2];

        // Compute normal from vectors v1, v2 (cross product).
        vertex n;
        n.x = v1.y*v2.z - v1.z*v2.y;
        n.y = v1.z*v2.x - v1.x*v2.z;
        n.z = v1.x*v2.y - v1.y*v2.x;

        for ( uint8_t j=0; j<3; j++ ) {
            tri_data.insert( tri_data.end(),
                             { p[j].x, p[j].y, p[j].z, n.x, n.y, n.z } );
            tri_indices.push_back(i+j);
        }
    }
}



/**
 * @brief Draws 3D mesh upload with glcore::upload_data().
 * @param obj       GLObject.
 * @param x         Draw area width in pixels.
 * @param y         Draw area height in pixels.
 */
void glcore::Draw_mesh( GLObject *obj, int x, int y )
{
    if ( obj->mesh == NULL ) return;

    GLfloat aspect = ((GLfloat)x/y);

    // Camera view.
    GLfloat zoom[] = { -20.0f*aspect*obj->zoomMultip, 20.0f*aspect*obj->zoomMultip,
                       -20.0f*obj->zoomMultip, 20.0f*obj->zoomMultip };
    glm::mat4 camera = glm::ortho( zoom[0], zoom[1],            // left, right
                                   zoom[2], zoom[3],            // bottom, top
                                   -200.0f, 200.0f );           // zNear, zFar
    // Camera position, direction & orientation.
    glm::mat4 view = glm::lookAt( glm::vec3(0.0f, 0.0f, 0.0f),
                                  glm::vec3(0.0f, 0.0f, 1.0f),
                                  glm::vec3(0.0f, -1.0f, 0.0f) );
    camera = camera*view;

    // Object scaling.
    glm::mat4 scale;
    // scale = glm::scale(scale, glm::vec3( 1.0/obj->zoomMultip ));

    // Object translation.
    if ( obj->mouse2Down ) {
        GLdouble sensitivity = obj->zoomMultip * SQUARE_WIN_SIZE /
                               (PAN_SENSITIVITY * (GLdouble)y);
        obj->viewPosY -= obj->deltaX * sensitivity;
        obj->viewPosX -= obj->deltaY * sensitivity;
    }
    glm::mat4 translate;
    translate = glm::translate( translate,
                                glm::vec3(obj->viewPosY, obj->viewPosX, 0.0) );

    // Object rotation.
    if ( obj->mouse1Down ) {
        obj->rtriY -= obj->deltaY;
        obj->rtriX += obj->deltaX;
    }
    glm::mat4 rotate;
    rotate = glm::rotate( rotate, glm::radians((GLfloat)obj->rtriY),
                          glm::vec3(1.0f, 0.0f, 0.0f) );
    rotate = glm::rotate( rotate, glm::radians((GLfloat)obj->rtriX),
                          glm::vec3(0.0f, 0.0f, 1.0f) );

    // Model view matrix.
    glm::mat4 model = translate*rotate*scale;

    // Send camera & model to shaders.
    GLint loc = glGetUniformLocation( obj->shader_program, "camera" );
    glUniformMatrix4fv( loc, 1, GL_FALSE, glm::value_ptr(camera) );
    loc = glGetUniformLocation( obj->shader_program, "model" );
    glUniformMatrix4fv( loc, 1, GL_FALSE, glm::value_ptr(model) );

    // Compute normal matrix. In GLSL >= 1.3 this can be done in shader,
    // but GLSL 1.2 doesn't offer inverse() nor transpose() methods.
    glm::mat3 normal_matrix = glm::inverseTranspose( glm::mat3(model) );
    loc = glGetUniformLocation( obj->shader_program, "normal_matrix" );
    glUniformMatrix3fv( loc, 1, GL_FALSE, glm::value_ptr(normal_matrix) );

    // Draw filled polygons.
    glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
    GLint wireframe = glGetUniformLocation( obj->shader_program, "wireframe" );
    glUniform1f(wireframe, false);

    // Draw triangles.
    auto& tris = obj->mesh->get_triangle_indices();
    glBindVertexArray(obj->vao);
    glDrawElements( GL_TRIANGLES, tris.size(), GL_UNSIGNED_INT, 0 );

    // Draw polygon edges in black if requested.
    if ( obj->polygonFill ) {
        glUniform1f(wireframe, true);
        GLint uniColor = glGetUniformLocation( obj->shader_program, "edge_color" );
        glUniform3f( uniColor, 0.0f, 0.0f, 0.0f );
        glLineWidth(1);
        glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );

        // Triangle edges.
        glBindVertexArray(obj->vao);
        glDrawElements( GL_TRIANGLES, tris.size(), GL_UNSIGNED_INT, 0 );
    }
}
