
#include <cfloat>
#include <cmath>
#include <algorithm>

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


struct RT : public AppTime
{
    // constructeur : donner les dimensions de l'image, et eventuellement la version d'openGL.
    RT( const char *filename ) : AppTime(1024, 640) 
    {
        m_mesh= read_mesh(filename);
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

        // transfere les donnees dans la texture, 4 float par texel
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, window_width(), window_height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glBindTexture(GL_TEXTURE_2D, 0);

        // Triangles
        // storage buffers
        /*glGenBuffers(1, &m_raybuffer);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_raybuffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float) * window_width() * window_height(), nullptr, GL_STREAM_COPY);*/
        glGenBuffers(1, &m_buffer);
        glBindBuffer(GL_UNIFORM_BUFFER, m_buffer);
        
        struct triangle 
        {
            glsl::vec3 a;
            glsl::vec3 ab;
            glsl::vec3 ac;
        };
        
        std::vector<triangle> data;
        data.reserve(m_mesh.triangle_count());
        for(int i= 0; i < m_mesh.triangle_count(); i++)
        {
            TriangleData t= m_mesh.triangle(i);
            data.push_back( { Point(t.a), Point(t.b) - Point(t.a), Point(t.c) - Point(t.a) } );
        }
        
        // alloue le buffer
        // alloue au moins 1024 triangles, cf le shader
        if(data.size() < 1024)
            data.resize(1024);
        glBufferData(GL_UNIFORM_BUFFER, data.size() * sizeof(triangle), data.data(), GL_STATIC_READ);

        m_compute_program= read_program("tps/tp3/trace_cp.glsl");

        program_print_errors(m_compute_program);

        // Framebuffer
        glGenFramebuffers(1, &m_framebuffer);

        // associe l'uniform buffer a l'entree 0
        GLint index= glGetUniformBlockIndex(m_compute_program, "triangleData");
        glUniformBlockBinding(m_compute_program, index, 0);
        
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

        glUseProgram(m_compute_program);
        program_uniform(m_compute_program, "mvpInvMatrix", mvp.inverse());
        program_uniform(m_compute_program, "invViewport", Viewport(window_width(), window_height()).inverse());
        /*Transform tmp = Viewport(window_width(), window_height()).inverse();

        Vector p1 = tmp(Vector(0, 0, 0)) - Vector(1, 1, 1);
        Vector p2 = tmp(Vector(window_width(), window_height(), 0)) - Vector(1, 1, 1);
        std::cout << p1 << std::endl;
        std::cout << p2 << std::endl;
        p1 = mvp.inverse()(p1);
        p2 = mvp.inverse()(p2);
        std::cout << p1 << std::endl;
        std::cout << p2 << std::endl << std::endl;*/
        program_uniform(m_compute_program, "triangle_count", m_mesh.triangle_count());

        glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_buffer);

        glBindImageTexture(0, m_texture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);

        
        // calcule le nombre de groupes de threads
        int nt = window_width() / 8;
        int mt = window_height() / 8;
        if (window_width() % 8) nt = nt + 1;
        if (window_height() % 8) mt = mt + 1;

        glDispatchCompute(nt, mt, 1);

        // etape 2 : synchronisation
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        //glBindTexture(GL_TEXTURE_2D, m_texture);

        glBindFramebuffer(GL_READ_FRAMEBUFFER, m_framebuffer);
        glViewport(0, 0, window_width(), window_height());
        
        glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                            GL_TEXTURE_2D, m_texture, 0);

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBlitFramebuffer(0, 0, window_width(), window_height(),
                        0, 0, window_width(), window_height(),
                        GL_COLOR_BUFFER_BIT, GL_LINEAR);
        
        //glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		//glDrawArrays(GL_TRIANGLES, 0, 3);
        
        /* Reset image binding. */
		glBindImageTexture(0, 0, 0, false, 0, GL_READ_WRITE, GL_RGBA8UI);
        
        return 1;
    }
    
protected:
    Mesh m_mesh;
    Orbiter m_camera;

    GLuint m_compute_program;
    GLuint m_vao;
    GLuint m_buffer;
    GLuint m_framebuffer;

    GLuint m_texture;
};

    
int main( int argc, char **argv )
{
    const char *filename= "tps/tp3/cornell.obj";
    if(argc > 1)
        filename= argv[1];
    
    RT app(filename);
    app.run();
    
    return 0;
}
