#version 420

#include "pbr_shared.glsl"

out vec4 outputColor;
in vec2 TexCoords;

layout(binding=0) uniform sampler2D diffuseTex;
layout(binding=1) uniform sampler2D normalMap;
layout(binding=2) uniform sampler2D armMap; // ao, roughness, metalness

// Reoriented Normal Mapping
// http://discourse.selfshadow.com/t/blending-in-detail/21/18
// via https://bgolus.medium.com/normal-mapping-for-a-triplanar-shader-10bf39dca05a
vec3 rnmBlendUnpacked(vec3 n1, vec3 n2)
{
    n1 += vec3( 0,  0, 1);
    n2 *= vec3(-1, -1, 1);
    return n1*dot(n1, n2)/n1.z - n2;
}

vec3 getTint() {
    float slope = min(1.0,dot(Normal,vec3(0.,1.,0.)));

    return mix(vec3(1.,1.,1.),vec3(1.,0.25,0.25),1.0 - slope*slope);
}

void main()
{
    // Triplanar mapping for normals
    // https://bgolus.medium.com/normal-mapping-for-a-triplanar-shader-10bf39dca05a
    vec3 blend = abs(Normal);
    blend = max(blend - 0.4, 0);
    blend /= dot(blend, vec3(1,1,1));
    //vec3 blend = abs(Normal);
     // blend /= blend.x + blend.y + blend.z;

    float triplanarScale = 4.f;
    vec2 uvX = WorldPos.zy / triplanarScale;
    vec2 uvY = WorldPos.xz / triplanarScale;
    vec2 uvZ = WorldPos.xy / triplanarScale;

    vec3 tnormalX = texture(normalMap, uvX).xyz * 2.0 - 1.0;
    vec3 tnormalY = texture(normalMap, uvY).xyz * 2.0 - 1.0;
    vec3 tnormalZ = texture(normalMap, uvZ).xyz * 2.0 - 1.0;

    // Get absolute value of normal to ensure positive tangent "z" for blend
    vec3 absVertNormal = abs(Normal);

    // Swizzle world normals to match tangent space and apply RNM blend
    tnormalX = rnmBlendUnpacked(vec3(Normal.zy, absVertNormal.x), tnormalX);
    tnormalY = rnmBlendUnpacked(vec3(Normal.xz, absVertNormal.y), tnormalY);
    tnormalZ = rnmBlendUnpacked(vec3(Normal.xy, absVertNormal.z), tnormalZ);

    // Get the sign (-1 or 1) of the surface normal
    vec3 axisSign = sign(Normal);

    // Reapply sign to Z
    tnormalX.z *= axisSign.x;
    tnormalY.z *= axisSign.y;
    tnormalZ.z *= axisSign.z;

    // calculate blended normal
    vec3 N = normalize(
        tnormalX.xyz * blend.x +
        tnormalY.xyz * blend.y +
        tnormalZ.xyz * blend.z +
        Normal
    );

    // Get triplanar albedo

    vec3 albedoX = texture(diffuseTex, uvX).rgb;
    vec3 albedoY = texture(diffuseTex, uvY).rgb;
    vec3 albedoZ = texture(diffuseTex, uvZ).rgb;

    vec3 albedo = (blend.x * albedoX + blend.y * albedoY + blend.z * albedoZ) ;//* getTint();

    vec3 armX = texture(armMap, uvX).rgb;
    vec3 armY = texture(armMap, uvY).rgb;
    vec3 armZ = texture(armMap, uvZ).rgb;

    vec3 arm = (blend.x * armX + blend.y * armY + blend.z * armZ);
	
    // lighting
    vec3 color = CalculateLighting(N,albedo,arm.x,arm.y,arm.z);

    outputColor = vec4(ToneMap(color),1.0);
}