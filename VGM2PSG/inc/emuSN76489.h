/*****************************************************************************/
/* SN76489 Sound Chip Emulation                                              */
/*                                                                           */
/* Copyright (C) 2013 Laszlo Arvai                                           */
/* All rights reserved.                                                      */
/*                                                                           */
/* This software may be modified and distributed under the terms             */
/* of the BSD license.  See the LICENSE file for details.                    */
/*****************************************************************************/
		
#ifndef __emuSN76489_h
#define __emuSN76489_h

///////////////////////////////////////////////////////////////////////////////
// Includes
#include <Types.h>

///////////////////////////////////////////////////////////////////////////////
// Constants
#define emuSN76489_REGISTER_COUNT 8

///////////////////////////////////////////////////////////////////////////////
// Types

// Chip state
typedef struct
{
	uint32_t ClockFrequency;

	uint8_t RegisterIndex;
	uint16_t Registers[emuSN76489_REGISTER_COUNT];
	uint16_t PrevRegisters[emuSN76489_REGISTER_COUNT];
	uint16_t ToneRegisters[emuSN76489_REGISTER_COUNT];

	bool NoiseRegisterChanged;

} emuSN76489State;

///////////////////////////////////////////////////////////////////////////////
// Function prototypes
void emuSN76489Reset(emuSN76489State* in_state);
void emuN76496WriteRegister(emuSN76489State* in_state, uint8_t in_data);
void emuSN76489ClearRegisterChanged(emuSN76489State* in_state);

void emuSN76489SetClockFrequency(int in_clock_frequency);


#endif