#pragma once
#include<string>
#include"geometry.h"

//Definition for light
//Intensity: RGB vector，values:[0,1]
//Diffuse for objects：RGB vector，values:[0,255]

class Light {
public:
	Vec3f I;             //Intensity
	Vec3f dir;           //Direction
	std::string type;    //spot light or directional light
	Light() {
		I = Vec3f(1.0, 1.0, 1.0);
		dir = Vec3f(1, 1, 1).normalize();
	}
	void setI(Vec3f _I) {
		I = _I;
	}

	virtual void f() {};

};


class Spotlight :public Light {
public:
	Vec3f pos; 
	Spotlight() {
		I = Vec3f(1.0, 1.0, 1.0);
		pos = Vec3f(0, 0, 0);
		type = "spot";
	}
	void setPos(Vec3f _pos) {
		pos = _pos;

	}

};

class Directlight :public Light {
public:
	Directlight() {
		I = Vec3f(1.0, 1.0, 1.0);
		dir = Vec3f(1, 1, 1).normalize();
		type = "direct";
	}
	void setDir(Vec3f _dir) {
		dir = _dir.normalize();
	}

}; 
