#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Payloader.h"
#include "Globals.h"
#include "utils/common_fileio.h"
#include "utils/Converter.h"
#include "utils/Helper.h"

unsigned char* payloadParseReversedPlainBytes(const char* arg)
{
	uint32_t i, j;
	unsigned char temp;
	unsigned char* p = payloadParsePlainBytes(arg);

	for ( i = 0, j = payload_ln-1; i < payload_ln; i++, j-- )
	{
		if ( j <= i )
			break;

		temp = p[i];
		p[i] = p[j];
		p[j] = temp;
	}

	return p;
}

unsigned char* payloadParsePlainBytes(const char* arg)
{
	uint32_t i, j;
	int arg_ln = strnlen(arg, MAX_PAYLOAD_LN);
	unsigned char* p;
	char byte[3] = {0};

	if ( arg_ln % 2 != 0 || arg_ln == 0 )
	{
		printf("Error: Payload is not byte aligned!");
		payload_ln = 0;
		return NULL;
	}

	p = (unsigned char*) malloc(arg_ln/2);

	for ( i = 0, j = 0; i < arg_ln; i += 2 )
	{
		byte[0] = arg[i];
		byte[1] = arg[i + 1];

		p[j++] = parseUint8(byte);
	}

	payload_ln = arg_ln / 2;

	return p;
}

void insert()
{
	unsigned char buf[BLOCKSIZE_LARGE];
	const int buf_ln = BLOCKSIZE_LARGE;
	int n = buf_ln;
	FILE* fi;
	uint64_t i, j, offset;

	if ( start > file_size )
	{
		overwrite();
		return;
	}

	fi = fopen(file_name, "rb+");
	if ( !fi )
	{
		printf("File %s does not exist.\n", file_name);
		return;
	}

	offset = start;
	fseek(fi, start, SEEK_SET);
	while ( n == buf_ln )
	{
		n = fread(buf, 1, buf_ln, fi);

		fseek(fi, offset, SEEK_SET);		// f: .....0123456789ABCDEF, buf = 0123456789ABCDEF, payload = DEAD0BEA
		fwrite(payload, 1, payload_ln, fi); // f: .....DEAD0BEA89ABCDEF, buf = 0123456789ABCDEF, payload = DEAD0BEA
		if ( n > payload_ln )
		{
			fwrite(buf, 1, n-payload_ln, fi);   // f: .....DEAD0BEA01234567, buf = 0123456789ABCDEF, payload = DEAD0BEA

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
		fwrite(payload, 1, payload_ln, fi);
	else
		fwrite(payload, 1, n, fi);

	fclose(fi);
}

void overwrite()
{
	FILE* src;
	// backup
//	FILE* bck;
//	char buf[1024];
//	int buf_ln = 1024;
//	char dest_file_name[128];
//	getTempFile(dest_file_name, "hexter.bck");
//	int n = buf_ln;
	// end backup

	src = fopen(file_name, "rb+");
	if ( !src )
	{
		printf("File %s does not exist.\n", file_name);
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

	fseek(src, start, SEEK_SET);
	fwrite(payload, 1, payload_ln, src);

	fclose(src);
}
