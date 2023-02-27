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
#include <filePSG.h>
#include <drvWaveOut.h>
#include <emuSN76489.h>


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

///////////////////////////////////////////////////////////////////////////////
// Types

// VGM player state
typedef enum
{
	PSG_Idle,
	PSG_CommandProcessing,
	PSG_Waiting,
	PSG_RenderingBufferWaiting,
	PSG_Ending
} PSGPlayerState;


///////////////////////////////////////////////////////////////////////////////
// Local functions
static uint8_t filePSGGetNextByte(void);
static void filePSGWaitingAndRendering(void);
static void	filePSGRenderingBufferWaiting(void);
static void filePSGProcessCommand(void);
static void filePSGWaitingAndRendering(void);
static void	filePSGRenderingBufferWaiting(void);

///////////////////////////////////////////////////////////////////////////////
// Module variables

// PSG buffer
static uint8_t* l_psg_buffer;
static int l_psg_buffer_max_length;
static int l_psg_buffer_pos = 0;
static int l_frame_count = 0;

static PSGPlayerState l_player_state = PSG_Idle;

static uint8_t* l_psg_loop_start;
static uint32_t l_loop_start_remaining_bytes;

// get next byte variables
static uint8_t* l_psg_resume_pointer;
static uint32_t l_psg_resume_remaining_bytes;
static uint8_t* l_psg_current_pointer;
static uint32_t l_psg_current_remaining_bytes;
static uint32_t l_psg_current_frame_count;

// wait variables
static uint16_t l_wait_sample_count;
static uint16_t l_wait_sample_pos;

// rendering buffer
static int16_t* l_rendering_buffer;
static uint16_t l_rendering_buffer_length;
static uint16_t l_rendering_buffer_pos;

// positon variables
static uint32_t l_current_sample_pos = 0;

// PSG chip state
static emuSN76489State l_SN76489;

uint16_t g_frame_sample_count = 44100 / 50;


///////////////////////////////////////////////////////////////////////////////
// Initialize PSG player
void filePSGPlayerInit(void)
{
	l_SN76489.ClockFrequency = 3579545;
}

///////////////////////////////////////////////////////////////////////////////
// Pepares PSG file for playback
void filePSGPlayerStart(uint8_t* in_psg_buffer, int in_psg_file_length)
{
	l_psg_current_frame_count = 0;

	l_psg_buffer = in_psg_buffer;
	l_psg_buffer_max_length = in_psg_file_length;

	l_psg_current_pointer = in_psg_buffer;
	l_psg_current_remaining_bytes = in_psg_file_length;

	l_loop_start_remaining_bytes = 0;
	l_psg_loop_start = NULL;

	l_player_state = PSG_CommandProcessing;

	emuSN76489Reset(&l_SN76489);
}

///////////////////////////////////////////////////////////////////////////////
// Player periodic callback
void filePSGPlayerProcess(void)
{
	// process VHM commands
	switch (l_player_state)
	{
		// player isidle -> do nothing
		case PSG_Idle:
			break;

		// process VGM command
		case PSG_CommandProcessing:
			filePSGProcessCommand();
			break;

		// waiting for the next command and redering audio data
		case PSG_Waiting:
			filePSGWaitingAndRendering();
			break;

		// waiting for rendering buffer to available
		case PSG_RenderingBufferWaiting:
			filePSGRenderingBufferWaiting();
			break;

		// PSG end
		case PSG_Ending:
			if (!waveIsBusy())
				l_player_state = PSG_Idle;
			break;
	}
}

///////////////////////////////////////////////////////////////////////////////
// Returns true when player is busy
bool filePSGPlayerIsBusy(void)
{
	return l_player_state != PSG_Idle;
}

///////////////////////////////////////////////////////////////////////////////
// Returns current sample pos
uint32_t filePSGGetCurrentSamplePos(void)
{
	return l_psg_current_frame_count * g_frame_sample_count;
}

///////////////////////////////////////////////////////////////////////////////
// Sets playback framerate
void filePSGSetFramerate(int in_framerate)
{
	g_frame_sample_count = 44100 / in_framerate;
}

///////////////////////////////////////////////////////////////////////////////
// Set SN76489 clock frequency
void filePSGSetClockFrequency(int in_clock_frequency)
{
	l_SN76489.ClockFrequency = in_clock_frequency;
}

/*****************************************************************************/
/* Local functions                                                           */
/*****************************************************************************/

