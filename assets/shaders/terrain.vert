#version 420
// Adapted from: https://learnopengl.com/PBR/Lighting

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aOffset;

layout(binding=3) uniform sampler2DArray chunkData;

uniform float scale;

out vec4 tCoord;
out int tInstanceID;
out vec3 Normal;

uniform mat4 projection;
uniform mat4 view;
uniform mat3 normalMatrix;

void main()
{  
    vec4 chunk = texture(chunkData, vec3(aPos.xy,gl_InstanceID));

    tInstanceID = gl_InstanceID;

    tCoord = vec4(aPos,aOffset);
    vec3 WorldPos = vec3(vec4(scale*aPos.x+aOffset.x,chunk.w,scale*aPos.y+aOffset.y, 1.0));
    //Normal = chunk.xyz;

    gl_Position =  vec4(WorldPos, 1.0);
}