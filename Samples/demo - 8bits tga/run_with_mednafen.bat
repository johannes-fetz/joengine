@ECHO Off
SET EMULATOR_DIR=..\..\Emulators
SET MEDNAFEN_EXECUTABLE_PATH=%EMULATOR_DIR%\mednafen\mednafen.exe

if not exist %MEDNAFEN_EXECUTABLE_PATH% (
	echo ---
	echo Please install Mednafen here %EMULATOR_DIR%
	echo ---
	pause
	exit
)

if exist game.cue (
"%MEDNAFEN_EXECUTABLE_PATH%" "%cd%\game.cue" -sound.volume "150"
) else (
echo Please compile first !
)
