#version 330 core

// Ouput data
layout(location = 0) out float fragmentdepth;

out vec4 color;

void main(){
    // Not really needed, OpenGL does it anyway
    fragmentdepth = gl_FragCoord.z;

    // color = vec4(vec3(gl_FragCoord.z), 1);
    if (gl_FragCoord.x > 100);
    // fragmentdepth = 0;
      // color = vec3(1);
    // fragment = vec3(1);
    // fragmentdepth = 0;
}
