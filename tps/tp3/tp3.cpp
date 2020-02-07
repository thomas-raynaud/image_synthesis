
#include <cfloat>
#include <cmath>
#include <algorithm>
#include <stack>

#include "app_time.h"

#include "vec.h"
#include "color.h"
#include "mat.h"

#include "mesh.h"
#include "wavefront.h"

#include "program.h"
#include "uniforms.h"

#include "orbiter.h"

// cf tuto_storage
namespace glsl 
{
    template < typename T >
    struct alignas(8) gvec2
    {
        alignas(4) T x, y;
        
        gvec2( ) {}
        gvec2( const vec2& v ) : x(v.x), y(v.y) {}
    };
    
    typedef gvec2<float> vec2;
    typedef gvec2<int> ivec2;
    typedef gvec2<unsigned int> uvec2;
    typedef gvec2<int> bvec2;
    
    template < typename T >
    struct alignas(16) gvec3
    {
        alignas(4) T x, y, z;
        
        gvec3( ) {}
        gvec3( const vec3& v ) : x(v.x), y(v.y), z(v.z) {}
        gvec3( const Point& v ) : x(v.x), y(v.y), z(v.z) {}
        gvec3( const Vector& v ) : x(v.x), y(v.y), z(v.z) {}
    };
    
    typedef gvec3<float> vec3;
    typedef gvec3<int> ivec3;
    typedef gvec3<unsigned int> uvec3;
    typedef gvec3<int> bvec3;
    
    template < typename T >
    struct alignas(16) gvec4
    {
        alignas(4) T x, y, z, w;
        
        gvec4( ) {}
        gvec4( const vec4& v ) : x(v.x), y(v.y), z(v.z), w(v.w) {}
    };
    
    typedef gvec4<float> vec4;
    typedef gvec4<int> ivec4;
    typedef gvec4<unsigned int> uvec4;
    typedef gvec4<int> bvec4;
}

struct triangle {
    glsl::vec3 a;
    glsl::vec3 ab;
    glsl::vec3 ac;
};

struct Node {
    vec3 bmin;
    int g;
    vec3 bmax;
    int d;
};

Point getPoint(glsl::vec3 p) {
    return Point(p.x, p.y, p.z);
};

Point centroid_triangle(const triangle &t) {
    Point a, b, c;
    a = getPoint(t.a);
    b = a + getPoint(t.ab);
    c = a + getPoint(t.ac);
    return Point(
            (a(0) + b(0) + c(0)) / 3.0f,
            (a(1) + b(1) + c(1)) / 3.0f,
            (a(2) + b(2) + c(2)) / 3.0f
    );
}

// Trouve les points min et max de la boite englobante des triangles d'indice entre begin et end
void bounds(std::vector<triangle> & triangles, const int begin, const int end, Point & bmin, Point & bmax) {
    Point a, b, c;
    bmin = getPoint(triangles[begin].a);
    bmax = getPoint(triangles[begin].a);
    //std::cout << begin << " " << end << std::endl;
    for (int i = begin; i < end; ++i) {
        a = getPoint(triangles[i].a);
        b = a + getPoint(triangles[i].ab);
        c = a + getPoint(triangles[i].ac);
        //std::cout << a << " " << b << " " << c << std::endl;
        bmin = min(bmin, min(a, min(b, c)));
        bmax = max(bmax, max(a, max(b, c)));
    }
    //std::cout << triangles.size() << " " << begin << " " << end << std::endl;
}

// boite englobante des centres 
void centroid_bounds(std::vector<triangle> & triangles, int begin, int end, Point & cmin, Point & cmax) {
    cmin = centroid_triangle(triangles[begin]);
    cmax = centroid_triangle(triangles[begin]);
    for (int i = begin; i < end; ++i) {
        cmin = min(cmin, centroid_triangle(triangles[i]));
        cmax = max(cmax, centroid_triangle(triangles[i]));
    }
}

// Retourne l'axe où cmin et cmax sont les plus éloignés 
int bounds_max(Point cmin, Point cmax) {
    Point center = Point(
        cmax.x - cmin.x,
        cmax.y - cmin.y,
        cmax.z - cmin.z
    );
    int axis = 0;
    if (center(axis) > center.y) axis = 1;
    if (center(axis) > center.z) axis = 2;
    return axis;
}

// Foncteur
struct centroid_less {
    centroid_less(int _axis, float _center_box_axis): axis(_axis), center_box_axis(_center_box_axis) { }
    bool operator() (triangle &x) {
        return centroid_triangle(x)(axis) < center_box_axis;
    }
    int axis;
    float center_box_axis;
};

