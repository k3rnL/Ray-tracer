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

float DistributionGGX(vec3 N, vec3 H, float roughness);
float GeometrySchlickGGX(float NdotV, float roughness);
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness);
vec3 fresnelSchlick(float cosTheta, vec3 F0);

void main(){

  // vec3 normal = mat3(model_view) * normal_input;
  vec3 normal = normal_input;

  // normal.xy = vec2(cos(time) * 20, sin(time) * 20);
  // normal.xz = vec2(cos(to_shade_vertex.x * 20 + time), sin(to_shade_vertex.z * 20 + time));
  normal.x = cos(to_shade_vertex.x * 30 + time);

  vec3 light_vector = normalize(light_pos - to_shade_vertex);

  float angle = clamp(dot(light_vector, normalize(normal)), 0.0, 1.0);
  // float angle = clamp(dot((rot * vec4(light_vector, 0)).xyz, normalize(normal)), 0.0, 1.0);

  vec3 albedo;
  if (mt_data.diffuse_map == 0)
  {
    // albedo = mt_data.diffuse_color * angle;
    albedo = mt_data.diffuse_color;
  }
  else
  {
    // albedo = vec3(texture(diffuse_map, uv)) * angle;
    // uv = uv * (to_shade_vertex.z / 100);
    albedo = vec3(texture(diffuse_map, (uv * 10.0)));
    vec3 snow = vec3(1.0);
    snow = snow * (to_shade_vertex.y / 1.0);
    snow = clamp(snow, 0, 1);
    albedo += snow;
  }

  color = albedo * angle;
  return;
  vec3 H = normalize(vector_to_camera + light_vector);

  float roughness = 5.5;
  float metallic = 5.5;
  float ao = 0.1;
  vec3 F0 = vec3(0.04);
  F0 = mix(F0, albedo, metallic);
  vec3 Lo = vec3(0.0);

  float distance = length(light_pos - to_shade_vertex);
  float attenuation = 1.0 / (distance * distance);
  // vec3 radiance = vec3(1.0);
  vec3 radiance = vec3(1.0) * attenuation;

  float NDF = DistributionGGX(normal, H, roughness);
  float G   = GeometrySmith(normal, vector_to_camera, light_vector, roughness);
  vec3 F    = fresnelSchlick(max(dot(H, vector_to_camera), 0.0), F0);

  vec3 kS = F;
  vec3 kD = vec3(1.0) - kS;
  kD *= 1.0 - metallic;

  vec3 nominator    = NDF * G * F;
  float denominator = 4 * max(dot(normal, vector_to_camera), 0.0) * max(dot(normal, light_vector), 0.0) + 0.001;
  vec3 specular     = nominator / denominator;

  // add to outgoing radiance Lo
  float NdotL = max(dot(normal, light_vector), 0.0);
  Lo += (kD * albedo / M_PI + specular) * radiance * NdotL;

  vec3 ambient = vec3(0.03) * albedo * ao;
  vec3 color_tmp = ambient + Lo;

  color_tmp = color_tmp / (color_tmp + vec3(1.0));
  color_tmp = pow(color_tmp, vec3(1.0/2.2));

  color = color_tmp;
}


float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = M_PI * denom * denom;

    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}
