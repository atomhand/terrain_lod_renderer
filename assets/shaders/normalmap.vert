// Vertex shader to demonstrate normal mapping
// The toObjectLocal matrix calculation was taken from:
// OpenGL 4.0 Shading language cookbook, second edition 

#version 420

// These are the vertex attributes
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 texcoord;
layout(location = 3) in vec3 tangent;		// vertex tangent vector

// Uniform variables passed in from the application
uniform mat4 model, view, projection;
uniform vec4 lightpos;

// Output the vertex colour, the texture coordinate, the light direction and the view direction
// which are needed in the fragment shader to calculate the normal mapped lighting
out vec4 fcolour;
out vec2 ftexcoord;
out vec3 flightdir;
out vec3 fviewdir;

// vec3 light_dir_bak = vec3(0.0, 5.0, 10.0);

void main()
{
	// diffuse_colour is used to show how to combine colouring with the texture and normal mapping
	vec4 diffuse_colour;			
	vec4 position_h = vec4(position, 1.0);		// vertex position in homogeneous coordinates
	vec4 tangent_h = vec4(tangent, 1.0);		// tangent vector in homogeneous coordinates

	mat4 mv_matrix = view * model;
	mat3 normalmatrix = mat3(transpose(inverse(mv_matrix)));

	vec3 light_pos3 = lightpos.xyz / lightpos.w;	// Convert back from homogeneous coors to 3D coords	

	// Calculate the normal, tangent and binormal vectors in model-view space
	vec3 norm = normalize(normalmatrix * normal);
	vec3 tang = normalize(normalmatrix * tangent);
	vec3 binormal = normalize(cross(norm, tang)) * tangent_h.w;

	/* Define the matrix used to transform the light direction and view direction
	   into tangent space */
	mat3 toObjectLocal = mat3(tang.x, binormal.x, norm.x,
							  tang.y, binormal.y, norm.y,
							  tang.z, binormal.z, norm.z);

	vec4 P = mv_matrix * position_h;
	vec3 pos = P.xyz / P.w;

	// Calculate the light direction in local vertex space (tangent space)
	// for a directional light source
	//flightdir = normalize(toObjectLocal * light_pos3);

	// Note, if using a positional light source then you would want to calculate the
	// light direction to the position as in the commented out line below
	flightdir = normalize(toObjectLocal * (light_pos3 - pos));

	// Calculate the view direction in local vertex space (tangent space)
	fviewdir = toObjectLocal * normalize(-pos);

	// Define the vertex position
	gl_Position = projection * view * model * position_h;

	// Output the texture coordinates
	ftexcoord = texcoord.xy;
}

