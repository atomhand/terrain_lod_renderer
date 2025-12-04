// from https://learnopengl.com/PBR/IBL/Diffuse-irradiance

#version 420
out vec4 FragColor;
in vec3 SamplePos;
in vec4 ClipPos;

uniform samplerCube environmentMap;

uniform mat4 projection;
uniform mat4 view;

void main()
{
    // sample environment map (skybox)
    vec3 envColor = texture(environmentMap, SamplePos).rgb;
    
    vec4 worldPos = inverse(projection * view) * ClipPos;
    worldPos/= worldPos.w;
    vec3  fogColor  = vec3(0.5,0.6,0.7);
    float threshold = 128.0;
    //envColor = mix(fogColor,envColor, clamp((worldPos.y-threshold)/threshold,0.0,1.0));
    
    // HDR tonemap and gamma correct
    envColor = envColor / (envColor + vec3(1.0));
    envColor = pow(envColor, vec3(1.0/2.2)); 
    
    FragColor = vec4(envColor, 1.0);
}