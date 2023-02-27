/*****************************************************************************/
/* PSG2TXT - Converts PSG filt to simple text explanation                    */
/*                                                                           */
/* Copyright (C) 2023 Laszlo Arvai                                           */
/* All rights reserved.                                                      */
/*                                                                           */
/* This software may be modified and distributed under the terms             */
/* of the BSD license.  See the LICENSE file for details.                    */
/*****************************************************************************/
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

///////////////////////////////////////////////////////////////////////////////
// PSG File format description
///////////////////////////////////////////////////////////////////////////////
//PSG simplistic approach(log of writes to SN76489 port)
//
//- No header
//- % 1cct xxxx = Latch / Data byte for SN76489 channel c, type t, data xxxx(4 bits)
//- % 01xx xxxx = Data byte for SN76489 latched channel and type, data xxxxxx(6 bits)
//- % 00xx xxxx = escape / control byte(values 0x00 - 0x3f), see following table #1
//
//Table #1
//
//% 0000 0000 - end of data[value 0x00](compulsory, at the end of file)
//
//% 0000 0001 - loop begin marker[value 0x01](optional, songs with no loop won't have this)
//
//	% 0000 0nnn - RESERVED for future expansions[values 0x02 - 0x07]
//	* PLANNED: GameGear stereo - the following byte sets the stereo configuration
//	* PLANNED : event callback - the following byte will be passed to the callback function
//	* PLANNED : longer waits(8 - 255) - the following byte gives the additional frames
//	* PLANNED : compression for longer substrings(52 - 255) - followed by a byte that gives the length
//	and a word that gives the offset
// 
//	%0000 1xxx - COMPRESSION: repeat block of len 4 - 11 bytes
//	%0001 xxxx - COMPRESSION: repeat block of len 12 - 27 bytes
//	%0010 xxxx - COMPRESSION: repeat block of len 28 - 43
//	%0011 0xxx - COMPRESSION: repeat block of len 44 - 51 [values 0x08 - 0x37]
//	This is followed by a little - endian word which is the offset(from begin of data) of the repeating block
//
//	% 0011 1nnn - end of frame, wait nnn additional frames(0 - 7)[values 0x38 - 0x3f]
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// Defines
#define IS_LATCH_BYTE(x) (((x)&0x80)!= 0)
#define IS_DATA_BYTE(x) (((x)&0xc0)==0x40)
#define IS_ESCAPE_BYTE(x) (((x)&0xc0)==0x00)
#define IS_END_OF_FRAME(x) (((x)&0xf8)==0x38)
#define IS_END_OF_FILE(x) ((x)==0)
#define IS_BEGIN_LOOP(x) ((x)==0x01)
#define IS_COMPRESSION(x) ((x)>=8 && (x)<=8+MAX_COMPRESSION_LENGTH-MIN_COMPRESSON_LENGTH)

#define MIN_COMPRESSON_LENGTH 4
#define MAX_COMPRESSION_LENGTH 51 // 47+4
#define GET_COMPRESSION_LENGTH(x) ((x)-8+MIN_COMPRESSON_LENGTH)
#define GET_WAIT_FRAME_COUNT(x) (((x)&0x07)+1)
#define GET_LATCH_REGISTER(x) (((x)&0x70)>>4)
#define GET_LATCH_VALUE(x) ((x)&0x0f)

#define SN76489REG_CH0_TONE		0
#define SN76489REG_CH0_ATT		1
#define SN76489REG_CH1_TONE		2
#define SN76489REG_CH1_ATT		3
#define SN76489REG_CH2_TONE		4
#define SN76489REG_CH2_ATT		5
#define SN76489REG_NOISE_CTRL	6
#define SN76489REG_NOISE_ATT	7

#define PSG_REGISTER_COUNT 8
#define FILE_BUFFER_SIZE 64*1024

static uint8_t l_psg_buffer[FILE_BUFFER_SIZE];
static int l_psg_length;

///////////////////////////////////////////////////////////////////////////////
// Local functions
static bool PSGProcessCommand(void);
static uint8_t filePSGGetNextByte(void);

static uint16_t l_psg_registers[PSG_REGISTER_COUNT];
static uint8_t l_psg_latch_register;

// get next byte variables
static uint32_t l_psg_resume_index;
static uint32_t l_psg_resume_remaining_bytes;
static uint32_t l_psg_current_index;
static uint32_t l_psg_current_remaining_bytes;
static uint32_t l_psg_current_frame_count;

///////////////////////////////////////////////////////////////////////////////
// Main functions
int main(int argc, char* argv[])
{
	int i;
  FILE* psg_file;

  if (argc != 2)
  {
    printf("Usage: psg2txt inputfile.PSG\n");
    return (1);
  }

  psg_file = fopen(argv[1], "rb");
  l_psg_length = (int)fread(&l_psg_buffer, 1, FILE_BUFFER_SIZE, psg_file);      // read input file
  fclose(psg_file);

	// initialize
	l_psg_latch_register = 0;
	for (i = 0; i < PSG_REGISTER_COUNT; i++)
		l_psg_registers[i] = 0;

	l_psg_current_index = 0;
	l_psg_current_remaining_bytes = l_psg_length;;
	l_psg_current_frame_count = 0;

  printf("Info: input file size is %d bytes\n", l_psg_length);

	while (PSGProcessCommand());

	return 0;
}

