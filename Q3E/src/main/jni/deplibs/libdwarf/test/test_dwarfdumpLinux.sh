#!/bin/sh
# Copyright (C) 2021 David Anderson
# This script is hereby placed in the Public Domain
# for anyone to use in any way for any purpose.
#
# To call this:
# Either set arg1 to the top source dir
# or set env var DWTOPSRCDIR to the top source dir.
echo "dwarfdumpLinux.sh start. values before starting up."
echo "topsrcdir $top_srcdir"
echo "topblddir $top_blddir"
echo "DWTOPSRCDIR $DWTOPSRCDIR"
ninja=n

if [ $# -gt 0  ]
then
  t="$1"
  if [ $# -gt 1  ]
  then
    y="$2"
    if [ "$y" = "ninja" ]
    then
       # Needed for meson as test run is not in test/ itself
       minja=y
       top_blddir=`pwd`
       echo "ninja: reset top blddir to $top_blddir"
    fi
    # else ignore  $2
  fi
else
  if [ x$DWTOPSRCDIR = "x" ]
  then
    # Running from the source tree
    t=$top_blddir
  else
    # Running outside of source tree (the usual case)
    t=$DWTOPSRCDIR
  fi
fi
# Do some setup
. $t/test/test_dwarfdumpsetup.sh $t $y
echo "Now top_srcdir  $top_srcdir"
echo "Now top_blddir  $top_blddir"
f=$top_srcdir/test/testuriLE64ELf.testme
b=$top_srcdir/test/testuriLE64ELf.base
localsrc=$top_srcdir/test
testbin=$top_blddir/test
tx=$testbin/junk.testuriLE64ELf.base
tx2=$testbin/junk2.testuriLE64ELf.base
rm -f $tx
echo "start  test_dwarfdumpLinux.sh sanity check on $f"
echo "Run: $dd -a -vvv  $f | head -n $textlim"
$dd -vvv -a $f | head -n $textlim > $tx
r=$?
chkres $r "test_dwarfdumpLinux.sh running $dd $f output to $tx base $b "
if [ $r -ne 0 ]
then
  echo "$dd failed"
  exit $r
fi
echo "if update required, mv $tx $b"
# Result winds up in $tx, and $tx2 was just a temp file.
fixlasttime $tx $tx2
${localsrc}/test_dwdiff.py $b $tx
r=$?
chkres $r "FAIL dwarfdumpLinux.sh dwdiff.py"
echo "report file lengths"
wc -l $b  $tx
if [ $r -ne 0 ]
then
  echo "FAIL diff $b $tx"
  echo "To update , mv  $tx $b"
  exit 0
fi
chkres $r "FAIL test_dwarfdumpLinux.sh diff of $b $tx"
rm -f dwarfdump.conf
rm -f $tx
rm -f $tx.diff
exit 0
