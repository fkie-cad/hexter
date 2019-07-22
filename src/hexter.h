#ifndef HEXTER_SRC_HEXTER_H
#define HEXTER_SRC_HEXTER_H

#ifdef HEXTER_EXPORTS
	#define HEXTER_API __declspec(dllexport)
#else
	#define HEXTER_API __declspec(dllimport)
#endif

#include <limits.h>
#include <stdint.h>

#include "Globals.h"

//HEXTER_API extern uint64_t file_size;
//HEXTER_API extern char file_path[PATH_MAX];
//
//HEXTER_API extern uint64_t length;
//
//HEXTER_API extern uint8_t print_col_mask;
//HEXTER_API extern uint8_t print_offset_mask;
//HEXTER_API extern uint8_t print_hex_mask;
//HEXTER_API extern uint8_t print_ascii_mask;
//
//HEXTER_API extern uint8_t clean_printing;
//
//HEXTER_API extern uint8_t find_f;
//HEXTER_API extern uint8_t continuous_f;

HEXTER_API int printFile(char* _file_name, uint64_t _start, uint64_t _length);
HEXTER_API int printProcess(uint32_t _pid, uint64_t _start, uint64_t _length, int _lpm, int _lpx, int _lph, int _lpt);

#endif
