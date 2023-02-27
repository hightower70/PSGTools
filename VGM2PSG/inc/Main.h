/*****************************************************************************/
/* SN76489 Sound Chip Emulation                                              */
/*                                                                           */
/* Copyright (C) 2013 Laszlo Arvai                                           */
/* All rights reserved.                                                      */
/*                                                                           */
/* This software may be modified and distributed under the terms             */
/* of the BSD license.  See the LICENSE file for details.                    */
/*****************************************************************************/

#ifndef __Main_h
#define __Main_h

///////////////////////////////////////////////////////////////////////////////
// Includes
#include <Types.h>
#include <emuSN76489.h>

#define FILE_BUFFER_LENGTH (1024*1024)

extern emuSN76489State g_SN76489_state;

#endif