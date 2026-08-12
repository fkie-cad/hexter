#ifndef HEXTER_SRC_GLOBALS_H
#define HEXTER_SRC_GLOBALS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <limits.h>

#include "Bool.h"

#include "utils/env.h"



#ifdef _WIN32
    #define ENTER (0xd) // aka \r
#else
    #define ENTER (0xa) // aka \n
#endif

#ifdef _WIN32
// warning C4996: '_getpid': The POSIX name for this item is deprecated. Instead, use the ISO C and C++ conformant name: _getpid.
    #define getpid _getpid
#endif

#if defined(Win64) || defined(_WIN64)
#define fseek(f, o, t) _fseeki64(f, o, t)
#define ftell(s) _ftelli64(s)
#endif

#ifdef _LINUX
#include <sys/param.h>
#define max MAX
#endif

//#define DEBUG_PRINT_INFO (0)

#include "print.h"


#ifndef PATH_MAX
    #define PATH_MAX _MAX_PATH
#endif

//#define BLOCK_SIZE (0x40)
#define BLOCK_SIZE (0x400)

// max col size bound to block size
// for bigger size printing gets more complicated
#define MAX_COL_SIZE (BLOCK_SIZE)

#define DEFAULT_LENGTH (0x100)

#define HEX_COL_SIZE (0x10)
#define ASCII_HEX_COL_SIZE (0x10)
#define ASCII_COL_SIZE (0x40)
//#define UNICODE_HEX_COL_SIZE (0x10)
//#define UNICODE_COL_SIZE (0x80)
#define BYTE_STRING_COL_SIZE (0x1)

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

#define COL_MASK_OFFSET (0x1)
#define COL_MASK_HEX (0x2)
#define COL_MASK_ASCII (0x4)
//#define COL_MASK_UNICODE (0x8)
#define COL_MASK_BYTE_STRING (0x10)

#define MODE_FLAG_INSERT                 (0x01)
#define MODE_FLAG_OVERWRITE              (0x02)
#define MODE_FLAG_DELETE                 (0x04)
#define MODE_FLAG_FIND                   (0x08)
#define MODE_FLAG_FIND_ALL               (0x10)
#define MODE_FLAG_CONTINUOUS_PRINTING    (0x20)
#define MODE_FLAG_CLEAN_PRINTING         (0x40)
#define MODE_FLAG_CASE_INSENSITIVE       (0x80)
#define MODE_FLAG_PRINT_START_OFFSET    (0x100)

#define FIND_FLAG_CASE_INSENSITIVE (0x1)
#define FIND_FLAG_ASCII            (0x2)
#define FIND_FLAG_UNICODE          (0x4)


typedef struct _FILE_INFO {
    size_t size;
    char path[PATH_MAX];
} FILE_INFO;
//extern FILE_INFO g_file_info;

typedef struct _PRINT_CFG {
    size_t block_length;
    size_t start;
    size_t end;
    uint32_t skip;
    uint32_t mode;
    uint32_t cols;
    uint32_t value_size;
} PRINT_CFG;
extern PRINT_CFG g_print_flags;

typedef struct _COL_SIZES {
    uint32_t custom;
    uint8_t main;
    uint32_t hex;
    uint32_t ascii;
    //uint32_t unicode;
    uint32_t line;
} COL_SIZES;
extern COL_SIZES g_col_sizes;

typedef struct _FIND_CFG {
    uint32_t max_count; // 00 max number of found entries before break
    uint32_t flags; // 04 mode flags for finding
    uint8_t* needle; // 08 needle to find
    uint32_t needle_ln; // 10 needle cb
} FIND_CFG; // 14
//extern FIND_CFG g_find_cfg;


#ifndef ALIGN_UP_BY
#define ALIGN_UP_BY(__value__, __align__) ( ((uint64_t)(__value__) + (__align__) - 1) & ~((uint64_t)(__align__) - 1) )
#endif

#ifndef ALIGN_DOWN_BY
#define ALIGN_DOWN_BY(__value__, __align__) ((uint64_t)(__value__) & ~((uint64_t)(__align__) - 1))
#endif

#define ARE_FLAGS_SET(__value__, __flags__) (((__value__)&(__flags__)) == (__flags__))
#define IS_FLAG_SET(__value__, __flags__) ((__value__)&(__flags__))
#define IS_ONLY_ONE_FLAG_SET(__value__, __flags__, __expected__) (((__value__)&(__flags__))==(__expected__))

#endif
