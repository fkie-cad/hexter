#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_WARNINGS
#endif

#include "utils/env.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#if defined(_LINUX)
    #include <unistd.h>
#endif

#define HEXTER_EXPORTS
#define INFO_PRINT

// for usage in Diller
//#define DILLER

#include "hexter.h"
#include "Globals.h"
#include "utils/common_fileio.h"
#include "Printer.h"
#include "utils/payload.h"
#include "Writer.h"
#include "utils/Converter.h"
#include "utils/Helper.h"
#if defined(_LINUX)
    #include "ProcessHandlerLinux.h"
#elif defined(_WIN32)
    #include <process.h>
    #include <time.h>
    #include <io.h>
    #include "utils/win/processes.h"
    #include "ProcessHandlerWin.h"
#endif
#include "utils/Strings.h"

#define BIN_NAME "hexter"
#define BIN_VS "1.11.4"
#define BIN_LAST_CHANGED "12.08.2026"

#define LIN_PARAM_IDENTIFIER ('-')
#define WIN_PARAM_IDENTIFIER ('/')


FILE_INFO g_file_info;

static uint32_t g_process_list_flags;

PRINT_CFG g_print_flags;
COL_SIZES g_col_sizes;
//FIND_CFG g_find_cfg;

typedef enum RunMode { RUN_MODE_NONE, RUN_MODE_FILE, RUN_MODE_PID } RunMode;
static RunMode run_mode;

static int payload_arg_id;


#define FORMAT_ASCII ('a')
#define FORMAT_UNICODE ('u')
#define FORMAT_BYTE ('b')
#define FORMAT_WORD ('w')
#define FORMAT_D_WORD ('d')
#define FORMAT_Q_WORD ('q')
#define FORMAT_PLAIN_HEX_1 ('h')
#define FORMAT_PLAIN_HEX_2 ('x')
#define FORMAT_FILL_BYTE ('f')

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) \
  ( sizeof(a) / sizeof(*(a)) )
#endif

static const char format_types[] = { 
    FORMAT_ASCII, FORMAT_UNICODE, FORMAT_BYTE, FORMAT_FILL_BYTE, FORMAT_WORD, FORMAT_D_WORD, FORMAT_Q_WORD, FORMAT_PLAIN_HEX_1, FORMAT_PLAIN_HEX_2 
};
#define format_types_ln (ARRAY_SIZE(format_types))

static void printVersion();
static void printUsage();
static void printHelp();
static void initParameters();
static int parseArgs(int argc, char** argv, FIND_CFG *find_cfg);
static uint8_t isArgOfType(const char* arg, const char* type);
static uint8_t isCallForHelp(const char* arg1);
static uint8_t isCheckForVersion(const char* arg1);
static uint8_t isFormatArgOfType(char* arg, char* type);
static uint8_t hasValue(char* type, int i, int end_i);
static int sanitizeDeleteParams();
static int sanitizePrintParams(uint32_t pid, PRINT_CFG *print_flags, COL_SIZES *col_sizes, FIND_CFG *find_cfg);
static uint32_t parsePayload(const char format, const char* value, uint8_t** payload);

static int run(const char payload_format, const char* raw_payload, FIND_CFG *find_cfg);
static void cleanUp(uint8_t* payload);

static uint8_t keepLengthPrintingRange(size_t start, size_t end, size_t* length);
//static uint8_t keepLengthInFile(size_t start, size_t size, size_t* length);

#ifdef DILLER
HEXTER_API
#endif
int
#ifdef _WIN32
__cdecl
#endif
main(int argc, char** argv)
{
    int s;

    FIND_CFG find_cfg = { 0 };

    if ( argc < 2 )
    {
        printUsage();
        return -1;
    }

    if ( isCallForHelp(argv[1]) )
    {
        printHelp();
        return 0;
    }

    if ( isCheckForVersion(argv[1]) )
    {
        printVersion();
        return 0;
    }

    initParameters();
    s = parseArgs(argc, argv, &find_cfg);
    if ( s != 0 )
        return -2;

    if ( payload_arg_id > 0 )
        s = run(argv[payload_arg_id][2], argv[payload_arg_id + 1], &find_cfg);
    else
        s = run(0, NULL, &find_cfg);

    return s;
}

