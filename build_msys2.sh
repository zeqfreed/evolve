#!/usr/bin/env bash

OBJDIR="_build"
BINDIR="bin"
DATASYMLINK="${BINDIR}/data"

CC="g++ -std=c++11"
OC="g++"
FLAGS="-mno-ms-bitfields -D__USE_MINGW_ANSI_STDIO=1 -DCOLOR_BGR"
WFLAGS="-Wall -Wno-missing-braces -Wno-unused-variable -Wno-unused-function"
#CFLAGS="-c ${FLAGS} ${WFLAGS} -g -O0 -Isrc -DDEBUG -DPLATFORM=WINDOWS"
CFLAGS="-c ${FLAGS} ${WFLAGS} -O2 -march=native -DPLATFORM=WINDOWS -Isrc"
LINK_FLAGS="-static-libgcc -static-libstdc++ -static -lwinpthread"
LIBS="-luser32 -lkernel32 -lgdi32"

exitcode=0

function prepare() {
  if [ ! -d $OBJDIR ]; then
    mkdir -p $OBJDIR;
  fi

  if [ ! -d $BINDIR ]; then
    mkdir -p $BINDIR;
  fi

  if [ ! -x $DATASYMLINK ]; then
    ln -s ../data $DATASYMLINK
  fi
}

function build_viewer() {
  prepare

  $CC src/viewer/main.cpp $CFLAGS -o $OBJDIR/viewer.o
  exitcode=$?

  if [ $exitcode -eq 0 ]; then
    $CC -shared $OBJDIR/viewer.o $LINK_FLAGS -o $OBJDIR/viewer.dll
    exitcode=$?
    cp $OBJDIR/viewer.dll $BINDIR/viewer.dll
  fi
}

function build_cubes() {
  prepare

  $CC src/cubes/main.cpp $CFLAGS -o $OBJDIR/cubes.o
  exitcode=$?

  if [ $exitcode -eq 0 ]; then
    $CC -shared $OBJDIR/cubes.o $LINK_FLAGS -o $OBJDIR/cubes.dll
    exitcode=$?
    cp $OBJDIR/cubes.dll $BINDIR/cubes.dll
  fi
}

function build_exe() {
  prepare

  EXE="evolve"
  OBJS="$OBJDIR/main.o"

  $OC src/windows/main.cpp $CFLAGS -o $OBJDIR/main.o
  $CC $LINK_FLAGS -o $BINDIR/$EXE $OBJS $LIBS

  exitcode=$?
}

# function force_reload() {
#   if [ $exitcode -eq 0 ]; then
#     killall -USR1 evolve
#   fi
# }

function build_targets() {
  for target in $1
  do
    if [ $exitcode -eq 0 ]; then
      eval "build_$target"
    fi
  done

  #force_reload
}

function build_all() {
  build_targets "viewer cubes exe"
}

case "$1" in
  "") build_all;;
  "all") build_all;;
  "viewer") build_targets "viewer exe";;
  "cubes") build_targets "cubes exe";;
  "dbcdump") build_targets "dbcdump";;
  "mkfont") build_targets "mkfont";;
  *) echo "Unknown target: $1";;
esac

exit $exitcode
