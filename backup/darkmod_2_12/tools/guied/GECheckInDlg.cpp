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



#include "../../sys/win32/rc/guied_resource.h"

#include "GEApp.h"

typedef struct
{
	const char*		mFilename;
	idStr*			mComment;
	
} GECHECKINDLG;

/*
================
GECheckInDlg_GeneralProc

Dialog procedure for the check in dialog
================
*/
static INT_PTR CALLBACK GECheckInDlg_GeneralProc ( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	GECHECKINDLG* dlg = (GECHECKINDLG*) GetWindowLongPtr ( hwnd, GWLP_USERDATA );
	
	switch ( msg )
	{
		case WM_INITDIALOG:		
			SetWindowLongPtr ( hwnd, GWLP_USERDATA, lParam );
			dlg = (GECHECKINDLG*) lParam;
			
			SetWindowText ( GetDlgItem ( hwnd, IDC_GUIED_FILENAME ), dlg->mFilename );
			break;
			
		case WM_COMMAND:
			switch ( LOWORD ( wParam ) )
			{
				case IDOK:
				{
					char* temp;
					int	  tempsize;
					
					tempsize = GetWindowTextLength ( GetDlgItem ( hwnd, IDC_GUIED_COMMENT ) );
					temp = new char [ tempsize + 2 ];
					GetWindowText ( GetDlgItem ( hwnd, IDC_GUIED_COMMENT ), temp, tempsize + 1 );
					
					*dlg->mComment = temp;
					
					delete[] temp;
					
					EndDialog ( hwnd, 1 );
					break;
				}
					
				case IDCANCEL:
					EndDialog ( hwnd, 0 );
					break;
			}
			break;
	}
	
	return FALSE;
}

/*
================
GECheckInDlg_DoModal

Starts the check in dialog
================
*/
bool GECheckInDlg_DoModal ( HWND parent, const char* filename, idStr* comment )
{
	GECHECKINDLG	dlg;
	
	dlg.mComment = comment;
	dlg.mFilename = filename;
	
	if ( !DialogBoxParam ( gApp.GetInstance(), MAKEINTRESOURCE(IDD_GUIED_CHECKIN), parent, GECheckInDlg_GeneralProc, (LPARAM) &dlg ) )
	{
		return false;
	}
	
	return true;
}

