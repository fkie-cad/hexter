#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
//#include <stdlib.h>
#include <string.h>

#include "utils/env.h"

#if defined(_LINUX) || defined(__APPLE__)
    #include <unistd.h>
#elif defined(_WIN32)
    #include <io.h>
#endif

//#include "utils/common_fileio.h"
//#include "utils/Strings.h"
#include "Writer.h"
#include "Globals.h"
//#include "utils/Converter.h"

static int truncateFile(FILE* fp, size_t file_size, size_t ln);



/**
 * Insert payload into file.
 *
 * @param file_path
 * @param payload
 * @param payload_ln
 * @param offset
 */
int insert(const char* path, uint8_t* payload, uint32_t payload_ln, size_t offset)
{
    FEnter();

    uint8_t* buffer = NULL;
    size_t buffer_size = 0;
    size_t bytes_read = 0;
    FILE* fp = NULL;
    size_t i, j;
    size_t written;
    size_t to_write;
    int errsv = 0;
    int s = 0;

    // insertion is just complicated in the middle of a file
    // just write after the end of a file, if offset is bigger than file size
    if ( offset > file_size )
    {
        s = overwrite(path, payload, payload_ln, offset);
        FLeave();
        return s;
    }

    buffer_size = max(BLOCK_SIZE, payload_ln);
    buffer = malloc(buffer_size);
    if ( !buffer )
    {
        s = errno;
        EPrint("No memory for buffer of size 0x%zx! (0y%x)\n", buffer_size, s);
        goto clean;
    }
    bytes_read = buffer_size;

    // "ab+" results in strange behaviour
    // that's why this construct is used
    //errno = 0;
    fp = fopen(path, "rb+");
    //int errsv = errno;
    if ( !fp )
    {
        errno = 0;
        fp = fopen(path, "wb+");
        errsv = errno;
        if ( !fp )
        {
            s = errsv;
            EPrint("Could not open \"%s\"! (0x%x)\n", path, s);
            goto clean;
        }
    }

    // read the file in buffer_size chunks
    while ( bytes_read == buffer_size )
    {
        // jump to insert offset
        s = fseek(fp, offset, SEEK_SET);
        errsv = errno;
        if ( s != 0 )
        {
            s = errsv;
            EPrint("FSeek in \"%s\"! (0x%x)\n", path, errsv);
            goto clean;
        }

        // read a block original data into buf
        bytes_read = fread(buffer, 1, buffer_size, fp);
        errsv = errno;
        if ( ferror(fp) )
        {
            s = errsv;
            EPrint("fread in \"%s\"! (0x%x)\n", path, s);
            goto clean;
        }

        // jump back to insert offset
        s = fseek(fp, offset, SEEK_SET);        // f: .....0123456789ABCDEF, buf = 0123456789ABCDEF, payload = DEAD0BEA
        errsv = errno;
        if ( s != 0 )
        {
            s = errsv;
            EPrint("FSeek in \"%s\"! (0x%x)\n", path, errsv);
            goto clean;
        }

        // write payload overwriting the original data
        written = fwrite(payload, 1, payload_ln, fp); // f: .....DEAD0BEA89ABCDEF, buf = 0123456789ABCDEF, payload = DEAD0BEA
        errsv = errno;
        if ( ferror(fp) )
        {
            s = errsv;
            EPrint("fwrite in \"%s\"! (0x%x)\n", path, errsv);
            goto clean;
        }

        if ( bytes_read > payload_ln )
        {
            // since payload did'nt overwrite the whole current window
            // write first fitting original bytes from read buffer into remaing of the current window
            to_write = bytes_read-payload_ln;
            written = fwrite(buffer, 1, to_write, fp);   // f: .....DEAD0BEA01234567, buf = 0123456789ABCDEF, payload = DEAD0BEA
            errsv = errno;
            if ( ferror(fp) )
            {
                s = errsv;
                EPrint("fwrite in \"%s\"! (0x%x)\n", path, errsv);
                goto clean;
            }

            // fill payload with original bytes from read buffer
            // payload_ln >= bytes_read - to_write <=> bytes_read - (bytes_read-payload_ln) <=> payload_ln
            for ( i = to_write, j=0; i < bytes_read; i++, j++ )
            {
                payload[j] = buffer[i]; // , buf = 0123456789ABCDEF, payload = 89ABCDEF
            }
        }
        else // bytes_read <= payload_ln
        {
            // fill whole payload with original bytes from read buffer
            for ( i = 0; i < bytes_read; i++ )
            {
                payload[i] = buffer[i];
            }
        }
        
        if ( bytes_read > payload_ln )
            offset += bytes_read;
        else  // bytes_read <= payload_ln
            offset += payload_ln;
    }
    if ( bytes_read > payload_ln )
    {
        DPrint("bytes_read > payload_ln\n");
        DPrint("  offset: 0x%zx\n", offset);
        s = fseek(fp, offset, SEEK_SET);
        errsv = errno;
        if ( s != 0 )
        {
            s = errsv;
            EPrint("FSeek in \"%s\"! (0x%x)\n", path, errsv);
            goto clean;
        }
        
        DPrint("  payload_ln: 0x%x\n", payload_ln);
        DPrint("  bytes_read: 0x%zx\n", bytes_read);
        written = fwrite(payload, 1, payload_ln, fp);
        errsv = errno;
        if ( ferror(fp) )
        {
            s = errsv;
            EPrint("fwrite in \"%s\"! (0x%x)\n", path, errsv);
            goto clean;
        }
    }
    else // bytes_read <= payload_ln
    {
        DPrint("bytes_read <= payload_ln\n");
        DPrint("  offset: 0x%zx\n", offset);
        s = fseek(fp, offset, SEEK_SET);
        errsv = errno;
        if ( s != 0 )
        {
            s = errsv;
            EPrint("FSeek in \"%s\"! (0x%x)\n", path, errsv);
            goto clean;
        }
        
        DPrint("  payload_ln: 0x%x\n", payload_ln);
        DPrint("  bytes_read: 0x%zx\n", bytes_read);
        written = fwrite(payload, 1, bytes_read, fp);
        errsv = errno;
        if ( ferror(fp) )
        {
            s = errsv;
            EPrint("fwrite in \"%s\"! (0x%x)\n", path, errsv);
            goto clean;
        }
    }

clean:
    if ( fp )
        fclose(fp);
    if ( buffer )
        free(buffer);

    FLeave();
    return s;
}

