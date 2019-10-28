#ifndef OBJECT_H
#define OBJECT_H

#include "draw.h"

Mesh create_mesh(float x_size, float y_size, float z_size) {

    Mesh m = Mesh(GL_TRIANGLES);
    m.color(Color(1, 1, 1));

    float x_offset = x_size / 2.f;
    float y_offset = y_size / 2.f;
    float z_offset = z_size / 2.f;

    //                              0                               1         
    float pt[8][3] = { {-x_offset,-y_offset,-z_offset}, {x_offset,-y_offset,-z_offset},
    //                                      2                               3                               4
                            {x_offset, -y_offset, z_offset}, {-x_offset, -y_offset, z_offset}, {-x_offset, y_offset, -z_offset},
    //                                  5                                  6                                7
                            {x_offset, y_offset, -z_offset}, {x_offset, y_offset, z_offset}, {-x_offset, y_offset, z_offset} };
    int f[6][4] = {    {0,1,2,3}, {5,4,7,6}, {2,1,5,6}, {0,3,7,4}, {3,2,6,7}, {1,0,4,5} };
    float n[6][3] = { {0,-1,0}, {0,1,0}, {1,0,0}, {-1,0,0}, {0,0,1}, {0,0,-1} };

    for (int i = 0; i < 6; ++i) {
        m.normal(  n[i][0], n[i][1], n[i][2] );

        // Triangle 1

        m.texcoord(0,0);
        m.vertex( pt[ f[i][0] ][0], pt[ f[i][0] ][1], pt[ f[i][0] ][2] );

        m.texcoord(1,0);
        m.vertex( pt[ f[i][1] ][0], pt[ f[i][1] ][1], pt[ f[i][1] ][2] );

        m.texcoord(0,1);
        m.vertex( pt[ f[i][3] ][0], pt[ f[i][3] ][1], pt[ f[i][3] ][2] );


        // Triangle 2

        m.texcoord(0,1);
        m.vertex( pt[ f[i][1] ][0], pt[ f[i][1] ][1], pt[ f[i][1] ][2] );

        m.texcoord(1,0);
        m.vertex( pt[ f[i][2] ][0], pt[ f[i][2] ][1], pt[ f[i][2] ][2] );

        m.texcoord(1,1);
        m.vertex( pt[ f[i][3] ][0], pt[ f[i][3] ][1], pt[ f[i][3] ][2] );
    }

    for (int i = 0; i < m.vertex_buffer_size(); ++i) {
        if (i % 3 == 0) std::cout << std::endl;
        std::cout << m.vertex_buffer()[i] << " ";
    }

    return m;
}

#endif