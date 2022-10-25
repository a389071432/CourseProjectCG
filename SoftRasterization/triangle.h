#ifndef RENDERER_TRIANGLE_H
#define RENDERER_TRIANGLE_H


#include "tgaimage.h"
#include "geometry.h"
#include "Shader.h"

//Bresenham's line algorithm
void line(int x0, int y0, int x1, int y1, TGAImage& image, TGAColor color);

//Calculate the center of gravity of a triangle
Vec3f barycentric(Vec2f A, Vec2f B, Vec2f C, Vec2f P);

void triangle(Vec4f* pts, Shader& shader, TGAImage& image, float* zbuffer);

#endif //RENDERER_TRIANGLE_H
