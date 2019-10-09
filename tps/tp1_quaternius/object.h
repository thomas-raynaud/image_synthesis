#ifndef OBJECT_H
#define OBJECT_H

#include "mesh.h"

Mesh create_mesh(float x_size, float y_size, float z_size) {
    //                          0                           1                      
    float pt[8][3] = { {-x_size,-y_size,-z_size}, {x_size,-y_size,-z_size},
    //                          2                      3                          4
                            {x_size, -y_size, z_size}, {-x_size, -y_size, z_size}, {-x_size, y_size, -z_size},
    //                          5                           6                         7
                            {x_size, y_size, -z_size}, {x_size, y_size, z_size}, {-x_size, y_size, z_size} };
    int f[6][4] = {    {0,1,2,3}, {5,4,7,6}, {2,1,5,6}, {0,3,7,4}, {3,2,6,7}, {1,0,4,5} };
    float n[6][3] = { {0,-1,0}, {0,1,0}, {1,0,0}, {-1,0,0}, {0,0,1}, {0,0,-1} };

    Mesh mesh = Mesh(GL_TRIANGLE_STRIP);
    mesh.color(Color(1, 1, 1));

    for (int i = 0; i < 6; ++i) {
        mesh.normal(  n[i][0], n[i][1], n[i][2] );

        mesh.texcoord(0,0);
        mesh.vertex( pt[ f[i][0] ][0], pt[ f[i][0] ][1], pt[ f[i][0] ][2] );

        mesh.texcoord( 1,0);
        mesh.vertex( pt[ f[i][1] ][0], pt[ f[i][1] ][1], pt[ f[i][1] ][2] );

        mesh.texcoord(0,1);
        mesh.vertex(pt[ f[i][3] ][0], pt[ f[i][3] ][1], pt[ f[i][3] ][2] );

        mesh.texcoord(1,1);
        mesh.vertex( pt[ f[i][2] ][0], pt[ f[i][2] ][1], pt[ f[i][2] ][2] );

        mesh.restart_strip();
    }
    return mesh;
}

#endif