@echo off
setlocal

set FILEDIR=%1

FOR %%I IN (%FILEDIR%\*.bsp) DO .\entextractfile.bat %FILEDIR% %%~nxI
