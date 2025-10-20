// from https://learnopengl.com/PBR/IBL/Diffuse-irradiance

#version 420
layout (location = 0) in vec3 aPos;

#include "shared/uniforms_shared.glsl"

out vec3 SamplePos;
out vec4 ClipPos;

void main()
{
    SamplePos = aPos;

	mat4 rotView = mat4(mat3(view));
	vec4 clipPos = projection * rotView * vec4(SamplePos, 1.0);

	ClipPos = vec4(clipPos.xy,0.0,clipPos.w);
	gl_Position = ClipPos;
}