#version 330 core

uniform sampler2D image;

uniform vec3 color_in;

in vec2 uv;

out vec3 color;
void main(){
  color = texture(image, (uv+1.0)/2).rgb;
}
