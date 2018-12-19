#version 330 core

uniform vec4 color;
uniform vec4 border_color;
uniform int corner;
uniform int border;
uniform vec2 dimension;

out vec4 color_out;
in vec2 varying_uv;

// return quentity of color (alpha) to made a rounded rectangle at pos
float roundedFrame (vec2 pos,float radius)
{
  pos = abs(pos);
  vec2 corner_center = abs(dimension - radius) - dimension / 2;
  if (pos.x < corner_center.x || pos.y < corner_center.y)
    return (1.);

  float value = length(distance(pos, corner_center));

  return (smoothstep(value+0.75, value+0.75, radius));
}

float 	roundedBorder(vec2 pos, float size, float radius)
{
  pos = abs(pos);
  vec2 limit = dimension/2 - size;
  vec2 corner_center = abs(dimension - radius) - dimension / 2;
  if (pos.x > corner_center.x && pos.y > corner_center.y)
  {
	float dist = distance(pos,corner_center);
	if (dist < radius - size || dist > radius)
		return 0.;
	return(smoothstep(dist, dist+0.75, radius));
	return(smoothstep(dist, dist+10.75, radius));
  }
   
  if (pos.x < limit.x && pos.y < limit.y)
	return (0.);



  return 1.;
  //return (smoothstep(value-0.75, value, radius));
}

void main(){
  color_out = color;
  vec2 uv = varying_uv;
  vec2 centeredPos = (uv-0.5) * dimension;

  if (corner > 0) {
	float value = roundedFrame(centeredPos, corner);
	color_out = vec4(color.xyz, value+0);
	float border = roundedBorder(centeredPos, 1., corner);
 }
  if (border > 0) {
	float value = roundedBorder(centeredPos, border, corner);
	if (value >  0)
		color_out = vec4(border_color.xyz, value);
  }
}
