#version 330

#ifdef VERTEX_SHADER
out vec2 position;

void main(void)
{
    vec3 positions[3]= vec3[3]( vec3(-1, 1, -1), vec3( -1, -3, -1), vec3( 3,  1, -1) );
    
    gl_Position= vec4(positions[gl_VertexID], 1.0);
    position= positions[gl_VertexID].xy;
}
#endif


#ifdef FRAGMENT_SHADER

in vec2 position;
out vec4 fragment_color;

uniform sampler2D text;

void main( )
{
	fragment_color = texture(text, position); 
}
#endif
