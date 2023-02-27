/*****************************************************************************/
/* VGMPlayer - Video Game Music Player                                       */
/* Common Type Declarations                                                  */
/*                                                                           */
/* Copyright (C) 2013 Laszlo Arvai                                           */
/* All rights reserved.                                                      */
/*                                                                           */
/* This software may be modified and distributed under the terms             */
/* of the BSD license.  See the LICENSE file for details.                    */
/*****************************************************************************/
#ifndef __fileVGM_h
#define __fileVGM_h

///////////////////////////////////////////////////////////////////////////////
// Includes
#include <Types.h>

///////////////////////////////////////////////////////////////////////////////
// Constants
#define VGM_IDENT 0x206d6756ul
#define GD3_IDENT 0x20336447ul
#define GD3_BUFFER_LENGTH 1024
#define VGM_DATA_BUFFER_LENGTH 512
#define VGM_DUAL_CHIP_BIT 0x40000000ul
#define VGM_CLOCK_MASK    0x3ffffffful

///////////////////////////////////////////////////////////////////////////////
// Types

#pragma pack(push, 1)

// VGM File header
typedef struct
{
	uint32_t VGMIdent;
  uint32_t EOFOffset;
	uint32_t Version;
  uint32_t SN76489Clock;
	uint32_t YM2413Clock;
  uint32_t GD3Offset;
	uint32_t TotalNumberOfSamples;
	uint32_t LoopOffset;
  uint32_t LoopNumberOfSamples;
	uint32_t PlaybackRate;
  uint16_t NoiseFeedback;
	uint8_t NoiseShiftRegister;
	uint8_t SN76489Flags;
  uint32_t YM2612Clock;
	uint32_t YM2151Clock;
  uint32_t DataOffset;
	uint32_t SegaPCMClock;
	uint32_t SegaPCMInterfaceRegister;

	uint32_t lngHzRF5C68;
	uint32_t lngHzYM2203;
	uint32_t lngHzYM2608;
	uint32_t lngHzYM2610;
	uint32_t lngHzYM3812;
	uint32_t lngHzYM3526;
	uint32_t lngHzY8950;
	uint32_t lngHzYMF262;
	uint32_t lngHzYMF278B;
	uint32_t lngHzYMF271;
	uint32_t lngHzYMZ280B;
	uint32_t lngHzRF5C164;
	uint32_t lngHzPWM;
	uint32_t lngHzAY8910;
	uint8_t bytAYType;
	uint8_t bytAYFlag;
	uint8_t bytAYFlagYM2203;
	uint8_t bytAYFlagYM2608;
	uint8_t bytVolumeModifier;
	uint8_t bytReserved2;
	int8_t bytLoopBase;
	uint8_t bytLoopModifier;
	uint32_t lngHzGBDMG;
	uint32_t lngHzNESAPU;
	uint32_t lngHzMultiPCM;
	uint32_t lngHzUPD7759;
	uint32_t lngHzOKIM6258;
	uint8_t bytOKI6258Flags;
	uint8_t bytK054539Flags;
	uint8_t bytC140Type;
	uint8_t bytReservedFlags;
	uint32_t lngHzOKIM6295;
	uint32_t lngHzK051649;
	uint32_t lngHzK054539;
	uint32_t lngHzHuC6280;
	uint32_t lngHzC140;
	uint32_t lngHzK053260;
	uint32_t lngHzPokey;
	uint32_t lngHzQSound;

	uint32_t Reserved1;
	uint32_t ExtraHeaderOffset;

} VGMFileHeaderType;

// VGM File GD3 header
typedef struct 
{
	uint32_t GD3Ident;
	uint32_t Version;
	uint32_t Length;
} VGMFileGD3TagHeader;

// extra header clock header entry
typedef struct
{
	uint8_t ChipID;
	uint32_t Clock;
} VGMFileChipClockHeaderEntry;
#pragma pack(pop)


///////////////////////////////////////////////////////////////////////////////
// Global variables
extern VGMFileHeaderType g_vgm_file_header;
extern VGMFileGD3TagHeader g_gd3_header;



///////////////////////////////////////////////////////////////////////////////
// Function prototypes
bool fileVGMOpen(uint8_t* in_buffer, int in_buffer_length);
void fileVGMClose(void);

void fileVGMPlayerStart(void);
void fileVGMPlayerProcess(int in_sample_pos_increment);
bool fileVGMPlayerIsBusy(void);
uint32_t fileVGMGetTotalSampleCount(void);
uint32_t fileVGMGetCurrentSamplePos(void);
bool fileVGMIsBehindLoopStart(void);

#endif