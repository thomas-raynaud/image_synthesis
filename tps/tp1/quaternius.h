#ifndef QUATERNIUS_H
#define QUATERNIUS_H

#include "wavefront.h"
#include "draw.h"
#include "uniforms.h"

#define NB_MESHES 23    // Nombre de mesh utilisés pour animer le robot

class Quaternius {
public:
    Quaternius(float speed, Transform pv, Transform source_pv);

    ~Quaternius();

    // Interpolation dans l'animation
    void computeInterpolation(float gt);

    void render_shadowmap(GLuint program, float gt, const Transform &source_mvp);
    void render(const GLuint &program, const float &gt, const Transform &bp, const Transform &source_vp,
                        const Transform &model, const vec3 &camera_pos, const vec3 light_pos, const GLuint &zbuffer);

protected:
    float m_speed;
    GLuint m_vao;
    GLuint m_vertex_buffer;
    GLuint m_material_buffer;             // Ensenble des matériaux dans le mesh
    GLuint m_ns_buffer;                   // Exposants Blinn-Phong des matériaux
    GLuint m_material_index_buffer;       // Indice des matériaux pour chaque triangle
    int m_vertex_count;

    // Interpolation
    int m_kf1;
    int m_kf2;
    float m_dt;

    Transform m_pv;
    Transform m_source_pv;
};

#endif