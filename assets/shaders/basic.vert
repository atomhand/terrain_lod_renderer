// Minimal vertex shader
#version 420

layout(location = 0) in vec4 position;

uniform mat4 transform;
uniform mat4 view;
uniform mat4 projection;

void main()
{
	int index = gl_VertexID;

	gl_Position = projection * view * transform * position;
}