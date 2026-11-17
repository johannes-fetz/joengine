@ECHO Off
rem Mednafen as a European console (PAL, 50 Hz): the game runs at 25 frames per second (30 on a NTSC console)
SET EMULATOR_DIR=..\..\Emulators
SET MEDNAFEN_EXECUTABLE_PATH=%EMULATOR_DIR%\mednafen\mednafen.exe

if not exist %MEDNAFEN_EXECUTABLE_PATH% goto no_mednafen
if not exist game.cue goto no_game
"%MEDNAFEN_EXECUTABLE_PATH%" "%cd%\game.cue" -ss.region_autodetect "0" -ss.region_default "eu" -sound.volume "100" -sound.buffer_time "100" -video.glvsync "0" -sound.driver sdl
goto end

:no_mednafen
echo ---
echo Please install Mednafen here %EMULATOR_DIR%
echo ---
pause
goto end
:no_game
echo Please compile first !
pause
:end
