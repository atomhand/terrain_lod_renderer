// Minimal vertex shader
#version 420

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;
layout(location = 3) in vec3 tangent;
layout(location = 4) in vec3 bitangent;

uniform mat4 model, view, projection;
uniform vec4 color;

out vec3 fnormal, fposition;
out vec4 fcolor;
out vec2 fTexCoord;
out mat3 TBN;

void main()
{
    vec3 T = normalize(vec3(model * vec4(tangent,   0.0)));
    vec3 B = normalize(vec3(model * vec4(bitangent, 0.0)));
    vec3 N = normalize(vec3(model * vec4(normal,    0.0)));
    TBN = mat3(T, B, N);



	gl_Position = (projection * view * model) * vec4(position,1.0);
    fposition = position.xyz;
    fcolor = color;
    fTexCoord = texCoord;
}