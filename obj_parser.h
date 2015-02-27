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

enum OBJ_DATA_TYPE { NOT_SET, FACE_DEF, POSITION, UV, NORMAL, PARAM_SPACE };

const vector<float> extractFloats(const string &s);
const vector< vector<int> > extractFaceSequence(const string &s);

class vertex_data
{
public:
	vertex_data(const vector<float> &p, const vector<float> &uv, const vector<float> &n) :
		v_data(p), vt_data(uv), vn_data(n) { setData(); }

	~vertex_data(){};

	const int getUVOffset() const { return v_data.size() * sizeof(float); }
	const int getNOffset() const { return getUVOffset() + (vt_data.size() * sizeof(float)); }
	const int getStride() const { return face_data.size() * sizeof(float); }
	const bool containsUV() const { return vt_data.size(); }
	const bool containsN() const { return vn_data.size(); }
	const int getVCount() const { return v_data.size(); }
	const int getVTCount() const { return vt_data.size(); }
	const int getVNCount() const { return vn_data.size(); }
	const vector<float> getVData() const { return v_data; }
	const vector<float> getVTData() const { return vt_data; }
	const vector<float> getVNData() const { return vn_data; }

	vector<float> getData() const { return face_data; }

private:
	void setData();
	vector<float> v_data;
	vector<float> vt_data;
	vector<float> vn_data;

	vector<float> face_data;
};

class obj_contents
{
public:
	obj_contents(const char* obj_file);
	~obj_contents(){};

	const int getFloatCount() const { return interleave_data.size(); }

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
	const int getFaceCount() const { return face_count; }

	const vector<float> getVData() const { return all_v_data; }
	const vector<float> getVTData() const { return all_vt_data; }
	const vector<float> getVNData() const { return all_vn_data; }
	const vector<float> getVPData() const { return all_vp_data; }

private:
	void setData();

	//uses vector<float> because # of floats per vertex varies
	//data direct from obj file, unformatted
	map<int, vector<float> > v_data;
	map<int, vector<float> > vt_data;
	map<int, vector<float> > vn_data;
	map<int, vector<float> > vp_data;

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
	//# of vertices are stored total
	int vertex_count;
	//# of faces are stored total
	int face_count;
};

#endif