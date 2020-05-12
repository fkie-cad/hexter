#ifndef COMMON_FILEIO_H
#define COMMON_FILEIO_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "../env.h"

#if defined(Win64) || defined(_WIN64)
    #define fseek(f, o, t) _fseeki64(f, o, t)
#endif
//#define _chsize(fp, s) _chsize64(fp, s)

size_t getSize(const char* finame);
//size_t readBlock(const char* finame, size_t begin);
//size_t readLargeBlock(const char* finame, size_t begin);
//size_t readCharArrayFile(const char* finame, unsigned char ** pData, size_t begin, size_t stopAt);
//size_t readCharArrayFileNA(const char* finame, unsigned char* data, size_t data_size, size_t begin, size_t stopAt);
//size_t cfio_sanitizeFileSize(const char* finame, size_t file_size, size_t begin, size_t stopAt);
//size_t cfio_readFile(const char* finame, size_t begin, size_t size, unsigned char* data);
size_t readFile(FILE* fi, size_t begin, size_t size, unsigned char* data);
int cfio_getErrno();

#endif
