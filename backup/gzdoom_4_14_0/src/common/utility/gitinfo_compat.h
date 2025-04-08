#ifndef _GIT_INFO_COMPAT_H
#define _GIT_INFO_COMPAT_H

#if !defined(__ANDROID__) //karin: ignore git
#include "gitinfo.h"
#else
#define GIT_DESCRIPTION "GZDoom is a feature centric port for all Doom engine games, based on ZDoom, adding an OpenGL renderer and powerful scripting capabilities"
#define GIT_HASH "25ec8a689d5654a7f57869e09c0ce0d0892ba6a4"
#define GIT_TIME "Dec 18, 2024"
#endif

#endif