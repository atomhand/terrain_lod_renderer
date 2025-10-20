#version 420
out vec4 FragColor;
  
in vec2 TexCoords;

layout(binding=0) uniform sampler2D myTexture;

void main()
{
    vec3 col = texture(myTexture, TexCoords).xyz;
    FragColor = vec4(col,1.0);
}