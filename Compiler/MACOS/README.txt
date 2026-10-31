Jo Engine on macOS (Intel and Apple Silicon)
============================================

The Jo Engine needs a GCC cross compiler for the SH-2 CPU of the Sega Saturn.
The compilers in Compiler/LINUX and Compiler/WINDOWS don't run on macOS, so you
have to build it once (about 15 minutes):

1) Install the Xcode command line tools:

    xcode-select --install

2) Install Homebrew (https://brew.sh) and the libraries needed by GCC, plus
   cdrtools for mkisofs (used to create the CD image):

    brew install gmp mpfr libmpc cdrtools

3) Build the toolchain (it is installed in this folder):

    cd Compiler/MACOS
    ./build_toolchain.sh

4) Compile a sample:

    cd "Samples/demo1"
    ./compile.sh

The makefile automatically uses Compiler/MACOS/bin/sh-elf-gcc. If you already
have an SH-2 toolchain (SaturnSDK, etc.), make sure sh-elf-gcc and
sh-elf-objcopy are in your PATH and skip step 3.

To run the game, open game.cue with an emulator (Mednafen, Kronos, Yabause...).
