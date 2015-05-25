#ifndef OBJ_PARSER_H
#define OBJ_PARSER_H

#include <vector>
#include <string>
#include <fstream>
#include <map>
#include <math.h>
#include <iostream>
#include <glm.hpp>

using std::string;
using std::map;
using std::vector;
using std::fstream;

class vertex_data;
class material_data;
class mesh_data;
class obj_contents;

#define PRINTLINE std::cout << __FILE__ << ", " << __LINE__ << std::endl;

enum DATA_TYPE { UNDEFINED, OBJ_MTLLIB, OBJ_F, OBJ_V, OBJ_VT, OBJ_VN, OBJ_VP, OBJ_G, OBJ_USEMTL,
				MTL_NEWMTL, MTL_KA, MTL_KD, MTL_KS, MTL_NS, MTL_D, 
				MTL_MAP_KA, MTL_MAP_KD, MTL_MAP_KS, MTL_MAP_D, MTL_MAP_NS, MTL_MAP_BUMP, MTL_MAP_DISP, MTL_DECAL
};

const vector<float> extractFloats(const string &s);
const vector< vector<int> > extractFaceSequence(const string &s);
const vector<mesh_data> generateMeshes(const char* file_path);
const map<string, material_data> generateMaterials(const char* file_path);
const DATA_TYPE getDataType(const string &line);
const string extractName(const string &line);

class vertex_data
{
public:
	vertex_data(const vector<float> &p, const vector<float> &uv, const vector<float> &n) :
		v_data(p), vt_data(uv), vn_data(n) { 
		v_count = p.size();
		vt_count = uv.size();
		vn_count = n.size();
		setVertexData();

		if (v_count < 3 || v_count > 4)
			throw;

		if (vt_count != 0 && vt_count != 2)
			throw;

		if ((vn_count < 3 || vn_count > 4) && vn_count != 0)
			throw;
	}

	~vertex_data(){};

	const int getUVOffset() const { return v_data.size() * sizeof(float); }
	const int getNOffset() const { return getUVOffset() + (vt_data.size() * sizeof(float)); }
	const int getStride() const { return all_data.size() * sizeof(float); }
	const int getVSize() const { return v_count; }
	const int getVTSize() const { return vt_count; }
	const int getVNSize() const { return vn_count; }
	const vector<float> getVData() const { return v_data; }
	const vector<float> getVTData() const { return vt_data; }
	const vector<float> getVNData() const { return vn_data; }

	//modifyPosition does not affect normals
	void modifyPosition(const glm::mat4 &translation_matrix);
	//rotate modifies position data and normals
	void rotate(const glm::mat4 &rotation_matrix);

	vector<float> getAllData() const { return all_data; }

	bool operator == (const vertex_data &other);
	bool operator != (const vertex_data &other) { return !((*this) == other); }

	float x, y, z, w;
	glm::vec2 xy;
	glm::vec3 xyz;
	glm::vec4 xyzw;

	float u, v;
	glm::vec2 uv;
	
	float n_x, n_y, n_z, n_w;
	glm::vec2 n_xy;
	glm::vec3 n_xyz;
	glm::vec4 n_xyzw;

private:
	
	void setVertexData();
	vector<float> v_data;
	vector<float> vt_data;
	vector<float> vn_data;
	vector<float> vp_data;

	vector<float> all_data;

	unsigned short v_count, vt_count, vn_count, vp_count;
};

class mesh_data
{
public:
	mesh_data() : total_face_count(0), vertex_count(0) {};
	~mesh_data(){};

	void setMeshName(string n) { mesh_name = n; }
	void setMaterialName(string n) { material_name = n; }

	const string getMaterialName() const { return material_name; }
	const string getMeshlName() const { return mesh_name; }

	//this data keeps list of vertex information as used by OpenGL
	void addVData(const vector<float> &data) { all_v_data.insert(all_v_data.end(), data.begin(), data.end()); }
	void addVTData(const vector<float> &data) { all_vt_data.insert(all_vt_data.end(), data.begin(), data.end()); }
	void addVNData(const vector<float> &data) { all_vn_data.insert(all_vn_data.end(), data.begin(), data.end()); }
	void addVPData(const vector<float> &data) { all_vp_data.insert(all_vp_data.end(), data.begin(), data.end()); }
	void addFace(const vector<vertex_data> &data);

	const int getInterleaveStride() const { return interleave_stride; }
	const int getInterleaveVTOffset() const { return interleave_vt_offset; }
	const int getInterleaveVNOffset() const { return interleave_vn_offset; }
	const vector<float> getInterleaveData() const;
	void getIndexedVertexData(vector<unsigned int> &indices, vector<float> &v_data, vector<float> &vt_data, vector<float> &vn_data) const;
	const vector<float> getIndexedVertexData(vector<unsigned int> &indices) const;

