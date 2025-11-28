// Minimal vertex shader
#version 420

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

uniform mat4 model, view, projection;
uniform vec4 colour;

out vec3 fnormal, fposition;
out vec4 fcolour;

void main()
{
	int index = gl_VertexID;

	gl_Position = (projection * view * model) * vec4(position,1.0);
    fnormal = normal.xyz;
    fposition = position.xyz;
    fcolour = colour;
}