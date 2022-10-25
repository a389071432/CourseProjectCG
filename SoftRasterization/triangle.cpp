#include "triangle.h"

void line(int x0, int y0, int x1, int y1, TGAImage& image, TGAColor color) {
	bool isSteep = abs(y1 - y0) > abs(x1 - x0);
	if (isSteep) {
		std::swap(x0, y0);
		std::swap(x1, y1);
	}
	if (x0 > x1) {
		std::swap(x0, x1);
		std::swap(y0, y1);
	}

	int deltax = x1 - x0;
	int deltay = abs(y1 - y0);
	float error = deltax / 2;
	int ystep;
	int y = y0;
	ystep = y0 < y1 ? 1 : -1;

	for (int x = x0; x <= x1; x++) {
		isSteep ? image.set(y, x, color) : image.set(x, y, color);
		error -= deltay;
		if (error < 0) {
			y += ystep;
			error += deltax;
		}
	}
}

Vec3f barycentric(Vec2f A, Vec2f B, Vec2f C, Vec2f P) {
	Vec3f triangle[2];
	for (int i = 2; i--; ) {
		triangle[i][0] = C[i] - A[i];
		triangle[i][1] = B[i] - A[i];
		triangle[i][2] = A[i] - P[i];
	}
	Vec3f u = cross(triangle[0], triangle[1]);
	if (std::abs(u[2]) > 1e-2) {
		return Vec3f(1.f - (u.x + u.y) / u.z, u.y / u.z, u.x / u.z);
	}

	return Vec3f(-1, -1, -1);
}