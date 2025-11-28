// Minimal vertex shader
#version 420

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

uniform mat4 model, view, projection;
uniform vec4 colour;
uniform vec4 lightpos;
uniform mat3 normalmatrix;

out vec3 fnormal, fposition, flightdir;
out vec4 fcolour;

void main()
{
    vec4 position_h = vec4(position,1.0);
    
	mat4 mv_matrix = view * model;				// Calculate the model-view transformation
    fposition = (mv_matrix * position_h).xyz;
    fnormal = normalize(normalmatrix * normal);
    flightdir = (lightpos.xyz / lightpos.w) - fposition;
    fcolour = colour;

    gl_Position = (projection * view * model) * position_h;
}