@echo off

mkdir ..\build
pushd ..\build
cl -Zi ..\code\asteroids_win32.cpp user32.lib
popd