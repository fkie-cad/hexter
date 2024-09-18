#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
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

// for usage in Diller
//#define DILLER

#include "hexter.h"
#include "Globals.h"
#include "utils/common_fileio.h"
#include "Printer.h"
#include "Writer.h"
#include "utils/Converter.h"
#include "utils/Helper.h"
#if defined(_LINUX)
    #include "ProcessHandlerLinux.h"
#elif defined(_WIN32)
    #include <process.h>
    #include <time.h>
    #include "utils/win/processes.h"
    #include "ProcessHandlerWin.h"
#endif
#include "utils/Strings.h"

#define BIN_NAME ("hexter")
#define BIN_VS "1.8.3"
#define BIN_LAST_CHANGED  "17.09.2024"

#define LIN_PARAM_IDENTIFIER ('-')
#define WIN_PARAM_IDENTIFIER ('/')


size_t file_size;
char file_path[PATH_MAX];
static size_t start;
size_t length;
static uint8_t skip_bytes;

uint8_t print_col_mask;

uint32_t process_list_flags;
uint32_t mode_flags;

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

static void printUsage();
void printHelp();
static void initParameters();
static int parseArgs(int argc, char** argv);
static uint8_t isArgOfType(const char* arg, const char* type);
static uint8_t isCallForHelp(const char* arg1);
static uint8_t isFormatArgOfType(char* arg, char* type);
static uint8_t hasValue(char* type, int i, int end_i);
static int sanitizeDeleteParams();
static int sanitizePrintParams(uint32_t pid);
static uint32_t parsePayload(const char format, const char* value, uint8_t** payload);

static int run(const char payload_format, const char* raw_payload);
void cleanUp(uint8_t* payload);

static uint8_t keepStartInFile();
static uint8_t keepLengthInFile();

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
    if ( argc < 2 )
    {
        printUsage();
        return -1;
    }

    if ( isCallForHelp(argv[1]) )
    {
        printHelp();
        return 1;
    }

    initParameters();
    s = parseArgs(argc, argv);
    if ( s != 0 )
        return -2;

    if ( payload_arg_id > 0 )
        s = run(argv[payload_arg_id][2], argv[payload_arg_id + 1]);
    else
        s = run(0, NULL);

    return s;
}

