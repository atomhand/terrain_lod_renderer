// Tom Kellett 2025
#version 420

in vec3 fColor;
out vec4 outputColor;

void main()
{
    outputColor = vec4(fColor,1.0);
}