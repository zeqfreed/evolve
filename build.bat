set WORKDIR=%~dp0
cd %WORKDIR%

set BUILDDIR="%WORKDIR%_build"
set EXEDIR="%WORKDIR%bin"
set EXE="%EXEDIR%\evolve"

if not exist %BUILDDIR% mkdir %BUILDDIR%
if not exist %EXEDIR% mkdir %EXEDIR%

cl.exe src\windows\main.cpp user32.lib /W4 /wd4100 /WX /Fo%BUILDDIR%\evolve.obj /Fe%EXE%
cl.exe -Isrc src\viewer\main.cpp /std:c++14 /EHsc /LD /W4 /wd4018 /wd4100 /wd4201 /wd4505 /wd4996 /WX /Fo%BUILDDIR%\viewer.obj /Fe%BUILDDIR%\viewer.dll