/**
 * Overwrite bytes in file with payload.
 *
 * @param    file_path char*
 * @param    payload uint8_t* the bytes to write
 * @param    payload_ln uint32_t the ln of the bytes to write
 * @param    offset size_t the offset to write the bytes at
 */
int overwrite(const char* path, uint8_t* payload, uint32_t payload_ln, size_t offset)
{
    FILE* fp = NULL;
    int s = 0;
    size_t written;

    // backup
//    FILE* bck;
//    char buf[1024];
//    int buf_ln = 1024;
//    char dest_file_name[128];
//    getTempFile(dest_file_name, "hexter.bck");
//    int n = buf_ln;
    // end backup

    errno = 0;
    fp = fopen(path, "rb+");
    int errsv = errno;
    if ( !fp )
    {
        s = errsv;
        EPrint("Could not open \"%s\"! (0x%x)\n", path, s);
        return s;
    }
    // backup
//    bck = fopen(dest_file_name, "wb");
//    if ( !bck )
//    {
//        printf("File %s could not be created.\n", dest_file_name);
//        return;
//    }
//
//    while ( n == buf_ln )
//    {
//        n = fread(buf, 1, buf_ln, src);
//        fwrite(buf, 1, n, bck);
//    }
//    fclose(bck);
    // end backup

    s = fseek(fp, offset, SEEK_SET);
    errsv = errno;
    if ( s != 0 )
    {
        s = errsv;
        EPrint("FSeek in \"%s\"! (0x%x)\n", path, errsv);
        goto clean;
    }

    written = fwrite(payload, 1, payload_ln, fp);
    errsv = errno;
    if ( ferror(fp) )
    {
        s = errsv;
        EPrint("fwrite in \"%s\"! (0x%x)\n", path, errsv);
        goto clean;
    }

clean:
    if ( fp )
        fclose(fp);

    return s;
}

/**
 * Delete bytes in file of the passed length.
 *
 * @param path
 * @param start size_t start offset of the deletion.
 * @param ln size_t ln of the bytes to delete.
 */
int deleteBytes(const char* path, size_t start, size_t ln)
{
    uint8_t buf[BLOCK_SIZE];
    size_t n = BLOCK_SIZE;
    FILE* fp = NULL;
    size_t offset;
    size_t end;
    size_t written;
    int s = 0;

    if ( start > file_size )
        return -1;

    errno = 0;
    fp = fopen(path, "rb+");
    int errsv = errno;
    if ( !fp )
    {
        s = errsv;
        EPrint("Could not open \"%s\"! (0x%x)\n", path, s);
        return s;
    }

    // If delete from start offset to end of file, just truncate.
    if ( start + ln >= file_size )
    {
        ln = file_size - start;
        s = truncateFile(fp, file_size, ln);
        goto clean;
    }

    end = start + ln;
    offset = end;
    s = fseek(fp, offset, SEEK_SET);
    errsv = errno;
    if ( s != 0 )
    {
        s = errsv;
        EPrint("FSeek in \"%s\"! (0x%x)\n", path, errsv);
        goto clean;
    }

    while ( n == BLOCK_SIZE )
    {
        memset(buf, 0, BLOCK_SIZE);

        // read from offset
        s = fseek(fp, offset, SEEK_SET);
        errsv = errno;
        if ( s != 0 )
        {
            s = errsv;
            EPrint("FSeek failed in \"%s\"! (0x%x)\n", path, s);
            goto clean;
        }

        n = fread(buf, 1, BLOCK_SIZE, fp);
        errsv = errno;
        if ( ferror(fp) )
        {
            s = errsv;
            EPrint("fread failed in \"%s\"! (0x%x)\n", path, s);
            goto clean;
        }

        // write to start
        s = fseek(fp, start, SEEK_SET);     // f: ....0123456789ABCDEF, buf = 01234567, ln =
        errsv = errno;
        if ( s != 0 )
        {
            s = errsv;
            EPrint("FSeek failed in \"%s\"! (0x%x)\n", path, s);
            goto clean;
        }

        written = fwrite(buf, 1, n, fp);               // f: 01234567896789ABCDEF...., buf = 01234567
        errsv = errno;
        if ( ferror(fp) )
        {
            s = errsv;
            EPrint("fwrite failed in \"%s\"! (0x%x)\n", path, s);
            goto clean;
        }

        // increase offset and start
        offset += n;
        start += n;
    }

    s = truncateFile(fp, file_size, ln);

clean:
    if ( fp )
        fclose(fp);

    return s;
}

int truncateFile(FILE* fp, size_t size, size_t ln)
{
    int s = 0;
    errno = 0;
#if defined(_LINUX) || defined(__APPLE__)
    s = ftruncate(fileno(fp), size-ln);
    if ( s != 0 )
    {
        s = errno;
    }
#elif defined(_WIN32)
    #if defined(_32BIT)
        s = _chsize(_fileno(fp), size-ln);
    #else
        s = _chsize_s(_fileno(fp), size-ln);
    #endif
#endif
    return s;
}
