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
#include <Types.h>
#include <emuSN76489.h>

///////////////////////////////////////////////////////////////////////////////
// Functions
void filePSGStart(uint8_t* in_psg_buffer, int in_psg_buffer_length);
void filePSGUpdate(emuSN76489State* in_SN76489_state, bool in_behind_loop_start);
void filePSGFinish(void);
int filePSGGetLength(void);

#endif