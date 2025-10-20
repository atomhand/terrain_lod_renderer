#version 450
// From https://learnopengl.com/Guest-Articles/2021/CSM
// No original work here
    
layout(triangles, invocations = 5) in;
layout(triangle_strip, max_vertices = 3) out;
    
#include "shared/uniforms_shared.glsl"
    
void main()
{          
    for (int i = 0; i < 3; ++i)
    {
        gl_Position = 
            lightSpaceMatrices[gl_InvocationID] * gl_in[i].gl_Position;
        gl_Layer = gl_InvocationID;
        EmitVertex();
    }
    EndPrimitive();
}  