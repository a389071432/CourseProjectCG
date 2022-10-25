#include<iostream>
#include<stdlib.h>
#include<conio.h>
#include<GLFW/glfw3.h>
#include<glm/glm.hpp>
#include<vector>
#include<algorithm>
#include<fstream>

using namespace std;


float w, h;                                        //Size of the camera's imaging plane
int Width, Height;                                 //Size of the viewport(screen)
int sample;                                        //Sample rate
float* frame_buffer_temp; 
char* frame_buffer;  
int diffusion_sample;
int filterSize;

float n = 2, f = 100, fov = 1.05, aspect = 1.33;   //Parameters for the viewing frustum 
glm::vec3 camera_pos;  
glm::vec3 light_dir; 
float I;                                          //Light intensity
float N0;                                         //Refraction index of the environment

int max_step;                                     //Maximum tracing depth
glm::vec3 ambient; 

//Definition of an objection
class MyObj {
public:
	string type;                                 //Sphere, Plane or Triangle
	string material_type;                        //Reflection, Diffusion or Refraction 
	glm::vec3 pos; 
	glm::vec3 diffuse; 
	glm::vec3 specular; 
	float N;                                     //Refraction index of the object
	glm::vec4 albedo;  
	MyObj(const string& _type, const string& _material, const glm::vec3& _pos) {
		type = _type;
		material_type = _material;
		pos = _pos;
		//Default settings for the object's material
		diffuse = glm::vec3(100, 0, 0);
		specular = glm::vec3(255, 0, 0);
		N = 1.2;
	}

	void setPosition(const glm::vec3 _pos) {
		pos = _pos;
	}

	void setMaterial(const glm::vec3& _diffuse, const glm::vec3& _specular, const glm::vec4& _albedo, const float& _N) {
		diffuse = _diffuse;
		specular = _specular;
		albedo = _albedo;
		N = _N;
	}

	//Find the first intersection point of the ray and the object
	virtual float FirstIntersectPoint(const glm::vec3& start, const glm::vec3& dir) {
		return -1;
	}
};

//Definition for a sphere
class Sphere :public MyObj {
public:
	float R;  //Radius
	Sphere(const string& _material, const glm::vec3& _pos, const float& _R) :MyObj("Sphere", _material, _pos) {
		R = _R;
	}

	virtual float FirstIntersectPoint(const glm::vec3& start, const glm::vec3& dir) {
		glm::vec3 C = pos;
		glm::vec3 e = start - C;
		float a = glm::dot(dir, dir);
		float b = glm::dot(dir, e);
		float c = glm::dot(e, e) - R * R;
		float delta = b * b - c;
		float delta_sqrt = sqrt(delta);
		if (delta < 0)
			return -1;
		float t1 = (-b - delta_sqrt) / a;
		float t2 = (-b + delta_sqrt) / a;
		if (t1 >= 1e-3)
			return t1;
		if (t2 >= 1e-3)
			return t2;
		return -1;
	}
};

//Definition for a infinite plane
class Plane :public MyObj {
public:
	glm::vec3 ori;    //Any point in the plane
	glm::vec3 n;      //Normal vector of the plane
	Plane(const glm::vec3& _ori, const glm::vec3& _n, const string& _material) :MyObj("Plane", _material, _ori) {
		ori = _ori;
		n = glm::normalize(_n);
	}
	virtual float FirstIntersectPoint(const glm::vec3& start, const glm::vec3& dir) {
		float t = glm::dot(n, ori - start) / glm::dot(n, dir);
		if (t >= 1e-3)
			return t;
		return -1;
	}
}; 

//Definition for a triangle
class Triangle :public MyObj {
public:
	glm::vec3 v0, v1, v2;
	glm::vec3 n; 
	Triangle(const glm::vec3& _v0, const glm::vec3& _v1, const glm::vec3& _v2, const string& _material) :MyObj("Triangle", _material, _v0) {
		v0 = _v0;
		v1 = _v1;
		v2 = _v2;
		n = glm::cross(v0 - v1, v0 - v2);
		//n = -n;
	}
	virtual float FirstIntersectPoint(const glm::vec3& start, const glm::vec3& dir) {
		float t = glm::dot(n, v0 - start) / glm::dot(n, dir);
		if (t < 1e-3)
			return -1;
		glm::vec3 P = start + dir * t;
		//Check whether the intersection is inside the triangle
		glm::vec3 a, b, c;
		a = glm::cross(P - v0, P - v1);
		b = glm::cross(P - v1, P - v2);
		c = glm::cross(P - v2, P - v0);
		if ((glm::dot(a, b) < 0) || (glm::dot(a, c) < 0) || (glm::dot(b, c) < 0))
			return -1;
		return t;
	}


};