int run(const char payload_format, const char* raw_payload, FIND_CFG *find_cfg)
{
    uint32_t pid = 0;
    int s;
    uint8_t* payload = NULL;
    uint32_t payload_ln = 0;
    char* file_name = NULL;

    if ( run_mode == RUN_MODE_FILE )
    {
        g_file_info.size = getSize(g_file_info.path);
        if ( g_file_info.size == 0 && !(g_print_flags.mode&MODE_FLAG_INSERT) )
            return 0;
    }
    else if ( run_mode == RUN_MODE_PID )
    {
        s = parseUint32(g_file_info.path, &pid, 0);
        if ( s != 0 )
            return -1;

        if ( pid == 0 )
            pid = getpid();

#ifdef _WIN32
        if ( IsProcessElevated(pid) )
        {
            DPrint("elevated!\n");
            PCHAR privileges[1] = {
                SE_DEBUG_NAME
            };
            ULONG privilegesCount = _countof(privileges);
    
            s = AddPrivileges(privileges, privilegesCount);
            if ( s != 0 )
            {
                IPrint("AddPrivileges failed! (0x%x)\n", GetLastError());
            }
            else
            {
                DPrint("debug enabled!\n");
            }
        }
#endif

        g_file_info.size = getSizeOfProcess(pid);
        if ( g_file_info.size == 0 )
            return -2;
    }

    DPrint("file_path: %s\n", g_file_info.path);
    DPrint("file_size: 0x%zx\n", g_file_info.size);
    DPrint("start: 0x%zx\n", g_print_flags.start);
    DPrint("end: 0x%zx\n", g_print_flags.end);
    DPrint("length: 0x%zx\n", g_print_flags.block_length);
    DPrint("col_mask only: %d\n", g_print_flags.cols);
    DPrint("mode: 0x%x\n", g_print_flags.mode);
    DPrint("  insert: %d\n", (g_print_flags.mode&MODE_FLAG_INSERT)>0);
    DPrint("  overwrite: %d\n", (g_print_flags.mode&MODE_FLAG_OVERWRITE)>0);
    DPrint("  delete: %d\n", (g_print_flags.mode&MODE_FLAG_DELETE)>0);
    DPrint("  find: %d\n", (g_print_flags.mode&MODE_FLAG_FIND)>0);
    DPrint("  find all: %d\n", (g_print_flags.mode&MODE_FLAG_FIND_ALL)>0);
    DPrint("  continuous: %d\n", (g_print_flags.mode&MODE_FLAG_CONTINUOUS_PRINTING)>0);
    DPrint("  clean printing: %d\n", (g_print_flags.mode&MODE_FLAG_CLEAN_PRINTING)>0);
    DPrint("  case insensitive: %d\n", (g_print_flags.mode&MODE_FLAG_CASE_INSENSITIVE)>0);
    DPrint("  print start offset: %d\n", (g_print_flags.mode&MODE_FLAG_PRINT_START_OFFSET)>0);
    DPrint("\n");

    find_cfg->flags = 0;
    if ( (g_print_flags.mode&(MODE_FLAG_INSERT|MODE_FLAG_OVERWRITE|MODE_FLAG_FIND)) && payload_format > 0 )
    {
        payload_ln = parsePayload(payload_format, raw_payload, &payload);
        if ( payload == NULL)
            return 3;

        if ( ARE_FLAGS_SET(g_print_flags.mode, (MODE_FLAG_FIND|MODE_FLAG_CASE_INSENSITIVE))
             && ( payload_format == FORMAT_ASCII || payload_format == FORMAT_UNICODE ) )
        {
            // For utf-16le (-fu) ascii text the high bytes are 0x00, 
            // so uppercasing the low letter bytes with toUpperCaseA works the same as for ascii (-fa).
            toUpperCaseA((char*)payload, payload_ln);
            find_cfg->flags = FIND_FLAG_CASE_INSENSITIVE
                | ( payload_format == FORMAT_ASCII ? FIND_FLAG_ASCII : FIND_FLAG_UNICODE );
        }
    }

    if ( (g_print_flags.mode&MODE_FLAG_INSERT) )
    {
        s = insert(&g_file_info, payload, payload_ln, g_print_flags.start);
        g_file_info.size = getSize(g_file_info.path);
    }
    else if ( (g_print_flags.mode&MODE_FLAG_OVERWRITE) && run_mode == RUN_MODE_FILE )
    {
        overwrite(&g_file_info, payload, payload_ln, g_print_flags.start);
        g_file_info.size = getSize(g_file_info.path);
    }
    else if ( (g_print_flags.mode&MODE_FLAG_OVERWRITE) && run_mode == RUN_MODE_PID )
    {
        writeProcessMemory(pid, payload, payload_ln, g_print_flags.start);
    }
    else if ( (g_print_flags.mode&MODE_FLAG_DELETE) )
    {
        if ( sanitizeDeleteParams() != 0 )
        {
            cleanUp(payload);
            return 1;
        }
        deleteBytes(&g_file_info, g_print_flags.start, g_print_flags.block_length);
        g_file_info.size = getSize(g_file_info.path);
        g_print_flags.block_length = (DEFAULT_LENGTH <= g_file_info.size) ? DEFAULT_LENGTH : g_file_info.size;
        g_print_flags.start = 0;
    }

    if ( g_file_info.size == 0 )
        return -1;

    
    s = sanitizePrintParams(pid, &g_print_flags, &g_col_sizes, find_cfg);
    if ( s != 0 )
        return -1;
    
    find_cfg->needle = payload;
    find_cfg->needle_ln = payload_ln;

    setPrintingStyle(g_print_flags.mode);
    if ( run_mode == RUN_MODE_FILE )
    {
        getFileNameL(g_file_info.path, &file_name);
        printf("file: %s\n", file_name);

        print(&g_file_info, &g_print_flags, find_cfg);
    }
    else if ( run_mode == RUN_MODE_PID )
    {
        printf("pid: %u\n", pid);
        if ( g_process_list_flags & PROCESS_LIST_RUNNING_PROCESSES )
            listRunningProcesses();
        if ( g_process_list_flags & PROCESS_LIST_MEMORY )
            listProcessMemory(pid);
        if ( g_process_list_flags & PROCESS_LIST_MODULES )
            listProcessModules(pid);
        if ( g_process_list_flags & PROCESS_LIST_THREADS )
            listProcessThreads(pid);
        if ( g_process_list_flags & (PROCESS_LIST_HEAPS | PROCESS_LIST_HEAP_BLOCKS) )
        {
            uint32_t flag = (g_process_list_flags & (PROCESS_LIST_HEAPS|PROCESS_LIST_HEAP_BLOCKS)) >> 3;
            listProcessHeaps(pid, flag);
        }

        if ( g_process_list_flags == 0 )
        {
            printProcessRegions(pid, g_print_flags.start, g_print_flags.skip, find_cfg);
        }
    }

    cleanUp(payload);

    return 0;
}

