#ifndef HEXTER_SRC_UTILS_STRINGS_H
#define HEXTER_SRC_UTILS_STRINGS_H

#include <stdio.h>

size_t split(char* str, const char* delimiter, char** bucket, const size_t bucket_max);
size_t splitArgs(char *buffer, char *argv[], size_t argv_size);
size_t splitArgsCSM(char *buffer, char *argv[], size_t argv_size, char som, char scm);


int UTF8ToUTF16LE(unsigned char* outb, size_t* outlen, const unsigned char* in, size_t* inlen);
int UTF16LEToUTF8(unsigned char* out, size_t* outlen, const unsigned char* inb, size_t* inlenb);

#endif