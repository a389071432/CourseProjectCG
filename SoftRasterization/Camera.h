#pragma once
#ifndef RENDERER_CAMERA_H
#define RENDERER_CAMERA_H

#include "geometry.h"

//Definition for camera
class Camera {
public:
	//Parameters for pose
	Vec3f pos;
	Vec3f target;
	float xzAngle;
	float yAngle;
	float xzPlaneR;
	float distance;
	float rotateAngle;
	float cur_angle;
	Vec3f up_assit;
	Vec3f D;
	Vec3f R;
	Vec3f U;

	//Parameters for projection,defined by a viewing frustum
	float aspect; 
	float fov; 
	float n;   //near plane
	float f;   //far plane
	float M_PI = 3.14;

	Camera() {
		pos = Vec3f(0, 0, 0);
		target = Vec3f(0, 0, -1);
		xzAngle = M_PI / 2;
		yAngle = M_PI / 2;
		distance = (pos - target).norm();
		xzPlaneR = sqrt(pow(distance, 2) - pow((pos - target).y, 2));
		rotateAngle = M_PI / 24;
		cur_angle = M_PI / 2;
		pos.x = xzPlaneR * cos(cur_angle);
		pos.z = xzPlaneR * sin(cur_angle);

		up_assit = Vec3f(0, 1, 0);
		D = pos - target;
		R = cross(up_assit, D);
		U = cross(D, R);
		D = D.normalize();
		R = R.normalize();
		U = U.normalize();

	}
	mat<4, 4, float>getPoseR();
	mat<4, 4, float>getPoseT();
	mat<4, 4, float>getProjection();
	mat<4, 4, float>getViewPort();

	//Float number interval restriction with the interval [min,max]
	float float_clamp(float f, float min, float max);
    
	//Relevant to the control of camera view
	void setXZ();
	void setY();
	void operate_camera(int key);
    
	//Relevant to pose and projection
	void setPose(const Vec3f& pos, const Vec3f& target, const Vec3f& up);
	void setInstrinics(const float& aspect, const float& fov, const float& n, const float& f);
	void setViewPort(const int width, const int height);

private:
	//Matrixes for transforming vertex from world space to screen space
	mat<4, 4, float> World2Camera_R;
	mat<4, 4, float> World2Camera_T;
	mat<4, 4, float> Projection;
	mat<4, 4, float> ViewPort;
};



#endif //RENDERER_CAMERA_H
