// Tom Kellett 2025
#version 420

in vec3 fColor;
in vec3 wireframeDist;
out vec4 outputColor;

void main()
{
    vec3 d = fwidth(wireframeDist); 
    vec3 a3 = smoothstep(vec3(0.0), d * 1.5, wireframeDist);
    float edgeFactor = min(min(a3.x, a3.y), a3.z);

    outputColor = vec4(mix(vec3(1.0), vec3(0.5), edgeFactor), 1.0);
}