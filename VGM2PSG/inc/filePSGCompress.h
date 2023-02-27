/*****************************************************************************/
/* VGM2PSG PSG Compression function                                          */
/*                                                                           */
/* Copyright (C) 2023 Laszlo Arvai                                           */
/* All rights reserved.                                                      */
/*                                                                           */
/* This software may be modified and distributed under the terms             */
/* of the BSD license.  See the LICENSE file for details.                    */
/*****************************************************************************/

#ifndef __filePSGCompress_h
#define __filePSGCompress_h

///////////////////////////////////////////////////////////////////////////////
// Includes
#include <Types.h>

int filePSGCompress(uint8_t* in_buffer, int in_buffer_length);

#endif