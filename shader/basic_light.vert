#version 330 core


layout(location = 0) in vec3 vertex;
layout(location = 1) in vec3 vertex_normal;
layout(location = 2) in vec2 vertex_uv;
layout(location = 3) in vec3 tangent;
layout(location = 4) in vec3 bitangent;

// Values that stay constant for the whole mesh.
uniform mat4 light_mvp;
uniform mat4 projection; // Projected view
uniform mat4 view;
// layout(location = 12) in mat4 model_view; // Camera view
uniform mat4 model_view;
uniform vec3 camera_position;

uniform vec3 light_pos;

out vec3 to_shade_vertex;
out vec3 normal_input;
out vec3 vector_to_camera;
out vec2 uv;
out vec4 shadow_coord;

out vec3 light_vector_tangent;

out vec3 normal_cameraspace;
out vec3 tangent_cameraspace;
out vec3 bitangent_cameraspace;

out vec3 tangent_;

void main(){
  tangent_ = tangent;
    // compute true position of actual vertex and pass it to fragment shader
    to_shade_vertex = (model_view * vec4(vertex, 1)).xyz;
    normal_input = mat3(transpose(inverse(model_view))) * vertex_normal;

    vector_to_camera = camera_position - to_shade_vertex;
    // vector_to_camera = to_shade_vertex - camera_position;
    vector_to_camera = normalize(vector_to_camera);

    // Output position of the vertex, in clip space : MVP * position
    gl_Position =  projection * view * model_view * vec4(vertex, 1);
    shadow_coord = light_mvp * model_view * vec4(vertex, 1);

    uv = vertex_uv;

    // normal_cameraspace = mat3(view * model_view)  * vec3(0,1,0);
    normal_cameraspace = normalize(mat3(model_view) * vertex_normal);
    tangent_cameraspace = normalize(mat3(model_view) * tangent);
    bitangent_cameraspace = normalize(mat3(model_view) * bitangent);

    mat3 mat_to_tangent = transpose(mat3(tangent_cameraspace,bitangent_cameraspace,normal_cameraspace));
    light_vector_tangent = mat_to_tangent * normalize(light_pos - to_shade_vertex);
}
