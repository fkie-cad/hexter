#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "common_fileio.h"

//static int errsv;

// Get file size.
// Returns actual size in bytes.
size_t getSize(const char* finame)
{
    int errsv;
    int s;
    FILE* fi;
    size_t pos = 0, Filesize = 0;
    errno = 0;

    fi = fopen(finame, "rb");
    errsv = errno;
    if ( !fi )
    {
#ifdef ERROR_PRINT
        printf("ERROR (0x%x): Could not open \"%s\".\n", errsv, finame);
#endif
        return 0;
    }

    pos = ftell(fi);
    errno = 0;
    s = fseek(fi, 0, SEEK_END);
    errsv = errno;
    if ( s != 0 )
    {
#ifdef ERROR_PRINT
        printf("ERROR (0x%x): FSeek in \"%s\".\n", errsv, finame);
#endif
        Filesize = 0;
        goto clean;
    }
    errno = 0;
    Filesize = ftell(fi);
    errsv = errno;
    if ( errsv != 0 )
    {
#ifdef ERROR_PRINT
        printf("ERROR (0x%x): FTell in \"%s\".\n", errsv, finame);
#endif
        if ( errsv == 0x16 )
        {
#ifdef ERROR_PRINT
            printf("The file may be too big.\n");
#endif
        }
        Filesize = 0;
    }
    errno = 0;
    fseek(fi, pos, SEEK_SET);
    errsv = errno;

    clean:
    fclose(fi);

    return Filesize;
}

size_t readFile(FILE* fi, size_t begin, size_t size, unsigned char* data, int* errsv)
{
    size_t n = 0;

    fseek(fi, begin, SEEK_SET);
    errno = 0;
    n = fread(data, 1, size, fi);
    *errsv = errno;

    return n;
}

//int cfio_getErrno()
//{
//    int errsv;
//    return errsv;
//}
