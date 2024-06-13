#ifdef _WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <ctype.h>
#include <string.h>

#include "Strings.h"
#include "../print.h"

static const int le = 1;

/**
 * Split string into its parts, determined by the delimiter.
 * str will be changed!!!
 * TODO: show, if max_bucket is exceeded
 *
 * @param	str char* the source string
 * @param	delimiter char* the splitting delimiter
 * @param	bucket char** the bucket to hold the parts (should be allocated memory)
 * @param	bucket_max size_t the max size of the bucket (if there are more parts, the loop will break before)
 * @return	size_t the size of the actual bucket, i.e. number of found parts.
 */
size_t split(char* str, const char* delimiter, char** bucket, const size_t bucket_max)
{
    char* token;
    size_t token_id = 0;

    if (str == NULL)
    {
        EPrint("str is NULL!\n");
        return 0;
    }
    if (bucket == NULL)
    {
        EPrint("bucket is NULL!\n");
        return 0;
    }
    if (bucket_max == 0)
    {
        EPrint("bucket_max is 0!");
        return 0;
    }

    token = strtok(str, delimiter);

    while ( token != NULL )
    {
        bucket[token_id] = token;
        token = strtok(NULL, delimiter);

        token_id++;
        if (token_id >= bucket_max)
            break;
    }

    return token_id;
}

/**
 * Split args string into parts, delimited by space.
 * A string in args is marked by a '"'.
 *
 * @param buffer char* the args buffer string
 * @param argv char** the pre-allocated argv container
 * @param argv_size size_t the maximum expected number of args, i.e. size of argv
 * @return size_t the actual number of parsed argv.
 */
size_t splitArgs(char* buffer, char* argv[], size_t argv_size)
{
    return splitArgsCSM(buffer, argv, argv_size, '"', '"');
}

/**
 * Split args string into parts, delimited by space.
 * Spaces in between chars enclosed by som, scm (som chars chars scm) are not split. 
 *
 * @param buffer char* the args buffer string
 * @param argv char** the pre-allocated argv container
 * @param argv_size size_t the maximum expected number of args, i.e. size of argv
 * @param som char a (opening)´string marker. A characters sequence opened by this marker is not split by spaces.
 * @param scm char a (closing) string marker. A characters sequence closed by this marker is not split by spaces.
 * @return size_t the actual number of parsed argv.
 */
size_t splitArgsCSM(char* buffer, char* argv[], size_t argv_size, char som, char scm)
{
    char* p = NULL;
    char* start_of_word = NULL;
    int c;
    enum states
    {
        DULL, IN_WORD, IN_STRING
    } state = DULL;
    size_t argc = 0;
    size_t som_count = 0;
    size_t scm_count = 0;

    for ( p = buffer; argc < argv_size && *p != '\0'; p++ )
    {
        c = (unsigned char) *p;
        switch ( state )
        {
            case DULL:
                if ( isspace(c) )
                    continue;

                if ( c == som )
                {
                    state = IN_STRING;
                    start_of_word = p + 1;
                    som_count = 1;
                    continue;
                }
                state = IN_WORD;
                start_of_word = p;
                continue;

            case IN_STRING:
                if ( c == som && som != scm )
                    som_count++;

                if ( c == scm && ++scm_count == som_count )
                {
                    *p = 0;
                    argv[argc++] = start_of_word;
                    state = DULL;
                    som_count = 0;
                    scm_count = 0;
                }
                continue;

            case IN_WORD:
                if ( isspace(c))
                {
                    *p = 0;
                    argv[argc++] = start_of_word;
                    state = DULL;
                }
                continue;
        }
    }

    if ( state != DULL && argc < argv_size )
        argv[argc++] = start_of_word;

    return argc;
}


 #define IS_LC_CHAR_A(__char__) \
     ( (__char__) >= 'a' && (__char__) <= 'z' )
 #define IS_UC_CHAR_A(__char__) \
     ( (__char__) >= 'A' && (__char__) <= 'Z' )
 #define LC_TO_UC_CHAR_A(__char_ptr__) \
     (*(__char_ptr__)) -= 0x20
 #define UC_TO_LC_CHAR_A(__char_ptr__) \
     (*(__char_ptr__)) += 0x20

