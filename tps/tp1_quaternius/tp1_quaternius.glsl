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
uniform mat4 sourceMatrix;
uniform float time;

uniform vec3 view_pos;
uniform vec3 light_pos;

out vec3 vertex_normal;
out vec3 shadowmap_coord;
out vec3 view_dir;
out vec3 light_dir;
flat out int material_idx;

void main()
{
    vec3 vertex_position = position1 * (1.0 - time) + position2 * time;
    vertex_normal = mat3(normalMatrix) * ((normal1 * (1.0 - time)) + (normal2 * time));
    material_idx = material_idx_v;

    vec4 p = mvpMatrix * vec4(vertex_position, 1);
    view_dir = normalize(view_pos - vec3(p));
    light_dir = normalize(light_pos - vec3(p));
    gl_Position = p;

    shadowmap_coord = vec3(sourceMatrix * vec4(vertex_position, 1));
    shadowmap_coord.x = (shadowmap_coord.x + 1) / 2;
    shadowmap_coord.y = (shadowmap_coord.y + 1) / 2;
    //shadowmap_coord.z = (shadowmap_coord.z + 1) / 2;
}

#endif


#ifdef FRAGMENT_SHADER

in vec3 vertex_normal;
in vec3 shadowmap_coord;
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

uniform sampler2D shadowmap;


void main()
{
    
    vec4 diffuse = colors[material_idx * 2];
    vec4 specular = colors[(material_idx * 2) + 1];

    vec3 normal = normalize(vertex_normal);
    float ns = ns_tab[material_idx];

    vec3 h = normalize(view_dir + light_dir);
    float theta_h = max(0, dot(normal, h));
    float cos_theta = max(0, dot(normal, light_dir));
   
  	
    // diffuse
    diffuse = (diffuse / M_PI) ;
    
    // specular
    float spec = ((ns + 8)  / (8 * M_PI)) * pow(theta_h, ns);
    specular =  (spec * specular);
        
    vec4 result =  3 * (diffuse + specular) * cos_theta;

    // Mettre à l'ombre les objets non éclairés par la source de lumière
    float depth = texture(shadowmap, shadowmap_coord.xy).z;
    float shadow;
    if (depth < shadowmap_coord.z) {
        shadow = 1.0;
    }
    else {
        shadow = 0.5;
    }

    fragment_color = result * shadow;
}

#endif