	vector< vector<float> > getTriangles();
	vector< vector<float> > getQuads();

	//returns # of floats per vertex type
	const int getVSize() const { return v_size; }
	const int getVTSize() const { return vt_size; }
	const int getVNSize() const { return vn_size; }
	//returns # of vertices stored
	const int getVertexCount() const { return vertex_count; }
	//returns # of faces stored
	const int getFaceCount() const { return total_face_count; }
	const int getFloatCount() const { return total_float_count; }

	const vector<float> getVData() const { return all_v_data; }
	const vector<float> getVTData() const { return all_vt_data; }
	const vector<float> getVNData() const { return all_vn_data; }
	const vector<float> getVPData() const { return all_vp_data; }

	const vector<float> getData(DATA_TYPE) const;

	//modifyPosition does not affect normals
	void modifyPosition(const glm::mat4 &translation_matrix);
	//rotate modifies position data and normals
	void rotate(const glm::mat4 &rotation_matrix);

	vector< std::pair<glm::vec4, glm::vec4> > getMeshEdgesVec4() const;
	vector< std::pair<glm::vec3, glm::vec3> > getMeshEdgesVec3() const;
	vector< vector<glm::vec4> >getMeshTrianglesVec4() const;
	vector< vector<glm::vec3> >getMeshTrianglesVec3() const;

	void setMeshData();

private:
	//vector of faces, each face is a vector of vertices
	vector< vector<vertex_data> > faces;
	map<unsigned int, vertex_data > vertex_map;
	vector<unsigned int> element_index;

	string mesh_name;
	string material_name;

	//contain all vertices for all faces, accessed when
	//vertices are not to be interleaved
	vector<float> all_v_data;
	vector<float> all_vt_data;
	vector<float> all_vn_data;
	vector<float> all_vp_data;

	//interleaved data values
	int interleave_stride;
	int interleave_vt_offset;
	int interleave_vn_offset;

	//counts floats per vertex of each type (3 = vec3, 4 = vec4)
	int v_size;
	int vt_size;
	int vn_size;
	int vp_size;
	//# of vertices are stored total
	int vertex_count;
	//# of faces are stored total
	int total_face_count;
	int total_float_count;
};

class obj_contents
{
public:
	obj_contents(const char* obj_file);
	~obj_contents(){};

	const map<int, vector<float> > getAllRawVData() const { return raw_v_data; }
	const map<int, vector<float> > getAllRawVTData() const { return raw_vt_data; }
	const map<int, vector<float> > getAllRawVNData() const { return raw_vn_data; }
	const map<int, vector<float> > getAllRawVPData() const { return raw_vp_data; }

	const vector<float> getRawVData(int n) const { return raw_v_data.at(n); }
	const vector<float> getRawVTData(int n) const { return raw_vt_data.at(n); }
	const vector<float> getRawVNData(int n) const { return raw_vn_data.at(n); }
	const vector<float> getRawVPData(int n) const { return raw_vp_data.at(n); }

	const int getMeshCount() const { return meshes.size(); }
	const vector<mesh_data> getMeshes() const { return meshes; }

	vector<string> getErrors() const { return error_log; }

	const string getMTLFilename() const { return mtl_filename; }

private:
	void addRawData(const vector<float> &floats, DATA_TYPE dt);
	//uses vector<float> because # of floats per vertex varies
	//data direct from obj file, unformatted
	map<int, vector<float> > raw_v_data;
	map<int, vector<float> > raw_vt_data;
	map<int, vector<float> > raw_vn_data;
	map<int, vector<float> > raw_vp_data;
	string mtl_filename;

	int v_index_counter;
	int vt_index_counter;
	int vn_index_counter;
	int vp_index_counter;

	vector<string> error_log;
	vector<mesh_data> meshes;
};

class material_data
{
public:
	material_data(){};
	material_data(string s): material_name(s) {};
	~material_data(){};

	void setMaterialName(string s) { material_name = s; }
	void setTextureFilename(string s) { texture_filename = s; }

	void setData(DATA_TYPE dt, vector<float> floats) { data[dt] = floats; }

	const string getTextureFilename() const { return texture_filename; }
	const string getMaterialName() const { return material_name; }
	const vector<float> getData(DATA_TYPE dt) const;
private:
	string material_name;
	string texture_filename;

	map<DATA_TYPE, vector<float> > data;
};

class mtl_contents
{
public:
	mtl_contents(const char* mtl_file);
	~mtl_contents(){};

	const string getTextureFilename(string material_name) const;
	const map<string, material_data> getMaterials() const { return materials; }

private:
	vector<string> error_log;
	map<string, material_data> materials;
};

#endif