:: Build for Visual Studio compiler.
:: Run your copy of vcvarsall.bat to setup command-line compiler.
:: I use the 64-bit version of libraries, so pass the x86_amd64 flag (or your equivalent)
@echo off

:: Create a .build directory (hidden)
if not exist ".build" mkdir .build
attrib +h .build /s /d
pushd .build

:: Copy libtcc1.a (windows) and libtcc.dll (windows) to the same directory as executable
xcopy /y "..\libtcc\win\libtcc1.a" . > nul
xcopy /y "..\libtcc\win\libtcc.dll" . > nul

:: -Od: Turns off optimizations and speeds up compilation
:: -Zi: Generates debug symbols
:: -WX: Treat warnings as errors
:: -MD: Use DLL run-time library
set CF=-Zi -MD -nologo -Od -WX -W3 -wd4100 -wd4189 -wd4996 -wd4055


:: -subsystem:console: Open a console
:: -debug: Create debugging information into .pdb
set LIBGLFW=../libglfw/lib-vc2010-64/glfw3.lib
set LIBTCC=../libtcc/win/tcc.lib
set LF=-debug %LIBGLFW% %LIBTCC% opengl32.lib user32.lib gdi32.lib shell32.lib

cl %CF% ../main.cpp /link %LF% -out:shaderport.exe

:: if not errorlevel 1 shaderport.exe :: only run if we successfully compiled

:: Temporary: testing live C++ code reloading
::                                 >file to run      >place to store .dll trash
if not errorlevel 1 shaderport.exe script/script.cpp .build

popd
