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

RPI4="-DRPI4=TRUE"
CMAKE_ADD="-DCMAKE_C_FLAGS=-m32 -DCMAKE_CXX_FLAGS=-m32 -DUSE_I386_NASM_ASM=TRUE"
# Right now we force x86, though...
# for old x86 distros use:
# CMAKE_ADD="-DCMAKE_C_FLAGS=-mmmx -DCMAKE_CXX_FLAGS=-mmmx -DUSE_I386_NASM_ASM=TRUE"

for var in "$1" "$2" "$3" "$4" "$5" "$6" "$7" "$8" "$9"; do 
if [ "$var" == "$RPI4" ]; then
  CMAKE_ADD="-DUSE_I386_NASM_ASM=FALSE"
fi
done;

echo $CMAKE_ADD
cmake -DCMAKE_BUILD_TYPE=Release $CMAKE_ADD ..  $1 $2 $3 $4 $5 $6 $7 $8 $9

echo "ECC first"
make ecc
echo "Then the rest..."
make -j$NCPU
make install
