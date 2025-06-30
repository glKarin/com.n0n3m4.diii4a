//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#pragma once
#if !defined( WRECTH )
#define WRECTH

typedef struct rect_s
{
	int	left;
	int right;
	int top;
	int bottom;

#ifdef __cplusplus
	int Width()
	{
		return right - left;
	}

	int Height()
	{
		return bottom - top;
	}
#endif
} wrect_t;

#endif
