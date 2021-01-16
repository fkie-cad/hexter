#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(__linux__) || defined(__linux) || defined(linux)
	#include <unistd.h>
#elif defined(_WIN32)
	#include <io.h>
#endif

#include "utils/Strings.h"
#include "Writer.h"
#include "Globals.h"
#include "utils/Converter.h"

static void truncateFile(FILE* fp, size_t file_size, size_t length);

/**
 * Parse the arg as a byte.
 * Allocates payload. Caller has to free it.
 *
 * @param arg
 * @param payload
 * @return
 */
uint32_t payloadParseByte(const char* arg, unsigned char** payload)
{
	int s;
	uint32_t arg_ln = strnlen(arg, MAX_PAYLOAD_LN);
	if ( arg_ln < 1 )
	{
		printf("Error: Payload byte has no value!\n");
		return 0;
	}
	if ( arg_ln > 2 )
	{
		printf("Error: Payload byte is too big!\n");
		return 0;
	}
	arg_ln = 1;  // 1 byte
	unsigned char* p = (unsigned char*) malloc(arg_ln);

	s = parseUint8(&arg[0], p, 16);
	if ( s != 0 )
	{
		return 0;
	}

	*payload = p;
	return arg_ln;
}

/**
 * Parse the fill byte and fill the payload buffer of the passed length with the fill byte.
 * Allocates payload. Caller has to free it.
 *
 * @param arg
 * @param payload
 * @param length
 * @return
 */
