#include "obj_parser.h"

void vertex_data::setVertexData()
{
	for (vector<float>::const_iterator it = v_data.begin(); it != v_data.end(); it++)
		face_data.push_back(*it);

	for (vector<float>::const_iterator it = vt_data.begin(); it != vt_data.end(); it++)
		face_data.push_back(*it);

	for (vector<float>::const_iterator it = vn_data.begin(); it != vn_data.end(); it++)
		face_data.push_back(*it);
}

void mesh_data::setMeshData()
{
	for (vector< vector<vertex_data> >::const_iterator faces_it = faces.begin();
		faces_it != faces.end(); faces_it++)
	{
		//object data format will be:
		//		position.x, position.y, position.z, [position.w],
		//		uv.x, [uv.y], [uv.w],
		//		normal.x, normal.y, normal.z,
		//	bracketed values are only included if they were in the original obj file

		//for each vertex in each face, pass the stored, ordered data to obj_data
		for (vector<vertex_data>::const_iterator vertex_it = faces_it->begin();
			vertex_it != faces_it->end(); vertex_it++)
		{
			vector<float> all_face_data(vertex_it->getData());
			interleave_data.insert(interleave_data.end(), all_face_data.begin(), all_face_data.end());
		}
	}

	if (faces.begin() != faces.end())
	{
		interleave_stride = faces.begin()->begin()->getStride();
		interleave_vt_offset = faces.begin()->begin()->getUVOffset();
		interleave_vn_offset = faces.begin()->begin()->getNOffset();

		v_count = faces.begin()->begin()->getVCount();
		vt_count = faces.begin()->begin()->getVTCount();
		vn_count = faces.begin()->begin()->getVNCount();
	}
}

obj_contents::obj_contents(const char* obj_file)
{
	fstream file;
	file.open(obj_file, std::ifstream::in);

	bool end_of_vertex_data = false;

	meshes.push_back(mesh_data());
	vector<mesh_data>::iterator current_mesh = meshes.begin();

	int v_counter = 1;
	int vt_counter = 1;
	int vn_counter = 1;
	int vp_counter = 1;

	vector<OBJ_DATA_TYPE> index_order;

	while (!file.eof())
	{
		OBJ_DATA_TYPE type = NOT_SET;

		string line;
		std::getline(file, line, '\n');

		switch (line[0])
		{
		case 'v':
			if (end_of_vertex_data)
			{		
				meshes.push_back(mesh_data());
				current_mesh = meshes.end() - 1;
				end_of_vertex_data = false;
				index_order.clear();
			}

			if (line[1] == ' ')
				type = V_DATA;

			if (line[1] == 't')
				type = VT_DATA;

			if (line[1] == 'n')
				type = VN_DATA;

			if (line[1] == 'p')
				type = VP_DATA;

			break;

		case 'f': type = F_DATA; break;
		case '#': continue;
		case '\n': continue;
		case 's': continue;
		case 'g': end_of_vertex_data = true; continue;
		default: continue;
		}

		if (type != F_DATA && type != NOT_SET)
		{
			if (std::find(index_order.begin(), index_order.end(), type) == index_order.end())
				index_order.push_back(type);

			vector<float> floats(extractFloats(line));

			switch (type)
			{
			case V_DATA:
				raw_v_data[v_counter] = floats;
				v_counter++;
				break;
			case VT_DATA:
				raw_vt_data[vt_counter] = floats;
				vt_counter++;
				break;
			case VN_DATA:
				raw_vn_data[vn_counter] = floats;
				vn_counter++;
				break;
			case VP_DATA:
				raw_vp_data[vp_counter] = floats;
				vp_counter++;
				break;
			default: break;
			}
		}

		else if (type == F_DATA)
		{
			//face_data contains the index list for each line (each face)
			//	1/1/1  2/2/2  3/3/3
			vector< vector<int> > extracted_face_data(extractFaceSequence(line));
			
			//generate vertex data objects from sequences passed
			vector<vertex_data> extracted_vertices;
			for (int i = 0; i < extracted_face_data.size(); i++)
			{
				int v_index = 0;
				int vt_index = 0;
				int vn_index = 0;
				int vp_index = 0;
				vector<float> position_data;
				vector<float> uv_data;
				vector<float> normal_data;

				for (int n = 0; n < index_order.size(); n++)
				{
					switch (index_order[n])
					{
					case V_DATA:
						v_index = extracted_face_data[i][n];
						position_data = raw_v_data.at(v_index);
						break;
					case VT_DATA:
						vt_index = extracted_face_data[i][n];
						uv_data = raw_vt_data.at(vt_index);
						break;
					case VN_DATA:
						vn_index = extracted_face_data[i][n];
						normal_data = raw_vn_data.at(vn_index);
						break;
					case VP_DATA:
						vp_index = extracted_face_data[i][n];
						break;
					default: throw;
					}
				}

				vertex_data vert(position_data, uv_data, normal_data);
				extracted_vertices.push_back(vert);

				current_mesh->addVData(position_data);
				current_mesh->addVTData(uv_data);
				current_mesh->addVNData(normal_data);
			}

			//from extracted vertices, create 1 face for triangulated meshes,
			//separate quadrangular meshes into 2 separate faces
			if (extracted_face_data.size() == 3)
			{
				vector<vertex_data> face;
				face.push_back(extracted_vertices[0]);
				face.push_back(extracted_vertices[1]);
				face.push_back(extracted_vertices[2]);
				current_mesh->addFace(face);
			}

			else if (extracted_face_data.size() == 4)
			{
				vector<vertex_data> face1;
				face1.push_back(extracted_vertices[0]);
				face1.push_back(extracted_vertices[1]);
				face1.push_back(extracted_vertices[3]);
				current_mesh->addFace(face1);

				vector<vertex_data> face2;
				face2.push_back(extracted_vertices[1]);
				face2.push_back(extracted_vertices[2]);
				face2.push_back(extracted_vertices[3]);
				current_mesh->addFace(face2);
			}
		}
	}
	file.close();

	for (vector<mesh_data>::iterator it = meshes.begin(); it != meshes.end(); it++)
		it->setMeshData();
}

