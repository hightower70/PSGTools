/*****************************************************************************/
/* Wave out device handler                                                   */
/*                                                                           */
/* Copyright (C) 2014 Laszlo Arvai                                           */
/* All rights reserved.                                                      */
/*                                                                           */
/* This software may be modified and distributed under the terms             */
/* of the BSD license.  See the LICENSE file for details.                    */
/*****************************************************************************/

#ifndef __drvWaveOut_h
#define __drvWaveOut_h

///////////////////////////////////////////////////////////////////////////////
// Const
#define WAVE_BUFFER_LENGTH 16384

///////////////////////////////////////////////////////////////////////////////
// Includes
#include <stdint.h>
#include <stdbool.h>

///////////////////////////////////////////////////////////////////////////////
// Global variables
extern uint16_t g_sample_rate;
extern bool g_stereo_mode;
			
///////////////////////////////////////////////////////////////////////////////
// Function prototypes
bool waveOpen(void);
int16_t* waveGetBuffer(void);
void waveClose(bool in_force_close);
bool waveIsBusy(void);

#endif