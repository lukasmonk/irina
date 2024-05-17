@echo off
PATH=\MinGW32\bin;\msys\1.0\bin;%PATH%
del irina.exe
make all -s
strip irina.exe
del *.o

