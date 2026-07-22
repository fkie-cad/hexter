#ifndef COMMON_FILEIO_H
#define COMMON_FILEIO_H

//#include <stdint.h>
#include <stdio.h>

size_t getSize(const char* finame);
size_t readFile(FILE* fi, size_t begin, size_t size, unsigned char* data, int* errsv);

#endif
