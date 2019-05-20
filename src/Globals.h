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

#define MIN_PRINTABLE_ASCII_RANGE 0x20
#define MAX_PRINTABLE_ASCII_RANGE 0x7E

#define COL_SEPARATOR '|'
#define NO_PRINT_ASCII_SUBSTITUTION '.'

#define MAX_PAYLOAD_LN 512

extern uint64_t file_size;
extern char file_name[PATH_MAX];

extern uint64_t start;
extern uint64_t length;
extern uint8_t ascii_only;
extern uint8_t hex_only;
extern uint8_t clean_printing;

extern uint8_t insert_f;
extern uint8_t overwrite_f;
extern uint8_t find_f;

#endif
