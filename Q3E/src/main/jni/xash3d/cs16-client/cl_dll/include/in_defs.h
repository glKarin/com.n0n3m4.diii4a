//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#pragma once
#if !defined( IN_DEFSH )
#define IN_DEFSH

// up / down
#define	PITCH	0
// left / right
#define	YAW		1
// fall over
#define	ROLL	2 

#ifdef _WIN32
#define HSPRITE WINAPI_HSPRITE
#include <windows.h>
#undef HSPRITE
#else
#ifndef PORT_H
typedef struct point_s{
	int x;
	int y;
} POINT;
#endif
#define GetCursorPos(x)
#define SetCursorPos(x,y)
#endif

#endif
