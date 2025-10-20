#ifndef PBR_SHARED_GLSL
#define PBR_SHARED_GLSL

// Adapted from: https://learnopengl.com/PBR/Lighting
// https://learnopengl.com/PBR/IBL/Diffuse-irradiance
// https://learnopengl.com/PBR/IBL/Specular-IBL

// Most of the contents of this file are copied directly from LearnOpengl
// The only modifications are integrating it with my lighting/occlusion functions and material parameters

#include "shared/coordinate_shared.glsl"
#include "shared/shadow_shared.glsl"
#include "shared/volume_shared.glsl"
#include "shared/uniforms_shared.glsl"

// textures
layout(binding=6) uniform samplerCube irradianceMap;
layout(binding=7) uniform samplerCube prefilterMap;
layout(binding=8) uniform sampler2D brdfLUT;

const float PI = 3.14159265359;

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
	
    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return num / denom;
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
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 outRadiance(vec3 L, vec3 V, vec3 N, vec3 F0, vec3 albedo, vec3 radiance, float roughness, float metallic) {
    vec3 H = normalize(V + L);
    
    // cook-torrance brdf
    float NDF = DistributionGGX(N, H, roughness);        
    float G   = GeometrySmith(N, V, L, roughness);      
    vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);       
    
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;	  
    
    vec3 numerator    = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular     = numerator / denominator;  
        
    // add to outgoing radiance Lo
    float NdotL = max(dot(N, L), 0.0);                
    return (kD * albedo / PI + specular) * radiance * NdotL; 
}

vec3 DirectLightContribution(vec3 worldPos, vec3 N, vec3 V, vec3 F0, vec3 albedo, float roughness, float metallic) {
    vec3 Lo = vec3(0.0);

    // directional light
    {
        vec3 L = normalize(-lightDirection.xyz);
        vec3 inRadiance = lightColor.rgb * ShadowAtSurface(L,worldPos) * volumetricShadow(worldPos,worldPos-lightDirection.xyz*512.0);
        Lo += outRadiance(L,V,N,F0,albedo,inRadiance,roughness,metallic);
    }
    return Lo;
}

vec3 IBL(vec3 N, vec3 V, vec3 R, vec3 F0, vec3 albedo, float ao, float roughness, float metallic) {
    vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);

    vec3 kS = F;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;

    vec3 irradiance = texture(irradianceMap, N).rgb;
    vec3 diffuse    = irradiance * albedo;

    // Specular IBL
    const float MAX_REFLECTION_LOD = 4.0;
    vec3 prefilteredColor = textureLod(prefilterMap, R,  roughness * MAX_REFLECTION_LOD).rgb;    
    vec2 brdf  = texture(brdfLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
    vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

    vec3 ambient = (kD * diffuse + specular) * ao;
    return ambient;
}

vec3 CalculateLighting(vec3 worldPos, vec3 N, vec3 albedo, float ao, float roughness, float metallic) {
    vec3 V = normalize(viewPos.xyz - worldPos);
    vec3 R = reflect(-V, N);

    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);

    	
    /*
    shadow cascade preview

    */
    if(previewCascades > 0.f) {
        int layer = GetCascadeLayer(worldPos);
        vec3 c = getCascadeColor(layer);

        vec4 fragPosLightSpace = lightSpaceMatrices[layer] * vec4(worldPos, 1.0);
        vec3 ndc = fragPosLightSpace.xyz/fragPosLightSpace.w;
        vec3 uv = NdcToUv(ndc);

        float fragDepth =  uv.z;

        if(clamp(uv,vec3(0.f),vec3(1.f)) != uv)
            c = vec3(0.);
        
        albedo = c * uv.z;
    }
    // end
                
    // reflectance equation
    vec3 Lo = DirectLightContribution(worldPos, N, V,F0, albedo,roughness,metallic);

    // IBL Diffuse ambient term from https://learnopengl.com/PBR/IBL/Diffuse-irradiance

    vec3 ambient = IBL(N,V,R,F0, albedo, ao, roughness, metallic);
    return applyFogRaymarch(worldPos, ambient + Lo);
}

#endif