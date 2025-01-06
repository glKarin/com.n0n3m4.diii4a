#ifndef _GIT_INFO_COMPAT_H
#define _GIT_INFO_COMPAT_H

#if !defined(__ANDROID__) //karin: ignore git
#include "gitinfo.h"
#else
#define GIT_DESCRIPTION "GZDoom is a feature centric port for all Doom engine games, based on ZDoom, adding an OpenGL renderer and powerful scripting capabilities"
#define GIT_HASH "71c40432e5e893c629a1c9c76a523a0ab22bd56a"
#define GIT_TIME "Apr 28, 2024"
#endif

#endif