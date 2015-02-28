#ifndef OBJ_PARSER_H
#define OBJ_PARSER_H

#include <vector>
#include <string>
#include <fstream>
#include <map>
#include <math.h>
#include <iostream>

using std::string;
using std::map;
using std::vector;
using std::fstream;

class vertex_data;
class mesh_data;
class obj_contents;

#define PRINTLINE std::cout << __FILE__ << ", " << __LINE__ << std::endl;

enum OBJ_DATA_TYPE { NOT_SET, F_DATA, V_DATA, VT_DATA, VN_DATA, VP_DATA };

const vector<float> extractFloats(const string &s);
const vector< vector<int> > extractFaceSequence(const string &s);
const vector<mesh_data> generateMeshes(const char* file_path);

class vertex_data
{
public:
	vertex_data(const vector<float> &p, const vector<float> &uv, const vector<float> &n) :
		v_data(p), vt_data(uv), vn_data(n) { setVertexData(); }

	~vertex_data(){};

	const int getUVOffset() const { return v_data.size() * sizeof(float); }
	const int getNOffset() const { return getUVOffset() + (vt_data.size() * sizeof(float)); }
	const int getStride() const { return face_data.size() * sizeof(float); }
	const int getVCount() const { return v_data.size(); }
	const int getVTCount() const { return vt_data.size(); }
	const int getVNCount() const { return vn_data.size(); }
	const vector<float> getVData() const { return v_data; }
	const vector<float> getVTData() const { return vt_data; }
	const vector<float> getVNData() const { return vn_data; }

	vector<float> getData() const { return face_data; }

private:
	void setVertexData();
	vector<float> v_data;
	vector<float> vt_data;
	vector<float> vn_data;
	vector<float> vp_data;

	vector<float> face_data;
};

class mesh_data
{
public:
	mesh_data() : total_face_count(0), vertex_count(0) {};
	~mesh_data(){};

	void addFace(vector<vertex_data> &data) { faces.push_back(data); total_face_count++; vertex_count += data.size(); }

	//this data keeps list of vertex information as used by OpenGL
	void addVData(const vector<float> &data) { all_v_data.insert(all_v_data.end(), data.begin(), data.end()); }
	void addVTData(const vector<float> &data) { all_vt_data.insert(all_vt_data.end(), data.begin(), data.end()); }
	void addVNData(const vector<float> &data) { all_vn_data.insert(all_vn_data.end(), data.begin(), data.end()); }
	void addVPData(const vector<float> &data) { all_vp_data.insert(all_vp_data.end(), data.begin(), data.end()); }

	void setPolygonSize(int n) { vertices_per_face = n; }

	const int getInterleaveStride() const { return interleave_stride; }
	const int getInterleaveVTOffset() const { return interleave_vt_offset; }
	const int getInterleaveVNOffset() const { return interleave_vn_offset; }
	const vector<float> getInterleaveData() const { return interleave_data; }

	//returns # of floats per vertex type
	const int getVCount() const { return v_count; }
	const int getVTCount() const { return vt_count; }
	const int getVNCount() const { return vn_count; }
	//returns # of vertices stored
	const int getVertexCount() const { return vertex_count; }
	//returns # of faces stored
	const int getFaceCount() const { return total_face_count; }
	const int getFaceSize() const { return vertices_per_face; }
	const int getFloatCount() const { return interleave_data.size(); }

	const vector<float> getVData() const { return all_v_data; }
	const vector<float> getVTData() const { return all_vt_data; }
	const vector<float> getVNData() const { return all_vn_data; }
	const vector<float> getVPData() const { return all_vp_data; }

	void setMeshData();

private:
	//vector of faces, each face is a vector of vertices
	vector< vector<vertex_data> > faces;

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
	vector<float> interleave_data;

	//counts floats per vertex of each type (3 = vec3, 4 = vec4)
	int v_count;
	int vt_count;
	int vn_count;
	int vp_count;
	//# of vertices are stored total
	int vertex_count;
	//# of faces are stored total
	int total_face_count;
	//3 for triangles, 4 for quads
	int vertices_per_face;
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

private:
	//uses vector<float> because # of floats per vertex varies
	//data direct from obj file, unformatted
	map<int, vector<float> > raw_v_data;
	map<int, vector<float> > raw_vt_data;
	map<int, vector<float> > raw_vn_data;
	map<int, vector<float> > raw_vp_data;

	vector<mesh_data> meshes;
};

#endif