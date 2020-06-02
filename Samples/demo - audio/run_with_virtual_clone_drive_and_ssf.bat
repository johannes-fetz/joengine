@ECHO Off
SET EMULATOR_DIR=..\..\Emulators
SET VCD_DIR=C:\Program Files (x86)\Elaborate Bytes\VirtualCloneDrive

if exist game.iso (
echo CD Audio Track doesn't work because Virtual Clone Drive doesn't support CUE file
echo Mounting image...
"%VCD_DIR%\vcdmount.exe" game.iso
cd "%EMULATOR_DIR%\SSF\"
echo Running SSF...
"SSF.exe"
echo Unmounting image...
"%VCD_DIR%\vcdmount.exe" /u
) else (
echo Please compile first !
pause
)
