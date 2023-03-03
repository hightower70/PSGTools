@sjasmplus.exe -Wno-rdlow --raw=psgplayer.bin --syntax=abf main.a80
@copy /b psgplayer.bin + %1.psg psgplayer.bin
@tvctape psgplayer.bin %1.cas -a 1 -o


