#ifndef HEXTER_SRC_UTILS_HELPER_H
#define HEXTER_SRC_UTILS_HELPER_H

#include "../bool.h"

void expandFilePath(const char* src, char* dest);
int getTempFile(char* buf, const char* prefix);
void getFileNameL(char* file_path, char** file_name);
void getFileName(const char* file_path, char* file_name);
char* getFileNameP(const char* file_path);
int64_t getFileNameOffset(const char* file_path);
uint8_t countHexWidth64(uint64_t value);
uint64_t normalizeOffset(uint64_t offset, uint8_t* remainder);
uint8_t getColSize();
size_t split(char* str, const char* delimiter, char** bucket, const size_t bucket_max);
size_t splitArgs(char *buffer, char *argv[], size_t argv_size);
bool confirmContinueWithNextRegion(char* name, uint64_t address);
void setAnsiFormat(char* format);
void resetAnsiFormat();

#endif
