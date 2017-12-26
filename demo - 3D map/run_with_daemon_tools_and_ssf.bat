@ECHO Off
if exist sl_coff.iso (
echo Mounting image...
"C:\Program Files (x86)\DAEMON Tools Lite\DTLite.exe" -mount 0,sl_coff.cue
cd "..\Emulators\SSF\"
echo Running SSF...
"SSF.exe"
echo Unmounting image...
"C:\Program Files (x86)\DAEMON Tools Lite\DTLite.exe" -unmount 0
) else (
echo Please compile first !
pause
)
