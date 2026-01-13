#!/bin/bash
set -e

mkdir -p libs/armeabi
cp ../doom.arm libs/armeabi/libdoom3.so
cp ../gamearm-base.so libs/armeabi/libgame-base.so
ant debug
adb install -r bin/Dante-debug.apk
