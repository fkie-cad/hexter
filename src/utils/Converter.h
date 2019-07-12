#ifndef HEXTER_SRC_UTILS_CONVERTER_H
#define HEXTER_SRC_UTILS_CONVERTER_H

#include <stdint.h>

int parseUint64Auto(const char* arg, uint64_t* value);
int parseUint64(const char* arg, uint64_t* value, uint8_t base);
int parseUint32Auto(const char* arg, uint32_t* value);
int parseUint32(const char* arg, uint32_t* value, uint8_t base);
int parseUint16Auto(const char* arg, uint16_t* value);
int parseUint16(const char* arg, uint16_t* value, uint8_t base);
int parseUint8Auto(const char* arg, uint8_t* value);
int parseUint8(const char* arg, uint8_t* value, uint8_t base);

uint16_t swapUint16(uint16_t value);
uint32_t swapUint32(uint32_t value);
uint64_t swapUint64(uint64_t value);

uint8_t isHexChar(const char c);

#endif
