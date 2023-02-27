/*****************************************************************************/
/* Wave out device selector                                                  */
/*                                                                           */
/* Copyright (C) 2014 Laszlo Arvai                                           */
/* All rights reserved.                                                      */
/*                                                                           */
/* This software may be modified and distributed under the terms             */
/* of the BSD license.  See the LICENSE file for details.                    */
/*****************************************************************************/

///////////////////////////////////////////////////////////////////////////////
// Includes
#include <drvWaveOut.h>
#include <Windows.h>

#include <stdio.h>

///////////////////////////////////////////////////////////////////////////////
// Constants
#define WAVE_BUFFER_COUNT 8

///////////////////////////////////////////////////////////////////////////////
// Types

// wave output buffer
typedef struct
{
	bool      Free;
	WAVEHDR   Header;
  INT16     Buffer[WAVE_BUFFER_LENGTH];
} WaveOutBuffer;

///////////////////////////////////////////////////////////////////////////////
// Module global variables

static HWAVEOUT l_waveout_handle = NULL;
static HANDLE l_waveout_event = NULL;
static WaveOutBuffer l_waveout_buffer[WAVE_BUFFER_COUNT];
static int l_waveout_buffer_index;

///////////////////////////////////////////////////////////////////////////////
// Global variables
WORD g_sample_rate = 44100;
bool g_stereo_mode = false;

///////////////////////////////////////////////////////////////////////////////
// Local functions
static void PlayWaveOutBuffer(int in_buffer_index);
static int GetFreeWaveOutBufferIndex(void);


///////////////////////////////////////////////////////////////////////////////
// Gets buffer for rendering
INT16* waveGetBuffer(void)
{
	// send buffer to the output device
	if( l_waveout_buffer_index != -1)
	{
		PlayWaveOutBuffer(l_waveout_buffer_index);
		l_waveout_buffer_index = -1;
	}

	// get new free buffer
	if(l_waveout_buffer_index < 0)
		l_waveout_buffer_index = GetFreeWaveOutBufferIndex();

	if(l_waveout_buffer_index < 0)
		return NULL;
	else
		return (INT16*)l_waveout_buffer[l_waveout_buffer_index].Buffer;
}

///////////////////////////////////////////////////////////////////////////////
// Opens wave output device
bool waveOpen(void)
{
	MMRESULT result;
  WAVEFORMATEX wave_format;
  bool success = true;
	int i;

  // create event
	l_waveout_event = CreateEvent( NULL, true, false, NULL); // create event for sync.
	if(l_waveout_event == NULL)
		return false;

	ResetEvent(l_waveout_event);

  // prepare for opening
	ZeroMemory( &wave_format, sizeof(wave_format) );

	wave_format.wBitsPerSample		= 16;
	wave_format.wFormatTag				= WAVE_FORMAT_PCM;
	wave_format.nChannels 				= g_stereo_mode ? 2 : 1;
	wave_format.nSamplesPerSec		= g_sample_rate;
	wave_format.nAvgBytesPerSec		= wave_format.nSamplesPerSec * wave_format.wBitsPerSample / 8 * wave_format.nChannels;
	wave_format.nBlockAlign 			= wave_format.wBitsPerSample * wave_format.nChannels / 8;

  // open device
  result = waveOutOpen( &l_waveout_handle, WAVE_MAPPER, &wave_format, (DWORD)l_waveout_event, 0, CALLBACK_EVENT );
	if( result != MMSYSERR_NOERROR )
		success = false;

  // prepare buffers
	if(success)
	{
		for(i = 0; i < WAVE_BUFFER_COUNT; i++)
		{
			ZeroMemory( &l_waveout_buffer[i].Header, sizeof( WAVEHDR ) );
			l_waveout_buffer[i].Header.dwBufferLength = sizeof(l_waveout_buffer[i].Buffer);
			l_waveout_buffer[i].Header.lpData         = (LPSTR)(l_waveout_buffer[i].Buffer);
			l_waveout_buffer[i].Header.dwFlags        = 0;
			l_waveout_buffer[i].Free                  = true;
		}
	}

	// init
	l_waveout_buffer_index = -1;

	return success;
}

