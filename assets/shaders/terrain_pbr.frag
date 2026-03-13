#version 460
// Tom Kellett 2025
// Normals reconstruction code from https://bgolus.medium.com/normal-mapping-for-a-triplanar-shader-10bf39dca05a
// Texture splatting code original mine

in vec3 WorldPos;
in vec3 Normal;
in vec3 debugColor;

#include "shared/uniforms_shared.glsl"
#include "terrain_shared.glsl"
#include "shared/deferred_shared.glsl"

void main()
{
    TriplanarSample X, Y, Z;
    GetTriplanarSamples(WorldPos, Normal, X,Y,Z);

    // Triplanar blend weights    
    vec3 weights = TriplanarWeights(Normal, X.h, Y.h, Z.h);

    // Triplanar normals reconstruction
    // https://bgolus.medium.com/normal-mapping-for-a-triplanar-shader-10bf39dca05a

    // Get absolute value of normal to ensure positive tangent "z" for blend
    vec3 absVertNormal = abs(Normal);

    // Swizzle world normals to match tangent space and apply RNM blend
    vec3 tnormalX = rnmBlendUnpacked(vec3(Normal.zy, absVertNormal.x), X.normal);
    vec3 tnormalY = rnmBlendUnpacked(vec3(Normal.xz, absVertNormal.y), Y.normal);
    vec3 tnormalZ = rnmBlendUnpacked(vec3(Normal.xy, absVertNormal.z), Z.normal);

    // Get the sign (-1 or 1) of the surface normal
    vec3 axisSign = sign(Normal);

    // Reapply sign to Z
    tnormalX.z *= axisSign.x;
    tnormalY.y *= axisSign.y;
    tnormalZ.z *= axisSign.z;

    // calculate blended normal
    vec3 N = normalize(
        tnormalX.zyx * weights.x +
        tnormalY.xzy * weights.y +
        tnormalZ.xyz * weights.z +
        Normal
    );

    // final blend
    vec3 albedo = weights.x * X.albedo + weights.y * Y.albedo + weights.z * Z.albedo;
    vec3 arm = weights.x * X.arm + weights.y * Y.arm + weights.z * Z.arm;

    if(previewNormalsMode == 1.f) {
        N = Normal;
    }
    
    WriteGBuffer(WorldPos, N, albedo, arm.x, arm.y, arm.z);
}