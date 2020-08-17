#ifndef HEXTER_SRC_GLOBALS_H
#define HEXTER_SRC_GLOBALS_H

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <limits.h>

#include "Bool.h"

#if defined(_WIN32)
	#define ENTER (0xd) // aka \r
#else
	#define ENTER (0xa) // aka \n
#endif

#define DEBUG_PRINT_INFO (0)
#define debug_info(...) if (DEBUG_PRINT_INFO) fprintf(stdout, __VA_ARGS__)

#ifndef PATH_MAX
	#define PATH_MAX _MAX_PATH
#endif

#define BLOCKSIZE (0x200)
#define BLOCKSIZE_LARGE (0x400)

#define DEFAULT_LENGTH (0x100)
//#define DEFAULT_ASCII_LENGTH (0x80)

#define TRIPLE_COL_SIZE (0x10)
#define DOUBLE_COL_SIZE (0x10)
#define ASCII_COL_SIZE (0x40)
#define HEX_COL_SIZE (0x10)

#define MIN_PRINTABLE_ASCII_RANGE (0x20)
#define MAX_PRINTABLE_ASCII_RANGE (0x7E)

#define COL_SEPARATOR ('|')
#define NO_PRINT_ASCII_SUBSTITUTION ('.')

#define NEXT ('n')
#define QUIT ('q')
#define CONTINUE ('c')

#define MAX_PAYLOAD_LN (512)
#define FIND_FAILURE SIZE_MAX

extern size_t file_size;
extern char file_path[PATH_MAX];

extern size_t length;

extern uint8_t print_col_mask;
extern uint8_t print_offset_mask;
extern uint8_t print_hex_mask;
extern uint8_t print_ascii_mask;

extern uint8_t clean_printing;

extern uint8_t find_f;
extern uint8_t continuous_f;

#endif
