#!/bin/sh
# This is meant to be done by hand
# when changes made. Not done during build or install.
#   see doc/Makefile.am
# mips_extensions.mm is a historical document
# and should never change, so we do not build or
# install mips_extensions.pdf

here=`pwd`

#if [ ! "$(top_srcdir)/doc" = "$here" ]
#then
#  echo "Run doc/make doc only in $(top_srcdir)/doc"
#  echo "Give up as we are in $here"
#  exit 1
#fi
c="n"
p="n"
if [ $# -lt 1 ]
then
  echo "Usage: pdfbld.sh [-c] [-p]"
  echo "where: -c formats libdwarf.pdf"
  echo "where: -p formats libdwarfp.pdf"
  echo "where: Run it only in the doc source directory"
  exit 1
fi
for i in $*
do
  case $i in
    -c) c="y"
       echo "Build libdwarf consumer pdf for $src"
       shift ;;
    -p) p="y"
       fin=libdwarfp.mm
       fout=libdwarfp.pdf
       echo "Build libdwarf consumer pdf for $src"
       shift ;;
    *)  echo "Giving up: unknown argument use argument -c or -p"
       exit 1 ;;
  esac
done

ckres() {
  if [ $1 -eq 0 ]
  then
    return 0
  fi
  echo "FAIL $2"
  exit 1
}

if [ $c = "y" ]
then
  if [ -d latex -o -d latex ]
  then
    rm -rf latex
    ckres $? "rm existing latex directory FAIL"
  fi
  doxygen
  ckres $? "doc doxygen FAIL "
  cd latex
  ckres $? "doc cd to latex dir FAIL "
  make
  ckres $? "doc make latex fail"
  cd ..
  ckres $? "doc cd .. FAIL "
  cp latex/refman.pdf libdwarf.pdf
  ckres $? "doc copy latex/refman.pdf libdwarf.pdf FAIL"
fi

if [ ! $p = "y" ]
then
  exit 0
fi

src=$1
echo "Build pdf for $src"

set -x
TROFF=/usr/bin/groff
#TROFFDEV="-T ps"
PSTOPDF=/usr/bin/ps2pdf
rm -f $fout

pr -t -e $fin | tbl | $TROFF -n16 -mm >temp.ps
if [  $? -ne 0 ]
then
  echo "Building $fout FAILED in the pr/tbl/$TROFF step"
  exit 1
fi
$PSTOPDF temp.ps $fout
if [  $? -ne 0 ]
then
  echo "Building $fout FAILED in the $PSTOPDF step"
  exit 1
fi

set +x
rm -f temp.ps
exit 0
