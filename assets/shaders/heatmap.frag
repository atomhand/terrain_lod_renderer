#version 420

#include "shared/uniforms_shared.glsl"

out vec4 FragColor;
  
in vec2 TexCoords;

layout(binding=0) uniform sampler2D myTexture;
layout(binding=1) uniform sampler2D exposureTex;

float saturate( float x ) { return clamp( x, 0.0, 1.0 ); }
vec3 saturate( vec3 x ) { return clamp( x, vec3(0.0,0.0,0.0), vec3(1.0,1.0,1.0) ); }

// Viridis approximation, Jerome Liard, August 2016
// https://www.shadertoy.com/view/XtGGzG
vec3 viridis_quintic( float x )
{
	x = saturate( x );
	vec4 x1 = vec4( 1.0, x, x * x, x * x * x ); // 1 x x2 x3
	vec4 x2 = x1 * x1.w * x; // x4 x5 x6 x7
	return saturate( vec3(
		dot( x1.xyzw, vec4( +0.280268003, -0.143510503, +2.225793877, -14.815088879 ) ) + dot( x2.xy, vec2( +25.212752309, -11.772589584 ) ),
		dot( x1.xyzw, vec4( -0.002117546, +1.617109353, -1.909305070, +2.701152864 ) ) + dot( x2.xy, vec2( -1.685288385, +0.178738871 ) ),
		dot( x1.xyzw, vec4( +0.300805501, +2.614650302, -12.019139090, +28.933559110 ) ) + dot( x2.xy, vec2( -33.491294770, +13.762053843 ) ) ) );
}

void main()
{
    const float gamma = 2.2;

    vec3 hdrColor = texture(myTexture, TexCoords).xyz;

    vec3 mapped = viridis_quintic(hdrColor.x);
    // gamma correct
    mapped = pow(mapped, vec3(1.0 / gamma));
    FragColor = vec4(mapped,1.0);
}