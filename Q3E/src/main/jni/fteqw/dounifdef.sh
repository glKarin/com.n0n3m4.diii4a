#!/bin/sh
#this script is DANGEROUS
#be sure to have committed *BEFORE* running this script.

#Note: This script does not understand dead files (including botlib).
#expect '-Wmisleading-indentation' warnings (that were previously muted by nearby ifdefs).
#DO NOT COMMIT THE RESULTS TO FTE'S TRUNK

CONFIG=wastes

#must have trailing slashes
SRCDIR=./
NEWDIR=/tmp/fte-$CONFIG/

echo "WARNING: This script will lock-in a build config upon your C files."
echo "The resulting files will support only your choice of feature set, instead of having lots of unused code mixed in."
echo "THIS IS DESTRUCTIVE SO MUST ONLY BE USED FOR FORKS."
read -p "Press name the build config (or ctrl+c to abort)" CONFIG

if [ "$foo" == "" ]; then
	echo "no config specified."
	exit 1
fi

mkdir -p $NEWDIR
cat $SRCDIR/engine/common/config_$CONFIG.h | grep "#define" | sed "s/\/\/#define/#undef/g" > $NEWDIR/unifdefrules
cat $SRCDIR/engine/common/config_$CONFIG.h | grep "#undef" >> $NEWDIR/unifdefrules

if [ "$SRCDIR" != "$NEWDIR" ]; then
	echo "Copying files to strip to $NEWDIR."
	cp -r $SRCDIR* $NEWDIR
else
	echo "WARNING: WRITING FILES IN PLACE MUST ONLY  BE USED FOR FORKS."
	read -p "Press y<enter> to confirm (or ctrl+c to abort)" foo
	if [ "$foo" != "y" ]; then
		exit 1
	fi
fi
cd $NEWDIR

for FILENAME in engine/*/*.c; do
	unifdef -f unifdefrules -m $FILENAME
done

#headers keep any defines that will be expanded in code.
cat $NEWDIR/unifdefrules | grep -v FULLENGINENAME | grep -v DISTRIBUTION | grep -v ENGINEWEBSITE | grep -v MAX_SPLITS | grep GAME_SHORTNAME > $NEWDIR/unifdefhrules

for FILENAME in engine/*/*.h; do
	unifdef -f unifdefhrules -m $FILENAME
done

rm $NEWDIR/unifdefrules

echo "Files in $NEWDIR have now been stripped down."
echo "Some things may require hand-editing to remove warnings (or just compile with CFLAGS=-Wno-misleading-indentation)."
echo "You still need to set FTE_CONFIG too."
read -p "Press enter to test-compile" foo

cd $NEWDIR/engine && make sv-rel m-rel -j8 FTE_CONFIG=$CONFIG -k
