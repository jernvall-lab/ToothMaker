#version 120

// Vertex shader.
// Passes vertices, colors and normals to the fragment shaders.

attribute vec3 vertex;
attribute vec4 color;
attribute vec3 normal;

uniform mat4 model;
uniform mat4 camera;

varying vec3 frag_vertex;
varying vec3 frag_normal;
varying vec4 frag_color;

void main() {
    frag_vertex = vertex;
    frag_color = color;
    frag_normal = normal;
    
    gl_Position = camera * model * vec4(vertex, 1.0);
}

