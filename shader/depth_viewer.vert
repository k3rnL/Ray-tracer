#version 330 core

layout(location = 0) in vec3 vertex;
layout(location = 2) in vec2 vertex_uv;

uniform mat4 projection; // Projected view
uniform mat4 view;
// layout(location = 12) in mat4 model_view; // Camera view
uniform mat4 model_view;


out vec2 uv;

void main() {
  gl_Position =  projection * view * model_view * vec4(vertex, 1);
  uv = vertex_uv;
}
