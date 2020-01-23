#version 430

#ifdef COMPUTE_SHADER

#define inf 999999.0
#define EPSILON 0.0001
#define PI 3.1415


struct Triangle {
	vec3 a;	    // sommet
	vec3 ab;	// arete 1
	vec3 ac;	// arete 2
};

// storage buffer 0
layout(binding = 0) readonly buffer triangleData 
{
    Triangle triangles[];
};

// storage buffer 1
layout(binding = 1) readonly buffer sphereData 
{
    vec4 spheres[10];
};

uniform mat4 mvpInvMatrix;
uniform mat4 invViewport;
uniform int triangle_count;
uniform int sphere_count;
layout(binding = 0, rgba8) uniform image2D image;
//layout(binding = 1, uint) uniform uimage2D image_bruit;


bool intersectTriangle(const Triangle triangle, const vec3 o, const vec3 d, const float tmax, out float rt, out vec3 n)
{
    vec3 pvec= cross(d, triangle.ac);
    float det= dot(triangle.ab, pvec);
    float inv_det= 1.0f / det;

	vec3 tvec= o - triangle.a; 
    float u= dot(tvec, pvec) * inv_det;
    vec3 qvec= cross(tvec, triangle.ab);
    float v= dot(d, qvec) * inv_det;

    // Normale
    n = -normalize(cross(triangle.ab, triangle.ac));
    //if (dot(n, d) < 0)
        //n = -n;

    /* calculate t, ray intersects triangle */
    rt= dot(triangle.ac, qvec) * inv_det;

    // ne renvoie vrai que si l'intersection est valide : 
	// interieur du triangle, 0 < u < 1, 0 < v < 1, 0 < u+v < 1
	if(any(greaterThan(vec3(u, v, u+v), vec3(1, 1, 1))) || any(lessThan(vec2(u, v), vec2(0, 0))))
		return false;
	// comprise entre 0 et tmax du rayon
        return (rt < tmax && rt > 0);
}

bool triangleVisible(const vec3 o, const vec3 d, const float tmax) {
    float t;
    vec3 n;
    for(int i= 0; i < triangle_count; i++) {
        if(intersectTriangle(triangles[i], o, d, tmax, t, n))
        {
            return true;
        }
    }
    return false;
}

bool intersectSphere(const vec4 sphere, const in vec3 o, const in vec3 d, const float tmax, out float rt, out vec3 n)
{
    vec3 center = sphere.xyz;
    float radius = sphere.w;

    vec3 oc = o - center;
    float b = dot(oc, d);
    float c = dot(oc, oc) - radius * radius;
    float h = b*b - dot(d, d) * c;
    if(h < 0.0) return false;
    
    h= sqrt(h);
    float t1= (-b - h);
    
    // solution classique avec les 2 racines
    float t2= (-b + h);
    if(t1 <= 0 && t2 <= 0)
        return false;
    else if(t1 > 0 && t2 > 0)
        rt = min(t1, t2); // plus petite racine postive
    else
        rt = max(t1, t2); // racine positive

    vec3 p = o + rt * d; // Point d'intersection
    n = normalize(p - center);

    return (rt < tmax && rt > 0);
}

const uint a= 1103515245;
const uint b= 12345;
const uint m= 1u << 31;
uint x = 0;

float getSample(inout uint x)    				// renvoie un reel aleatoire dans [0 1]
{
    x= (a*x + b) % m;
    return float(x) / float(m);
}


vec3 getAmbientOcclusion(in vec3 p, in vec3 n) {
    vec3 color = vec3(0, 0, 0);
    vec3 N, T, B, v_local, v;
    vec3 v_tmp;
    float u1, u2, phi;
    float a, d, sign2;

    N = normalize(n);

    for (int i = 0; i < 32; ++i) {
        // Générer une direction aléatoire depuis p
        u1 = getSample(x);
        u2 = getSample(x);

        phi = 2 * PI * u1;
        v_local = vec3(cos(phi) * sqrt(1 - u2), sin(phi) * sqrt(1 - u2), sqrt(u2));

        // Passer la direction dans le repère monde
        if (N.z >= 0) sign2 = 1.0;
        else sign2 = -1.0;

        a= -1.0 / (sign2 + N.z);
        d= N.x * N.y * a;
        T= vec3(1.0f + sign2 * N.x * N.x * a, sign2 * d, -sign2 * N.x);
        B= vec3(d, sign2 + N.y * N.y * a, -N.y);

        v = normalize(v_local.x * T + v_local.y * B + v_local.z * N);

        if (!triangleVisible(p, v, 100)) {
            color = color + vec3(1, 1, 1);
        }
    }

    return color / 32;
}


layout(local_size_x = 8, local_size_y = 8) in;
void main() {
    vec2 pix = gl_GlobalInvocationID.xy;

    // construction du rayon pour le pixel, passage depuis le repere projectif
    vec4 oh = mvpInvMatrix * (invViewport * vec4(pix.xy, 0, 1));       // origine sur near
    vec4 eh = mvpInvMatrix * (invViewport * vec4(pix.xy, 1, 1));       // extremite sur far

    // origine et direction
    vec3 o= oh.xyz / oh.w;                              // origine
    vec3 d= eh.xyz / eh.w - oh.xyz / oh.w;              // direction

    float hit= length(d);	// tmax = far, une intersection valide est plus proche que l'extremite du rayon / far...
    d = normalize(d);
    vec3 color = vec3(0, 0, 0);
    float hitv= 0;
    int hitid= -1;

    vec3 n, n2, t2, b;

    float t, cos_theta;
    vec3 p;

    for(int i= 0; i < triangle_count; i++) {
        if(intersectTriangle(triangles[i], o, d, hit, t, n2)) {
            hit= t;
            hitid= i;
            n = n2;
        }
    }

    for(int i= 0; i < sphere_count; i++) {
        if(intersectSphere(spheres[i], o, d, hit, t, n2)) {
            hit= t;
            hitid= i;
            n = n2;
        }
    }

    // Get ambient occlusion
    if (hitid != -1) {
        if (dot(d, n) < 0) n = -n;
        vec3 p = o + hit * d; // Point d'intersection
        p = p + EPSILON * -n;
        color = getAmbientOcclusion(p, -n);
    }
    
    imageStore(image, ivec2(pix.xy), vec4(color, 1));
}

#endif