#version 330 core

uniform vec4 color;
uniform sampler2D font;
out vec4 color_out;

in vec2 uv;

void main(){
  color_out = vec4(1, 1, 1, texture2D(font, uv).r) * color;
}
