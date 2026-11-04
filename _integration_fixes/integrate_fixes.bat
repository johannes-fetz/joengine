@echo off
setlocal EnableExtensions
rem First person shooter sample: the fixes of the props and the guards. 2 commits, local only.
rem Option: /smileys (the messages with a smiley). Step 1: Claude (tools, generated models); step 2: the user.

set "DST=C:\GIT\master-anniversary"
set "INT=%DST%\_integration_fixes"
set "DEMO=Samples\demo - first person shooter"
set "CLAUDE=Claude <noreply@anthropic.com>"
set "SMILEYS=0"
if /i "%~1"=="/smileys" set "SMILEYS=1"

cd /d "%DST%" || goto fail
for /f "delims=" %%b in ('git rev-parse --abbrev-ref HEAD') do set "BRANCH=%%b"
echo Branch: %BRANCH%
git diff --quiet HEAD || (echo The working tree is not clean, commit or stash first & goto fail)
if not exist "%DST%\%DEMO%\tools\make_guard.py" (echo The first person shooter sample is not here & goto fail)
if not exist "%INT%\msg02.txt" (echo %INT% not found & goto fail)

for %%s in (01 02) do (
    call :step %%s || goto fail
)

echo.
echo Done (local only, nothing pushed). Rebuild the sample (compile.bat) to check it.
git log --oneline -2
cd /d "%DST%"
(goto) 2>nul & rd /s /q "%INT%"
exit /b 0

:step
set "S=%~1"
set "MSG=%INT%\msg%S%.txt"
if "%SMILEYS%"=="1" if exist "%INT%\msg%S%_s.txt" set "MSG=%INT%\msg%S%_s.txt"
xcopy /s /y /q "%INT%\step%S%" "%DST%\" >nul || exit /b 1
if "%S%"=="02" (
    rem (the makefile: 400 sprites; it can't be copied here)
    powershell -NoProfile -Command "$p='%DST%\%DEMO%\makefile'; $t=[IO.File]::ReadAllText($p); $t=$t -replace 'JO_MAX_SPRITE = 340', 'JO_MAX_SPRITE = 400'; [IO.File]::WriteAllText($p, $t)" || exit /b 1
)
git add -A -- "%DEMO%" jo_engine || exit /b 1
if "%S%"=="01" (
    git commit -q --author="%CLAUDE%" -F "%MSG%" || exit /b 1
) else (
    git commit -q -F "%MSG%" || exit /b 1
)
echo Step %S% committed
exit /b 0

:fail
echo.
echo FAILED. See git status.
exit /b 1