void cleanUp(uint8_t* payload)
{
    if ( payload != NULL )
        free(payload);
}

void initParameters()
{
    memset(&g_col_sizes, 0, sizeof(g_col_sizes));
    memset(&g_print_flags, 0, sizeof(g_print_flags));

    g_file_info.size = 0;

    g_print_flags.start = 0;
    g_print_flags.block_length = DEFAULT_LENGTH;
    g_print_flags.skip = 0;
    g_print_flags.mode = MODE_FLAG_CONTINUOUS_PRINTING;
    g_print_flags.cols = 0;


    g_process_list_flags = 0;

    run_mode = RUN_MODE_NONE;

    payload_arg_id = -1;
}

void printName()
{
    printf("%s\n", BIN_NAME);
    printf("A command line hex editor for files and processes.\n");
}

void printVersion()
{
    printf("Version: %s\n", BIN_VS);
    printf("Last changed: %s\n", BIN_LAST_CHANGED);
    printf("Compiled: %s %s\n", __DATE__, __TIME__);
}


void printUsage()
{
    printf("Usage: %s [options] -file a/file [options]\n", BIN_NAME);
    printf("Usage: %s [options] -pid 123 [options]\n", BIN_NAME);
}

void printHelp()
{
    printName();
    printf("\n");
    printVersion();
    printf("\n");
    printUsage();
    printf("\n");
    printf("Options:\n");
    printf(" * -file:string A file name to show the hex source of.\n"
           " * -pid:size_t A process id to print the virtuel memory and process info. Pass 0 for your own process.\n"
           " * -s:size_t Start offset. Default = 0.\n"
           " * -e:size_t End offset. Default = file size. (File mode only.)\n"
           " * -l:size_t Length of the part to display. Default = 0x100.\n"
           " * -b Force breaking mode. Will terminate after the first displayed block.\n"
           " * -pso Print start (real) offset.\n"
           " * -hvs Size of the printed hex values/groups. Maybe 1, 2, 4, 8. Defaults to 1.\n"
           " * -pp Print plain, not console styled output.\n"
           " * -cs Size of a printed column. Only respected if -px, -pa are not combined with each other.\n"
           " * Printing layouts:\n"
           "   (Not all possible combinations are allowed!)\n"
           "   * -po Print address column flag (1).\n"
           "   * -px Print HEX column flag (2).\n"
           "   * -pa Print ASCII column flag (4).\n"
           //"   * -pu Print UNICODE (utf-16) column flag (8).\n"
           "   * -pbs Print plain byte string flag (0x10).\n"
           "   * -cm Set the desired column mask directly as the given number.\n"
           " * File manipulation/examination.\n"
           "   * -d Delete -l bytes from offset -s. (File mode only.). Pass -l 0 to delete from -s to file end.\n"
           "   * -i* Insert hex byte sequence (destructive!). Where * is a format option. (File mode only.)\n"
           "   * -o* Overwrite hex byte sequence (destructive!). Where * is a format option.\n"
           "   * -f* Find hex byte sequence. Where * is a format option.\n"
           "   * Format options:\n"
           "     * %c: plain byte string, i.e. C007C0FF33\n"
           "     * %c: ascii/utf-8 text\n"
           "     * %c: unicode (windows utf16) text.\n"
           "     * %c: byte (uint8)\n"
           "     * %c: fill byte (will be inserted -l times)\n"
           "     * %c: word (uint16)\n"
           "     * %c: double word (uint32)\n"
           "     * %c: quad word (uint64).\n"
           "     Except for the string types, all values have to be passed as hex values, omitting `0x`.\n"
           "   * Find options:\n"
           "     * -ci: Case insensitive (ascii range, for -fa and -fu search).\n"
           "     * -all: Find all occurrences.\n"
           "     * -pfo: Print the exact found offset separately.\n"
           "     * -mfc: Number of max findable occurrences from the start.\n"
//         " * -ie:uint8_t Endianness of payload (little: 1, big:2). Defaults to 1 = little endian.\n"
           " * -pid only options:\n"
           "   * -lpx List entire process memory layout.\n"
           "   * -lpm List all process modules.\n"
           "   * -lpt List all process threads.\n"
           "   * -lph List all process heaps.\n"
           "   * -lphb List all process heaps and its blocks.\n"
           "   * -lrp List all running processes. Pass any pid or 0 to get it running.\n"
           " * -h Print this.\n",
           FORMAT_PLAIN_HEX_2, FORMAT_ASCII, FORMAT_UNICODE, FORMAT_BYTE, FORMAT_FILL_BYTE, FORMAT_WORD, FORMAT_D_WORD, FORMAT_Q_WORD
    );
    printf("\n");
    printf("Example: ./%s -file path/to/a.file -s 100 -l 128 -x\n", BIN_NAME);
    printf("Example: ./%s -file path/to/a.file -ih dead -s 0x100\n", BIN_NAME);
    printf("Example: ./%s -file path/to/a.file -oh 0bea -s 0x100\n", BIN_NAME);
    printf("Example: ./%s -file path/to/a.file -fh f001 -s 0x100\n", BIN_NAME);
    printf("Example: ./%s -file path/to/a.file -d -s 0x100 -l 0x8\n", BIN_NAME);
    printf("Example: ./%s -pid 0 -lrp\n", BIN_NAME);
    printf("Example: ./%s -pid 1234 -s 0x5000 -lpm\n", BIN_NAME);
    printf("\n");
    printf("In continuous mode press ENTER to continue, 'n' to find next or 'q' to quit.\n");
}

