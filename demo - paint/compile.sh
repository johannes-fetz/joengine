#!/bin/bash
command -v genisoimage >/dev/null 2>&1 || { echo "genisoimage is not installed.\
 Aborting." >&2; exit 1; }
command -v wine >/dev/null 2>&1 || { echo "wine is not installed. Aborting." >&\
2; exit 1; }

wine cmd.exe /c compile.bat 2> /dev/null

genisoimage -quiet -sysid "SEGA SATURN" -volid "SaturnApp" -volset "SaturnApp" \
-publisher "SEGA ENTERPRISES, LTD." -preparer "SEGA ENTERPRISES, LTD." -appid "\
SaturnApp" -abstract "ABS.TXT" -copyright "CPY.TXT" -biblio "BIB.TXT" -generic-\
boot ../Compiler/COMMON/IP.BIN -full-iso9660-filenames -o sl_coff.iso cd

exit 0
