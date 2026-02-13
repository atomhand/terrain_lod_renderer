// Tom Kellett 2025
#version 460


#include "shared/uniforms_shared.glsl"
#include "shared/deferred_shared.glsl"
#include "shared/coordinate_shared.glsl"
//in vec2 TexCoords;

in vec3 WorldPos;
in vec4 ClipPos;

// material parameters
uniform vec3 mAlbedo;
uniform float mMetallic;
uniform float mRoughness;
uniform float mAo;

layout(binding=0) uniform sampler2D depthBuffer;
layout(binding=1) uniform sampler2D normalMap1;
layout(binding=2) uniform sampler2D normalMap2;

void main()
{
    // For a wave/ripple effect I combine 2 scrolling normal maps
    vec2 uv = WorldPos.xz / 256.0;
    float anim = time * 0.2;
    vec3 n1 = texture(normalMap1,uv + vec2(1,1)*anim).xzy * 2.0 - 1.0;
    vec3 n2 = texture(normalMap2,uv + vec2(-1,1)*anim).xzy * 2.0 - 1.0;

    vec3 N = normalize(n1 + n2);//);

    vec3 screenUv = NdcToUv(ClipPos.xyz/ClipPos.w);

    float surfaceDepth = ClipPos.w;

    float texDepth = texture(depthBuffer, screenUv.xy).x;
    float seafloorDepth = LinearizeDepth(texture(depthBuffer, screenUv.xy).x);

    // Ref https://iquilezles.org/articles/fog/
    float b = 0.001;
    float w = min(1,1.0 - exp((surfaceDepth-seafloorDepth)*b));
    // hardcoded material params for now    
    vec3 albedo = mix(1.0,0.25,w)*vec3(0.465f, 0.797f, 0.991f);
    float ao = 1.0;// - d * 100.0;
    float roughness = 0.03f;
    float metallic = 0.f;

    WriteGBuffer(WorldPos,N,albedo,ao,roughness,metallic);
}