#ifndef HEXTER_SRC_GLOBALS_H
#define HEXTER_SRC_GLOBALS_H

#include <stdint.h>
#include <stddef.h>
#include <limits.h>

#define DEBUG_PRINT_INFO 0
#define debug_info(...) if (DEBUG_PRINT_INFO) fprintf(stdout, __VA_ARGS__)

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define BLOCKSIZE 0x200
#define BLOCKSIZE_LARGE 0x400

#define DEFAULT_LENGTH 0x50

#define DOUBLE_COL_SIZE 0x10
#define ASCII_COL_SIZE 0x40
#define HEX_COL_SIZE 0x10

#define MIN_PRINT_ASCII_RANGE 0x20
#define MAX_PRINT_ASCII_RANGE 0x80

#define COL_SEPARATOR '|'
#define NO_PRINT_ASCII_SUBSTITUTION '.'

uint64_t file_size;
char file_name[PATH_MAX];

uint64_t start;
uint64_t length;
uint8_t ascii_only;
uint8_t hex_only;

#endif
