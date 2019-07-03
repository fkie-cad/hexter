#ifndef HEXTER_SRC_FINDER_H
#define HEXTER_SRC_FINDER_H

uint64_t find(const unsigned char* needle, uint32_t needle_ln, uint64_t offset);
uint64_t findNeedle(const unsigned char* needle, uint32_t needle_ln, uint64_t offset, uint16_t* failure);
uint64_t findNeedleInFP(const unsigned char* needle, uint32_t needle_ln, uint64_t offset, const uint16_t* failure, FILE* fi);
uint64_t findNeedleInBlock(const unsigned char* needle, uint32_t needle_ln, const unsigned char* buf, uint64_t* j, const uint16_t* failure, size_t n);
void computeFailure(const unsigned char* pattern, uint64_t pattern_ln, uint16_t* failure);

#endif
