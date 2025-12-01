#version 420
// Adapted from: https://learnopengl.com/PBR/Lighting

#include "pbr_shared.glsl"

out vec4 outputColor;
in vec2 TexCoords;
in mat3 TBN;

layout(binding=0) uniform sampler2D albedoTex;
layout(binding=1) uniform sampler2D alphaMap;
layout(binding=2) uniform sampler2D normalMap;

void main()
{
    float alpha = texture(alphaMap,TexCoords).x;
	if(alpha < 0.5) { 
		discard;
	}

    vec3 tangentNormal = texture(normalMap,TexCoords).xyz * 2.0 - 1.0;
    vec3 N = normalize(TBN * tangentNormal);
    vec3 V = normalize(viewPos - WorldPos);

    vec3 albedo = matAlbedo * texture(albedoTex,TexCoords).xyz;

    vec3 color = CalculateLighting(N,albedo);
    
    outputColor = vec4(ToneMap(color),1.0);
}