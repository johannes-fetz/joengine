@ECHO OFF
REM Regression tests of the Jo Engine with Mednafen, see tests\README.md
REM   run_tests.bat              quick: builds, checks of the test ROM - before a commit
REM   run_tests.bat --full       everything: images, sound, measures, samples - on demand, after a big change
REM   run_tests.bat --approve    record the current results as the references in tests\golden, to commit
REM   run_tests.bat --help       the other options
SETLOCAL
CD /D "%~dp0"
SET PYTHON=
WHERE py >NUL 2>NUL
IF NOT ERRORLEVEL 1 SET PYTHON=py -3
IF DEFINED PYTHON GOTO run
WHERE python >NUL 2>NUL
IF NOT ERRORLEVEL 1 SET PYTHON=python
IF DEFINED PYTHON GOTO run
ECHO Python 3 not found: install it from https://www.python.org - no package needed
EXIT /B 2
:run
%PYTHON% run_tests.py %*
SET RESULT=%ERRORLEVEL%
IF "%RESULT%"=="0" GOTO end
ECHO.
ECHO Some tests failed: see tests\_work\report.html
:end
EXIT /B %RESULT%