#define BREAK_ON_NO_VALUE(_a_, _i_, _end_i_, _s_) \
            if ( !hasValue(_a_, _i_, _end_i_) ) \
            { \
                EPrint("Missing value for "_a_"!\n"); \
                _s_ = -1; \
                break; \
            }

#define BREAK_ON_FAILED_INT_PARSING(_a_, _i_, _end_i_, _s_) \
            if ( _s_ != 0 ) \
            { \
                EPrint("Parsing "_a_" value failed!\n"); \
                _s_ = -1; \
                break; \
            }

int parseArgs(int argc, char** argv, FIND_CFG *find_cfg)
{
    FEnter();

    int start_i = 1;
    int end_i = argc - 1;
    int i = 0;
    int s = 0;
    uint8_t length_found = 0;
    const char* source = NULL;

    for ( i = start_i; i < argc; i++ )
    {
        //if ( argv[i][0] != LIN_PARAM_IDENTIFIER && argv[i][0] != WIN_PARAM_IDENTIFIER )
        //    break;

        if ( isArgOfType(argv[i], "-px") )
        { 
            g_print_flags.cols |= COL_MASK_HEX;
        }
        else if ( isArgOfType(argv[i], "-pa") )
        {
            g_print_flags.cols |= COL_MASK_ASCII;
        }
        //else if ( isArgOfType(argv[i], "-pu") )
        //{
        //    g_print_flags.cols |= COL_MASK_UNICODE;
        //}
        else if ( isArgOfType(argv[i], "-po") )
        {
            g_print_flags.cols |= COL_MASK_OFFSET;
        }
        else if ( isArgOfType(argv[i], "-cm") )
        {
            BREAK_ON_NO_VALUE("-cm", i, end_i, s);

            s = parseUint32(argv[i + 1], &g_print_flags.cols, 0);
            BREAK_ON_FAILED_INT_PARSING("-cm", i, end_i, s)

            i++;
        }
        else if ( isArgOfType(argv[i], "-pp") )
        {
            g_print_flags.mode |= MODE_FLAG_CLEAN_PRINTING;
        }
        else if ( isArgOfType(argv[i], "-pbs") )
        {
            g_print_flags.cols |= COL_MASK_BYTE_STRING;
        }
        else if ( isArgOfType(argv[i], "-d") )
        {
            g_print_flags.mode |= MODE_FLAG_DELETE;
        }
        else if ( isArgOfType(argv[i], "-b") )
        {
            g_print_flags.mode &= ~MODE_FLAG_CONTINUOUS_PRINTING;
        }
        else if ( isArgOfType(argv[i], "-lpx") )
        {
            g_process_list_flags |= PROCESS_LIST_MEMORY;
        }
        else if ( isArgOfType(argv[i], "-lpm") )
        {
            g_process_list_flags |= PROCESS_LIST_MODULES;
        }
        else if ( isArgOfType(argv[i], "-lpt") )
        {
            g_process_list_flags |= PROCESS_LIST_THREADS;
        }
        else if ( isArgOfType(argv[i], "-lph") )
        {
            g_process_list_flags |= PROCESS_LIST_HEAPS;
        }
        else if ( isArgOfType(argv[i], "-lphb") )
        {
            g_process_list_flags |= PROCESS_LIST_HEAP_BLOCKS;
        }
        else if ( isArgOfType(argv[i], "-lrp") )
        {
            g_process_list_flags |= PROCESS_LIST_RUNNING_PROCESSES;
        }
        else if ( isArgOfType(argv[i], "-file") )
        {
            BREAK_ON_NO_VALUE("-file", i, end_i, s);
            
            source = argv[i + 1];
            run_mode = RUN_MODE_FILE;
            i++;
        }
        else if ( isArgOfType(argv[i], "-pid") )
        {
            BREAK_ON_NO_VALUE("-pid", i, end_i, s);

            source = argv[i + 1];
            run_mode = RUN_MODE_PID;
            i++;
        }
        else if ( isArgOfType(argv[i], "-s") )
        {
            BREAK_ON_NO_VALUE("-s", i, end_i, s);

            s = parseSizeAuto(argv[i + 1], &g_print_flags.start);
            BREAK_ON_FAILED_INT_PARSING("-s", i, end_i, s);

            i++;
        }
        else if ( isArgOfType(argv[i], "-e") )
        {
            BREAK_ON_NO_VALUE("-e", i, end_i, s);

            s = parseSizeAuto(argv[i + 1], &g_print_flags.end);
            BREAK_ON_FAILED_INT_PARSING("-e", i, end_i, s);

            i++;
        }
        else if ( isArgOfType(argv[i], "-l") )
        {
            BREAK_ON_NO_VALUE("-l", i, end_i, s);

            s = parseSizeAuto(argv[i + 1], &g_print_flags.block_length);
            BREAK_ON_FAILED_INT_PARSING("-l", i, end_i, s);

            length_found = 1;
            i++;
        }
        else if ( isFormatArgOfType(argv[i], "-i") )
        {
            BREAK_ON_NO_VALUE("-i", i, end_i, s);

            g_print_flags.mode |= MODE_FLAG_INSERT;
            payload_arg_id = i;
            i++;
        }
        else if ( isFormatArgOfType(argv[i], "-o") )
        {
            BREAK_ON_NO_VALUE("-o", i, end_i, s);

            g_print_flags.mode |= MODE_FLAG_OVERWRITE;
            payload_arg_id = i;
            i++;
        }
        else if ( isFormatArgOfType(argv[i], "-f") )
        {
            BREAK_ON_NO_VALUE("-f", i, end_i, s);

            g_print_flags.mode |= MODE_FLAG_FIND;
            payload_arg_id = i;
            i++;
        }
        else if ( isArgOfType(argv[i], "-ci") )
        {
            g_print_flags.mode |= MODE_FLAG_CASE_INSENSITIVE;
        }
        else if ( isArgOfType(argv[i], "-all") )
        {
            g_print_flags.mode |= MODE_FLAG_FIND_ALL;
        }
        else if ( isArgOfType(argv[i], "-mfc") )
        {
            BREAK_ON_NO_VALUE("-mfc", i, end_i, s);
            
            s = parseUint32(argv[i + 1], &find_cfg->max_count, 0);
            BREAK_ON_FAILED_INT_PARSING("-mfc", i, end_i, s);

            i++;
        }
        else if ( isArgOfType(argv[i], "-pso") )
        {
            g_print_flags.mode |= MODE_FLAG_PRINT_START_OFFSET;
        }
        else if ( isArgOfType(argv[i], "-pfo") )
        {
            g_print_flags.mode |= MODE_FLAG_PRINT_START_OFFSET;
        }
        else if ( isArgOfType(argv[i], "-hvs") )
        {
            BREAK_ON_NO_VALUE("-hvs", i, end_i, s);

            s = parseUint32(argv[i + 1], &g_print_flags.value_size, 0);
            BREAK_ON_FAILED_INT_PARSING("-hvs", i, end_i, s);

            i++;
        }
        else if ( isArgOfType(argv[i], "-cs") )
        {
            BREAK_ON_NO_VALUE("-cs", i, end_i, s);

            s = parseUint32(argv[i + 1], &g_col_sizes.custom, 0);
            BREAK_ON_FAILED_INT_PARSING("-cs", i, end_i, s);

            i++;
        }
        else
        {
            EPrint("Unknown arg type \"%s\"\n", argv[i]);
            s = -1;
            break;
        }
    }

    if ( s != 0 )
    {
        return s;
    }

    if ( run_mode == RUN_MODE_NONE )
    {
//      EPrint("You have to specify either a -file or a -pid!\n");
        printUsage();
        return -1;
    }
   
    DPrint("g_print_flags.mode: 0x%x\n", g_print_flags.mode);
    uint32_t f = g_print_flags.mode&(MODE_FLAG_FIND|MODE_FLAG_OVERWRITE|MODE_FLAG_INSERT|MODE_FLAG_DELETE);
    if ( (f & (f-1)) != 0 )
    {
        EPrint("Overwrite, insert, delete and find have to be used exclusively!\n");
        s = -2;
        goto clean;
    }


    //
    // check args
    //

    if ( (g_print_flags.mode&MODE_FLAG_DELETE) && !length_found )
    {
        EPrint("Could not parse length of part to delete! Pass -l 0, if you want to delete from -s to the end of file.\n");
        s = -3;
        goto clean;
    }

    if ( run_mode == RUN_MODE_PID && (g_print_flags.mode&(MODE_FLAG_INSERT|MODE_FLAG_DELETE)) > 0 )
    {
        EPrint("Inserting or deleting is not supported in process mode!\n");
        s = -4;
        goto clean;
    }
    

    if ( run_mode == RUN_MODE_FILE )
    {
        s = expandFilePath(source, g_file_info.path);
        if ( s != 0 )
            goto clean;
    }    
    else
        snprintf(g_file_info.path, PATH_MAX, "%s", source);
    
clean:
    FEnter();
    return s;
}

