#ifndef RENDERER_TRANSFORMATION_H
#define RENDERER_TRANSFORMATION_H


#include "geometry.h"

//extern Matrix ModelView;
//extern Matrix Viewport;
//extern Matrix Projection;

Matrix ModelView;
Matrix Viewport;
Matrix Projection;

void viewport(int x, int y, int w, int h);
void projection(float coeff = 0.f); // coeff = -1/c
void lookat(Vec3f camera, Vec3f target, Vec3f up);

#endif //RENDERER_TRANSFORMATION_H
