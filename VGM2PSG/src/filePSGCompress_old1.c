/*****************************************************************************/
/* VGM2PSG PSG Compression function                                          */
/*                                                                           */
/* Copyright (C) 2023 Laszlo Arvai                                           */
/* All rights reserved.                                                      */
/*                                                                           */
/* This software may be modified and distributed under the terms             */
/* of the BSD license.  See the LICENSE file for details.                    */
/*****************************************************************************/

///////////////////////////////////////////////////////////////////////////////
// Include files
#include <stdio.h>
#include <string.h>
#include <Main.h>
#include <filePSG.h>
#include <filePSGCompress.h>

///////////////////////////////////////////////////////////////////////////////
// Defines
#define CHARACTER_COUNT 256

#define PSG_SUBSTRING									0x08
#define PSG_SUBSTRING_MIN_LEN         4
#define PSG_SUBSTRING_MAX_LEN         51        // 47+4

#define PSG_CBS_UNUSED			0
#define PSG_CBS_REFERENCED	1
#define PSG_CBS_SUBSTRING		2
#define PSG_CBS_OFFSET			3

///////////////////////////////////////////////////////////////////////////////
// Local functions
static void filePSGPrepareJumpTable(uint8_t* in_string, uint8_t in_length);
static int filePSGSearchPattern(uint8_t* in_buffer, int in_start_index, int in_pattern_start_index, int in_pattern_length);

///////////////////////////////////////////////////////////////////////////////
// Module global variables
static uint8_t l_compression_buffer_state[FILE_BUFFER_LENGTH];
static uint8_t l_jump_table[CHARACTER_COUNT];

///////////////////////////////////////////////////////////////////////////////
// Compresses the buffer
int filePSGCompress(uint8_t* in_buffer, int in_buffer_length)
{
	int current_start_index;
	int current_index;
	int substring_start_index;
	int substring_found;
	int copy_from;
	int copy_to;
	int copy_count;
	int expected_substring_length;
	int offset;
	int current_end;
	int pos;

	// no compression for short files
	if (in_buffer_length < PSG_SUBSTRING_MIN_LEN)
		return in_buffer_length;

	// mark all byte status as unused
	for (current_index = 0; current_index < in_buffer_length; current_index++)
		l_compression_buffer_state[current_index] = PSG_CBS_UNUSED;

	// start compression with all possible substring length
	for (expected_substring_length = PSG_SUBSTRING_MAX_LEN; expected_substring_length >= PSG_SUBSTRING_MIN_LEN; expected_substring_length--)
	{
		printf(".");

		// select string for compression
		current_start_index = expected_substring_length;
		while (current_start_index < in_buffer_length - expected_substring_length)
		{
			// all bytes of the string must be unused
			current_index = current_start_index;
			current_end = current_start_index + expected_substring_length;
			while (current_index < current_end && l_compression_buffer_state[current_index] == PSG_CBS_UNUSED)
				current_index++;

			// not all bytes are unused -> move to the next byte after the used byte and try again 
			if (current_index != current_end)
			{
				current_start_index = current_index + 1;
				continue;
			}

			// string is selected, build the jump table
			filePSGPrepareJumpTable(&in_buffer[current_start_index], expected_substring_length);

			// try to find the repetition string before the selected string position
			substring_start_index = 0; // start from the first character
			substring_found = false;
			while (!substring_found && substring_start_index <= current_start_index - expected_substring_length)
			{
				pos = filePSGSearchPattern(in_buffer, substring_start_index, current_start_index, expected_substring_length);

				// if found a repetition substring
				if (pos >= 0)
				{
					// none of the element of the substring can be 'substring' or 'offset' (no recursive compression is supported)
					current_end = pos + expected_substring_length;
					current_index = pos;
					while (current_index < current_end && l_compression_buffer_state[current_index] < PSG_CBS_SUBSTRING)
						current_index++;

					if (current_index != current_end)
					{
						// skip unusable substring characters
						substring_start_index = current_index + 1;
					}
					else
					{
						// substring found
						substring_start_index = pos;
						substring_found = true;
					}
				}
				else
				{
					// substring not found -> exit loop
					break;
				}
			}

			// if substring found -> replace oroginal string with a reference to the substring
			if (substring_found)
			{
				// mark referenced bytes (substring)
				for (current_index = substring_start_index; current_index < substring_start_index + expected_substring_length; current_index++)
					l_compression_buffer_state[current_index] = PSG_CBS_REFERENCED;

				// create reference
				in_buffer[current_start_index] = (expected_substring_length - PSG_SUBSTRING_MIN_LEN) + PSG_SUBSTRING;
				in_buffer[current_start_index + 1] = (substring_start_index & 0xFF);
				in_buffer[current_start_index + 2] = (substring_start_index >> 8);

				// mark substring and offset
				l_compression_buffer_state[current_start_index] = PSG_CBS_SUBSTRING;
				l_compression_buffer_state[current_start_index + 1] = PSG_CBS_OFFSET;
				l_compression_buffer_state[current_start_index + 2] = PSG_CBS_OFFSET;

				// compact remaining bytes
				copy_from = current_start_index + expected_substring_length;
				copy_to = current_start_index + 3;
				copy_count = in_buffer_length - copy_from;
				while (copy_count > 0)
				{
					in_buffer[copy_to] = in_buffer[copy_from];
					l_compression_buffer_state[copy_to] = l_compression_buffer_state[copy_from];
					copy_from++;
					copy_to++;
					copy_count--;
				}

				// update offsets to the moved area
				for (current_index = current_start_index + 3; current_index < in_buffer_length; current_index++)
				{
					if (l_compression_buffer_state[current_index] == PSG_CBS_SUBSTRING)
					{
						offset = in_buffer[current_index + 1] + (in_buffer[current_index + 2] << 8);

						// if offset is inside the moved area
						if (offset > current_start_index)
						{
							offset -= expected_substring_length - 3;
							in_buffer[current_index + 1] = offset & 0xff;
							in_buffer[current_index + 2] = offset >> 8;
						}
					}
				}

				// move to the next string in the buffer
				in_buffer_length -= expected_substring_length - 3;
				current_start_index += 3;
			}
			else
			{
				// not compressible -> move to the next string
				current_start_index++;
			}
		}
	}

	return in_buffer_length;
}

///////////////////////////////////////////////////////////////////////////////
// Prepares jump table for string search
static void filePSGPrepareJumpTable(uint8_t* in_string, uint8_t in_length)
{
	int i;

	for (i = 0; i < CHARACTER_COUNT; i++)
	{
		l_jump_table[i] = in_length;
	}

	for (i = 0; i < in_length - 1; i++)
	{
		l_jump_table[in_string[i]] = in_length - 1 - i;
	}
}

///////////////////////////////////////////////////////////////////////////////
// Performs a Boyer-Moore-Horspool pattern matching
static int filePSGSearchPattern(uint8_t* in_buffer, int in_start_index, int in_pattern_start_index, int in_pattern_length)
{
	int skip;

	skip = in_start_index;

	while (in_pattern_start_index - skip >= in_pattern_length)
	{
		if (memcmp(&in_buffer[skip], &in_buffer[in_pattern_start_index], in_pattern_length) == 0)
		{
			return skip;
		}
		else
		{
			skip += l_jump_table[in_buffer[skip + in_pattern_length - 1]];
		}
	}

	return -1;
}