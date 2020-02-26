
#include "wavefront.h"
#include "texture.h"

#include "orbiter.h"
#include "draw.h"        
#include "app.h"
#include "app_time.h"
#include "program.h"
#include "uniforms.h"
#include "quaternius.h"

#include "ortho.h"
#include "gen_cube.h"


class TP : public App {
public:
    // constructeur : donner les dimensions de l'image, et eventuellement la version d'openGL.
    TP() : App(1024, 640) {}
    
    // creation des objets de l'application
    int init( ) {
        m_framebuffer_width  = 1024 * 4;
        m_framebuffer_height = 1024 * 4;

        glGenTextures(1, &m_zbuffer);
        glBindTexture(GL_TEXTURE_2D, m_zbuffer); // sélectionner la texture
        glTexImage2D(GL_TEXTURE_2D, 0,
            GL_DEPTH_COMPONENT, m_framebuffer_width, m_framebuffer_height, 0,
            GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);


        glGenFramebuffers(1, &m_framebuffer);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_framebuffer);
        glFramebufferTexture(GL_DRAW_FRAMEBUFFER, /* attachment */ GL_DEPTH_ATTACHMENT, /* texture */ m_zbuffer, /* mipmap level */ 0);

        // nettoyage
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

        m_camera.lookat(Point(0, 10, -7), Point(0, 0, 7));
        m_camera.rotation(-90, 0);
        m_light_pos = Point(10, 10, 0);

        m_projection = Perspective(45, (float)window_width() / (float)window_height(), 0.01, 1000);
        m_sourceViewProjection = Ortho(-25, 25, -25, 25, 0.1, 1000)                         // Projection
                                    * Lookat(m_light_pos, Point(0, 0, 0), Vector(0, 1, 0)); // Vue

