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

uniform vec3 lightPos;
uniform vec3 viewPos;

uniform MaterialBlock
{
    vec3 colors[NB_MATERIALS];
};
uniform IndexBlock
{
    int color_indices[NB_MT_TRIANGLES];
};
uniform ExposantBlinnPhong
{
    float ns_tab[NB_MATERIALS];
};


void main()
{
    int color_index = color_indices[gl_PrimitiveID];
    vec3 diffuse = colors[color_index * 3];
    vec3 emission = colors[(color_index * 3) + 1];
    vec3 specular = colors[(color_index * 3) + 2];
    vec3 normal = normalize(vertex_normal);
    float ns = ns_tab[color_index];
  	
    // diffuse
    vec3 lightDir = normalize(lightPos - vertex_position);
    float diff = max(0, dot(normal, lightDir));
    diffuse = diff * diffuse;
    
    // specular
    vec3 viewDir = normalize(viewPos - vertex_position);
    vec3 reflectDir = reflect(-lightDir, normal);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), ns);
    specular = spec * specular;  
        
    vec3 result = emission + diffuse + specular;
    fragment_color = vec4(result, 1.0);
}

#endif
