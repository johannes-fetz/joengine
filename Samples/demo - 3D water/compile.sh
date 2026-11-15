#!/bin/bash
export NCPU=`nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 1`
make clean && make -j${NCPU} all
exit 0

