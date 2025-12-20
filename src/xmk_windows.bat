@echo off
PATH=h:\MinGW64\bin;\msys\1.0\bin;%PATH%
make clean
make all -s
strip irina.exe
