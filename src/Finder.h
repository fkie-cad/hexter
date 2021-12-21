#ifndef HEXTER_SRC_FINDER_H
#define HEXTER_SRC_FINDER_H

void Finder_initFailure(
    uint8_t* needle, 
    uint32_t needle_ln
);

size_t find(
    const char* 
    file_path, 
    const uint8_t* needle, 
    uint32_t needle_ln, 
    size_t offset, 
    size_t max_offset
);

size_t findNeedleInFile(
    const char* file_path, 
    const uint8_t* needle, 
    uint32_t needle_ln, 
    size_t offset, 
    size_t max_offset
);

size_t findNeedleInFP(
    const uint8_t* needle, 
    uint32_t needle_ln, 
    size_t offset, 
    FILE* fi, 
    size_t max_offset
);

size_t findNeedleInBlock(
    const uint8_t* needle, 
    uint32_t needle_ln, 
    const uint8_t* buf, 
    size_t* j, 
    size_t n
);

void computeFailure(
    const uint8_t* pattern, 
    size_t pattern_ln, 
    uint16_t* failure
);

void Finder_cleanUp();

#endif