uint8_t isArgOfType(const char* arg, const char* type)
{
    size_t i;
    size_t type_ln;
    if ( arg[0] != LIN_PARAM_IDENTIFIER && arg[0] != WIN_PARAM_IDENTIFIER )
        return 0;

    type_ln = strlen(type);

    for ( i = 1; i < type_ln; i++ )
    {
        if ( arg[i] != type[i] )
            return 0;
    }
    return arg[i] == 0;
}

uint8_t isCallForHelp(const char* arg1)
{
    return isArgOfType(arg1, "/h") || 
           isArgOfType(arg1, "/?");
}

uint8_t isCheckForVersion(const char* arg1)
{
    return isArgOfType(arg1, "/version") || 
           isArgOfType(arg1, "/vs");
}

uint8_t isFormatArgOfType(char* arg, char* type)
{
    uint8_t i, j;
    uint8_t arg_ln = (uint8_t)strlen(arg);
    uint8_t type_ln = (uint8_t)strlen(type);

    if ( arg_ln <= type_ln )
        return 0;

    if ( arg[0] != LIN_PARAM_IDENTIFIER && arg[0] != WIN_PARAM_IDENTIFIER )
        return 0;

    for ( i = 1; i < type_ln; i++ )
        if ( arg[i] != type[i] )
            return 0;

    j = i;
    for ( i = 0; i < format_types_ln; i++ )
        if ( format_types[i] == arg[j] )
            return arg[j+1]==0;

    return 0;
}

