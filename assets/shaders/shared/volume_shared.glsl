#ifndef VOLUME_SHARED_GLSL
#define VOLUME_SHARED_GLSL

#include "shared/uniforms_shared.glsl"

// modified from https://www.shadertoy.com/view/XlBSRz
void getParticipatingMedia(out float sigmaS, out float sigmaE, in vec3 pos)
{
    //float heightFog = clamp((heightFogTransitionStart-pos.y)/heightFogTransitionDuration, 0.0, 1.0);

    float b = 1.0 / heightFogTransitionDuration;
    float heightFog = exp(-(pos.y-heightFogTransitionStart)/heightFogTransitionDuration);

    sigmaS = constantFogFactor + heightFog*heightFogFactor;
   
    const float sigmaA = 0.0;
    sigmaE = max(0.000000001, sigmaA + sigmaS); // to avoid division by zero extinction
}


// from https://www.shadertoy.com/view/XlBSRz
float phaseFunction()
{
    return 1.0/(4.0*3.14);
}

// from https://www.shadertoy.com/view/XlBSRz
float volumetricShadow(vec3 from, vec3 to) {
    const float numStep = 16.0;
    float shadow = 1.0;
    float sigmaS = 0.0;
    float sigmaE = 0.0;
    float dd = length(to-from)/numStep;
    for(float s=0.5; s<(numStep-0.1); s+=1.0) {
        vec3 pos = from+(to-from)*(s/(numStep));
        getParticipatingMedia(sigmaS,sigmaE,pos);
        shadow *= exp(-sigmaE*dd);
    }
    return shadow;
}

// Volume integration algorithm from https://www.shadertoy.com/view/XlBSRz
// Adapted to integrate with my shadow system but otherwise unchanged
vec3 applyFogRaymarch(vec3 worldPos, vec3 col) {
    float transmittance = 1.0;
    vec3 inscattering  = vec3(0.0,0.0,0.0);
    if(max(heightFogFactor,constantFogFactor) > 0.) {
        float sigmaS, sigmaE;

        vec3 ro = viewPos.xyz;
        vec3 rd = normalize(worldPos -viewPos.xyz);

        const int numIter = 100;
        float stepsize = length(worldPos -viewPos.xyz) / float(numIter);

        float d = stepsize;
        for(int i=0; i<numIter; i++) {
            vec3 p = ro + d * rd;

            getParticipatingMedia(sigmaS,sigmaE, p);

            // Note - would be more efficient to march between 2 points in light space?
            vec3 inRadiance = lightColor.rgb * ShadowAtPos(p);//  * volumetricShadow(WorldPos,WorldPos-lightDirections[0]*32.0);

            /*
            // Evaluate point lights
            for(int i = 0; i < 4; ++i) 
            {
                float distance    = length(lightPositions[i] - WorldPos);
                float attenuation = 1.0 / (distance * distance);
                inRadiance += lightColors[i] * attenuation;
            }
            */

            vec3 S = inRadiance *sigmaS * phaseFunction(); // * volumetricShadow
            vec3 Sint = (S - S * exp(-sigmaE * stepsize)) / sigmaE; // integrate along current segment
            inscattering += transmittance * Sint;

            transmittance *= exp(-sigmaE * stepsize);

            d+= stepsize;
        }
    }

    return col*transmittance + inscattering;
}

#endif