//========= Copyright (c) 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#pragma once
#if !defined( HUD_IFACEH )
#define HUD_IFACEH

#include "exportdef.h"
// redefine
// typedef int (*pfnUserMsgHook)( const char *pszName, int iSize, void *pbuf );
#include "wrect.h"
#include "../engine/cdll_int.h"
extern cl_enginefunc_t gEngfuncs;
#endif
