/*****************************************************************************/
/* SN76489 Sound Chip Emulation                                              */
/*                                                                           */
/* Copyright (C) 2013-2023 Laszlo Arvai                                      */
/* All rights reserved.                                                      */
/*                                                                           */
/* This software may be modified and distributed under the terms             */
/* of the BSD license.  See the LICENSE file for details.                    */
/*****************************************************************************/
		
#ifndef __emuSN76489_h
#define __emuSN76489_h

///////////////////////////////////////////////////////////////////////////////
// Includes
#include <stdint.h>

///////////////////////////////////////////////////////////////////////////////
// Types

// Chip state
typedef struct
{
	uint32_t ClockFrequency;
	uint32_t ClockCounter;

	uint8_t RegisterIndex;
	uint16_t Registers[8];

	int8_t Paning[4]; // -127 ... 0 ... 127 (Left-Center-Right)

	uint16_t Frequency[3];
	uint16_t Counter[3];
	int8_t Output[3];
	uint16_t Amplitude[4];
	uint8_t NoiseControl;
	uint16_t NoiseCounter;
	int8_t NoiseOutput;
	uint16_t NoiseShiftRegister;
	uint16_t NoiseTap;

} emuSN76489State;

///////////////////////////////////////////////////////////////////////////////
// Function prototypes
void emuSN76489Reset(emuSN76489State* in_state);
void emuSN76496WriteRegister(emuSN76489State* in_state, uint8_t in_data);
void emuSN76489RenderAudioStream(emuSN76489State* in_state, int16_t* out_stream, uint16_t in_sample_count, uint8_t in_attenuation);
void emuSN76489SetPanning(emuSN76489State* in_state, uint8_t in_channel, uint8_t in_panning);

#endif