@ECHO Off
if exist sl_coff.iso (
"..\Emulators\yabause\yabause.exe" -a -i sl_coff.cue
) else (
echo Please compile first !
)
