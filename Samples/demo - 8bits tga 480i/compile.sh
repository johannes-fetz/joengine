#!/bin/bash
export NCPU=`nproc`
make clean && make -j${NCPU} all
exit 0

