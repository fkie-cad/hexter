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

#define MAX_COL_SIZE (BLOCK_SIZE)

#define DEFAULT_LENGTH (0x100)
//#define DEFAULT_ASCII_LENGTH (0x80)

#define HEX_COL_SIZE (0x10)
#define ASCII_HEX_COL_SIZE (0x10)
#define ASCII_COL_SIZE (0x40)
#define UNICODE_COL_SIZE (0x80)
#define BYTE_STRING_COL_SIZE (0x10)

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

extern size_t g_length;

//extern uint8_t print_col_mask;
//#define PRINT_ASCII_MASK     (0x1)
//#define PRINT_UNICODE_MASK   (0x2)
//#define PRINT_HEX_MASK       (0x4)
//#define PRINT_OFFSET_MASK    (0x8)
//#define PRINT_BYTES_STRING  (0x10)

#define COL_MASK_OFFSET (0x1)
#define COL_MASK_HEX (0x2)
#define COL_MASK_ASCII (0x4)
#define COL_MASK_UNICODE (0x8)
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

extern uint32_t mode_flags;

extern uint32_t g_col_mask;
extern uint32_t g_hex_size;

//typedef struct print_flags {
//    uint8_t col_mask;
//    uint8_t hex_size;
//    uint8_t main_col_size;
//    uint32_t hex_col_size;
//    uint32_t ascii_col_size;
//    uint32_t unicode_col_size;
//    uint32_t line_size;
//};

typedef struct _col_sizes {
    uint32_t custom;
    uint8_t main;
    uint32_t hex;
    uint32_t ascii;
    uint32_t unicode;
    uint32_t line;
} col_sizes;
extern col_sizes g_col_sizes;


#ifndef ALIGN_UP_BY
#define ALIGN_UP_BY(__value__, __align__) ( ((uint64_t)(__value__) + (__align__) - 1) & ~((uint64_t)(__align__) - 1) )
#endif

#ifndef ALIGN_DOWN_BY
#define ALIGN_DOWN_BY(__value__, __align__) ((uint64_t)(__value__) & ~((uint64_t)(__align__) - 1))
#endif

#define ARE_FLAGS_SET(__value__, __flags__) ((__value__)&(__flags__)) == (__flags__) 
#define IS_FLAG_SET(__value__, __flags__) ((__value__)&(__flags__))

#endif
