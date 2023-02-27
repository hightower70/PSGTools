/*****************************************************************************/
/* VGMFile - TVC Game Music File Creator                                     */
/* TGM File Handing                                                          */
/*                                                                           */
/* Copyright (C) 2023 Laszlo Arvai                                           */
/* All rights reserved.                                                      */
/*                                                                           */
/* This software may be modified and distributed under the terms             */
/* of the BSD license.  See the LICENSE file for details.                    */
/*****************************************************************************/

///////////////////////////////////////////////////////////////////////////////
// Includes
#include <stdio.h>
#include <string.h>
#include <filePSG.h>
#include <Main.h>

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
//	% 0000 1xxx &
//	%0001 xxxx &
//	%0010 xxxx &
//	%0011 0xxx - COMPRESSION: repeat block of len 4 - 51[values 0x08 - 0x37]
//	This is followed by a little - endian word which is the offset(from begin of data) of the repeating block
//
//	% 0011 1nnn - end of frame, wait nnn additional frames(0 - 7)[values 0x38 - 0x3f]
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// Defines
#define IS_ATTENUATION_REGISTER(x) (((x) & 1) != 0)
#define IS_NOISE_CONTROL_REGISTER(x) ((x) == 6)

#define PSG_WRITE_LATCH(r, d) (0x80 + ((r) << 4) + (d))
#define PSG_WRITE_DATA(d) (0x40 + ((d) & 0x3f))
#define PSG_WRITE_END_OF_FRAME(w) (0x38 + ((w)&0x07))
#define PSG_END_OF_DATA 0x00
#define PSG_IS_END_OF_FRAME(x) (((x)&0xf8)==0x38)
#define PSG_READ_WAIT_COUNT(x) ((x)&0x07)
#define PSG_MAX_WAIT_COUNT 7
#define PSG_LOOP_START 0x01

#define PSG_SUBSTRING									0x08
#define PSG_SUBSTRING_MIN_LEN         4
#define PSG_SUBSTRING_MAX_LEN         51        // 47+4

#define PSG_CBS_UNUSED			0
#define PSG_CBS_REFERENCED	1
#define PSG_CBS_SUBSTRING		2
#define PSG_CBS_OFFSET			3

///////////////////////////////////////////////////////////////////////////////
// Local functions


///////////////////////////////////////////////////////////////////////////////
// Module variables
static uint8_t* l_psg_buffer;
static int l_psg_buffer_max_length;
static int l_psg_buffer_pos = 0;
static int l_frame_count = 0;
static bool l_prev_behind_loop_start = false;
static int l_last_register_index = -1;

static uint8_t l_compression_buffer_state[FILE_BUFFER_LENGTH];
///////////////////////////////////////////////////////////////////////////////
// Creates empty PSG file in memory buffer
void filePSGStart(uint8_t* in_psg_buffer, int in_psg_buffer_length)
{
	l_psg_buffer_max_length = in_psg_buffer_length;
	l_psg_buffer = in_psg_buffer;
	l_psg_buffer_pos = 0;
	l_prev_behind_loop_start = false;
	l_last_register_index = -1;
}

///////////////////////////////////////////////////////////////////////////////
// Writes one frame into the PSG memory file
void filePSGUpdate(emuSN76489State* in_SN76489_state, bool in_behind_loop_start)
{
	int register_index;
	bool register_changed = false;

	// write register values
	for (register_index = 0; register_index < emuSN76489_REGISTER_COUNT; register_index++)
	{
		// if register is an attenuation register
		if (IS_ATTENUATION_REGISTER(register_index))
		{
			// and it has been changed
			if (in_SN76489_state->Registers[register_index] != in_SN76489_state->PrevRegisters[register_index])
			{
				// write attenuation register write command
				l_psg_buffer[l_psg_buffer_pos++] = PSG_WRITE_LATCH(register_index, in_SN76489_state->Registers[register_index] & 0x0f);
				
				l_last_register_index = register_index;
				register_changed = true;
			}
		}
		else
		{
			// if register is the noise control register
			if (IS_NOISE_CONTROL_REGISTER(register_index))
			{
				if (in_SN76489_state->NoiseRegisterChanged)
				{
					// write noise control register write command
					l_psg_buffer[l_psg_buffer_pos++] = PSG_WRITE_LATCH(register_index, in_SN76489_state->Registers[register_index] & 0x07);
					l_last_register_index = register_index;
					register_changed = true;
				}
			}
			else
			{
				// tone registers
				if (in_SN76489_state->Registers[register_index] != in_SN76489_state->PrevRegisters[register_index])
				{
					// tone register low bits
					//if (((in_SN76489_state->Registers[register_index] & 0xf) != (in_SN76489_state->PrevRegisters[register_index] & 0xf)) || l_last_register_index != register_index)
					{
						// write tone register write command
						l_psg_buffer[l_psg_buffer_pos++] = PSG_WRITE_LATCH(register_index, in_SN76489_state->Registers[register_index] & 0x0f);
						l_last_register_index = register_index;
						register_changed = true;
					}

					// tone registers high bits
					if ((in_SN76489_state->Registers[register_index] & 0x3f0) != (in_SN76489_state->PrevRegisters[register_index] & 0x3f0))
					{
						l_psg_buffer[l_psg_buffer_pos++] = PSG_WRITE_DATA((in_SN76489_state->Registers[register_index] >> 4) & 0x3f);
						register_changed = true;
					}
				}
			}
		}
	}

	// close frame
	if (register_changed)
	{
		l_psg_buffer[l_psg_buffer_pos++] = PSG_WRITE_END_OF_FRAME(0);
	}
	else
	{
		int wait_count;

		// empty frame, try to update the previous frame end with wait count
		if (PSG_IS_END_OF_FRAME(l_psg_buffer[l_psg_buffer_pos - 1]))
		{
			// increase wait count
			wait_count = PSG_READ_WAIT_COUNT(l_psg_buffer[l_psg_buffer_pos - 1]);
			if (wait_count < PSG_MAX_WAIT_COUNT)
			{
				wait_count++;
				l_psg_buffer[l_psg_buffer_pos - 1] = PSG_WRITE_END_OF_FRAME(wait_count);
			}
			else
			{
				// wait count reached the amximum value -> insert new end of frame
				l_psg_buffer[l_psg_buffer_pos++] = PSG_WRITE_END_OF_FRAME(0);
			}
		}
		else
		{
			l_psg_buffer[l_psg_buffer_pos++] = PSG_WRITE_END_OF_FRAME(0);
		}
	}

	if (in_behind_loop_start && !l_prev_behind_loop_start)
	{
		l_psg_buffer[l_psg_buffer_pos++] = PSG_LOOP_START;
	}
	l_prev_behind_loop_start = in_behind_loop_start;

	l_frame_count++;
	emuSN76489ClearRegisterChanged(in_SN76489_state);
}

