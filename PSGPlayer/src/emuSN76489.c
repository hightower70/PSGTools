/*****************************************************************************/
/* SN76489 Sound Chip Emulation                                              */
/*                                                                           */
/* Copyright (C) 2023 Laszlo Arvai                                           */
/* All rights reserved.                                                      */
/*                                                                           */
/* This software may be modified and distributed under the terms             */
/* of the BSD license.  See the LICENSE file for details.                    */
/*****************************************************************************/

///////////////////////////////////////////////////////////////////////////////
// Includes
#include <emuSN76489.h>
#include <drvWaveOut.h>

///////////////////////////////////////////////////////////////////////////////
// Constants
#define CLOCK_DIVISOR 16
#define NOISE_INITIAL_STATE 0x8000
#define NOISE_DEFAULT_TAP 0x0009
#define NOISE_CONTROL_REGISTER 6

///////////////////////////////////////////////////////////////////////////////
// Local functions
static uint16_t CalculateParity(uint16_t in_value);

///////////////////////////////////////////////////////////////////////////////
// Local variables

// Aplitude table with 2db steps. Max amplitude is 8000
static uint16_t l_amplitude_table[16] =
{ 8000, 6355, 5048, 4009, 3185, 2530, 2010, 1596, 1268, 1007, 800, 635, 505, 401, 318, 0 };

///////////////////////////////////////////////////////////////////////////////
// Types


///////////////////////////////////////////////////////////////////////////////
// Resets SN76489
void emuSN76489Reset(emuSN76489State* in_state)
{
	uint8_t i;

	in_state->ClockCounter = 0;

	for(i=0; i<8; i++)
		in_state->Registers[i] = 0;

	for(i=0; i<3; i++)
		in_state->Frequency[i] = 1;

	for(i=0; i<3; i++)
		in_state->Counter[i] = 0;

	for(i=0; i<3; i++)
		in_state->Output[i] = 1;

	for(i=0; i<4; i++)
	{
		in_state->Amplitude[i] = 0;
		in_state->Paning[i] = 0;
	}

	in_state->NoiseShiftRegister	= NOISE_INITIAL_STATE;
	in_state->NoiseOutput					= in_state->NoiseShiftRegister & 1;
	in_state->NoiseTap						= NOISE_DEFAULT_TAP;
	in_state->NoiseControl				= 0;
	in_state->NoiseCounter				= 0;
}

///////////////////////////////////////////////////////////////////////////////
// Writes SN76496 Register
void emuSN76496WriteRegister(emuSN76489State* in_state, uint8_t in_data)
{
	uint8_t register_index;
	uint8_t register_value;
	/*
	// determine register value
	if ((in_data & 0x80) != 0)
	{
		register_index = in_state->RegisterIndex = (in_data >> 4) & 0x07;
		if (register_index != NOISE_CONTROL_REGISTER)
			register_value = (in_state->Registers[register_index] & 0x3f0) | (in_data & 0xf);
		else
			register_value = in_data & 0x07;
	}
	else
	{
		register_index = in_state->RegisterIndex;
		if ((register_index & 1) == 1)
		{
			register_value = in_data & 0x0f;
		}
		else
		{
			if (register_index != NOISE_CONTROL_REGISTER)
				register_value = (in_state->Registers[register_index] & 0x00f) | ((in_data & 0x3f) << 4);
			else
				register_value = in_data & 0x07;
		}
	}
	
	in_state->Registers[register_index] = register_value;*/

	// determine register value
	if ((in_data & 0x80) != 0)
	{
		register_index = in_state->RegisterIndex = (in_data >> 4) & 0x07;
		in_state->Registers[register_index] = (in_state->Registers[register_index] & 0x3f0) | (in_data & 0xf);
	}
	else
	{
		register_index = in_state->RegisterIndex;
		in_state->Registers[register_index] = (in_state->Registers[register_index] & 0x00f) | ((in_data & 0x3f) << 4);
	}

	// update register
	switch (register_index)
  {
    case 0: // tone 0: frequency
    case 2: // tone 1: frequency
    case 4: // tone 2: frequency
			// check for zero frequency
			in_state->Frequency[register_index/2] = in_state->Registers[register_index];
      break;

    case 1: // tone 0: attenuation
    case 3:	// tone 1: attenuation
    case 5: // tone 2: attenuation
    case 7: // Noise attenuation 
			in_state->Amplitude[register_index/2] = l_amplitude_table[in_data & 0x0f];
      break;

    case 6: // Noise control register
      in_state->NoiseControl				= (in_data & 0x07);									// set noise register
			in_state->NoiseShiftRegister	= NOISE_INITIAL_STATE;							// reset shift register
			in_state->NoiseOutput					= in_state->NoiseShiftRegister & 1; // set output
      break;
  }
}

