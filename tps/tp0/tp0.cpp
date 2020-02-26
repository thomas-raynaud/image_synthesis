
//! \file tuto7.cpp reprise de tuto6.cpp mais en derivant App::init(), App::quit() et bien sur App::render().

#include "wavefront.h"
#include "texture.h"

#include "orbiter.h"
#include "draw.h"        
#include "app.h"        // classe Application a deriver
#include "app_time.h"
#include "program.h"
#include "uniforms.h"

struct Buffers
{
    GLuint vao;
    GLuint vertex_buffer;
    GLuint texture_buffer;
    int vertex_count;

    Buffers( ) : vao(0), vertex_buffer(0), vertex_count(0) {}
    
    void create( const Mesh& mesh,  const GLuint texture)
    {
        if(!mesh.vertex_buffer_size()) return;
        
        // cree et initialise le buffer: conserve la positions des sommets
        size_t offset= 0;
        size_t size= mesh.vertex_buffer_size();
        glGenBuffers(1, &vertex_buffer);
        glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
        glBufferData(GL_ARRAY_BUFFER, mesh.vertex_buffer_size(), mesh.vertex_buffer(), GL_STATIC_DRAW);

        
        
        // cree et configure le vertex array object: conserve la description des attributs de sommets
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        
        // attribut 0, position des sommets, declare dans le vertex shader : in vec3 position;
        glVertexAttribPointer(0, 
            3, GL_FLOAT,    // size et type, position est un vec3 dans le vertex shader
            GL_FALSE,       // pas de normalisation des valeurs
            0,              // stride 0, les valeurs sont les unes a la suite des autres
            0               // offset 0, les valeurs sont au debut du buffer
        );
        glEnableVertexAttribArray(0);

        // Textures
        glGenBuffers(1, &texture_buffer);
        glBindBuffer(GL_ARRAY_BUFFER, texture_buffer);
        glBufferData(GL_ARRAY_BUFFER, mesh.texcoord_buffer_size(), mesh.texcoord_buffer(), GL_STATIC_DRAW);

        // attribut 1, textures
        glVertexAttribPointer(1, 
            2, GL_FLOAT,    // size et type, position est un vec3 dans le vertex shader
            GL_FALSE,       // pas de normalisation des valeurs
            0,              // stride 0, les valeurs sont les unes a la suite des autres
            0             // offset 0, les valeurs sont au debut du buffer
        );
        glEnableVertexAttribArray(1);
        
        // conserve le nombre de sommets
        vertex_count= mesh.vertex_count();

        // nettoyage
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    
    void release( )
    {
        glDeleteBuffers(1, &vertex_buffer);
        glDeleteBuffers(1, &texture_buffer);
        glDeleteVertexArrays(1, &vao);
    }
};


class TP : public AppTime
{
public:
    // constructeur : donner les dimensions de l'image, et eventuellement la version d'openGL.
    TP( ) : AppTime(1024, 640) {}
    
    // creation des objets de l'application
    int init( )
    {
        m_texture= read_texture(0, "data/debug2x2red.png");
        // creer le vertex buffer et le vao
        m_objet.create(read_mesh("data/cube.obj"), m_texture);
        m_camera.lookat(Point(-1, -1, -1), Point(10, 10, 1));

        m_program= read_program("tps/tp0/tp0.glsl");
        program_print_errors(m_program);

        

        // etat openGL par defaut
        glClearColor(0.2f, 0.2f, 0.2f, 1.f);        // couleur par defaut de la fenetre
        
        glClearDepth(1.f);                          // profondeur par defaut
        glDepthFunc(GL_LESS);                       // ztest, conserver l'intersection la plus proche de la camera
        glEnable(GL_DEPTH_TEST);                    // activer le ztest

        return 0;   // ras, pas d'erreur
    }
    
    // destruction des objets de l'application
    int quit()
    {
        release_program(m_program);
        m_objet.release();
        glDeleteTextures(1, &m_texture);
        
        return 0;
    }
    
    // dessiner une nouvelle image
    int render( )
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // deplace la camera
        int mx, my;
        unsigned int mb= SDL_GetRelativeMouseState(&mx, &my);
        if(mb & SDL_BUTTON(1))              // le bouton gauche est enfonce
            m_camera.rotation(mx, my);
        else if(mb & SDL_BUTTON(3))         // le bouton droit est enfonce
            m_camera.move(mx);
        else if(mb & SDL_BUTTON(2))         // le bouton du milieu est enfonce
            m_camera.translation((float) mx / (float) window_width(), (float) my / (float) window_height());

        glBindVertexArray(m_objet.vao);

        glUseProgram(m_program);
        Transform projection= m_camera.projection(window_width(), window_height(), 45);
        Transform mvp, vp = projection * m_camera.view();
        
        
        //draw(m_objet, m_camera, m_texture);
        Transform model;
        double nbBoxes = 100;
        for (int i = 0; i < sqrt(nbBoxes); ++i) {
            for (int j = 0; j < sqrt(nbBoxes); ++j) {
                model = Translation(vec3(i * 2, j * 2, 0));
                mvp = vp * model;
                program_uniform(m_program, "mvpMatrix", mvp);
                program_use_texture(m_program, "color_texture", 0, m_texture, 0);
                //draw(m_objet, m_program);
                //draw(m_objet, model, m_camera, m_texture);
                // selectionner les attributs et les buffers de l'objet
                glBindVertexArray(m_objet.vao);
                
                // dessiner les triangles de l'objet
                glDrawArrays(GL_TRIANGLES, 0, m_objet.vertex_count);
            }
        }
        // nettoyage
        glUseProgram(0);
        glBindVertexArray(0);
        return 1;
    }

protected:
    Buffers m_objet;
    GLuint m_texture;
    GLuint m_program;
    Orbiter m_camera;
    
};


int main( int argc, char **argv )
{
    // il ne reste plus qu'a creer un objet application et la lancer 
    TP tp;
    tp.run();
    
    return 0;
}