// int toLowerCaseA(char* buffer, size_t size)
// {
    // size_t i;
    // char* end = buffer + size;
    // char* ptr = buffer;
//
    // while ( ptr < end )
    // {
        // if ( IS_UC_CHAR(*ptr) )
        // {
            // UC_TO_LC_CHAR(ptr);
        // }
        // ++ptr;
    // }
    // return 0;
// }

int toUpperCaseCA(char* c)
{
    if ( IS_LC_CHAR_A(*c) )
    {
        LC_TO_UC_CHAR_A(c);
    }
    return 0;
}

int toUpperCaseA(char* buffer, size_t size)
{
    size_t i;
    char* end = buffer + size;
    char* ptr = buffer;

    while ( ptr < end )
    {
        if ( IS_LC_CHAR_A(*ptr) )
        {
            LC_TO_UC_CHAR_A(ptr);
        }
        ++ptr;
    }
    return 0;
}

/**
 * UTF8ToUTF16LE:
 * @outb:  a pointer to an array of bytes to store the result
 * @outlen:  the length of @outb
 * @in:  a pointer to an array of UTF-8 chars
 * @inlen:  the length of @in
 *
 * Take a block of UTF-8 chars in and try to convert it to an UTF-16LE
 * block of chars out.
 *
 * Returns the number of byte written, or -1 by lack of space, or -2
 *     if the transcoding failed.
 *
 * (c) https://dev.w3.org/XML/encoding.c
 */
int UTF8ToUTF16LE(unsigned char* outb, size_t* outlen, const unsigned char* in, size_t* inlen)
{
    unsigned short* out = (unsigned short*) outb;
    const unsigned char* processed = in;
    unsigned short* outstart = out;
    unsigned short* outend;
    const unsigned char* inend = in + *inlen;
    unsigned int c, d;
    int trailing;
    unsigned char* tmp;
    unsigned short tmp1, tmp2;

    if ( in == NULL )
    {
        //	initialization, add the Byte Order Mark
        if ( *outlen >= 2 )
        {
            outb[0] = 0xFF;
            outb[1] = 0xFE;
            *outlen = 2;
            *inlen = 0;
#ifdef DEBUG_ENCODING
            debug_info(xmlGenericErrorContext,
            "Added FFFE Byte Order Mark\n");
#endif
            return (2);
        }
        *outlen = 0;
        *inlen = 0;
        return (0);
    }
    outend = out + (*outlen / 2);
    while ( in < inend )
    {
        d = *in++;
        if ( d < 0x80 )
        {
            c = d;
            trailing = 0;
        }
        else if ( d < 0xC0 )
        {
            // trailing byte in leading position
            *outlen = (out - outstart) * 2;
            *inlen = processed - in;
            return (-2);
        }
        else if ( d < 0xE0 )
        {
            c = d & 0x1F;
            trailing = 1;
        }
        else if ( d < 0xF0 )
        {
            c = d & 0x0F;
            trailing = 2;
        }
        else if ( d < 0xF8 )
        {
            c = d & 0x07;
            trailing = 3;
        }
        else
        {
            // no chance for this in UTF-16
            *outlen = (out - outstart) * 2;
            *inlen = processed - in;
            return (-2);
        }

        if ( inend - in < trailing )
        {
            break;
        }

        for ( ; trailing; trailing-- )
        {
            if ((in >= inend) || (((d = *in++) & 0xC0) != 0x80))
                break;
            c <<= 6;
            c |= d & 0x3F;
        }

        // assertion: c is a single UTF-4 value
        if ( c < 0x10000 )
        {
            if ( out >= outend )
                break;
            if ( le )
            {
                *out++ = (unsigned short)c;
            }
            else
            {
                tmp = (unsigned char*) out;
                *tmp = (unsigned char)c;
                *(tmp + 1) = (unsigned char)(c >> 8);
                out++;
            }
        }
        else if ( c < 0x110000 )
        {
            if ( out + 1 >= outend )
                break;
            c -= 0x10000;
            if ( le )
            {
                *out++ = 0xD800 | (unsigned short)(c >> 10);
                *out++ = 0xDC00 | (c & 0x03FF);
            }
            else
            {
                tmp1 = 0xD800 | (unsigned short)(c >> 10);
                tmp = (unsigned char*) out;
                *tmp = (unsigned char) tmp1;
                *(tmp + 1) = tmp1 >> 8;
                out++;

                tmp2 = 0xDC00 | (c & 0x03FF);
                tmp = (unsigned char*) out;
                *tmp = (unsigned char) tmp2;
                *(tmp + 1) = tmp2 >> 8;
                out++;
            }
        }
        else
            break;
        processed = in;
    }
    *outlen = (out - outstart) * 2;
    *inlen = processed - in;
    return (0);
}

