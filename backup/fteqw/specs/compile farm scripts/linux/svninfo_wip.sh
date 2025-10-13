TEMP1=/tmp/moodlesjunk1.txt
TEMP2=/tmp/moodlesjunk2.txt
TIMETAKENTF=/tmp/timetaken.txt
if [ -f $TEMP1 ];
then
	rm $TEMP1
fi
if [ -f $TEMP2 ];
then
	rm $TEMP2
fi

cd /home/moodles/wip/wip/engine
svn info > svninfo.txt
echo "4Unstable WIP SVN branch updated by:" >> $TEMP1
AUTHOR=`cat svninfo.txt | grep 'Last Changed Author' | sed s/'Last Changed Author: '//`
echo $AUTHOR >> $TEMP1
if [ $AUTHOR == "acceptthis" ];
then
	echo "(AKA Spike)" >> $TEMP1
elif [ $AUTHOR == "isthisinuse" ];
then
	echo "(AKA bigfoot)" >> $TEMP1
fi
echo "*" >> $TEMP1
cat svninfo.txt | grep 'Revision' | sed s/'Revision'/'Revision'/ >> $TEMP1
echo "(Build #"`~/buildnumber.sh`")" >> $TEMP1
echo "* New binaries @ http://www.triptohell.info/moodles/unstable/" >> $TEMP1
cat /tmp/timetaken.txt >> $TEMP1
tr "\n" " " <$TEMP1 >$TEMP2
echo -e '\n' >> $TEMP2
cat $TEMP2