uint8_t hasValue(char* type, int i, int end_i)
{
    if ( i >= end_i )
    {
        IPrint("Arg \"%s\" has no value! Skipped!\n", type);
        return 0;
    }

    return 1;
}

int sanitizeDeleteParams()
{
    if ( !(g_print_flags.mode&MODE_FLAG_DELETE) )
        return 0;
    
    uint8_t info_line_break = 0;

    if ( g_print_flags.start >= g_file_info.size )
    {
        EPrint("Start offset 0x%zx is greater then the file size of 0x%zx!\n",
                g_print_flags.start, g_file_info.size);
        return 1;
    }
    
    if ( g_print_flags.block_length == 0 )
    {
        printf("Info: Length is 0. Setting it to remaining file size 0x%zx!\n", 
            g_file_info.size - g_print_flags.start);
        g_print_flags.block_length = g_file_info.size - g_print_flags.start;
        info_line_break = 1;
    }
    
    if ( g_print_flags.start + g_print_flags.block_length > g_file_info.size )
    {
        g_print_flags.block_length = g_file_info.size - g_print_flags.start;
    }

    if ( info_line_break )
        printf("\n");
    
    return 0;
}


int alignValueUpToValueSize(uint32_t vs, size_t* value)
{
    int is_aligned = 0;
    if ( vs == 2 && *value % 2 != 0 )
    {
        *value = ALIGN_UP_BY(*value, 2);
        is_aligned = 1;
    }
    else if ( vs == 4 && *value % 4 != 0 )
    {
        *value = ALIGN_UP_BY(*value, 4);
        is_aligned = 1;
    }
    else if ( vs == 8 && *value % 8 != 0 )
    {
        *value = ALIGN_UP_BY(*value, 8);
        is_aligned = 1;
    }

    return is_aligned;
}

int alignValueDownToValueSize(uint32_t vs, size_t* value)
{
    int is_aligned = 0;
    if ( vs == 2 && *value % 2 != 0 )
    {
        *value = ALIGN_DOWN_BY(*value, 2);
        is_aligned = 1;
    }
    else if ( vs == 4 && *value % 4 != 0 )
    {
        *value = ALIGN_DOWN_BY(*value, 4);
        is_aligned = 1;
    }
    else if ( vs == 8 && *value % 8 != 0 )
    {
        *value = ALIGN_DOWN_BY(*value, 8);
        is_aligned = 1;
    }

    return is_aligned;
}

