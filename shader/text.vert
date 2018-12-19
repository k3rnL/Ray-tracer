#version 330 core


layout(location = 0) in vec3 vertex;
layout(location = 1) in vec2 uv_buffer;

out vec2 uv;

void main(){

    gl_Position =  vec4(vertex * vec3(1,-1,1), 1);
	uv = uv_buffer;
}
