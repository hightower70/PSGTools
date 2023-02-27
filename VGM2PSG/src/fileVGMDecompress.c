/*****************************************************************************/
/* VGM2PSG VGM Loader/decompressor                                           */
/*                                                                           */
/* Copyright (C) 2023 Laszlo Arvai                                           */
/* All rights reserved.                                                      */
/*                                                                           */
/* This software may be modified and distributed under the terms             */
/* of the BSD license.  See the LICENSE file for details.                    */
/*****************************************************************************/
#include <tinfl.c>
#include <stdio.h>
#include <string.h>
#include <Main.h>
#include <fileVGMDecompress.h>

///////////////////////////////////////////////////////////////////////////////
// Defines
#define MAGIC_GZIP 0x8b1f
#define MAGIC_VGM 0x6756

#define COMPRESSION_METHOD_DEFLATE 8

// GZIP header flags
#define GZIP_HF_FTEXT			0x01
#define GZIP_HF_FHCRC			0x02
#define GZIP_HF_FEXTRA		0x04
#define GZIP_HF_FNAME			0x08
#define GZIP_HF_FCOMMENT	0x10

#pragma pack(push, 1)


///////////////////////////////////////////////////////////////////////////////
// Types

// GZIP File header
typedef struct
{
	uint16_t Magic;
	uint8_t CompressionMethod;
	uint8_t Flags;
	uint32_t Timestamp;
	uint8_t CompressionFlags;
	uint8_t OperationSystem;
} GZIPHeader;

#pragma pack(pop)

///////////////////////////////////////////////////////////////////////////////
// Local functions
static void fileVGMSkipString(uint8_t* in_buffer, int* inout_pos);

///////////////////////////////////////////////////////////////////////////////
// Module local variables
static uint8_t l_file_buffer[FILE_BUFFER_LENGTH];

///////////////////////////////////////////////////////////////////////////////
// Loads VGM file into the given buffer. If file is compressed, deflates it.
int fileVGMLoad(char* in_filename, uint8_t* in_buffer, int in_buffer_length)
{
	FILE* vgm_file;
	int vgm_file_length;
	uint16_t magic;
	int data_start_pos;

	// load file into temp buffer
	vgm_file = fopen(in_filename, "rb");
	if (vgm_file == NULL)
		return 0;

	vgm_file_length = fread(l_file_buffer, 1, FILE_BUFFER_LENGTH, vgm_file);
	if (vgm_file_length == 0 || vgm_file_length == FILE_BUFFER_LENGTH)
		return 0;

	fclose(vgm_file);

	// determine file type
	magic = l_file_buffer[0] + 256 * l_file_buffer[1];

	// if file is a noncompressed VGM file
	if (magic == MAGIC_VGM)
	{
		memcpy(in_buffer, l_file_buffer, vgm_file_length);
		return vgm_file_length;
	}

	// if file is not compressed -> unknown file, error
	if (magic != MAGIC_GZIP)
		return 0;

	// handle compressed file
	GZIPHeader* gzip_header = (GZIPHeader*)&l_file_buffer[0];

	if (gzip_header->CompressionMethod != COMPRESSION_METHOD_DEFLATE)
		return 0;

	data_start_pos = sizeof(GZIPHeader);

	if ((gzip_header->Flags & GZIP_HF_FNAME) != 0)
	{
		fileVGMSkipString(l_file_buffer, &data_start_pos);
	}

	if ((gzip_header->Flags & GZIP_HF_FCOMMENT) != 0)
	{
		fileVGMSkipString(l_file_buffer, &data_start_pos);
	}

	if ((gzip_header->Flags & GZIP_HF_FHCRC) != 0)
	{
		data_start_pos += 2;
	}

	vgm_file_length = (int)tinfl_decompress_mem_to_mem(in_buffer, FILE_BUFFER_LENGTH, &l_file_buffer[data_start_pos], vgm_file_length - data_start_pos, 0);

	return vgm_file_length;
}

///////////////////////////////////////////////////////////////////////////////
// Skips string in the GZIP header
static void fileVGMSkipString(uint8_t* in_buffer, int* inout_pos)
{
	int pos = *inout_pos;

	while (in_buffer[pos] != '\0')
		pos++;

	*inout_pos = pos;
}