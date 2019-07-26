#ifndef HEXTER_SRC_WRITER_H
#define HEXTER_SRC_WRITER_H

uint32_t payloadParseByte(const char* arg, unsigned char** payload);
uint32_t payloadParseWord(const char* arg, unsigned char** payload);
uint32_t payloadParseDWord(const char* arg, unsigned char** payload);
uint32_t payloadParseQWord(const char* arg, unsigned char** payload);
uint32_t payloadParseString(const char* arg, unsigned char** payload);
uint32_t payloadParseReversedPlainBytes(const char* arg, unsigned char** payload);
uint32_t payloadParsePlainBytes(const char* arg, unsigned char** payload);

void insert(char* file_path, unsigned char* payload, uint32_t payload_ln, uint64_t offset);
void overwrite(char* file_path, unsigned char* payload, uint32_t payload_ln, uint64_t offset);
void deleteBytes(char* file_path, uint64_t start, uint64_t length);

#endif
