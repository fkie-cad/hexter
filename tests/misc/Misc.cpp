#include <fstream>

#include "Misc.h"

using namespace std;

Misc::Misc()
{
	random_device rd;
	gen = new mt19937_64(rd());
	dis = new uniform_int_distribution<uint8_t>(0, UINT8_MAX);
}

Misc::~Misc()
{
	delete(gen);
	delete(dis);
}

vector<uint8_t> Misc::createBinary(const string& file_src, size_t f_size)
{
	ofstream f;
	vector<uint8_t> values;
	values.resize(f_size);

	f.open(file_src, ios::binary | std::ios::out);
	f.clear();

	for ( size_t i = 0; i < f_size; i++ )
	{
		uint8_t value = (*dis)(*gen);
		uint8_t size = sizeof(value);
		f.write(reinterpret_cast<char *>(&(value)), size);
		values[i] = value;
	}

	f.close();

	return values;
}
