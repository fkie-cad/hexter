#ifndef HEXTER_SRC_FINDER_H
#define HEXTER_SRC_FINDER_H

uint64_t find(const unsigned char* needle, uint32_t needle_ln);
void computeFailure(const unsigned char* pattern, uint64_t pattern_ln, uint16_t* failure);

#endif
