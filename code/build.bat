@echo off

mkdir ..\build
pushd ..\build
cl -nologo -GR- -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -wd4129 -FC -Zi ..\code\asteroids_win32.cpp user32.lib gdi32.lib xinput.lib dsound.lib winmm.lib
popd

