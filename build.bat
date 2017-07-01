set WORKDIR=%~dp0
cd %WORKDIR%

set BUILDDIR="%WORKDIR%_build"
set EXEDIR="%WORKDIR%bin"
set EXE="%EXEDIR%\evolve"

if not exist %BUILDDIR% mkdir %BUILDDIR%
if not exist %EXEDIR% mkdir %EXEDIR%

cl.exe src\windows\main.cpp user32.lib /W4 /wd4100 /WX /Fo%BUILDDIR%\evolve.obj /Fe%EXE%
