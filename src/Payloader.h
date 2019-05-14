#ifndef HEXTER_SRC_PAYLOADER_H
#define HEXTER_SRC_PAYLOADER_H


unsigned char* payloadParseByte(const char* arg);
unsigned char* payloadParseWord(const char* arg);
unsigned char* payloadParseDoubleWord(const char* arg);
unsigned char* payloadParseQuadWord(const char* arg);
unsigned char* payloadParseString(const char* arg);
unsigned char* payloadParseReversedPlainBytes(const char* arg);
unsigned char* payloadParsePlainBytes(const char* arg);

void insert();
void overwrite();

#endif
