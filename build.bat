@REM Build for Visual Studio compiler.
@REM Run your copy of vcvars32.bat or vcvarsall.bat to setup command-line compiler.
@echo off


if not exist ".build" mkdir .build
attrib +h .build /s /d
pushd .build


REM -Od: Turns off optimizations and speeds up compilation
REM -Zi: Generates debug symbols
REM -WX: Treat warnings as errors
REM -MD: Use DLL run-time library
set CF=-Zi -MD -nologo -Od -WX -W3 -wd4100 -wd4189 -wd4996 -wd4055


REM -subsystem:console: Open a console
REM -debug: Create debugging information into .pdb
set LF=-debug ../glfw/lib-vc2010-32/glfw3.lib opengl32.lib user32.lib gdi32.lib shell32.lib

cl %CF% ../main.cpp /link %LF% -out:native.exe


popd
.build\native.exe
