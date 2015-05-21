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

const vector<float> mesh_data::getInterleaveData() const
{
	vector<float> interleave_data;
	interleave_data.reserve(total_float_count);
	for (vector< vector<vertex_data> >::const_iterator faces_it = faces.begin();
		faces_it != faces.end(); faces_it++)
	{
		//object data format will be:
		//		position.x, position.y, position.z, [position.w],
		//		uv.x, [uv.y], [uv.w],
		//		normal.x, normal.y, normal.z,
		//	bracketed values are only included if they were in the original obj file

		//for each vertex in each face, pass the stored, ordered data to interleave_data
		for (vector<vertex_data>::const_iterator vertex_it = faces_it->begin();
			vertex_it != faces_it->end(); vertex_it++)
		{
			vector<float> all_face_data(vertex_it->getData());
			interleave_data.insert(interleave_data.end(), all_face_data.begin(), all_face_data.end());
		}
	}
	return interleave_data;
}

const vector<float> mesh_data::getIndexedInterleaveData(vector<unsigned int> &indices) const
{
	vector<float> interleaved_vertices = getInterleaveData();
	vector<float> unique_vertices;
	indices.clear();

	map<unsigned, vector<float> > index_map;
	
	vector<float> current_index;
	unsigned int stride_floats = interleave_stride / sizeof(float);
	current_index.reserve(stride_floats);
	unsigned int index_count = 0;
	for (int i = 0; i < interleaved_vertices.size(); i++)
	{
		current_index.push_back(interleaved_vertices.at(i));

		if (current_index.size() == stride_floats)
		{
			if (index_map.size() == 0)
			{
				unique_vertices.insert(unique_vertices.end(), current_index.begin(), current_index.end());
				index_map.insert(std::pair<unsigned int, vector<float> >(index_count, current_index));
				indices.push_back(index_count);
				index_count++;
			}

			else
			{
				bool match_found = false;
				for (auto mapped_index : index_map)
				{
					for (int j = 0; j < stride_floats; j++)
					{
						float difference = current_index.at(j) - mapped_index.second.at(j);
						if (abs(difference) > .00001f)
							break;

						if (j == stride_floats - 1)
						{
							match_found = true;
							indices.push_back(mapped_index.first);
						}
					}

					if (match_found)
						break;
				}

				if (!match_found)
				{
					unique_vertices.insert(unique_vertices.end(), current_index.begin(), current_index.end());
					index_map.insert(std::pair<unsigned int, vector<float> >(index_count, current_index));
					indices.push_back(index_count);
					index_count++;
				}
			}

			current_index.clear();
		}
	}

	return unique_vertices;
}

void mesh_data::setMeshData()
{
	if (faces.begin() != faces.end())
	{
		interleave_stride = faces.begin()->begin()->getStride();
		interleave_vt_offset = faces.begin()->begin()->getUVOffset();
		interleave_vn_offset = faces.begin()->begin()->getNOffset();

		v_size = faces.begin()->begin()->getVSize();
		vt_size = faces.begin()->begin()->getVTSize();
		vn_size = faces.begin()->begin()->getVNSize();

		total_float_count = (v_size + vt_size + vn_size) * faces.size();
	}
}

