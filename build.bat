set WORKDIR=%~dp0
cd %WORKDIR%

set BUILDDIR="%WORKDIR%_build"
set EXEDIR="%WORKDIR%bin"
set EXE="%EXEDIR%\evolve"

if not exist %BUILDDIR% mkdir %BUILDDIR%
if not exist %EXEDIR% mkdir %EXEDIR%

set BFLAGS=/DCOLOR_BGR /DPLATFORM_WINDOWS /Fd%BUILDDIR%\
REM set CFLAGS=/MP /Zi /EHsc /FC
set CFLAGS=/MP /Ox /EHsc /FC
REM 4267 is actually useful, should review!
set WFLAGS=/W4 /wd4018 /wd4100 /wd4189 /wd4201 /wd4244 /wd4334 /wd4267 /wd4505 /wd4996 /WX
set LFLAGS=/SUBSYSTEM:WINDOWS

cl.exe -Isrc src\windows\main.cpp user32.lib kernel32.lib gdi32.lib %BFLAGS% %CFLAGS% %WFLAGS% /Fo%BUILDDIR%\evolve.obj /Fe%EXE% /link %LFLAGS%
if not %errorlevel% == 0 goto :error

cl.exe -Isrc src\viewer\main.cpp /LD %BFLAGS% %CFLAGS% %WFLAGS% /Fo%BUILDDIR%\viewer.obj /Fe%EXEDIR%\viewer.dll
if not %errorlevel% == 0 goto :error

cl.exe -Isrc src\cubes\main.cpp /LD %BFLAGS% %CFLAGS% %WFLAGS% /Fo%BUILDDIR%\cubes.obj /Fe%EXEDIR%\cubes.dll
if not %errorlevel% == 0 goto :error

goto :success

:error
REM Error
exit /B 1

:success
REM Success
exit /B 0
