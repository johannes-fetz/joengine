@ECHO Off
SET EMULATOR_DIR=..\..\Emulators
SET NOVA_BIOS_PATH=%EMULATOR_DIR%\nova\bios\bios.bin

if not exist %NOVA_BIOS_PATH% (
	echo ---
	echo Nova doesn't support HLE bios today.
	echo Therefore, please put Sega Saturn bios here %NOVA_BIOS_PATH%
	echo ---
	pause
	exit
)

if exist sl_coff.iso (
"%EMULATOR_DIR%\nova\nova.exe" "%cd%\sl_coff.cue"
) else (
echo Please compile first !
)