///////////////////////////////////////////////////////////////////////////////
// Closes wave output devoce
void waveClose(bool in_force_close)
{
	int i;
	bool finished;

	if(!in_force_close)
	{
		// wait until buffers are free
		do
		{
			// check if any buffer is still used
			finished = true;
			for(i = 0; i < WAVE_BUFFER_COUNT && finished; i++)
			{
				if(!l_waveout_buffer[i].Free)
					finished = false;
			}

			// at least one buffer is used -> wait
			if(!finished)
			{
				if(WaitForSingleObject(l_waveout_event, INFINITE) == WAIT_OBJECT_0)
				{
					ResetEvent(l_waveout_event);

					// find freed buffer
					for(i = 0; i < WAVE_BUFFER_COUNT; i++)
					{
						if(!l_waveout_buffer[i].Free && (l_waveout_buffer[i].Header.dwFlags & WHDR_DONE) != 0)
						{
							// release header
							waveOutUnprepareHeader(l_waveout_handle, &l_waveout_buffer[i].Header, sizeof(WAVEHDR));
							l_waveout_buffer[i].Free = true;
							l_waveout_buffer[i].Header.dwFlags = 0;
						}
					}
				}
			}
		}	while(!finished);
	}

	// close wave out device
	if(l_waveout_handle != NULL)
	{
		waveOutReset(l_waveout_handle);
		waveOutClose(l_waveout_handle);
		l_waveout_handle = NULL;
	}

	if(l_waveout_event != NULL)
	{
		CloseHandle(l_waveout_event);
		l_waveout_event = NULL;
	}
}

///////////////////////////////////////////////////////////////////////////////
// Returns true if wave device is still busy (there is buffer with data)
bool waveIsBusy(void)
{
	bool finished;
	int i;

	// check if any buffer is still used
	finished = true;
	for(i = 0; i < WAVE_BUFFER_COUNT && finished; i++)
	{
		if(!l_waveout_buffer[i].Free)
		{
			if((l_waveout_buffer[i].Header.dwFlags & WHDR_DONE) != 0)
			{
				// release header
				waveOutUnprepareHeader(l_waveout_handle, &l_waveout_buffer[i].Header, sizeof(WAVEHDR));
				l_waveout_buffer[i].Free = true;
				l_waveout_buffer[i].Header.dwFlags = 0;
			}
			else
				finished = false;
		}
	}

	return !finished;
}


/*****************************************************************************/
/* Local functions                                                           */
/*****************************************************************************/

///////////////////////////////////////////////////////////////////////////////
// Adds the specified buffer to the playback queue
static void PlayWaveOutBuffer(int in_buffer_index)
{
	// flag header
	l_waveout_buffer[in_buffer_index].Free = false;

  // prepare header
	waveOutPrepareHeader(l_waveout_handle, &l_waveout_buffer[in_buffer_index].Header, sizeof(WAVEHDR));

	// write header
	waveOutWrite(l_waveout_handle, &l_waveout_buffer[in_buffer_index].Header, sizeof(WAVEHDR));
}

///////////////////////////////////////////////////////////////////////////////
// Gets next free buffer inedx
static int GetFreeWaveOutBufferIndex(void)
{
	int buffer_index = -1;
	int i;

	//get buffer
	do
	{
		// check for free buffer
		for(i = 0; i < WAVE_BUFFER_COUNT; i++)
		{
			if(l_waveout_buffer[i].Free)
			{
				buffer_index = i;
				break;
			}
		}

		// there is no free buffer, wait until one buffer is finished
		if( buffer_index < 0 && WaitForSingleObject(l_waveout_event, INFINITE) == WAIT_OBJECT_0 )
		{
			ResetEvent(l_waveout_event);

			// find freed buffer
			for(i = 0; i < WAVE_BUFFER_COUNT; i++)
			{
				if(!l_waveout_buffer[i].Free && (l_waveout_buffer[i].Header.dwFlags & WHDR_DONE) != 0)
				{
					// release header
					waveOutUnprepareHeader( l_waveout_handle, &l_waveout_buffer[i].Header, sizeof(WAVEHDR));
					l_waveout_buffer[i].Free = true;
					l_waveout_buffer[i].Header.dwFlags = 0;
				}
			}
		}
	}	while(buffer_index < 0);

	return buffer_index;
}

