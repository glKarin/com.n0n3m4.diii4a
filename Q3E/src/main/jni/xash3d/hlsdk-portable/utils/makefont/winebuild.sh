#!/bin/sh

winegcc -g -m32 makefont.cpp ../common/wadlib.c ../common/cmdlib.c -I../common/ -I../../common/ -mwindows -mno-cygwin -lwinmm -fno-stack-protector 
