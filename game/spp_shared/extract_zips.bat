set GameDir=%~dp0
set GameDir=%GameDir:~0,-1%

cd %GameDir%\content\background_movies\media

%GameDir%\tools\7za.exe x *.7z.001

pause