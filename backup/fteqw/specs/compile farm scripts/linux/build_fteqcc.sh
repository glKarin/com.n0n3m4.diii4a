#!/bin/bash
START=$(date +%s)
HOME=/home/moodles
WEBFOLDER=/htdocs/fteqcc
LOGFOLDER=/htdocs/build_logs/fteqcc
BUILDFOLDER=$HOME$WEBFOLDER
BUILDLOGFOLDER=$HOME$LOGFOLDER
THREADS="-j 4"
PATH=$PATH:/opt/mac/bin:/usr/local/bin:/opt/llvm/bin:/opt/intel/Compiler/11.0/083/bin/ia32/:/opt/morphos-dev/bin

cd /home/moodles/fteqw/engine/qclib
rm version.txt
rm -rf /home/moodles/fteqw/fteqw/trunk/engine/export
rm $BUILDFOLDER/*
rm /home/moodles/htdocs/build_logs/fteqcc/*

svn update
svn info >> version.txt
cd ..

svn export qclib export
cp ./qclib/version.txt ./export/version.txt
cd export
tar -cjf fteqcc_sourcecode.tar.bz2 *
mv *.bz2 $BUILDFOLDER/
cd ..
rm -rf export

cd qclib

svn info >> $BUILDFOLDER/version.txt

make clean
make $THREADS win CC=i586-mingw32msvc-gcc >> $BUILDLOGFOLDER/win32-fteqccgui.txt 2>> $BUILDLOGFOLDER/win32-fteqccgui.txt
mv fteqcc.exe $BUILDFOLDER/win32-fteqccgui.exe

make clean
make $THREADS win CC="x86_64-w64-mingw32-gcc -m64" >> $BUILDLOGFOLDER/win64-fteqccgui.txt 2>> $BUILDLOGFOLDER/win64-fteqccgui.txt
mv fteqcc.exe $BUILDFOLDER/win64-fteqccgui.exe

make clean
make $THREADS >> $BUILDLOGFOLDER/linux32-fteqcc.txt 2>> $BUILDLOGFOLDER/linux32-fteqcc.txt
mv fteqcc.bin $BUILDFOLDER/linux32-fteqcc

make clean
make $THREADS CC="llvm-gcc -m32" >> $BUILDLOGFOLDER/linux32-fteqcc_llvm.txt 2>> $BUILDLOGFOLDER/linux32-fteqcc_llvm.txt
mv fteqcc.bin $BUILDFOLDER/linux32-fteqcc.llvm

make clean
make $THREADS CC="clang -m32 -Dinline=" >> $BUILDLOGFOLDER/linux32-fteqcc_clang.txt 2>> $BUILDLOGFOLDER/linux32-fteqcc_clang.txt
mv fteqcc.bin $BUILDFOLDER/linux32-fteqcc.clang

make clean
make $THREADS CC="icc -m32" >> $BUILDLOGFOLDER/linux32-fteqcc_icc.txt 2>> $BUILDLOGFOLDER/linux32-fteqcc_icc.txt
mv fteqcc.bin $BUILDFOLDER/linux32-fteqcc.icc

make clean
make $THREADS CC="gcc -m64" >> $BUILDLOGFOLDER/linux64-fteqcc.txt 2>> $BUILDLOGFOLDER/linux64-fteqcc.txt
mv fteqcc.bin $BUILDFOLDER/linux64-fteqcc

make clean
make $THREADS CC="clang -m64 -Dinline=" >> $BUILDLOGFOLDER/linux64-fteqcc_clang.txt 2>> $BUILDLOGFOLDER/linux64-fteqcc_clang.txt
mv fteqcc.bin $BUILDFOLDER/linux64-fteqcc.clang

make clean
make $THREADS CC=i586-mingw32msvc-gcc >> $BUILDLOGFOLDER/win32-fteqcc.txt 2>> $BUILDLOGFOLDER/win32_fteqcc.txt
mv fteqcc.bin $BUILDFOLDER/win32-fteqcc.exe

make clean
make $THREADS win CC="x86_64-w64-mingw32-gcc -m64" >> $BUILDLOGFOLDER/win64-fteqcc.txt 2>> $BUILDLOGFOLDER/win64-fteqcc.txt
mv fteqcc.exe $BUILDFOLDER/win64-fteqcc.exe

make clean
make $THREADS CC="powerpc-apple-darwin8-gcc -arch ppc -arch i686" >> $BUILDLOGFOLDER/macosx_tiger-fteqcc.txt 2>> $BUILDLOGFOLDER/macosx_tiger-fteqcc.txt
mv fteqcc.bin $BUILDFOLDER/macosx_tiger-fteqcc

make clean
make $THREADS CC="ppc-morphos-gcc -noixemul" >> $BUILDLOGFOLDER/morphos-fteqcc.txt 2>> $BUILDLOGFOLDER/morphos-fteqcc.txt
mv fteqcc.bin $BUILDFOLDER/morphos-fteqcc
chmod ugo+x $BUILDFOLDER/morphos-fteqcc

END=$(date +%s)
DIFF=$(( $END - $START ))
echo "It took $DIFF seconds"