class Light {
public:
	glm::vec3 color;
	float I;
	Light() {
		I = 1.0;
		color = glm::vec3(1, 1, 1);
	}
	void setI(const float& _I) {
		I = _I;
	}
	void setColor(const glm::vec3& _color) {
		color = _color;
	}
	virtual void f() {};
};

class DirectionLight :public Light {
public:
	DirectionLight(const glm::vec3 &_dir) :Light() {
		dir = glm::normalize(_dir);
	}
	glm::vec3 dir;
};

class SpotLight :public Light {
public:
	SpotLight(const glm::vec3 &_pos) {
		position = _pos;
	}
	glm::vec3 position;
};

vector<MyObj*>objects;        //All objects in the scene
vector<Light*>lights;         //All lights in the scene

//Record the computed color of a pixel in the frame buffer
void to_buffer(int x, int y, glm::vec3 &color) {
	int index = y * Width + x;
	if (index >= Width * Height || index < 0)
		return;
	index *= 3;
	frame_buffer_temp[index] = color[0];
	frame_buffer_temp[index + 1] = color[1];
	frame_buffer_temp[index + 2] = color[2];
}

//Calculate the reflective vector
glm::vec3 get_reflect_vector(const glm::vec3 &in, const glm::vec3 &n) {
	glm::vec3 temp = n * glm::dot(in, n);
	temp = 2.0f* (temp - in);
	return glm::normalize(in + temp);
}

//Calculate the refractive vector
glm::vec3 get_refract_vector(const glm::vec3& in, const glm::vec3& n, const float& N_in, const float& N_out) {
	float cos_r = glm::dot(in, n);
	float sin_r = sqrt(1 - cos_r * cos_r);
	float sin_i = N_in * sin_r / N_out;
	float cos_i = sqrt(1 - sin_i * sin_i);
	glm::vec3 temp = glm::cross(in, n);
	glm::vec3 x = glm::cross(temp, n);
	x = glm::normalize(x);
	return glm::normalize(-n * cos_i + x * sin_i);
}

//Check whether there's any object between a point and the light
bool checkShelter(const glm::vec3 pos, const glm::vec3 light_pos) {
	glm::vec3 start = pos;
	glm::vec3 dir = glm::normalize(light_pos - pos);
	int i;
	float max_t = (light_pos - pos)[0] / dir[0];  //¹âÔ´ÓëÄ¿±êµãµÄÖ®¼äµÄt¾àÀë
	for (i = 0; i < objects.size(); i++) {
		MyObj* obj = objects[i];
		if (obj->type == "Sphere") {
			float t = dynamic_cast<Sphere*>(obj)->FirstIntersectPoint(start, dir);
			if (t > 1e-3&&t < max_t)
				return 1;
		}
		else if (obj->type == "Plane") {
			float t = dynamic_cast<Plane*>(obj)->FirstIntersectPoint(start, dir);
			if (t > 1e-3&&t < max_t)
				return 1;
		}
		else if (obj->type == "Triangle") {
			float t = dynamic_cast<Triangle*>(obj)->FirstIntersectPoint(start, dir);
			if (t > 1e-3 && t < max_t)
				return 1;
		}

	}
	return 0;
}

