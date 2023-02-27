/*****************************************************************************/
/* SN76489 Sound Chip Emulation                                              */
/*                                                                           */
/* Copyright (C) 2013 Laszlo Arvai                                           */
/* All rights reserved.                                                      */
/*                                                                           */
/* This software may be modified and distributed under the terms             */
/* of the BSD license.  See the LICENSE file for details.                    */
/*****************************************************************************/

///////////////////////////////////////////////////////////////////////////////
// Includes
#include <emuSN76489.h>


///////////////////////////////////////////////////////////////////////////////
// Constants

#define SN76489REG_CH0_TONE		0
#define SN76489REG_CH0_ATT		1
#define SN76489REG_CH1_TONE		2
#define SN76489REG_CH1_ATT		3
#define SN76489REG_CH2_TONE		4
#define SN76489REG_CH2_ATT		5
#define SN76489REG_NOISE_CTRL	6
#define SN76489REG_NOISE_ATT	7

///////////////////////////////////////////////////////////////////////////////
// Local functions


///////////////////////////////////////////////////////////////////////////////
// Local variables
static int l_target_clock_frequency = 3579545;


///////////////////////////////////////////////////////////////////////////////
// Types


///////////////////////////////////////////////////////////////////////////////
// Resets SN76489
void emuSN76489Reset(emuSN76489State* in_state)
{
	uint8_t i;

	for (i = 0; i < emuSN76489_REGISTER_COUNT; i++)
	{
		in_state->Registers[i] = 0;
		in_state->PrevRegisters[i] = 0;
		in_state->ToneRegisters[i] = 0;
	}

	in_state->NoiseRegisterChanged = false;
}

///////////////////////////////////////////////////////////////////////////////
// Writes SN76496 Register
void emuN76496WriteRegister(emuSN76489State* in_state, uint8_t in_data)
{
	uint8_t register_index;
	uint16_t register_value;

	// determine register value
	if ((in_data & 0x80) != 0)
	{
		// latch register command
		register_index = in_state->RegisterIndex = (in_data >> 4) & 0x07;

		switch (register_index)
		{
			// attenuation register
			case SN76489REG_CH0_ATT:
			case SN76489REG_CH1_ATT:
			case SN76489REG_CH2_ATT:
			case SN76489REG_NOISE_ATT:
				register_value = in_data & 0x0f;
				break;

				// tone registers
			case SN76489REG_CH0_TONE:
			case SN76489REG_CH1_TONE:
			case SN76489REG_CH2_TONE:
				register_value = (in_state->ToneRegisters[register_index] & 0x3f0) | (in_data & 0xf);
				break;

				// noise control register
			case SN76489REG_NOISE_CTRL:
				register_value = in_data & 0x07;
				break;

			default:
				register_value = 0;
				break;
		}
	}
	else
	{
		// data register command
		register_index = in_state->RegisterIndex;

		switch (register_index)
		{
			// attenuation register
			case SN76489REG_CH0_ATT:
			case SN76489REG_CH1_ATT:
			case SN76489REG_CH2_ATT:
			case SN76489REG_NOISE_ATT:
				register_value = in_data & 0x0f;
				break;

				// tone registers
			case SN76489REG_CH0_TONE:
			case SN76489REG_CH1_TONE:
			case SN76489REG_CH2_TONE:
				register_value = (in_state->ToneRegisters[register_index] & 0x00f) | ((in_data & 0x3f) << 4);
				break;

				// noise control register
			case SN76489REG_NOISE_CTRL:
				register_value = in_data & 0x07;
				break;

			default:
				register_value = 0;
				break;
		}
	}


	switch (register_index)
	{
		// attenuation register & noise control register
		case SN76489REG_CH0_ATT:
		case SN76489REG_CH1_ATT:
		case SN76489REG_CH2_ATT:
		case SN76489REG_NOISE_ATT:
			in_state->Registers[register_index] = register_value & 0x0f;
			break;

			// tone registers
		case SN76489REG_CH0_TONE:
		case SN76489REG_CH1_TONE:
		case SN76489REG_CH2_TONE:
			//save oroginal register content
			in_state->ToneRegisters[register_index] = register_value;

			// calculate new frequency value

			// store register value
			in_state->Registers[register_index] = register_value;
			break;

		case SN76489REG_NOISE_CTRL:
			in_state->Registers[register_index] = register_value & 0x07;
			in_state->NoiseRegisterChanged = true;
			break;


		default:
			break;
	}
}

///////////////////////////////////////////////////////////////////////////////
// Clears register changed
void emuSN76489ClearRegisterChanged(emuSN76489State* in_state)
{
	for (int i = 0; i < emuSN76489_REGISTER_COUNT; i++)
	{
		in_state->PrevRegisters[i] = in_state->Registers[i];
		in_state->NoiseRegisterChanged = false;
	}
}

///////////////////////////////////////////////////////////////////////////////
// Sets chip target clock frequency
void emuSN76489SetClockFrequency(int in_clock_frequency)
{
	l_target_clock_frequency = in_clock_frequency;
}
