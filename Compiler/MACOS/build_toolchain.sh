#!/bin/bash
#
# Jo Engine - SH-2 toolchain builder for macOS (Intel and Apple Silicon)
#
# Builds binutils, GCC (C compiler + libgcc) and newlib (C library) for the Sega Saturn CPU (SH-2, sh-elf target)
# and installs them in this folder (Compiler/MACOS). The Jo Engine makefile uses them automatically.
#
# Requirements:
#   - Xcode command line tools: xcode-select --install
#   - Homebrew (https://brew.sh): brew install gmp mpfr libmpc cdrtools
#     (cdrtools provides mkisofs, used to create the CD image)
#
# Usage:
#   ./build_toolchain.sh
#
# Optional environment variables:
#   BINUTILS_VERSION, GCC_VERSION, NEWLIB_VERSION  Versions to build (default: 2.42, 13.2.0 and 4.4.0.20231231)
#   BINUTILS_SRC, GCC_SRC, NEWLIB_SRC               Use already extracted sources instead of downloading them
#   JOBS                           Parallel jobs (default: CPU count)
#
# It takes about 15 minutes on a recent Mac.
#

set -e

BINUTILS_VERSION=${BINUTILS_VERSION:-2.42}
GCC_VERSION=${GCC_VERSION:-13.2.0}
NEWLIB_VERSION=${NEWLIB_VERSION:-4.4.0.20231231}
TARGET=sh-elf
PREFIX="$(cd "$(dirname "$0")" && pwd)"
WORK="${PREFIX}/build"
GNU_MIRROR=${GNU_MIRROR:-https://ftp.gnu.org/gnu}
NEWLIB_MIRROR=${NEWLIB_MIRROR:-https://sourceware.org/pub/newlib}

if [ -z "${JOBS}" ]; then
    JOBS=$(sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 2)
fi

echo "Jo Engine SH-2 toolchain: binutils ${BINUTILS_VERSION}, GCC ${GCC_VERSION}"
echo "Install folder: ${PREFIX}"

# GMP, MPFR and MPC (needed to build GCC)
GMP_FLAGS=""
if command -v brew >/dev/null 2>&1; then
    for lib in gmp mpfr libmpc; do
        if ! brew list "${lib}" >/dev/null 2>&1; then
            echo "Missing Homebrew package: ${lib} (brew install gmp mpfr libmpc)"
            exit 1
        fi
    done
    GMP_FLAGS="--with-gmp=$(brew --prefix gmp) --with-mpfr=$(brew --prefix mpfr) --with-mpc=$(brew --prefix libmpc)"
fi

mkdir -p "${WORK}"
cd "${WORK}"

# Sources
download()
{
    if [ ! -f "$(basename "$1")" ]; then
        echo "Downloading $1"
        curl -fL -o "$(basename "$1")" "$1"
    fi
    tar xf "$(basename "$1")"
}

if [ -z "${BINUTILS_SRC}" ]; then
    download "${GNU_MIRROR}/binutils/binutils-${BINUTILS_VERSION}.tar.xz"
    BINUTILS_SRC="${WORK}/binutils-${BINUTILS_VERSION}"
fi
if [ -z "${GCC_SRC}" ]; then
    download "${GNU_MIRROR}/gcc/gcc-${GCC_VERSION}/gcc-${GCC_VERSION}.tar.xz"
    GCC_SRC="${WORK}/gcc-${GCC_VERSION}"
fi
if [ -z "${NEWLIB_SRC}" ]; then
    download "${NEWLIB_MIRROR}/newlib-${NEWLIB_VERSION}.tar.gz"
    NEWLIB_SRC="${WORK}/newlib-${NEWLIB_VERSION}"
fi
if [ -z "${GMP_FLAGS}" ] && [ ! -d "${GCC_SRC}/gmp" ]; then
    if printf '#include <gmp.h>\n#include <mpfr.h>\n#include <mpc.h>\n' | cc -E - >/dev/null 2>&1; then
        echo "Using the installed GMP, MPFR and MPC"
    else
        # Not installed: build GMP, MPFR and MPC with GCC
        (cd "${GCC_SRC}" && ./contrib/download_prerequisites)
    fi
fi

# Binutils (assembler, linker, objcopy)
rm -rf build-binutils
mkdir build-binutils
cd build-binutils
"${BINUTILS_SRC}/configure" --target=${TARGET} --prefix="${PREFIX}" \
    --disable-nls --disable-werror --disable-gdb --disable-sim --disable-gprofng
make -j${JOBS}
make install
cd ..

export PATH="${PREFIX}/bin:${PATH}"

# GCC: C compiler and libgcc only (the Jo Engine doesn't use a C library)
rm -rf build-gcc
mkdir build-gcc
cd build-gcc
"${GCC_SRC}/configure" --target=${TARGET} --prefix="${PREFIX}" ${GMP_FLAGS} \
    --enable-languages=c --without-headers --with-newlib \
    --disable-nls --disable-shared --disable-threads --disable-libssp \
    --disable-libquadmath --disable-libgomp --disable-libatomic \
    --with-multilib-list=m2 --with-endian=big --enable-lto
make -j${JOBS} all-gcc all-target-libgcc
make install-gcc install-target-libgcc
cd ..

# Newlib: C library (sprintf, memcpy, malloc, etc.), small malloc like the Linux and Windows toolchains
rm -rf build-newlib
mkdir build-newlib
cd build-newlib
"${NEWLIB_SRC}/configure" --target=${TARGET} --prefix="${PREFIX}" \
    --disable-nls --enable-newlib-nano-malloc --disable-newlib-multithread
make -j${JOBS}
make install
cd ..

echo
echo "Done: ${PREFIX}/bin/${TARGET}-gcc"
"${PREFIX}/bin/${TARGET}-gcc" --version | head -1
echo "You can delete the build folder: ${WORK}"
exit 0
