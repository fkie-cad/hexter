#include <ctype.h>
#include <string.h>

#include "Strings.h"

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
	char* next_token;

//	token = strtok(str, delimiter);
	token = strtok_s(str, delimiter, &next_token);
	size_t token_id = 0;

	while ( token != NULL)
	{
		bucket[token_id] = token;
//		token = strtok(NULL, delimiter);
		token = strtok_s(NULL, delimiter, &next_token);

		token_id++;
		if ( token_id >= bucket_max )
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
 * A string in args may be marked by sm.
 *
 * @param buffer char* the args buffer string
 * @param argv char** the pre-allocated argv container
 * @param argv_size size_t the maximum expected number of args, i.e. size of argv
 * @param sm char a string marker. A characters enclosed by this marker are not split by spaces.
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
