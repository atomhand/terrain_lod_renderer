#version 420

#include "shared/uniforms_shared.glsl"

out vec4 FragColor;
  
in vec2 TexCoords;

layout(binding=0) uniform sampler2D myTexture;
layout(binding=1) uniform sampler2D exposureTex;

void main()
{
    const float gamma = 2.2;

    vec3 hdrColor = texture(myTexture, TexCoords).xyz;

    vec3 mapped;
    if(applyExposure > 0.0) {
        float lumAvg = texture(exposureTex, vec2(0.5,0.5)).x;
        float whiteOut = 4.0;
        float w2 = whiteOut*whiteOut;

        // ratio of lumNax to lumAvg
        // Chosen empirically (altho there may be a more principled way to go about it)
        const float lumMaxRatio = 2.0;

        // apply exposure adjustment to input luminance
        vec3 adjustedColor = hdrColor / (lumMaxRatio * lumAvg);
        // Reinhard operator
        mapped = adjustedColor * (1.0 + adjustedColor / w2) / (adjustedColor + 1.0);

    } else {
        mapped = hdrColor / (1.0 + hdrColor);
    }

    // gamma correct
    mapped = pow(mapped, vec3(1.0 / gamma));
    FragColor = vec4(mapped,1.0);
}