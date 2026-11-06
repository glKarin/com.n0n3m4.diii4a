#!/bin/sh
#
# Makefile.am   set env var DWTOPSRCDIR, no args
# CMakeLIsts.txt pass in 2 args  1.top source dir
#    no env vars.

chkres() {
r=$1
m=$2
if [ $r -ne 0 ]
then
  echo "FAIL $m.  Exit status for the test $r"
fi
}

echo "Argument count: $#"
blddir=`pwd`
bname=`basename $blddir`
top_blddir="$blddir"
if [ x$bname = "xtest" ]
then
  # Running in-source-tree.
  top_blddir="$blddir/.."
fi
if [ $# -gt 0  ]
then
  DWTOPSRCDIR="$1"
  echo  "DWTOPSRCDIR from arg $1"
  export DWTOPSRCDIR
fi

if [ x$DWTOPSRCDIR = "x" ]
then
  # Assume runing tests in source
  top_srcdir=$top_blddir
  echo "set DWTOPSRCDIR from top_blddir: $top_blddir"
  DWTOPSRCDIR=$top_blddir
  export DWTOPSRCDIR
else
  top_srcdir=$DWTOPSRCDIR
  echo "set top_srcdir from DWTOPSRCDIR: $top_srcdir"
fi
if [ "x$top_srcdir" = "x.." ]
then
  # This case hopefully eliminates relative path to test dir.
  top_srcdir=$top_blddir
  DWTOPSRCDIR=$top_blddir
  echo "set DWTOPSRCDIR from top_blddir: $top_blddir"
  export DWTOPSRCDIR
fi

# bldloc is the executable directories.
bldloc=$top_blddir/src/bin/dwarfexample
#localsrc is the source dir with baseline data
localsrc=$top_srcdir/test
srcdir=$top_srcdir/test
testbin=$top_blddir/test
testsrc=$top_srcdir/test
echo "TOP topsrc  : $top_srcdir"
echo "TOP topbld  : $top_blddir"
echo "TOP localsrc: $localsrc"

b=$top_srcdir/test/jitreader.base
localsrc=$top_srcdir/test
testbin=$top_blddir/test
tx=$testbin/junk.jitreader.new
if [ -f $top_blddir/src/bin/dwarfexample/.libs/jitreader.exe ]
then
  bldx=$top_blddir/src
  jr=$bldx/bin/dwarfexample/.libs/jitreader.exe
  #echo "dadebug jitreader name ok?"
  #ldd $dd
  #echo "dadebug dll already in dir?"
  ls $bldx/bin/dwarfexample/.libs/libdwarf-*.dll
  # we expect the above ls to usually fail.
  if [ ! -f $bldx/bin/dwarfexample/.libs/libdwarf-*.dll ]
  then
    cp $bldx/lib/libdwarf/.libs/libdwarf-*.dll \
     $bldx/bin/dwarfexample/.libs/
    #echo "dadebug dll now in dir?"
    #ls $bldx/bin/dwarfexample/.libs/libdwarf*
  fi
  bldx=
elif [ -f $top_blddir/src/bin/dwarfexample/jitreader.exe ]
then
  bldx=$top_blddir/src
  jr=$bldx/bin/dwarfexample/jitreader.exe
  #echo "dadebug jitreader name ok?"
  #ldd $dd
  #echo "dadebug dll already in dir?"
  ls $bldx/bin/dwarfexample/libdwarf-*.dll
  # we expect the above ls to usually fail.
  if [ ! -f $bldx/bin/dwarfexample/libdwarf-*.dll ]
  then
    cp $bldx/lib/libdwarf/libdwarf-*.dll \
     $bldx/bin/dwarfexample/
    #echo "dadebug dll now in dir?"
    #ls $bldx/bin/dwarfexample/libdwarf*
  fi
  bldx=
else
  jr=$top_blddir/src/bin/dwarfexample/jitreader
fi
rm -f $tx
echo "Running: $jr with env var DWTOPSRCDIR: $DWTOPSRCDIR"
$jr
$jr > $tx
r=$?
chkres $r "$jr printing output to $tx base $b "
if [ $r -ne 0 ]
then
   cat $tx
   exit $r
fi
echo "Running: python3 ${localsrc}/test_dwdiff.py $b $tx"
python3 ${localsrc}/test_dwdiff.py $b $tx
r=$?
if [ $r -ne 0 ]
then
  echo "Failed diff above."
  echo "To update , mv  $tx $b"
  exit $r
fi
rm -f $tx
exit 0
