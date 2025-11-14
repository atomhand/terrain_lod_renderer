// Minimal vertex shader
#version 420

layout(location = 0) in vec4 position;
layout(location = 1) in vec4 normal;

uniform mat4 model, view, projection;

out vec3 fnormal, fposition;
out vec4 fcolour;

void main()
{
	int index = gl_VertexID;

	gl_Position = (projection * view * model) * position;
    fnormal = normal.xyz;
    fposition = position.xyz;
    fcolour = vec4(1.0,0.0,1.0,1.0);
}