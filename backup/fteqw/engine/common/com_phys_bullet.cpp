#include "quakedef.h"

#ifdef USE_INTERNAL_BULLET
#define FTEENGINE
#undef FTEPLUGIN
#include "../../plugins/bullet/bulletplug.cpp"
#endif