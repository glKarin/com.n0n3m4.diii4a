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



#define MASKEDIT_MAXINVALID	1024
typedef struct
{
	WNDPROC	mProc;
	char	mInvalid[MASKEDIT_MAXINVALID];
} rvGEMaskEdit;

/*
================
MaskEdit_WndProc

Prevents the invalid characters from being entered
================
*/
LRESULT CALLBACK MaskEdit_WndProc ( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	rvGEMaskEdit* edit = (rvGEMaskEdit*)GetWindowLongPtr ( hWnd, GWLP_USERDATA );
	WNDPROC		  wndproc = edit->mProc;

	switch ( msg )
	{
		case WM_CHAR:
			if ( strchr ( edit->mInvalid, wParam ) )
			{
				return 0;
			}
			
			break;
			
		case WM_DESTROY:
			delete edit;
			SetWindowLongPtr ( hWnd, GWLP_WNDPROC, (LONG_PTR)wndproc );
			break;
	}

	return CallWindowProc ( wndproc, hWnd, msg, wParam, lParam );
}

/*
================
MaskEdit_Attach

Attaches the mask edit control to a normal edit control
================
*/
void MaskEdit_Attach ( HWND hWnd, const char* invalid )
{
	rvGEMaskEdit* edit = new rvGEMaskEdit;
	edit->mProc = (WNDPROC)GetWindowLongPtr ( hWnd, GWLP_WNDPROC );
	strcpy ( edit->mInvalid, invalid );
	SetWindowLongPtr ( hWnd, GWLP_USERDATA, (LONG_PTR)edit );
	SetWindowLongPtr ( hWnd, GWLP_WNDPROC, (LONG_PTR)MaskEdit_WndProc );
}

/*
================
NumberEdit_Attach

Allows editing of floating point numbers
================
*/
void NumberEdit_Attach ( HWND hWnd )
{
	static const char invalid[] = "`~!@#$%^&*()_+|=\\qwertyuiop[]asdfghjkl;'zxcvbnm,/QWERTYUIOP{}ASDFGHJKL:ZXCVBNM<>";
	MaskEdit_Attach ( hWnd, invalid );
}
