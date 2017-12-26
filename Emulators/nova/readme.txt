==============================
Nova - a Sega Saturn emulator
Copyright (C) 2017 Steve Kwok
==============================



What's new
===============


v0.1.2
---------------
Fixed a mirroring bug of SH2 division unit, many games are playable now, sprites disappearance of some games is resolved too.


v0.1.1
---------------
Disabled thousands of warnings of illegal memory access of m68k, make Lunar Silver Star Story playable.
Fixed 4 instructions of m68k, CMP, CMPA, CMPI, CMPM, CD player and many games play sound properly now.
Fixed the issue of always formatting backup RAM when startup.


v0.1.0
---------------
Initial release.



Current status
===============
Nova is capable to run several commercial games, but it's still in its early stage. 


Features:
1.automatically select regions.
2.automatically select cartridge types by editing cart.json.
3.automatically save backup memory.

Nova doesn't support:
1.save/load states
2.run games via CD drive

Nova supports the following CD image formats: .cue, .mds, .ccd, .bin.


How to use
===============
Put bios.bin in bios folder. (Nova DOESN't provide bios, please don't ask for that.)

Key mapping:
UP: w
DOWN: s
LEFT: a
RIGHT: d
START: h
A: j
B: k
C: l
X: u
Y: i
Z: o
L: y
R: p

1: NBG0 ON/OFF
2: NBG1 ON/OFF
3: NBG2 ON/OFF
4: NBG3 ON/OFF
5: RBG0 ON/OFF
8: Sprite ON/OFF
9: VDP2 C++/JIT toggle

Nova only supports one player currently.


Special thanks!
===============
antime, TyRaNiD for documents.
Karl Stenerud, I referenced the source code of Musashi when I wrote my m68k emulator.
flamewing, BCD instructions emulation of m68k based on his research.
Charles MacDonald for VDP2 color RAM mirroring.
Yabause team, for undocumented CD Block commands E0, E1.
MITSUNARI Shigeo, for his great x86/x64 JIT assembler xbyak.
Niels Lohmann, for his great json parser.


Contact
===============
https://twitter.com/realSteveKwok/
