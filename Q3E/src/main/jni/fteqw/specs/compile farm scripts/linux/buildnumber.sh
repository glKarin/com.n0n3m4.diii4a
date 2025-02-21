#!/bin/bash
CURRENTYEAR=$(date +'%Y')
CURRENTDAYOFYEAR=$(date +'%j')

while [ $CURRENTYEAR != 1998 ]
do
	CURRENTYEAR=$(expr $CURRENTYEAR - 1)

	if [ `expr $CURRENTYEAR % 4` -ne 0 ]
	then
		echo hi > /dev/null
	elif [ `expr $CURRENTYEAR % 400` -eq 0 ]
	then
		CURRENTDAYOFYEAR=$(expr $CURRENTDAYOFYEAR + 1)
	elif [ `expr $CURRENTYEAR % 100` -eq 0 ]
	then
		echo hi > /dev/null
	else
		CURRENTDAYOFYEAR=$(expr $CURRENTDAYOFYEAR + 1)		
	fi

	if [ $CURRENTYEAR == 1998 ]
	then
		CURRENTDAYOFYEAR=$(expr $CURRENTDAYOFYEAR + 15)
	else
		CURRENTDAYOFYEAR=$(expr $CURRENTDAYOFYEAR + 365)
	fi
done

echo $CURRENTDAYOFYEAR
