/*****************************************************************************/
/* VGMFile - Video Game Music Player                                         */
/* VGM File Handing                                                          */
/*                                                                           */
/* Copyright (C) 2013 Laszlo Arvai                                           */
/* All rights reserved.                                                      */
/*                                                                           */
/* This software may be modified and distributed under the terms             */
/* of the BSD license.  See the LICENSE file for details.                    */
/*****************************************************************************/

///////////////////////////////////////////////////////////////////////////////
// Includes
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <fileVGM.h>
#include <Main.h>

///////////////////////////////////////////////////////////////////////////////
// Constants

// VGM File soecific constants
#define VGM_MAX_COMMAND_LENGTH 5

// VGM commands
#define VGM_CMD_GG_STEREO               0x4F
#define VGM_CMD_PSG                     0x50
#define VGM_CMD_PSG_2nd									0x30
#define VGM_CMD_YM2413                  0x51
#define VGM_CMD_YM2612_0                0x52
#define VGM_CMD_YM2612_1                0x53
#define VGM_CMD_2151                    0x54
#define VGM_CMD_WAIT                    0x61
#define VGM_CMD_WAIT_735                0x62
#define VGM_CMD_WAIT_882                0x63
#define VGM_CMD_WAIT_1	                0x70
#define VGM_CMD_WAIT_2	                0x71
#define VGM_CMD_WAIT_3	                0x72
#define VGM_CMD_WAIT_4	                0x73
#define VGM_CMD_WAIT_5	                0x74
#define VGM_CMD_WAIT_6	                0x75
#define VGM_CMD_WAIT_7	                0x76
#define VGM_CMD_WAIT_8	                0x77
#define VGM_CMD_WAIT_9	                0x78
#define VGM_CMD_WAIT_10	                0x79
#define VGM_CMD_WAIT_11	                0x7a
#define VGM_CMD_WAIT_12	                0x7b
#define VGM_CMD_WAIT_13	                0x7c
#define VGM_CMD_WAIT_14	                0x7d
#define VGM_CMD_WAIT_15	                0x7e
#define VGM_CMD_WAIT_16	                0x7f
#define VGM_PAUSE_BYTE                  0x64
#define VGM_CMD_EOF                     0x66
#define VGM_CMD_DATA_BLOCK              0x67
#define VGM_CMD_YM2612_PCM_SEEK         0xE0

#define VGM_MAX_VOLUME									32767

///////////////////////////////////////////////////////////////////////////////
// Types

// VGM player state
typedef enum
{
	VPS_CommandProcessing,
	VPS_Waiting,
	VPS_Finished
} VGMPlayerState;

///////////////////////////////////////////////////////////////////////////////
// Global variables
VGMFileHeaderType g_vgm_file_header;

///////////////////////////////////////////////////////////////////////////////
// Module global variables
static uint8_t* l_vgm_file_buffer;
static uint32_t l_vgm_file_pos;

static VGMPlayerState l_player_state = VPS_CommandProcessing;
static bool l_looping;

// positon variables
static uint32_t l_current_sample_pos = 0;
static uint32_t l_target_sample_pos = 0;


///////////////////////////////////////////////////////////////////////////////
// Local functions
static void fileVGMProcessCommand(void);

///////////////////////////////////////////////////////////////////////////////
// Loads VGM file
bool fileVGMOpen(uint8_t* in_buffer, int in_buffer_length)
{
	bool success = true;

	l_vgm_file_buffer = in_buffer;

	// load header
	memcpy(&g_vgm_file_header, l_vgm_file_buffer, sizeof(g_vgm_file_header));

	// check header
	if(g_vgm_file_header.VGMIdent != VGM_IDENT)
		success = false;

	// determine data offset
	if(success)
	{
		if (g_vgm_file_header.Version < 0x150)
			g_vgm_file_header.DataOffset = 0x40;
		else
			if (g_vgm_file_header.DataOffset == 0)
				g_vgm_file_header.DataOffset = 0x40;
			else
				g_vgm_file_header.DataOffset += 0x34;
	}

	l_current_sample_pos = 0;
	l_target_sample_pos = 0;

	return success;
}

///////////////////////////////////////////////////////////////////////////////
// Closes input stream
void fileVGMClose(void)
{
}

///////////////////////////////////////////////////////////////////////////////
// Starts VGM player
void fileVGMPlayerStart(void)
{
	// start to play
	l_vgm_file_pos = g_vgm_file_header.DataOffset;
	l_current_sample_pos = 0;
	l_target_sample_pos = 0;
	l_looping = false;
	l_player_state = VPS_CommandProcessing;
}

