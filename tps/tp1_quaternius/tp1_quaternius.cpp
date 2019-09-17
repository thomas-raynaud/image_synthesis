
//! \file tuto7.cpp reprise de tuto6.cpp mais en derivant App::init(), App::quit() et bien sur App::render().

#include "wavefront.h"
#include "texture.h"

#include "orbiter.h"
#include "draw.h"        
#include "app.h"        // classe Application a deriver
#include "app_time.h"
#include "program.h"
#include "uniforms.h"

#define NB_MESHES 24

struct Buffers
{
    GLuint vao;
    GLuint vertex_buffer1;
    GLuint vertex_buffer2;
    GLuint normal_buffer1;
    GLuint normal_buffer2;
    GLuint material_buffer;
    GLuint material_index_buffer;
    Mesh meshes[NB_MESHES];
    int vertex_count;

    Buffers( ) : vao(0), vertex_buffer1(0), vertex_buffer2(0),
                 normal_buffer1(0), normal_buffer2(0),
                 material_buffer(0), material_index_buffer(0),
                 vertex_count(0) {}

    void create() {
        glGenBuffers(1, &vertex_buffer1);
        glGenBuffers(1, &vertex_buffer2);
        glGenBuffers(1, &normal_buffer1);
        glGenBuffers(1, &normal_buffer2);
        glGenBuffers(1, &material_buffer);
        glGenBuffers(1, &material_index_buffer);
        glGenVertexArrays(1, &vao);

        // Materiaux
        // creer et remplir le buffer avec les parametres des matieres.
        glBindBuffer(GL_UNIFORM_BUFFER, material_buffer);
        vec3 material_colors[meshes[0].mesh_materials().size()];
        Color color_temp;
        for (unsigned int i = 0; i < meshes[0].mesh_materials().size(); ++i) {
            color_temp = meshes[0].mesh_materials()[i].diffuse;
            material_colors[i] = vec3(color_temp.r, color_temp.g, color_temp.b);
        }
        glBufferData(GL_UNIFORM_BUFFER, meshes[0].mesh_materials().size() * sizeof(vec3), material_colors, GL_STATIC_DRAW);
       
        // creer et remplir le buffer contenant l'indice de la matiere de chaque triangle
        glBindBuffer(GL_UNIFORM_BUFFER, material_index_buffer);
        GLuint material_triangle_index[meshes[0].materials().size()];
        for (unsigned int i = 0; i < meshes[0].materials().size(); ++i) {
            material_triangle_index[i] = meshes[0].materials()[i];
        }
        glBufferData(GL_UNIFORM_BUFFER, meshes[0].materials().size(), material_triangle_index, GL_STATIC_DRAW);
    }
    
    void loadMeshes(const int ind_mesh1, const int ind_mesh2)
    {
        if(!meshes[ind_mesh1].vertex_buffer_size() || !meshes[ind_mesh2].vertex_buffer_size()) return;

        // configure le vertex array object: conserve la description des attributs de sommets
        glBindVertexArray(vao);
        
        // initialise le buffer du mesh 1: conserve la positions des sommets
        glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer1);
        glBufferData(GL_ARRAY_BUFFER, meshes[ind_mesh1].vertex_buffer_size(), meshes[ind_mesh1].vertex_buffer(), GL_STATIC_DRAW);
        // attribut 0, position des sommets du mesh 1, declare dans le vertex shader : in vec3 position1;
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(0);

        // initialise le buffer du mesh 1: normales
        glBindBuffer(GL_ARRAY_BUFFER, normal_buffer1);
        glBufferData(GL_ARRAY_BUFFER, meshes[ind_mesh1].normal_buffer_size(), meshes[ind_mesh1].normal_buffer(), GL_STATIC_DRAW);
        // attribut 1, position des sommets du mesh 1, declare dans le vertex shader : in vec3 position1;
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(1);

