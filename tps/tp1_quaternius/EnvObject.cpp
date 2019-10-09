#include "draw.h"

#include "EnvObject.h"

EnvObject::EnvObject(float x_size, float y_size, float z_size, Point position)
    : Object(Point(-x_size, -y_size, -z_size), Point(x_size, y_size, z_size))
    {
        CollideManager::getInstance()->addCollider((Object*)this, std::string("EnvObject"));
        setPosition(position);
        //                          0                           1                      
        float pt[8][3] = { {-x_size,-y_size,-z_size}, {x_size,-y_size,-z_size},
        //                          2                      3                          4
                                {x_size, -y_size, z_size}, {-x_size, -y_size, z_size}, {-x_size, y_size, -z_size},
        //                          5                           6                         7
                                {x_size, y_size, -z_size}, {x_size, y_size, z_size}, {-x_size, y_size, z_size} };
        int f[6][4] = {    {0,1,2,3}, {5,4,7,6}, {2,1,5,6}, {0,3,7,4}, {3,2,6,7}, {1,0,4,5} };
        float n[6][3] = { {0,-1,0}, {0,1,0}, {1,0,0}, {-1,0,0}, {0,0,1}, {0,0,-1} };

        m_mesh = Mesh(GL_TRIANGLE_STRIP);
        m_mesh.color(Color(0.5, 0.5, 0.5));

        for (int i = 0; i < 6; ++i) {
            m_mesh.normal(  n[i][0], n[i][1], n[i][2] );

            m_mesh.texcoord(0,0);
            m_mesh.vertex( pt[ f[i][0] ][0], pt[ f[i][0] ][1], pt[ f[i][0] ][2] );

            m_mesh.texcoord( 1,0);
            m_mesh.vertex( pt[ f[i][1] ][0], pt[ f[i][1] ][1], pt[ f[i][1] ][2] );

            m_mesh.texcoord(0,1);
            m_mesh.vertex(pt[ f[i][3] ][0], pt[ f[i][3] ][1], pt[ f[i][3] ][2] );

            m_mesh.texcoord(1,1);
            m_mesh.vertex( pt[ f[i][2] ][0], pt[ f[i][2] ][1], pt[ f[i][2] ][2] );

            m_mesh.restart_strip();
        }
}

void EnvObject::draw(const Transform& v, const Transform& p) {
    ::draw(m_mesh, transform(), v, p);
}

void EnvObject::activateCollision(Object* obj) {};