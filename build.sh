#!/usr/bin/env bash

OBJDIR="_build"
BINDIR="bin"
DATASYMLINK="${BINDIR}/data"

CC="g++"
WFLAGS="-Wall -Wno-missing-braces -Wno-unused-variable -Wno-unused-function"
#CFLAGS="-c ${WFLAGS} -g -gmodules -O0 -DMACOSX -Isrc -DDEBUG"
CFLAGS="-c ${WFLAGS} -O2 -mssse3 -mtune=core2 -march=native -fomit-frame-pointer -DMACOSX -Isrc"
LIBS="-framework Cocoa -framework OpenGL"

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
    libtool -macosx_version_min 10.11 -dynamic $OBJDIR/viewer.o -lstdc++ -lSystem -o $OBJDIR/viewer.dylib
    exitcode=$?
    cp $OBJDIR/viewer.dylib $BINDIR/viewer.dylib
  fi
}

function build_cubes() {
  prepare

  $CC src/cubes/main.cpp $CFLAGS -o $OBJDIR/cubes.o
  exitcode=$?

  if [ $exitcode -eq 0 ]; then
    libtool -macosx_version_min 10.11 -dynamic $OBJDIR/cubes.o -lstdc++ -lSystem -o $OBJDIR/cubes.dylib
    exitcode=$?
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

  exitcode=$?
}

function force_reload() {
  if [ $exitcode -eq 0 ]; then
    killall -USR1 evolve
  fi
}

function build_targets() {
  for target in $1
  do
    if [ $exitcode -eq 0 ]; then
      eval "build_$target"
    fi
  done

  force_reload
}

function build_all() {
  build_targets "viewer cubes exe"
}

case "$1" in 
  "") build_all;;
  "all") build_all;;
  "viewer") build_targets "viewer exe";;
  "cubes") build_targets "cubes exe";;
  *) echo "Unknown target: $1";;
esac

exit $exitcode
