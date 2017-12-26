@ECHO Off
if exist sl_coff.iso (
echo Mounting image...
"C:\Program Files (x86)\Elaborate Bytes\VirtualCloneDrive\vcdmount.exe" sl_coff.iso
cd "..\Emulators\SSF\"
echo Running SSF...
"SSF.exe"
echo Unmounting image...
"C:\Program Files (x86)\Elaborate Bytes\VirtualCloneDrive\vcdmount.exe" /u
) else (
echo Please compile first !
pause
)
