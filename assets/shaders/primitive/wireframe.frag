#version 460

#include "shared/uniforms_shared.glsl"

in vec3 fColor;
in vec3 wireframeDist;
out vec4 outputColor;

void main()
{
    if(debugWireframe > 0.f) {        
        vec3 d = fwidth(wireframeDist); 
        vec3 a3 = smoothstep(vec3(0.0), d * 1.f, wireframeDist);
        float edgeFactor = min(min(a3.x, a3.y), a3.z);

        outputColor = vec4(mix(vec3(1.0), vec3(0.0), edgeFactor), 1.0);
    } else {
        outputColor = vec4(fColor,1.0);
    }
}