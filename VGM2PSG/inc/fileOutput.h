/*****************************************************************************/
/* VGM2PSG Output File Writer                                                */
/*                                                                           */
/* Copyright (C) 2023 Laszlo Arvai                                           */
/* All rights reserved.                                                      */
/*                                                                           */
/* This software may be modified and distributed under the terms             */
/* of the BSD license.  See the LICENSE file for details.                    */
/*****************************************************************************/

#ifndef __fileOutput_h
#define __fileOutput_h

#include <stdint.h>
#include <stdbool.h>

bool fileOutputCreate(char* in_filename, bool in_text_mode);
void fileOutputWriteBlock(uint8_t* in_data, int in_data_length);
void fileOutputClose(void);

#endif