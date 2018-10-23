@echo off
setlocal

set FILEDIR=%1

FOR %%I IN (%FILEDIR%\*.wav) DO .\resamplefile.bat %FILEDIR% %%~nxI