//Find the first intersection point of the ray and all objects in the scene
void findFirstIntersectPoint(const glm::vec3& start, const glm::vec3& dir, int &obj_index, glm::vec3 &intersectPoint, glm::vec3 &n) {
	obj_index = -1;
	MyObj* obj = NULL;
	float min_t = 0x3f3f3f;
	string type;
	for (int i = 0; i < objects.size(); i++) {
		obj = objects[i];
		if (obj->type == "Sphere") {
			float t = dynamic_cast<Sphere*>(obj)->FirstIntersectPoint(start, dir);
			if (t > 1e-3 && t < min_t)
				obj_index = i, min_t = t, type = "Sphere";
		}
		else if (obj->type == "Plane") {
			float t = dynamic_cast<Plane*>(obj)->FirstIntersectPoint(start, dir);
			if (t > 1e-3 && t < min_t)
				obj_index = i, min_t = t, type = "Plane";
		}
		else if (obj->type == "Triangle") {
			float t = dynamic_cast<Triangle*>(obj)->FirstIntersectPoint(start, dir);
			if (t > 1e-3 && t < min_t)
				obj_index = i, min_t = t, type = "Triangle";
		}
	}
	if (obj_index >= 0) {
		intersectPoint = start + dir * min_t;
		if (type == "Sphere")
			n = glm::normalize(intersectPoint - objects[obj_index]->pos);
		else if (type == "Plane")
			n = dynamic_cast<Plane*>(objects[obj_index])->n;
		else if (type == "Triangle")
			n = dynamic_cast<Triangle*>(objects[obj_index])->n;


	}
	return;
}

//Apply local illumination to a point using Phong model, all light sources are included
glm::vec3 calcu_local_illumination(const int &obj_index, const glm::vec3 & intersectPoint, const glm::vec3& n, const int& light_index, const glm::vec3 &dir) {
	float I_diffuse;
	float I_specular;
	MyObj* obj = objects[obj_index];
	Light* light = lights[light_index];
	glm::vec3 ldir;
	if (dynamic_cast<DirectionLight*>(light) != NULL)   //Æ½ÐÐ¹â
		ldir = dynamic_cast<DirectionLight*>(light)->dir;
	else if (dynamic_cast<SpotLight*>(light) != NULL)                          //µã¹âÔ´
		ldir = glm::normalize(dynamic_cast<SpotLight*>(light)->position - intersectPoint);
	I_diffuse = max(0.0f, glm::dot(ldir, n));
	I_specular = pow(max(0.0f, glm::dot(glm::normalize(ldir + dir), n)), double(obj->albedo[1]));
	glm::vec3 color = (I_diffuse + I_specular) * obj->diffuse*light->I;
	return obj->albedo[0] * color;
}

//The main workflow of ray-tracer
glm::vec3 RayTraceForOnePixel(const glm::vec3& start, const glm::vec3& dir, bool in_object, int step) {
	if (step == max_step)   
		return ambient;

	int obj_index;                  //Index of the first object intersecting with the ray
	glm::vec3 intersectPoint;  
	glm::vec3 n;    
	findFirstIntersectPoint(start, dir, obj_index, intersectPoint, n);  

	if (obj_index == -1)           //No intersection detected, return ambient
		return ambient;
	glm::vec3 ans(0, 0, 0);
	if (in_object == 0) {
		for (int i = 0; i < lights.size(); i++) {
			//If the intersection is not inside the object and there are nothing between it and the light, calculate its local illumination
			Light* light = lights[i];
			bool tag;
			if (dynamic_cast<DirectionLight*>(light) != NULL)
				tag = checkShelter(intersectPoint, intersectPoint + 10000.0f * dynamic_cast<DirectionLight*>(light)->dir);
			else if (dynamic_cast<SpotLight*>(light) != NULL)
				tag = checkShelter(intersectPoint, dynamic_cast<SpotLight*>(light)->position);
			if (!tag)
				ans += calcu_local_illumination(obj_index, intersectPoint, n, i, -dir);
		}
	}
	MyObj* obj = objects[obj_index];
	//Trace on the reflective path
	glm::vec3 reflect = get_reflect_vector(-dir, n);  
	if(obj->material_type=="Reflect")
	   ans += obj->albedo[2] * RayTraceForOnePixel(intersectPoint, reflect, in_object, step + 1); //µÝ¹é×·×Ù·´ÉäÂ·¾¶
	if (obj->material_type == "Diffusion")
	{
		for (int i = 0; i < diffusion_sample; i++)
		{
			glm::vec3 reflect_diffusion;
			float x = 1.0f*(rand() % 101) / 100;
			float y = 1.0f*(rand() % 101) / 100;
			float z = 1.0f*(rand() % 101) / 100;
			reflect_diffusion = glm::normalize(glm::vec3(x, y, z));
			float dot = glm::dot(reflect_diffusion, reflect);
			if (dot < 0)
			{
				reflect_diffusion = -reflect_diffusion;
				dot = -dot;
			}
			ans += dot*glm::normalize(obj->diffuse) * RayTraceForOnePixel(intersectPoint, reflect_diffusion, in_object, step + 1);
		}
	}
	//Trace on the refractive path
	if (obj->albedo[3] > 0) {
		float cos0 = glm::dot(-dir, n);
		float sin0 = sqrt(1 - cos0 * cos0);
		float c0 = asin(obj->N / N0);           //Critical angle of total reflection
		float sinc = sin(c0);
		if (in_object == 0) {                   //Handle the case where the intersection is on the outer surface of the object
			glm::vec3 refract = get_refract_vector(-dir, n, N0, obj->N);
			ans += obj->albedo[3] * RayTraceForOnePixel(intersectPoint, refract, 1 - in_object, step + 1);
		}
		else if ((in_object == 1))              //Handle the case where the intersection is on the inner surface of the object
		{
			glm::vec3 refract = get_refract_vector(-dir, -n, obj->N, N0);
			ans += obj->albedo[3] * RayTraceForOnePixel(intersectPoint, refract, 1 - in_object, step + 1);
		}
	}

	return ans;
}

