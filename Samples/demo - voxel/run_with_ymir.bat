@ECHO Off
SET EMULATOR_DIR=..\..\Emulators
SET YMIR_EXECUTABLE_PATH=%EMULATOR_DIR%\Ymir\ymir-sdl3.exe

if not exist "%YMIR_EXECUTABLE_PATH%" (
	echo ---
	echo Ymir is not installed.
	echo Please download it from https://github.com/StrikerX3/Ymir/releases/latest
	echo and extract it here: %EMULATOR_DIR%\Ymir
	echo Ymir also needs a Sega Saturn BIOS, see the Ymir documentation.
	echo ---
	pause
	exit
)

if exist game.cue (
"%YMIR_EXECUTABLE_PATH%" "%cd%\game.cue"
) else (
echo Please compile first !
)
