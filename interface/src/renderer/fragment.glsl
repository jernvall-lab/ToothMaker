#version 120

// Fragment shader.
// Dim ambient white light + diffuse white light source at +z infinity.
// Draws either filled polygons or wireframe only.

uniform bool wireframe;             // Set 'true' to draw wireframe only.
uniform vec3 edge_color;            // Wireframe color.
varying vec3 frag_vertex;
varying vec3 frag_normal;
varying vec4 frag_color;
uniform mat4 model;                 // Model matrix.
uniform mat3 normal_matrix;         // Inverse transpose of the model matrix (3x3 sub).

void main() 
{
    const vec4 ambient_color = vec4(0.5, 0.5, 0.5, 1.0);
    const vec4 diffuse_color = vec4(1.0, 1.0, 1.0, 1.0);
    const vec3 light_position = vec3(0.0, 0.0, 1.0);

    vec3 normal = normalize(normal_matrix * frag_normal);
    vec3 frag_position = vec3(model * vec4(frag_vertex, 1));
    float brightness = dot(normal, light_position) / (length(normal) * length(light_position));
    brightness = 0.5 * clamp(brightness, 0.0, 1.0);
   
    if (wireframe == true) {
        gl_FragColor = vec4(edge_color, 1.0);
    }
    else {
        gl_FragColor = (ambient_color + diffuse_color * brightness) * frag_color;
    }
}

