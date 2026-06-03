#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils/env.h"

#if defined(_LINUX) || defined(__APPLE__)
    #include <unistd.h>
#elif defined(_WIN32)
    #include <io.h>
#endif

#include "utils/common_fileio.h"
#include "utils/Strings.h"
#include "Writer.h"
#include "Globals.h"
#include "utils/Converter.h"

static int truncateFile(FILE* fp, size_t file_size, size_t ln);

/**
 * Parse the arg as a byte.
 * Allocates payload. Caller has to free it.
 *
 * @param arg
 * @param payload
 * @return
 */
uint32_t payloadParseByte(const char* arg, uint8_t** payload)
{
    int s;
    uint32_t arg_ln = (uint32_t)strnlen(arg, 4);
    if ( arg_ln < 1 )
    {
        EPrint("Payload byte has no value!\n");
        return 0;
    }
    if ( arg_ln > 2 )
    {
        EPrint("Payload byte is too big!\n");
        return 0;
    }
    arg_ln = 1;  // 1 byte
    uint8_t* p = (uint8_t*) malloc(arg_ln);
    if ( p == NULL )
    {
        EPrint("Allocating memory failed!\n");
        return 0;
    }

    s = parseUint8(&arg[0], p, 16);
    if ( s != 0 )
    {
        arg_ln = 0;
        goto clean;
    }

clean:
    if ( s != 0 )
    {
        if ( p )
            free(p);
    }
    else
    {
        *payload = p;
    }

    return arg_ln;
}

/**
 * Parse the fill byte and fill the payload buffer of the passed ln with the fill byte.
 * Allocates payload. Caller has to free it.
 *
 * @param arg
 * @param payload
 * @param ln
 * @return
 */
uint32_t payloadParseFillBytes(const char* arg, uint8_t** payload, size_t ln)
{
    int s;
    uint32_t arg_ln = (uint32_t)strnlen(arg, MAX_PAYLOAD_LN);
    uint8_t fill_byte = 0;
    if ( arg_ln < 1 )
    {
        EPrint("Fill byte has no value!\n");
        return 0;
    }
    if ( arg_ln > 2 )
    {
        EPrint("Fill byte is too big!\n");
        return 0;
    }
    arg_ln = (uint32_t)ln;
    uint8_t* p = (uint8_t*) malloc(arg_ln);
    if ( p == NULL )
    {
        EPrint("Allocating memory failed!\n");
        return 0;
    }

    s = parseUint8(&arg[0], &fill_byte, 16);
    if ( s != 0 )
    {
        arg_ln = 0;
        goto clean;
    }
    memset(p, fill_byte, arg_ln);
    
clean:
    if ( s != 0 )
    {
        if ( p )
            free(p);
    }
    else
    {
        *payload = p;
    }
    return arg_ln;
}

/**
 * Parse the arg as a word/uint16_t
 * Allocates payload. Caller has to free it.
 *
 * @param arg
 * @param payload
 * @return
 */
uint32_t payloadParseWord(const char* arg, uint8_t** payload)
{
    int s;
    uint32_t arg_ln = (uint32_t)strnlen(arg, MAX_PAYLOAD_LN);
    if ( arg_ln < 1 )
    {
        EPrint("Payload byte has no value!\n");
        return 0;
    }
    if ( arg_ln > 4 )
    {
        EPrint("Payload word is too big!\n");
        return 0;
    }
    arg_ln = 2;  // 2 bytes
    uint8_t* p = (uint8_t*) malloc(arg_ln);
    if ( p == NULL )
    {
        EPrint("Allocating memory failed!\n");
        return 0;
    }

    uint16_t temp;
    s = parseUint16(&arg[0], &temp, 16);
    if ( s != 0 )
    {
        arg_ln = 0;
        goto clean;
    }

    // bytes are reversed using memcpy
    memcpy(p, &temp, arg_ln);

clean:
    if ( s != 0 )
    {
        if ( p )
            free(p);
    }
    else
    {
        *payload = p;
    }

    return arg_ln;
}

/**
 * Parse the arg as a dword/uint32_t
 * Allocates payload. Caller has to free it.
 *
 * @param arg
 * @param payload
 * @return
 */
uint32_t payloadParseDWord(const char* arg, uint8_t** payload)
{
    int s;
    uint32_t arg_ln = (uint32_t)strnlen(arg, MAX_PAYLOAD_LN);
    if ( arg_ln < 1 )
    {
        EPrint("Payload byte has no value!\n");
        return 0;
    }
    if ( arg_ln > 8 )
    {
        EPrint("Payload dword is too big!\n");
        return 0;
    }
    arg_ln = 4;  // 4 bytes
    uint8_t* p = (uint8_t*) malloc(arg_ln);
    if ( p == NULL )
    {
        EPrint("Allocating memory failed!\n");
        return 0;
    }

    uint32_t temp;
    s = parseUint32(&arg[0], &temp, 16);
    if ( s != 0 )
    {
        arg_ln = 0;
        goto clean;
    }

    // bytes are reversed using memcpy
    memcpy(p, &temp, arg_ln);

clean:
    if ( s != 0 )
    {
        if ( p )
            free(p);
    }
    else
    {
        *payload = p;
    }

    return arg_ln;
}

