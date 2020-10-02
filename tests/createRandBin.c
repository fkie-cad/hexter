#define _CRT_SECURE_NO_WARNINGS

#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE (0x1000)

int main(int argc, char** argv)
{
	char* path;
	char* endptr;
	size_t size, i, j, end_i, offset;
	FILE* fp = NULL;
	uint16_t r;
	int r_size = sizeof(r);
	int size_of_r;
	int errsv;
	uint8_t buffer[BUFFER_SIZE];
	int rest;

	if ( argc < 3 )
	{
		printf("Usage: createRandBin path size\n");
		return 0;
	}

	path = argv[1];
	size = strtoull(argv[2], &endptr, 0);
	
	printf("Creating random binary\n"
		" - path: \"%s\"\n"
  		" - size: 0x%zx\n", path, size);
	
	errno = 0;
	fp = fopen(path, "wb");
	errsv = errno;
	if (!fp)
	{
		printf("ERROR (0x%x): File could not be opened!\n", errsv);
		return 1;
	}

	
	srand((unsigned int)time(NULL));
	end_i = size / r_size;
	rest = size % r_size;
	
	printf(" - writing:\n");
	memset(buffer, 0, BUFFER_SIZE);
	offset = 0;
	j = 0;
	for ( i = 0, j = 0; i < end_i; i++, j=j+r_size)
	{
		r = rand();
		memcpy(&buffer[offset], &r, r_size);
		offset += r_size;

		if (offset >= BUFFER_SIZE)
		{
			fwrite(buffer, BUFFER_SIZE, 1, fp);
			//fwrite(buffer, 1, BUFFER_SIZE, fp);
			memset(buffer, 0, BUFFER_SIZE);
			offset = 0;
		}

		printf(" - - 0x%zx/0x%zx : %02d%%\r", (j), size, (int)((float)(j) / size * 100));
		//printf(" - - 0x%zx/0x%zx : %02d%%\r", (i+1), end_i, (int)((float)(i+1) / end_i * 100));
	}
	if (offset < BUFFER_SIZE)
	{
		fwrite(buffer, offset, 1, fp);
		//fwrite(buffer, 1, offset, fp);
	}
	if (rest > 0)
	{
		r = rand();
		fwrite(&r, 1, rest, fp);
	}
	printf("\n");
	
	fclose(fp);
	
	printf(" - done\n");
	
	return 0;
}