set WORKDIR=%~dp0
cd %WORKDIR%

set BUILDDIR=%WORKDIR%_build
set EXEDIR=%WORKDIR%bin
set EXE=%EXEDIR%\evolve

if not exist %BUILDDIR% mkdir %BUILDDIR%
if not exist %EXEDIR% mkdir %EXEDIR%

set BFLAGS=/DCOLOR_BGR /DPLATFORM_WINDOWS
REM set CFLAGS=/MP /Zi /EHsc /FC /Fd"%BUILDDIR%/"
set CFLAGS=/MP /Ox /EHsc /FC
REM 4267 and 4018 is actually useful, should review!
set WFLAGS=/W4 /wd4018 /wd4100 /wd4189 /wd4201 /wd4244 /wd4334 /wd4267 /wd4505 /wd4996 /WX
set LFLAGS=/SUBSYSTEM:WINDOWS

cl.exe -Isrc /Fo"%BUILDDIR%\evolve.obj" /Fe"%EXE%" /Fd"%BUILDDIR%/" /Tc src\windows\main.cpp user32.lib kernel32.lib gdi32.lib dsound.lib %BFLAGS% %CFLAGS% %WFLAGS% /link %LFLAGS%
if not %errorlevel% == 0 goto :error

cl.exe -Isrc /Fo"%BUILDDIR%\viewer.obj" /Fe"%EXEDIR%\viewer.dll" /Fd"%BUILDDIR%/" src\viewer\main.cpp /LD %BFLAGS% %CFLAGS% %WFLAGS%
if not %errorlevel% == 0 goto :error

cl.exe -Isrc /Fo"%BUILDDIR%\cubes.obj" /Fe"%EXEDIR%\cubes.dll" /Fd"%BUILDDIR%/" src\cubes\main.cpp /LD %BFLAGS% %CFLAGS% %WFLAGS%
if not %errorlevel% == 0 goto :error

cl.exe -Isrc /Fo"%BUILDDIR%\sound.obj" /Fe"%EXEDIR%\sound.dll" /Fd"%BUILDDIR%/" src\apps\sound\main.cpp /LD %BFLAGS% %CFLAGS% %WFLAGS% /link %LFLAGS%
if not %errorlevel% == 0 goto :error

goto :success

:error
REM Error
exit /B 1

:success
REM Success
exit /B 0
