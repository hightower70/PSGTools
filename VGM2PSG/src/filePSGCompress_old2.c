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

//#define PSG_CBS_DATA				0
//#define PSG_CBS_REFERENCED	1
//#define PSG_CBS_COMPRESSED	-1


///////////////////////////////////////////////////////////////////////////////
// Types

typedef enum
{
	PSG_CBS_DATA = 0,
	PSG_CBS_REFERENCED = 1,
	PSG_CBS_COMPRESSED = -1

} CompressionByteStatus;

typedef struct
{
	uint8_t Length;
	uint16_t Offset;
	int8_t Status;
} CompressionByteInfo;

///////////////////////////////////////////////////////////////////////////////
// Local functions
static void filePSGPrepareJumpTable(uint8_t* in_string, uint8_t in_length);
static int filePSGSearchPattern(uint8_t* in_buffer, int in_start_index, int in_pattern_start_index, int in_pattern_length);

///////////////////////////////////////////////////////////////////////////////
// Module global variables
static CompressionByteInfo l_compression_info[FILE_BUFFER_LENGTH];

///////////////////////////////////////////////////////////////////////////////
// Compresses the buffer
int filePSGCompress(uint8_t* in_buffer, int in_buffer_length)
{
	int string_start_index;
	int string_index;
	uint8_t* string;
	int substring_start_index;
	int substring_index;
	uint8_t* substring;
	int matching_length;
	int matching_offset;
	int max_matching_length;
	int max_matching_offset;
	int matching_length_limit;
	int expected_substring_length;

	int copy_from;
	int copy_to;
	int delta;

	// no compression for short files
	if (in_buffer_length < PSG_SUBSTRING_MIN_LEN)
		return in_buffer_length;

	// prepare compression byte info
	for (string_index = 0; string_index < in_buffer_length; string_index++)
	{
		l_compression_info[string_index].Length = 0;
		l_compression_info[string_index].Offset = 0;
		l_compression_info[string_index].Status = PSG_CBS_DATA;
	}

	// fill out compression info, find repeated substrings
	string_start_index = PSG_SUBSTRING_MIN_LEN;
	while (string_start_index < in_buffer_length - PSG_SUBSTRING_MIN_LEN)
	{
		// find longest repeated substring before the current_start_index position
		max_matching_length = 0;
		for (substring_start_index = 0; substring_start_index < string_start_index - PSG_SUBSTRING_MIN_LEN; substring_start_index++)
		{
			matching_length_limit = string_start_index - substring_start_index;
			if (matching_length_limit > PSG_SUBSTRING_MAX_LEN)
				matching_length_limit = PSG_SUBSTRING_MAX_LEN;

			substring = &in_buffer[substring_start_index];
			string = &in_buffer[string_start_index];
			matching_length = 0;
			while (*substring++ == *string++ && matching_length < matching_length_limit)
				matching_length++;

			if (matching_length > max_matching_length)
			{
				max_matching_length = matching_length;
				max_matching_offset = substring_start_index;
			}
		}

		if (max_matching_length >= PSG_SUBSTRING_MIN_LEN)
		{
			while (max_matching_length > 0)
			{
				l_compression_info[string_start_index].Length = max_matching_length--;
				l_compression_info[string_start_index].Offset = max_matching_offset++;

				string_start_index++;
			}
		}
		else
		{
			string_start_index++;
		}
	}

	//determine which substrings will be used
	for (expected_substring_length = PSG_SUBSTRING_MAX_LEN; expected_substring_length >= PSG_SUBSTRING_MIN_LEN; expected_substring_length--)
	{
		string_start_index = 0;
		while (string_start_index < in_buffer_length)
		{
			if (l_compression_info[string_start_index].Length == expected_substring_length)
			{
				// check the minimum length is really compressible
				string_index = string_start_index;
				substring_index = l_compression_info[string_start_index].Offset;
				matching_length = 0;
				while (matching_length < PSG_SUBSTRING_MIN_LEN)
				{
					// string must be uncompressed, unreferenced
					if (l_compression_info[string_index].Status != PSG_CBS_DATA)
						break;

					// substring must be uncomressed
					if (l_compression_info[substring_index].Status < PSG_CBS_DATA)
						break;

					string_index++;
					substring_index++;
					matching_length++;
				}

				// if minimum length is reached -> mark for string for compressed substrnig for referenced
				if (matching_length >= PSG_SUBSTRING_MIN_LEN)
				{
					string_index = string_start_index;
					substring_index = l_compression_info[string_start_index].Offset;
					matching_length = 0;
					while (matching_length < expected_substring_length)
					{
						// string must be uncompressed, unreferenced
						if (l_compression_info[string_index].Status != PSG_CBS_DATA)
							break;

						// substring must be uncomressed
						if (l_compression_info[substring_index].Status < PSG_CBS_DATA)
							break;

						l_compression_info[string_index].Status = PSG_CBS_COMPRESSED;
						//l_compression_info[string_index].Length = 0;
						l_compression_info[substring_index].Status = PSG_CBS_REFERENCED;
						//l_compression_info[substring_index].Length = 0;

						string_index++;
						substring_index++;
						matching_length++;
					}

					// update length of the compression
					l_compression_info[string_start_index].Length = matching_length;

				}
				else
				{
					//l_compression_info[string_start_index].Length = 0;
				}
			}

			string_start_index++;
		}
	}

	// remove short substring
	for (string_index = 0; string_index < in_buffer_length; string_index++)
	{
		if (l_compression_info[string_index].Length < PSG_SUBSTRING_MIN_LEN)
		{
			l_compression_info[string_index].Length = 0;
			l_compression_info[string_index].Offset = 0;
		}
	}

	// compress buffer content
	copy_from = 0;
	copy_to = 0;
	while (copy_from < in_buffer_length)
	{
		if (l_compression_info[copy_from].Length < PSG_SUBSTRING_MIN_LEN)
		{
			// copy content
			in_buffer[copy_to] = in_buffer[copy_from];
			copy_to++;
			copy_from++;
		}
		else
		{
			// compress content
			in_buffer[copy_to++] = l_compression_info[copy_from].Length + PSG_SUBSTRING - PSG_SUBSTRING_MIN_LEN;
			in_buffer[copy_to++] = l_compression_info[copy_from].Offset & 0xFF;
			in_buffer[copy_to++] = l_compression_info[copy_from].Offset >> 8;

			// update offsets pointing position after the current compression point
			delta = l_compression_info[copy_from].Length - 3;

			string_index = copy_from;
			while (string_index < in_buffer_length)
			{
				if (l_compression_info[string_index].Length > 0 && l_compression_info[string_index].Offset > copy_to)
					l_compression_info[string_index].Offset -= delta;

				string_index++;
			}

			copy_from += l_compression_info[copy_from].Length;
		}
	}

	return copy_to;
}

#if 0
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
#endif