/**
 * UTF16LEToUTF8:
 * @out:  a pointer to an array of bytes to store the result
 * @outlen:  the length of @out
 * @inb:  a pointer to an array of UTF-16LE passwd as a byte array
 * @inlenb:  the length of @in in UTF-16LE chars
 *
 * Take a block of UTF-16LE ushorts in and try to convert it to an UTF-8
 * block of chars out. This function assume the endian properity
 * is the same between the native type of this machine and the
 * inputed one.
 *
 * Returns the number of byte written, or -1 by lack of space, or -2
 *     if the transcoding fails (for *in is not valid utf16 string)
 *     The value of *inlen after return is the number of octets consumed
 *     as the return value is positive, else unpredictiable.
 *
 * (c) https://dev.w3.org/XML/encoding.c
 */
int UTF16LEToUTF8(unsigned char* out, size_t* outlen, const unsigned char* inb, size_t* inlenb)
{
    unsigned char* outstart = out;
    const unsigned char* processed = inb;
    unsigned char* outend = out + *outlen;
    unsigned short* in = (unsigned short*) inb;
    unsigned short* inend;
    unsigned int c, d, inlen;
    unsigned char *tmp;
    int bits;

    if ((*inlenb % 2) == 1)
        (*inlenb)--;
    inlen = (unsigned int)(*inlenb / 2);
    inend = in + inlen;
    while ((in < inend) && (out - outstart + 5u < *outlen)) {
        if (le) {
            c= *in++;
        } else {
            tmp = (unsigned char *) in;
            c = *tmp++;
            c = c | (((unsigned int)*tmp) << 8);
            in++;
        }
        if ((c & 0xFC00) == 0xD800) {    /* surrogates */
            if (in >= inend) {           /* (in > inend) shouldn't happens */
                break;
            }
            if (le) {
                d = *in++;
            } else {
                tmp = (unsigned char *) in;
                d = *tmp++;
                d = d | (((unsigned int)*tmp) << 8);
                in++;
            }
            if ((d & 0xFC00) == 0xDC00) {
                c &= 0x03FF;
                c <<= 10;
                c |= d & 0x03FF;
                c += 0x10000;
            }
            else {
                *outlen = out - outstart;
                *inlenb = processed - inb;
                return(-2);
            }
        }

        /* assertion: c is a single UTF-4 value */
        if (out >= outend)
            break;
        if      (c <    0x80) {  *out++= (unsigned char)c;                bits= -6; }
        else if (c <   0x800) {  *out++= ((c >>  6) & 0x1F) | 0xC0;  bits=  0; }
        else if (c < 0x10000) {  *out++= ((c >> 12) & 0x0F) | 0xE0;  bits=  6; }
        else                  {  *out++= ((c >> 18) & 0x07) | 0xF0;  bits= 12; }

        for ( ; bits >= 0; bits-= 6) {
            if (out >= outend)
                break;
            *out++= ((c >> bits) & 0x3F) | 0x80;
        }
        processed = (const unsigned char*) in;
    }
    *outlen = out - outstart;
    *inlenb = processed - inb;
    return(0);
}
