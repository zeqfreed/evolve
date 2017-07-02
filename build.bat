set WORKDIR=%~dp0
cd %WORKDIR%

set BUILDDIR="%WORKDIR%_build"
set EXEDIR="%WORKDIR%bin"
set EXE="%EXEDIR%\evolve"

if not exist %BUILDDIR% mkdir %BUILDDIR%
if not exist %EXEDIR% mkdir %EXEDIR%

set BFLAGS=/Fd%BUILDDIR%\
set CFLAGS=/Zi /EHsc
set WFLAGS=/W4 /wd4018 /wd4100 /wd4201 /wd4505 /wd4996 /WX

cl.exe -Isrc src\windows\main.cpp user32.lib %BFLAGS% %CFLAGS% %WFLAGS% /Fo%BUILDDIR%\evolve.obj /Fe%EXE%
if not %errorlevel% == 0 goto :error

cl.exe -Isrc src\viewer\main.cpp /LD %BFLAGS% %CFLAGS% %WFLAGS% /Fo%BUILDDIR%\viewer.obj /Fe%EXEDIR%\viewer.dll
if not %errorlevel% == 0 goto :error

REM cl.exe -Isrc src\cubes\main.cpp /LD %BFLAGS% %CFLAGS% %WFLAGS% /Fo%BUILDDIR%\cubes.obj /Fe%EXEDIR%\cubes.dll
if not %errorlevel% == 0 goto :error

goto :success

:error
REM Error
exit 1

:success
REM Success