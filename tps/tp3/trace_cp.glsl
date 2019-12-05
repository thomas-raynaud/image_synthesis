#version 430

#ifdef COMPUTE_SHADER


#define inf 999999.0
#define EPSILON 0.0001

struct Triangle
{
	vec3 a;	// sommet
	vec3 ab;	// arete 1
	vec3 ac;	// arete 2
};

// uniform buffer 0
uniform triangleData
{
	Triangle triangles[1024];
};


bool intersectTriangle(const Triangle triangle, const vec3 o, const vec3 d, const float tmax, out float rt, out float ru, out float rv )
{
    vec3 pvec= cross(d, triangle.ac);
    float det= dot(triangle.ab, pvec);
    float inv_det= 1.0f / det;

	vec3 tvec= o - triangle.a; 
    float u= dot(tvec, pvec) * inv_det;
    vec3 qvec= cross(tvec, triangle.ab);
    float v= dot(d, qvec) * inv_det;

    vec3 n = normalize(cross(triangle.ab, triangle.ac));

    float cos_theta = max(0, dot(-n, d));

    /* calculate t, ray intersects triangle */
    rt= dot(triangle.ac, qvec) * inv_det;
    ru= cos_theta;
    rv= cos_theta;

    // ne renvoie vrai que si l'intersection est valide : 
	// interieur du triangle, 0 < u < 1, 0 < v < 1, 0 < u+v < 1
	if(any(greaterThan(vec3(u, v, u+v), vec3(1, 1, 1))) || any(lessThan(vec2(u, v), vec2(0, 0))))
		return false;
	// comprise entre 0 et tmax du rayon
        return (rt < tmax && rt > 0);
}

bool intersectSphere( const in vec3 o, const in vec3 d, const float tmax, const in vec3 center, const in float radius, out float rt)
{
    vec3 oc= o + center;
    float b= dot(oc, d);
    float c= dot(oc, oc) - radius*radius;
    float h= b*b - 1.0 * c;
    //rt = inf;
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

    return (rt < tmax && rt > 0);
}


uniform mat4 mvpInvMatrix;
uniform mat4 invViewport;
uniform int triangle_count;
layout(binding=0, rgba8) uniform image2D image;


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
    float hitu= 0;
    float hitv= 0;
    int hitid= 0;
    for(int i= 0; i < triangle_count; i++)
    {
        float t, u, v;
        if(intersectTriangle(triangles[i], o, d, hit, t, u, v))
        {
            hit= t;
            hitu= u;
            hitv= v;
            hitid= i;
        }
        if (intersectSphere(o, d, hit, vec3(0, 0, 0), 0.5, t)) {
            hit = t;
            hitu= 1;
            hitv= 1;
            hitid= i;
        }
    }
    
    imageStore(image, ivec2(pix.xy), vec4(hitu, hitv, hitv, 1));
}

#endif