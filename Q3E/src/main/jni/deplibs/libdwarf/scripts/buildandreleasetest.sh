#!/bin/sh
# Copyright (C) 2021 David Anderson
# This test script is in the public domain for use
# by anyone for any purpose.
echo "sh scripts/buildandreleasetest.sh [--static|--shared]"
echo "   [--disable-dwarfgen] [--enable-wall] [--savebart]"
echo "   --savebart means do not delete temp files"
echo "   Defaults to shared library build and use"

#  A script verifying the distribution gets all needed files
#  for building, including "make check"
#
#  For a guaranteed clean run:
#    sh autogen.sh
#    sh scripts/buildandreleasetest.sh
#  All the generated files are in /tmp/bart
#

echo 'Starting buildandreleasetest.sh:' \
   `date "+%Y-%m-%d %H:%M:%S"`
stsecs=`date '+%s'`

shared=y
configureopt="--enable-shared --disable-static"
cmakeopt="-DBUILD_SHARED=NO -DBUILD_NON_SHARED=YES"
mesonopt="--default-library static"

# accomodate differences in cmake install file count:
# 16 is for 32bit build
# 18 is for 64bit build
#expectlen32=16
#expectlen64=18
genopta="--enable-dwarfgen"
genoptb="-DBUILD_DWARFGEN=ON"
wd=`pwd`
# If passes, remove the /tmp/bart working directory.
# Useful to consider if all intended files actually present,
# including any possibly not used.
savebart=n
enablewall=n
staticbuild=y
while [ $# -ne 0 ]
do
  case $1 in
   --static ) shared=n ;
     staticbuild=y
     configureopt="--enable-static --disable-shared"
     cmakeopt="-DBUILD_SHARED=NO -DBUILD_NON_SHARED=YES"
     mesonopt="--default-library static"
     shift  ;;
   --shared ) shared=y ;
     staticbuild=n
     configureopt="--enable-shared --disable-static"
     cmakeopt="-DBUILD_SHARED=YES -DBUILD_NON_SHARED=NO"
     mesonopt="--default-library shared"
     shift  ;;
   --enable-wall )  enablewall=y;  shift  ;;
   --savebart ) savebart=y ; shift  ;;
   --disable-dwarfgen ) genopta='' ; genoptb='' ; shift  ;;
   * ) echo "Unknown buildandreleasetest.sh option $1. Error." ; exit 1 ;;
  esac
done
if [ "x$enablewall" = "y" ]
then
    configureopt="$configureopt --enable-wall"
    cmakeopt="$cmakeopt -DWALL=YES"
    mesonopt="$mesonopt -Dwerror=true"
else
    cmakeopt="$cmakeopt"
    mesonopt="$mesonopt -Dwerror=false"
fi
echo "Build specific options:"
echo " configure   : $configureopt"
echo " cmake       : $cmakeopt"
echo " meson       : $mesonopt"
echo "savebart flag:...: $savebart"
if [ "$staticbuild" = "y" ]
then
  if [ "x$USERDOMAIN" = "xMSYS" ]
  then
    echo "Libdwarf configure objects to a static build"
    echo "on Windows Msys2 so this script will not work"
    echo "here. Giving up."
    exit 1
  fi
fi
if [ -f ./configure.ac ]
then
  f=./configure.ac
else
  if [ -f ../configure.ac ]
  then
    f=../configure.ac
  else
    echo "FAIL Running distribution test from the wrong place."
    exit
  fi
fi
if [ ! -x ./configure ]
then
  echo "./configure is missing or not executable."
  echo "Possibly one needs to run autogen.sh?"
  echo "Giving up"
  exit 1
fi
v=`./configure --version | head -n 1 | cut -f 3 -d " "`
echo "configure.ac version is v=$v"
if [ x$v = "x" ]
then
   echo FAIL did not get configure.ac version
   exit 1
fi

chkres() {
  if [ $1 -ne 0 ]
  then
    echo "$2"
    exit 1
  fi
}

