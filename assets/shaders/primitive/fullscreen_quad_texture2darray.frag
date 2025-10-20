#version 420
out vec4 FragColor;
  
in vec2 TexCoords;

layout(binding=0) uniform sampler2DArray myTexture;

void main()
{
    vec3 size = textureSize(myTexture,0);

    float h = ceil(sqrt(size.z));

    vec2 coords = TexCoords*h;

    vec2 offset = floor(coords);

    float layer = offset.x + offset.y * h;

    vec2 uv = coords - offset;
    vec3 col = texture(myTexture, vec3(uv,layer)).xyz;
    FragColor = layer < size.z ? vec4(col,1.0) : vec4(0);
}