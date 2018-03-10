:: Build for Visual Studio compiler.
:: Run your copy of vcvarsall.bat to setup command-line compiler.
:: I use the 64-bit version of libraries, so pass the x86_amd64 flag (or your equivalent)
@echo off

:: Create a .build directory (hidden)
if not exist ".build" mkdir .build
attrib +h .build /s /d
pushd .build

:: -Od: Turns off optimizations and speeds up compilation
:: -Zi: Generates debug symbols
:: -WX: Treat warnings as errors
:: -MD: Use DLL run-time library
set CF=-Zi -MD -nologo -Od -WX -W3 -wd4100 -wd4189 -wd4996 -wd4055


:: -subsystem:console: Open a console
:: -debug: Create debugging information into .pdb
set LIBGLFW=../libglfw/lib-vc2010-64/glfw3.lib
set LF=-debug %LIBGLFW% opengl32.lib user32.lib gdi32.lib shell32.lib

del shaderport.pdb
del vc110.pdb
del shaderport.ilk
del main.obj
del shaderport.exe

cl %CF% ../src/main.cpp /link %LF% -out:shaderport.exe

:: if not errorlevel 1 shaderport.exe :: only run if we successfully compiled

:: Temporary: testing live C++ code reloading
::                                 >file to run      >place to store .dll trash
if not errorlevel 1 shaderport.exe C:/Programming/shaderport/script/script.cpp C:/Temp/build/

popd
