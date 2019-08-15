@echo off

mkdir ..\build
pushd ..\build
cl -nologo -GR- -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -wd4129 -FC -Zi ..\code\asteroids_win32.cpp ..\code\asteroids.cpp ..\code\collision.cpp ..\code\common.cpp ..\code\entities.cpp ..\code\game_object.cpp ..\code\geometry.cpp ..\code\input.cpp ..\code\level_management.cpp ..\code\memory.cpp ..\code\model.cpp ..\code\render.cpp user32.lib gdi32.lib xinput.lib dsound.lib winmm.lib
popd