obj_contents::obj_contents(const char* obj_file)
{
	v_index_counter = 1;
	vt_index_counter = 1;
	vn_index_counter = 1;
	vp_index_counter = 1;

	fstream file;
	file.open(obj_file, std::ifstream::in);

	if (!file.is_open())
	{
		string error = "unable to open obj file: ";
		error += obj_file;
		std::cout << error << std::endl;
		error_log.push_back(error);
		return;
	}

	bool end_of_vertex_data = false;

	meshes.push_back(mesh_data());
	vector<mesh_data>::iterator current_mesh = meshes.begin();

	vector<DATA_TYPE> index_order;

	string current_material;

	while (!file.eof())
	{
		string line;
		std::getline(file, line, '\n');

		DATA_TYPE type = getDataType(line);

		if (type == UNDEFINED)
			continue;

		//"g" prefix indicates the previous geometry data has ended
		else if (type == OBJ_G)
		{
			current_mesh->setMeshName(extractName(line));
			end_of_vertex_data = true;
			continue;
		}

		if (type == OBJ_USEMTL)
		{
			current_material = extractName(line);
			current_mesh->setMaterialName(current_material);
			continue;
		}

		if (type == OBJ_MTLLIB)
		{
			mtl_filename = extractName(line);
			continue;
		}

		//detects if a new geometry is starting, resets params and operates on new mesh
		if (type == OBJ_V && end_of_vertex_data)
		{
			meshes.push_back(mesh_data());
			current_mesh = meshes.end() - 1;
			current_mesh->setMaterialName(current_material);
			end_of_vertex_data = false;
			index_order.clear();
		}

		if (type == OBJ_V || type == OBJ_VT || type == OBJ_VN || type == OBJ_VP)
		{
			if (std::find(index_order.begin(), index_order.end(), type) == index_order.end())
				index_order.push_back(type);

			vector<float> floats(extractFloats(line));
			addRawData(floats, type);
		}

		else if (type == OBJ_F)
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
					if (v_index = extracted_face_data[i][n] == 0)
						continue;

					switch (index_order[n])
					{
					case OBJ_V:
						v_index = extracted_face_data[i][n];
						position_data = raw_v_data.at(v_index);
						break;
					case OBJ_VT:
						vt_index = extracted_face_data[i][n];
						uv_data = raw_vt_data.at(vt_index);
						break;
					case OBJ_VN:
						vn_index = extracted_face_data[i][n];
						normal_data = raw_vn_data.at(vn_index);
						break;
					case OBJ_VP:
						vp_index = extracted_face_data[i][n];
						break;
					default: throw;
					}
				}

				vertex_data vert(position_data, uv_data, normal_data);
				extracted_vertices.push_back(vert);
			}

			//from extracted vertices, create 1 face for triangulated meshes,
			//separate quadrangulated meshes into 2 separate faces
			if (extracted_face_data.size() == 3)
			{
				vector<vertex_data> face;
				face.push_back(extracted_vertices[0]);
				face.push_back(extracted_vertices[1]);
				face.push_back(extracted_vertices[2]);
				current_mesh->addFace(face);

				//add data to each respective all_data vector, for retrieving individual sets
				for (vector<vertex_data>::iterator it = face.begin(); it != face.end(); it++)
				{
					current_mesh->addVData(it->getVData());
					current_mesh->addVTData(it->getVTData());
					current_mesh->addVNData(it->getVNData());
				}
			}

			else if (extracted_face_data.size() == 4)
			{
				vector<vertex_data> face1;
				face1.push_back(extracted_vertices[0]);
				face1.push_back(extracted_vertices[1]);
				face1.push_back(extracted_vertices[3]);
				current_mesh->addFace(face1);

				for (vector<vertex_data>::iterator it = face1.begin(); it != face1.end(); it++)
				{
					current_mesh->addVData(it->getVData());
					current_mesh->addVTData(it->getVTData());
					current_mesh->addVNData(it->getVNData());
				}

				vector<vertex_data> face2;
				face2.push_back(extracted_vertices[1]);
				face2.push_back(extracted_vertices[2]);
				face2.push_back(extracted_vertices[3]);
				current_mesh->addFace(face2);

				for (vector<vertex_data>::iterator it = face2.begin(); it != face2.end(); it++)
				{
					current_mesh->addVData(it->getVData());
					current_mesh->addVTData(it->getVTData());
					current_mesh->addVNData(it->getVNData());
				}
			}
		}
	}
	file.close();

	for (vector<mesh_data>::iterator it = meshes.begin(); it != meshes.end(); it++)
		it->setMeshData();
}