/*****************************************************************************/
/* Local functions                                                           */
/*****************************************************************************/

///////////////////////////////////////////////////////////////////////////////
// Processes one PSG command
static bool PSGProcessCommand(void)
{
	uint8_t command;
	bool retval = true;

	// gets next byte
	printf("0x%04X: ", l_psg_current_index);
	command = filePSGGetNextByte();
	printf("0x%02X    ", command);

	// register latch command
	if (IS_LATCH_BYTE(command))
	{
		l_psg_latch_register = GET_LATCH_REGISTER(command);
		switch (l_psg_latch_register)
		{
			case SN76489REG_CH0_TONE:
			case SN76489REG_CH1_TONE:
			case SN76489REG_CH2_TONE:
				l_psg_registers[l_psg_latch_register] = (l_psg_registers[l_psg_latch_register] & 0x3f0) | GET_LATCH_VALUE(command);
				printf("Latch/Data: Tone Ch #%d -> 0x%03X\n", l_psg_latch_register / 2, l_psg_registers[l_psg_latch_register]);
				break;

			case SN76489REG_CH0_ATT:
			case  SN76489REG_CH1_ATT:
			case  SN76489REG_CH2_ATT:
			case SN76489REG_NOISE_ATT:
				l_psg_registers[l_psg_latch_register] = GET_LATCH_VALUE(command);
				printf("Latch/Data: Volume Ch #%d -> 0x%02X (%d%%)\n", l_psg_latch_register / 2, GET_LATCH_VALUE(command), (15 - GET_LATCH_VALUE(command)) * 100 / 15);
				break;

			case SN76489REG_NOISE_CTRL:
				printf("Noise Type: ");
				if ((command & 0x04) != 0)
					printf("white, ");
				else
					printf("periodic, ");
				switch (command & 0x03)
				{
					case 0:
						printf("low (N/512)\n");
						break;

					case 1:
						printf("medium (N/1024)\n");
						break;

					case 2:
						printf("high (N/2048)\n");
						break;

					case 3:
						printf("tone #3\n");
						break;

					default:
						break;
				}
				break;

			default:
				break;
		}
	}
	else
	{
		// data byte
		if (IS_DATA_BYTE(command))
		{
			switch (l_psg_latch_register)
			{
				case SN76489REG_CH0_TONE:
				case SN76489REG_CH1_TONE:
				case SN76489REG_CH2_TONE:
					l_psg_registers[l_psg_latch_register] = ((command & 0x3f) << 4) | (l_psg_registers[l_psg_latch_register] & 0xf);
					printf("      Data: Tone Ch #%d -> 0x%03X\n", l_psg_latch_register / 2, l_psg_registers[l_psg_latch_register]);
					break;
			}
		}
		else
		{
			// must be escape
			if (IS_COMPRESSION(command))
			{
				uint8_t posl = filePSGGetNextByte();
				uint8_t posh = filePSGGetNextByte();

				uint16_t compression_pos = (uint16_t)((posh << 8) + posl);

				printf("<< compression pos: 0x%04X, length: %2d       >>\n", compression_pos, GET_COMPRESSION_LENGTH(command));

				l_psg_resume_index = l_psg_current_index;
				l_psg_resume_remaining_bytes = l_psg_current_remaining_bytes;

				l_psg_current_index = compression_pos;
				l_psg_current_remaining_bytes = GET_COMPRESSION_LENGTH(command);
			}
			else
			{
				// end of frame
				if (IS_END_OF_FRAME(command))
				{
					printf("---------- end of frame ------------ (%02d:%02d.%02d)\n", l_psg_current_frame_count / 50 / 60, (l_psg_current_frame_count / 50) % 60, (l_psg_current_frame_count * 2) % 100);
					l_psg_current_frame_count += GET_WAIT_FRAME_COUNT(command);
				}
				else
				{
					if (IS_BEGIN_LOOP(command))
					{
						printf("begin loop\n");
					}
					else
					{
						// end of file
						if (IS_END_OF_FILE(command))
						{
							retval = false;
						}
					}
				}
			}
		}
	}

	return retval;
}


///////////////////////////////////////////////////////////////////////////////
// Gets next byte from the PSG data
static uint8_t filePSGGetNextByte(void)
{
	// if no more bytes -> resume position
	if (l_psg_current_remaining_bytes == 0)
	{
		l_psg_current_index = l_psg_resume_index;
		l_psg_current_remaining_bytes = l_psg_resume_remaining_bytes;
	}

	// get byte
	uint8_t data = l_psg_buffer[l_psg_current_index];

	l_psg_current_index++;
	l_psg_current_remaining_bytes--;

	return data;
}
