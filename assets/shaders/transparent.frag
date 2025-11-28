// Minimal fragment shader
#version 420

in vec3 fnormal, fposition;
in vec4 fcolour;

in vec2 fTexCoord;
in mat3 TBN;

layout(binding=0) uniform sampler2D albedoTex;
layout(binding=1) uniform sampler2D alphaTex;
layout(binding=2) uniform sampler2D normalTex;

uniform mat4 model, view, projection;
uniform mat3 normalmatrix;
uniform vec4 lightpos; // light intensity stored in W

uniform float shininess;

out vec4 outputColor;

// Global lighting constants (for this vertex shader)
vec3 specular_albedo = vec3(1.0, 0.8, 0.6);
vec3 global_ambient = vec3(0.05, 0.05, 0.05);

// Adapted Phong example to fragment shader and directional light
void main()
{
	vec3 emissive = vec3(0);				// Create a vec3(0, 0, 0) for our emmissive light
	vec4 position_h = vec4(fposition, 1.0);	// Convert the (x,y,z) position to homogeneous coords (x,y,z,w)

	float alpha = texture(alphaTex,fTexCoord).x;
	if(alpha < 0.5) { 
		discard;
	}

	vec4 diffuse_albedo = fcolour * texture(albedoTex,fTexCoord);					// This is the vertex colour, used to handle the colourmode change
	vec3 light_pos3 = lightpos.xyz;

	vec3 ambient = diffuse_albedo.xyz *0.2;

	// Define our vectors to calculate diffuse and specular lighting
	mat4 mv_matrix = view * model;		// Calculate the model-view transformation
	vec4 P = mv_matrix * position_h;	// Modify the vertex position (x, y, z, w) by the model-view transformation

	vec3 normal = texture(normalTex, fTexCoord).rgb;
	normal = normal * 2.0 - 1.0;

	vec3 N = normal; //normalize(normalmatrix * fnormal);		// Modify the normals by the normal-matrix (i.e. to model-view (or eye) coordinates )
	vec3 L = TBN * (light_pos3 - fposition);		// Calculate the vector from the light position to the vertex in eye space
	L = normalize(L);					// Normalise our light vector
	
	// Calculate the diffuse component
	vec3 diffuse = lightpos.w * 0.1 * max(dot(N, L), 0.0) * diffuse_albedo.xyz;

	// Calculate the specular component using Phong specular reflection
	vec3 V = TBN * normalize((view * vec4(0.,0.,0.,1.)).xyz-P.xyz);	
	vec3 R = reflect(-L, N);
	vec3 specular = lightpos.w * pow(max(dot(R, V), 0.0), shininess) * specular_albedo;

	// (Distance attenuation removed, since it's not applicable to sunlight)

	// Calculate the output colour, includung attenuation on the diffuse and specular components
	// Note that you may want to exclude the ambient form the attenuation factor so objects
	// are always visible, or include a global ambient
	outputColor = vec4(((ambient + diffuse + specular) + emissive + global_ambient)  * alpha, alpha);
}