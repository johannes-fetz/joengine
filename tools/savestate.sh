#!/bin/bash
# Usage: savestate.sh <cue> <seconds> <out>: run Mednafen, save the state after the delay (F5), copy it to <out>
CUE="$1"; WAIT="$2"; OUT="$3"
rm -f ~/.mednafen/mcs/*
export DISPLAY=:77
Xvfb :77 -screen 0 1024x768x24 >/dev/null 2>&1 &
XPID=$!
sleep 1
/usr/games/mednafen -sound 0 -video.driver sdl -video.fs 0 -ss.xscale 2 -ss.yscale 2 -ss.region_autodetect 1 -ss.cart none "$CUE" >/tmp/mednafen.log 2>&1 &
MPID=$!
sleep "$WAIT"
WID=$(xdotool search --pid $MPID 2>/dev/null | tail -1)
xdotool windowactivate --sync $WID >/dev/null 2>&1
xdotool key --window "$WID" F9 2>/dev/null
sleep 1
xdotool key --window "$WID" F5 2>/dev/null
sleep 3
kill $MPID; sleep 0.5; kill $XPID
cp ~/.mednafen/mcs/* "$OUT" 2>/dev/null; ls -la "$OUT"
