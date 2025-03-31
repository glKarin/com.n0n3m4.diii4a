/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#include "precompiled.h"
#pragma hdrstop



#include "ColorButton.h"

static const int ARROW_SIZE_CX = 4 ;
static const int ARROW_SIZE_CY = 2 ;

/*
================
ColorButton_SetColor

Sets the current color button color
================
*/
void ColorButton_SetColor ( HWND hWnd, COLORREF color )
{
	if ( NULL == hWnd )
	{
		return;
	}
	SetWindowLongPtr ( hWnd, GWLP_USERDATA, color );
	InvalidateRect ( hWnd, NULL, FALSE );
}

void ColorButton_SetColor ( HWND hWnd, const char* color )
{
	float red;
	float green;
	float blue;
	float alpha;

	if ( NULL == hWnd )
	{
		return;
	}
	
	sscanf ( color, "%f,%f,%f,%f", &red, &green, &blue, &alpha );
	
	ColorButton_SetColor ( hWnd, RGB(red*255.0f, green*255.0f, blue*255.0f) );
}

void AlphaButton_SetColor ( HWND hWnd, const char* color )
{
	float red;
	float green;
	float blue;
	float alpha;

	if ( NULL == hWnd )
	{
		return;
	}
	
	sscanf ( color, "%f,%f,%f,%f", &red, &green, &blue, &alpha );
	
	ColorButton_SetColor ( hWnd, RGB(alpha*255.0f, alpha*255.0f, alpha*255.0f) );
}

/*
================
ColorButton_GetColor

Retrieves the current color button color
================
*/
COLORREF ColorButton_GetColor ( HWND hWnd )
{
	return (COLORREF) GetWindowLongPtr ( hWnd, GWLP_USERDATA );
}

/*
================
ColorButton_DrawArrow

Draws the arrow on the color button
================
*/
static void ColorButton_DrawArrow ( HDC hDC, RECT* pRect, COLORREF color )
{
	POINT ptsArrow[3];

	ptsArrow[0].x = pRect->left;
	ptsArrow[0].y = pRect->top;
	ptsArrow[1].x = pRect->right;
	ptsArrow[1].y = pRect->top;
	ptsArrow[2].x = (pRect->left + pRect->right)/2;
	ptsArrow[2].y = pRect->bottom;
	
	HBRUSH arrowBrush = CreateSolidBrush ( color );
	HPEN   arrowPen   = CreatePen ( PS_SOLID, 1, color );
	
	HGDIOBJ oldBrush = SelectObject ( hDC, arrowBrush );
	HGDIOBJ oldPen   = SelectObject ( hDC, arrowPen );
	
	SetPolyFillMode(hDC, WINDING);
	Polygon(hDC, ptsArrow, 3);
	
	SelectObject ( hDC, oldBrush );
	SelectObject ( hDC, oldPen );
	
	DeleteObject ( arrowBrush );
	DeleteObject ( arrowPen );
}

/*
================
ColorButton_DrawItem

Draws the actual color button as as reponse to a WM_DRAWITEM message
================
*/
void ColorButton_DrawItem ( HWND hWnd, LPDRAWITEMSTRUCT dis )
{
	assert ( dis );

	HDC		hDC		 = dis->hDC;
	UINT    state    = dis->itemState;
    RECT	rDraw    = dis->rcItem;
	RECT	rArrow;

	// Draw outter edge
	UINT uFrameState = DFCS_BUTTONPUSH|DFCS_ADJUSTRECT;

	if (state & ODS_SELECTED)
	{
		uFrameState |= DFCS_PUSHED;
	}

	if (state & ODS_DISABLED)
	{
		uFrameState |= DFCS_INACTIVE;
	}
	
	DrawFrameControl ( hDC, &rDraw, DFC_BUTTON, uFrameState );

	// Draw Focus
	if (state & ODS_SELECTED)
	{
		OffsetRect(&rDraw, 1,1);
	}

	if (state & ODS_FOCUS) 
    {
		RECT rFocus = {rDraw.left,
					   rDraw.top,
					   rDraw.right - 1,
					   rDraw.bottom};
  
        DrawFocusRect ( hDC, &rFocus );
    }

	InflateRect ( &rDraw, -GetSystemMetrics(SM_CXEDGE), -GetSystemMetrics(SM_CYEDGE) );

	// Draw the arrow
	rArrow.left		= rDraw.right - ARROW_SIZE_CX - GetSystemMetrics(SM_CXEDGE) /2;
	rArrow.right	= rArrow.left + ARROW_SIZE_CX;
	rArrow.top		= (rDraw.bottom + rDraw.top)/2 - ARROW_SIZE_CY / 2;
	rArrow.bottom	= (rDraw.bottom + rDraw.top)/2 + ARROW_SIZE_CY / 2;

	ColorButton_DrawArrow ( hDC, &rArrow, (state & ODS_DISABLED) ? ::GetSysColor(COLOR_GRAYTEXT) : RGB(0,0,0) );

	rDraw.right = rArrow.left - GetSystemMetrics(SM_CXEDGE)/2;

	// Draw separator
	DrawEdge ( hDC, &rDraw, EDGE_ETCHED, BF_RIGHT);

	rDraw.right -= (GetSystemMetrics(SM_CXEDGE) * 2) + 1 ;

	// Draw Color				  
	if ((state & ODS_DISABLED) == 0)
	{
		HBRUSH color = CreateSolidBrush ( (COLORREF)GetWindowLongPtr ( hWnd, GWLP_USERDATA ) );
		FillRect ( hDC, &rDraw, color );
		FrameRect ( hDC, &rDraw, (HBRUSH)::GetStockObject(BLACK_BRUSH));
		DeleteObject( color );
	}
}
