
#include "wavefront.h"
#include "texture.h"

#include "orbiter.h"
#include "draw.h"        
#include "app.h"
#include "app_time.h"
#include "program.h"
#include "uniforms.h"

#include "ortho.h"
#include "object.h"

#define NB_MESHES 23    // Nombre de mesh utilisés pour animer le robot
#define SPEED 4000.f    // Vitesse de l'animation du robot


class TP : public App {
public:
    // constructeur : donner les dimensions de l'image, et eventuellement la version d'openGL.
    TP() : App(1024, 640) {}
    
    // creation des objets de l'application
    int init( ) {
        m_framebuffer_width  = 1024;
        m_framebuffer_height = 1024;

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

        //glDrawBuffer(GL_NONE);

        // nettoyage
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

        m_camera.lookat(Point(0, 2, 5), Point(0, 2, 0));
        m_light_pos = Point(10, 10, 5);
        m_obj_pos = Point(3, 4, 1);

        m_sourceView = Lookat(m_light_pos, Point(0, 0, 0), Vector(0, 1, 0));
        m_sourceProjection = Ortho(-6, 6, -6, 6, 0.001, 1000);
        Mesh mesh;

        glGenBuffers(1, &m_vertex_buffer);
        glGenBuffers(1, &m_vertex_buffer_obj);
        glGenBuffers(1, &m_material_buffer);
        glGenBuffers(1, &m_ns_buffer);
        glGenBuffers(1, &m_material_index_buffer);

        // Objets de la scene.
        glGenVertexArrays(1, &m_vao_obj);
        glBindVertexArray(m_vao_obj);
        mesh = read_mesh("data/cube.obj");
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

        glGenVertexArrays(1, &m_vao);
        glBindVertexArray(m_vao);

        // charger les attributs du mesh (positions et normales
        mesh = read_mesh("robot_quaternius/run/Robot_000001.obj");
        m_vertex_count = mesh.vertex_count();
        glBindBuffer(GL_ARRAY_BUFFER, m_vertex_buffer);
        glBufferData(GL_ARRAY_BUFFER, m_vertex_count * sizeof(vec3) * NB_MESHES * 2, nullptr, GL_STATIC_DRAW);
        
        int offset = 0;
        std::string mesh_path;
        for (int i = 1; i <= NB_MESHES; ++i) {
            mesh_path.clear();
            mesh_path = "robot_quaternius/run/Robot_0000";
            if (i < 10) mesh_path += "0";
            mesh_path += std::to_string(i) + ".obj";
            mesh = read_mesh(mesh_path.c_str());
            glBufferSubData(GL_ARRAY_BUFFER, offset, mesh.vertex_buffer_size(), mesh.vertex_buffer());
            offset += mesh.vertex_buffer_size();
            glBufferSubData(GL_ARRAY_BUFFER, offset, mesh.normal_buffer_size(), mesh.normal_buffer());
            offset += mesh.normal_buffer_size();
        }

        // Matériaux
        // créer et remplir le buffer avec les paramètres des matières.
        glBindBuffer(GL_UNIFORM_BUFFER, m_material_buffer);
        vec4 material_colors[mesh.mesh_materials().size() * 2];
        Color color_temp;
        for (unsigned int i = 0; i < mesh.mesh_materials().size(); ++i) {
            color_temp = mesh.mesh_materials()[i].diffuse;
            material_colors[i * 2] = vec4(color_temp.r, color_temp.g, color_temp.b, color_temp.a);
            color_temp = mesh.mesh_materials()[i].specular;
            material_colors[(i * 2) + 1] = vec4(color_temp.r, color_temp.g, color_temp.b, color_temp.a);
        }
        glBufferData(GL_UNIFORM_BUFFER, mesh.mesh_materials().size() * sizeof(vec4) * 2, material_colors, GL_STATIC_DRAW);

        // créer et remplir le buffer avec les exposants pour les reflets blinn-phong
        glBindBuffer(GL_UNIFORM_BUFFER, m_ns_buffer);
        float ns[mesh.mesh_materials().size()];
        for (unsigned int i = 0; i < mesh.mesh_materials().size(); ++i) {
            ns[i] = mesh.mesh_materials()[i].ns;
        }
        glBufferData(GL_UNIFORM_BUFFER, mesh.mesh_materials().size() * sizeof(float), ns, GL_STATIC_DRAW);
       
        // créer et remplir le buffer contenant l'indice de la matière de chaque triangle
        glBindBuffer(GL_ARRAY_BUFFER, m_material_index_buffer);
        GLint material_triangle_index[mesh.triangle_count() * 3];
        for (int i = 0; i < mesh.triangle_count(); ++i) {
            material_triangle_index[i * 3]     = mesh.materials()[i];
            material_triangle_index[i * 3 + 1] = mesh.materials()[i];
            material_triangle_index[i * 3 + 2] = mesh.materials()[i];
        }
        glBufferData(GL_ARRAY_BUFFER, mesh.triangle_count() * 3 * sizeof(GLint), material_triangle_index, GL_STATIC_DRAW);
        glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(4);

        std::string definitions;
        definitions.append("#define NB_MATERIALS " + std::to_string(mesh.mesh_materials().size() * 2));
        definitions.append("\n");

        m_program           = read_program("tps/tp1_quaternius/quaternius.glsl", definitions.c_str());
        m_program_shadowmap = read_program("tps/tp1_quaternius/shadowmap.glsl");
        m_program_object    = read_program("tps/tp1_quaternius/object.glsl");
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

        glDeleteBuffers(1, &m_vertex_buffer);
        glDeleteBuffers(1, &m_vertex_buffer_obj);
        glDeleteBuffers(1, &m_material_buffer);
        glDeleteBuffers(1, &m_ns_buffer);
        glDeleteBuffers(1, &m_material_index_buffer);
        glDeleteTextures(1, &m_zbuffer);
        glDeleteVertexArrays(1, &m_vao);
        glDeleteVertexArrays(1, &m_vao_obj);
        glDeleteFramebuffers(1, &m_framebuffer);
        
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

        // Interpolation dans l'animation
        int t0 = (gt / (SPEED / 24.f));
        int kf1 = t0 % NB_MESHES;
        int kf2 = (t0 + 1) % NB_MESHES;
        float t = (gt / (SPEED / 24.f));
        float dt = t - t0;

        glBindVertexArray(m_vao);
        glBindBuffer(GL_ARRAY_BUFFER, m_vertex_buffer);
        
        // Positions des sommets de la keyframe 1
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (const GLvoid *) (m_vertex_count * sizeof(vec3) * kf1 * 2));
        glEnableVertexAttribArray(0);
        // Normales des sommets de la keyframe 1
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (const GLvoid *) ((m_vertex_count * sizeof(vec3) * kf1 * 2) + (sizeof(vec3) * m_vertex_count)));
        glEnableVertexAttribArray(1);

        // Positions des sommets de la keyframe 2
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (const GLvoid *) (m_vertex_count * sizeof(vec3) * kf2 * 2));
        glEnableVertexAttribArray(2);
        // Normales des sommets de la keyframe 2
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, (const GLvoid *) ((m_vertex_count * sizeof(vec3) * kf2 * 2) + (sizeof(vec3) * m_vertex_count)));
        glEnableVertexAttribArray(3);

        // Matrices
        Transform projection = m_camera.projection(window_width(), window_height(), 45);
        Transform model = Identity();
        Transform mvp = projection * m_camera.view() * model;
        Transform mvpShadowMap = m_sourceProjection * m_sourceView * model;


        // SHADOW MAP
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_framebuffer);
        glViewport(0, 0, m_framebuffer_width, m_framebuffer_height);
        glClearColor(0.2, 0.2, 0.2, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindVertexArray(m_vao);
        glUseProgram(m_program_shadowmap);

        program_uniform(m_program_shadowmap, "mvpMatrix", mvpShadowMap);
        program_uniform(m_program_shadowmap, "time", dt);
        program_uniform(m_program_shadowmap, "useInterpolation", 1);

        glDrawArrays(GL_TRIANGLES, 0, m_vertex_count);

        // Dessiner la scène
        glBindVertexArray(m_vao_obj);
        program_uniform(m_program_shadowmap, "mvpMatrix", m_sourceProjection * m_sourceView * Translation(Vector(m_obj_pos)));
        program_uniform(m_program_shadowmap, "useInterpolation", 0);

        glDrawArrays(GL_TRIANGLES, 0, m_vertex_count_obj);


        // FRAMEBUFFER PAR DEFAUT
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glViewport(0, 0, window_width(), window_height());
        glClearColor(0.2f, 0.2f, 0.2f, 1.f);        // couleur par defaut de la fenetre
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Dessiner Quaternius
        glBindVertexArray(m_vao);
        glUseProgram(m_program);

        program_uniform(m_program, "mvpMatrix", mvp);
        program_uniform(m_program, "normalMatrix", model.normal());
        program_uniform(m_program, "view_pos", m_camera.position());
        program_uniform(m_program, "light_pos", vec3(m_light_pos.x, m_light_pos.y, m_light_pos.z));
        program_uniform(m_program, "sourceMatrix", Viewport(1, 1) * mvpShadowMap);
        program_uniform(m_program, "time", dt);

        program_use_texture(m_program, "shadowmap", 0, m_zbuffer, 0);

        // Materiaux du mesh
        GLuint material_block = glGetUniformBlockIndex(m_program, "MaterialBlock");
        glUniformBlockBinding(m_program, material_block, 0);
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_material_buffer);

        // + exposants blinn-phong
        GLuint ns_material = glGetUniformBlockIndex(m_program, "ExposantBlinnPhong");
        glUniformBlockBinding(m_program, ns_material, 2);
        glBindBufferBase(GL_UNIFORM_BUFFER, 2, m_ns_buffer);

        glDrawArrays(GL_TRIANGLES, 0, m_vertex_count);

        // Dessiner la scène
        glBindVertexArray(m_vao_obj);
        glUseProgram(m_program_object);

        model = Translation(Vector(m_obj_pos));

        program_uniform(m_program_object, "mvpMatrix", projection * m_camera.view() * model);
        program_uniform(m_program_object, "light_pos", vec3(m_light_pos.x, m_light_pos.y, m_light_pos.z));
        program_uniform(m_program_object, "normalMatrix", model.normal());
        glDrawArrays(GL_TRIANGLES, 0, m_vertex_count_obj);
        

        glBindTexture(GL_TEXTURE_2D, 0);
        return 1;
    }

protected:
    Transform m_model;
    Transform m_sourceView;
    Transform m_sourceProjection;
    Orbiter m_camera;
    Point m_light_pos;
    Point m_obj_pos;

    GLuint m_vao;
    GLuint m_vao_obj;
    GLuint m_vertex_buffer;
    GLuint m_vertex_buffer_obj;
    GLuint m_material_buffer;             // Ensenble des matériaux dans le mesh
    GLuint m_ns_buffer;                   // Exposants Blinn-Phong des matériaux
    GLuint m_material_index_buffer;       // Indice des matériaux pour chaque triangle
    GLuint m_framebuffer;
    GLuint m_zbuffer;                     // Texture de la shadowmap de la source de lumière
    int m_vertex_count;
    int m_vertex_count_obj;

    GLuint m_program;
    GLuint m_program_shadowmap;
    GLuint m_program_object;

    int m_framebuffer_width;
    int m_framebuffer_height;
};


int main( int argc, char **argv ) {
    TP tp;
    tp.run();
    return 0;
}