///////////////////////////////////////////////////////////////////////////////
// Processes one VGM command
static void filePSGProcessCommand(void)
{
	uint8_t command;

	while (l_player_state == PSG_CommandProcessing)
	{

		// gets next byte
		command = filePSGGetNextByte();

		// register latch command
		if (IS_LATCH_BYTE(command))
		{
			emuSN76496WriteRegister(&l_SN76489, command);
		}
		else
		{
			// data byte
			if (IS_DATA_BYTE(command))
			{
				emuSN76496WriteRegister(&l_SN76489, command);
			}
			else
			{
				// must be escape
				if (IS_COMPRESSION(command))
				{
					uint8_t posl = filePSGGetNextByte();
					uint8_t posh = filePSGGetNextByte();

					uint16_t compression_pos = (uint16_t)((posh << 8) + posl);

					l_psg_resume_pointer = l_psg_current_pointer;
					l_psg_resume_remaining_bytes = l_psg_current_remaining_bytes;

					l_psg_current_pointer = l_psg_buffer + compression_pos;
					l_psg_current_remaining_bytes = GET_COMPRESSION_LENGTH(command);
				}
				else
				{
					// end of frame
					if (IS_END_OF_FRAME(command))
					{
						// start waiting
						l_wait_sample_count = GET_WAIT_FRAME_COUNT(command) * g_frame_sample_count;
						l_wait_sample_pos = 0;

						l_psg_current_frame_count += GET_WAIT_FRAME_COUNT(command);

						l_player_state = PSG_Waiting;
					}
					else
					{
						if (IS_BEGIN_LOOP(command))
						{
							l_psg_loop_start = l_psg_current_pointer;
							l_loop_start_remaining_bytes = l_psg_current_remaining_bytes;
						}
						else
						{
							// end of file
							if (IS_END_OF_FILE(command))
							{
								if (l_loop_start_remaining_bytes > 0)
								{
									l_psg_current_pointer = l_psg_loop_start;
									l_psg_current_remaining_bytes = l_loop_start_remaining_bytes;
								}
								else
								{
									waveGetBuffer(); // send current buffer to the wave out
									l_player_state = PSG_Ending;
								}
							}
						}
					}
				}
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// Gets next byte from the PSG data
static uint8_t filePSGGetNextByte(void)
{
	// if no more bytes -> resume position
	if (l_psg_current_remaining_bytes == 0)
	{
		l_psg_current_pointer = l_psg_resume_pointer;
		l_psg_current_remaining_bytes = l_psg_resume_remaining_bytes;
	}

	// get byte
	uint8_t data = *l_psg_current_pointer;

	l_psg_current_pointer++;
	l_psg_current_remaining_bytes--;

	return data;
}

///////////////////////////////////////////////////////////////////////////////
// Renders audio stream
static void filePSGWaitingAndRendering(void)
{
	uint16_t sample_count;
	uint16_t wait_sample_count;

	// check rendering buffer
	if (l_rendering_buffer == NULL || l_rendering_buffer_length == 0 || l_rendering_buffer_pos >= l_rendering_buffer_length)
	{
		// get new rendering buffer
		l_player_state = PSG_RenderingBufferWaiting;
	}
	else
	{
		if (l_wait_sample_pos >= l_wait_sample_count)
		{
			l_player_state = PSG_CommandProcessing;
		}
		else
		{
			// number of free sample places in to buffer
			sample_count = l_rendering_buffer_length - l_rendering_buffer_pos;
			if (g_stereo_mode)
				sample_count /= 2;

			// number of remaining wait samples
			wait_sample_count = l_wait_sample_count - l_wait_sample_pos;

			// min(wait_sample_count, available_sample_count)
			if (wait_sample_count < sample_count)
				sample_count = wait_sample_count;

			// render audio
			emuSN76489RenderAudioStream(&l_SN76489, &l_rendering_buffer[l_rendering_buffer_pos], sample_count, 1);

			// update buffer position
			if (g_stereo_mode)
				l_rendering_buffer_pos += sample_count * 2;
			else
				l_rendering_buffer_pos += sample_count;

			l_wait_sample_pos += sample_count;

			if (l_rendering_buffer_pos >= l_rendering_buffer_length)
				l_player_state = PSG_RenderingBufferWaiting;

			l_current_sample_pos += sample_count;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// Waiting for rendering buffer to be available
static void	filePSGRenderingBufferWaiting(void)
{
	int16_t* buffer = waveGetBuffer();
	uint16_t i;

	if (buffer != NULL)
	{
		l_rendering_buffer = buffer;

		// clear buffer
		for (i = 0; i < WAVE_BUFFER_LENGTH; i++)
			*buffer++ = 0;

		// buffer size in samples
		l_rendering_buffer_length = WAVE_BUFFER_LENGTH;

		// init
		l_rendering_buffer_pos = 0;

		// change state
		l_player_state = PSG_Waiting;
	}
}

