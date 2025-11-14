// Minimal fragment shader
#version 420

in vec3 fnormal, fposition;
in vec4 fcolour;

uniform mat4 model, view, projection;
uniform mat3 normalmatrix;
uniform vec4 lightpos;

out vec4 outputColor;

// Global lighting constants (for this vertex shader)
vec3 specular_albedo = vec3(1.0, 0.8, 0.6);
vec3 global_ambient = vec3(0.05, 0.05, 0.05);
int  shininess = 8;

void main()
{
	vec3 emissive = vec3(0);				// Create a vec3(0, 0, 0) for our emmissive light
	vec4 position_h = vec4(fposition, 1.0);	// Convert the (x,y,z) position to homogeneous coords (x,y,z,w)
	vec4 diffuse_albedo = fcolour;					// This is the vertex colour, used to handle the colourmode change
	vec3 light_pos3 = lightpos.xyz;

	vec3 ambient = diffuse_albedo.xyz *0.2;

	// Define our vectors to calculate diffuse and specular lighting
	mat4 mv_matrix = view * model;		// Calculate the model-view transformation
	vec4 P = mv_matrix * position_h;	// Modify the vertex position (x, y, z, w) by the model-view transformation
	vec3 N = normalize(normalmatrix * fnormal);		// Modify the normals by the normal-matrix (i.e. to model-view (or eye) coordinates )
	vec3 L = light_pos3 - P.xyz;		// Calculate the vector from the light position to the vertex in eye space
	float distanceToLight = length(L);	// For attenuation
	L = normalize(L);					// Normalise our light vector
	
	// Calculate the diffuse component
	vec3 diffuse = max(dot(N, L), 0.0) * diffuse_albedo.xyz;

	// Calculate the specular component using Phong specular reflection
	vec3 V = normalize(-P.xyz);	
	vec3 R = reflect(-L, N);
	vec3 specular = pow(max(dot(R, V), 0.0), shininess) * specular_albedo;

	// Calculate the attenuation factor;
	// Define attenuation constants. These could be uniforms for greater flexibility
	float attenuation_k1 = 0.5;
	float attenuation_k2 = 0.5;
	float attenuation_k3 = 0.5;
	float attenuation = 1.0 / (attenuation_k1 + attenuation_k2*distanceToLight + 
								attenuation_k3 * pow(distanceToLight, 2));

	// temp
	attenuation =1.0;

	// Calculate the output colour, includung attenuation on the diffuse and specular components
	// Note that you may want to exclude the ambient form the attenuation factor so objects
	// are always visible, or include a global ambient
	outputColor = vec4(attenuation*(ambient + diffuse + specular) + emissive + global_ambient, 1.0);
}