        // Objets de la scene.
        glGenBuffers(1, &m_vertex_buffer_obj);
        glGenVertexArrays(1, &m_vao_obj);
        glBindVertexArray(m_vao_obj);
        Mesh mesh;
        mesh = create_mesh(Point(0, -0.5, 0), 50, 1, 50);
        m_vertex_count_obj = mesh.vertex_count();
        glBindBuffer(GL_ARRAY_BUFFER, m_vertex_buffer_obj);
        glBufferData(GL_ARRAY_BUFFER, m_vertex_count_obj * sizeof(vec3) * 2, nullptr, GL_STATIC_DRAW);
        // Position des sommets
        glBufferSubData(GL_ARRAY_BUFFER, 0, mesh.vertex_buffer_size(), mesh.vertex_buffer());
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (const GLvoid *) 0);
        glEnableVertexAttribArray(0);
        // Normales
        glBufferSubData(GL_ARRAY_BUFFER, mesh.vertex_buffer_size(), mesh.normal_buffer_size(), mesh.normal_buffer());
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (const GLvoid *) (m_vertex_count_obj * sizeof(vec3)));
        glEnableVertexAttribArray(1);


        // Quaternius
        mesh = read_mesh("robot_quaternius/run/Robot_000001.obj");

        m_quaternius1 = new Quaternius(200.f, m_projection * m_camera.view(), m_sourceViewProjection);
        m_quaternius2 = new Quaternius(500.f, m_projection * m_camera.view(), m_sourceViewProjection);

        std::string definitions;
        definitions.append("#define NB_MATERIALS " + std::to_string(mesh.mesh_materials().size() * 2));
        definitions.append("\n");

        m_program           = read_program("tps/tp1/quaternius.glsl", definitions.c_str());
        m_program_shadowmap = read_program("tps/tp1/shadowmap.glsl");
        m_program_object    = read_program("tps/tp1/object.glsl");
        program_print_errors(m_program);

        mesh.release();

        glBindVertexArray(0);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // etat openGL par defaut
        glClearColor(0.2f, 0.2f, 0.2f, 1.f);        // couleur par defaut de la fenetre
        
        glClearDepth(1.f);                          // profondeur par defaut
        glDepthFunc(GL_LESS);                       // ztest, conserver l'intersection la plus proche de la camera
        glEnable(GL_DEPTH_TEST);                    // activer le ztest

        return 0;
    }
    
    // Destruction des objets de l'application
    int quit() {
        release_program(m_program);
        release_program(m_program_shadowmap);
        release_program(m_program_object);

        glDeleteBuffers(1, &m_vertex_buffer_obj);
        glDeleteTextures(1, &m_zbuffer);
        glDeleteVertexArrays(1, &m_vao_obj);
        glDeleteFramebuffers(1, &m_framebuffer);
        delete m_quaternius1;
        delete m_quaternius2;
        return 0;
    }
    
    // Dessiner une nouvelle image
    int render() {
        float gt = global_time();
        // déplacer la camera
        int mx, my;
        unsigned int mb = SDL_GetRelativeMouseState(&mx, &my);
        if(mb & SDL_BUTTON(1))              // le bouton gauche est enfoncé
            m_camera.rotation(mx, my);
        else if(mb & SDL_BUTTON(3))         // le bouton droit est enfoncé
            m_camera.move(mx);
        else if(mb & SDL_BUTTON(2))         // le bouton du milieu est enfoncé
            m_camera.translation((float) mx / (float) window_width(), (float) my / (float) window_height());


        // Matrices
        Transform model_quaternius1 = RotationY(-gt / 36) * Translation(2, 0, 0);
        Transform model_quaternius2 = RotationY(gt / 36) * Translation(6, 0, 0) * RotationY(180);
        Transform vp = m_projection * m_camera.view();


        // SHADOW MAP
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_framebuffer);
        glViewport(0, 0, m_framebuffer_width, m_framebuffer_height);
        glClearColor(0.2, 0.2, 0.2, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(m_program_shadowmap);

        m_quaternius1->render_shadowmap(m_program_shadowmap, gt, m_sourceViewProjection * model_quaternius1);
        m_quaternius2->render_shadowmap(m_program_shadowmap, gt, m_sourceViewProjection * model_quaternius2);

        // Dessiner la scène
        glBindVertexArray(m_vao_obj);
        program_uniform(m_program_shadowmap, "mvpMatrix", m_sourceViewProjection * Identity());
        program_uniform(m_program_shadowmap, "useInterpolation", 0);

        glDrawArrays(GL_TRIANGLES, 0, m_vertex_count_obj);


        // FRAMEBUFFER PAR DEFAUT
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glViewport(0, 0, window_width(), window_height());
        glClearColor(0.2f, 0.2f, 0.2f, 1.f);        // couleur par defaut de la fenetre
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Dessiner Quaternius
        glUseProgram(m_program);

        program_use_texture(m_program, "shadowmap", 0, m_zbuffer, 0);

        m_quaternius1->render(m_program, gt, vp, m_sourceViewProjection, model_quaternius1, m_camera.position(), m_light_pos, m_zbuffer);
        m_quaternius2->render(m_program, gt, vp, m_sourceViewProjection, model_quaternius2, m_camera.position(), m_light_pos, m_zbuffer);

        // Dessiner la scène
        glBindVertexArray(m_vao_obj);
        glUseProgram(m_program_object);

        program_uniform(m_program_object, "mvpMatrix", m_projection * m_camera.view() * Identity());
        program_uniform(m_program_object, "light_pos", vec3(m_light_pos.x, m_light_pos.y, m_light_pos.z));
        program_uniform(m_program_object, "normalMatrix", Identity().normal());
        program_uniform(m_program_object, "sourceMatrix", Viewport(1, 1) * m_sourceViewProjection);

        program_use_texture(m_program_object, "shadowmap", 0, m_zbuffer, 0);
        glDrawArrays(GL_TRIANGLES, 0, m_vertex_count_obj);
        

        glBindTexture(GL_TEXTURE_2D, 0);
        return 1;
    }

protected:
    Transform m_projection;
    Transform m_sourceViewProjection;
    Orbiter m_camera;
    Point m_light_pos;

    GLuint m_vao_obj;
    GLuint m_vertex_buffer_obj;
    GLuint m_framebuffer;
    GLuint m_zbuffer;                     // Texture de la shadowmap de la source de lumière
    int m_vertex_count_obj;

    GLuint m_program;
    GLuint m_program_shadowmap;
    GLuint m_program_object;

    int m_framebuffer_width;
    int m_framebuffer_height;

    Quaternius *m_quaternius1;
    Quaternius *m_quaternius2;
};

class Object {

};


int main( int argc, char **argv ) {
    TP tp;
    tp.run();
    return 0;
}
