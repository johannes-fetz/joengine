@ECHO Off
SET COMPILER_DIR=..\..\Compiler
SET PATH=%COMPILER_DIR%\SH_COFF\Other Utilities;%COMPILER_DIR%\SH_COFF\sh-coff\bin;%COMPILER_DIR%\TOOLS;%PATH%
make re
JoEngineCueMaker