from io import *
import sys

file = FileIO('files.txt', 'r')
buf = BufferedReader(file)
textbuf = TextIOWrapper(buf)
for line in textbuf.readlines():
    if line.startswith('C'):
        sys.stdout.write(line)

file.close()
