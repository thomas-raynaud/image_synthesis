#version 330

#define M_PI 3.1415926535897932384626433832795

#ifdef VERTEX_SHADER
layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;


uniform mat4 mvpMatrix;
uniform mat4 normalMatrix;
uniform mat4 sourceMatrix;
uniform vec3 light_pos;

out vec3 vertex_normal;
out vec3 light_dir;
out vec3 shadowmap_coord;

void main()
{
    vertex_normal = mat3(normalMatrix) * normal;
    light_dir = normalize(light_pos - vec3(position));
    gl_Position = mvpMatrix * vec4(position, 1);

    shadowmap_coord = vec3(sourceMatrix * vec4(position + 0.02 * normal, 1));
}

#endif


#ifdef FRAGMENT_SHADER

in vec3 vertex_normal;
in vec3 light_dir;
in vec3 shadowmap_coord;

out vec4 fragment_color;

uniform sampler2D shadowmap;

void main()
{
    vec4 color = vec4(1, 1, 1, 1);

    vec3 normal = normalize(vertex_normal);
    float cos_theta = max(0, dot(normal, light_dir));

    float depth = texture(shadowmap, shadowmap_coord.xy).x;
    float shadow;
    if (depth < shadowmap_coord.z) {
        shadow = 0.1;
    }
    else {
        shadow = 1.0;
    }

    fragment_color = color * cos_theta * shadow;
}

#endif
