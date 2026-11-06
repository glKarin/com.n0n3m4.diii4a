#!/bin/sh
# Copyright (C) 2021 David Anderson
# This script is hereby placed in the Public Domain
# for anyone to use in any way for any purpose.
#
# Assumes running from the build directory: path/test
# which is sometimes
#    /var/tmp/bld/test
# Sets variables top_srcdir and top_blddir
# If DWTOPSRCDIR is set on entry, top_srcdir
# is set to the value of $DWTOPSRCDIR
# as in this case we are running the script outside
# of the source tree.
#
# This works for cmake,configure, not meson
top_blddir=`pwd`/..

if [ $# -gt 0 ]
then
  top_srcdir="$1"
  if [ $# -gt 1 ]
  then
    x="$2"
    if [ "$x" = "ninja" ]
    then
      #  For meson only. build run in base build, not test/
      top_blddir=`pwd`
      echo "For ninja set top blddir $top_blddir"
    else
      echo "ignore, we leave top_blddir as above."
    fi
  fi
else
  if [ x$DWTOPSRCDIR = "x" ]
  then
    # Running in the source tree
      top_srcdir=$top_blddir
  else
    # Running outside the source tree (the normal case)
    top_srcdir=$DWTOPSRCDIR
  fi
fi
srcdir=$top_srcdir/test
echo "TOP topsrc $top_srcdir topbld $top_blddir localsrc $srcdir"
chkres() {
r=$1
m=$2
if [ $r -ne 0 ]
then
  echo "FAIL $m.  Exit status for the test $r"
fi
}

# In addition the zero date for file time in line tables
# prints differently for different time zones.
textlim=700
cp "$top_srcdir/src/bin/dwarfdump/dwarfdump.conf" .
# detects Windows msys3 mingw executable
if [ -f $top_blddir/src/bin/dwarfdump/.libs/dwarfdump.exe ]
then
  bldx=$top_blddir/src
  dd=$bldx/bin/dwarfdump/.libs/dwarfdump.exe
  if [ ! -f  $bldx/bin/dwarfdump/.libs/libdwarf-*.dll ]
  then
    x="cp $bldx/lib/libdwarf/.libs/libdwarf-*.dll \
     $bldx/bin/dwarfdump/.libs/"
    echo "Do: $x"
    $x
    ls  $bldx/bin/dwarfdump/.libs/libdwarf*
  fi
  bldx=
else
  dd=$top_blddir/src/bin/dwarfdump/dwarfdump
fi

# Delete what follows 'last time 0x0'
fixlasttime() {
  i=$1
  t=$2
  echo "Fix Last Time to 0, mv $t $i"
  sed 's/last time 0x.*/last time 0x0/' <$i >$t
  mv $t $i
}