///////////////////////////////////////////////////////////////////////////////
// Player periodic callback
void fileVGMPlayerProcess(int in_sample_pos_increment)
{
	l_target_sample_pos += in_sample_pos_increment;

	// process VGM commands
	while (l_player_state == VPS_CommandProcessing ||
		     (l_player_state == VPS_Waiting && l_current_sample_pos < l_target_sample_pos))
	{
		fileVGMProcessCommand();
	}
}

///////////////////////////////////////////////////////////////////////////////
// Returns true when player is busy
bool fileVGMPlayerIsBusy(void)
{
	return l_player_state != VPS_Finished;
}

///////////////////////////////////////////////////////////////////////////////
// Returns number of total samples
uint32_t fileVGMGetTotalSampleCount(void)
{
	return g_vgm_file_header.TotalNumberOfSamples + g_vgm_file_header.LoopNumberOfSamples;
}

///////////////////////////////////////////////////////////////////////////////
// Returns current sample pos
uint32_t fileVGMGetCurrentSamplePos(void)
{
	return l_current_sample_pos;
}

///////////////////////////////////////////////////////////////////////////////
// True if file position is behind the loop start position
bool fileVGMIsBehindLoopStart(void)
{
	return l_vgm_file_pos >= g_vgm_file_header.LoopOffset + offsetof(VGMFileHeaderType, LoopOffset);
}

/*****************************************************************************/
/* Local functions                                                           */
/*****************************************************************************/

///////////////////////////////////////////////////////////////////////////////
// Processes one VGM command
static void fileVGMProcessCommand(void)
{
	uint8_t command;
	uint16_t word_buffer;
	uint8_t byte_buffer;

	l_player_state = VPS_CommandProcessing;

	// process command
	command = l_vgm_file_buffer[l_vgm_file_pos++];
	switch(command)
	{
		case VGM_CMD_PSG:
			byte_buffer = l_vgm_file_buffer[l_vgm_file_pos++];
			emuN76496WriteRegister(&g_SN76489_state, byte_buffer);
			break;

		case VGM_CMD_PSG_2nd:
			byte_buffer = l_vgm_file_buffer[l_vgm_file_pos++];
			break;

		case VGM_CMD_WAIT:
			// get sample length
			word_buffer = l_vgm_file_buffer[l_vgm_file_pos] + (l_vgm_file_buffer[l_vgm_file_pos+1] << 8);
			l_vgm_file_pos += 2;

			// start waiting
			l_current_sample_pos += word_buffer;

			l_player_state = VPS_Waiting;
			break; 
				
		case VGM_CMD_WAIT_735:
			// start waiting
			l_current_sample_pos += 735;

			l_player_state = VPS_Waiting;
			break;

		case VGM_CMD_WAIT_882:
			// start waiting
			l_current_sample_pos += 882;

			l_player_state = VPS_Waiting;
			break;

		case VGM_CMD_WAIT_1:
		case VGM_CMD_WAIT_2:
		case VGM_CMD_WAIT_3:
		case VGM_CMD_WAIT_4:
		case VGM_CMD_WAIT_5:
		case VGM_CMD_WAIT_6:
		case VGM_CMD_WAIT_7:
		case VGM_CMD_WAIT_8:
		case VGM_CMD_WAIT_9:
		case VGM_CMD_WAIT_10:
		case VGM_CMD_WAIT_11:
		case VGM_CMD_WAIT_12:
		case VGM_CMD_WAIT_13:
		case VGM_CMD_WAIT_14:
		case VGM_CMD_WAIT_15:
		case VGM_CMD_WAIT_16:
			l_current_sample_pos += (command & 0x0f) + 1;

			l_player_state = VPS_Waiting;
			break;

		case VGM_CMD_EOF:
			/*if (!l_looping && g_vgm_file_header.LoopOffset != 0)
			{
				l_looping = true;

				// position file to the first datablock
				fileInputStreamSeek(g_vgm_file_header.LoopOffset + 0x1c);

				l_data_buffer_length = (uint16_t)fileInputStreamRead(&l_data_buffer, VGM_DATA_BUFFER_LENGTH);
				l_data_buffer_pos = 0;

				l_player_state = VPS_CommandProcessing;
			}
			else
			{
				*/
			l_player_state = VPS_Finished;

			break;

		case VGM_CMD_GG_STEREO:
			byte_buffer = l_vgm_file_buffer[l_vgm_file_pos++];
			break;

		// invalid or unknown command
		default:
			printf("Unknown VGM command: %2X ", command);
			break;
	}
}


