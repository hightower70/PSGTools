## PSGTVC
The PSGTVC folder contains the source code of the Z80 assembly player routines. It also includes a simple TV Computer application to play PSG files.
There are two playback libraries. The 'PSGPlayer.a80' is the more complex version, it is able to recalculate the sound chip frequencies based on their click signal, so the pitch of the music will be the same independently of the sound chip clock frequency.
There is a define to control the calculation. If 'PSGFastFreqCalculation' is non zero, the calculation is done by multiplying the pitch register value by 7/8 which is the approximation of the clock differences (3.125MHz/3.679MHz)
The 'PSGFastFreqCalculation' can be zero, in this case an accurate (but slower) table based calculation will be done using the exact values of (3.125/3.679)

There is a simple playback library called 'psgplayer_nofcalc.a80' which can't handle the clock frequency differences, it simply sends the exact same pitch value stored in the PSG file.

The folowing functions can be called (in this order):
1. Call 'DetectSndCard'
It detects the installed SN76489 based sound card. Currently two different cards are supported. The SoundMagic card ([TVC Multichip Soundcard](https://github.com/dikdom/TVC-Multichip-Soundcard)) and Game card ([TVC Game Card](https://github.com/dikdom/TVC-GameCard)). The detected card type can be accessed at the 'SndCardType' variable. The type can be the folowing: 
* 'SndCardGame'=1 - Game Card
* 'SndCardSndMx'=2 - Sound Magic
* 'SndCardNone'=0 - No sound card installed

2. Call 'InitMusicPlayer'
Initializes music player libary. Resets the internal variables.

3. Initialize interrupt system and prepare to call 'MusicPlayer_IT' from the interrupt handler
The music playback is done from the 50Hz interrupt. The interrupt vector must be initialized and for the playback the 'MusicPlayer_IT' subroutine must be called from each interrupt request. The subroutine saves all used registers into the stack, so no registers will be modified.

4. Load PSG file to the memory and store starting address of the memory in the 'PSGFile' variable
The PSG file must be in the memory and before starting the playback the 'PSGFile' variable must be filled with the memory address.

5. Call 'StartMusic' routine for staring music playing
Calling the 'StartMusic' starts the music playback

6. Call 'StopMusic' routine for turning off the sound
The 'StopMusic' subroutine can be called anytime to stop the music playback and mute the sound chip.
