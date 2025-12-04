// tessellation control shader
#version 420

// specify number of control points per patch output
// this value controls the size of the input and output arrays
layout (vertices=4) out;

uniform mat4 projection;
uniform mat4 view;

in vec4 tCoord[];
in int tInstanceID[];

out vec4 Coord[];
out int instanceID[];

// varying input from vertex shader
//in vec3 Normal[];
// varying output to evaluation shader

void main()
{
    // ----------------------------------------------------------------------
    // pass attributes through
    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
    //fNormal[gl_InvocationID] = Normal[gl_InvocationID];
    Coord[gl_InvocationID] = tCoord[gl_InvocationID];
    instanceID[gl_InvocationID] = tInstanceID[gl_InvocationID];

    // ----------------------------------------------------------------------
    // invocation zero controls tessellation levels for the entire patch
    if (gl_InvocationID == 0)
    {
        const int MIN_TESS_LEVEL = 1;
        const int MAX_TESS_LEVEL = 16;
        const float MIN_DISTANCE = 20;
        const float MAX_DISTANCE = 2048;

        vec4 eyeSpacePos00 = view * gl_in[0].gl_Position;
        vec4 eyeSpacePos01 = view * gl_in[1].gl_Position;
        vec4 eyeSpacePos10 = view * gl_in[2].gl_Position;
        vec4 eyeSpacePos11 = view * gl_in[3].gl_Position;

        // "distance" from camera scaled between 0 and 1
        float distance00 = clamp( (abs(eyeSpacePos00.z) - MIN_DISTANCE) / (MAX_DISTANCE-MIN_DISTANCE), 0.0, 1.0 );
        float distance01 = clamp( (abs(eyeSpacePos01.z) - MIN_DISTANCE) / (MAX_DISTANCE-MIN_DISTANCE), 0.0, 1.0 );
        float distance10 = clamp( (abs(eyeSpacePos10.z) - MIN_DISTANCE) / (MAX_DISTANCE-MIN_DISTANCE), 0.0, 1.0 );
        float distance11 = clamp( (abs(eyeSpacePos11.z) - MIN_DISTANCE) / (MAX_DISTANCE-MIN_DISTANCE), 0.0, 1.0 );

        float tessLevel0 = mix( MAX_TESS_LEVEL, MIN_TESS_LEVEL, min(distance10, distance00) );
        float tessLevel1 = mix( MAX_TESS_LEVEL, MIN_TESS_LEVEL, min(distance00, distance01) );
        float tessLevel2 = mix( MAX_TESS_LEVEL, MIN_TESS_LEVEL, min(distance01, distance11) );
        float tessLevel3 = mix( MAX_TESS_LEVEL, MIN_TESS_LEVEL, min(distance11, distance10) );

        gl_TessLevelOuter[0] = tessLevel0;
        gl_TessLevelOuter[1] = tessLevel1;
        gl_TessLevelOuter[2] = tessLevel2;
        gl_TessLevelOuter[3] = tessLevel3;

        gl_TessLevelInner[0] = max(tessLevel1, tessLevel3);
        gl_TessLevelInner[1] = max(tessLevel0, tessLevel2);
    }
}