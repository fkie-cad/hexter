#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include "Globals.h"
#include "common_fileio.h"
#include "Printer.h"
#include "utils/Converter.h"
#include "utils/Helper.h"

#define BINARYNAME ("hexter")

void printUsage();
void parseArgs(int argc, char **argv);
void sanitizeOffsets();

//* Name 'hhex' (das ist kurz, das erste 'h' steht für Henning ;))
//* Für das Linux-terminal, Cross-OS C-Code auch für Windows. (Es gibt
//  keinen vernünftigen Hex-Editor für die cmd, d.h. für
//  Batchverarbeitung.)
//* Printed exakt 16 (0x10) Bytes als %02x auf linke Seite, getrennt durch
//  '|' von rechter Seite mit Ascii-Darstellung.
//  Ascii-Darstellung:  nur values von 0x20 - 0x80 mit %c printen, alles
//  andere direkt als '.' darstellen.
//* hexedit läd auf gar keinen Fall das ganze File, sondern immer nur
//  eine 'Seite', d.h. einen kleinen Ausschnitt, der ins Terminal passt.
//  (und terminiert dann.)
//* Optionale Parameter:
//  * -s Zahl (Startoffset, sonst 0)
//  * -l zahl (Länge des darzustellenden Ausschnitts, sonst 50)
//  * -a (NUR Asciiprint, dafür aber gleich pro Zeile 0x40 Stück)
//  * -x (NUR hexprint, 0x10 Zeichen pro Zeile)
int main(int argc, char **argv)
{
    if (argc < 2)
    {
        printUsage();
        return -1;
    }

    file_size = 0;
	start = 0;
	length = 50;
	ascii_only = 0;
	hex_only = 0;

	parseArgs(argc, argv);

	file_size = getSize(file_name);
    if ( file_size == 0 ) return 0;

    sanitizeOffsets();

	debug_info("file_size: %lu\n", file_size);
	debug_info("start: %lu\n", start);
	debug_info("length: %lu\n", length);
	debug_info("ascii only: %d\n", ascii_only);
	debug_info("hex only: %d\n", hex_only);

	print();

	return 0;
}

void printUsage()
{
	printf("Usage: ./%s filename [options]\nVersion 1.0.0\n",BINARYNAME);
	printf(" * -s:uint64_t Startoffset. Default = 0.\n"
		   " * -l:uint64_t Length of the part to display. Default = 50.\n"
		   " * -a ASCII only print.\n"
		   " * -x HEX only print.\n");
	printf("Example: ./%s filename -s 100 -l 128 -x\n",BINARYNAME);
}

void parseArgs(int argc, char **argv)
{
	int i;

	expandFilePath(argv[1], file_name);
	debug_info("file_name: %s\n", file_name);

	if ( argc <= 2 )
		return;

	for ( i = 2; i < argc; i++ )
	{
		if ( strncmp(argv[i], "-s", 2) == 0 && i < argc - 1 )
			start = parseUint64(argv[i + 1]);
		if ( strncmp(argv[i], "-l", 2) == 0 && i < argc - 1 )
			length = parseUint64(argv[i + 1]);
		if ( strncmp(argv[i], "-a", 2) == 0 )
			ascii_only = 1;
		if ( strncmp(argv[i], "-x", 2) == 0 )
			hex_only = 1;
	}
}

void sanitizeOffsets()
{
	if ( start > file_size )
	{
		fprintf(stderr, "Start offset %lu is greater the the file_size %lu\n", start, file_size);
		exit(0);
	}
	if ( start + length > file_size )
	{
		fprintf(stderr, "Start offset %lu plus length %lu is greater the the file_size %lu\n", start, length, file_size);
		exit(0);
	}

	if ( length == 0 )
	{
		printf("Length is 0. Setting to 50!\n");
		length = 50;
	}
}