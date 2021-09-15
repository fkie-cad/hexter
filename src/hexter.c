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
    #include "ProcessHandlerWin.h"
    #include "utils/Strings.h"
#endif

#define BIN_NAME ("hexter")
#define BIN_VS "1.5.27"
#define BIN_LAST_CHANGED  "20.08.2021"

size_t file_size;
char file_path[PATH_MAX];
static size_t start;
size_t length;
uint8_t clean_printing;
static uint8_t skip_bytes;

uint8_t print_col_mask;

static uint8_t insert_f;
static uint8_t overwrite_f;
uint8_t find_f;
static uint8_t delete_f;
uint8_t continuous_f;

uint32_t process_list_flags;

typedef enum RunMode { RUN_MODE_NONE, RUN_MODE_FILE, RUN_MODE_PID } RunMode;
static RunMode run_mode;

static int payload_arg_id;


#define FORMAT_ASCII ('a')
#define FORMAT_UNICODE ('u')
#define FORMAT_BYTE ('b')
#define FORMAT_WORD ('w')
#define FORMAT_D_WORD ('d')
#define FORMAT_Q_WORD ('q')
#define FORMAT_PLAIN_HEX ('h')
#define FORMAT_FILL_BYTE ('f')

static const char format_types[8] = { FORMAT_ASCII, FORMAT_UNICODE, FORMAT_BYTE, FORMAT_FILL_BYTE, FORMAT_WORD, FORMAT_D_WORD, FORMAT_Q_WORD, FORMAT_PLAIN_HEX };
static const uint8_t format_types_ln = 8;

static void printUsage();
void printHelp();
static void initParameters();
static int parseArgs(int argc, char** argv);
static uint8_t isArgOfType(char* arg, char* type);
static uint8_t isFormatArgOfType(char* arg, char* type);
static uint8_t hasValue(char* type, int i, int end_i);
static int sanitizeDeleteParams();
static void sanitizePrintParams(uint32_t pid);
static uint32_t parsePayload(const char format, const char* value, unsigned char** payload);

static int run(const char payload_format, const char* raw_payload);
void cleanUp(unsigned char* payload);

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

    if ( isArgOfType(argv[1], "-h") )
    {
        printHelp();
        return 0;
    }

    initParameters();
    s = parseArgs(argc, argv);
    if ( s != 0 )
        return -2;

    if ( payload_arg_id > 0 )
        run(argv[payload_arg_id][2], argv[payload_arg_id + 1]);
    else
        run(0, NULL);

    return 0;
}

