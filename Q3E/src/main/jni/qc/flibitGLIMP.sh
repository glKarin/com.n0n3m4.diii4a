#!/bin/bash
set -ex
cd "`dirname "$0"`"
cd sys/linux
m4 glimp_dlopen.cpp.m4 > glimp_dlopen.cpp
m4 glimp_local.h.m4 > glimp_local.h
