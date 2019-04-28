@echo on

set BaseDir=%~dp0
set BaseDir=%BaseDir:~0,-1%
set BinDir=%BaseDir%\bin

%BinDir%\xwad.exe -BaseDir %BaseDir%\spp_shared\content\hl1_extra -vtex -WadFile %1