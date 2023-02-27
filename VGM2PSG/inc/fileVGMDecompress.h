/*****************************************************************************/
/* VGM2PSG VGM Loader/decompressor                                           */
/*                                                                           */
/* Copyright (C) 2023 Laszlo Arvai                                           */
/* All rights reserved.                                                      */
/*                                                                           */
/* This software may be modified and distributed under the terms             */
/* of the BSD license.  See the LICENSE file for details.                    */
/*****************************************************************************/

#ifndef __fileVGMDecompress_h
#define __fileVMGDecompress_h

#include <stdint.h>
#include <stdbool.h>

int fileVGMLoad(char* in_filename, uint8_t* in_buffer, int in_buffer_length);

#endif