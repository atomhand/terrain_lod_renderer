
#version 450
layout (location = 0) in vec3 aPos;
// From https://learnopengl.com/Guest-Articles/2021/CSM
// No original work here
    
uniform mat4 model;
    
void main()
{
    gl_Position = model * vec4(aPos, 1.0);
}