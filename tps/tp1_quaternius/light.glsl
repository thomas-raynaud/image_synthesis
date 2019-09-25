#version 330

#ifdef VERTEX_SHADER

uniform mat4 mvpMatrix;

void main()
{
    gl_Position = mvpMatrix * vec4(p, 1);
}

#endif


#ifdef FRAGMENT_SHADER

out vec4 fragment_color;

void main()
{
    fragment_color = Vec4(1, 1, 1, 1);
}

#endif
