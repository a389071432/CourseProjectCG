#include<algorithm>
#include<omp.h>
#include<cmath>
#include "shader.h"
#include"triangle.h"

using namespace std;

//Vertex shader
void Shader::shade_a_vertex(const int& f_id, const int&vth) {
	//World -> Screen
	Vec3f wpos = model.get_vertex(f_id, vth);
	worldPos.set_col(vth, embed<4>(wpos));
	cameraPos.set_col(vth, camera.getPoseR() * camera.getPoseT() * worldPos.col(vth));
	Vec4f temp = camera.getProjection() * cameraPos.col(vth);
	temp = temp / temp[3];
	clipPos.set_col(vth, temp);
	screenPos.set_col(vth, proj<2>(camera.getViewPort() * temp));
	//Record UV coordinates
	UV.set_col(vth, model.get_uv(f_id, vth));
	vertexNormal.set_col(vth, model.get_normal(f_id, vth));
}

void Shader::shade_a_triangle() {

}

void Shader::shade_a_fragment() {

}

void RealShader::shade_a_triangle(float* z_buffer, float* shadow_buffer) {
	//Calculate the TBN maxtrix
	Vec3f E1 = proj<3>(worldPos.col(1) - worldPos.col(0));
	Vec3f E2 = proj<3>(worldPos.col(2) - worldPos.col(0));
	mat<3, 2, float> E;
	E.set_col(0, E1);
	E.set_col(1, E2);
	mat<2, 3, float>EE = E.transpose();
	Vec2f delta1 = UV.col(1) - UV.col(0);
	Vec2f delta2 = UV.col(2) - UV.col(0);
	mat<2, 2, float>A;
	A.set_col(0, delta1);
	A.set_col(1, delta2);
	A = A.transpose();
	mat<2, 3, float>TB = A.invert() * EE;
	Vec3f T = TB[0], B = TB[1];
	TBN.set_col(0, T.normalize());
	TBN.set_col(1, B.normalize());

	//Bounding box
	int minx = 9999, miny = 9999, maxx = -1, maxy = -1;

	for (int j = 0; j < 3; j++) {
		minx = std::min(minx, int(screenPos[0][j]));
		miny = std::min(miny, int(screenPos[1][j]));
		maxx = std::max(maxx, int(screenPos[0][j]));
		maxy = std::max(maxy, int(screenPos[1][j]));
	}

	//Handling pixels in order
	for (int y = miny; y <= maxy; y++) {
		for (int x = minx; x <= maxx; x++) {
			if (x < 0 || x >= width || y < 0 || y >= height)
				continue;

			Vec3f screen_bar = barycentric(proj<2>(screenPos.col(0)), proj<2>(screenPos.col(1)), proj<2>(screenPos.col(2)), Vec2f(x, y));
			if (screen_bar.x < 0 || screen_bar.y < 0 || screen_bar.z < 0)  //忽略三角形以外的像素
				continue;

			//Clipping test
			float Z_clip = clipPos[2] * screen_bar;
			if (Z_clip < -1 || Z_clip>1)
				continue;
			//Depth test
			if (Z_clip > z_buffer[y * width + x])
				continue;
			z_buffer[y * width + x] = Z_clip;

			//Calculation of the center of gravity coordinates in 3D space for interpolation
			Vec3f temp = Vec3f(1.0f / cameraPos[2][0], 1.0f / cameraPos[2][1], 1.0f / cameraPos[2][2]);
			float Zp = 1.0f / (screen_bar * temp);
			Vec3f bar = multi_by_element(screen_bar, temp) * Zp;
			Vec3i color = shade_a_fragment(bar);

			//Shadow mapping
			Vec4f wpos = embed<4>(proj<3>(worldPos * bar)); 
			Vec4f cpos = falseCamera.getPoseR() * falseCamera.getPoseT() * wpos;
			Vec4f clipos = falseCamera.getProjection() * cpos;
			clipos = clipos / clipos[3];
			Vec4f spos = falseCamera.getViewPort() * clipos;
			int index = int(spos[1])*width + int(spos[0]);
			if (index < width * height&& index >= 0) {
				if (shadow_buffer[index] < clipos[2] - 0.060)  //Trick to solve the problem of Z-fighting, -0.060 filters out most of the discontinuous shadow points
					color = color * 0.3;
			}

			//Ambient
			color = color + Vec3i(25, 25, 25);
			TGAColor tcolor(color[0], color[1], color[2]);
			frame_buffer.set(x, y, tcolor);
		}
	}

}

//Fragment shader
Vec3i RealShader::shade_a_fragment(const Vec3f& bar) {

	Vec3i color;
	Vec3f N = vertexNormal * bar;
	TBN.set_col(2, N);

	//Normal vector of a fragment in world space
	Vec2f uv = UV * bar;
	Vec3f n = TBN * model.get_normal(uv);
	n = n.normalize();

	//Get information from mapping
	Vec3f diffuse = model.get_diffuse(uv);
	float specular = model.get_specular(uv);

	//Apply local illumination to a fragment based on Phong model 
	Vec3f ldir = light->dir;
	if (light->type == "direct") {
		//Intensity of diffuse
		Vec3f pos = proj<3>(worldPos * bar);
		float I_diffuse = max(ldir * n, 0.0f);
		//Intensity of specular
		double alpha = acos(n*((camera.pos - pos).normalize() + ldir).normalize());
		double I_specular= max(0.0, pow(cos(alpha), double(specular)));
		color = multi_by_element(light->I * (I_specular + I_diffuse), diffuse);

		for (int i = 0; i < 3; i++) {
			if (color[i] > 255)
				color[i] = 255;
		}
	}
	return color;
}

void ShadowShader::shade_a_triangle(float* shadow_buffer) {
	//Bounding box
	int minx = 9999, miny = 9999, maxx = -1, maxy = -1;
	for (int j = 0; j < 3; j++) {
		minx = std::min(minx, int(screenPos[0][j]));
		miny = std::min(miny, int(screenPos[1][j]));
		maxx = std::max(maxx, int(screenPos[0][j]));
		maxy = std::max(maxy, int(screenPos[1][j]));
	}

	//Handle pixels in order
	for (int y = miny; y <= maxy; y++) {
		for (int x = minx; x <= maxx; x++) {
			if (x < 0 || x >= width || y < 0 || y >= height)
				continue;
			Vec3f screen_bar = barycentric(proj<2>(screenPos.col(0)), proj<2>(screenPos.col(1)), proj<2>(screenPos.col(2)), Vec2f(x, y));
			if (screen_bar.x < 0 || screen_bar.y < 0 || screen_bar.z < 0)  //忽略三角形以外的像素
				continue;
			//Depth test. First interpolating to get the Z-coordinate in the crip space
			float Z_clip = clipPos[2] * screen_bar;
			if (Z_clip > 1 || Z_clip < -1)
				continue;
			if (Z_clip > shadow_buffer[y * width + x])
				continue;
			shadow_buffer[y * width + x] = Z_clip;

			//Used for visualizing the shadow buffer, not necessary for the main workflow
			//float coeff = (1-Z_clip)/2.0f;
			//coeff *= 1.5;
			////coeff *= coeff;
			////coeff += 0.1;
			//Vec3f color = Vec3f(min(1.0f, coeff), min(1.0f, coeff), min(1.0f, coeff)) * 255;
			//TGAColor tcolor(color[0], color[1], color[2]);
			//frame_buffer.set(x, y, tcolor);
		}
	}
};
