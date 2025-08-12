#ifndef _GIT_INFO_COMPAT_H
#define _GIT_INFO_COMPAT_H

#if !defined(__ANDROID__) //karin: ignore git
#include "gitinfo.h"
#else
#define GIT_DESCRIPTION "GZDoom is a feature centric port for all Doom engine games, based on ZDoom, adding an OpenGL renderer and powerful scripting capabilities"
#define GIT_HASH "99aa489d09015a95bb78df2b30ede29f328cc874"
#define GIT_TIME "May 2, 2025"
#endif

#endif