#!/bin/bash
./ccache-alias.sh
START=$(date +%s)
HOME=/home/moodles
WEBFOLDER=/htdocs/unstable
LOGFOLDER=/htdocs/unstable/build_logs
BUILDFOLDER=$HOME$WEBFOLDER
BUILDLOGFOLDER=$HOME$LOGFOLDER
SVNFOLDER=$HOME/wip/wip/engine/release
THREADS="-j 4"
PATH=$PATH:/opt/mac/bin:/usr/local/bin:/opt/llvm/bin:/opt/intel/Compiler/11.0/083/bin/ia32/:/opt/morphos-dev/bin
TIMETAKENTF=/tmp/timetaken.txt
if [ -f $TIMETAKENTF ];
then
	rm $TIMETAKENTF
fi

cd $BUILDLOGFOLDER/
echo "Deleting old build logs"
rm *.txt

cd $BUILDFOLDER/
echo "Deleteing old binaries"
rm * >> /dev/null 2>> /dev/null
rm -rf ./win32/
rm -rf ./linux_32bit/
rm -rf ./linux_64bit/
rm -rf ./morphos/
rm -rf ./macosx_tiger_10.4/

echo "Making folders"
mkdir win32
mkdir linux_32bit
mkdir linux_64bit
mkdir morphos
mkdir macosx_tiger_10.4

cd /home/moodles/wip/wip/engine

echo "SVN Update"
svn update

