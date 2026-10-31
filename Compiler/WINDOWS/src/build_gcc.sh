#!/bin/bash
#
# Build of the Windows sh-elf-gcc of the Jo Engine (GCC 8.4.0, C, link time optimization)
#
# Runs on Linux (Ubuntu 24.04): a "Canadian cross" build, the compiler runs on Windows (i686-w64-mingw32, 32 bits,
# no DLL needed) and makes code for the Saturn (sh-elf, SH-2).
#
# Why GCC 8.4 and not 9.3 (the previous Windows compiler):
#   - the previous Windows compiler had no lto1.exe: -flto was not possible on Windows
#   - on the tropical island bench (Mednafen), GCC 8 with -flto is the fastest: GCC 9.3 / 9.5 without LTO are 3 to 4 %
#     slower, and GCC 9.5 with LTO is not faster than without
#
# Requirements (Ubuntu): gcc-mingw-w64-i686 g++-mingw-w64-i686 gcc g++ make texinfo flex bison libgmp-dev libmpfr-dev
# libmpc-dev, the sources of gcc-8 (Ubuntu 20.04 "focal" deb-src: apt-get source gcc-8), gmp, mpfr4, mpclib3 and
# binutils-source (apt-get source / apt-get install)
#
# The binutils of the Jo Engine (Compiler/WINDOWS/sh-elf/bin, 2.34, with the linker plugin support) are kept.
#
# Usage: build_gcc.sh <directory of the sources> <work directory>
#   sources: gcc-8.4.0/, gmp-6.3.0+dfsg/, mpfr4-4.2.1/, mpclib3-1.3.1/, binutils-2.42/
#   output: <work>/windows (the files to copy into Compiler/WINDOWS)
#
set -e
SRC=$(realpath "$1")
WORK=$(realpath "$2")
TARGET=sh-elf
HOST=i686-w64-mingw32
JOBS=$(nproc)
DEPS=$WORK/mingw-deps
LINUX=$WORK/linux
mkdir -p "$WORK/build"

# GCC 8 with the recent mingw-w64 headers: abort() is a macro of GCC, <windows.h> declares abort()
for f in diagnostic-color.c pretty-print.c; do
    sed -i 's|^\(# *\)include <windows.h>$|\1undef abort\n\1include <windows.h>\n\1define abort() fancy_abort (__FILE__, __LINE__, __FUNCTION__)|' \
        "$SRC/gcc-8.4.0/gcc/$f"
done
# The Debian gmp sources have no documentation
mkdir -p "$SRC/gmp-6.3.0+dfsg/doc"
[ -f "$SRC/gmp-6.3.0+dfsg/doc/Makefile.in" ] || printf 'all:\ninstall:\ncheck:\nclean:\ndistclean:\n' > "$SRC/gmp-6.3.0+dfsg/doc/Makefile.in"

# 1. gmp, mpfr, mpc for Windows (static)
cd "$WORK/build" && rm -rf gmp mpfr mpc && mkdir gmp mpfr mpc
(cd gmp && "$SRC/gmp-6.3.0+dfsg/configure" --host=$HOST --prefix="$DEPS" --disable-shared --enable-static --disable-assembly && make -j$JOBS && make install)
(cd mpfr && "$SRC/mpfr4-4.2.1/configure" --host=$HOST --prefix="$DEPS" --with-gmp="$DEPS" --disable-shared --enable-static && make -j$JOBS && make install)
(cd mpc && "$SRC/mpclib3-1.3.1/configure" --host=$HOST --prefix="$DEPS" --with-gmp="$DEPS" --with-mpfr="$DEPS" --disable-shared --enable-static && make -j$JOBS && make install)

# 2. binutils and gcc for Linux (sh-elf): needed to build the Windows compiler and libgcc
mkdir -p binutils-linux && cd binutils-linux
"$SRC/binutils-2.42/configure" --target=$TARGET --prefix="$LINUX" --disable-nls --disable-werror --disable-gdb --disable-sim
make -j$JOBS && make install
cd .. && mkdir -p gcc-linux && cd gcc-linux
"$SRC/gcc-8.4.0/configure" --target=$TARGET --prefix="$LINUX" --enable-languages=c --disable-nls --disable-libssp \
    --without-headers --with-newlib --enable-lto --disable-multilib --with-cpu=m2 \
    CFLAGS="-O2 -std=gnu11 -fcommon" CXXFLAGS="-O2 -std=gnu++14"
make -j$JOBS all-gcc all-target-libgcc && make install-gcc install-target-libgcc
cd ..
export PATH="$LINUX/bin:$PATH"

# 3. gcc for Windows
mkdir -p gcc-windows && cd gcc-windows
"$SRC/gcc-8.4.0/configure" --build=x86_64-linux-gnu --host=$HOST --target=$TARGET --prefix=/jo \
    --enable-languages=c,lto --disable-nls --disable-libssp --without-headers --with-newlib --enable-lto \
    --disable-multilib --with-cpu=m2 --with-gmp="$DEPS" --with-mpfr="$DEPS" --with-mpc="$DEPS" --disable-win32-registry \
    --with-gnu-ld --with-gnu-as LDFLAGS="-static -static-libgcc -static-libstdc++" CXXFLAGS="-O2 -std=gnu++14"
# The linker of the Jo Engine supports the plugins (binutils 2.34): the driver passes the LTO plugin to the linker.
# configure doesn't find it in a Canadian cross build
make -j$JOBS configure-gcc
sed -i 's/^#define HAVE_LTO_PLUGIN 0$/#define HAVE_LTO_PLUGIN 2/' gcc/auto-host.h
make -j$JOBS all-gcc
cd ..
# The LTO plugin is a DLL (the static build of the compiler gives a static library only)
mkdir -p lto-plugin && cd lto-plugin
"$SRC/gcc-8.4.0/lto-plugin/configure" --build=x86_64-linux-gnu --host=$HOST --target=$TARGET --prefix=/jo \
    --enable-shared --disable-static --with-libiberty="$WORK/build/gcc-windows/libiberty" LDFLAGS="-static-libgcc"
make -j$JOBS
cd ..

# 4. The files of Compiler/WINDOWS
OUT=$WORK/windows
rm -rf "$OUT" && mkdir -p "$OUT/bin" "$OUT/libexec/gcc/$TARGET/8.4.0" "$OUT/lib/gcc/$TARGET/8.4.0"
$HOST-strip -o "$OUT/bin/sh-elf-gcc.exe" gcc-windows/gcc/xgcc.exe
for f in cc1 lto1 lto-wrapper collect2; do
    $HOST-strip -o "$OUT/libexec/gcc/$TARGET/8.4.0/$f.exe" gcc-windows/gcc/$f.exe
done
cp lto-plugin/.libs/liblto_plugin-0.dll "$OUT/libexec/gcc/$TARGET/8.4.0/"
cp -r "$LINUX/lib/gcc/$TARGET/8.4.0/include" "$LINUX/lib/gcc/$TARGET/8.4.0/include-fixed" \
      "$LINUX/lib/gcc/$TARGET/8.4.0/libgcc.a" "$LINUX/lib/gcc/$TARGET/8.4.0/libgcc-Os-4-200.a" "$OUT/lib/gcc/$TARGET/8.4.0/"
echo "Done: $OUT"
