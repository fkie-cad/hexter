#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Payloader.h"
#include "Globals.h"
#include "utils/common_fileio.h"
#include "utils/Helper.h"

void insert()
{
	uint64_t p;
	uint64_t end = start + length;
	uint16_t block_size = BLOCKSIZE_LARGE;
	uint64_t block_start = start;
	uint64_t read_size = 0;
	uint64_t parts = length / block_size;
	if ( length % block_size != 0 ) parts++;

	unsigned char* block = NULL;

	debug_info("start: %lu\n", start);
	debug_info("end: %lu\n", end);
	debug_info("block_size: %d\n", block_size);
	debug_info("block_start: %lu\n", block_start);
	debug_info("parts: %lu\n", parts);
	debug_info("\n");

	FILE* fi;
	fi = fopen(file_name, "rb");
	if ( !fi )
	{
		printf("File %s does not exist.\n", file_name);
		return;
	}

	block = (unsigned char*) malloc(block_size);
	if ( !block )
	{
		printf("Malloc block failed.\n");
		return;
	}


	free(block);
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
	// end backup

	fseek(src, start, SEEK_SET);
	fwrite(payload, 1, payload_ln, src);

	fclose(src);
//	fclose(bck);
}
