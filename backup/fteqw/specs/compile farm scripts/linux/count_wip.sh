#!/bin/bash
TEMPFILE=/tmp/moodlescountjunk.txt
TEMPFILE2=/tmp/moodlescountjunk2.txt
AUTHOR=`cat /home/moodles/wip/wip/engine/svninfo.txt | grep 'Last Changed Author' | sed s/'Last Changed Author: '//`
if [ -f $TEMPFILE ];
then
	rm $TEMPFILE
fi
if [ -f $TEMPFILE2 ];
then
	rm $TEMPFILE2
fi

# count folder contents
LINUX32=$(ls -1 /home/moodles/htdocs/unstable/linux_32bit/ | wc -l)
LINUX64=$(ls -1 /home/moodles/htdocs/unstable/linux_64bit/ | wc -l)
WIN32=$(ls -1 /home/moodles/htdocs/unstable/win32/ | wc -l)
MORPHOS=$(ls -1 /home/moodles/htdocs/unstable/morphos/ | wc -l)
MACOSX=$(ls -1 /home/moodles/htdocs/unstable/macosx_tiger_10.4/ | wc -l)

# known count when all binaries build
LIN32TOTAL=24 # 4 compilers (GCC, ICC, CLANG & LLVM) * 3 targets (sv, mingl, gl) * 2 (SDL versions)
LIN64TOTAL=6 # 2 compilers (GCC, CLANG) * 3 targets (sv, mingl, gl)
WIN32TOTAL=6 # 1 compiler (MinGW32) * 3 targets (sv, mingl, gl) * 2 (SDL versions)
MORPHTOTAL=1 # 1 compiler (GCC based by bigfoot) * 2 targets (gl, mingl)
MACOSTOTAL=4 # 2 compilers (10.4 x86 and ppc) * 3 targets (gl, mingl, sv)

# subtract known directory count from realtime counted directory count to find total of failed builds
L32NUM=`expr $LIN32TOTAL - $LINUX32`
L64NUM=`expr $LIN64TOTAL - $LINUX64`
W32NUM=`expr $WIN32TOTAL - $WIN32`
MORNUM=`expr $MORPHTOTAL - $MORPHOS`
MACNUM=`expr $MACOSTOTAL - $MACOSX`

if [ $L32NUM -ne 0 ];
then
	echo "$L32NUM/$LIN32TOTAL Lin32 failed" >> $TEMPFILE 
fi
if [ $L64NUM -ne 0 ];
then
        echo " $L64NUM/$LIN64TOTAL Lin64 failed" >> $TEMPFILE
fi
if [ $W32NUM -ne 0 ];
then
        echo " $W32NUM/$WIN32TOTAL Win32 failed" >> $TEMPFILE
fi
if [ $MORNUM -ne 0 ];
then
        echo " $MORNUM/$MORPHTOTAL MorphOS failed" >> $TEMPFILE
fi
if [ $MACNUM -ne 0 ];
then
        echo " $MACNUM/$MACOSTOTAL MacOSX 10.4 failed" >> $TEMPFILE
fi
if [ $L32NUM -eq 0 ] && [ $L64NUM -eq 0 ] && [ $W32NUM -eq 0 ] && [ $MORNUM -eq 0 ] && [ $MACNUM -eq 0 ];
then
	echo "Everything built successfully" >> $TEMPFILE

elif [ $LIN32TOTAL -eq 0 ] && [ $LIN64TOTAL -eq 0 ] && [ $WIN32TOTAL -eq 0 ] && [ $MORPHTOTAL -eq 0 ] && [ $MACOSTOTAL -eq 0 ];
then
        echo " Nothing built at all!" >> $TEMPFILE
elif [ $L32NUM -ne 0 ] && [ $L64NUM -ne 0 ] && [ $W32NUM -ne 0 ] && [ $MORNUM -ne 0 ] && [ $MACNUM -ne 0 ];
then
        if [ $AUTHOR == "acceptthis" ];
        then
		echo " At least 1 target from each OS failed to build, on the bright side, at least everything didn't compile (Your typical average Spike commit, breaking Lunix as usual)" >> $TEMPFILE
	else
               echo " At least 1 target from each OS failed to build, on the bright side, at least everythinng didn't compile" >> $TEMPFILE
	fi	
else
	if [ $AUTHOR == "acceptthis" ];
	then
		echo " Everything else built successfully (Your typical average Spike commit, breaking Lunix as usual)" >> $TEMPFILE
	else
		echo " Everything else built successfully" >> $TEMPFILE
	fi
fi

tr "\n" "," <$TEMPFILE | sed "s/,$/\./" >$TEMPFILE2
echo -e '\n' >> $TEMPFILE2
cat $TEMPFILE2
