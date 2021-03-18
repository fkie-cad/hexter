#ifndef HEXTER_SRC_UTILS_HELPER_H
#define HEXTER_SRC_UTILS_HELPER_H

#include <stdint.h>

#include "../Bool.h"

// Files => FilesUtils.h
/**
 * Expand a leading '~' in src to a full file path in dest.
 * dest has be size of PATH_MAX.
 *
 * @param src char*
 * @param dest char*
 */
void expandFilePath(const char* src, char* dest);

/**
 * Create a temporary file and store its name in buf.
 * Currently Linux only.
 *
 * @param	buf char[128]
 * @param	prefix char* name prefix of the tmp file
 * @return	int status code
 */
int getTempFile(char* buf, const char* prefix);

/**
 * Extract the base file name out of a file_path.
 * "Light" version just pointing to the file_name in the memory of file_path.
 *
 * @param file_path char*
 * @param file_name char**
 */
void getFileNameL(char* path, char** name);

/**
 * Extract the base file name out of a file_path.
 * Copying the found name into file_name.
 * Make sure, file_name char[] has a capacity of PATH_MAX!
 *
 * @param file_path char*
 * @param file_name char*
 */
void getFileName(const char* path, char* name);

/**
 * Extract the base file name out of a file_path.
 * Copying the found name into file_name allocated char*.
 * Caller is responsible for freeing it!
 *
 * @param 	file_path char*
 * @return	char* the file name
 */
char* getFileNameP(const char* path);

/**
 * Extract the base file name offset out of a file_path.
 *
 * @param file_path char*
 * @param file_name char*
 */
int32_t getFileNameOffset(const char* path);

/**
 * List all files in a directory.
 *
 * @param path char* the directory path.
 */
void listFilesOfDir(char* path);

// Numbers
uint8_t countHexWidth64(uint64_t value);
uint8_t countHexWidth32(uint32_t value);

// project specific => Helper
size_t normalizeOffset(size_t offset, uint8_t* remainder);
uint8_t getColSize();
Bool confirmContinueWithNextRegion(char* name, size_t address);

// Terminal
void setAnsiFormat(char* format);
void resetAnsiFormat();

#endif
