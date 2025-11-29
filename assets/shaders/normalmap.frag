// Vertex shader to demonstrate normal mapping
// This fragment shader was adapted from the normal mapping
// example in chapter 4 of :
// OpenGL 4.0 Shading language cookbook, second edition 

#version 420

in vec2 ftexcoord;
in vec3 flightdir;		// in tangent space
in vec3 fviewdir;		// in tangent space

layout (binding=0) uniform sampler2D tex1;		// texture
layout(binding=1) uniform sampler2D alphaMap;
layout (binding=2) uniform sampler2D normalMap;		// normal map inside a texture

out vec4 FragColour;		// Output fragment colour

uniform uint colourmode;	// Enables us to cycle through drawing modes to show the textures

uniform vec3 lightColours[4];

// Function to calculate per-vertex lighting
vec3 phongModel( vec3 norm, vec3 diffcolour, vec3 speccolour ) 
{
	// alpha clipping
	float alpha = texture(alphaMap,ftexcoord).x;
	if(alpha < 0.5) { 
		discard;
	}

	vec3 light_dir = normalize(flightdir);
	vec3 view_dir = normalize(fviewdir);
	// Calcuate dot product between normal (perturbed by normal map) and light direction in tangent space
    float sDotN = max( dot(light_dir, norm), 0.0 );

	// Calculate per-fragment diffuse reflection in tangent space
    vec3 diffuse = lightColours[0] * diffcolour * sDotN;
	vec3 ambient = diffcolour  * 0.2;

	// Calculate per-fragment specular colour 
	// Using a lazy specular hard-coded colour, should really import specular colour and shineness from application!
    vec3 r = reflect( -light_dir, norm );
	vec3 spec = vec3(0.0);

    if( sDotN > 0.0 )
	{
        spec = pow( max( dot(r,view_dir), 0.0 ), 1.0) * lightColours[0] * speccolour;
	}

    return ambient + diffuse + spec;
}

void main() 
{
    // Lookup the normal from the normal map 
	vec4 normal_from_map = texture(normalMap, ftexcoord);  

	// Convert the extracted normal_from_map to a surface normal
    vec4 normal = 2.0 * normal_from_map - 1.0;
	
	// Extract the texture from the texture map
    vec4 tex_colour = texture( tex1, ftexcoord );

	vec3 spec_colour = vec3(0.5, 0.5, 0.5);	// dull grey specular

	// Calculate phong lighting, using the normal from the normal map
	// to perturb the real noraml to simulate bumps and dips in the texture
	// Being a bit lazy and hard-coding the specular colour to be white
    vec4 normal_map_lighting = vec4( phongModel(normal.xyz, tex_colour.rgb, spec_colour), 1.0 );

	vec4 normal_map_lighting_notexture = vec4( phongModel(normal.xyz, vec3(1.0, 1.0, 1.0), spec_colour), 1.0 );

	// Switch the output colour based on colour mode to cycle through
	// texture, normal map extract and full normal map lighting
	if (colourmode == 0)
		FragColour = tex_colour;
	else if (colourmode == 1)
		FragColour = normal_from_map;
	else if (colourmode == 2)
		FragColour = normal;
	else if (colourmode == 3)
		FragColour = normal_map_lighting_notexture;
	else if (colourmode == 4)
		FragColour = normal_map_lighting;
	FragColour = normal_map_lighting;
}
