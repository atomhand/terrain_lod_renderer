// Minimal vertex shader
#version 420

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

uniform mat4 model, view, projection;
uniform vec4 colour;
uniform vec3 lightPositions[4];
uniform mat3 normalmatrix;

out vec3 fnormal, fposition, flightdir;
out vec4 fcolour;

void main()
{
    vec4 position_h = vec4(position,1.0);
    
	mat4 mv_matrix = view * model;				// Calculate the model-view transformation
    fposition = (mv_matrix * position_h).xyz;
    fnormal = normalize(normalmatrix * normal);
    flightdir = (lightPositions[0]) - fposition;
    fcolour = colour;

    gl_Position = (projection * view * model) * position_h;
}