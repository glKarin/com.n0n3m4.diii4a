#!/bin/sh
# Copyright (C) 2021 David Anderson
# This script is hereby placed in the Public Domain
# for anyone to use in any way for any purpose.
#
# to call this either pass top srcdir as arg1
# or set env var DWTOPSRCDIR to that directory.
#
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
. $t/test/test_dwarfdumpsetup.sh $t

f=$top_srcdir/test/test-mach-o-32.dSYM
b=$top_srcdir/test/test-mach-o-32.base
localsrc=$top_srcdir/test
testbin=$top_blddir/test
tx=$testbin/junk.test-mach-o-32.base
tx2=$testbin/junk2.test-mach-o-32.base
rm -f $tx
echo "start dwarfdumpMacos.sh dwarfdump sanity check on $f"
echo "Run: $dd -a -vvv  $f | head -n $textlim"
$dd -a -vvv $f | head -n $textlim > $tx
r=$?
chkres $r "FAIL test_dwarfdumpMacos.sh $dd $f to $tx base $b "
echo "report file lengths"
wc -l $b  $tx
if [ $r -ne 0 ]
then
  echo "$dd FAILED"
  exit 1
fi
echo "if update required, mv $tx $b"
# tx2 is a temp file, tx is the input and the output.
fixlasttime $tx $tx2
${localsrc}/test_dwdiff.py $b $tx
r=$?
chkres $r "FAIL test_dwarfdumpMacos.sh diff of $b $tx"
if [ $r -ne 0 ]
then
  echo "FAIL  test_dwdiff $b $tx"
  echo "to update , mv $tx $b"
  exit $r
fi
rm -f dwarfdump.conf
rm -f $tx
rm -f $tx.diff
echo "PASS test_dwarfdumpMacos.sh"
exit 0
