#version 330 core


layout(location = 0) in vec3 vertex;
layout(location = 1) in vec2 uv_buffer;

uniform vec2 dimension;

out vec2 varying_uv;

void main(){

    gl_Position =  vec4(vertex * vec3(1, -1, 1), 1);
    varying_uv = uv_buffer;

}
