rm version.txt
rm macosx_fteqw*
rm *.zip

svn update

svn info >> version.txt

REV=`cat version.txt | grep 'Last Changed Rev:' | sed s/'Last Changed Rev: '//`
DATE=`date +%B-%d-%Y`

make clean

make FTE_TARGET=macosx gl-rel mingl-rel sv-rel

cd release

mv macosx_fteqw.mingl ../macosx_fteqw.mingl_x86
mv macosx_fteqw.gl ../macosx_fteqw.gl_x86
mv macosx_fteqw.sv ../macosx_fteqw.sv_x86

cd ..

make clean

mv Makefile Makefile.backup
mv GoodMakefile Makefile

make FTE_TARGET=macosx gl-rel mingl-rel sv-rel CFLAGS="-arch ppc -L/Users/pcwiz/ppc-lib/lib/ -I/Users/pcwiz/ppc-lib/include/"

mv Makefile GoodMakefile
mv Makefile.backup Makefile

cd release

mv macosx_fteqw.mingl ../macosx_fteqw.mingl_ppc
mv macosx_fteqw.gl ../macosx_fteqw.gl_ppc
mv macosx_fteqw.sv ../macosx_fteqw.sv_ppc

cd ..

lipo -create macosx_fteqw.gl_x86 macosx_fteqw.gl_ppc -output macosx_fteqw.gl
lipo -create macosx_fteqw.mingl_x86 macosx_fteqw.mingl_ppc -output macosx_fteqw.mingl
lipo -create macosx_fteqw.sv_x86 macosx_fteqw.sv_ppc -output macosx_fteqw.sv

zip -9 -v "macosx_Leopard_10.5_-fteqwgl(r$REV $DATE).zip" macosx_fteqw.gl version.txt LICENSE
zip -9 -v "macosx_Leopard_10.5_-fteqwmingl(r$REV $DATE).zip" macosx_fteqw.mingl version.txt LICENSE
zip -9 -v "macosx_Leopard_10.5_-fteqwsv(r$REV $DATE).zip" macosx_fteqw.sv version.txt LICENSE
