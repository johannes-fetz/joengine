#!/bin/bash
command -v yabause >/dev/null 2>&1 || { echo "yabause is not installed.\
 Aborting." >&2; exit 1; }

if [ -f sl_coff.iso ];
then
   yabause -a -i sl_coff.cue
else
   echo "Please compile first !" >&2
fi
