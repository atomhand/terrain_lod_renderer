#ifndef CURVATURE_SHARED_GLSL
#define CURVATURE_SHARED_GLSL

#include "shared/uniforms_shared.glsl"

// Tom Kellett 2025

// Displace a world space position to fake a globe curvature effect
vec3 getCurvedPosition(vec3 worldPos) {
    if(fakeCurvature > 0) {
        float d = length(worldPos.xyz-viewPos.xyz) / farPlane;

        float curveFactor = max(0.,pow(d,3.));

        float drop = curveFactor * 3000.f;
        float curvedY = mix(worldPos.y - drop,-drop,min(1.,pow(curveFactor,3.0)));
        return vec3(worldPos.x,curvedY,worldPos.z);
    } else {
        return worldPos;
    }
}

#endif