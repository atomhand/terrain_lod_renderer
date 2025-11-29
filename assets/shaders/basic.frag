// Minimal fragment shader
#version 420

in vec3 fnormal, fposition, flightdir;
in vec4 fcolour;

uniform float shininess;

out vec4 outputColor;

uniform vec3 lightColours[4];

// Global lighting constants (for this vertex shader)
vec3 specular_albedo = vec3(1.0, 0.8, 0.6);
vec3 global_ambient = vec3(0.05, 0.05, 0.05);

// Adapted Phong example to fragment shader and directional light
void main()
{
	vec3 emissive = vec3(0);				// Create a vec3(0, 0, 0) for our emmissive light
	vec4 position_h = vec4(fposition, 1.0);	// Convert the (x,y,z) position to homogeneous coords (x,y,z,w)

	vec4 diffuse_albedo = fcolour;					// This is the vertex colour, used to handle the colourmode change
	vec3 ambient = diffuse_albedo.xyz *0.2;
	float distancetolight = length(flightdir) / 6.0;

	vec3 N = normalize(fnormal);
	vec3 L = normalize(flightdir);
	
	// Calculate the diffuse component
	vec3 diffuse = lightColours[0] * max(dot(N, L), 0.0) * diffuse_albedo.xyz;

	// Calculate the specular component using Phong specular reflection
	vec3 V = normalize(-fposition);	
	vec3 R = reflect(-L, N);
	vec3 specular = lightColours[0] * pow(max(dot(R, V), 0.0), shininess) * specular_albedo;

	// Define attenuation constants. These could be uniforms for greater flexibility
	float attenuation_k1 = 0.5;
	float attenuation_k2 = 0.5;
	float attenuation_k3 = 0.5;
	float attenuation = 1.0 / (attenuation_k1 + attenuation_k2*distancetolight + 
								attenuation_k3 * pow(distancetolight, 2));

	// Calculate the output colour, includung attenuation on the diffuse and specular components
	// Note that you may want to exclude the ambient form the attenuation factor so objects
	// are always visible, or include a global ambient
	outputColor = vec4(attenuation * ((ambient + diffuse + specular) + emissive + global_ambient), 1.0);
}