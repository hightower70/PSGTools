/*****************************************************************************/
/* VGM2PSG Output File Writer                                                */
/*                                                                           */
/* Copyright (C) 2023 Laszlo Arvai                                           */
/* All rights reserved.                                                      */
/*                                                                           */
/* This software may be modified and distributed under the terms             */
/* of the BSD license.  See the LICENSE file for details.                    */
/*****************************************************************************/

///////////////////////////////////////////////////////////////////////////////
// Includes
#include <fileOutput.h>
#include <stdio.h>

///////////////////////////////////////////////////////////////////////////////
// Defines
#define BYTE_COUNT_IN_LINE 16

///////////////////////////////////////////////////////////////////////////////
// Module global variables
static FILE* l_output_file;
static bool l_text_mode = false;
static int l_file_length;

//////////////////////////////////////////////////////////////////////////////
// Creates output file in binary ot text mode
bool fileOutputCreate(char* in_filename, bool in_text_mode)
{
	// init
	l_text_mode = in_text_mode;
	l_file_length = 0;

	// create file
	if (in_text_mode)
	{
		l_output_file = fopen(in_filename, "wt");
	}
	else
	{
		l_output_file = fopen(in_filename, "wb");
	}

	return l_output_file != NULL;
}

///////////////////////////////////////////////////////////////////////////////
// Writes block of binary data
void fileOutputWriteBlock(uint8_t* in_data, int in_data_length)
{
	int pos;

	if (l_text_mode)
	{
		for (pos = 0; pos < in_data_length; pos++)
		{
			// start a new line if required
			if ((l_file_length % BYTE_COUNT_IN_LINE) == 0)
			{
				if (l_file_length > 0)
				{
					fprintf(l_output_file, "\n");
				}
				fprintf(l_output_file, "    .db ");
			}
			else
			{
				fprintf(l_output_file, ", ");
			}

			fprintf(l_output_file, "0%02Xh", in_data[pos]);
		}
	}
	else
	{
		fwrite(in_data, sizeof(uint8_t), in_data_length, l_output_file);
		l_file_length += in_data_length;
	}
}

///////////////////////////////////////////////////////////////////////////////
// Closes output file
void fileOutputClose(void)
{
	if (l_output_file != NULL)
	{
		fclose(l_output_file);
		l_output_file = NULL;
	}
}

