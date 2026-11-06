#!/bin/sh
# This is a script used to create
# dummyexecutable and dummyexecutable.debug
# Just doing what objcopy provides.
# It's currently not useable for automated regression testing
# without more work to make it independent of source/build
# location.
# so do not run this.
exit 1
d=dummyexecutable
mv dummysourceignore $d.c
cc -g $d.c -o $d
objcopy --only-keep-debug  $d $d.debug
objcopy --strip-debug      $d
objcopy --add-gnu-debuglink=$d.debug  $d
# by moving the $d.debug we ensure that
# it can only be found if the proper path is provided
# to dwdebuglink
rm -rf dummydir
mkdir -p dummydir/home/davea/dwarf/code/dwarfexample/
cp $d.debug dummydir/home/davea/dwarf/code/dwarfexample/
echo "Test1 "
/tmp/dwdebuglink $d

echo "Test2 "
/tmp/dwdebuglink --add-debuglink-path=/globala/ax --add-debuglink-path=./dummydir $d
rm -rf dummydir
