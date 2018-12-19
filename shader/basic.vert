#version 330 core


layout(location = 0) in vec3 vertex;


out vec2 uv;

void main(){

    // Output position of the vertex, in clip space : MVP * position
    gl_Position =  vec4(vertex, 1);
	uv = vertex.xy;
}
