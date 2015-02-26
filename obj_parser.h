#ifndef OBJ_PARSER_H
#define OBJ_PARSER_H

#include <vector>
#include <string>
#include <fstream>
#include <map>
#include <glm.hpp>

using std::map;
using glm::vec2;
using glm::vec3;
using glm::vec4;
using std::vector;

class vertex_data
{
public:
	/*
	//constructors with vec4 as position
	vertex_data(const vec4 &p);
	vertex_data(const vec4 &p, const vec2 &uv);
	vertex_data(const vec4 &p, const vec3 &n);
	vertex_data(const vec4 &p, const vec2 &uv, const vec3 &n);

	//constructors with vec3 as position
	vertex_data(const vec3 &p) : vertex_data(vec4(p, 1.0f)) {};
	vertex_data(const vec3 &p, const vec2 &uv) : vertex_data(vec4(p, 1.0f), uv) {};
	vertex_data(const vec3 &p, const vec3 &n) : vertex_data(vec4(p, 1.0f), n) {};
	vertex_data(const vec3 &p, const vec2 &uv, const vec3 &n) : vertex_data(vec4(p, 1.0f), uv, n) {};
	*/
	vertex_data(const vector<float> &p, const vector<float> &uv, const vector<float> &n) :
		position(p), uv_coords(uv), normals(n) { setData(); }

	~vertex_data(){};

	const int getUVOffset() const { return position.size() * sizeof(float); }
	const int getNOffset() const { return getUVOffset() + (uv_coords.size() * sizeof(float)); }
	const int getStride() const { return data.size() * sizeof(float); }
	const bool containsUV() const { return uv_coords.size(); }
	const bool containsN() const { return normals.size(); }

private:
	void setData();
	vector<float> position;
	vector<float> uv_coords;
	vector<float> normals;

	vector<float> data;
};

/*
vertex_data::vertex_data(const vec4 &p)
{
	position.push_back(p.x);
	position.push_back(p.y);
	position.push_back(p.z);
	position.push_back(p.w);

	setData();
}

vertex_data::vertex_data(const vec4 &p, const vec2 &uv)
{
	position.push_back(p.x);
	position.push_back(p.y);
	position.push_back(p.z);
	position.push_back(p.w);

	uv_coords.push_back(uv.x);
	uv_coords.push_back(uv.y);

	setData();
}

vertex_data::vertex_data(const vec4 &p, const vec3 &n)
{
	position.push_back(p.x);
	position.push_back(p.y);
	position.push_back(p.z);
	position.push_back(p.w);

	normals.push_back(n.x);
	normals.push_back(n.y);
	normals.push_back(n.z);

	setData();
}

vertex_data::vertex_data(const vec4 &p, const vec2 &uv, const vec3 &n)
{
	position.push_back(p.x);
	position.push_back(p.y);
	position.push_back(p.z);
	position.push_back(p.w);

	uv_coords.push_back(uv.x);
	uv_coords.push_back(uv.y);

	normals.push_back(n.x);
	normals.push_back(n.y);
	normals.push_back(n.z);

	setData();
}
*/

void vertex_data::setData()
{
	for (vector<float>::const_iterator it = position.begin(); it != position.end(); it++)
		data.push_back(*it);

	for (vector<float>::const_iterator it = uv_coords.begin(); it != uv_coords.end(); it++)
		data.push_back(*it);

	for (vector<float>::const_iterator it = normals.begin(); it != normals.end(); it++)
		data.push_back(*it);
}

template<typename v_type, typename vt_type, typename vp_type>
class obj_contents
{
public:
	obj_contents();
	~obj_contents(){};

	const int getStride() const { return face_data.begin()->size(); }

private:
	void setData();
	map<int, v_type> v_data;
	map<int, vec2> vt_data;
	map<int, vec3> n_data;
	map<int, vp_type> vp_data;

	//vector of faces, each face is a vector of vertices
	vector< vector<vertex_data> > face_data;
	vector<float> boj_data;
};

#endif