#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
//#include <stdlib.h>
#include <string.h>
//
//#include "utils/env.h"
//
//#if defined(_LINUX) || defined(__APPLE__)
//    #include <unistd.h>
//#elif defined(_WIN32)
//    #include <io.h>
//#endif
//
#include "../print.h"
#include "../Globals.h"
//#include "utils/common_fileio.h"
#include "Strings.h"
//#include "Writer.h"
#include "Converter.h"
#include "payload.h"

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
