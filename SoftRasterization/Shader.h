#ifndef RENDERER_SHADER_H
#define RENDERER_SHADER_H

#include"Camera.h"
#include"geometry.h"
#include"Model.h"
#include"Light.h"

class Shader {
public:
	TGAImage& frame_buffer;
	Camera& camera;
	Light* light;
	Model& model;
	int& width, height; //Size of screen
	Shader(TGAImage& _buffer, Camera& _camera, Light* _light, Model& _model, int& _width, int& _height) :frame_buffer(_buffer), camera(_camera), light(_light), model(_model), width(_width), height(_height) {
	}
	void shade_a_vertex(const int&f_id, const int& vth);
	virtual void shade_a_triangle();
	virtual void shade_a_fragment();
	mat<2, 3, int>screenPos;   //Vertex coornates in screen space

protected:
	mat<4, 3, float> worldPos;     //Vertex coornates in world space
	mat<4, 3, float> cameraPos;    //Vertex coornates in camera space
	mat<4, 3, float>clipPos;       //Vertex coornates in clip space

	mat<2, 3, float>UV;            //UV coordinate for vertex
	mat<3, 3, float>vertexNormal;  //Normal vectors of vertex in world space
	mat<3, 3, float>TBN;           //Transformation from tangent space to world space
};

//Shader for the second shading process
class RealShader :public Shader {
public:
	RealShader(TGAImage& _buffer, Camera& _camera, Camera& _falseCamera, Light* _light, Model& _model, int& _width, int& _height) :falseCamera(_falseCamera), Shader(_buffer, _camera, _light, _model, _width, _height)
	{}
	virtual void shade_a_triangle(float* z_buffer, float* shadow_buffer);
	virtual Vec3i shade_a_fragment(const Vec3f& bar); //返回颜色
private:
	Camera& falseCamera; //用于阴影测试的伪相机
};

//Shader for the first shading process, used for shadow mapping
class ShadowShader :public Shader {
public:
	ShadowShader(TGAImage& _buffer, Camera& _camera, Light* _light, Model& _model, int& _width, int& _height) :Shader(_buffer, _camera, _light, _model, _width, _height) {}
	virtual void shade_a_triangle(float* shadow_buffer);
};

#endif //RENDERER_SHADER_H
