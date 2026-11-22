@echo off
g++ -std=c++17 -O2 -mwindows src/stub.cpp -o stub.exe
g++ -std=c++17 -O2 -Iinclude src/main.cpp -lwinhttp -o VctrGameSim.exe
echo Kompajliranje zavrseno!