const vector<float> extractFloats(const string &s)
{
	vector<float> floats;

	vector<int> digits;
	bool negative = false;
	bool decimal_found = false;
	int decimal_places = 0;
	bool values_begin = false;

	for (int i = 0; i < s.size(); i++)
	{
		if (s[i] == '-')
			negative = true;

		else if (s[i] == '.')
			decimal_found = true;

		else if (s[i] >= '0' && s[i] <= '9')
		{
			int char_int = '0';
			char_int = s[i] - char_int;
			digits.push_back(char_int);
			if (decimal_found)
				decimal_places++;
		}

		//if space found or end of string
		if ((s[i] == ' ' && values_begin) || i == s.size() - 1)
		{
			float extracted = 0;

			for (int n = 0; n < digits.size(); n++)
			{
				//example--> 470.258
				//size = 6
				//decimal_places = 3
				//first iteration of loop points to 4, which is in the
				//10^2 location. 
				//6 - 3 - 1 - 0 = 2 for the first loop
				//6 - 3 - 1 - 1 = 1 for the second loop
				int nth = digits.size() - decimal_places - 1 - n;
				float multiplier = pow(10.0f, float(nth));
				float toAdd = digits[n] * multiplier;
				extracted += toAdd;
			}
			
			if (negative)
				extracted *= -1.0f;

			floats.push_back(extracted);

			//resets counters
			digits.clear();
			negative = false;
			decimal_found = false;
			decimal_places = 0;
		}

		else if (s[i] == ' ' && !values_begin)
			values_begin = true;
	}

	return floats;
}

const vector< vector<int> > extractFaceSequence(const string &s)
{
	vector< vector<int> > index_list;

	vector<int> digits;
	vector<int> sequence;
	bool values_begin = false;

	for (int i = 0; i < s.size(); i++)
	{
		if (s[i] == '/')
		{
			if (digits.size() == 0)
			{
				sequence.push_back(0);
				continue;
			}

			float extracted = 0;

			for (int n = 0; n < digits.size(); n++)
			{
				int nth = digits.size() - 1 - n;
				float multiplier = pow(10.0f, float(nth));
				float toAdd = digits[n] * multiplier;
				extracted += toAdd;
			}

			sequence.push_back(int(extracted));

			//resets counters
			digits.clear();
		}

		else if (s[i] >= '0' && s[i] <= '9')
		{
			int char_int = '0';
			char_int = s[i] - char_int;
			digits.push_back(char_int);
		}

		//if space found or end of string
		if ((s[i] == ' ' && values_begin) || i == s.size() - 1)
		{
			float extracted = 0;

			for (int n = 0; n < digits.size(); n++)
			{
				int nth = digits.size() - 1 - n;
				float multiplier = pow(10.0f, float(nth));
				float toAdd = digits[n] * multiplier;
				extracted += toAdd;
			}

			sequence.push_back(int(extracted));
			index_list.push_back(sequence);

			//resets counters
			sequence.clear();
			digits.clear();
		}

		else if (s[i] == ' ' && !values_begin)
			values_begin = true;
	}

	return index_list;
}

const vector<mesh_data> generateMeshes(const char* file_path)
{
	obj_contents contents(file_path);
	return contents.getMeshes();
}