int build_node_centroids(std::vector<triangle> & triangles, std::vector<Node> & nodes, const int begin, const int end) {
    Point bmin, bmax;
    bounds(triangles, begin, end, bmin, bmax); //  englobant
    Point cmin, cmax;
    if(end - begin < 2) { // 1 triangle, construire une feuille
        nodes.push_back( {(vec3)bmin, -(begin + 1), (vec3)bmax, -(end + 1)} );
        return int(nodes.size()) -1;
    }
    centroid_bounds(triangles, begin, end, cmin, cmax); // englobant des centres
    
    int axis = bounds_max(cmin, cmax);
    float center_box_axis = (cmax(axis) + cmin(axis)) / 2.0f;
    int m = std::distance(triangles.data(), // repartir les triangles
                std::partition(
                    triangles.data() + begin, triangles.data() + end, centroid_less(axis, center_box_axis)
                )
        );
    if (begin == m) {
        m = begin + 1;
    } else if (end == m) {
        m = end - 1;
    }

    // construire le noeud
    int left = -1;
    int right = -1;
    int node_id = nodes.size();
    nodes.push_back( {(vec3)bmin, left, (vec3)bmax, right} );

    left = build_node_centroids(triangles, nodes, begin, m); //  construire  les  fils  du  noeud
    right = build_node_centroids(triangles, nodes, m, end);

    nodes[node_id].g = left;
    nodes[node_id].d = right;
    
    return node_id;
}

void build_bvh_cousu(std::vector<Node> & nodes, std::vector<Node> & nodesCousus, int root) {
    // fils gauche = fils
    // fils droit = successeur
    // Racine
    std::stack<int> successeurs;
    int fils = nodes[root].g;
    int droite = nodes[root].d;
    int next = -1;
    nodesCousus.push_back( {nodes[root].bmin, fils, nodes[root].bmax, next} );
    next = droite;
    successeurs.push(droite);
    int current_node = fils;
    while (current_node != -1) {
        fils = nodes[current_node].g;
        droite = nodes[current_node].d;
        nodesCousus.push_back( {nodes[current_node].bmin, fils, nodes[current_node].bmax, next} );
        next = droite;
        
        if (next >= 0) {
            successeurs.push(next);
            current_node = fils;
        } else {
            if (successeurs.empty()) {
                current_node = -1;
            }
            else {
                next = successeurs.top();
                successeurs.pop();
                current_node = next;
                if (successeurs.empty())
                    next = -1;
                else
                    next = successeurs.top();
            }
            
        }
    }
}

struct RT : public AppTime
{
    // constructeur : donner les dimensions de l'image, et eventuellement la version d'openGL.
    RT( const char *filename ) : AppTime(1024, 640) 
    {
        m_mesh = read_mesh(filename);
    }
    
    int init( )
    {
        if(m_mesh == Mesh::error())
            return -1;
        
        Point pmin, pmax;
        m_mesh.bounds(pmin, pmax);
        m_camera.lookat(pmin, pmax);
        
        glGenVertexArrays(1, &m_vao);
        glBindVertexArray(m_vao);

        // Texture
        glGenTextures(1, &m_texture);
        glBindTexture(GL_TEXTURE_2D, m_texture);
        
        // fixe les parametres de filtrage par defaut
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, window_width(), window_height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glBindTexture(GL_TEXTURE_2D, 0);

        // Texture de bruit
        glGenTextures(1, &m_texture_bruit);
        glBindTexture(GL_TEXTURE_2D, m_texture_bruit);

        // fixe les parametres de filtrage par defaut
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI, window_width(), window_height(), 0, GL_RED_INTEGER, GL_UNSIGNED_INT, nullptr);
        glBindTexture(GL_TEXTURE_2D, 0);

        // Triangles
        // storage buffers
        glGenBuffers(1, &m_triangle_buffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_triangle_buffer);

        std::vector<triangle> dataTriangle;
        dataTriangle.reserve(m_mesh.triangle_count());
        for(int i= 0; i < m_mesh.triangle_count(); i++)
        {
            TriangleData t= m_mesh.triangle(i);
            dataTriangle.push_back( { Point(t.a), Point(t.b) - Point(t.a), Point(t.c) - Point(t.a) } );
        }
        
        // alloue le buffer
        // alloue au moins 1024 triangles, cf le shader
        //if(dataTriangle.size() < 1024)
        //    dataTriangle.resize(1024);
        glBufferData(GL_SHADER_STORAGE_BUFFER, dataTriangle.size() * sizeof(triangle), dataTriangle.data(), GL_STATIC_READ);

        // BVH des triangles
        std::vector<Node> bvh, bvh_cousu;
        int root = build_node_centroids(dataTriangle, bvh, 0, dataTriangle.size());
        build_bvh_cousu(bvh, bvh_cousu, root);

