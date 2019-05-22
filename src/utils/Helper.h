#ifndef HEXTER_SRC_UTILS_HELPER_H
#define HEXTER_SRC_UTILS_HELPER_H

void expandFilePath(char* src, char* dest);
int getTempFile(char* buf, char* prefix);
uint8_t countHexWidth64(uint64_t value);

#endif
