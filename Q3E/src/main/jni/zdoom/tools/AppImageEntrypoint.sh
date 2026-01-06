#!/bin/bash

if [ -f "/usr/lib/libopenal.so.1" ] || [ -f "/usr/lib64/libopenal.so.1" ] || [ -f "/lib/x86_64-linux-gnu/libopenal.so.1" ]
then
	echo "System OpenAL found"
else
	echo "System OpenAL NOT found"
	export LD_LIBRARY_PATH="$APPDIR/usr/lib/fallback:$LD_LIBRARY_PATH"
fi

exec "$APPDIR/usr/bin/uzdoom.bin" "$@"