mdirs() {
while [ $# -ne 0 ]
do
  f=$1
  rm -rf $f
  mkdir $f
  chkres $? "mkdir $f failed!"
  shift
done
}

safecd() {
  f=$1
  cd $f
  chkres $? "cd $f failed $2"
}
safemv() {
  s=$1
  t=$2
  echo "mv $s $t"
  mv $s $t
  chkres $?  "mv $f $t failed  $3"
}

showinstalled()  {
  dir=$1
  msg=$2
  tmpdir=$3

  echo "REPORT OF INSTALLED FILES  in $dir $msg"
  if [ ! -d $dir ]
  then
     echo "Target install directory $1 does not exist"
     echo "Fatal error"
     exit 1
  fi
  find $dir -type f -print >$tmpdir
  len=`wc -l <$tmpdir`
  echo "Number of files $len"
  cat $tmpdir
  echo "======end of install list"
  echo ""
}

configloc=$wd/configure
bart=/tmp/bart
abld=$bart/a-dwbld
ainstall=$bart/a-install
binstrelp=$bart/a-installrelp
binstrelbld=$bart/b-installrelbld
blibsrc=$bart/b-libsrc
crelbld=$bart/c-installrelbld
cinstrelp=$bart/c-installrelp
dbigend=$bart/d-bigendian
ecmakebld=$bart/e-cmakebld
fcmakebld=$bart/f-cmakebld
fcmakeinst=$bart/f-cmakeinstalltarg
gcmakebld=$bart/g-cmakebld
hcmakebld=$bart/h-cmakebld
imesonbld=$bart/i-mesonbld
mdirs $bart $abld $ainstall $binstrelp $binstrelbld $crelbld
mdirs $cinstrelp $dbigend $ecmakebld $fcmakebld $gcmakebld
mdirs $hcmakebld $imesonbld
relset=$bart/a-gzfilelist
atfout=$bart/a-tarftout
btfout=$bart/b-tarftout
btfoutb=$bart/b-tarftoutb
ftfout=$bart/f-tarftout
itfout=$bart/i-tarftout
rm -rf $bart/a-dwrelease
rm -rf $blibsrc

arelgz=$bart/a-dwrelease.tar.gz
brelgz=$bart/b-dwrelease.tar.gz
rm -rf $arelgz
rm -rf $brelgz
echo "dirs created empty"

echo cd $abld
safecd $abld "FAIL A cd failed"
echo "now: $configloc $configureopt --prefix=$ainstall"
$configloc $configureopt --prefix=$ainstall
r=$?
chkres $r "FAIL A4a configure fail"
echo "TEST Section A: initial $ainstall make install"
make
make doc
chkres $? "FAIL Section A 4rb make doc"
make install
chkres $? "FAIL Section A 4b make install"
showinstalled $ainstall "using configure" $atfout
ls -lR $ainstall
make dist
chkres $? "FAIL make dist Section A"
# We know there is just one tar.gz in $abld, that we just created
ls -1 ./*tar.gz
chkres $? "FAIL Section A  ls ./*tar.gz"
safemv ./*.tar.gz $arelgz "FAIL Section A moving gz"
ls -l $arelgz
tar -zxf $arelgz
chkres $? "FAIL A B2tar tar -zxf $arelgz"
safemv  libdwarf-$v $blibsrc "FAIL moving libdwarf srcdir"
echo "  End Section A  $bart"
################ End Section A
################ Start Section B
echo "TEST Section B: now cd $binstrelbld for second build install"
safecd $binstrelbld "FAIL B cd"
echo "TEST: now second install install, prefix $binstrelp"
echo "TEST: Expecting src in $blibsrc"
$blibsrc/configure $configureopt --enable-dwarfgen --enable-dwarfexample --prefix=$binstrelp
chkres $? "FAIL configure fail in Section B"
echo "TEST: In $binstrelbld make install from $blibsrc/configure"
make
chkres $? "FAIL make fail in Section B"
make doc
chkres $? "FAIL make doc fail in Section B"
make install
chkres $? "FAIL Section B install fail"
showinstalled $binstrelp "config, secondary install" $btfoutb
echo "TEST: Now lets see if make check works"
make check
chkres $? "FAIL make check in Section B"
make dist
chkres $? "FAIL make dist  Section B"
# We know there is just one tar.gz in $abld, that we just created
ls -1 ./*tar.gz
safemv ./*.tar.gz $brelgz "FAIL Section B moving gz"
ls -l $arelgz
ls -l $brelgz
# gzip does not build diffs quite identically to the byte.
# Lots of diffs, So we do tar tf to get the file name list.
echo "Now tar -tf on $arelgz and $brelgz "
# Sort as freebsd64 manages a distinct order at times.
tar -tf $arelgz | sort > $atfout
tar -tf $brelgz | sort > $btfout
echo "=========================diff make dist content========="
echo "Now diff the tar tf from $arelgz and $brelgz"
diff $atfout $btfout
chkres $? "FAIL second gen tar gz file list does not match first gen"
echo "  End Section B  $bart"
################ End section B

################ Start section C
echo "TEST Section C: now cd $dbigend for big-endian build (not runnable) "

safecd $dbigend "FAIL C be1 "
echo "TEST: now second install install, prefix $crelbld"
echo "TEST: Expecting src in $blibsrc"
echo "TEST: $blibsrc/configure $genopta --enable-wall --enable-dwarfexample --prefix=$crelbld"
$blibsrc/configure $configureopt $genopta  --enable-dwarfexample --prefix=$cinstrelp
chkres $? "FAIL be2  configure fail"
echo "#define WORDS_BIGENDIAN 1" >> config.h
echo "TEST: Compile In $dbigend make from $blibsrc/configure"
make
chkres $? "FAIL be3  make: Build failed"
make doc
chkres $? "FAIL be3  make doc: failed"
echo "  End Section C  $bart"
################ End section C

################ Start section D
safecd $crelbld "FAIL section D cd "
echo "TEST: Now configure from source dir $blibsrc/ in build dir $crelbld"
$blibsrc/configure $configureopt --enable-dwarfexample $genopta
chkres $? "FAIL C9  $blibsrc/configure"
make
chkres $? "FAIL C9  $blibsrc/configure  make"
make doc
chkres $? "FAIL C9  $blibsrc/configure  make doc"
echo "  End Section D  $bart"
################### End Section D
################### Cmake test E
safecd $ecmakebld "FAIL C10 Section E cd"
havecmake=n
which cmake >/dev/null
if [ $? -eq 0 ]
then
  havecmake=y
  echo "We have cmake and can test it."
fi
if [ $havecmake = "y" ]
then
  echo "TEST E: Now cmake from source dir $blibsrc/ in build dir  $ecmakebld"
  cmake -G Ninja $cmakeopt $genoptb \
       -DBUILD_NON_SHARED=ON \
       -DBUILD_DWARFEXAMPLE=ON\
       -DDO_TESTING=ON $blibsrc
  chkres $? "FAIL C10b  cmake in $ecmakdbld"
  ninja
  chkres $? "FAIL C10c  cmake make in $ecmakebld"
  ninja test
  #chkres $? "FAIL C10d  cmake make test in $ecmakebld"
  #ctest -R self
  chkres $? "FAIL C10e  ctest -R self in $ecmakebld"
else
  echo "cmake not installed so Test section E not tested."
fi
echo " End Section E  $bart (ls output follows)"
ls  $bart
############ End Section E
################### Cmake test F
safecd $fcmakebld "FAIL C11 Section F cd"
havecmake=n
which cmake >/dev/null
if [ $? -eq 0 ]
then
  havecmake=y
  echo "We have cmake and can test it: test F."
fi
if [ x$havecmake = "xy" ]
then
  echo "TEST F: Now cmake from source dir $blibsrc/ in build dir  $fcmakebld"
  # We are doing -DBUILD_DWARFGEN=ON as a sanity check.
  # Building lidwarfp and dwarfgen.
  # You should not be building or installing dwarfgen
  # or libdwarfp, it is unlikely you have a use
  # for lidwarfp and dwarfgen.
  cmake -G "Unix Makefiles" $cmakeopt $genoptb \
    -DCMAKE_INSTALL_PREFIX=$fcmakeinst \
    -DWALL=ON \
    -DBUILD_DWARFEXAMPLE=ON \
    -DDO_TESTING=ON $blibsrc
  chkres $? "FAIL Sec F C11b  cmake in $ecmakdbld"
  make
  chkres $? "FAIL Sec F C11c  cmake make in $fcmakebld"
  make test
  chkres $? "FAIL Sec F C11d  cmake make test in $fcmakebld"
  make install
  chkres $? "FAIL Sec F C11d  cmake install in $fcmakebld"
  showinstalled $fcmakeinst "using cmake" $ftfout
  ctest -R self
  chkres $? "FAIL Sec F C11e  ctest -R self in $fcmakebld"
else
  echo "cmake not installed (sec. F) not tested."
fi
echo " End Section F  $bart (ls output follows)"
ls  $bart
############ End Section F
################### Cmake test G
safecd $gcmakebld "FAIL C11 Section G cd"
havecmake=n
which cmake >/dev/null
if [ $? -eq 0 ]
then
  havecmake=y
  echo "We have cmake and can test it."
fi
if [ $havecmake = "y" ]
then
  echo "TEST: Now cmake sharedlib from source dir $blibsrc/ in build dir  $gcmakebld"
  echo " lidwarfp expects to see hidden symbols. "
  cmake -G "Unix Makefiles" $cmakeopt $genoptb  \
    -DDO_TESTING=ON \
    -DBUILD_DWARFEXAMPLE=ON $blibsrc
  chkres $? "FAIL Sec F C11b  cmake in $gcmakdbld"
  make
  chkres $? "FAIL Sec F C11d cmake  make in $gcmakebld"
  LD_LIBRARY_PATH="$gcmakebld/src/lib/libdwarf:$LD_LIBRARY_PATH" ctest -R self
  chkres $? "FAIL Sec F C11e  ctest -R self in $gcmakebld"
else
  echo "cmake not installed so Section G not tested."
fi
echo " End Section G  $bart (ls output follows)"
ls  $bart
############ End Section G

################### Cmake test H
safecd $hcmakebld "FAIL C12 Section H cd"
havecmake=n
which cmake >/dev/null
if [ $? -eq 0 ]
then
  havecmake=y
  echo "We have cmake and can test it."
else
  echo "We do NOT have cmake, cannot test it."
fi
if [ $havecmake = "y" ]
then
  echo "TEST: Now cmake from source dir $blibsrc/ in build dir  $gcmakebld"
  cmake -G "Unix Makefiles" $cmakeopt \
    -DDO_TESTING=ON  \
    -DBUILD_DWARFEXAMPLE=ON $blibsrc
  chkres $? "FAIL Sec H C12b  cmake in $hcmakdbld"
  make
  chkres $? "FAIL Sec H C12d  cmake make test in $hcmakebld"
  ctest -R self
  chkres $? "FAIL Sec H C12e  ctest -R self in $hcmakebld"
else
  echo "cmake not installed so Section H not tested."
fi
echo " End Section H  $bart (ls output follows)"
ls  $bart
############ End Section H

################### Cmake test I
safecd $imesonbld "FAIL C13 Section I cd"
havemeson=n
haveninja=n
which meson >/dev/null
if [ $? -eq 0 ]
then
  havemeson=y
  echo "We have meson and can test it."
  which ninja >/dev/null
  if [ $? -eq 0 ]
  then
    haveninja=y
  else
    echo "We do NOT have ninja, cannot test it."
  fi
else
  echo "We do NOT have meson, cannot test it or ninja."
fi
if [ $havemeson = "y" ]
then
  echo "TEST: Now meson from source dir $blibsrc/ in build dir $imesonbld"
  meson $wd $mesonopt --prefix=$imesonbld-dist
  if [ $haveninja = "y" ]
  then
    ninja -j8 install
    chkres $? " FAIL C13 ninja -j8 install"
    showinstalled $imesonbld-dist "using meson" $itfout
    ninja test
    chkres $? " FAIL C13 ninja test"
  else
    echo "Skipping ninja, it is not installed"
  fi
else
  echo "meson not installed so Section I not tested."
fi
echo " End Section I  $bart (ls output follows)"
ls  $bart
############ End Section I

ndsecs=`date '+%s'`
showminutes() {
   t=`expr  \( $2 \- $1 \+ 29  \) \/ 60`
   echo "Run time in minutes: $t"
}
showminutes $stsecs $ndsecs

echo "PASS scripts/buildandreleasetest.sh"
if [ "$savebart" = "n" ]
then
  rm -rf $bart
fi
exit 0
