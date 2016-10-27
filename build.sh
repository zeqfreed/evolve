#!/usr/bin/env bash

OBJDIR="_build"
BINDIR="bin"
DATASYMLINK="${BINDIR}/data"

CC="g++"
CFLAGS="-c -Wall -Wno-missing-braces -g -gmodules -O0 -DMACOSX -Isrc"
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

function build_renderer() {
  prepare

  $CC src/renderer/renderer.cpp $CFLAGS -o $OBJDIR/renderer.o
  result=$?

  if [ $result -eq 0 ]; then
    libtool -macosx_version_min 10.11 -dynamic $OBJDIR/renderer.o -lstdc++ -lSystem -o $OBJDIR/renderer.dylib
    result=$?
    cp $OBJDIR/renderer.dylib $BINDIR/renderer.dylib
  fi

  if [ $result -eq 0 ]; then
    killall -USR1 evolve
  fi
}

function build_exe() {
  prepare

  EXE="evolve"
  OBJS="$OBJDIR/window.o $OBJDIR/assets.o"

  $CC src/macos/window.m $CFLAGS -o $OBJDIR/window.o
  $CC src/macos/assets.cpp $CFLAGS -o $OBJDIR/assets.o
  $CC -o $BINDIR/$EXE $OBJS $LIBS

  return $?
}

function build_all() {
    build_renderer
    build_exe
}

case "$1" in 
    "") build_all;;
    "all") build_all;;
    "renderer") build_renderer;;
    *) echo "Unknown target: $1";;
esac

exit $result
