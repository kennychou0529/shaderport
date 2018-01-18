:: Make sure that vcvarsall has been invoked before calling this script
:: Change the SDL include and library paths below to suit your system
:: Make sure that SDL2.dll is inside the .build directory

@echo off

:: Make a .build directory that is hidden
if not exist ".build" mkdir .build
attrib +h .build /s /d

pushd .build

REM Compiler flags
REM -Od: Turns off optimizations and speeds up compilation
REM -Zi: Generates debug symbols
REM -WX: Treat warnings as errors
REM -W4: Highest level of warnings
REM -MD: Use DLL run-time library
set SDLI=-I"C:/Programming/lib-sdl/include"
set SDLL=-LIBPATH:"C:/Programming/lib-sdl/.build"
set CF=-Zi -nologo -Od -WX -W4 -wd4100 -wd4189 -wd4996 /MD %SDLI%

REM Linker flags
REM -subsystem:console: Open a console
REM -debug: Create debugging information into .pdb
set LF=-subsystem:console -debug %SDLL% SDL2main.lib SDL2.lib opengl32.lib user32.lib

cl %CF% ../main.cpp /link %LF% -out:native.exe
popd
.build\native.exe
