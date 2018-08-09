@echo off

set GameDir=%~dp0
set GameDir=%GameDir:~0,-1%

if not exist "%GameDir%\gameinfo_hammer.txt" goto no_gameinfo

goto main

:no_gameinfo
echo Error: "gameinfo_hammer.txt" not found.
echo You must first run "prepfortools_1runmod.bat".
pause
goto end

:main
@echo on
copy %GameDir%\gameinfo.txt %GameDir%\gameinfo.txt.bak
copy %GameDir%\gameinfo_hammer.txt %GameDir%\gameinfo.txt
@echo off
pause
@echo on
copy %GameDir%\gameinfo.txt.bak %GameDir%\gameinfo.txt
@echo off
goto end

REM ****************
REM END
REM ****************
:end

echo.