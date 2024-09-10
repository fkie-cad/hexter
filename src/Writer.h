#ifndef HEXTER_SRC_WRITER_H
#define HEXTER_SRC_WRITER_H

uint32_t payloadParseByte(
    const char* arg, 
    uint8_t** payload
);

uint32_t payloadParseFillBytes(
    const char* arg, 
    uint8_t** payload, 
    size_t length
);

uint32_t payloadParseWord(
    const char* arg, 
    uint8_t** payload
);

uint32_t payloadParseDWord(
    const char* arg, 
    uint8_t** payload
);

uint32_t payloadParseQWord(
    const char* arg, 
    uint8_t** payload
);

uint32_t payloadParseUtf8(
    const char* arg, 
    uint8_t** payload
);

uint32_t payloadParseUtf16(
    const char* arg, 
    uint8_t** payload
);

uint32_t payloadParseReversedPlainBytes(
    const char* arg, 
    uint8_t** payload
);

int cleanBytes(
    const char* input, 
    char** output
);

uint32_t payloadParsePlainBytes(
    const char* arg, 
    uint8_t** payload
);



void insert(
    const char* file_path, 
    uint8_t* payload, 
    uint32_t payload_ln, 
    size_t offset
);

void overwrite(
    const char* file_path, 
    uint8_t* payload, 
    uint32_t payload_ln, 
    size_t offset
);

void deleteBytes(
    const char* file_path, 
    size_t start, 
    size_t length
);

#endif
