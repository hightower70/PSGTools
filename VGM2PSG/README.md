# VGM2PSG
VGM2PSG is a tool to convert VGM music files to PSG file format. Handles resampling of the original VGM file for frame-based timing of the PSGF file. The frame rate is 50 Hz, but can be changed with a command line switch. It also supports SN76489 frequency command recalculation to handle differences in chip clock frequency. The default clock is 3.579 MHz, but this also can be changed with a command line switch. The output format is the binary PSG file, but the program can also produce build-friendly '.db' data blocks.

The usage is the folowing:
VGM2PSG musicfile.vgm musicfile.psg [options]

Where options can be:
- -asm           - sets output file format to Z80 ASM file. If not specified, binary output will be produced.
- -clock n       - sets SN76489 clock frequency to n Hz. The default is 3579545Hz
- -framerate n   - sets the playback framerate to n Hz. The default is 50Hz
- -insertlength  - inserts PSG file length into the begining of the output file (2 bytes, low-high order)
- -noncompressed - creates PSG file without comressed elements
- -?             - prints help text
