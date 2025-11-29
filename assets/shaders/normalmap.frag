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

out vec4 FragColor;		// Output fragment color

uniform uint colormode;	// Enables us to cycle through drawing modes to show the textures

uniform vec3 lightColors[4];

// Function to calculate per-vertex lighting
vec3 phongModel( vec3 norm, vec3 diffcolor, vec3 speccolor ) 
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
    vec3 diffuse = lightColors[0] * diffcolor * sDotN;
	vec3 ambient = diffcolor  * 0.2;

	// Calculate per-fragment specular color 
	// Using a lazy specular hard-coded color, should really import specular color and shineness from application!
    vec3 r = reflect( -light_dir, norm );
	vec3 spec = vec3(0.0);

    if( sDotN > 0.0 )
	{
        spec = pow( max( dot(r,view_dir), 0.0 ), 1.0) * lightColors[0] * speccolor;
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
    vec4 tex_color = texture( tex1, ftexcoord );

	vec3 spec_color = vec3(0.5, 0.5, 0.5);	// dull grey specular

	// Calculate phong lighting, using the normal from the normal map
	// to perturb the real noraml to simulate bumps and dips in the texture
	// Being a bit lazy and hard-coding the specular color to be white
    vec4 normal_map_lighting = vec4( phongModel(normal.xyz, tex_color.rgb, spec_color), 1.0 );

	vec4 normal_map_lighting_notexture = vec4( phongModel(normal.xyz, vec3(1.0, 1.0, 1.0), spec_color), 1.0 );

	// Switch the output color based on color mode to cycle through
	// texture, normal map extract and full normal map lighting
	if (colormode == 0)
		FragColor = tex_color;
	else if (colormode == 1)
		FragColor = normal_from_map;
	else if (colormode == 2)
		FragColor = normal;
	else if (colormode == 3)
		FragColor = normal_map_lighting_notexture;
	else if (colormode == 4)
		FragColor = normal_map_lighting;
	FragColor = normal_map_lighting;
}
