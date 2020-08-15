#ifndef COMMON_FILEIO_H
#define COMMON_FILEIO_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(Win64) || defined(_WIN64)
    #define fseek(f, o, t) _fseeki64(f, o, t)
    #define ftell(s) _ftelli64(s)
#endif
//#define _chsize(fp, s) _chsize64(fp, s)

size_t getSize(const char* finame);
size_t readFile(FILE* fi, size_t begin, size_t size, unsigned char* data);
int cfio_getErrno();

#endif