int run(const char payload_format, const char* raw_payload)
{
    uint32_t pid = 0;
    int s;
    unsigned char* payload = NULL;
    uint32_t payload_ln = 0;
    char* file_name = NULL;

    if ( run_mode == RUN_MODE_FILE )
    {
        file_size = getSize(file_path);
        if ( file_size == 0 )
            return 0;
    }
    else if ( run_mode == RUN_MODE_PID )
    {
        s = parseUint32Auto(file_path, &pid);
        if ( s != 0 )
            return 0;

        if ( pid == 0 )
#if defined(__linux__) || defined(__linux) || defined(linux)
            pid = getpid();
#elif defined(_WIN32)
            pid = _getpid();
#endif
        file_size = getSizeOfProcess(pid);
        if ( file_size == 0 )
            return 2;
    }

    debug_info("file_path: %s\n", file_path);
    debug_info("file_size: 0x%zx\n", file_size);
    debug_info("start: 0x%zx\n", start);
    debug_info("length: 0x%zx\n", length);
    debug_info("print_col_mask only: %d\n", print_col_mask);
    debug_info("insert: %d\n", insert_f);
    debug_info("overwrite: %d\n", overwrite_f);
    debug_info("find: %d\n", find_f);
    debug_info("delete: %d\n", delete_f);
    debug_info("\n");

    if ( (insert_f || overwrite_f || find_f) && payload_format > 0 )
    {
        payload_ln = parsePayload(payload_format, raw_payload, &payload);
        if ( payload == NULL)
            return 3;
    }

    if ( insert_f )
    {
        insert(file_path, payload, payload_ln, start);
        file_size = getSize(file_path);
    }
    else if ( overwrite_f && run_mode == RUN_MODE_FILE )
    {
        overwrite(file_path, payload, payload_ln, start);
        file_size = getSize(file_path);
    }
    else if ( overwrite_f && run_mode == RUN_MODE_PID )
    {
        writeProcessMemory(pid, payload, payload_ln, start);
    }

    if ( delete_f )
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

    sanitizePrintParams(pid);
    setPrintingStyle();
    if ( run_mode == RUN_MODE_FILE )
    {
        // recalculate file size, it may has changed due to reading or deleting
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

void cleanUp(unsigned char* payload)
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

    continuous_f = 1;
    insert_f = 0;
    overwrite_f = 0;
    find_f = 0;
    delete_f = 0;
    payload_arg_id = -1;

    process_list_flags = 0;

    clean_printing = 0;

    print_col_mask = 0;

    run_mode = RUN_MODE_NONE;
}

void printUsage()
{
    printf("Usage: %s [options] -file a/file [options]\n", BIN_NAME);
    printf("Usage: %s [options] -pid 123 [options]\n", BIN_NAME);
    printf("\n");
    printf("Version: %s\n", BIN_VS);
    printf("Last changed: %s\n", BIN_LAST_CHANGED);
}

void printHelp()
{
    printUsage();
    printf("\n");
    printf("Options:\n");
    printf(" * -file:string A file name to show the hex source of.\n"
           " * -pid:size_t A process id (in hex or dec) to show the hex source of. Pass 0 for your own process.\n"
           " * -s:size_t Startoffset. Default = 0.\n"
           " * -l:size_t Length of the part to display. Default = 0x100.\n"
           " * -a ASCII only print.\n"
           " * -x HEX only print.\n"
           " * -ix Insert hex byte sequence (destructive!). Where x is an format option. (File mode only.)\n"
           " * -ox Overwrite hex byte sequence (destructive!). Where x is an format option.\n"
           " * -fx Find hex byte sequence. Where x is an format option.\n"
           " * Format options:\n"
           "   * %c: plain bytes\n"
           "   * %c: ascii/utf-8 text\n"
           "   * %c: unicode (windows utf16) text.\n"
           "   * %c: byte\n"
           "   * %c: fill byte (will be inserted -l times)\n"
           "   * %c: word\n"
           "   * %c: double word\n"
           "   * %c: quad word.\n"
           "   Expect for the string types, all values have to be passed as hex values, omitting `0x`.\n"
//		   " * -e:uint8_t Endianess of payload (little: 1, big:2). Defaults to 1 = little endian.\n"
           " * -d Delete -l bytes from offset -s. (File mode only.). Pass -l 0 to delete from -s to file end.\n"
           " * -pid only options:\n"
           "   * -lpx List entire process memory layout.\n"
           "   * -lpm List all process modules.\n"
           "   * -lpt List all process threads.\n"
           "   * -lph List all process heaps.\n"
           "   * -lphb List all process heaps and its blocks.\n"
           "   * -lrp List all running processes. Pass any pid or 0 to get it running.\n"
           " * -b Force breaking, not continuous mode.\n"
           " * -p Plain, not styled text output.\n"
           " * -h Print this.\n",
           FORMAT_PLAIN_HEX, FORMAT_ASCII, FORMAT_UNICODE, FORMAT_BYTE, FORMAT_FILL_BYTE, FORMAT_WORD, FORMAT_D_WORD, FORMAT_Q_WORD
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
        if ( argv[i][0] != '-' )
            break;

        if ( isArgOfType(argv[i], "-x") )
        {
            print_col_mask = print_col_mask | PRINT_HEX_MASK;
        }
        else if ( isArgOfType(argv[i], "-a") )
        {
            print_col_mask = print_col_mask | PRINT_ASCII_MASK;
        }
        else if ( isArgOfType(argv[i], "-p") )
        {
            clean_printing = 1;
        }
        else if ( isArgOfType(argv[i], "-d") )
        {
            delete_f = 1;
        }
        else if ( isArgOfType(argv[i], "-b") )
        {
            continuous_f = 0;
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
                insert_f = 1;
                payload_arg_id = i;
                i++;
            }
        }
        else if ( isFormatArgOfType(argv[i], "-o") )
        {
            if ( hasValue("-o", i, end_i))
            {
                overwrite_f = 1;
                payload_arg_id = i;
                i++;
            }
        }
        else if ( isFormatArgOfType(argv[i], "-f") )
        {
            if ( hasValue("-f", i, end_i))
            {
                find_f = 1;
                payload_arg_id = i;
                i++;
            }
        }
        else
        {
            printf("INFO: Unknown arg type \"%s\"\n", argv[i]);
        }
    }

    if ( run_mode == RUN_MODE_NONE )
    {
//		printf("ERROR: You have to specify either a -file or a -pid!\n");
        printUsage();
        return 1;
    }

    if ( (find_f + overwrite_f + insert_f + delete_f) > 1 )
    {
        printf("ERROR: overwrite, insert, delete and find have to be used exclusively!\n");
        return 2;
    }

    if ( delete_f && !length_found )
    {
        printf("ERROR: could not parse length of part to delete!\n");
        return 3;
    }

    if ( run_mode == RUN_MODE_PID && (insert_f + delete_f) > 0 )
    {
        printf("ERROR: Inserting or deleting is not supported in process mode!\n");
        return 4;
    }

    if ( run_mode == RUN_MODE_FILE )
        expandFilePath(source, file_path);
    else
        snprintf(file_path, PATH_MAX, "%s", source);
    
    return 0;
}

