#ifndef HEXTER_SRC_UTILS_CONVERTER_H
#define HEXTER_SRC_UTILS_CONVERTER_H

#include <stdint.h>

uint64_t parseUint64(const char* arg);
uint8_t parseUint8(const char* arg);
uint8_t isHexChar(char c);

#endif
