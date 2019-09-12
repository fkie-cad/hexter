#ifndef HEXTER_SRC_FINDER_H
#define HEXTER_SRC_FINDER_H

void Finder_initFailure(unsigned char* needle, uint32_t needle_ln);
size_t find(const char* file_path, const unsigned char* needle, uint32_t needle_ln, size_t offset, size_t max_offset);
size_t findNeedleInFile(const char* file_path, const unsigned char* needle, uint32_t needle_ln, size_t offset, size_t max_offset);
size_t findNeedleInFP(const unsigned char* needle, uint32_t needle_ln, size_t offset, FILE* fi, size_t max_offset);
size_t findNeedleInBlock(const unsigned char* needle, uint32_t needle_ln, const unsigned char* buf, size_t* j, size_t n);
void computeFailure(const unsigned char* pattern, size_t pattern_ln, uint16_t* failure);
void Finder_cleanUp();

#endif
