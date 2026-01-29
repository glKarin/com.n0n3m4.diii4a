#!/bin/bash
START=$(date +%s)
HOME=/home/moodles
WEBFOLDER=/htdocs/fteqtv
LOGFOLDER=/htdocs/build_logs/fteqtv
BUILDFOLDER=$HOME$WEBFOLDER
BUILDLOGFOLDER=$HOME$LOGFOLDER
THREADS="-j 4"
PATH=$PATH:/opt/mac/bin:/usr/local/bin:/opt/llvm/bin:/opt/intel/Compiler/11.0/083/bin/ia32/:/opt/morphos-dev/bin

rm /home/moodles/htdocs/build_logs/fteqtv/*
cd /home/moodles/fteqw/fteqw/trunk/fteqtv/
rm -r release
mkdir release

svn update
svn info >> version.txt
cp version.txt $BUILDFOLDER/
cp LICENSE $BUILDFOLDER/

cd ..
rm -rf export
svn export fteqtv export
cp $BUILDFOLDER/version.txt ./export/version.txt
cd export
tar -cjf fteqtv_sourcecode.tar.bz2 *
mv *.bz2 $BUILDFOLDER/

cd /home/moodles/fteqw/fteqw/trunk/fteqtv
make clean
make $THREADS >> $BUILDLOGFOLDER/linux32-fteqtv.txt 2>> $BUILDLOGFOLDER/linux32-fteqtv.txt
mv qtv $BUILDFOLDER/linux32-qtv

make clean
make $THREADS CC="gcc -m64" >> $BUILDLOGFOLDER/linux64-fteqtv.txt 2>> $BUILDLOGFOLDER/linux64-fteqtv.txt
mv qtv $BUILDFOLDER/linux64-qtv

make clean
make $THREADS CC="llvm-gcc" >> $BUILDLOGFOLDER/linux32-fteqtv_llvm.txt 2>> $BUILDLOGFOLDER/linux32-fteqtv_llvm.txt
mv qtv $BUILDFOLDER/linux32-qtv.llvm

make clean
make $THREADS CC="clang" >> $BUILDLOGFOLDER/linux32-fteqtv_clang.txt 2>> $BUILDLOGFOLDER/linux32-fteqtv_clang.txt
mv qtv $BUILDFOLDER/linux32-qtv.clang

make clean
make $THREADS CC="clang -m64" >> $BUILDLOGFOLDER/linux64-fteqtv_clang.txt 2>> $BUILDLOGFOLDER/linux64-fteqtv_clang.txt
mv qtv $BUILDFOLDER/linux64-qtv.clang

make clean
make $THREADS CC="icc" >> $BUILDLOGFOLDER/linux32-fteqtv_icc.txt 2>> $BUILDLOGFOLDER/linux32-fteqtv_icc.txt
mv qtv $BUILDFOLDER/linux32-qtv.icc

make clean
make $THREADS TOOLPREFIX=ppc-morphos- >> $BUILDLOGFOLDER/morphos-fteqtv.txt 2>> $BUILDLOGFOLDER/morphos-fteqtv.txt
mv qtv $BUILDFOLDER/morphos-qtv
chmod ugo+x $BUILDFOLDER/morphos-qtv

make clean
make $THREADS TOOLPREFIX=i586-mingw32msvc- qtv.exe >> $BUILDLOGFOLDER/win32-fteqtv.txt 2>> $BUILDLOGFOLDER/win32-fteqtv.txt
mv qtv.exe $BUILDFOLDER/win32-qtv.exe

make clean
make $THREADS TOOLPREFIX=powerpc-apple-darwin8- STRIPFLAGS="" CFLAGS="-arch i686 -arch ppc" >> $BUILDLOGFOLDER/macosx_tiger.txt 2>> $BUILDLOGFOLDER/macosx_tiger-fteqtv.txt
#powerpc-apple-darwin8-strip qtv.db -o qtv
mv qtv $BUILDFOLDER/macosx_tiger-qtv

END=$(date +%s)
DIFF=$(( $END - $START ))
echo "It took $DIFF seconds"

