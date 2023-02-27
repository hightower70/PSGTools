/*****************************************************************************/
/* VGM2PSG PSG memory file                                                   */
/*                                                                           */
/* Copyright (C) 2023 Laszlo Arvai                                           */
/* All rights reserved.                                                      */
/*                                                                           */
/* This software may be modified and distributed under the terms             */
/* of the BSD license.  See the LICENSE file for details.                    */
/*****************************************************************************/

#ifndef __filePSG_h
#define __filePSG_h

///////////////////////////////////////////////////////////////////////////////
// Includes
#include <stdint.h>
#include <stdbool.h>
#include <emuSN76489.h>

///////////////////////////////////////////////////////////////////////////////
// Functions
void filePSGPlayerInit(void);
void filePSGPlayerStart(uint8_t* in_psg_buffer, int in_psg_file_length);
void filePSGPlayerProcess(void);
bool filePSGPlayerIsBusy(void);
uint32_t filePSGGetCurrentSamplePos(void);

void filePSGSetFramerate(int in_framerate);
void filePSGSetClockFrequency(int in_clock_frequency);

#endif