uint32_t payloadParseFillBytes(const char* arg, unsigned char** payload, size_t length)
{
	int s;
	uint32_t arg_ln = strnlen(arg, MAX_PAYLOAD_LN);
	uint8_t fill_byte = 0;
	if ( arg_ln < 1 )
	{
		printf("Error: Fill byte has no value!\n");
		return 0;
	}
	if ( arg_ln > 2 )
	{
		printf("Error: Fill byte is too big!\n");
		return 0;
	}
	arg_ln = length;
	unsigned char* p = (unsigned char*) malloc(arg_ln);

	s = parseUint8(&arg[0], &fill_byte, 16);
	if ( s != 0 )
		return 0;
	memset(p, fill_byte, arg_ln);

	*payload = p;
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
uint32_t payloadParseWord(const char* arg, unsigned char** payload)
{
	int s;
	uint32_t arg_ln = strnlen(arg, MAX_PAYLOAD_LN);
	if ( arg_ln < 1 )
	{
		printf("Error: Payload byte has no value!\n");
		return 0;
	}
	if ( arg_ln > 4 )
	{
		printf("Error: Payload word is too big!\n");
		return 0;
	}
	arg_ln = 2;  // 2 bytes
	unsigned char* p = (unsigned char*) malloc(arg_ln);

	uint16_t temp;
	s = parseUint16(&arg[0], &temp, 16);
	if ( s != 0 )
		return 0;

	// bytes are reversed using memcpy
	memcpy(p, &temp, arg_ln);

	*payload = p;
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
uint32_t payloadParseDWord(const char* arg, unsigned char** payload)
{
	int s;
	uint32_t arg_ln = strnlen(arg, MAX_PAYLOAD_LN);
	if ( arg_ln < 1 )
	{
		printf("Error: Payload byte has no value!\n");
		return 0;
	}
	if ( arg_ln > 8 )
	{
		printf("Error: Payload double word is too big!\n");
		return 0;
	}
	arg_ln = 4;  // 4 bytes
	unsigned char* p = (unsigned char*) malloc(arg_ln);

	uint32_t temp;
	s = parseUint32(&arg[0], &temp, 16);
	if ( s != 0 )
		return 0;

	// bytes are reversed using memcpy
	memcpy(p, &temp, arg_ln);

	*payload = p;
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
uint32_t payloadParseQWord(const char* arg, unsigned char** payload)
{
	int s;
	uint32_t arg_ln = strnlen(arg, MAX_PAYLOAD_LN);
	if ( arg_ln < 1 )
	{
		printf("Error: Payload byte has no value!\n");
		return 0;
	}
	if ( arg_ln > 16 )
	{
		printf("Error: Payload quad word is too big!\n");
		return 0;
	}
	arg_ln = 8;  // 8 bytes
	unsigned char* p = (unsigned char*) malloc(arg_ln);

	uint64_t temp;
	s = parseUint64(&arg[0], &temp, 16);
	if ( s != 0 )
		return 0;

	// bytes are reversed using memcpy
	memcpy(p, &temp, arg_ln);

	*payload = p;
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
uint32_t payloadParseUtf8(const char* arg, unsigned char** payload)
{
	uint32_t i;
	uint32_t arg_ln = strnlen(arg, MAX_PAYLOAD_LN);
	if ( arg_ln < 1 )
	{
		printf("Error: Payload string has no value!\n");
		return 0;
	}
	unsigned char* p = (unsigned char*) malloc(arg_ln);

	for ( i = 0; i < arg_ln; i++ )
	{
		p[i] = (unsigned char) arg[i];
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
uint32_t payloadParseUtf16(const char* arg, unsigned char** payload)
{
	uint32_t i;
    size_t arg_ln = strnlen(arg, MAX_PAYLOAD_LN);

	// fill buffer to get the real size
    unsigned char outb[MAX_PAYLOAD_LN*2] = {0};
    size_t outlen = MAX_PAYLOAD_LN*2;

	if ( arg_ln < 1 )
	{
		printf("Error: Payload string has no value!\n");
		return 0;
	}

    int s = UTF8ToUTF16LE(outb, &outlen, (unsigned char*)arg, &arg_ln);

    if ( s != 0 )
    {
        printf("Error (0x%x): Converting to utf16.\n", s);
        return 0;
    }

    // alloc payload with real size
	unsigned char* p = (unsigned char*) malloc(outlen);

	for ( i = 0; i < outlen; i++ )
	{
		p[i] = outb[i];
	}

	*payload = p;

	return outlen;
}

/**
 * Parse the arg as plain bytes and reverse them.
 * Allocates payload. Caller has to free it.
 *
 * @param arg
 * @param payload
 * @return
 */
uint32_t payloadParseReversedPlainBytes(const char* arg, unsigned char** payload)
{
	uint32_t i, j;
	unsigned char temp;
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
 * Parse the arg as plain bytes.
 * Allocates payload. Caller has to free it.
 *
 * @param arg
 * @param payload
 * @return
 */
uint32_t payloadParsePlainBytes(const char* arg, unsigned char** payload)
{
	uint32_t i, j;
	uint16_t arg_ln = strnlen(arg, MAX_PAYLOAD_LN);
	unsigned char* p;
	char byte[3] = {0};
	uint32_t payload_ln;
	int s = 0;

	if ( arg_ln % 2 != 0 || arg_ln == 0 )
	{
		printf("Error: Payload is not byte aligned!\n");
		return 0;
	}

	p = (unsigned char*) malloc(arg_ln/2);

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
void insert(const char* file_path, unsigned char* payload, uint32_t payload_ln, size_t offset)
{
	unsigned char buf[BLOCKSIZE_LARGE];
	const int buf_ln = BLOCKSIZE_LARGE;
	size_t n = buf_ln;
	FILE* fp;
	size_t i, j;

	if ( offset > file_size )
	{
		overwrite(file_path, payload, payload_ln, offset);
		return;
	}

	errno = 0;
	fp = fopen(file_path, "rb+");
	int errsv = errno;
	if ( !fp )
	{
		printf("ERROR (0x%x): Could not open \"%s\".\n", errsv, file_path);
		return;
	}

	fseek(fp, offset, SEEK_SET);
	while ( n == buf_ln )
	{
		n = fread(buf, 1, buf_ln, fp);

		fseek(fp, offset, SEEK_SET);		// f: .....0123456789ABCDEF, buf = 0123456789ABCDEF, payload = DEAD0BEA
		fwrite(payload, 1, payload_ln, fp); // f: .....DEAD0BEA89ABCDEF, buf = 0123456789ABCDEF, payload = DEAD0BEA
		if ( n > payload_ln )
		{
			fwrite(buf, 1, n-payload_ln, fp);   // f: .....DEAD0BEA01234567, buf = 0123456789ABCDEF, payload = DEAD0BEA

			for ( i = n-payload_ln, j=0; i < n; i++ )
			{
				payload[j++] = buf[i]; // , buf = 0123456789ABCDEF, payload = 89ABCDEF
			}
		}
		else
		{
			for ( i = 0; i < n; i++ )
			{
				payload[i] = buf[i];
			}
		}

		offset += n;
	}
	if ( n > payload_ln )
		fwrite(payload, 1, payload_ln, fp);
	else
		fwrite(payload, 1, n, fp);

	fclose(fp);
}

/**
 * Overwrite bytes in file with payload.
 *
 * @param	file_path char*
 * @param	payload unsigned char* the bytes to write
 * @param	payload_ln uint32_t the length of the bytes to write
 * @param	offset size_t the offset to write the bytes at
 */
void overwrite(const char* file_path, unsigned char* payload, uint32_t payload_ln, size_t offset)
{
	FILE* fp;
	// backup
//	FILE* bck;
//	char buf[1024];
//	int buf_ln = 1024;
//	char dest_file_name[128];
//	getTempFile(dest_file_name, "hexter.bck");
//	int n = buf_ln;
	// end backup

	errno = 0;
	fp = fopen(file_path, "rb+");
	int errsv = errno;
	if ( !fp )
	{
		printf("ERROR (0x%x): Could not open \"%s\".\n", errsv, file_path);
		return;
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

	fseek(fp, offset, SEEK_SET);
	fwrite(payload, 1, payload_ln, fp);

	fclose(fp);
}

/**
 * Delete bytes in file of the passed length.
 *
 * @param file_path
 * @param start size_t start offset of the deletion.
 * @param length size_t length of the bytes to delete.
 */
void deleteBytes(const char* file_path, size_t start, size_t length)
{
	unsigned char buf[BLOCKSIZE_LARGE];
	const int buf_ln = BLOCKSIZE_LARGE;
	size_t n = buf_ln;
	FILE* fp;
	size_t offset;
	size_t end;

	if ( start > file_size )
		return;

	errno = 0;
	fp = fopen(file_path, "rb+");
	int errsv = errno;
	if ( !fp )
	{
		printf("ERROR (0x%x): Could not open \"%s\".\n", errsv, file_path);
		return;
	}

	// If delete from start to end of file, just truncate.
	if ( start + length >= file_size )
	{
		length = file_size - start;
		truncateFile(fp, file_size, length);
		fclose(fp);
		return;
	}

	end = start + length;
	offset = end;
	fseek(fp, offset, SEEK_SET);
	while ( n == buf_ln )
	{
		memset(buf, 0, BLOCKSIZE_LARGE);

		// read from offset
		fseek(fp, offset, SEEK_SET);
		n = fread(buf, 1, buf_ln, fp);

		// write to start
		fseek(fp, start, SEEK_SET);	 // f: ....0123456789ABCDEF, buf = 01234567, length =
		fwrite(buf, 1, n, fp);               // f: 01234567896789ABCDEF...., buf = 01234567

		// increase offset and start
		offset += n;
		start += n;
	}

	truncateFile(fp, file_size, length);

	fclose(fp);
}

void truncateFile(FILE* fp, size_t size, size_t length)
{
#if defined(__linux__) || defined(__linux) || defined(linux) || defined(__APPLE__)
	ftruncate(fileno(fp), size-length);
#elif defined(_WIN32)
	_chsize(_fileno(fp), size-length);
#endif
}
