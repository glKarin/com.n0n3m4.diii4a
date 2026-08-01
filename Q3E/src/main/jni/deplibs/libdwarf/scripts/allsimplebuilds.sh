#!/bin/sh
# Copyright (C) 2021 David Anderson
# This test script is in the public domain for use
# by anyone for any purpose.

echo "simple builds. "
echo " sh simplebuilds "
echo " Run from the libdwarf base source directory"
echo " as scripts/allsimplebuilds.sh "

if [ $# -ne 0 ]
then
  echo " No arguments are necessary or allowed"
  exit 1
fi

spath=`pwd`
d=2.3.2
tpath="/tmp/allsimple-v$d"
bbase=$tpath
tbase="/tmp/allsimpleinstalled-v$d"

echo 'Starting allsimplebuilds.sh' \
   `date "+%Y-%m-%d %H:%M:%S"`
stsecs=`date '+%s'`


if [ ! -d src ]
then
  echo "Looks like not running from a source directory."
  echo "Instead make a temporary directory and run from there."
  exit 1
elif [ ! -d test ]
then
  echo "Looks like not running from a source directory."
  echo "Instead make a temporary directory and run from there."
  exit 1
elif [ ! -d doc ]
then
  echo "Looks like not running from a source directory."
  echo "Instead make a temporary directory and run from there."
  exit 1
elif [ ! -f configure.ac ]
then
  echo "Looks like not running from a source directory."
  echo "Instead make a temporary directory and run from there."
  exit 1
elif [ ! -f README.md ]
then
  echo "Looks like not running from a source directory."
  echo "Instead make a temporary directory and run from there."
  exit 1
fi

haveapp() {
  x=`which $1`
  if [ "x$x" = "x" ]
  then
    return 1
  fi
  xb=`basename $x`
  if [ "x$xb" = "x$1" ]
  then
    return 0
  fi
  return 1
}

chkres() {
  if [ $1 -ne 0 ]
  then
    echo "FAIL: $2"
    exit 1
  fi
}


haveapp make
if [ $? -eq 0 ]
then
  havemake=yes
else
  havemake=no
fi
haveapp automake
if [ $? -eq 0 ]
then
  haveautomake=yes
else
  haveautomake=no
fi
haveapp cmake
if [ $? -eq 0 ]
then
  havecmake=yes
else
  havecmake=no
fi
haveapp meson
if [ $? -eq 0 ]
then
  havemeson=yes
else
  havemeson=no
fi
haveapp ninja
if [ $? -eq 0 ]
then
  haveninja=yes
else
  haveninja=no
fi
if [ "x$USERDOMAIN" = "xMSYS" ]
then
  havemsys2=yes
else
  havemsys2=no
fi
echo "have make...................: $havemake"
echo "have automake...............: $haveautomake"
echo "have cmake..................: $havecmake"
echo "have meson..................: $havemeson"
echo "have ninja..................: $haveninja"
echo "have msys2..................: $havemsys2"

echo "Source directory............: $spath"
echo "Run builds in temp directory: $bbase"
echo "Store install directories in: $tbase"
echo ""
echo "sleep 6"
sleep 6

echo "Clean out $bbase $tbase entirely."
rm -rf $bbase
rm -rf $tbase
mkdir $bbase
chkres $? "Failed to create $bbase"
mkdir $tbase
chkres $? "Failed to create $tbase"

cd $tpath
chkres $? "cd to $tpath fails"
rm -rf *

echo "You are unlikely to ever need libdwarfp or"
echo "dwarfgen but we build those here for completeness"
echo "Building with configure, shared library"
if [ "$havemake" = "no" ]
then
   echo "SKIP configure shared library "
else
  prefx=$tbase/allconfiguresharedinstalled
  bb=$bbase/configureshared
  mkdir $bb
  chkres $? "setup $bb configure shared directory fail a"
  cd $bb
  chkres $? "cd configure shared directory fail b"
  $spath/configure \
    --disable-static   \
    --enable-shared \
    --enable-dwarfexample \
    --enable-dwarfgen \
    --prefix=$prefx
  chkres $? "configure setup shared fail c"
  make
  chkres $? "configure setup shared fail d"
  make install
  chkres $? "configure setup shared fail e"
  make check
  chkres $? "configure setup shared fail f"
fi

echo ""
echo "Building with configure, static library"
if [ "$havemake" = "no" ]
then
   echo "SKIP configure static library"
elif [ "$havemsys2" = "yes"  ]
then
   echo "SKIP configure static library on Windows msys2"
else
  prefx=$tbase/allconfigurestaticinstalled
  bb=$bbase/configurestatic
  mkdir $bb
  chkres $? "setup $bb configure static directory fail a"
  cd $bb
  chkres $? "cd configure static directory fail b"
  $spath/configure \
    --disable-shared   \
    --enable-static \
    --enable-dwarfexample \
    --enable-dwarfgen \
    --prefix=$prefx
  chkres $? "configure setup static fail c"
  make
  chkres $? "configure setup static fail d"
  make install
  chkres $? "configure setup static fail e"
  make check
  chkres $? "configure setup static fail f"
fi

echo ""
echo "Building with meson, shared library"
if [ "$havemeson" = "no" -o "$haveninja" = "no" ]
then
   echo "SKIP meson shared library"
else
  prefx=$tbase/allmesonsharedinstalled
  bb=$bbase/mesonshared
  mkdir $bb
  chkres $? "setup $bb configure static directory fail a"
  cd $bb
  chkres $? "cd meson shared directory fail b"
  meson setup \
    --default-library shared \
    --prefix=$prefx \
    -Dwerror=false  \
    -Ddwarfexample=true \
    -Ddwarfgen=true \
    . $spath
  chkres $? "Meson setup shared fail c"
  ninja
  chkres $? "Meson ninja shared fail d"
  ninja install
  chkres $? "Meson ninja install shared fail e"
  ninja test
  chkres $? "Meson ninja test shared fail f"
fi

echo ""
echo "Building with meson, static library"
if [ "$havemeson" = "no" -o  "$haveninja" = "no" ]
then
   echo "SKIP meson static library"
else
  prefx=$tbase/allmesonstaticinstalled
  bb=$bbase/mesonstatic
  mkdir $bb
  chkres $? "setup $bb meson static directory fail a"
  cd $bb
  chkres $? "cd meson static directory fail b"
  meson setup \
    --default-library static \
    --prefix=$prefx \
    -Dwerror=false  \
    -Ddwarfexample=true \
    -Ddwarfgen=true \
    . $spath
  chkres $? "Meson setup static library fail c"
  ninja
  chkres $? "Meson ninja static library fail d"
  ninja install
  chkres $? "Meson ninja install static library fail e"
  ninja test
  chkres $? "Meson ninja test static library fail f"
fi

echo ""
echo "Building with cmake, shared library"
if [ "$havecmake" = "no" -o  "$haveninja" = "no" ]
then
   echo "SKIP cmake shared library"
else
  prefx=$tbase/allcmakesharedinstalled
  bb=$bbase/cmakeshared
  mkdir $bb
  chkres $? "setup $bb cmake shared directory fail a"
  cd $bb
  chkres $? "cd cmake shared directory fail b"
  cmake -G Ninja  \
    -DBUILD_SHARED=YES \
    -DBUILD_NON_SHARED=NO \
    -DBUILD_DWARFEXAMPLE:BOOL=YES \
    -DBUILD_DWARFGEN:BOOL=YES \
    -DDO_TESTING:BOOL=YES \
    -DCMAKE_INSTALL_PREFIX=$prefx \
    $spath
  chkres $? "cmake setup shared library fail c"
  ninja
  chkres $? "cmake setup shared library fail d"
  ninja install
  chkres $? "cmake setup shared library fail e"
  ninja test
  chkres $? "cmake setup shared library fail f"
fi

echo ""
echo "Building with cmake, static library"
if [ "$havecmake" = "no" -o  "$haveninja" = "no" ]
then
   echo "SKIP cmake static library"
else
  prefx=$tbase/allscmakestaticinstalled
  bb=$bbase/cmakestatic
  mkdir $bb
  chkres $? "setup $bb cmake static directory fail a"
  cd $bb
  chkres $? "cd cmake static directory fail b"
  cmake -G Ninja  \
    -DBUILD_DWARFEXAMPLE:BOOL=YES \
    -DBUILD_DWARFGEN:BOOL=YES \
    -DBUILD_SHARED=NO \
    -DBUILD_NON_SHARED=YES \
    -DDO_TESTING:BOOL=YES \
    -DCMAKE_INSTALL_PREFIX=$prefx \
    $spath
  chkres $? "cmake setup static library fail c"
  ninja
  chkres $? "cmake setup static library fail d"
  ninja install
  chkres $? "cmake setup static library fail e"
  ninja test
  chkres $? "cmake setup static library fail f"
fi

for i in $tbase/allconfiguresharedinstalled $tbase/allconfigurestaticinstalled $tbase/allmesonsharedinstalled $tbase/allmesonstaticinstalled $tbase/allcmakesharedinstalled $tbase/allscmakestaticinstalled
do
  if [ -d $i ]
  then
    find . -type f -print >/tmp/x.$$
    l=`wc -l < /tmp/x.$$`
    echo "$i has $l files"
    rm /tmp/x.$$
  else
    echo "Skip $i, it is not built."
  fi
done
cd $spath
ndsecs=`date '+%s'`
showminutes() {
   t=`expr  \( $2 \- $1 \+ 29  \) \/ 60`
   echo "Run time in minutes: $t"
}
showminutes $stsecs $ndsecs
echo "PASS scripts/allsimplebuilds.sh"
exit 0
