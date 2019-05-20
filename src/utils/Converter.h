#ifndef HEXTER_SRC_UTILS_CONVERTER_H
#define HEXTER_SRC_UTILS_CONVERTER_H

#include <stdint.h>

int parseUint64(const char* arg, uint64_t* value);
int parseUint8(const char* arg, uint8_t* value);
uint8_t isHexChar(char c);

#endif
