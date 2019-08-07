#ifndef HEXTER_SRC_UTILS_HELPER_H
#define HEXTER_SRC_UTILS_HELPER_H

#include <stdint.h>

#include "../Bool.h"

// Files
void expandFilePath(const char* src, char* dest);
int getTempFile(char* buf, const char* prefix);
void getFileNameL(char* file_path, char** file_name);
void getFileName(const char* file_path, char* file_name);
char* getFileNameP(const char* file_path);
int64_t getFileNameOffset(const char* file_path);
void listFilesOfDir(char* path);

// Numbers
uint8_t countHexWidth64(uint64_t value);

// project specific => Helper
uint64_t normalizeOffset(uint64_t offset, uint8_t* remainder);
uint8_t getColSize();
Bool confirmContinueWithNextRegion(char* name, uint64_t address);

// Terminal
void setAnsiFormat(char* format);
void resetAnsiFormat();

#endif
