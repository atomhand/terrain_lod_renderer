#ifndef PBR_SHARED
#define PBR_SHARED

// Adapted from: https://learnopengl.com/PBR/Lighting
// https://learnopengl.com/PBR/IBL/Diffuse-irradiance
// https://learnopengl.com/PBR/IBL/Specular-IBL

in vec3 WorldPos;
in vec3 Normal;

// camera uniforms
uniform vec3 viewPos;

// material parameters
uniform vec3 mAlbedo;
uniform float mMetallic;
uniform float mRoughness;
uniform float mAo;

// textures
layout(binding=5) uniform sampler2DShadow shadowMap;
layout(binding=6) uniform samplerCube irradianceMap;
layout(binding=7) uniform samplerCube prefilterMap;
layout(binding=8) uniform sampler2D brdfLUT;

// point lights
uniform vec3 lightPositions[4];
uniform vec3 lightColors[4];

// directional lights
uniform vec3 lightDirections[4];
uniform vec3 directionalLightColors[4];
uniform mat4 directionLightMatrix;

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

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 outRadiance(vec3 L, vec3 V, vec3 N, vec3 F0, vec3 albedo, vec3 radiance, float roughness, float metallic) {
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
    return (kD * albedo / PI + specular) * radiance * NdotL; 
}

// Returns a random number based on a vec3 and an int.
float random(vec3 seed, int i){
	vec4 seed4 = vec4(seed,i);
	float dot_product = dot(seed4, vec4(12.9898,78.233,45.164,94.673));
	return fract(sin(dot_product) * 43758.5453);
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

    // Bias is caled based on the light angle and geometry normal
    float bias = 0.01 * clamp(dot(Normal, L),0.,1.);

    // Basic pcf filter

    // kernel from https://www.opengl-tutorial.org/intermediate-tutorials/tutorial-16-shadow-mapping/#pcf
    /*
    vec2 poissonDisk[4] = vec2[](
        vec2( -0.94201624, -0.39906216 ),
        vec2( 0.94558609, -0.76890725 ),
        vec2( -0.094184101, -0.92938870 ),
        vec2( 0.34495938, 0.29387760 )
    );
    */
    vec2 poissonDisk[16] = vec2[]( 
        vec2( -0.94201624, -0.39906216 ), 
        vec2( 0.94558609, -0.76890725 ), 
        vec2( -0.094184101, -0.92938870 ), 
        vec2( 0.34495938, 0.29387760 ), 
        vec2( -0.91588581, 0.45771432 ), 
        vec2( -0.81544232, -0.87912464 ), 
        vec2( -0.38277543, 0.27676845 ), 
        vec2( 0.97484398, 0.75648379 ), 
        vec2( 0.44323325, -0.97511554 ), 
        vec2( 0.53742981, -0.47373420 ), 
        vec2( -0.26496911, -0.41893023 ), 
        vec2( 0.79197514, 0.19090188 ), 
        vec2( -0.24188840, 0.99706507 ), 
        vec2( -0.81409955, 0.91437590 ), 
        vec2( 0.19984126, 0.78641367 ), 
        vec2( 0.14383161, -0.14100790 ) 
    );

    vec2 texelSize = 1.0 / textureSize(shadowMap, 0) * 2.0;
    float occlusion = 0.0;
    for(int i =0; i<4; i++) {
        int index = int(16.0*random(floor(WorldPos.xyz*1000.0), i))%16;
        occlusion += 0.25 * (1.0 - texture(shadowMap, vec3(uv.xy + poissonDisk[index] * texelSize,fragDepth-bias)).r);
    }

    return occlusion;
}

vec3 DirectLightContribution(vec3 N, vec3 V, vec3 F0, vec3 albedo, float roughness, float metallic) {
    vec3 Lo = vec3(0.0);
    for(int i = 0; i < 4; ++i) 
    {
        // calculate per-light radiance
        vec3 L = normalize(lightPositions[i] - WorldPos);
        float distance    = length(lightPositions[i] - WorldPos);
        float attenuation = 1.0 / (distance * distance);
        vec3 inRadiance     = lightColors[i] * attenuation;
        Lo += outRadiance(L,V,N,F0,albedo,inRadiance,roughness,metallic);
    }

    // directional lights
    for(int i = 0; i < 1; ++i) 
    {
        vec4 lightSpacePos = directionLightMatrix * vec4(WorldPos,1.0);

        vec3 L = normalize(-lightDirections[i]);
        vec3 inRadiance = directionalLightColors[i] * (1.0-CalculateOcclusion(lightSpacePos,N,L));
        Lo += outRadiance(L,V,N,F0,albedo,inRadiance,roughness,metallic);
    }
    return Lo;
}

vec3 IBL(vec3 N, vec3 V, vec3 R, vec3 F0, vec3 albedo, float ao, float roughness, float metallic) {
    vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);

    vec3 kS = F;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;

    vec3 irradiance = texture(irradianceMap, N).rgb;
    vec3 diffuse    = irradiance * albedo;

    // Specular IBL
    const float MAX_REFLECTION_LOD = 4.0;
    vec3 prefilteredColor = textureLod(prefilterMap, R,  roughness * MAX_REFLECTION_LOD).rgb;    
    vec2 brdf  = texture(brdfLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
    vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

    vec3 ambient = (kD * diffuse + specular) * ao;
    return ambient;
}

// Height fog formula from IQuilez
// https://iquilezles.org/articles/fog/
vec3 applyFog(vec3 col) {
    float a = 0.01; // base fog intensity
    float b = 0.1; // height falloff term

    vec3 L = normalize(-lightDirections[0]);

    vec3 rd = normalize(WorldPos -viewPos);
    float t = length(WorldPos -viewPos);

    float fogAmount = (a/b) * exp(-viewPos.y*b) * (1.0-exp(-t*rd.y*b))/rd.y;
    vec3  fogColor  = vec3(0.5,0.6,0.7);
    return mix( col, fogColor, min(1.,fogAmount) );
    /*
    float fogAmount = 1.0 - exp(-dist*b);
    float sunAmount = max(dot(V,L),0.0);
    vec3  fogColor  = mix( vec3(0.5,0.6,0.7), // blue
                           vec3(1.0,0.9,0.7), // yellow
                           pow(sunAmount,8.0) );
    return mix( col, fogColor, fogAmount );
    */
}

vec3 CalculateLighting(vec3 N, vec3 albedo, float ao, float roughness, float metallic) {
    vec3 V = normalize(viewPos - WorldPos);
    vec3 R = reflect(-V, N);

    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);
                
    // reflectance equation
    vec3 Lo = DirectLightContribution(N,V,F0, albedo,roughness,metallic);

    // IBL Diffuse ambient term from https://learnopengl.com/PBR/IBL/Diffuse-irradiance

    vec3 ambient = IBL(N,V,R,F0, albedo, ao, roughness, metallic);
    return applyFog(ambient + Lo);
}

vec3 ToneMap(vec3 color) {
    color = color / (color + vec3(1.0));
    return pow(color, vec3(1.0/2.2));
}

#endif