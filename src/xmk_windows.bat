@echo off
PATH=h:\MinGW32\bin;h:\msys\1.0\bin;%PATH%
del irina.exe
make all -s
strip irina.exe
del *.o

