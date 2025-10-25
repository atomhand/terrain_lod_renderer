// Minimal vertex shader
#version 420

layout(location = 0) in vec4 position;

uniform mat4 view;
uniform mat4 projection;

void main()
{
	int index = gl_VertexID;

	vec4 vertexPositions[6] = vec4[6](
            vec4(0.f, 0.f, 0.0f, 1.0f),
            vec4(1.f, 0.f, 0.0f, 1.0f),
            vec4(1.f, 0.f, 1.0f, 1.0f),

            vec4(0.f, 0.f, 0.0f, 1.0f),
            vec4(0.f, 0.f, 1.0f, 1.0f),
            vec4(1.f, 0.f, 1.0f, 1.0f)
        );

	gl_Position = projection * view * vertexPositions[index];
}