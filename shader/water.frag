/**
 * @Author: daniel_b
 * @Date:   2017-07-27T03:32:34+02:00
 * @Last modified by:   daniel_b
 * @Last modified time: 2017-07-30T20:32:03+02:00
 */



#version 330 core

#define M_PI 3.1415926535897932384626433832795

in vec3 to_shade_vertex;

uniform vec3 light_pos;

in mat4 model_view; // Camera view

struct MaterialData
{
              vec3  diffuse_color;
              int   diffuse_map;
};

uniform MaterialData mt_data;
uniform float time;

uniform sampler2D diffuse_map;

out vec3 color;
in vec3 normal_input;
in vec3 vector_to_camera;
in vec2 uv;

vec3 textureToNormal(vec4 orgNormalColor);

void main(){

  vec3 normal = normal_input ;

  // normal.xy = vec2(cos(time) * 20, sin(time) * 20);
  // normal.xz = vec2(cos(to_shade_vertex.x * 20 + time), sin(to_shade_vertex.z * 20 + time));
  // normal.x = cos(to_shade_vertex.x * 50 + time);

  vec3 light_vector = normalize(light_pos - to_shade_vertex);


  vec3 albedo;
  if (mt_data.diffuse_map == 0)
  {
    // albedo = mt_data.diffuse_color * angle;
    albedo = mt_data.diffuse_color;
  }
  else
  {
    albedo = vec3(texture(diffuse_map, uv * 10));
    normal = textureToNormal(vec4(albedo, 1));
    float tmp = normal.y;
    normal.y = normal.z;
    normal.z = tmp;
  }

  float angle = clamp(dot(light_vector, normalize(normal)), 0.0, 1.0);
  // float angle = clamp(dot((rot * vec4(light_vector, 0)).xyz, normalize(normal)), 0.0, 1.0);

  color = albedo * angle;
  return;
}

vec3 textureToNormal(vec4 orgNormalColor)
{
	return normalize(vec3(clamp(orgNormalColor.r*2.0 - 1.0, -1.0, 1.0),
                        clamp(orgNormalColor.g*2.0 - 1.0, -1.0, 1.0),
                        clamp(orgNormalColor.b*2.0 - 1.0, -1.0, 1.0)));
}
