#version 330

#ifdef VERTEX_SHADER
layout(location=0) in vec3 position1;
layout(location=2) in vec3 position2;

uniform mat4 mvpMatrix;
uniform float time;
uniform int useInterpolation;

void main()
{
    vec3 p;
    if (useInterpolation == 1) {
        p = position1 * (1.0 - time) + position2 * time;
    } else {
        p = position1;
    }
    
    gl_Position = mvpMatrix * vec4(p, 1);
}

#endif