#include "quaternius.h"

Quaternius::Quaternius(float speed, Transform pv, Transform source_pv) {
    m_speed = speed;
    m_pv = pv;
    m_source_pv = source_pv;

    glGenBuffers(1, &m_vertex_buffer);
    glGenBuffers(1, &m_material_buffer);
    glGenBuffers(1, &m_ns_buffer);
    glGenBuffers(1, &m_material_index_buffer);

    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);

    // charger les attributs du mesh (positions et normales
    Mesh mesh = read_mesh("data/robot_quaternius/run/Robot_000001.obj");
    m_vertex_count = mesh.vertex_count();
    glBindBuffer(GL_ARRAY_BUFFER, m_vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, m_vertex_count * sizeof(vec3) * NB_MESHES * 2, nullptr, GL_STATIC_DRAW);
    
    int offset = 0;
    std::string mesh_path;
    for (int i = 1; i <= NB_MESHES; ++i) {
        mesh_path.clear();
        mesh_path = "data/robot_quaternius/run/Robot_0000";
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
}


Quaternius::~Quaternius() {
    glDeleteBuffers(1, &m_vertex_buffer);
    glDeleteBuffers(1, &m_material_buffer);
    glDeleteBuffers(1, &m_ns_buffer);
    glDeleteBuffers(1, &m_material_index_buffer);
    glDeleteVertexArrays(1, &m_vao);
}


void Quaternius::computeInterpolation(float gt) {
    float t = (gt / (1000.f * 24.f) * m_speed);
    int t0 = t;
    m_kf1 = t0 % NB_MESHES;
    m_kf2 = (t0 + 1) % NB_MESHES;
    m_dt = t - t0;
}


void Quaternius::render_shadowmap(GLuint program, float gt, const Transform &source_mvp) {
    glBindVertexArray(m_vao);

    //computeInterpolation(gt);

    program_uniform(program, "mvpMatrix", source_mvp);
    program_uniform(program, "time", m_dt);
    program_uniform(program, "useInterpolation", 1);

    glDrawArrays(GL_TRIANGLES, 0, m_vertex_count);
}


void Quaternius::render(const GLuint &program, const float &gt, const Transform &vp, const Transform &source_vp,
                        const Transform &model, const vec3 &camera_pos, const vec3 light_pos, const GLuint &zbuffer) {

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vertex_buffer);

    computeInterpolation(gt);
    
    // Positions des sommets de la keyframe 1
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (const GLvoid *) (m_vertex_count * sizeof(vec3) * m_kf1 * 2));
    glEnableVertexAttribArray(0);
    // Normales des sommets de la keyframe 1
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (const GLvoid *) ((m_vertex_count * sizeof(vec3) * m_kf1 * 2) + (sizeof(vec3) * m_vertex_count)));
    glEnableVertexAttribArray(1);

    // Positions des sommets de la keyframe 2
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (const GLvoid *) (m_vertex_count * sizeof(vec3) * m_kf2 * 2));
    glEnableVertexAttribArray(2);
    // Normales des sommets de la keyframe 2
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, (const GLvoid *) ((m_vertex_count * sizeof(vec3) * m_kf2 * 2) + (sizeof(vec3) * m_vertex_count)));
    glEnableVertexAttribArray(3);
    
    program_uniform(program, "mvpMatrix", vp * model);
    program_uniform(program, "normalMatrix", model.normal());
    program_uniform(program, "view_pos", camera_pos);
    program_uniform(program, "light_pos", light_pos);
    program_uniform(program, "sourceMatrix", Viewport(1, 1) * source_vp * model);
    program_uniform(program, "time", m_dt);
    
    program_use_texture(program, "shadowmap", 0, zbuffer, 0);

    // Materiaux du mesh
    GLuint material_block = glGetUniformBlockIndex(program, "MaterialBlock");
    glUniformBlockBinding(program, material_block, 0);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_material_buffer);

    // + exposants blinn-phong
    GLuint ns_material = glGetUniformBlockIndex(program, "ExposantBlinnPhong");
    glUniformBlockBinding(program, ns_material, 2);
    glBindBufferBase(GL_UNIFORM_BUFFER, 2, m_ns_buffer);

    glDrawArrays(GL_TRIANGLES, 0, m_vertex_count);
}