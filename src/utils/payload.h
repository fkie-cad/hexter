#pragma once


int cleanBytes(const char* input, char** output);

uint32_t payloadParseByte(const char* arg, uint8_t** payload);

uint32_t payloadParseFillBytes(const char* arg, uint8_t** payload, size_t ln);

uint32_t payloadParseWord(const char* arg, uint8_t** payload);

uint32_t payloadParseDWord(const char* arg, uint8_t** payload);

uint32_t payloadParseQWord(const char* arg, uint8_t** payload);

uint32_t payloadParseUtf8(const char* arg, uint8_t** payload);

uint32_t payloadParseUtf16(const char* arg, uint8_t** payload, size_t max_payload_ln);

uint32_t payloadParseReversedPlainBytes(const char* arg, uint8_t** payload);

uint32_t payloadParsePlainBytes(const char* arg, uint8_t** payload);