///////////////////////////////////////////////////////////////////////////////
// Closes PSG memory file
void filePSGFinish(void)
{
	l_psg_buffer[l_psg_buffer_pos++] = PSG_END_OF_DATA;
}


///////////////////////////////////////////////////////////////////////////////
// Gets PSG memory file length
int filePSGGetLength(void)
{
	return l_psg_buffer_pos;
}


#if 0
int filePSGCompress1(uint8_t* in_buffer, int in_buffer_length)
{
	int current_start_index;
	int current_index;
	int substring_index;
	int substring_start_index;
	int substring_length;
	int copy_from;
	int copy_to;
	int copy_count;
	int expected_substring_length;
	int offset;

	// no compression for short files
	if (in_buffer_length < PSG_SUBSTRING_MIN_LEN)
		return;

	// mark status as unused
	for (current_index = 0; current_index < in_buffer_length; current_index++)
		l_compression_buffer_state[current_index] = PSG_CBS_UNUSED;

	// start compression
	for (expected_substring_length = PSG_SUBSTRING_MAX_LEN; expected_substring_length >= PSG_SUBSTRING_MIN_LEN; expected_substring_length--)
	{
		printf(".");

		for (current_start_index = expected_substring_length; current_start_index < in_buffer_length - expected_substring_length; current_start_index++)
		{
			if (l_compression_buffer_state[current_start_index] != PSG_CBS_UNUSED)
				continue;

			// find substring
			substring_start_index = 0;
			while (substring_start_index < current_start_index)
			{
				if (l_compression_buffer_state[substring_start_index] > PSG_CBS_REFERENCED)
				{
					substring_start_index++;
					continue;
				}

				substring_index = substring_start_index;
				current_index = current_start_index;
				while (in_buffer[substring_index] == in_buffer[current_index] &&
					(current_index - current_start_index) < expected_substring_length &&
					l_compression_buffer_state[substring_index] < PSG_CBS_SUBSTRING &&
					l_compression_buffer_state[current_index] == PSG_CBS_UNUSED)
				{
					substring_index++;
					current_index++;
				}

				substring_length = current_index - current_start_index;

				if (substring_length == expected_substring_length)
				{
					// mark referenced bytes
					for (current_index = substring_start_index; current_index < substring_start_index + substring_length; current_index++)
						l_compression_buffer_state[current_index] = PSG_CBS_REFERENCED;

					in_buffer[current_start_index] = (substring_length - PSG_SUBSTRING_MIN_LEN) + PSG_SUBSTRING;
					in_buffer[current_start_index + 1] = (substring_start_index & 0xFF);
					in_buffer[current_start_index + 2] = (substring_start_index >> 8);

					l_compression_buffer_state[current_start_index] = PSG_CBS_SUBSTRING;
					l_compression_buffer_state[current_start_index + 1] = PSG_CBS_OFFSET;
					l_compression_buffer_state[current_start_index + 2] = PSG_CBS_OFFSET;

					// compact remaining bytes
					copy_from = current_start_index + substring_length;
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

					// update offsets
					for (current_index = current_start_index + 3; current_index < in_buffer_length; current_index++)
					{
						if (l_compression_buffer_state[current_index] == PSG_CBS_SUBSTRING)
						{
							offset = in_buffer[current_index + 1] + (in_buffer[current_index + 2] << 8);

							if (offset > current_start_index)
							{
								offset -= substring_length - 3;
								in_buffer[current_index + 1] = offset & 0xff;
								in_buffer[current_index + 2] = offset >> 8;
							}

						}
					}

					in_buffer_length -= substring_length - 3;
					current_start_index += 2;

					break;
				}

				substring_start_index++;
			}
		}
	}

	return in_buffer_length;
}
#endif