//Trace for a pixel
//A pixel may be subdivided into several grids for higher rendering quality
void RayTraceOnce() {
	int totalx = sample * Width, totaly = sample * Height;
	int sample_squre = sample * sample;
	int px, py;
#pragma omp parallel for
	for (px = 0; px < Width; px++) {
		if (px % 10 == 0)
			cout << px << endl;
		for (py = 0; py < Height; py++) {
			glm::vec3 final_color(0, 0, 0);
			int tx = sample * px, ty = sample * py;
			for (tx = sample * px; tx < sample * (px + 1); tx++) {
				for (ty = sample * py; ty < sample * (py + 1); ty++) {
					float X = 1.0f * tx * w / totalx, Y = 1.0f * ty * h / totaly;
					X -= w / 2, Y -= h / 2;
					glm::vec3 pos(X, Y, -n);
					glm::vec3 dir = glm::normalize(pos - camera_pos);
					final_color += RayTraceForOnePixel(camera_pos, dir, 0, 0);
				}
			}
			final_color /= 1.0f*sample_squre;
			to_buffer(px, py, final_color);
		}
	}
}

//Unify all RGB colors to the range of [0,255]
void NormalizeColor() {
	float MAX = -1;
	float MIN = 0x3F3F3F;
	int i;
	for (i = 0; i < Width * Height * 3; i++)
		MAX = max(MAX, frame_buffer_temp[i]);

	for (i = 0; i < Width * Height * 3; i++)
		frame_buffer[i] = 255 * frame_buffer_temp[i] / MAX;

	for (i = 0; i < Width * Height * 3; i++) {
		if (frame_buffer_temp[i] > 255)
			frame_buffer[i] = 255;
		else
			frame_buffer[i] = frame_buffer_temp[i];
	}
}

//Moving average filter applying to the rendering image 
void MovingAverageFilter()
{
	if (filterSize == 0)
		return;
	for (int px = 0; px < Width-filterSize; px++)
	{
		for (int py = 0; py < Height-filterSize; py++)
		{
			glm::vec3 aveColor(0, 0, 0);
			for (int tx = 0; tx < filterSize;tx++)
			{
				for (int ty = 0; ty < filterSize; ty++)
				{
					int tempIndex= (py+ty) * Width + (px+tx);
					tempIndex *= 3;
					aveColor[0] += frame_buffer_temp[tempIndex];
					aveColor[1] += frame_buffer_temp[tempIndex+1];
					aveColor[2] += frame_buffer_temp[tempIndex+2];
				}
			}
			aveColor /= filterSize * filterSize;
			int index = py * Width + px;
			index *= 3;
			frame_buffer_temp[index] = aveColor[0];
			frame_buffer_temp[index + 1] = aveColor[1];
			frame_buffer_temp[index + 2] = aveColor[2];
		}
	}
}

//initial settings for rendering
void init() {
	camera_pos = glm::vec3(0, 0, 0);
	h = n * tan(fov / 2) * 2;  
	w = h * aspect;
	Width = 800, Height = 600;  
	frame_buffer_temp = new float[Width * Height * 3];
	frame_buffer = new char[Width * Height * 3];
	sample = 4;
	diffusion_sample = 1;
	filterSize = 0;
	ambient = glm::vec3(0, 0, 0);
	N0 = 1.0;
	max_step = 3;
}