uint8_t isArgOfType(char* arg, char* type)
{
    uint8_t type_ln = (uint8_t)strnlen(type, 10);
    return strnlen(arg, 10) == type_ln && strncmp(arg, type, type_ln) == 0;
}

uint8_t isFormatArgOfType(char* arg, char* type)
{
    uint8_t i, j;
    uint8_t arg_ln = (uint8_t)strnlen(arg, 10);
    uint8_t type_ln = (uint8_t)strnlen(type, 10);

    if ( arg_ln <= type_ln )
        return 0;

    for ( i = 0; i < type_ln; i++ )
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
    if ( !delete_f )
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

void sanitizePrintParams(uint32_t pid)
{
    uint8_t col_size;

    if ( print_col_mask == 0 )
        print_col_mask = (PRINT_OFFSET_MASK | PRINT_ASCII_MASK | PRINT_HEX_MASK);

    if ( insert_f || overwrite_f || delete_f )
        continuous_f = 0;

    col_size = getColSize();

    // normalize length to block size for continuous printing
    if ( continuous_f )
    {
        if ( length % col_size != 0 )
        {
            length = length + col_size - (length % col_size);
            printf("INFO: Normalized length to 0x%zx\n", length);
        }
    }

    uint8_t info_line_break = 0;
    // check start offset
    if ( run_mode == RUN_MODE_FILE )
        info_line_break = keepStartInFile();
    else if ( run_mode == RUN_MODE_PID )
        info_line_break = makeStartHitAccessableMemory(pid, &start);

    // normalize start offset to block size
    // called after insert and overwrite
    if ( !find_f && !delete_f )
    {
        start = normalizeOffset(start, &skip_bytes);
        if ( !continuous_f )
            length += skip_bytes;
    }

    // check length
    if ( run_mode == RUN_MODE_FILE )
        info_line_break = keepLengthInFile();
//	else if ( type == RUN_MODE_PID )
//		info_line_break = keepLengthInModule(pid);

    if ( length == 0 )
    {
        printf("Info: Length is 0. Setting to 0x%x!\n", DEFAULT_LENGTH);
        length = DEFAULT_LENGTH;
        info_line_break = 1;
    }

    if ( info_line_break )
        printf("\n");
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
        continuous_f = 0;
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
uint32_t parsePayload(const char format, const char* value, unsigned char** payload)
{
    uint32_t ln = 0;

    if ( strnlen(value, MAX_PAYLOAD_LN) < 1 )
    {
        printf("ERROR: Payload greater max payload size of 0x%x)!\n", MAX_PAYLOAD_LN);
        return 0;
    }
    if ( format == FORMAT_BYTE )
        ln = payloadParseByte(value, payload);
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
//	else if ( format == 'r' )
//		ln = payloadParseReversedPlainBytes(arg, payload);
    else if ( format == FORMAT_PLAIN_HEX )
        ln = payloadParsePlainBytes(value, payload);
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
    expandFilePath(_file_name, file_path);

    run_mode = RUN_MODE_FILE;
    start = _start;
    length = _length;
    continuous_f = false;

//	print_col_mask = print_col_mask | PRINT_HEX_MASK;

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
    continuous_f = false;

    process_list_flags = flags;

//	print_col_mask = print_col_mask | PRINT_HEX_MASK;

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
//	AttachConsole(GetCurrentProcessId());
//	AttachConsole(-1);
//	AttachConsole(_getpid());
    freopen("conin$", "r", stdin);
    freopen("conout$", "w", stdout);
    freopen("conout$", "w", stderr);

    (void) hwnd;
    (void) hinst;
    (void) nCmdShow;

    debug_info("the param cmd line: %s\n", lpszCmdLine);

    int i;
    uint8_t argv_max = 20;
    uint8_t argc;
    char* argv[20];
    argc = (uint8_t)splitArgs(lpszCmdLine, argv, argv_max);

    debug_info("argc: %u\n", argc);
    for ( i = 0; i < argc; i++ )
        debug_info("arg%d: %s\n", i, argv[i]);

    main(argc, argv);
    getchar();
}

#endif
