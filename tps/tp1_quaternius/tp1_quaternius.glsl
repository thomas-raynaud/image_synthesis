#version 330

#ifdef VERTEX_SHADER
layout(location=0) in vec3 position1;
layout(location=1) in vec3 normal1;
layout(location=2) in vec3 position2;
layout(location=3) in vec3 normal2;

uniform mat4 mMatrix;
uniform mat4 mvpMatrix;
uniform mat4 normalMatrix;
uniform float time;

out vec3 vertex_normal;
out vec3 vertex_position;

void main()
{
    vec3 p;
    p = position1 * (1.0 - time) + position2 * time;
    gl_Position= mvpMatrix * vec4(p, 1);
    vertex_normal = mat3(normalMatrix) * ((normal1 * (1.0 - time)) + (normal2 * time));
    vertex_position= vec3(mMatrix * vec4(position1, 1));
}

#endif


#ifdef FRAGMENT_SHADER

in vec3 vertex_normal;
in vec3 vertex_position;
out vec4 fragment_color;

uniform vec3 light;

uniform MaterialBlock
{
    vec3 colors[256];
};
uniform IndexBlock
{
    int color_indices[256];
};


void main()
{
    int color_index = color_indices[gl_PrimitiveID];
    vec3 color = colors[color_index];
    //vec3 color = vec3(1, 1, 1);
    vec3 normal = normalize(vertex_normal);

    float cos_theta = 1;
    cos_theta = max(0, dot(normal, normalize(light - vertex_position))); // eclairage, uniquement des faces bien orientees
    
    color.rgb = color.rgb * cos_theta;
    fragment_color = vec4(color, 1);
}

#endif
