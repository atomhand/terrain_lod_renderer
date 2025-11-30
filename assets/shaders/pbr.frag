#version 420
// Adapted from: https://learnopengl.com/PBR/Lighting

out vec4 outputColor;
in vec2 TexCoords;
in vec3 WorldPos;
in vec3 Normal;

layout(binding=5) uniform sampler2DShadow shadowMap;

// material parameters
uniform vec3 albedo;
uniform float metallic;
uniform float roughness;
uniform float ao;

// point lights
uniform vec3 lightPositions[4];
uniform vec3 lightColors[4];

// directional lights
uniform vec3 lightDirections[4];
uniform vec3 directionalLightColors[4];
uniform mat4 directionLightMatrix;

uniform vec3 viewPos;

const float PI = 3.14159265359;

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
	
    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return num / denom;
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 outRadiance(vec3 L, vec3 V, vec3 N, vec3 F0, vec3 surfAlbedo, vec3 radiance) {
    vec3 H = normalize(V + L);
    
    // cook-torrance brdf
    float NDF = DistributionGGX(N, H, roughness);        
    float G   = GeometrySmith(N, V, L, roughness);      
    vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);       
    
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;	  
    
    vec3 numerator    = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular     = numerator / denominator;  
        
    // add to outgoing radiance Lo
    float NdotL = max(dot(N, L), 0.0);                
    return (kD * surfAlbedo / PI + specular) * radiance * NdotL; 
}

float CalculateOcclusion(vec4 lightSpacePos, vec3 N, vec3 L) {
    vec3 ndc = lightSpacePos.xyz / lightSpacePos.w;
    // transform ndc to 0..1
    vec3 uv = ndc * 0.5 + 0.5;

    // current fragment's depth from light's perspective
    float fragDepth = uv.z;

    // if frag is beyond our far depth, assume it's unoccluded
    if(fragDepth > 1.0)
        return 1.;

    float bias = 0.01 * clamp(dot(N, L),0.,1.);
    float occlusion = 0.0;

    // Basic pcf filter

    // kernel from https://www.opengl-tutorial.org/intermediate-tutorials/tutorial-16-shadow-mapping/#pcf
    vec2 poissonDisk[4] = vec2[](
        vec2( -0.94201624, -0.39906216 ),
        vec2( 0.94558609, -0.76890725 ),
        vec2( -0.094184101, -0.92938870 ),
        vec2( 0.34495938, 0.29387760 )
    );

    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int i =0; i<4; i++) {
        occlusion += 0.25 * (1.0 - texture(shadowMap, vec3(uv.xy + poissonDisk[i] * texelSize,fragDepth-bias)).r);
    }

    return occlusion;
}

void main()
{		
    vec3 N = normalize(Normal);
    vec3 V = normalize(viewPos - WorldPos);

    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);
	           
    // reflectance equation
    vec3 Lo = vec3(0.0);
    for(int i = 0; i < 4; ++i) 
    {
        // calculate per-light radiance
        vec3 L = normalize(lightPositions[i] - WorldPos);
        float distance    = length(lightPositions[i] - WorldPos);
        float attenuation = 1.0 / (distance * distance);
        vec3 inRadiance     = lightColors[i] * attenuation;
        Lo += outRadiance(L,V,N,F0,albedo,inRadiance);
    }

    // directional lights
    for(int i = 0; i < 4; ++i) 
    {
        vec4 lightSpacePos = directionLightMatrix * vec4(WorldPos,1.0);

        vec3 L = normalize(-lightDirections[i]);
        vec3 inRadiance = directionalLightColors[i] * (1.0-CalculateOcclusion(lightSpacePos,N,L));
        Lo += outRadiance(L,V,N,F0,albedo,inRadiance);
    } 
  
    vec3 ambient = vec3(0.03) * albedo * ao;
    vec3 color = ambient + Lo;
	
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));  
   
    outputColor = vec4(color, 1.0);
}