int main()
{

	init();
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	GLFWwindow * window = glfwCreateWindow(Width, Height, "First window", nullptr, nullptr);
	glfwMakeContextCurrent(window);

	//Sphere* sphere0 = new Sphere("Refract", glm::vec3(7, 5, -25), 3);
	//Sphere* sphere1 = new Sphere("Reflect", glm::vec3(-10, 0, -80), 7);
	//sphere0->setMaterial(glm::vec3(255, 255, 255), glm::vec3(0, 0, 0), glm::vec4(0, 10, 0, 0.8), 1.5);  //ÕÛÉä²ÄÖÊÇò£¨Í¸Ã÷£©
	////sphere1->setMaterial(glm::vec3(255, 255, 255), glm::vec3(0, 0, 0),glm::vec4(0.1,10,0.7,0),1.0); //·´Éä²ÄÖÊÇò
	//sphere1->setMaterial(glm::vec3(255, 255, 255), glm::vec3(0, 0, 0), glm::vec4(0.01, 10, 0.85, 0), 1.0);

	//Plane* plane0 = new Plane(glm::vec3(0, -150, 0), glm::vec3(0, 1, 0), "Diffusion"); //·´Éä²ÄÖÊÆ½Ãæ
	//plane0->setMaterial(glm::vec3(255, 255, 255), glm::vec3(0, 0, 0), glm::vec4(0.7, 10, 0.3, 0), 1.0);

	//Plane* plane1 = new Plane(glm::vec3(0, 150, 0), glm::vec3(0, -1, 0), "Diffusion");
	//plane1->setMaterial(glm::vec3(255, 255, 255), glm::vec3(0, 0, 0), glm::vec4(0.8, 10, 0.2, 0), 1.0);

	//Plane* plane2 = new Plane(glm::vec3(-100, 0, 0), glm::vec3(1, 0, 0), "Reflect");
	//plane2->setMaterial(glm::vec3(239, 191, 255), glm::vec3(0, 0, 0), glm::vec4(0.7, 10, 0.25, 0), 1.0);

	//Plane* plane3 = new Plane(glm::vec3(100, 0, 0), glm::vec3(-1, 0, 0), "Reflect");
	//plane3->setMaterial(glm::vec3(239, 191, 255), glm::vec3(0, 0, 0), glm::vec4(0.7, 10, 0.25, 0), 1.0);

	//Plane* plane4 = new Plane(glm::vec3(0, 0, -400), glm::vec3(0, 0, 1), "Reflect");
	////plane4->setMaterial(glm::vec3(239, 191, 255), glm::vec3(0, 0, 0), glm::vec4(0.3, 10, 0.7, 0), 1.0);
	//plane4->setMaterial(glm::vec3(255, 255, 255), glm::vec3(0, 0, 0), glm::vec4(0.3, 10, 0.7, 0), 1.0);

	//Triangle* triangle = new Triangle(glm::vec3(-20, -20, -85), glm::vec3(-35, -30, -60), glm::vec3(-5, -30, -60), "Reflect");
	//triangle->setMaterial(glm::vec3(0, 0, 255), glm::vec3(0, 0, 0), glm::vec4(0.01, 10, 0.85, 0), 1.0);

	//objects.push_back(sphere0);
	//objects.push_back(sphere1);
	//objects.push_back(plane0);
	//objects.push_back(plane1);
	//objects.push_back(plane2);
	//objects.push_back(plane3);
	//objects.push_back(plane4);
	////objects.push_back(triangle);

	//SpotLight* spotlight0 = new SpotLight(glm::vec3(0, 0, 0));
	//spotlight0->setI(0.4);
	//SpotLight* spotlight1 = new SpotLight(glm::vec3(0, 120, -250));
	//spotlight1->setI(0.4);
	//SpotLight* spotlight2 = new SpotLight(glm::vec3(0, 120, -200));
	//spotlight2->setI(0.5);
	//SpotLight* spotlight3 = new SpotLight(glm::vec3(0, 120, -350));
	//spotlight3->setI(0.5);
	//DirectionLight* dirlight = new DirectionLight(glm::vec3(0, 0, 1));
	//lights.push_back(spotlight0);
	////lights.push_back(spotlight1);
	//lights.push_back(spotlight2);
	//lights.push_back(spotlight3);

	//Add objects to the scene
	Sphere* sphere0 = new Sphere("Refract", glm::vec3(7, 5, -25), 4);
	Sphere* sphere1 = new Sphere("Reflect", glm::vec3(-10, 0, -80), 10);
	sphere0->setMaterial(glm::vec3(255, 255, 255), glm::vec3(0, 0, 0), glm::vec4(0, 10, 0, 0.8), 1.5);  
	sphere1->setMaterial(glm::vec3(255, 255, 255), glm::vec3(0, 0, 0), glm::vec4(0.01, 10, 0.85, 0), 1.0);

	Plane* plane0 = new Plane(glm::vec3(0, -50, 0), glm::vec3(0, 1, 0), "Reflect"); 
	plane0->setMaterial(glm::vec3(247, 238, 214), glm::vec3(0, 0, 0), glm::vec4(0.7, 10, 0.3, 0), 1.0);

	Plane* plane1 = new Plane(glm::vec3(0, 55, 0), glm::vec3(0, -1, 0), "Reflect");
	plane1->setMaterial(glm::vec3(247, 238, 214), glm::vec3(0, 0, 0), glm::vec4(0.8, 10, 0.2, 0), 1.0);

	Plane* plane2 = new Plane(glm::vec3(-70, 0, 0), glm::vec3(1, 0, 0), "Reflect");
	plane2->setMaterial(glm::vec3(239, 191, 255), glm::vec3(0, 0, 0), glm::vec4(0.7, 10, 0.25, 0), 1.0);

	Plane* plane3 = new Plane(glm::vec3(70, 0, 0), glm::vec3(-1, 0, 0), "Reflect");
	plane3->setMaterial(glm::vec3(239, 191, 255), glm::vec3(0, 0, 0), glm::vec4(0.7, 10, 0.25, 0), 1.0);

	Plane* plane4 = new Plane(glm::vec3(0, 0, -175), glm::vec3(0, 0, 1), "Reflect");
	plane4->setMaterial(glm::vec3(247, 238, 214), glm::vec3(0, 0, 0), glm::vec4(0.3, 10, 0.7, 0), 1.0);

	Triangle* triangle = new Triangle(glm::vec3(-20, -20, -85), glm::vec3(-35, -30, -60), glm::vec3(-5, -30, -60), "Reflect");
	triangle->setMaterial(glm::vec3(0, 0, 255), glm::vec3(0, 0, 0), glm::vec4(0.01, 10, 0.85, 0), 1.0);

	objects.push_back(sphere0);
	objects.push_back(sphere1);
	objects.push_back(plane0);
	objects.push_back(plane1);
	objects.push_back(plane2);
	objects.push_back(plane3);
	objects.push_back(plane4);
	//objects.push_back(triangle);

	//Add light sources to the scene
	SpotLight* spotlight0 = new SpotLight(glm::vec3(0, 0, 0));
	spotlight0->setI(0.4);
	SpotLight* spotlight1 = new SpotLight(glm::vec3(0, 27, -120));
	spotlight1->setI(0.75);
	DirectionLight* dirlight = new DirectionLight(glm::vec3(0, 0, 1));
	lights.push_back(spotlight0);
	lights.push_back(spotlight1);

	glClear(GL_COLOR_BUFFER_BIT);
	glfwPollEvents();
	RayTraceOnce();
	MovingAverageFilter();
	NormalizeColor();

	ofstream ofs;
	ofs.open("E:/raytracer.ppm", std::ios::binary);
	ofs << "P6\n" << Width << " " << Height << "\n255\n";
	for (int index = 0; index < Width * Height; index += 3)
		ofs << (char)frame_buffer[index] << (char)frame_buffer[index + 1] << (char)frame_buffer[index + 2];
	ofs.close();

	//Show the rendering result
	glDrawPixels(800, 600, GL_RGB, GL_UNSIGNED_BYTE, frame_buffer);
	glfwSwapBuffers(window);

	memset(frame_buffer, 0, sizeof(frame_buffer));
	glFlush();

	system("pause");
	return 0;
}