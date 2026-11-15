#!/bin/bash
# Ymir: the ymir-sdl3 program in the PATH, or the Flatpak (io.github.strikerx3.ymir)
if command -v ymir-sdl3 >/dev/null 2>&1; then
   YMIR="ymir-sdl3"
elif command -v flatpak >/dev/null 2>&1 && flatpak info io.github.strikerx3.ymir >/dev/null 2>&1; then
   YMIR="flatpak run io.github.strikerx3.ymir"
else
   echo "---" >&2
   echo "Ymir is not installed." >&2
   echo "Please download it from https://github.com/StrikerX3/Ymir/releases/latest" >&2
   echo "or install it with: flatpak install flathub io.github.strikerx3.ymir" >&2
   echo "Ymir also needs a Sega Saturn BIOS, see the Ymir documentation." >&2
   echo "---" >&2
   exit 1
fi

if [ -f game.cue ];
then
   $YMIR "$PWD/game.cue"
else
   echo "Please compile first !" >&2
fi
