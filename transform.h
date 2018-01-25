#pragma once
#include "frameinput.h"

float NdcToFbX(float x_ndc)
{
    return (0.5f+0.5f*x_ndc)*frame_input.framebuffer_w;
}
float NdcToFbY(float y_ndc)
{
    return (0.5f-0.5f*y_ndc)*frame_input.framebuffer_h;
}

// void UserToNdc(float x, float y, float z, float *x_ndc, float *y_ndc, float *z_ndc)
// {
//     GLfloat P[16];
//     GLfloat M[16];
//     glGetFloatv(GL_PROJECTION_MATRIX,P);
//     glGetFloatv(GL_MODELVIEW_MATRIX,M);
//     float x_view = M[ 0]*x + M[ 1]*y + M[ 2]*z + M[ 3];
//     float y_view = M[ 4]*x + M[ 5]*y + M[ 6]*z + M[ 7];
//     float z_view = M[ 8]*x + M[ 9]*y + M[10]*z + M[11];
//     float w_view = M[12]*x + M[13]*y + M[14]*z + M[15];

//     float x_clip = P[ 0]*x_view + P[ 1]*y_view + P[ 2]*z_view + P[ 3]*w_view;
//     float y_clip = P[ 4]*x_view + P[ 5]*y_view + P[ 6]*z_view + P[ 7]*w_view;
//     float z_clip = P[ 8]*x_view + P[ 9]*y_view + P[10]*z_view + P[11]*w_view;
//     float w_clip = P[12]*x_view + P[13]*y_view + P[14]*z_view + P[15]*w_view;

//     *x_ndc = x_clip/w_clip;
//     *y_ndc = y_clip/w_clip;
//     *z_ndc = z_clip/w_clip;
// }
