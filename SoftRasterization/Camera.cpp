#include<cmath>
#include<iostream>
#include "Camera.h"

using namespace std;

#define EPSILON 1e-5

void Camera::setXZ() {
	pos.x = xzPlaneR * cos(xzAngle);
	pos.z = xzPlaneR * sin(xzAngle);
}

void Camera::setY() {
	yAngle = float_clamp(yAngle, EPSILON, M_PI - EPSILON);
	pos.y = distance * (float)cos(yAngle);
}

float Camera::float_clamp(float f, float min, float max) {
	return f < min ? min : (f > max ? max : f);
}

void Camera::operate_camera(int key) {
	if (key == 0) {
		xzAngle += rotateAngle;
		setXZ();
	}
	else if (key == 1) {
		xzAngle -= rotateAngle;
		setXZ();
	}
	else if (key == 10) {
		yAngle += rotateAngle;
		xzPlaneR = distance * sin(yAngle);
		setY();
	}
	else if (key == 11) {
		yAngle -= rotateAngle;
		xzPlaneR = distance * sin(yAngle);
		setY();
	}
}

mat<4, 4, float> Camera::getPoseR() {
	return World2Camera_R;
}
mat<4, 4, float>Camera::getPoseT() {
	return World2Camera_T;
}
mat<4, 4, float>Camera::getProjection() {
	return Projection;
}

mat<4, 4, float>Camera::getViewPort() {
	return ViewPort;
}

void Camera::setPose(const Vec3f& _pos, const Vec3f& _target, const Vec3f& up) {
	pos = _pos;
	target = _target;
	D = pos - target;
	R = cross(up, D);
	U = cross(D, R);
	D.normalize();
	R.normalize();
	U.normalize();
	World2Camera_R[0] = embed<4>(R);
	World2Camera_R[0][3] = 0;
	World2Camera_R[1] = embed<4>(U);
	World2Camera_R[1][3] = 0;
	World2Camera_R[2] = embed<4>(D);
	World2Camera_R[2][3] = 0;
	World2Camera_R[3] = embed<4>(Vec3f(0.0f, 0.0f, 0.0f));

	World2Camera_T = Matrix::identity();
	World2Camera_T.set_col(3, embed<4>(_pos * -1.0f));
}

void Camera::setInstrinics(const float& aspect, const  float& fov, const  float& n, const float& f)
{
	Projection[0] = Vec4f(1.0 / (tan(fov / 2) * aspect), 0, 0, 0);
	Projection[1] = Vec4f(0, 1.0 / tan(fov / 2), 0, 0);
	Projection[2] = Vec4f(0, 0, (n + f) / (n - f), 2 * n*f / (n - f));
	Projection[3] = Vec4f(0, 0, -1, 0);
}

void Camera::setViewPort(const int width, const int height) {
	ViewPort[0] = Vec4f(width / 2.0f, 0, 0, width / 2.0f);
	ViewPort[1] = Vec4f(0, height / 2.0f, 0, height / 2.0f);
	ViewPort[2] = Vec4f(0, 0, 1, 0);
	ViewPort[3] = Vec4f(0, 0, 0, 1);
}