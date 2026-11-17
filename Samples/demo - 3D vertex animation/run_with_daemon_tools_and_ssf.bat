@ECHO Off

SET EMULATOR_DIR=..\..\Emulators
SET DT_DIR=C:\Program Files (x86)\DAEMON Tools Lite

if exist game.iso (
echo Mounting image...
"%DT_DIR%\DTLite.exe" -mount 0,game.cue
cd "%EMULATOR_DIR%\SSF\"
echo Running SSF...
"SSF.exe"
echo Unmounting image...
"%DT_DIR%\DTLite.exe" -unmount 0
) else (
echo Please compile first !
pause
)
