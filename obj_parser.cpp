#include "obj_parser.h"

void vertex_data::setData()
{
	for (vector<float>::const_iterator it = v_data.begin(); it != v_data.end(); it++)
		face_data.push_back(*it);

	for (vector<float>::const_iterator it = vt_data.begin(); it != vt_data.end(); it++)
		face_data.push_back(*it);

	for (vector<float>::const_iterator it = vn_data.begin(); it != vn_data.end(); it++)
		face_data.push_back(*it);
}

obj_contents::obj_contents(const char* obj_file)
{
	fstream file;
	file.open(obj_file, std::ifstream::in);

	int position_counter = 1;
	int uv_counter = 1;
	int normal_counter = 1;
	int paramspace_counter = 1;
	vertex_count = 0;
	face_count = 0;

	while (!file.eof())
	{
		OBJ_DATA_TYPE type = NOT_SET;

		string line;
		std::getline(file, line, '\n');

		switch (line[0])
		{
		case 'v':
			if (line[1] == ' ')
				type = POSITION;

			if (line[1] == 't')
				type = UV;

			if (line[1] == 'n')
				type = NORMAL;

			if (line[1] == 'p')
				type = PARAM_SPACE;

			break;

		case 'f': type = FACE_DEF; break;
		case '#': continue;
		case '\n': continue;
		default: break;
		}

		if (type != FACE_DEF && type != NOT_SET)
		{
			vector<float> floats(extractFloats(line));

			switch (type)
			{
			case POSITION:
				v_data[position_counter] = floats;
				position_counter++;
				break;
			case UV:
				vt_data[uv_counter] = floats;
				uv_counter++;
				break;
			case NORMAL:
				vn_data[normal_counter] = floats;
				normal_counter++;
				break;
			case PARAM_SPACE:
				vp_data[paramspace_counter] = floats;
				paramspace_counter++;
				break;
			default: break;
			}
		}

		else if (type == FACE_DEF)
		{
			//face_data contains the index list for each line (each face)
			//	1/1/1  2/2/2  3/3/3
			vector< vector<int> > extracted_face_data(extractFaceSequence(line));
		
			vector<vertex_data> face;
			//generates positions, uv coordinates, and normals for the line (each sequence line = 1 face)
			for (vector< vector<int> >::iterator it = extracted_face_data.begin(); it != extracted_face_data.end(); it++)
			{
				int v_index = (*it)[0];
				int vt_index = (*it)[1];
				int vn_index = (*it)[2];
				vector<float> position_data = v_data.at(v_index);
				vector<float> uv_data = vt_data.at(vt_index);
				vector<float> normal_data = vn_data.at(vn_index);

				vertex_data vert(position_data, uv_data, normal_data);
				vertex_count++;
				face.push_back(vert);

				all_v_data.insert(all_v_data.end(), position_data.begin(), position_data.end());
				all_vt_data.insert(all_vt_data.end(), uv_data.begin(), uv_data.end());
				all_vn_data.insert(all_vn_data.end(), normal_data.begin(), normal_data.end());
			}
			faces.push_back(face);
			face_count++;
		}
	}

	setData();
	file.close();
}

void obj_contents::setData()
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