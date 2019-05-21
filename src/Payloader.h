#ifndef HEXTER_SRC_PAYLOADER_H
#define HEXTER_SRC_PAYLOADER_H

uint32_t payloadParseByte(const char* arg, unsigned char** payload);
uint32_t payloadParseWord(const char* arg, unsigned char** payload);
uint32_t payloadParseDWord(const char* arg, unsigned char** payload);
uint32_t payloadParseQWord(const char* arg, unsigned char** payload);
uint32_t payloadParseString(const char* arg, unsigned char** payload);
uint32_t payloadParseReversedPlainBytes(const char* arg, unsigned char** payload);
uint32_t payloadParsePlainBytes(const char* arg, unsigned char** payload);

void insert(unsigned char* payload, uint32_t payload_ln);
void overwrite(unsigned char* payload, uint32_t payload_ln);

#endif