///////////////////////////////////////////////////////////////////////////////
// Sets panning for stereo output
void emuSN76489SetPanning(emuSN76489State* in_state, uint8_t in_channel, uint8_t in_panning)
{
	in_state->Paning[in_channel] = in_panning;
}

///////////////////////////////////////////////////////////////////////////////
// Renders audio stream
void emuSN76489RenderAudioStream(emuSN76489State* in_state, int16_t* out_stream, uint16_t in_sample_count, uint8_t in_attenuation)
{
	int16_t sample[4];
	int16_t sample_sum;
	uint16_t sample_weight;
	uint16_t clock_cycles;
	uint16_t clock;
	uint16_t noise_divisor;
	uint8_t noise_register_shift_count = 0;
	uint8_t i;

	while (in_sample_count > 0)
	{
		// update ClockCounter
		in_state->ClockCounter += in_state->ClockFrequency / CLOCK_DIVISOR;
		clock_cycles = (uint16_t)(in_state->ClockCounter / g_sample_rate);
		in_state->ClockCounter -= clock_cycles * g_sample_rate;

		// handle channels 0,1,2
		for (i = 0; i < 3; i++)
		{
			if (in_state->Frequency[i] == 0 || in_state->Frequency[i] == 1)
			{
				in_state->Output[i] = 1;
			}
			else
			{
				if (in_state->Counter[i] < clock_cycles)
				{
					// run clock_cycle number of sound generarion cycle
					clock = clock_cycles;
					while (in_state->Counter[i] < clock)
					{
						// update output
						in_state->Output[i] = -in_state->Output[i];

						// output for noise register
						if (i == 3)
							noise_register_shift_count++;

						// update counter
						clock -= in_state->Counter[i];
						in_state->Counter[i] = in_state->Frequency[i];
					}
					in_state->Counter[i] -= clock;
				}
				else
				{
					in_state->Counter[i] -= (uint16_t)clock_cycles;
				}
			}

			// calculate sample
			sample[i] = in_state->Output[i] * in_state->Amplitude[i];
		}

		noise_register_shift_count = 0;

		if (in_state->NoiseCounter < clock_cycles)
		{
			switch (in_state->NoiseControl & 3)
			{
				case 0:
					noise_divisor = 2 * CLOCK_DIVISOR * 16;
					break;

				case 1:
					noise_divisor = 4 * CLOCK_DIVISOR * 16;
					break;

				case 2:
					noise_divisor = 8 * CLOCK_DIVISOR * 16;
					break;

				case 3:
					noise_divisor = in_state->Frequency[2] * CLOCK_DIVISOR * 2;
					break;
			}

			in_state->NoiseCounter = noise_divisor / CLOCK_DIVISOR + in_state->NoiseCounter - clock_cycles;
			noise_register_shift_count = 1;
		}
		else
		{
			in_state->NoiseCounter -= clock_cycles;
		}

		// handle noise register
		while (noise_register_shift_count > 0)
		{
			// update shift register
			in_state->NoiseShiftRegister = (in_state->NoiseShiftRegister >> 1) |
				((((in_state->NoiseControl & 4) != 0) ? CalculateParity(in_state->NoiseShiftRegister & in_state->NoiseTap) : in_state->NoiseShiftRegister & 1) << 15);

			noise_register_shift_count--;
		}

		// update output
		in_state->NoiseOutput = ((in_state->NoiseShiftRegister & 1) == 0) ? -1 : 1;

		// add noise output to the sample
		sample[3] = in_state->NoiseOutput * in_state->Amplitude[3];

		// generate sample output
		if (g_stereo_mode)
		{
			// stereo output

			// left channel
			sample_sum = 0;
			for (i = 0; i < 4; i++)
			{
				sample_weight = 127 - in_state->Paning[i];
				if (sample_weight > 254)
					sample_weight = 254;

				sample_sum += (int16_t)((int32_t)sample[i] * sample_weight / 254);
			}
			*out_stream += sample_sum / in_attenuation;
			out_stream++;

			// right channel
			sample_sum = 0;
			for (i = 0; i < 4; i++)
			{
				sample_weight = in_state->Paning[i] + 127;
				if (sample_weight < 0)
					sample_weight = 0;

				sample_sum += (int16_t)((int32_t)sample[i] * sample_weight / 254);
			}
			*out_stream += sample_sum / in_attenuation;
			out_stream++;
		}
		else
		{
			// mono output

			// sum of all channel
			sample_sum = sample[0] + sample[1] + sample[2] + sample[3];

			// store sample
			*out_stream += sample_sum / in_attenuation;
			out_stream++;
		}

		in_sample_count--;
	}
}

///////////////////////////////////////////////////////////////////////////////
// Calculates parity of a byte
static uint16_t CalculateParity(uint16_t in_value)
{
	in_value ^= in_value >> 8;
	in_value ^= in_value >> 4;
	in_value ^= in_value >> 2;
	in_value ^= in_value >> 1;
  
	return in_value&1;
 }