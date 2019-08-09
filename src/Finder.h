#ifndef HEXTER_SRC_FINDER_H
#define HEXTER_SRC_FINDER_H

void Finder_initFailure(unsigned char* needle, uint32_t needle_ln);
uint64_t find(const char* file_path, const unsigned char* needle, uint32_t needle_ln, uint64_t offset, uint64_t max_offset);
uint64_t findNeedleInFile(const char* file_path, const unsigned char* needle, uint32_t needle_ln, uint64_t offset, uint64_t max_offset);
uint64_t findNeedleInFP(const unsigned char* needle, uint32_t needle_ln, uint64_t offset, FILE* fi, uint64_t max_offset);
uint64_t findNeedleInBlock(const unsigned char* needle, uint32_t needle_ln, const unsigned char* buf, uint64_t* j, size_t n);
void computeFailure(const unsigned char* pattern, uint32_t pattern_ln, uint16_t* failure);
void Finder_cleanUp();

#endif
