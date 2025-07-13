#!/usr/bin/env bash

OBJDIR="_build"
BINDIR="bin"
DATASYMLINK="${BINDIR}/data"

CC="g++ -std=c++11"
OC="g++"
WFLAGS="-Wall -Wno-missing-braces -Wno-unused-variable -Wno-unused-function"
#CFLAGS="-c ${FLAGS} ${WFLAGS} -g -gmodules -O0 -DMACOSX -Isrc -DDEBUG -DPLATFORM_MACOS"
#CFLAGS="-c ${FLAGS} ${WFLAGS} -O2 -mssse3 -mtune=core2 -march=native -fomit-frame-pointer -DPLATFORM_MACOS -DMACOSX -Isrc"
CFLAGS="-c ${FLAGS} ${WFLAGS} -O3 -march=native -DPLATFORM_MACOS -DMACOSX -Isrc"
#CFLAGS="-c ${FLAGS} ${WFLAGS} -g -fsanitize=address -fsanitize-address-use-after-scope -fno-optimize-sibling-calls -fno-omit-frame-pointer -march=native -DPLATFORM_MACOS -DMACOSX -Isrc"
#EXTRA_FLAGS="-fsanitize=address"
LIBS="-framework Cocoa -framework OpenGL"

exitcode=0

export SDKROOT=$(xcrun --sdk macosx --show-sdk-path)
export LIBRARY_PATH="$LIBRARY_PATH:$SDKROOT/usr/lib"

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
    $CC -dynamiclib $OBJDIR/viewer.o $EXTRA_FLAGS -lstdc++ -lSystem -o $OBJDIR/viewer.dylib
    exitcode=$?
    cp $OBJDIR/viewer.dylib $BINDIR/viewer.dylib
  fi
}

function build_cubes() {
  prepare

  $CC src/cubes/main.cpp $CFLAGS -o $OBJDIR/cubes.o
  exitcode=$?

  if [ $exitcode -eq 0 ]; then
    $CC -dynamiclib $OBJDIR/cubes.o $EXTRA_FLAGS -lstdc++ -lSystem -o $OBJDIR/cubes.dylib
    exitcode=$?
    cp $OBJDIR/cubes.dylib $BINDIR/cubes.dylib
  fi
}

function build_sound() {
  #OBJDIR="$OBJDIR/apps/sound"
  prepare

  $CC src/apps/sound/main.cpp $CFLAGS -o $OBJDIR/main.o
  exitcode=$?

  if [ $exitcode -eq 0 ]; then
    $CC -dynamiclib $OBJDIR/cubes.o $EXTRA_FLAGS -lstdc++ -lSystem -o $OBJDIR/sound.dylib
    exitcode=$?
    cp $OBJDIR/sound.dylib $BINDIR/sound.dylib
  fi
}

function build_exe() {
  prepare

  EXE="evolve"
  OBJS="$OBJDIR/platform.o $OBJDIR/main.o"

  $OC src/macos/main.m $CFLAGS -o $OBJDIR/main.o
  $CC src/macos/platform.cpp $CFLAGS -o $OBJDIR/platform.o
  $CC -o $BINDIR/$EXE $OBJS $LIBS $EXTRA_FLAGS

  exitcode=$?
}

function build_dbcdump() {
  OBJDIR="$OBJDIR/tools/dbcdump"
  prepare

  EXE="dbcdump"
  OBJS="$OBJDIR/main.o"

  $CC src/tools/dbcdump/main.cpp $CFLAGS -o $OBJDIR/main.o
  $CC -o $BINDIR/$EXE $OBJS

  exitcode=$?
}

function build_mkfont() {
  OBJDIR="$OBJDIR/tools/mkfont"
  prepare

  EXE="mkfont"
  OBJS="$OBJDIR/main.o"

  $CC src/tools/mkfont/main.cpp $CFLAGS -o $OBJDIR/main.o
  $CC -o $BINDIR/$EXE $OBJS

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
  "dbcdump") build_targets "dbcdump";;
  "mkfont") build_targets "mkfont";;
  "sound") build_targets "sound exe";;
  *) echo "Unknown target: $1";;
esac

exit $exitcode
