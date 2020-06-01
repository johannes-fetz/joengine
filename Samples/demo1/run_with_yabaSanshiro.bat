@ECHO Off
SET EMULATOR_DIR=..\..\Emulators

if exist game.iso (
echo Please wait, it's very slow, but I don't know why...
"%EMULATOR_DIR%\YabaSanshiro\yabasanshiro.exe" -a -i game.iso
) else (
echo Please compile first !
)
