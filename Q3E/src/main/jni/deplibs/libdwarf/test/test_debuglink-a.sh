#!/bin/sh
#
# Intended to be run only on local machine.
# Run only after config.h created in a configure
# in the source directory
#
# Either pass in the top source dir as an argument
# or set env var DWTOPSRCDIR to the source directory.

chkres() {
r=$1
m=$2
if [ $r -ne 0 ]
then
  echo "FAIL $m.  Exit status for the test $r"
  exit 1
fi
}

echo "Argument count: $#"
if [ $# -gt 0 ]
then
  top_srcdir="$1"
else
  if [ x$DWTOPSRCDIR = "x" ]
  then
    top_srcdir=$top_blddir
    echo "top_srcdir from top_blddir $top_srcdir"
  else
    top_srcdir=$DWTOPSRCDIR
    echo "top_srcdir from DWTOPSRCDIR $top_srcdir"
  fi
fi
blddir=`pwd`
bname=`basename $blddir`
top_blddir="$blddir"
if [ x$bname = "xtest" ]
then
  top_blddir="$blddir/.."
fi

if [ "x$top_srcdir" = "x.." ]
then
  # Should never happen if run by make or equivalent
  # This case hopefully eliminates relative path to test dir.
  top_srcdir=$top_blddir
fi
# bld loc to find dwdebuglink executable.
bldloc=$top_blddir/src/bin/dwarfexample
if [ -f $bldloc/.libs/dwdebuglink.exe ]
then
  bldx=$bldloc/
  dwdl=$bldloc/.libs/debuglink.exe
  cp $top_blddir/src/lib/libdwarf/.libs/msys-dwarf-*.dll \
     $bldloc/.libs/
  bldx=
else
  dwdl=$bldloc/dwdebuglink
fi
#localsrc is the source dir with baseline data
localsrc=$top_srcdir/test

testbin=$top_blddir
testsrc=$top_srcdir/test
# So we know the build. Because of debuglink.
echo "DWARF_BIGENDIAN=$DWARF_BIGENDIAN"

echo "TOP topsrc  : $top_srcdir"
echo "TOP topbld  : $top_blddir"
echo "TOP localsrc: $localsrc"

if [ x"$DWARF_BIGENDIAN" = "xyes" ]
then
  echo "SKIP test_debuglink-a.sh , cannot work on bigendian build "
else
  echo "test_debuglink-a.sh "
  o=junk.dlinka
  p="--add-debuglink-path=/exam/ple"
  p2="--add-debuglink-path=/tmp/phony"
  echo "Run: $dwdl $p $p2 $testsrc/dummyexecutable "
  $dwdl $p $p2 $testsrc/dummyexecutable > $testbin/$o
  r=$?
  chkres $r "test_debuglink-a.sh running dwdebuglink test1"
  # we strip out the actual localsrc and blddir for the obvious
  # reason: We want the baseline data to be meaningful no matter
  # where one's source/build directories are.
  ${localsrc}/canonicalpath.py $testbin/$o $localsrc content > $testbin/${o}ac
  ${localsrc}/canonicalpath.py $testbin/${o}ac $blddir content > $testbin/${o}a
  ${localsrc}/test_dwdiff.py $testsrc/debuglink.base $testbin/${o}a
  r=$?
  echo "To update test_debuglink-a.sh baseline:"
  echo "mv $testbin/${o}a $testsrc/debuglink.base"
  chkres $r "running test_debuglink-a.sh test1 diff against baseline"
fi
rm -f $testbin/$o
rm -f $testbin/${o}a
exit 0