make clean >> /dev/null
echo "Making Linux 32bit (llvm)"
make $THREADS FTE_TARGET=linux32 CC="llvm-gcc -m32" >> $BUILDLOGFOLDER/linux32_llvm.txt 2>> $BUILDLOGFOLDER/linux32_llvm.txt
cd $SVNFOLDER
ls -d *fteqw* | sed 's/\(.*\)32$/mv "&" "\132.llvm"/' | sh
mv *fteqw* $BUILDFOLDER/linux_32bit/
cd ..
echo "Nuking llvm object files"
make clean >> /dev/null
echo "Making Linux 32bit (clang)"
make $THREADS FTE_TARGET=linux32 CC="clang -DCLANG" >> $BUILDLOGFOLDER/linux32_clang.txt 2>> $BUILDLOGFOLDER/linux32_clang.txt
cd $SVNFOLDER
ls -d *fteqw* | sed 's/\(.*\)32$/mv "&" "\132.clang"/' | sh
mv *fteqw* $BUILDFOLDER/linux_32bit/
cd ..
echo "Nuking clang object files"
make clean >> /dev/null
echo "Making Linux 32bit (icc)"
make $THREADS FTE_TARGET=linux32 CC="icc" >> $BUILDLOGFOLDER/linux32_icc.txt 2>> $BUILDLOGFOLDER/linux32_icc.txt
cd $SVNFOLDER
ls -d *fteqw* | sed 's/\(.*\)32$/mv "&" "\132.icc"/' | sh
mv *fteqw* $BUILDFOLDER/linux_32bit/
cd ..
echo "Nuking icc object files"
make clean >> /dev/null
echo "Making Linux 32bit SDL (llvm)"
make $THREADS FTE_TARGET=sdl BITS=32 CC="llvm-gcc -m32" >> $BUILDLOGFOLDER/linux32_SDL_llvm.txt 2>> $BUILDLOGFOLDER/linux32_SDL_llvm.txt
cd $SVNFOLDER
ls -d *fteqw* | sed 's/\(.*\)32$/mv "&" "\132.llvm"/' | sh
mv *fteqw* $BUILDFOLDER/linux_32bit/
cd ..
echo "Nuking 32bit SDL llvm object files"
make clean >> /dev/null
echo "Making Linux 32bit SDL (clang)"
make $THREADS FTE_TARGET=sdl BITS=32 CC="clang -m32 -DCLANG" >> $BUILDLOGFOLDER/linux32_SDL_clang.txt 2>> $BUILDLOGFOLDER/linux32_SDL_clang.txt
cd $SVNFOLDER
ls -d *fteqw* | sed 's/\(.*\)32$/mv "&" "\132.clang"/' | sh
mv *fteqw* $BUILDFOLDER/linux_32bit/
cd ..
echo "Nuking 32bit SDL clang object files"
make clean >> /dev/null
echo "Making Linux 32bit SDL (icc)"
make $THREADS FTE_TARGET=sdl BITS=32 CC="icc -m32" >> $BUILDLOGFOLDER/linux32_SDL_icc.txt 2>> $BUILDLOGFOLDER/linux32_SDL_icc.txt
cd $SVNFOLDER
ls -d *fteqw* | sed 's/\(.*\)32$/mv "&" "\132.icc"/' | sh
mv *fteqw* $BUILDFOLDER/linux_32bit/
cd ..
echo "Nuking 32bit SDL icc object files"
make clean >> /dev/null
echo "Making Linux 32bit (gcc)"
make $THREADS FTE_TARGET=linux32 >> $BUILDLOGFOLDER/linux32.txt 2>> $BUILDLOGFOLDER/linux32.txt
cp $SVNFOLDER/* $BUILDFOLDER/linux_32bit/ >> /dev/null 2>> /dev/null
make clean >> /dev/null
echo "Making Windows\n"
make $THREADS FTE_TARGET=win32 sv-rel gl-rel mingl-rel >> $BUILDLOGFOLDER/win32.txt 2>> $BUILDLOGFOLDER/win32.txt
cp $SVNFOLDER/* $BUILDFOLDER/win32/ >> /dev/null 2>> /dev/null
make clean >> /dev/null
echo "Making MorphOS\n"
make $THREADS FTE_TARGET=morphos gl-rel mingl-rel  >> $BUILDLOGFOLDER/morphos.txt 2>> $BUILDLOGFOLDER/morphos.txt
cp $SVNFOLDER/* $BUILDFOLDER/morphos/ >> /dev/null 2>> /dev/null
chmod ugo+x $BUILDFOLDER/morphos/*
make clean >> /dev/null
echo "Making MacOSX"
make $THREADS FTE_TARGET=macosx sv-rel gl-rel mingl-rel CFLAGS="-I/home/moodles/mac/include/ -L/home/moodles/mac/lib"  >> $BUILDLOGFOLDER/osx_ppc.txt 2>> $BUILDLOGFOLDER/osx_ppc.txt
make $THREADS FTE_TARGET=macosx_x86 sv-rel gl-rel mingl-rel CFLAGS="-I/home/moodles/mac/x86/include/ -L/home/moodles/mac/x86/lib"  >> $BUILDLOGFOLDER/osx_86.txt 2>> $BUILDLOGFOLDER/osx_86.txt
cp $SVNFOLDER/* $BUILDFOLDER/macosx_tiger_10.4/ >> /dev/null 2>> /dev/null
make clean >> /dev/null
echo "Making Windows SDL"
make $THREADS FTE_TARGET=win32_SDL CFLAGS="-D_SDL" >> $BUILDLOGFOLDER/win32_SDL.txt 2>> $BUILDLOGFOLDER/win32_SDL.txt
cp $SVNFOLDER/* $BUILDFOLDER/win32/ >> /dev/null 2>> /dev/null
make clean >> /dev/null
echo "Making Linux 32bit SDL"
make $THREADS FTE_TARGET=SDL BITS=32 >> $BUILDLOGFOLDER/linux32_SDL.txt 2>> $BUILDLOGFOLDER/linux32_SDL.txt
cp $SVNFOLDER/* $BUILDFOLDER/linux_32bit/ >> /dev/null 2>> /dev/null
make clean >> /dev/null
echo "Making Linux 64bit"
make $THREADS FTE_TARGET=linux64 LDFLAGS="-L./libs/64/ -I./libs/64/ -lz -lX11-xcb -lxcb-xlib -lxcb -lXdmcp -lXpm -lXau -lX11 -lXext" >> $BUILDLOGFOLDER/linux64.txt 2>> $BUILDLOGFOLDER/linux64.txt
cp $SVNFOLDER/* $BUILDFOLDER/linux_64bit/ >> /dev/null 2>> /dev/null
make clean >> /dev/null
echo "Making Linux 64bit (clang)"
make $THREADS FTE_TARGET=linux64 CC="clang -m64 -DCLANG" LDFLAGS="-L./libs/64/ -I./libs/64/ -lz -lX11-xcb -lxcb-xlib -lxcb -lXdmcp -lXpm -lXau -lX11 -lXext" >> $BUILDLOGFOLDER/linux64_clang.txt 2>> $BUILDLOGFOLDER/linux64_clang.txt
cd $SVNFOLDER
ls -d *fteqw* | sed 's/\(.*\)64$/mv "&" "\164.clang"/' | sh
mv *fteqw* $BUILDFOLDER/linux_64bit/
cd ..
make clean >> /dev/null
echo "Making Linux 64bit SDL (very ambitious)"
make $THREADS FTE_TARGET=SDL gl-rel CC="gcc -m64" LDFLAGS="-L./libs/64/ -I./libs/64/ -lz -lX11-xcb -lxcb-xlib -lxcb -lXdmcp -lXpm -lXau -lX11 -lXext" >> $BUILDLOGFOLDER/linux64_SDL.txt 2>> $BUILDLOGFOLDER/linux64_SDL.txt

cd /home/moodles/wip/wip/engine
svn info >> $BUILDFOLDER/version.txt
echo "All done"

END=$(date +%s)
DIFF=$(( $END - $START ))
MINS=$(( $DIFF / 60 ))
echo "(Total Compile Time: $MINS minutes)" > $TIMETAKENTF
echo "Total Compile Time: $MINS minutes"

cd /home/moodles
rm .bitchxrc
cp ./wip/.bitchxrc ./
./BitchX -a gameservers.nj.us.quakenet.org -A -c "#fte" -n A_WIP_Gorilla
