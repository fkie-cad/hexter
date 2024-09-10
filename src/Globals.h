#ifndef HEXTER_SRC_GLOBALS_H
#define HEXTER_SRC_GLOBALS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <limits.h>

#include "Bool.h"

#define IS_LINUX (defined(__linux__) || defined(__linux) || defined(linux)) ? 1 : 0

#if defined(_WIN32)
    #define ENTER (0xd) // aka \r
#else
    #define ENTER (0xa) // aka \n
#endif


#if defined(_WIN32)
    #define getpid _getpid
#endif

//#define DEBUG_PRINT_INFO (0)

#include "print.h"


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
#define UNICODE_COL_SIZE (0x80)
#define HEX_COL_SIZE (0x10)

#define MIN_PRINTABLE_ASCII_RANGE (0x20)
#define MAX_PRINTABLE_ASCII_RANGE (0x7E)

#define COL_SEPARATOR ('|')
#define NO_PRINT_ASCII_SUBSTITUTION ('.')
#define NO_PRINT_UC_SUBSTITUTION ('.')

#define NEXT ('n')
#define QUIT ('q')
#define CONTINUE ('c')

#define MAX_PAYLOAD_LN (0xFFFFFFFF)
#define FIND_FAILURE SIZE_MAX

extern size_t file_size;
extern char file_path[PATH_MAX];

extern size_t length;

extern uint8_t print_col_mask;
#define PRINT_ASCII_MASK    (0x1)
#define PRINT_UNICODE_MASK  (0x2)
#define PRINT_HEX_MASK      (0x4)
#define PRINT_OFFSET_MASK   (0x8)
#define PRINT_BYTES_STRING  (0x10)


#define MODE_FLAG_INSERT                (0x01)
#define MODE_FLAG_OVERWRITE             (0x02)
#define MODE_FLAG_DELETE                (0x04)
#define MODE_FLAG_FIND                  (0x08)
#define MODE_FLAG_CONTINUOUS_PRINTING   (0x10)
#define MODE_FLAG_CLEAN_PRINTING        (0x20)
#define MODE_FLAG_CASE_INSENSITIVE      (0x40)

extern uint32_t mode_flags;

#endif
