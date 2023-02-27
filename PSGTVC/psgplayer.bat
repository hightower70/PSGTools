@sjasmplus.exe -Wno-rdlow --raw=psgplayer.bin --syntax=abf main.a80
@copy /b psgplayer.bin+DDRagon.psg psgplayer.bin
@tvctape psgplayer.bin DDragon.cas -a 1 -o


