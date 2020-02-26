#ifndef ORTHO_H
#define ORTHO_H

#include "math.h"

Transform Ortho( const float left, const float right, const float bottom, const float top, const float znear, const float zfar )
{
    float tx= - (right + left) / (right - left);
    float ty= - (top + bottom) / (top - bottom);
    float tz= - (zfar + znear) / (zfar - znear);
   
    return Transform(
        2.f / (right - left),                    0,                     0, tx,
                           0, 2.f / (top - bottom),                     0, ty,
        0,                                       0, -2.f / (zfar - znear), tz,
        0,                                       0,                     0, 1);
}

#endif