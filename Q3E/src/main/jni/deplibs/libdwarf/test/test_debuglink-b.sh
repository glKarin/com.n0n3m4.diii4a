#!/bin/sh
#
# Intended to be run only on local machine.
# Run only after config.h created in a configure
# in the source directory
#
# Assumes we run the script in the test directory of the build.
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
  # This case hopefully eliminates relative path to test dir.
  top_srcdir=$top_blddir
fi
# bldloc is the executable directories.
bldloc=$top_blddir/src/bin/dwarfexample
if [ -f $bldloc/.libs/dwdebuglink.exe ]
then
  bldx=$bldloc/dwarfexample
  dwdl=$bldloc/.libs/debuglink.exe
  cp $top_blddir/src/lib/libdwarf/.libs/msys-dwarf-*.dll \
     $bldloc/.libs/
  bldx=
else
  dwdl=$bldloc/dwdebuglink
fi

#localsrc is the source dir with baseline data
localsrc=$top_srcdir/test
srcdir=$top_srcdir/test
testbin=$top_blddir/test
testsrc=$top_srcdir/test
# So we know the build. Because of debuglink.

echo "TOP topsrc  : $top_srcdir"
echo "TOP topbld  : $top_blddir"
echo "TOP localsrc: $localsrc"
chkres() {
r=$1
m=$2
if [ $r -ne 0 ]
then
  echo "FAIL $m.  Exit status for the test $r"
  exit 1
fi
}

echo "test_debuglink-b.sh test2"
o=junk.dlinkb
p=" --no-follow-debuglink --add-debuglink-path=/exam/ple"
p2="--add-debuglink-path=/tmp/phony"
echo "Run: $dwdl $p $p2 $testsrc/dummyexecutable "
$dwdl $p $p2 $testsrc/dummyexecutable > $testbin/$o
r=$?
chkres $r "running dwdebuglink test2"
${localsrc}/canonicalpath.py $testbin/$o $localsrc content > $testbin/${o}ac
${localsrc}/canonicalpath.py $testbin/${o}ac $blddir content > $testbin/${o}c
${localsrc}/test_dwdiff.py $testsrc/debuglink2.base $testbin/${o}c
r=$?
echo "To update test_debuglink-b.sh  baseline:"
echo " mv $testbin/${o}c $testsrc/debuglink2.base"
chkres $r "running test_debuglink-b.sh  diff against baseline"
rm -f $testbin/$o
rm -f $testbin/${o}c
exit 0