int sanitizePrintParams(uint32_t pid, PRINT_CFG *print_flags, COL_SIZES *col_sizes, FIND_CFG *find_cfg)
{
    FEnter();

    uint32_t col_size;
    uint8_t info_line_break = 0;
    
    //
    // check col flags
    //
    // set to default mode if nothing is set
    // else check validity of mask
    //

    if ( !print_flags->cols )
    {
        print_flags->cols = (COL_MASK_OFFSET|COL_MASK_HEX|COL_MASK_ASCII);
    }
    else
    {
        //uint32_t f = print_flags->cols&(COL_MASK_ASCII|COL_MASK_UNICODE);
        //if ( (f & (f-1)) != 0 )
        //{
        //    EPrint("Ascii and unicode printing can't be combined!\n");
        //    return -5;
        //}
        if ( print_flags->cols == COL_MASK_OFFSET )
        {
            EPrint("Printing only offsets is not provided! Please select one or more of -pa, -px.\n");
            return -6;
        }
        DPrint("print_flags->cols: 0x%x\n", print_flags->cols);
        if ( ( print_flags->cols < COL_MASK_OFFSET || print_flags->cols > (COL_MASK_OFFSET|COL_MASK_HEX|COL_MASK_ASCII) )
            && ( print_flags->cols != COL_MASK_BYTE_STRING && print_flags->cols != (COL_MASK_OFFSET|COL_MASK_BYTE_STRING) ) )
        {
            EPrint("Invalid column flags!\n");
            return -7;
        }
    }

    //
    // check hex size
    //
    // default is 1
    // find mode forces 1
    //

    if ( !print_flags->value_size )
        print_flags->value_size = 1;
    if ( (print_flags->mode & MODE_FLAG_FIND) && print_flags->value_size != 1 )
    {
        IPrint("In find mode, currently just a hex value size of 1 is supported!\n");
        print_flags->value_size = 1;
    }
    
    //
    // align block length to hex value size

    int is_aligned = 0;
    is_aligned = alignValueUpToValueSize(print_flags->value_size, &print_flags->block_length);
    if ( is_aligned ) { 
        IPrint("Aligned length up tp 0x%zx bytes!\n", print_flags->block_length) }

    is_aligned = alignValueDownToValueSize(print_flags->value_size, &print_flags->start);
    if ( is_aligned ) { 
        IPrint("Aligned start down to 0x%zx bytes!\n", print_flags->start) }

    size_t value = col_sizes->custom;
    is_aligned = (uint32_t)alignValueDownToValueSize(print_flags->value_size, &value);
    if ( is_aligned )
    {
        if ( value < print_flags->value_size )
            value = print_flags->value_size;
        col_sizes->custom = (uint32_t)value;
        IPrint("Aligned custom col size to 0x%x bytes!\n", col_sizes->custom)
    }


    //
    // check col size
    // max col size is buffer size for uncomplicated printing

    if ( col_sizes->custom > MAX_COL_SIZE )
    {
        IPrint("Custom col size too big! Setting it to 0x%x\n", MAX_COL_SIZE);
        col_sizes->custom = MAX_COL_SIZE;
    }

    
    //
    // force breaking mode for special scenarios

    if ( print_flags->mode&(MODE_FLAG_INSERT|MODE_FLAG_OVERWRITE|MODE_FLAG_DELETE) )
        print_flags->mode &= ~MODE_FLAG_CONTINUOUS_PRINTING;
    else if ( ARE_FLAGS_SET(print_flags->mode, (MODE_FLAG_FIND|MODE_FLAG_FIND_ALL) ) )
        print_flags->mode &= ~MODE_FLAG_CONTINUOUS_PRINTING;

    // 
    // if stdout is redirected, set some special rules
#if defined(_LINUX)
    if ( !isatty(fileno(stdout)) || !isatty(fileno(stdin)) )
#elif defined(_WIN32)
    if ( !_isatty(_fileno(stdout)) || !_isatty(_fileno(stdin)) )
#endif
    {
        // always set breaking mode
        print_flags->mode &= ~MODE_FLAG_CONTINUOUS_PRINTING;

        // if in find mode
        if ( IS_FLAG_SET(print_flags->mode, MODE_FLAG_FIND) && !IS_FLAG_SET(print_flags->mode, MODE_FLAG_FIND_ALL) )
        {
            // if not -all, set -all
            print_flags->mode |= MODE_FLAG_FIND_ALL;
            // if not -mfc limit unknown result to 1
            if ( !find_cfg->max_count )
            {
                find_cfg->max_count = 1;
            }
        }
    }

    //
    // initialize col sizes

    col_size = getColSize(print_flags->cols, &g_col_sizes);
    if ( col_size == 0 )
    {
        EPrint("Col size error!\n");
        return -1;
    }

    // normalize block_length to block size for continuous printing
    if ( (print_flags->mode&MODE_FLAG_CONTINUOUS_PRINTING)
        && print_flags->cols != COL_MASK_BYTE_STRING )
    {
        if ( print_flags->block_length % col_size != 0 )
        {
            print_flags->block_length = (size_t)ALIGN_UP_BY(print_flags->block_length, col_size);
            IPrint("Normalized length to 0x%zx\n", print_flags->block_length);
        }
    }


    // check start offset
    if ( run_mode == RUN_MODE_FILE )
    {
        // 
        // set end to file size if not set or exceeding it
        if ( !print_flags->end || print_flags->end > g_file_info.size )
            print_flags->end = g_file_info.size;

        if ( print_flags->start >= print_flags->end )
        {
            EPrint("Start offset 0x%zx is greater then the end offset of 0x%zx!\n\n", print_flags->start, print_flags->end);
            return -1;
        }
        //if ( print_flags->start >= g_file_info.size )
        //{
        //    EPrint("Start offset 0x%zx is greater then the file size of 0x%zx!\n\n", print_flags->start, g_file_info.size);
        //    return -1;
        //}
    }
    else if ( run_mode == RUN_MODE_PID )
    {
        info_line_break = makeStartHitAccessableMemory(pid, &print_flags->start);
    }

    // normalize start offset to block size
    // called after insert and overwrite
    if ( !(print_flags->mode&(MODE_FLAG_FIND|MODE_FLAG_DELETE)) )
    {
        print_flags->start = normalizeOffset(print_flags->start, &print_flags->skip);
        if ( !(print_flags->mode&MODE_FLAG_CONTINUOUS_PRINTING) )
            print_flags->block_length += print_flags->skip;
    }

    // check block_length
    if ( run_mode == RUN_MODE_FILE )
        info_line_break = keepLengthPrintingRange(print_flags->start, print_flags->end, &print_flags->block_length);
        //info_line_break = keepLengthInFile(print_flags->start, g_file_info.size, &print_flags->block_length);
//  else if ( type == RUN_MODE_PID )
//      info_line_break = keepLengthInModule(pid);

    if ( print_flags->block_length == 0 )
    {
        printf("Info: Length is 0. Setting to 0x%x!\n", DEFAULT_LENGTH);
        print_flags->block_length = DEFAULT_LENGTH;
        info_line_break = 1;
    }

    if ( info_line_break )
        printf("\n");

    FLeave();
    return 0;
}

uint8_t keepLengthPrintingRange(size_t start, size_t end, size_t* length)
{
    if ( start + *length > end )
    {
        //printf("Info: Start offset 0x%zx plus length 0x%zx is greater then the file size 0x%zx\n"
        //    "Printing only to file size.\n",
        //start, *length, size);
        
        *length = end - start;
        //g_print_flags.mode &= ~MODE_FLAG_CONTINUOUS_PRINTING;
        return 0;
    }
    return 0;
}

//uint8_t keepLengthInFile(size_t start, size_t size, size_t* length)
//{
//    if ( start + *length > size )
//    {
//        //printf("Info: Start offset 0x%zx plus length 0x%zx is greater then the file size 0x%zx\n"
//        //    "Printing only to file size.\n",
//        //start, *length, size);
//        
//        *length = size - start;
//        //g_print_flags.mode &= ~MODE_FLAG_CONTINUOUS_PRINTING;
//        return 0;
//    }
//    return 0;
//}

