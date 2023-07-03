# PSGTools
The PSG file format is a simple sound chip port write log file. It is similar to the VGM (Video Game Music) file format, but PSG only supports the SN76489 sound chip and also supports data compression.
This repository contains some Windows command-line utilities for managing PSG files, as well as a Z80 assembly-based PSG player library written to the Videoton TV Computer.

## VGM2PSG
VGM2PSG is a tool to convert VGM music files to PSG file format. Handles resampling of the original VGM file for frame-based timing of the PSGF file. The frame rate is 50 Hz, but can be changed with a command line switch. It also supports SN76489 frequency command recalculation to handle differences in chip clock frequency. The default clock is 3.579 MHz, but this also can be changed with a command line switch. The output format is the binary PSG file, but the program can also produce assembler friendly '.db' data blocks.

## PSGTVC
The PSGTVC folder contains the source code of the Z80 assembly player routines. It also includes a simple TV Computer application to play PSG files.

## PSGPlayer
PSGPlayer is a Windows command-line utility to play PSG files using the default wave out device. It can be used to check the quality of converted PSG files.

## PSDDecompress
PSGDecompress is a small utility to convert compressed PSG files to uncompressed files. It's mostly for debugging purposes, there's no point in unpacking PSG files as they can be played without unpacking.

## PSG2TXT
PSG2TXT is another debugging utility. Converts a binary PSG file to a human-readable text file. It can be used to visually check the contents of a PSG file.
