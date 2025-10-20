// from https://learnopengl.com/PBR/IBL/Diffuse-irradiance

#version 420
out vec4 FragColor;
in vec3 SamplePos;
in vec4 ClipPos;

uniform samplerCube environmentMap;

#include "shared/uniforms_shared.glsl"

void main()
{
    // sample environment map (skybox)
    vec3 envColor = texture(environmentMap, SamplePos).rgb;    
    FragColor = vec4(envColor, 1.0);
}