#ifndef PRINT_H
#define PRINT_H

// #define ERROR_PRINT

#ifdef DEBUG_PRINT
#define debug_info(...) { printf("[d] "); printf(__VA_ARGS__); }
#else
#define debug_info(...)
#endif


#ifdef INFO_PRINT
#define IPrint(...) { printf(__VA_ARGS__); }
#else
#define IPrint(...)
#endif


#ifdef ERROR_PRINT
#define EPrint(...) \
                { printf("ERROR: "); \
                printf(__VA_ARGS__); }
#else
#define EPrint(...)    
#endif


#ifdef VERBOSE_MODE
#define header_info(...) { fprintf(stdout, __VA_ARGS__); }
#define header_error(...) { fprintf(stdout, __VA_ARGS__); }
#define prog_error(...) { fprintf(stderr, __VA_ARGS__); }
#else
#define header_info(...)    
#define header_error(...)    
#define prog_error(...)    
#endif

#endif 
