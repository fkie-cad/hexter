#ifndef HEXTER_TESTS_MISC_MISC_H
#define HEXTER_TESTS_MISC_MISC_H

#include <stdint.h>

#include <random>
#include <string>
#include <vector>

class Misc
{
	public:
		std::mt19937_64* gen;
		std::uniform_int_distribution<uint8_t>* dis;

		Misc();
		~Misc();

		std::vector<uint8_t> createBinary(const std::string& file_src, size_t f_size);
		void createBinary(const std::string& file_src, const std::vector<uint8_t>& bytes);
};

#endif
