#pragma once
#include<vector>
#include<iostream>
#include<stdlib.h>
#include<fstream>
#include<sstream>
#include"tgaimage.h"
#include"geometry.h"
#include"Model.h"

using namespace std;

Model::Model(const std::string filename) : vertexs(), uvs(), normals(), faces(), uv_idx(), normal_idx(), DiffuseMap(), NormalMap(), SpecularMap() {
	std::ifstream in;
	in.open(filename, std::ifstream::in);
	if (in.fail()) return;
	std::string line;
	while (!in.eof()) {
		std::getline(in, line);
		std::istringstream iss(line.c_str());
		char trash;
		if (!line.compare(0, 2, "v ")) {
			iss >> trash;
			Vec3f v;
			for (int i = 0; i < 3; i++) iss >> v[i];
			vertexs.push_back(v);
		}
		else if (!line.compare(0, 3, "vn ")) {
			iss >> trash >> trash;
			Vec3f n;
			for (int i = 0; i < 3; i++) iss >> n[i];
			normals.push_back(n.normalize());
		}
		else if (!line.compare(0, 3, "vt ")) {
			iss >> trash >> trash;
			Vec2f uv;
			for (int i = 0; i < 2; i++) iss >> uv[i];
			uvs.push_back(uv);
		}
		else if (!line.compare(0, 2, "f ")) {
			int f, t, n;
			iss >> trash;
			int cnt = 0;
			while (iss >> f >> trash >> t >> trash >> n) {
				faces.push_back(--f);
				uv_idx.push_back(--t);
				normal_idx.push_back(--n);
				cnt++;
			}
			if (3 != cnt) {
				std::cerr << "Error: the obj file is supposed to be triangulated" << std::endl;
				in.close();
				return;
			}
		}
	}
	in.close();
	std::cerr << "# v# " << get_nverts() << " f# " << get_nfaces() << " vt# " << uvs.size() << " vn# " << normals.size() << std::endl;
	load_texture(filename, "_diffuse.tga", DiffuseMap);
	load_texture(filename, "_nm_tangent.tga", NormalMap);
	load_texture(filename, "_spec.tga", SpecularMap);
}

int Model::get_nverts() {
	return vertexs.size();
}

int Model::get_nfaces() {
	return faces.size() / 3;
}

Vec3f Model::get_vertex(const int& face_id, const int& vth) {
	return vertexs[faces[face_id * 3 + vth]];
}

void Model::load_texture(std::string filename, const std::string suffix, TGAImage& map) {
	size_t dot = filename.find_last_of(".");
	if (dot == std::string::npos) return;
	std::string texfile = filename.substr(0, dot) + suffix;
	std::cerr << "texture file " << texfile << " loading " << (map.read_tga_file(texfile.c_str()) ? "ok" : "failed") << std::endl;
	map.flip_vertically();
}

Vec3f Model::get_diffuse(const Vec2f& uv) {
	TGAColor color = DiffuseMap.get(uv[0] * DiffuseMap.get_width(), uv[1] * DiffuseMap.get_height());
	return Vec3f(1.0f * color.bgra[2], 1.0f * color.bgra[1], 1.0f * color.bgra[0]);
}

Vec3f Model::get_normal(const Vec2f& uvf) {
	TGAColor c = NormalMap.get(uvf[0] * NormalMap.get_width(), uvf[1] * NormalMap.get_height());
	Vec3f res;
	for (int i = 0; i < 3; i++)
		res[2 - i] = c[i] / 255. * 2 - 1;
	return res;
}

float Model::get_specular(const Vec2f& uv) {
	return SpecularMap.get(uv[0] * SpecularMap.get_width(), uv[1] * SpecularMap.get_height())[0];
}

Vec2f Model::get_uv(const int& iface, const int& nthvert) {
	return uvs[uv_idx[iface * 3 + nthvert]];
}

Vec3f Model::get_normal(const int& iface, const int& nthvert) {
	return normals[normal_idx[iface * 3 + nthvert]];
}