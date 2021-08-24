#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "../src/hexter.h"

#pragma warning( disable : 4189 )

#ifdef _DEBUG
#ifdef _WIN64
#pragma comment(lib, "..\\build\\debug\\64\\Hexter.lib")
#else
#pragma comment(lib, "..\\build\\debug\\32\\Hexter.lib")
#endif
#else
#ifdef _WIN64
#pragma comment(lib, "..\\build\\64\\Hexter.lib")
#else
#pragma comment(lib, "..\\build\\32\\Hexter.lib")
#endif
#endif

int main(int argc, char** argv)
{
    char* path = NULL;
    char* cmd_line = NULL;
    uint32_t pid = 0;
    uint32_t flags = 0;
    uint32_t type = 0;
    size_t start = 0;
    size_t length = 0x100;

    if ( argc < 3 )
    {
        printf("Usage: test_hexter_dll type path|pid|cmdline -s start -l length -f pflags\n");
        printf("\n");
        printf("type: 1 = file, 2 = process, 3 = cmdline\n");
        printf("pflags: PROCESS_LIST_MEMORY (0x01), PROCESS_LIST_MODULES (0x02), PROCESS_LIST_THREADS (0x04), PROCESS_LIST_HEAPS (0x08), PROCESS_LIST_HEAP_BLOCKS (0x10), PROCESS_LIST_RUNNING_PROCESSES (0x20)\n");
        
        return 0;
    }

    int i;
    char* arg = NULL;
    char* val = NULL;
    if ( argc > 3 )
    {
        for ( i = 3; i < argc; i++ )
        {
            arg = argv[i];
            val = ( i < argc - 1 ) ? argv[i+1] : NULL;

            if ( arg == NULL )
                break;
            if ( arg[0] != '-' && arg[0] != '/' )
                continue;

            if ( arg[1] == 's' && arg[2] == 0 )
            {
                if ( val == NULL )
                    continue;

                start = strtoul(val, NULL, 0);
            }
            else if ( arg[1] == 'l' && arg[2] == 0 )
            {
                if ( val == NULL )
                    continue;

                length = strtoul(val, NULL, 0);
            }
            else if ( arg[1] == 'f' && arg[2] == 0 )
            {
                if ( val == NULL )
                    continue;

                flags = strtoul(val, NULL, 0);
            }
        }
    }

    type = strtoul(argv[1], NULL, 0);
    printf("type: 0x%x\n", type);
    if ( type == 1 )
    {
        path = argv[2];
        printf("path: %s\n", path);
        hexter_printFile(path, start, length);
    }
    else if ( type == 2 )
    {
        pid = strtoul(argv[2], NULL, 0);
        printf("pid: 0x%x\n", pid);
        hexter_printProcess(pid, start, length, flags);
    }
    else if ( type == 3 )
    {
        cmd_line = argv[2];
        printf("cmd_line: %s\n", cmd_line);
        runHexter(NULL, NULL, cmd_line, 0);
    }
    else
    {
        printf("Unknown type\n");
        return 0;
    }

    return 0;
}