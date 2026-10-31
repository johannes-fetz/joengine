#!/bin/sh
#
# Jo Engine - Cue sheet maker (portable shell version of CueMaker, used on macOS)
#
# Creates game.cue from game.iso (data track) and the audio files of the current folder
# (*.wav, *.wave and *.mp3 become the audio tracks 2, 3, etc. in alphabetical order).
#
# Usage: cuemaker.sh [folder]
#

cd "${1:-.}" || exit 1

ISO="game.iso"
if [ ! -f "${ISO}" ]; then
    echo "No ISO file found in path: $(pwd)"
    exit 1
fi

{
    echo "FILE \"${ISO}\" BINARY"
    echo "  TRACK 01 MODE1/2048"
    echo "    INDEX 01 00:00:00"
    echo "    POSTGAP 00:02:00"
    track=2
    for f in *; do
        case "$(echo "${f}" | tr '[:upper:]' '[:lower:]')" in
            *.wav|*.wave) type="WAVE" ;;
            *.mp3) type="MP3" ;;
            *) continue ;;
        esac
        [ -f "${f}" ] || continue
        echo "FILE \"${f}\" ${type}"
        printf "  TRACK %02d AUDIO\n" ${track}
        echo "    PREGAP 00:02:00"
        echo "    INDEX 01 00:00:00"
        track=$((track + 1))
    done
} > game.cue

echo "game.cue generated."
exit 0
