@ECHO Off
SET EMULATOR_DIR=..\..\Emulators
SET VCD_DIR=C:\Program Files (x86)\Elaborate Bytes\VirtualCloneDrive

if exist sl_coff.iso (
echo Mounting image...
"%VCD_DIR%\vcdmount.exe" sl_coff.iso
cd "%EMULATOR_DIR%\SSF\"
echo Running SSF...
"SSF.exe"
echo Unmounting image...
"%VCD_DIR%\vcdmount.exe" /u
) else (
echo Please compile first !
pause
)