int run(const char payload_format, const char* raw_payload)
{
    uint32_t pid = 0;
    int s;
    uint8_t* payload = NULL;
    uint32_t payload_ln = 0;
    char* file_name = NULL;

    if ( run_mode == RUN_MODE_FILE )
    {
        file_size = getSize(file_path);
        if ( file_size == 0 && !(mode_flags&MODE_FLAG_INSERT) )
            return 0;
    }
    else if ( run_mode == RUN_MODE_PID )
    {
        s = parseUint32Auto(file_path, &pid);
        if ( s != 0 )
            return -1;

        if ( pid == 0 )
            pid = getpid();

#ifdef _WIN32
        if ( IsProcessElevated(pid) )
        {
            debug_info("elevated!\n");
            PCHAR privileges[1] = {
                SE_DEBUG_NAME
            };
            ULONG privilegesCount = _countof(privileges);
    
            s = AddPrivileges(privileges, privilegesCount);
            if ( s != 0 )
            {
                EPrint("AddPrivileges failed! (0x%x)\n", GetLastError());
            }
            debug_info("debug enabled!\n");
        }
#endif

        file_size = getSizeOfProcess(pid);
        if ( file_size == 0 )
            return -2;
    }

    debug_info("file_path: %s\n", file_path);
    debug_info("file_size: 0x%zx\n", file_size);
    debug_info("start: 0x%zx\n", start);
    debug_info("length: 0x%zx\n", length);
    debug_info("print_col_mask only: %d\n", print_col_mask);
    debug_info("insert: %d\n", (mode_flags&MODE_FLAG_INSERT));
    debug_info("overwrite: %d\n", (mode_flags&MODE_FLAG_OVERWRITE));
    debug_info("find: %d\n", (mode_flags&MODE_FLAG_FIND));
    debug_info("delete: %d\n", (mode_flags&MODE_FLAG_DELETE));
    debug_info("\n");

    if ( (mode_flags&(MODE_FLAG_INSERT|MODE_FLAG_OVERWRITE|MODE_FLAG_FIND)) && payload_format > 0 )
    {
        payload_ln = parsePayload(payload_format, raw_payload, &payload);
        if ( payload == NULL)
            return 3;
        if ( ((mode_flags & (MODE_FLAG_FIND|MODE_FLAG_CASE_INSENSITIVE)) == (MODE_FLAG_FIND|MODE_FLAG_CASE_INSENSITIVE))
            && payload_format == FORMAT_ASCII )
        {
            toUpperCaseA((char*)payload, payload_ln);
        }
    }

    if ( (mode_flags&MODE_FLAG_INSERT) )
    {
        insert(file_path, payload, payload_ln, start);
        file_size = getSize(file_path);
    }
    else if ( (mode_flags&MODE_FLAG_OVERWRITE) && run_mode == RUN_MODE_FILE )
    {
        overwrite(file_path, payload, payload_ln, start);
        file_size = getSize(file_path);
    }
    else if ( (mode_flags&MODE_FLAG_OVERWRITE) && run_mode == RUN_MODE_PID )
    {
        writeProcessMemory(pid, payload, payload_ln, start);
    }

    if ( (mode_flags&MODE_FLAG_DELETE) )
    {
        if ( sanitizeDeleteParams() != 0 )
        {
            cleanUp(payload);
            return 1;
        }
        deleteBytes(file_path, start, length);
        file_size = getSize(file_path);
        length = (DEFAULT_LENGTH <= file_size) ? DEFAULT_LENGTH : file_size;
        start = 0;
    }

    if ( file_size == 0 )
        return -1;

    s = sanitizePrintParams(pid);
    if ( s != 0 )
        return -1;

    setPrintingStyle();
    if ( run_mode == RUN_MODE_FILE )
    {
        getFileNameL(file_path, &file_name);
        printf("file: %s\n", file_name);
        print(start, skip_bytes, payload, payload_ln);
    }
    else if ( run_mode == RUN_MODE_PID )
    {
        printf("pid: %u\n", pid);
        if ( process_list_flags & PROCESS_LIST_RUNNING_PROCESSES )
            listRunningProcesses();
        if ( process_list_flags & PROCESS_LIST_MEMORY )
            listProcessMemory(pid);
        if ( process_list_flags & PROCESS_LIST_MODULES )
            listProcessModules(pid);
        if ( process_list_flags & PROCESS_LIST_THREADS )
            listProcessThreads(pid);
        if ( process_list_flags & (PROCESS_LIST_HEAPS | PROCESS_LIST_HEAP_BLOCKS) )
        {
            uint32_t flag = (process_list_flags & (PROCESS_LIST_HEAPS|PROCESS_LIST_HEAP_BLOCKS)) >> 3;
            listProcessHeaps(pid, flag);
        }

        if ( process_list_flags == 0 )
            printProcessRegions(pid, start, skip_bytes, payload, payload_ln);
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
    file_size = 0;
    start = 0;
    length = DEFAULT_LENGTH;
    skip_bytes = 0;

    mode_flags = MODE_FLAG_CONTINUOUS_PRINTING;
    process_list_flags = 0;
    print_col_mask = 0;
    run_mode = RUN_MODE_NONE;

    payload_arg_id = -1;
}

void printVersion()
{
    printf("%s\n", BIN_NAME);
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
    printVersion();
    printf("\n");
    printUsage();
    printf("\n");
    printf("Options:\n");
    printf(" * -file:string A file name to show the hex source of.\n"
           " * -pid:size_t A process id to print the virtuel memory and process info. Pass 0 for your own process.\n"
           " * -s:size_t Start offset. Default = 0.\n"
           " * -l:size_t Length of the part to display. Default = 0x100.\n"
           " * -b Force breaking mode. Will terminate after the first displayed block.\n"
           " * Printing format:\n"
           "   * -pa ASCII only print.\n"
           "   * -pu UNICODE (utf-16) only print.\n"
           "   * -px HEX only print.\n"
           "   * -po Print address (only valid in combination with the other options).\n"
           "   * -pp Print plain, not console styled output.\n"
           "   * -pbs Print plain byte string.\n"
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
           "     Expect for the string types, all values have to be passed as hex values, omitting `0x`.\n"
           "   * Find options:\n"
           "     * -ci: case insensitive (for ascii search only).\n"
//         " * -e:uint8_t Endianess of payload (little: 1, big:2). Defaults to 1 = little endian.\n"
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

int parseArgs(int argc, char** argv)
{
    int start_i = 1;
    int end_i = argc - 1;
    int i, s;
    uint8_t length_found = 0;
    const char* source = NULL;

    for ( i = start_i; i < argc; i++ )
    {
        //if ( argv[i][0] != LIN_PARAM_IDENTIFIER && argv[i][0] != WIN_PARAM_IDENTIFIER )
        //    break;

        if ( isArgOfType(argv[i], "-px") )
        { 
            print_col_mask = print_col_mask | PRINT_HEX_MASK;
        }
        else if ( isArgOfType(argv[i], "-pa") )
        {
            print_col_mask = print_col_mask | PRINT_ASCII_MASK;
        }
        else if ( isArgOfType(argv[i], "-pu") )
        {
            print_col_mask = print_col_mask | PRINT_UNICODE_MASK;
        }
        else if ( isArgOfType(argv[i], "-po") )
        {
            print_col_mask = print_col_mask | PRINT_OFFSET_MASK;
        }
        else if ( isArgOfType(argv[i], "-pp") )
        {
            mode_flags |= MODE_FLAG_CLEAN_PRINTING;
        }
        else if ( isArgOfType(argv[i], "-pbs") )
        {
            print_col_mask = PRINT_BYTES_STRING;
        }
        else if ( isArgOfType(argv[i], "-d") )
        {
            mode_flags |= MODE_FLAG_DELETE;
        }
        else if ( isArgOfType(argv[i], "-b") )
        {
            mode_flags &= ~MODE_FLAG_CONTINUOUS_PRINTING;
        }
        else if ( isArgOfType(argv[i], "-lpx") )
        {
            process_list_flags |= PROCESS_LIST_MEMORY;
        }
        else if ( isArgOfType(argv[i], "-lpm") )
        {
            process_list_flags |= PROCESS_LIST_MODULES;
        }
        else if ( isArgOfType(argv[i], "-lpt") )
        {
            process_list_flags |= PROCESS_LIST_THREADS;
        }
        else if ( isArgOfType(argv[i], "-lph") )
        {
            process_list_flags |= PROCESS_LIST_HEAPS;
        }
        else if ( isArgOfType(argv[i], "-lphb") )
        {
            process_list_flags |= PROCESS_LIST_HEAP_BLOCKS;
        }
        else if ( isArgOfType(argv[i], "-lrp") )
        {
            process_list_flags |= PROCESS_LIST_RUNNING_PROCESSES;
        }
        else if ( isArgOfType(argv[i], "-file") )
        {
            if ( hasValue("-file", i, end_i) )
            {
                source = argv[i + 1];
                run_mode = RUN_MODE_FILE;
                i++;
            }
        }
        else if ( isArgOfType(argv[i], "-pid") )
        {
            if ( hasValue("-pid", i, end_i) )
            {
                source = argv[i + 1];
                run_mode = RUN_MODE_PID;
                i++;
            }
        }
        else if ( isArgOfType(argv[i], "-s") )
        {
            if ( hasValue("-s", i, end_i) )
            {
                s = parseSizeAuto(argv[i + 1], &start);
                if ( s != 0 )
                {
                    printf("INFO: Could not parse start. Setting it to %u!\n", 0);
                    start = 0x00;
                }
                i++;
            }
        }
        else if ( isArgOfType(argv[i], "-l") )
        {
            if ( hasValue("-l", i, end_i) )
            {
                s = parseSizeAuto(argv[i + 1], &length);
                if ( s != 0 )
                {
                    printf("INFO: Could not parse length. Setting it to 0x%x!\n", DEFAULT_LENGTH);
                    length = DEFAULT_LENGTH;
                }
                else
                    length_found = 1;
                i++;
            }
        }
        else if ( isFormatArgOfType(argv[i], "-i") )
        {
            if ( hasValue("-i", i, end_i))
            {
                mode_flags |= MODE_FLAG_INSERT;
                payload_arg_id = i;
                i++;
            }
        }
        else if ( isFormatArgOfType(argv[i], "-o") )
        {
            if ( hasValue("-o", i, end_i))
            {
                mode_flags |= MODE_FLAG_OVERWRITE;
                payload_arg_id = i;
                i++;
            }
        }
        else if ( isFormatArgOfType(argv[i], "-f") )
        {
            if ( hasValue("-f", i, end_i))
            {
                mode_flags |= MODE_FLAG_FIND;
                payload_arg_id = i;
                i++;
            }
        }
        else if ( isArgOfType(argv[i], "-ci") )
        {
            mode_flags |= MODE_FLAG_CASE_INSENSITIVE;
        }
        else
        {
            printf("INFO: Unknown arg type \"%s\"\n", argv[i]);
        }
    }

    if ( run_mode == RUN_MODE_NONE )
    {
//      printf("ERROR: You have to specify either a -file or a -pid!\n");
        printUsage();
        return -1;
    }
    
    
    uint32_t f = mode_flags&(MODE_FLAG_FIND|MODE_FLAG_OVERWRITE|MODE_FLAG_INSERT|MODE_FLAG_DELETE);
    if ( (f & (f-1)) != 0 )
    {
        EPrint("Overwrite, insert, delete and find have to be used exclusively!\n");
        return -2;
    }

    
    f = print_col_mask&(PRINT_UNICODE_MASK|PRINT_ASCII_MASK);
    if ( (f & (f-1)) != 0 )
    {
        EPrint("Ascii and unicode printing can't be combined!\n");
        return -5;
    }
    if ( print_col_mask == PRINT_OFFSET_MASK )
    {
        EPrint("Printing only offsets is not provided! Please select one or more of -pa, -pu, -px.\n");
        return -6;
    }


    if ( (mode_flags&MODE_FLAG_DELETE) && !length_found )
    {
        EPrint("Could not parse length of part to delete! Pass -l 0, if you want to delete from -s to the end of file.\n");
        return -3;
    }

    if ( run_mode == RUN_MODE_PID && (mode_flags&(MODE_FLAG_INSERT|MODE_FLAG_DELETE)) > 0 )
    {
        EPrint("Inserting or deleting is not supported in process mode!\n");
        return -4;
    }

    if ( run_mode == RUN_MODE_FILE )
    {
        s = expandFilePath(source, file_path);
        if ( s != 0 )
            return s;
    }    
    else
        snprintf(file_path, PATH_MAX, "%s", source);
    
    return 0;
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

uint8_t isFormatArgOfType(char* arg, char* type)
{
    uint8_t i, j;
    uint8_t arg_ln = (uint8_t)strnlen(arg, 10);
    uint8_t type_ln = (uint8_t)strnlen(type, 10);

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
            return 1;

    return 0;
}

uint8_t hasValue(char* type, int i, int end_i)
{
    if ( i >= end_i )
    {
        printf("INFO: Arg \"%s\" has no value! Skipped!\n", type);
        return 0;
    }

    return 1;
}

int sanitizeDeleteParams()
{
    if ( !(mode_flags&MODE_FLAG_DELETE) )
        return 0;
    
    uint8_t info_line_break = 0;

    if ( start >= file_size )
    {
        fprintf(stderr, "ERROR: Start offset 0x%zx is greater the the file size 0x%zx (%zu)!\n",
                start, file_size, file_size);
        return 1;
    }
    
    if ( length == 0 )
    {
        fprintf(stdout, "Info: Length is 0. Setting to end of file 0x%zx!\n", 
            file_size - start);
        length = file_size - start;
        info_line_break = 1;
    }
    
    if ( start + length > file_size )
    {
        length = file_size - start;
    }

    if ( info_line_break )
        printf("\n");
    
    return 0;
}

int sanitizePrintParams(uint32_t pid)
{
    uint8_t col_size;
    uint8_t info_line_break = 0;

    if ( print_col_mask == 0 )
        print_col_mask = (PRINT_OFFSET_MASK | PRINT_ASCII_MASK | PRINT_HEX_MASK);

    if ( mode_flags&(MODE_FLAG_INSERT|MODE_FLAG_OVERWRITE|MODE_FLAG_DELETE) )
        mode_flags &= ~MODE_FLAG_CONTINUOUS_PRINTING;

    col_size = getColSize();
    if ( col_size == 0 )
    {
        printf("ERROR: col size error!\n");
        return -1;
    }

    // normalize length to block size for continuous printing
    if ( (mode_flags&MODE_FLAG_CONTINUOUS_PRINTING)
        && print_col_mask != PRINT_BYTES_STRING )
    {
        if ( length % col_size != 0 )
        {
            length = length + col_size - (length % col_size);
            printf("INFO: Normalized length to 0x%zx\n", length);
        }
    }

    // check start offset
    if ( run_mode == RUN_MODE_FILE )
        info_line_break = keepStartInFile();
    else if ( run_mode == RUN_MODE_PID )
        info_line_break = makeStartHitAccessableMemory(pid, &start);

    // normalize start offset to block size
    // called after insert and overwrite
    if ( !(mode_flags&(MODE_FLAG_FIND|MODE_FLAG_DELETE)) )
    {
        start = normalizeOffset(start, &skip_bytes);
        if ( !(mode_flags&MODE_FLAG_CONTINUOUS_PRINTING) )
            length += skip_bytes;
    }

    // check length
    if ( run_mode == RUN_MODE_FILE )
        info_line_break = keepLengthInFile();
//  else if ( type == RUN_MODE_PID )
//      info_line_break = keepLengthInModule(pid);

    if ( length == 0 )
    {
        printf("Info: Length is 0. Setting to 0x%x!\n", DEFAULT_LENGTH);
        length = DEFAULT_LENGTH;
        info_line_break = 1;
    }

    if ( info_line_break )
        printf("\n");

    return 0;
}

uint8_t keepStartInFile()
{
    if ( start >= file_size )
    {
        printf("Info: Start offset 0x%zx is greater the the file size 0x%zx (%zu)!\nSetting to 0!\n", start, file_size, file_size);
        start = 0;
        return 1;
    }
    return 0;
}

uint8_t keepLengthInFile()
{
    if ( start + length > file_size )
    {
        //printf("Info: Start offset 0x%zx plus length 0x%zx is greater then the file size 0x%zx\n"
        //    "Printing only to file size.\n",
        //start + skip_bytes, (continuous_f) ? length : length - skip_bytes, file_size);

        length = file_size - start;
        mode_flags &= ~MODE_FLAG_CONTINUOUS_PRINTING;
        return 0;
    }
    return 0;
}

/**
 * Parse payload from
 * 
 * @param format char the format of the raw payload string
 * @param value char* the raw payload value
 * @param payload char** the array to store the formated payload in
 * @return uint32_t length of parsed payload.
 */
uint32_t parsePayload(const char format, const char* value, uint8_t** payload)
{
    uint32_t ln = 0;

    if ( strnlen(value, MAX_PAYLOAD_LN) < 1 )
    {
        printf("ERROR: Payload greater max payload size of 0x%x)!\n", MAX_PAYLOAD_LN);
        return 0;
    }
    if ( format == FORMAT_BYTE )
    {
        ln = payloadParseByte(value, payload);
    }
    else if ( format == FORMAT_FILL_BYTE )
    {
        if ( length > MAX_PAYLOAD_LN )
        {
            printf("INFO: Fill byte length is greater than 0x%x (%u). Setting to 0x%x (%u)!\n", MAX_PAYLOAD_LN, MAX_PAYLOAD_LN, MAX_PAYLOAD_LN, MAX_PAYLOAD_LN);
            length = MAX_PAYLOAD_LN;
        }
        ln = payloadParseFillBytes(value, payload, length);
        length = DEFAULT_LENGTH;
    }
    else if ( format == FORMAT_WORD )
        ln = payloadParseWord(value, payload);
    else if ( format == FORMAT_D_WORD )
        ln = payloadParseDWord(value, payload);
    else if ( format == FORMAT_Q_WORD )
        ln = payloadParseQWord(value, payload);
    else if ( format == FORMAT_ASCII )
        ln = payloadParseUtf8(value, payload);
    else if ( format == FORMAT_UNICODE )
        ln = payloadParseUtf16(value, payload);
//  else if ( format == 'r' )
//      ln = payloadParseReversedPlainBytes(arg, payload);
    else if (format == FORMAT_PLAIN_HEX_1 || format == FORMAT_PLAIN_HEX_2)
    {
        char* cleaned_value = NULL;
        int s = cleanBytes(value, &cleaned_value);
        if ( s != 0 )
            return 0;
        ln = payloadParsePlainBytes(cleaned_value, payload);
        free(cleaned_value);
    }
    else
    {
        printf("ERROR: %c is not a supported format!\n", format);
        ln = 0;
    }

    return ln;
}

/**
 * Library function to print a file (-t file).
 *
 * @param _file_name
 * @param _start
 * @param _length
 * @return int status info
 */
HEXTER_API int hexter_printFile(const char* _file_name, size_t _start, size_t _length)
{
    initParameters();
    int s = expandFilePath(_file_name, file_path);
    if ( s != 0 )
        return s;

    run_mode = RUN_MODE_FILE;
    start = _start;
    length = _length;
    mode_flags &= ~MODE_FLAG_CONTINUOUS_PRINTING;

//  print_col_mask = print_col_mask | PRINT_HEX_MASK;

    run(0, NULL);

    return 0;
}

/**
 * Library function to print a process (-t pid).
 *
 * @param _pid
 * @param _start
 * @param _length
 * @param flags
 * @return int status info
 */
HEXTER_API int hexter_printProcess(uint32_t _pid, size_t _start, size_t _length, uint32_t flags)
{
    initParameters();
#ifdef _WIN32
    snprintf(file_path, PATH_MAX, "%u", _pid);
#else
    snprintf(file_path, PATH_MAX, "%u", _pid);
#endif

    run_mode = RUN_MODE_PID;
    start = _start;
    length = _length;
    mode_flags &= ~MODE_FLAG_CONTINUOUS_PRINTING;

    process_list_flags = flags;

    run(0, NULL);

    return 0;
}

/**
 * Intended to be called by rundll32.
 * Usage: rundll32 hexter.dll,runHexter hexter params
 * Example: rundll32 hexter.dll,runHexter hexter -pid 0 -lrp
 * The "hexter" param is a dummy param and the rest of the params should be used as explained in normal usage:
 * -file|-pid xxx [options]
 * With the "hexter" dummy param, the splitted arguments may be passed to the main function.
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

    debug_info("the param cmd line: %s\n", lpszCmdLine);

    uint8_t argv_max = 20;
    uint8_t argc;
    char* argv[20];
    argc = (uint8_t)splitArgs(lpszCmdLine, argv, argv_max);

#ifdef DEBUG_PRINT
    int i;
    debug_info("argc: %u\n", argc);
    for ( i = 0; i < argc; i++ )
        debug_info("arg%d: %s\n", i, argv[i]);
#endif

    main(argc, argv);
    getchar();
}

#endif
