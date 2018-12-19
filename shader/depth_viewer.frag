#version 330 core

struct MaterialData
{
              vec3  diffuse_color;
              int   diffuse_map;
};

uniform MaterialData mt_data;
uniform sampler2DShadow diffuse_map;

in vec2 uv;

out vec3 color;

void main() {
  vec3 albedo;
  if (mt_data.diffuse_map == 0)
  {
    // albedo = mt_data.diffuse_color * angle;
    albedo = mt_data.diffuse_color;
  }
  else
  {
    // albedo = vec3(texture(diffuse_map, uv)) * angle;
    albedo = vec3(texture(diffuse_map, vec3(uv, gl_FragCoord.z/gl_FragCoord.w)));
  }
  color = albedo;
}
