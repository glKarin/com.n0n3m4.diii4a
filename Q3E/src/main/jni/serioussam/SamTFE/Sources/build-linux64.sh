#!/bin/bash

NCPU=`cat /proc/cpuinfo |grep vendor_id |wc -l`

if [[ $NCPU -eq 0 ]]
then
  let NCPU='4'
else
  let NCPU=$NCPU
fi

echo "Will build with 'make -j$NCPU' ... please edit this script if incorrect."

set -e
set -x

rm -rf cmake-build

mkdir $_
cd $_

# This is the eventual path for amd64.
cmake -DCMAKE_BUILD_TYPE=Release ..  $1 $2 $3 $4 $5 $6 $7 $8 $9

echo "ECC first"
make ecc
echo "Then the rest..."
make -j$NCPU
make install
