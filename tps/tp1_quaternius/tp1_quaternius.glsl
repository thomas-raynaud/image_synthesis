#version 330

#define M_PI 3.1415926535897932384626433832795

#ifdef VERTEX_SHADER
layout(location=0) in vec3 position1;
layout(location=1) in vec3 normal1;
layout(location=2) in vec3 position2;
layout(location=3) in vec3 normal2;
layout(location=4) in int material_idx_v;

uniform mat4 mvpMatrix;
uniform mat4 normalMatrix;
uniform float time;

uniform vec3 view_pos;
uniform vec3 light_pos;

out vec3 vertex_normal;
out vec3 vertex_position;
out vec3 view_dir;
out vec3 light_dir;
flat out int material_idx;

void main()
{
    vec3 p;
    p = position1 * (1.0 - time) + position2 * time;
    gl_Position = mvpMatrix * vec4(p, 1);
    vertex_normal = mat3(normalMatrix) * ((normal1 * (1.0 - time)) + (normal2 * time));
    vertex_position = vec3(mvpMatrix * vec4(p, 1));
    view_dir = normalize(view_pos - vertex_position);
    light_dir = normalize(light_pos - vertex_position);
    material_idx = material_idx_v;
}

#endif


#ifdef FRAGMENT_SHADER

in vec3 vertex_normal;
in vec3 vertex_position;
flat in int material_idx;
in vec3 view_dir;
in vec3 light_dir;

out vec4 fragment_color;

uniform MaterialBlock
{
    vec4 colors[NB_MATERIALS];
};
uniform ExposantBlinnPhong
{
    float ns_tab[NB_MATERIALS];
};


void main()
{
    
    vec4 diffuse = colors[material_idx * 2];
    vec4 specular = colors[(material_idx * 2) + 1];

    vec3 normal = normalize(vertex_normal);
    float ns = ns_tab[material_idx];

    vec3 h = normalize(view_dir + light_dir);
    //float theta_h = max(0, dot(normal, h));
    float cos_theta = max(0, dot(normal, h));
    
   
  	
    // diffuse
    diffuse = (diffuse / M_PI) ;
    
    // specular
    float spec = ((ns + 8)  / (8 * M_PI)) * pow(cos_theta, ns);
    specular =  (spec * specular);
        
    vec4 result =  3 * (diffuse + specular) * cos_theta;
    fragment_color = result;
}

#endif
