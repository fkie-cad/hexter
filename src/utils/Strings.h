#ifndef HEXTER_SRC_UTILS_STRINGS_H
#define HEXTER_SRC_UTILS_STRINGS_H

#include <stdio.h>

size_t split(char* str, const char* delimiter, char** bucket, const size_t bucket_max);
size_t splitArgs(char *buffer, char *argv[], size_t argv_size);
size_t splitArgsCSM(char *buffer, char *argv[], size_t argv_size, char som, char scm);

#endif