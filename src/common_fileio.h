#ifndef COMMON_FILEIO_H
#define COMMON_FILEIO_H

#include <stdint.h>

// Contains : lowlevel fileio support.

uint64_t getSize(const char* finame);
uint64_t readBlock(const char* finame, uint64_t begin);
uint64_t readLargeBlock(const char* finame, uint64_t begin);
uint64_t readCharArrayFile(const char* finame, unsigned char ** pData, uint64_t begin, uint64_t stopAt);

#endif