#include <iostream>
#include<time.h>
#include<string>
#include "Camera.h"
#include"Light.h"
#include"tgaimage.h"
#include"Shader.h"
#include"triangle.h"

using namespace std;

Camera camera;
Camera falseCamera; //用于测试阴影的伪相机
Directlight* light;
TGAImage frame_buffer;
float* z_buffer;
float* shadow_buffer;
int width, height;
string savePath;
string objPath[3];

void init()
{
	objPath[0] = "models/man_head/man_head.obj";
	objPath[1] = "models/man_head/man_head_eyes.obj";
	savePath = "results/result.tga";
	width = 800, height = 800;
	camera = Camera();
	camera.setPose(Vec3f(0, 0.55, 2.1), Vec3f(0, 0, 0), Vec3f(0, 1, 0));
	camera.setInstrinics(width / height, 1.05, 1, 200);
	camera.setViewPort(width, height);
	light = new Directlight();
	//light->setPos(Vec3f(4,4,4));
	light->setI(Vec3f(1, 1, 1));
	light->setDir(Vec3f(1, 1, 1));
	falseCamera = Camera();
	//falseCamera.setPose(Vec3f(2,2,2),light->pos-light->dir,Vec3f(0,1,0));
	falseCamera.setPose(Vec3f(2, 2, 2), Vec3f(0, 0, 0), Vec3f(0, 1, 0));
	falseCamera.setInstrinics(width / height, 1.0, 1, 100);
	falseCamera.setViewPort(width, height);
	z_buffer = new float[width * height * 3];
	shadow_buffer = new float[width * height * 3];
	frame_buffer = TGAImage(width, height, TGAImage::RGB);
	for (int i = 0; i < width * height; i++)
		z_buffer[i] = 99999999.0f;
	for (int i = 0; i < width*height; i++)
		shadow_buffer[i] = 9999999.0f;
}

int main() {

	init();
	double t0 = clock();
	for (int obj = 0; obj < 2; obj++)
	{
		Model model = Model(objPath[obj]);
		RealShader realShader(frame_buffer, camera, falseCamera, light, model, width, height);
		ShadowShader shadowShader(frame_buffer, falseCamera, light, model, width, height);
		for (int i = 0; i < model.get_nfaces(); i++) {
			for (int j = 0; j < 3; j++)
				shadowShader.shade_a_vertex(i, j);
			shadowShader.shade_a_triangle(shadow_buffer);
		}

		for (int i = 0; i < model.get_nfaces(); i++) {

			for (int j = 0; j < 3; j++)
				realShader.shade_a_vertex(i, j);
			realShader.shade_a_triangle(z_buffer, shadow_buffer);
		}
	}
	
	double t1 = clock();
	frame_buffer.write_tga_file(savePath);
	cout << "cost:" << (t1 - t0) / 1000 << "s";
}