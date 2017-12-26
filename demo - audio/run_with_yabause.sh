#!/bin/bash
command -v yabause >/dev/null 2>&1 || { echo "yabause is not installed.\
 Aborting." >&2; exit 1; }

echo "Sorry, Yabause doesn't support cue file" >&2