void obj_contents::addRawData(const vector<float> &floats, DATA_TYPE dt)
{
	switch (dt)
	{
	case OBJ_V:
		raw_v_data[v_index_counter] = floats;
		v_index_counter++;
		break;
	case OBJ_VT:
		raw_vt_data[vt_index_counter] = floats;
		vt_index_counter++;
		break;
	case OBJ_VN:
		raw_vn_data[vn_index_counter] = floats;
		vn_index_counter++;
		break;
	case OBJ_VP:
		raw_vp_data[vp_index_counter] = floats;
		vp_index_counter++;
		break;
	default: break;
	}
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

const string extractName(const string &line)
{
	string name;
	bool name_begin = false;
	for (int i = 0; i < line.size(); i++)
	{
		if (line[i] == ' ' && !name_begin)
		{
			name_begin = true;
			continue;
		}

		else if (name_begin)
			name += line[i];
	}

	return name;
}

const DATA_TYPE getDataType(const string &line)
{
	string prefix;
	for (int i = 0; i < line.size(); i++)
	{
		if (line[i] != ' ')
			prefix += line[i];

		else break;
	}

	if (prefix == "mtllib")
		return OBJ_MTLLIB;

	if (prefix == "v")
		return OBJ_V;

	if (prefix == "vt")
		return OBJ_VT;

	if (prefix == "vn")
		return OBJ_VN;

	if (prefix == "vp")
		return OBJ_VP;

	if (prefix == "f")
		return OBJ_F;

	if (prefix == "g")
		return OBJ_G;

	if (prefix == "usemtl")
		return OBJ_USEMTL;

	if (prefix == "newmtl")
		return MTL_NEWMTL;

	if (prefix == "Ka")
		return MTL_KA;

	if (prefix == "Kd")
		return MTL_KD;

	if (prefix == "Ks")
		return MTL_KS;

	if (prefix == "Ns")
		return MTL_NS;

	if (prefix == "Tr" || prefix == "d" || prefix == "Tf")
		return MTL_D;

	if (prefix == "map_Ka")
		return MTL_MAP_KA;

	if (prefix == "map_Kd")
		return MTL_MAP_KD;

	if (prefix == "map_Ks")
		return MTL_MAP_KS;

	if (prefix == "map_Ns")
		return MTL_MAP_NS;

	if (prefix == "map_d")
		return MTL_MAP_D;

	if (prefix == "map_bump" || prefix == "bump")
		return MTL_MAP_BUMP;

	if (prefix == "disp")
		return MTL_MAP_DISP;

	if (prefix == "decal")
		return MTL_DECAL;

	return UNDEFINED;
}

const vector<mesh_data> generateMeshes(const char* file_path)
{
	obj_contents contents(file_path);
	return contents.getMeshes();
}

const map<string, material_data> generateMaterials(const char* file_path)
{
	mtl_contents contents(file_path);
	return contents.getMaterials();
}

const vector<float> material_data::getData(DATA_TYPE dt) const
{
	vector<float> default_values = { 0.0f, 0.0f, 0.0f, 0.0f };
	map<DATA_TYPE, vector<float> >::const_iterator it = data.find(dt);
	if (it == data.end())
		return default_values;

	else return it->second;
}

mtl_contents::mtl_contents(const char* mtl_file)
{
	fstream file;
	file.open(mtl_file, std::ifstream::in);

	if (!file.is_open())
	{
		string error = "unable to open mtl file: ";
		error += mtl_file;
		std::cout << error << std::endl;
		error_log.push_back(error);
		return;
	}

	bool data_set = false;

	map<string, material_data>::iterator current_material;

	while (!file.eof())
	{
		string line;
		std::getline(file, line, '\n');

		DATA_TYPE type = getDataType(line);

		if (type == UNDEFINED)
			continue;

		if (type == MTL_NEWMTL)
		{
			string mtl_name = extractName(line);
			materials[mtl_name] = material_data(mtl_name);
			current_material = materials.find(mtl_name);
		}

		if (type == MTL_KD || type == MTL_KA || type == MTL_D)
		{
			vector<float> floats(extractFloats(line));
			current_material->second.setData(type, floats);
		}

		if (type == MTL_MAP_KD)
			current_material->second.setTextureFilename(extractName(line));
	}

	file.close();
}

const string mtl_contents::getTextureFilename(string material_name) const
{
	map<string, material_data>::const_iterator it = materials.find(material_name);
	if (it == materials.end())
		return "";

	else return it->second.getTextureFilename();
}