/**
 * Parse the arg as a qword/uint64_t
 * Allocates payload. Caller has to free it.
 *
 * @param arg
 * @param payload
 * @return
 */
uint32_t payloadParseQWord(const char* arg, uint8_t** payload)
{
    int s;
    uint32_t arg_ln = (uint32_t)strnlen(arg, MAX_PAYLOAD_LN);
    if ( arg_ln < 1 )
    {
        EPrint("Payload byte has no value!\n");
        return 0;
    }
    if ( arg_ln > 16 )
    {
        EPrint("Payload quad word is too big!\n");
        return 0;
    }
    arg_ln = 8;  // 8 bytes
    uint8_t* p = (uint8_t*) malloc(arg_ln);
    if ( p == NULL )
    {
        EPrint("Allocating memory failed!\n");
        return 0;
    }

    uint64_t temp;
    s = parseUint64(&arg[0], &temp, 16);
    if ( s != 0 )
    {
        arg_ln = 0;
        goto clean;
    }

    // bytes are reversed using memcpy
    memcpy(p, &temp, arg_ln);
    
clean:
    if ( s != 0 )
    {
        if ( p )
            free(p);
    }
    else
    {
        *payload = p;
    }
    return arg_ln;
}

/**
 * Parse the arg as an utf8 string.
 * Allocates payload. Caller has to free it.
 *
 * @param arg
 * @param payload
 * @return
 */
uint32_t payloadParseUtf8(const char* arg, uint8_t** payload)
{
    uint32_t i;
    uint32_t arg_ln = (uint32_t)strnlen(arg, MAX_PAYLOAD_LN);
    if ( arg_ln < 1 )
    {
        EPrint("Payload string has no value!\n");
        return 0;
    }
    uint8_t* p = (uint8_t*) malloc(arg_ln);
    if ( p == NULL )
    {
        EPrint("Allocating memory failed!\n");
        return 0;
    }

    for ( i = 0; i < arg_ln; i++ )
    {
        p[i] = (uint8_t) arg[i];
    }

    *payload = p;
    return arg_ln;
}

/**
 * Parse the arg as an utf16 (windows unicode) string.
 * Allocates payload. Caller has to free it.
 *
 * @param arg
 * @param payload
 * @return
 */
uint32_t payloadParseUtf16(const char* arg, uint8_t** payload, size_t max_payload_ln)
{
    uint32_t i;
    size_t arg_ln = strnlen(arg, max_payload_ln);
    
    size_t outlen = 0;
    uint8_t* outb = NULL;

    if ( arg_ln < 1 )
    {
        EPrint("Payload string has no value!\n");
        return 0;
    }

    // fill max buffer to get the real size
    // utf-8 is one to 4 bytes plus some arbitrary buffer, 2 bytes bom could be added
    outlen = arg_ln * 4 + 0x10;
    outb = (uint8_t*)malloc(outlen);
    if ( !outb )
        return 0;

    int s = UTF8ToUTF16LE(outb, &outlen, (uint8_t*)arg, &arg_ln);
    if ( s != 0 )
    {
        EPrint("Converting to utf16 failed! (0x%x)\n", s);
        outlen = 0;
        goto clean;
    }

    // alloc payload with actual needed size
    uint8_t* p = (uint8_t*) malloc(outlen);
    if ( p == NULL )
    {
        s = errno;
        EPrint("Allocating memory failed! (0x%x)\n", s);
        outlen = 0;
        goto clean;
    }

    for ( i = 0; i < outlen; i++ )
    {
        p[i] = outb[i];
    }

    *payload = p;

clean:
    if ( outb )
        free(outb);

    return (uint32_t)outlen;
}

/**
 * Parse the arg as plain bytes and reverse them.
 * Allocates payload. Caller has to free it.
 *
 * @param arg
 * @param payload
 * @return
 */
uint32_t payloadParseReversedPlainBytes(const char* arg, uint8_t** payload)
{
    uint32_t i, j;
    uint8_t temp;
    uint32_t payload_ln = payloadParsePlainBytes(arg, payload);

    for ( i = 0, j = payload_ln-1; i < payload_ln; i++, j-- )
    {
        if ( j <= i )
            break;

        temp = (*payload)[i];
        (*payload)[i] = (*payload)[j];
        (*payload)[j] = temp;
    }

    return payload_ln;
}

/**
 * Clean byte string of spaces or \x format tags
 */
