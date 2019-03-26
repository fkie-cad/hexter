#ifndef COMMON_FILEIO_H
#define COMMON_FILEIO_H

#include <stdint.h>

uint64_t getSize(const char* finame);
uint64_t readBlock(const char* finame, uint64_t begin);
uint64_t readLargeBlock(const char* finame, uint64_t begin);
uint64_t readCharArrayFile(const char* finame, unsigned char ** pData, uint64_t begin, uint64_t stopAt);
uint64_t readCharArrayFileNA(const char* finame, unsigned char* data, uint64_t data_size, uint64_t begin, uint64_t stopAt);
uint64_t cfio_sanitizeFileSize(const char* finame, uint64_t file_size, uint64_t begin, uint64_t stopAt);
uint64_t cfio_readFile(const char* finame, uint64_t begin, uint64_t size, unsigned char* data);
uint64_t readFile(FILE* fi, uint64_t begin, uint64_t size, unsigned char* data);

#endif