/**
 * Parse payload from
 * 
 * @param format char the format of the raw payload string
 * @param value char* the raw payload value
 * @param payload char** the array to store the formatted payload in
 * @return uint32_t length of parsed payload.
 */
uint32_t parsePayload(const char format, const char* value, uint8_t** payload)
{
    uint32_t ln = 0;

    if ( strnlen(value, MAX_PAYLOAD_LN) == MAX_PAYLOAD_LN )
    {
        EPrint("Payload greater max payload size of 0x%x!\n", MAX_PAYLOAD_LN);
        return 0;
    }

    switch ( format )
    {
        case FORMAT_BYTE:
        {
            ln = payloadParseByte(value, payload);
            break;
        }
        case FORMAT_FILL_BYTE:
        {
            if ( g_print_flags.block_length > MAX_PAYLOAD_LN )
            {
                IPrint("Fill byte length is greater than 0x%x (%u). Setting to 0x%x (%u)!\n", MAX_PAYLOAD_LN, MAX_PAYLOAD_LN, MAX_PAYLOAD_LN, MAX_PAYLOAD_LN);
                g_print_flags.block_length = MAX_PAYLOAD_LN;
            }
            ln = payloadParseFillBytes(value, payload, g_print_flags.block_length);
            g_print_flags.block_length = DEFAULT_LENGTH;
            break;
        }
        case FORMAT_WORD:
            ln = payloadParseWord(value, payload);
            break;
        case FORMAT_D_WORD:
            ln = payloadParseDWord(value, payload);
            break;
        case FORMAT_Q_WORD:
            ln = payloadParseQWord(value, payload);
            break;
        case FORMAT_ASCII:
            ln = payloadParseUtf8(value, payload);
            break;
        case FORMAT_UNICODE:
            ln = payloadParseUtf16(value, payload, 0x100);
            break;
    //  case 'r':
    //      ln = payloadParseReversedPlainBytes(arg, payload);
        case FORMAT_PLAIN_HEX_1:
        case FORMAT_PLAIN_HEX_2:
        {
            char* cleaned_value = NULL;
            int s = cleanBytes(value, &cleaned_value);
            if ( s != 0 )
                return 0;
            ln = payloadParsePlainBytes(cleaned_value, payload);
            free(cleaned_value);
            break;
        }
        default:
        {
            EPrint("%c is not a supported format!\n", format);
            ln = 0;
            break;
        }
    }
    
    return ln;
}

/**
 * Library function to print a file (-file <name>).
 *
 * @param _file_name
 * @param _start
 * @param _length
 * @return int status info
 */
HEXTER_API int hexter_printFile(const char* _file_name, size_t _start, size_t _length)
{
    FIND_CFG find_cfg = { 0 };

    initParameters();
    int s = expandFilePath(_file_name, g_file_info.path);
    if ( s != 0 )
        return s;

    run_mode = RUN_MODE_FILE;
    
    g_print_flags.start = _start;
    g_print_flags.block_length = _length;
    g_print_flags.mode &= ~MODE_FLAG_CONTINUOUS_PRINTING;

    run(0, NULL, &find_cfg);

    return 0;
}

/**
 * Library function to print a process (-pid <pid>).
 *
 * @param _pid
 * @param _start
 * @param _length
 * @param flags
 * @return int status info
 */
HEXTER_API int hexter_printProcess(uint32_t _pid, size_t _start, size_t _length, uint32_t flags)
{
    FIND_CFG find_cfg = { 0 };

    initParameters();
#ifdef _WIN32
    snprintf(g_file_info.path, PATH_MAX, "%u", _pid);
#else
    snprintf(g_file_info.path, PATH_MAX, "%u", _pid);
#endif

    run_mode = RUN_MODE_PID;
    g_print_flags.start = _start;
    g_print_flags.block_length = _length;
    g_print_flags.mode &= ~MODE_FLAG_CONTINUOUS_PRINTING;

    g_process_list_flags = flags;

    run(0, NULL, &find_cfg);

    return 0;
}

/**
 * Intended to be called by rundll32.
 * Usage: rundll32 hexter.dll,runHexter hexter params
 * Example: rundll32 hexter.dll,runHexter hexter -pid 0 -lrp
 * The "hexter" param is a dummy param and the rest of the params should be used as explained in normal usage:
 * -file|-pid xxx [options]
 * With the "hexter" dummy param, the split arguments may be passed to the main function.
 * Otherwise it had to be added internally.
 *
 * @param hwnd
 * @param hinst
 * @param lpszCmdLine
 * @param nCmdShow
 */
#ifdef _WIN32
HEXTER_API void runHexter(HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine, int nCmdShow)
{
    AllocConsole();
//  AttachConsole(GetCurrentProcessId());
//  AttachConsole(-1);
//  AttachConsole(_getpid());
    freopen("conin$", "r", stdin);
    freopen("conout$", "w", stdout);
    freopen("conout$", "w", stderr);

    (void) hwnd;
    (void) hinst;
    (void) nCmdShow;

    DPrint("the param cmd line: %s\n", lpszCmdLine);

    uint8_t argv_max = 20;
    uint8_t argc;
    char* argv[20];
    argc = (uint8_t)splitArgs(lpszCmdLine, argv, argv_max);

#ifdef DEBUG_PRINT
    int i;
    DPrint("argc: %u\n", argc);
    for ( i = 0; i < argc; i++ )
        DPrint("arg%d: %s\n", i, argv[i]);
#endif

    main(argc, argv);
    getchar();
}

#endif
