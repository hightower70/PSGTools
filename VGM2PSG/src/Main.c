#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fileVGM.h>
#include <fileVGMDecompress.h>
#include <filePSG.h>
#include <filePSGCompress.h>
#include <fileOutput.h>
#include <Main.h>

///////////////////////////////////////////////////////////////////////////////
// Defines


///////////////////////////////////////////////////////////////////////////////
// Local functions
static bool GetNumericParameter(int in_argc, char* in_argv[], int in_index, int in_min, int in_max, int* out_number);
static void PrintUsage(void);

///////////////////////////////////////////////////////////////////////////////
// Global variables
emuSN76489State g_SN76489_state;

///////////////////////////////////////////////////////////////////////////////
// Module global variables
static int l_psg_frame_step = 44100 / 50;
static uint8_t l_psg_buffer[FILE_BUFFER_LENGTH];
static uint8_t l_psg_compressed_buffer[FILE_BUFFER_LENGTH];
static bool l_insert_length = false;
static bool l_psg_compression = true;
static uint8_t l_vgm_buffer[FILE_BUFFER_LENGTH];

///////////////////////////////////////////////////////////////////////////////
// Main function
int main(int argc, char* argv[])
{
	int i;
	int value;
	char* vgm_filename = NULL;
	char* psg_filename = NULL;
	int output_length;

	for (i = 1; i < argc; i++)
	{
		if (argv[i][0] == '-')
		{
			// framerate param
			if (_strcmpi(argv[i], "-framerate") == 0)
			{
				if (!GetNumericParameter(argc, argv, i, 20, 100, &value))
					return -1;

				l_psg_frame_step = 44100 / value;
				i++;
			}
			else
			{
				// clock param
				if (_strcmpi(argv[i], "-clock") == 0)
				{
					if (!GetNumericParameter(argc, argv, i, 1000000, 4000000, &value))
						return -1;

					emuSN76489SetClockFrequency(value);

					i++;
				}
				else
				{
					if (_strcmpi(argv[i], "-insertlength") == 0)
					{
						l_insert_length = true;
					}
					else
					{
						if (_strcmpi(argv[i], "-noncompressed") == 0)
						{
							l_psg_compression = false;
						}
						else
						{
							if (_strcmpi(argv[i], "-?") == 0)
							{
								PrintUsage();
							}
							else
							{
								printf("ERROR: Invalid command line parameter: %s\n", argv[i]);
								return -1;
							}
						}
					}
				}
			}
		}
		else
		{
			if (vgm_filename == NULL)
			{
				vgm_filename = argv[i];
			}
			else
			{
				if (psg_filename == NULL)
				{
					psg_filename = argv[i];
				}
				else
				{
					printf("ERROR: Invalid parameter: %s\n", argv[i]);
					return -1;
				}
			}
		}
	}

	printf("Opening: %s\n", vgm_filename);

	int vgm_file_length = fileVGMLoad(vgm_filename, l_vgm_buffer, FILE_BUFFER_LENGTH);
	if (vgm_file_length == 0)
	{
		printf("ERROR: Can't load file.\n");
		return -1;
	}

	if (!fileVGMOpen(l_vgm_buffer, vgm_file_length))
	{
		printf("ERROR: Invalid file.\n");
		return -1;
	}

	// Init SN76489
	if (g_vgm_file_header.SN76489Clock > 0)
	{
		g_SN76489_state.ClockFrequency = g_vgm_file_header.SN76489Clock & VGM_CLOCK_MASK;
	}
	else
	{
		printf("The music is not composed for SN76489.\n");
		return -1;
	}
	
	filePSGStart(l_psg_buffer, FILE_BUFFER_LENGTH);

	emuSN76489Reset(&g_SN76489_state);
	fileVGMPlayerStart();
	while(fileVGMPlayerIsBusy())
	{
		fileVGMPlayerProcess(l_psg_frame_step);

		filePSGUpdate(&g_SN76489_state, fileVGMIsBehindLoopStart());
	}

	fileVGMClose();
	filePSGFinish();

	output_length = filePSGGetLength();

	if (l_psg_compression)
	{
		printf("Compressing");
		output_length = filePSGCompress(l_psg_buffer, filePSGGetLength());
		printf("\n");
	}

	printf("Creating: %s\n", psg_filename);

	// write output file
	if (!fileOutputCreate(psg_filename, false))
	{
		printf("Can't create output file: %s", argv[2]);
		return -1;
	}

	if (l_insert_length)
	{
		fileOutputWriteBlock((uint8_t*)&output_length, 2);
	}

	fileOutputWriteBlock(l_psg_buffer, output_length);
	fileOutputClose();

	printf("%d bytes written.\n", output_length);

	return 0;
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
	printf("VGM2PSG musicfile.vgm musicfile.psg [options]\n");
	printf("Options:\n");
	printf("  -clock n       - sets SN76489 clock frequency to n Hz. The default is 3579545Hz\n");
	printf("  -framerate n   - sets the playback framerate to n Hz. The default is 50Hz\n");
	printf("  -insertlength  - inserts PSG file length into the begining of the output file\n");
	printf("  -noncompressed - creates PSG file without comressed elements");
	printf("  -?             - prints this help text\n");
}
