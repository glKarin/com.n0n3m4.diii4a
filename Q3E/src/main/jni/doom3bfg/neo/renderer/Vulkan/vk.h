#ifndef _VK_H
#define _VK_H

#define VK_PROC(name) extern PFN_##name q##name;
#include "vkproc.h"

#include "vkdef.h"

#endif