        // initialise le buffer du mesh 2: conserve la positions des sommets
        glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer2);
        glBufferData(GL_ARRAY_BUFFER, meshes[ind_mesh2].vertex_buffer_size(), meshes[ind_mesh2].vertex_buffer(), GL_STATIC_DRAW);
        // attribut 2, position des sommets du mesh 2, declare dans le vertex shader : in vec3 position2;
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(2);

        // initialise le buffer du mesh 2: normales
        glBindBuffer(GL_ARRAY_BUFFER, normal_buffer2);
        glBufferData(GL_ARRAY_BUFFER, meshes[ind_mesh2].normal_buffer_size(), meshes[ind_mesh2].normal_buffer(), GL_STATIC_DRAW);
        // attribut 1, position des sommets du mesh 1, declare dans le vertex shader : in vec3 position1;
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(3);
        
        // conserve le nombre de sommets
        vertex_count = meshes[ind_mesh1].vertex_count();

        // nettoyage
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    
    void release( )
    {
        for (int i = 0; i < NB_MESHES; ++i) {
            meshes[i].release();
        }
        glDeleteBuffers(1, &vertex_buffer1);
        glDeleteBuffers(1, &vertex_buffer2);
        glDeleteBuffers(1, &normal_buffer1);
        glDeleteBuffers(1, &normal_buffer2);
        glDeleteBuffers(1, &material_buffer);
        glDeleteBuffers(1, &material_index_buffer);
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
        m_objet.meshes[0] = read_mesh("robot_quaternius/Robot.obj");
        // charger les meshes
        std::string mesh_path;
        for (int i = 1; i < NB_MESHES; ++i) {
            mesh_path.clear();
            mesh_path = "robot_quaternius/run/Robot_0000";
            if (i < 10) mesh_path += "0";
            mesh_path += std::to_string(i) + ".obj";
            m_objet.meshes[i] = read_mesh(mesh_path.c_str());

        }
        // charger les vertex buffers et le vao
        m_objet.create();
        m_camera.lookat(Point(0, 2, 5), Point(0, 2, 0));

        m_program = read_program("tps/tp1_quaternius/tp1_quaternius.glsl");
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
        
        return 0;
    }
    
    // dessiner une nouvelle image
    int render()
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        float gt = global_time();
        // deplace la camera
        int mx, my;
        unsigned int mb= SDL_GetRelativeMouseState(&mx, &my);
        if(mb & SDL_BUTTON(1))              // le bouton gauche est enfonce
            m_camera.rotation(mx, my);
        else if(mb & SDL_BUTTON(3))         // le bouton droit est enfonce
            m_camera.move(mx);
        else if(mb & SDL_BUTTON(2))         // le bouton du milieu est enfonce
            m_camera.translation((float) mx / (float) window_width(), (float) my / (float) window_height());

        glUseProgram(m_program);
        Transform projection= m_camera.projection(window_width(), window_height(), 45);
        Transform mvp = projection * m_camera.view() * Identity();
        program_uniform(m_program, "mMatrix", Identity());
        program_uniform(m_program, "mvpMatrix", mvp);
        program_uniform(m_program, "normalMatrix", Identity().normal());
        program_uniform(m_program, "light", vec3(10, 10, 5));
        float speed = 12000.f;
        int t0 = (gt / (speed / 24.f));
        int t1 = t0 + 1;
        int kf1 = t0 % 23 + 1;
        int kf2 = t1 % 23 + 1;
        float t = (gt / (speed / 24.f));
        float dt = (t - t0) / (t1 - t0);
        m_objet.loadMeshes(kf1, kf2);
        float time = ((int)t % 1000) / speed;
        program_uniform(m_program, "time", dt);

        // recuperer l'identifiant du block a associer au buffer
        GLuint material_block = glGetUniformBlockIndex(m_program, "MaterialBlock");
        // les uniform blocks sont numerotes, et c'est l'application qui doit choisir le numero...
        // associe le numero 0 au bloc "MaterialBLock"
        glUniformBlockBinding(m_program, material_block, 0);      
        // associe le contenu d'un buffer au block numero 0
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_objet.material_buffer);

        // on recommence pour les indices, avec un autre numero
        GLuint index_block = glGetUniformBlockIndex(m_program, "IndexBlock");
        glUniformBlockBinding(m_program, index_block, 1);
        glBindBufferBase(GL_UNIFORM_BUFFER, 1, m_objet.material_index_buffer);

        // selectionner les attributs et les buffers de l'objet
        glBindVertexArray(m_objet.vao);
        
        // dessiner les triangles de l'objet
        glDrawArrays(GL_TRIANGLES, 0, m_objet.vertex_count);

        // nettoyage
        glUseProgram(0);
        glBindVertexArray(0);
        return 1;
    }

protected:
    Buffers m_objet;
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
