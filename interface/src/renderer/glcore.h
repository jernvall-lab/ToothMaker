#pragma once

#if defined(__linux__)
#define GL_GLEXT_PROTOTYPES 1
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glx.h>
#endif

#if defined(__APPLE__)
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
// In OS X we need to use non-core profile (compatibility not available), but
// then the default VAOs don't work, instead need to call *APPLE extensions.
#define glBindVertexArray glBindVertexArrayAPPLE
#define glGenVertexArrays glGenVertexArraysAPPLE
#endif

#include "mesh.h"

// Pan sensitivity. Larger value means less sensitive.
#define PAN_SENSITIVITY 12.5

#define TEXTURES 0x01
#define VERTICES 0x02

#define PAINT_SCREEN 0x01
#define PAINT_FRAMEBUFFER 0x02


struct GLObject {
    GLuint texName;             // Texture object to 2D models (RENDER_PIXEL).
    GLuint framebuffer;         // Off-screen fbo.
    GLuint renderbuffer[2];     // Off-screen rendering buffers.
    GLuint scrfbo;              // Screenshot fbo.
    GLuint scrrender[2];        // Screenshot rendering buffers.
    GLuint vbo;                 // Vertex buffer object (vertex data).
    GLuint cbo;                 // Color buffer object (vertex colors).
    GLuint vao;                 // Vertex array object.
    GLuint ebo_tri;             // Element buffer object (indices; commonly ibo).
    GLuint shader_program;      // Shader program object.

    int renderMode;                         // RENDER_HUMPPA or RENDER_PIXEL.
    int pixelDataHeight, pixelDataWidth;    // Texture dimensions for RENDER_PIXEL.
    float zoomMultip, viewPosY, viewPosX;   // Model zoom, position.
    int startX, startY, deltaX, deltaY;     // Model translation.
    int rtriX, rtriY;                       // Model rotation.
    int mouse1Down, mouse2Down;             // Mouse button 1, 2 states.
    int fbo_dim[2];                         // FBO dimensions (width, height)

    double viewThreshold;                   // State of 'View threshold' (gl_legacy)
    int viewMode;                           // State of 'View mode' in the GUI (gl_legacy)
    int polygonFill;                        // State of 'Show mesh' in the GUI.

    GLfloat *img;                           // Data (texture) for RENDER_PIXEL.
    GLubyte *scrimg;                        // Buffer for storing the screenshot.
    Mesh* mesh;                             // 3D model mesh.
    std::vector<std::vector<float>>* cell_data; // Morphogen concentrations (gl_legacy)
};



namespace glcore {

int initGLObject(GLObject& obj);

void gl_error( std::string file, int line );
#define check_gl_error() gl_error(__FILE__,__LINE__)

void uploadData(GLObject& obj, int datatype);

void paintGL(GLObject& obj, int type);

void resizeGL(GLObject& obj, int w, int h);

void setVisualData( std::vector<std::vector<float>>*, GLObject& obj, Mesh* mesh );

void setVisualData2D(int height, int width, GLObject& obj);

int createGLContext_OSMesa();

int createGLContext();

int initializeGL(GLObject& obj, std::string);

void setRenderMode(int mode, GLObject& obj);

void setImageSize(int n, GLObject& obj);

void screenshotGL(GLObject& obj, int w, int h);

}

