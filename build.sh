#!/usr/bin/env bash

OBJDIR="_build"
BINDIR="bin"
DATASYMLINK="${BINDIR}/data"

CC="g++"
WFLAGS="-Wall -Wno-missing-braces -Wno-unused-variable -Wno-unused-function"
#CFLAGS="-c ${WFLAGS} -g -gmodules -O0 -DMACOSX -Isrc -DDEBUG"
CFLAGS="-c ${WFLAGS} -O2 -fomit-frame-pointer -ffast-math -DMACOSX -Isrc"
LIBS="-framework Cocoa -framework OpenGL"

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
  result=$?

  if [ $result -eq 0 ]; then
    libtool -macosx_version_min 10.11 -dynamic $OBJDIR/viewer.o -lstdc++ -lSystem -o $OBJDIR/viewer.dylib
    result=$?
    cp $OBJDIR/viewer.dylib $BINDIR/viewer.dylib
  fi
}

function build_cubes() {
  prepare

  $CC src/cubes/main.cpp $CFLAGS -o $OBJDIR/cubes.o
  result=$?

  if [ $result -eq 0 ]; then
    libtool -macosx_version_min 10.11 -dynamic $OBJDIR/cubes.o -lstdc++ -lSystem -o $OBJDIR/cubes.dylib
    result=$?
    cp $OBJDIR/cubes.dylib $BINDIR/cubes.dylib
  fi
}

function build_exe() {
  prepare

  EXE="evolve"
  OBJS="$OBJDIR/platform.o $OBJDIR/main.o"

  $CC src/macos/main.m $CFLAGS -o $OBJDIR/main.o
  $CC src/macos/platform.cpp $CFLAGS -o $OBJDIR/platform.o
  $CC -o $BINDIR/$EXE $OBJS $LIBS

  result=$?
}

function force_reload() {
  if [ $result -eq 0 ]; then
    killall -USR1 evolve
  fi
}

function build_all() {
    build_viewer
    build_cubes
    build_exe

    force_reload
}

case "$1" in 
    "") build_all;;
    "all") build_all;;
    "viewer") build_viewer;;
    "cubes") build_cubes;;
    *) echo "Unknown target: $1";;
esac

exit $result
