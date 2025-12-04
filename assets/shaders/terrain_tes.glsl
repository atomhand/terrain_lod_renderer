// tessellation evaluation shader
#version 420

layout (quads, fractional_odd_spacing, ccw) in;

layout(binding=3) uniform sampler2DArray chunkData;

uniform mat4 model;           // the model matrix
uniform mat4 view;            // the view matrix
uniform mat4 projection;      // the projection matrix

// received from Tessellation Control Shader - all texture coordinates for the patch vertices
in vec4 Coord[];
in int instanceID[];

uniform float scale;

uniform mat3 normalMatrix;

// send to Fragment Shader for coloring
out vec3 WorldPos;
out vec3 Normal;

void main()
{
    // get patch coordinate
    float u = gl_TessCoord.x;
    float v = gl_TessCoord.y;

    // ----------------------------------------------------------------------
    // retrieve control point texture coordinates

    /*
    vec2 t00 = TextureCoord[0];
    vec2 t01 = TextureCoord[1];
    vec2 t10 = TextureCoord[2];
    vec2 t11 = TextureCoord[3];

    // bilinearly interpolate texture coordinate across patch
    vec2 t0 = (t01 - t00) * u + t00;
    vec2 t1 = (t11 - t10) * u + t10;
    vec2 texCoord = (t1 - t0) * v + t0;
    */

    // lookup texel at patch coordinate for height and scale + shift as desired
    //Height = texture(heightMap, texCoord).y * 64.0 - 16.0;

    // ----------------------------------------------------------------------
    // retrieve control point position coordinates
    vec4 p00 = Coord[0];
    vec4 p01 = Coord[1];
    vec4 p10 = Coord[2];
    vec4 p11 = Coord[3];
    //vec4 p11 = gl_in[3].gl_Position;

    /*
    // compute patch surface normal
    vec4 uVec = p01 - p00;
    vec4 vVec = p10 - p00;
    vec4 normal = normalize( vec4(cross(vVec.xyz, uVec.xyz), 0) );

    // bilinearly interpolate position coordinate across patch
    vec4 p0 = (p01 - p00) * u + p00;
    vec4 p1 = (p11 - p10) * u + p10;
    vec4 p = (p1 - p0) * v + p0;

    // displace point along normal
    p += normal * Height;
    */
    vec4 p0 = (p01 - p00) * u + p00;
    vec4 p1 = (p11 - p10) * u + p10;
    vec4 p = (p1 - p0) * v + p0;

    vec2 offset = p.zw;

    vec4 chunk = texture(chunkData, vec3(p.xy,instanceID[0]));
    Normal = chunk.xyz;

    WorldPos = vec3(vec4(scale*p.x+offset.x,chunk.w,scale*p.y+offset.y, 1.0));

    // ----------------------------------------------------------------------
    // output patch point position in clip space
    gl_Position = projection * view * vec4(WorldPos,1.0);
}