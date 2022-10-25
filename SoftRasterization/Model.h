#pragma once
#include"tgaimage.h"

//Definition for objects

class Model {
public:
	Model(const std::string filename);
	Vec3f get_vertex(const int& f_id, const int& vth);
	Vec2f get_uv(const int& f_id, const int& vth);
	Vec3f get_normal(const Vec2f& uv);
	Vec3f get_normal(const int& face_id, const int& vth);
	Vec3f get_diffuse(const Vec2f& uv);
	float get_specular(const Vec2f& uv);
	int get_nverts();
	int get_nfaces();
private:
	std::vector<Vec3f>vertexs;    
	std::vector<Vec3f>normals;  
	std::vector<Vec2f>uvs;        //UV coordinate for each vertex
	std::vector<int>faces;        //Index of vertexes,every 3 adjacent items form a triangle  
	std::vector<int>normal_idx;
	std::vector<int>uv_idx;
	TGAImage NormalMap;
	TGAImage DiffuseMap;
	TGAImage SpecularMap;
	void load_texture(std::string filename, const std::string suffix, TGAImage& Map);
};
