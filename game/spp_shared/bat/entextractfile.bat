@echo off
setlocal
set EXEFILE="C:\Program Files (x86)\Steam\steamapps\common\Source SDK Base 2013 Multiplayer\bin\vbspinfo.exe"
set FILEDIR=%1
set FILENAME=%2


%EXEFILE% -X0 %FILEDIR%\%FILENAME%

