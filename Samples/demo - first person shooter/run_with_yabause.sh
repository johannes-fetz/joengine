#!/bin/bash
command -v yabause >/dev/null 2>&1 || { echo "yabause is not installed.\
 Aborting." >&2; exit 1; }

if [ -f game.iso ];
then
   yabause -a -i game.cue
else
   echo "Please compile first !" >&2
fi
