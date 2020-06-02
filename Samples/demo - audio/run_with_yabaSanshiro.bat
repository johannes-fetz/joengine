@ECHO Off
SET EMULATOR_DIR=..\..\Emulators

if exist game.iso (
echo CD Audio Track doesn't work with this emulator because it doesn't support CUE file
"%EMULATOR_DIR%\YabaSanshiro\yabasanshiro.exe" -a -i game.iso
) else (
echo Please compile first !
)
