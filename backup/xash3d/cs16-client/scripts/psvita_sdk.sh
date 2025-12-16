#!/bin/bash

cd $GITHUB_WORKSPACE

export VITASDK=/usr/local/vitasdk

echo "Downloading vitasdk..."
git clone https://github.com/vitasdk/vdpm.git --depth=1 || exit 1
pushd vdpm
./bootstrap-vitasdk.sh || exit 1
./vdpm taihen || exit 1
./vdpm kubridge || exit 1
./vdpm zlib || exit 1
./vdpm SceShaccCgExt || exit 1
./vdpm vitaShaRK || exit 1
./vdpm libmathneon || exit 1
popd

echo "Building vrtld..."
git clone https://github.com/fgsfdsfgs/vita-rtld.git --depth=1 || exit 1
pushd vita-rtld || die
cmake -S. -Bbuild -DCMAKE_BUILD_TYPE=Release || die_configure
cmake --build build -- -j$JOBS || die
cmake --install build || die
popd