        for (uint i = 0; i < bvh_cousu.size(); ++i) {
            std::string fils;
            if (bvh_cousu[i].g < 0) fils = "(" + std::to_string(abs(bvh_cousu[i].g) - 1) + ")";
            else fils = " " + std::to_string(bvh_cousu[i].g) + " ";
            std::cout << i << " - F = " << fils << ", N = " << bvh_cousu[i].d << std::endl;
            Point bmin = getPoint(bvh_cousu[i].bmin);
            Point bmax = getPoint(bvh_cousu[i].bmax);
            std::cout << bmin << " - " << bmax << std::endl;
            std::cout << std::endl;
        }

        glGenBuffers(1, &m_nodes_buffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_nodes_buffer);
        
        glBufferData(GL_SHADER_STORAGE_BUFFER, bvh_cousu.size() * sizeof(Node), bvh_cousu.data(), GL_STATIC_READ);

        // Spheres
        glGenBuffers(1, &m_sphere_buffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_sphere_buffer);

        std::vector<glsl::vec4> dataSphere;
        dataSphere.reserve(2);
        dataSphere.push_back({vec4(-0.7, 0.15, 0.4, 0.15)});
        dataSphere.push_back({vec4(-0.3, 1.29, -0.2, 0.08)});
        
        // alloue le buffer
        // alloue au moins 10 spheres, cf le shader
        //if(dataSphere.size() < 10)
        //    dataSphere.resize(10);
        glBufferData(GL_SHADER_STORAGE_BUFFER, dataSphere.size() * sizeof(glsl::vec4), dataSphere.data(), GL_STATIC_READ);

        m_program = read_program("tps/tp3/trace_cp.glsl");

        program_print_errors(m_program);

        // Framebuffer
        glGenFramebuffers(1, &m_framebuffer);

        glViewport(0, 0, window_width(), window_height());
        
        // On bind la texture
        glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texture, 0);

        return 0;
    }
    
    int quit( )
    {
        glDeleteFramebuffers(1, &m_framebuffer);
        return 0;
    }
    
    int render( )
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if(key_state('f'))
        {
            clear_key_state('f');
            Point pmin, pmax;
            m_mesh.bounds(pmin, pmax);
            m_camera.lookat(pmin, pmax);        
        }
        
        // deplace la camera
        int mx, my;
        unsigned int mb= SDL_GetRelativeMouseState(&mx, &my);
        if(mb & SDL_BUTTON(1))              // le bouton gauche est enfonce
            m_camera.rotation(mx, my);
        else if(mb & SDL_BUTTON(3))         // le bouton droit est enfonce
            m_camera.move(mx);
        else if(mb & SDL_BUTTON(2))         // le bouton du milieu est enfonce
            m_camera.translation((float) mx / (float) window_width(), (float) my / (float) window_height());

        
        Transform m;
        Transform v = m_camera.view();
        Transform p = m_camera.projection(window_width(), window_height(), 45);
        Transform mvp = p * v * m;

        glUseProgram(m_program);
        program_uniform(m_program, "mvpInvMatrix", mvp.inverse());
        program_uniform(m_program, "invViewport", Viewport(window_width(), window_height()).inverse());
        program_uniform(m_program, "triangle_count", m_mesh.triangle_count());
        program_uniform(m_program, "sphere_count", 2);

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_triangle_buffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_nodes_buffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_sphere_buffer);

        glBindImageTexture(0, m_texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);

        
        // calcule le nombre de groupes de threads
        int nt = window_width() / 8;
        int mt = window_height() / 8;
        if (window_width() % 8) nt = nt + 1;
        if (window_height() % 8) mt = mt + 1;

        glDispatchCompute(nt, mt, 1);

        // synchronisation
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        glBindFramebuffer(GL_READ_FRAMEBUFFER, m_framebuffer);
        glViewport(0, 0, window_width(), window_height());

        glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                    GL_TEXTURE_2D, m_texture, 0);
        

        // On copie la texture dans le framebuffer par défaut
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glBlitFramebuffer(0, 0, window_width(), window_height(),
                        0, 0, window_width(), window_height(),
                        GL_COLOR_BUFFER_BIT, GL_LINEAR);
        
        // Reset image binding.
		//glBindImageTexture(0, 0, 0, false, 0, GL_READ_WRITE, GL_RGBA8UI);
        
        return 1;
    }
    
protected:
    Mesh m_mesh;
    Orbiter m_camera;

    GLuint m_program;
    GLuint m_vao;
    GLuint m_triangle_buffer;
    GLuint m_nodes_buffer; // BVH cousu
    GLuint m_sphere_buffer;
    GLuint m_framebuffer;

    GLuint m_texture;
    GLuint m_texture_bruit;
};

    
int main( int argc, char **argv ) {
    const char *filename= "tps/tp3/cornell.obj";
    if(argc > 1)
        filename= argv[1];
    
    RT app(filename);
    app.run();
    
    return 0;
}