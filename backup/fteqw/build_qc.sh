#!/bin/bash
#FTEQCC and FTEQW must be defined.
#QSS should be defined...

BUILDFOLDER=~/htdocs
BUILDLOGFOLDER=$BUILDFOLDER/build_logs


if [ "$FTEQCC" == "" ]; then
	QSS=~/htdocs/qss/quakespasm-spiked-linux64
	FTEQW=~/htdocs/linux_amd64/fteqw64
	FTEQCC=~/htdocs/linux_amd64/fteqcc64
fi

#this really should use the native cpu type... until then we use 32bit in case anyone's still using a 32bit kernel.
if [ "$FTEQW" != "" ]; then
    echo "--- QC builds ---"
	echo "Making fteextensions.qc"

	BASEDIR=~/.fte
	GAMEDIR=fte
	mkdir -p $BASEDIR/$GAMEDIR/src

	#generate fte's extensions
	CFG=$BASEDIR/fte/minusargsaresilly.cfg
	echo "pr_dumpplatform -o fteextensions" > $CFG
	echo "pr_dumpplatform -o csqcsysdefs -Tcs" >> $CFG
	echo "pr_dumpplatform -o menusysdefs -Tmenu" >> $CFG
	$FTEQW -basedir $BASEDIR -nohome -quake -game $GAMEDIR +set snd_device none -nosound +set vid_renderer sv +exec minusargsaresilly.cfg +quit >> /dev/null

	#if we have fteqcc available then try to generate some symbol lists for our generic defs
	if [ "$FTEQCC" != "" ]; then
		#get QSS to spit out its defs for completeness.
		if [ "$QSS" != "" ] && [ -e "$BASEDIR/id1/pak1.pak" ] && [ -e "$QSS" ]; then
			CFG=$BASEDIR/$GAMEDIR/gen.cfg
			echo "pr_dumpplatform -Oqscsextensions -Tcs" > $CFG
			echo "pr_dumpplatform -Oqsextensions -Tss" >> $CFG
			echo "pr_dumpplatform -Oqsmenuextensions -Tmenu" >> $CFG
			$QSS -dedicated -nohome -game $GAMEDIR -basedir $BASEDIR +exec gen.cfg +quit >>/dev/null
		else
			echo "QSS not available?"
		fi
		
		if [ ! -e "$BASEDIR/fte/src/dpsymbols.src" ]; then
			ln -sr quakec/dpsymbols.src $BASEDIR/$GAMEDIR/src/
		fi
		if [ ! -e "$BASEDIR/fte/src/dpdefs/" ]; then
			echo "no dpdefs subdir found in $BASEDIR/fte/src/ ... manual intervention required"
		fi

		#generate symbol tables from the various engines's defs
		(	cd $BASEDIR/$GAMEDIR/src
			$FTEQCC -Fdumpsymbols -oqsscs.dat        qscsextensions.qc   && sort -o qss_cs.sym   qsscs.dat.sym
			$FTEQCC -Fdumpsymbols -oqssss.dat        qsextensions.qc     && sort -o qss_ss.sym   qssss.dat.sym
			$FTEQCC -Fdumpsymbols -oqssmn.dat        qsmenuextensions.qc && sort -o qss_menu.sym qssmn.dat.sym

			$FTEQCC -Fdumpsymbols -oftecs.dat -DCSQC fteextensions.qc    && sort -o fte_cs.sym   ftecs.dat.sym
			$FTEQCC -Fdumpsymbols -oftess.dat -DSSQC fteextensions.qc    && sort -o fte_ss.sym   ftess.dat.sym
			$FTEQCC -Fdumpsymbols -oftemn.dat -DMENU fteextensions.qc    && sort -o fte_menu.sym ftemn.dat.sym

			$FTEQCC -Fdumpsymbols -odpcs.dat  -DCSQC dpsymbols.src       && sort -o dp_cs.sym    dpcs.dat.sym
			$FTEQCC -Fdumpsymbols -odpss.dat  -DSSQC dpsymbols.src       && sort -o dp_ss.sym    dpss.dat.sym
			$FTEQCC -Fdumpsymbols -odpmn.dat  -DMENU dpsymbols.src       && sort -o dp_menu.sym  dpmn.dat.sym
		) >>/dev/null

		#generate generic extensions
		CFG=$BASEDIR/fte/minusargsaresilly.cfg
		echo "pr_dumpplatform     -Ocsqc_api -Fdepfilter -Tsimplecs" > $CFG
		echo "pr_dumpplatform -Ofte_csqc_api -Fdepfilter -Tcs" >> $CFG
		echo "pr_dumpplatform  -Odp_csqc_api -Fdepfilter -Tdpcs" >> $CFG
		echo "pr_dumpplatform     -Ossqc_api -Fdepfilter -Tnq" >> $CFG
		echo "pr_dumpplatform     -Omenu_api -Fdepfilter -Tmenu" >> $CFG
		$FTEQW -basedir $BASEDIR -nohome -quake -game $GAMEDIR +set snd_device none -nosound +set vid_renderer sv +exec minusargsaresilly.cfg +quit >> /dev/null
	fi

	#fix up and copy the results somewhere useful
	rm $CFG

	mkdir -p $BUILDFOLDER/ftedefs $BUILDFOLDER/genericdefs
	mv $BASEDIR/$GAMEDIR/src/fteextensions.qc $BUILDFOLDER/ftedefs
	mv $BASEDIR/$GAMEDIR/src/csqcsysdefs.qc $BUILDFOLDER/ftedefs
	mv $BASEDIR/$GAMEDIR/src/menusysdefs.qc $BUILDFOLDER/ftedefs
	mv $BASEDIR/$GAMEDIR/src/*_api.qc $BUILDFOLDER/genericdefs
fi


if [ "$FTEQCC" != "" ]; then
	mkdir -p $BUILDFOLDER/csaddon/

	(	cd quakec/csaddon/src
		echo -n "Making csaddon... "
		$FTEQCC -srcfile csaddon.src > $BUILDLOGFOLDER/csaddon.txt 2>&1
		if [ $? -eq 0 ]; then
			echo "done"
			cp ../csaddon.dat $BUILDFOLDER/csaddon/
			cd ..
			zip -q9 $BUILDFOLDER/csaddon/csaddon.pk3 csaddon.dat
		else
			echo "failed"
		fi
	)

	(	cd quakec/menusys
		echo -n "Making menusys... "
		$FTEQCC -srcfile menu.src > $BUILDLOGFOLDER/menu.txt 2>&1
		if [ $? -eq 0 ]; then
			echo "done"
			zip -q -q9 -o -r $BUILDFOLDER/csaddon/menusys_src.zip .
			cp ../menu.dat $BUILDFOLDER/csaddon/
			cd ..
			zip -q9 $BUILDFOLDER/csaddon/menusys.pk3 menu.dat
		else
			echo "failed"
		fi
	)
else
	echo "Skiping csaddon + qcmenu, no compiler build"
fi

