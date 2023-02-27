/*****************************************************************************/
/* TVCTape - Videoton TV Computer Tape Emulator                              */
/* Common Type Declarations                                                  */
/*                                                                           */
/* Copyright (C) 2013 Laszlo Arvai                                           */
/* All rights reserved.                                                      */
/*                                                                           */
/* This software may be modified and distributed under the terms             */
/* of the BSD license.  See the LICENSE file for details.                    */
/*****************************************************************************/
#ifndef __Types_h
#define __Types_h

#include <stdint.h>
#include <stdbool.h>

#ifndef HIGH
#define HIGH(x) ((x)>>8)
#endif

#ifndef BV
#define BV(x) (1<<(x))
#endif

#endif