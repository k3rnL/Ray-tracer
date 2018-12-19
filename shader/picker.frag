#version 330 core

uniform uint id;

layout(location = 0) out uvec4 out_color;

void main()
{
float a = id;
	uvec4 color;
	color.x = (id & 0xff0000u) >> 16;
	color.y = (id & 0x00ff00u) >>  8;
	color.z = (id & 0x0000ffu) >>  0;
	color.w = uint(255);

	out_color = color;
	return;
}
