@echo off
setlocal
set EXEFILE="C:\Users\peter\Downloads\ffmpeg-20171014-0655810-win64-shared\ffmpeg-20171014-0655810-win64-shared\bin\ffmpeg.exe"
set FILEDIR=%1
set FILENAME=%2

mkdir %FILEDIR%\backup
move %FILEDIR%\%FILENAME% %FILEDIR%\backup\%FILENAME%

%EXEFILE% -i %FILEDIR%\backup\%FILENAME% -ar 44100 %FILEDIR%\%FILENAME%

