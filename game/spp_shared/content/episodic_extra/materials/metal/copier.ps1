$filelist = py.exe '.\copier.py'

Copy-Item -Path $filelist -Destination '.'