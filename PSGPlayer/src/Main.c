/*****************************************************************************/
/* PSGPlayer - PSG Music File Player                                         */
/*                                                                           */
/* Copyright (C) 2023 Laszlo Arvai                                           */
/* All rights reserved.                                                      */
/*                                                                           */
/* This software may be modified and distributed under the terms             */
/* of the BSD license.  See the LICENSE file for details.                    */
/*****************************************************************************/

///////////////////////////////////////////////////////////////////////////////
// Includes
#include <Windows.h>
#include <stdio.h>
#include <conio.h>
#include <filePSG.h>
#include <drvWaveOut.h>

///////////////////////////////////////////////////////////////////////////////
// Defines
#define STOP_KEY VK_ESCAPE
#define BUFFER_SIZE 1024*1024

///////////////////////////////////////////////////////////////////////////////
// Local functions
static bool GetNumericParameter(int in_argc, char* in_argv[], int in_index, int in_min, int in_max, int* out_number);
static bool LoadPSG(char* in_file_name);
static void PrintUsage(void);

///////////////////////////////////////////////////////////////////////////////
// Global variables
uint8_t g_psg_buffer[BUFFER_SIZE];
uint32_t g_psg_length;

///////////////////////////////////////////////////////////////////////////////
// Main function
int main(int argc, char* argv[])
{
	char* filename = NULL;
	DWORD sample_count;
	int i;
	int value;

	// title
	printf("PSG Music file player (c) Laszlo Arvai 2023\n");
	filePSGPlayerInit();


	// process command line parameters
	if (argc < 2)
	{
		PrintUsage();
		return -1;
	}

	for (i = 1; i < argc; i++)
	{
		if (argv[i][0] == '-')
		{
			// framerate param
			if (_strcmpi(argv[i], "-framerate") == 0)
			{
				if (!GetNumericParameter(argc, argv, i, 20, 100, &value))
					return -1;

				filePSGSetFramerate(value);
				i++;
			}
			else
			{
				// clock param
				if (_strcmpi(argv[i], "-clock") == 0)
				{
					if (!GetNumericParameter(argc, argv, i, 1000000, 4000000, &value))
						return -1;

					filePSGSetClockFrequency(value);
					i++;
				}
				else
				{
					if (_strcmpi(argv[i], "-?") == 0)
					{
						PrintUsage();
					}
					else
					{
						printf("Invalid command line parameter: %s\n", argv[i]);
						return -1;
					}
				}
			}
		}
		else
		{
			if (filename == NULL)
			{
				filename = argv[i];
			}
			else
			{
				printf("Invalid parameter: %s\n", argv[i]);
				return -1;
			}
		}
	}

	// load PSG file
	if (!LoadPSG(filename))
	{
		return -1;
	}

	// open default wave out device
	waveOpen();

	printf("Press ESC to stop playback\n");

	// starts PSG player
	filePSGPlayerStart(g_psg_buffer, g_psg_length);
	while(filePSGPlayerIsBusy())
	{
		filePSGPlayerProcess();

		sample_count = filePSGGetCurrentSamplePos();
		sample_count /= 44100;

		printf("Playing: %3d:%02d \r",sample_count / 60, sample_count % 60);

		// check for stop key
		if (_kbhit())
		{
			if (_getch() == STOP_KEY)
			{
				printf("Stopping...        \r");
				break;
			}
		}
	}

	waveClose(false);

	printf("\r                 \n");

	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Loads PSG file
static bool LoadPSG(char* in_file_name)
{
	FILE* psg_file;

	// open PSG file
	printf("Opening: %s\n", in_file_name);
	psg_file = fopen(in_file_name, "rb");
	if (psg_file == NULL)
	{
		printf("Can't open file\n");
		return false;
	}
	
	// load file
	g_psg_length = fread(g_psg_buffer, 1, BUFFER_SIZE, psg_file);

	fclose(psg_file);

	// check load result
	if (g_psg_length == 0)
	{
		printf("Can't read file\n");
		return false;
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////
// Gets numeric parameter from the command line
static bool GetNumericParameter(int in_argc, char* in_argv[], int in_index, int in_min, int in_max, int* out_number)
{
	int number;

	if (in_index + 1 < in_argc)
	{
		number = atoi(in_argv[in_index + 1]);

		if (number < in_min || number>in_max)
		{
			printf("Invalid value: %d\n", number);
			return false;
		}
		else
		{
			*out_number = number;

			return true;
		}
	}
	else
	{
		printf("Invalid parameter: %s\n", in_argv[in_index]);
		return false;
	}
}

///////////////////////////////////////////////////////////////////////////////
// Prints help text
static void PrintUsage(void)
{
	printf("Usage:\n");
	printf("PSGPlay musicfile.psg [options]\n");
	printf("Options:\n");
	printf("  -clock n      - sets SN76489 clock frequency to n Hz. The default is 3579545Hz\n");
	printf("  -framerate n  - sets the playback framerate to n Hz. The default is 50Hz\n");
	printf("  -?            - prints this help text\n");
}