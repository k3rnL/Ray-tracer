/**
 * @Author: daniel_b
 * @Date:   2017-07-27T03:32:34+02:00
 * @Last modified by:   daniel_b
 * @Last modified time: 2017-07-30T20:32:03+02:00
 */

 #version 330 core

 #define M_PI 3.1415926535897932384626433832795


 uniform vec3 light_pos;
 uniform mat4 model_view;
 uniform mat4 view;

 struct MaterialData
 {
               vec3  diffuse_color;
               int   diffuse_map;
               int   normal_map;
               int   repeat;
 };

 uniform MaterialData mt_data;

 uniform sampler2D diffuse_map;
 uniform sampler2D normal_map;
 uniform sampler2DShadow shadow_map;

 out vec3 color;
 in vec3 normal_input;
 in vec3 vector_to_camera;
 in vec3 to_shade_vertex;
 in vec4 shadow_coord;
 in vec2 uv;

 // in vec3 light_vector_tangent;

 float DistributionGGX(vec3 N, vec3 H, float roughness);
 float GeometrySchlickGGX(float NdotV, float roughness);
 float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness);
 vec3 fresnelSchlick(float cosTheta, vec3 F0);

 in vec3 normal_cameraspace;
 in vec3 tangent_cameraspace;
 in vec3 bitangent_cameraspace;

 in vec3 tangent_;

 vec3 textureToNormal(vec4 orgNormalColor)
 {
 	return normalize(vec3(clamp(orgNormalColor.r*2.0 - 1.0, -1.0, 1.0),
                         256,
                         clamp(orgNormalColor.b*2.0 - 1.0, -1.0, 1.0)));
 }

 void main(){
   vec3 normal = normal_input;
   vec3 light_vector = normalize(light_pos - to_shade_vertex);
   vec3 pouet;
   mat3 mat_to_tangent = transpose(mat3(tangent_cameraspace,bitangent_cameraspace,normal_cameraspace));
   if (mt_data.normal_map == 1) {
     normal = texture(normal_map, uv * mt_data.repeat).rgb;
     normal.xy = normal.xy * 2.0 - 1.0;
   }
   else if (mt_data.diffuse_map == 1) {
     normal = mat_to_tangent * textureToNormal(texture(diffuse_map, uv * mt_data.repeat));
   }
   vec3 vertex_cameraspace = (model_view * vec4(to_shade_vertex, 1)).xyz;
   light_vector = normalize(((model_view)*vec4(light_pos,1)).xyz - vertex_cameraspace);
   light_vector = mat_to_tangent * light_vector;
   light_vector = normalize(light_vector);

   float angle = dot(light_vector, normalize(normal));// * 0.8 * normalize(-0.5+dot(light_pos - to_shade_vertex, light_pos - to_shade_vertex));

   vec3 albedo;
   if (mt_data.diffuse_map == 0)
     albedo = mt_data.diffuse_color;
   else
     albedo = vec3(texture(diffuse_map, uv * mt_data.repeat));

   vec4 shadow = shadow_coord;
   shadow.xy = (shadow_coord.xy / shadow_coord.w) / 2 + 0.5;
   color = (albedo * 0.1) + albedo * angle;

    if (texture(shadow_map, vec3(shadow_coord.xy/shadow_coord.w, shadow.z/shadow.w)) < (shadow_coord.z + 0.005)/shadow_coord.w) {
     color = albedo * 0.1;
   }
   return;
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
