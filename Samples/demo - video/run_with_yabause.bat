@ECHO Off
SET EMULATOR_DIR=..\..\Emulators

if exist game.iso (
"%EMULATOR_DIR%\yabause\yabause.exe" -a -i game.iso
) else (
echo Please compile first !
)
