============================================
Nova - A Sega Saturn emulator
Copyright (C) 2017 Steve Kwok
Twitter: https://twitter.com/realSteveKwok
Discord: https://discord.gg/yAQBa5C
Patreon: https://www.patreon.com/nova_emu
============================================


Requirements
=================
CPU: 2.0GHz
Memory: 1GB
Display card: Support DirectX9
OS: Win7/8/8.1/10
In addition, you may need to install the following components:
1.VS2017 redistributable
2.DirectX redistributable (June 2010)


Current status
=================
Nova is capable to run several commercial games, but it's still in its early stage. 


Features:
1.automatically select regions.
2.automatically select cartridge types by editing cart.json.
3.automatically save backup memory.

Nova doesn't support:
1.save/load states
2.run games via CD drive

Nova supports the following CD image formats: CUE/MDS/CCD/BIN/ISO.


How to use
=================
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

Nova can start from command line. The syntax is very simple:
> nova "path"


Special thanks
=================
antime, TyRaNiD for documents.
Charles MacDonald, I got lots of helps from his research on undocumented stuff.
Yabause team, for undocumented CDBLOCK commands E0, E1.
Karl Stenerud, I referenced the source code of Musashi when I wrote my m68k emulator.
flamewing, BCD instructions emulation of m68k based on his research.
MITSUNARI Shigeo, for his great x86/x64 JIT assembler Xbyak.
Niels Lohmann, for his great JSON parser.
Benjamin Siskoo, for his kindness and various helps.
Zet-sensei, for his kindness and the completed cart.json file.
Dwayne, the first patron of Nova.


Testing
=================
Benjamin Siskoo
Zet-sensei
bosshunter
Strelok
InsertMoreCoins
Nocturne
caesarkof


Translation
=================
Benjamin Siskoo
Hector (Literalmente Game)
InsertMoreCoins
theheroGAC
Deni
cia


Patreon Supporter
=================
Johannes Fetz
