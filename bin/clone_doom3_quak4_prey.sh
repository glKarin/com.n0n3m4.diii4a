#!/bin/bash

echo "Init repository......";
git init;
git remote add -t master origin https://github.com/glKarin/com.n0n3m4.diii4a;
git config core.sparsecheckout true;

echo "doom3" > .git/info/sparse-checkout;
echo "build_win_x64_doom3_quak4_prey.bat" >> .git/info/sparse-checkout;
echo "build_win_x86_doom3_quak4_prey.bat" >> .git/info/sparse-checkout;
echo "cmake_linux_build_doom3_quak4_prey.sh" >> .git/info/sparse-checkout;
echo "cmake_msvc_build_doom3_quak4_prey.bat" >> .git/info/sparse-checkout;

echo "Pull project......";
git pull origin master --depth=1;

echo "done";
