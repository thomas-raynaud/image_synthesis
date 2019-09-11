#version 330

#ifdef VERTEX_SHADER
layout(location= 0) in vec3 position;
layout(location= 1) in vec2 texcoord;
out vec2 vertex_texcoord;

uniform mat4 mvpMatrix;

void main( )
{
    gl_Position= mvpMatrix * vec4(position, 1);
    vertex_texcoord = texcoord;
}

#endif


#ifdef FRAGMENT_SHADER
in vec2 vertex_texcoord;
out vec4 fragment_color;
uniform sampler2D color_texture;


void main( )
{
    vec4 color= texture(color_texture, vertex_texcoord);
    fragment_color= color;
}

#endif