int cleanBytes(const char* input, char** output)
{
    // get max size of data
    size_t input_ln = strlen(input);
    int s = 0;

    // alloc output buffer + terminating zero
    char* local = (char*)malloc(input_ln+1);
    if ( !local )
    {
        s = errno;
        return s;
    }
    size_t local_cb = 0;

    const char* end_ptr = input + input_ln;
    char* local_ptr = local;
    for ( const char* input_ptr = input; input_ptr < end_ptr; input_ptr++ )
    {
        // skip spaces and separators
        if ( *input_ptr == ' ' 
          || *input_ptr == '|'
          || *input_ptr == '-' )
            continue;
        // skip "\x" marker
        if (*input_ptr == '\\'
            && input_ptr < end_ptr - 1
            && *(input_ptr + 1) == 'x')
        {
            input_ptr++;
            continue;
        }

        *local_ptr = *input_ptr;
        local_ptr++;
    }

    local_cb = local_ptr - local;
    if ( local_cb > MAX_PAYLOAD_LN )
    {
        free(local);
        return -2;
    }
    local[local_cb] = 0;

    *output = local;

    return 0;
}

/**
 * Parse the arg as plain bytes.
 * Allocates payload. Caller has to free it.
 *
 * @param arg
 * @param payload
 * @return
 */
uint32_t payloadParsePlainBytes(const char* arg, uint8_t** payload)
{
    uint32_t i, j;
    uint32_t arg_ln = (uint32_t)strnlen(arg, MAX_PAYLOAD_LN);
    uint8_t* p;
    char byte[3] = {0};
    uint32_t payload_ln;
    int s = 0;

    if ( arg_ln % 2 != 0 || arg_ln == 0 )
    {
        EPrint("Payload is not byte aligned!\n");
        return 0;
    }

    p = (uint8_t*) malloc(arg_ln/2);
    if ( p == NULL )
    {
        s = errno;
        EPrint("Allocating memory failed! (0x%x)\n", s);
        return 0;
    }

    for ( i = 0, j = 0; i < arg_ln; i += 2 )
    {
        byte[0] = arg[i];
        byte[1] = arg[i + 1];

         s = parseUint8(byte, &p[j++], 16);
         if ( s != 0 )
         {
            free(p);
            return 0;
         }
    }

    payload_ln = arg_ln / 2;

    *payload = p;
    return payload_ln;
}

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
        return s;
    }

    buffer_size = max(BLOCKSIZE_LARGE, payload_ln);
    buffer = malloc(buffer_size);
    if ( !buffer )
    {
        s = errno;
        EPrint("No memory for buffer of size 0x%zx! (0y%x)\n", buffer_size, s);
        goto clean;
    }
    bytes_read = buffer_size;

    // "ab+" results in strange behaviour
    // thats why this construct is used
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
        
        if ( bytes_read == buffer_size )
            offset += bytes_read;
        else
            offset += payload_ln;
    }
    if ( bytes_read > payload_ln )
    {
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
    }
    else // bytes_read <= payload_ln
    {
        s = fseek(fp, offset, SEEK_SET);
        errsv = errno;
        if ( s != 0 )
        {
            s = errsv;
            EPrint("FSeek in \"%s\"! (0x%x)\n", path, errsv);
            goto clean;
        }

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

    return s;
}

/**
 * Overwrite bytes in file with payload.
 *
 * @param	file_path char*
 * @param	payload uint8_t* the bytes to write
 * @param	payload_ln uint32_t the ln of the bytes to write
 * @param	offset size_t the offset to write the bytes at
 */
int overwrite(const char* path, uint8_t* payload, uint32_t payload_ln, size_t offset)
{
    FILE* fp = NULL;
    int s = 0;
    size_t written;

    // backup
//	FILE* bck;
//	char buf[1024];
//	int buf_ln = 1024;
//	char dest_file_name[128];
//	getTempFile(dest_file_name, "hexter.bck");
//	int n = buf_ln;
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
//	bck = fopen(dest_file_name, "wb");
//	if ( !bck )
//	{
//		printf("File %s could not be created.\n", dest_file_name);
//		return;
//	}
//
//	while ( n == buf_ln )
//	{
//		n = fread(buf, 1, buf_ln, src);
//		fwrite(buf, 1, n, bck);
//	}
//	fclose(bck);
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
    uint8_t buf[BLOCKSIZE_LARGE];
    size_t n = BLOCKSIZE_LARGE;
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

    while ( n == BLOCKSIZE_LARGE )
    {
        memset(buf, 0, BLOCKSIZE_LARGE);

        // read from offset
        s = fseek(fp, offset, SEEK_SET);
        errsv = errno;
        if ( s != 0 )
        {
            s = errsv;
            EPrint("FSeek failed in \"%s\"! (0x%x)\n", path, s);
            goto clean;
        }

        n = fread(buf, 1, BLOCKSIZE_LARGE, fp);
        errsv = errno;
        if ( ferror(fp) )
        {
            s = errsv;
            EPrint("fread failed in \"%s\"! (0x%x)\n", path, s);
            goto clean;
        }

        // write to start
        s = fseek(fp, start, SEEK_SET);	 // f: ....0123456789ABCDEF, buf = 01234567, ln =
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
