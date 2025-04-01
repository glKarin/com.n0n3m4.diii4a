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



#include "../../sys/win32/win_local.h"

//  source file that includes just the standard includes
//	Radiant.pch will be the pre-compiled header
//	stdafx.obj will contain the pre-compiled type information

/*
===============================================================================

	Afx initialization.

===============================================================================
*/

bool afxInitialized = false;

/*
================
InitAfx
================
*/
void InitAfx( void ) {
	if ( !afxInitialized ) {
		AfxWinInit( win32.hInstance, NULL, "", SW_SHOW );
		AfxInitRichEdit();
		afxInitialized = true;
	}
}


/*
===============================================================================

	Tool Tips.

===============================================================================
*/

/*
================
DefaultOnToolHitTest
================
*/
int DefaultOnToolHitTest( const toolTip_t *toolTips, const CDialog *dialog, CPoint point, TOOLINFO* pTI ) {
	CWnd *wnd;
	RECT clientRect, rect;

	dialog->GetWindowRect( &clientRect );
	point.x += clientRect.left;
	point.y += clientRect.top;
	for ( int i = 0; toolTips[i].tip; i++ ) {
		wnd = dialog->GetDlgItem( toolTips[i].id );
		if ( !( wnd->GetStyle() & WS_VISIBLE ) ) {
			continue;
		}
		wnd->GetWindowRect( &rect );
		if ( point.x >= rect.left && point.x <= rect.right && point.y >= rect.top && point.y <= rect.bottom ) {
			pTI->hwnd = dialog->GetSafeHwnd();
			pTI->uFlags |= TTF_IDISHWND;
			pTI->uFlags &= ~TTF_CENTERTIP;
			pTI->uId = (UINT_PTR) wnd->GetSafeHwnd();
			return pTI->uId;
		}
	}
	return -1;
}

/*
================
DefaultOnToolTipNotify
================
*/
BOOL DefaultOnToolTipNotify( const toolTip_t *toolTips, UINT id, NMHDR *pNMHDR, LRESULT *pResult ) {
	// need to handle both ANSI and UNICODE versions of the message
	TOOLTIPTEXTA* pTTTA = (TOOLTIPTEXTA*)pNMHDR;
	TOOLTIPTEXTW* pTTTW = (TOOLTIPTEXTW*)pNMHDR;

	*pResult = 0;

	UINT_PTR nID = pNMHDR->idFrom;
	if ( pTTTA->uFlags & TTF_IDISHWND ) {
		// idFrom is actually the HWND of the tool
		nID = ::GetDlgCtrlID((HWND)nID);
	}

	int i;
	for ( i = 0; toolTips[i].tip; i++ ) {
		if ( toolTips[i].id == nID ) {
			break;
		}
	}

	if ( !toolTips[i].tip ) {
		return FALSE;
	}

	if ( pNMHDR->code == TTN_NEEDTEXTA ) {
		lstrcpyn( pTTTA->szText, toolTips[i].tip, sizeof(pTTTA->szText) );
	} else {
		_mbstowcsz( pTTTW->szText, toolTips[i].tip, sizeof(pTTTW->szText) );
	}
	return TRUE;
}


/*
===============================================================================

	Common control tools.

===============================================================================
*/

/*
================
EditControlEnterHit

  returns true if [Enter] was hit in the edit box
  all 'return' characters in the text are removed and a single line is maintained
  the edit control must be multi-line with auto-vscroll
================
*/
bool EditControlEnterHit( CEdit *edit ) {
	CString strIn, strOut;
	if ( edit->GetLineCount() > 1 ) {
		edit->GetWindowText( strIn );
		for ( int i = 0; i < strIn.GetLength(); i++ ) {
			if ( strIn[i] >= ' ' ) {
				strOut.AppendChar( strIn[i] );
			}
		}
		edit->SetWindowText( strOut );
		edit->SetSel( 0, strOut.GetLength() );
		return true;
	}
	return false;
}

/*
================
EditVerifyFloat
================
*/
float EditVerifyFloat( CEdit *edit, bool allowNegative ) {

	CString strIn, strOut;
	bool dot = false;
	int start, end;

	edit->GetSel( start, end );
	edit->GetWindowText( strIn );
	for ( int i = 0; i < strIn.GetLength(); i++ ) {
		// first character may be a minus sign
		if ( allowNegative && strOut.GetLength() == 0 && strIn[i] == '-' ) {
			strOut.AppendChar( '-' );
		}
		// the string may contain one dot
		else if ( !dot && strIn[i] == '.' ) {
			strOut.AppendChar( strIn[i] );
			dot = true;
		}
		else if ( strIn[i] >= '0' && strIn[i] <= '9' ) {
			strOut.AppendChar( strIn[i] );
		}
	}
	edit->SetWindowText( strOut );
	edit->SetSel( start, end );

	return atof(strOut.GetBuffer(0));

}

/*
================
SpinFloatString
================
*/
void SpinFloatString( CString &str, bool up ) {
	int i, dotIndex = -1, digitIndex = -1;

	for ( i = 0; str[i]; i++ ) {
		if ( str[i] == '.' ) {
			if ( dotIndex == -1 ) {
				dotIndex = i;
			}
		}
		else if ( str[i] != '0' ) {
			if ( digitIndex == -1 ) {
				digitIndex = i;
			}
		}
	}
	if ( digitIndex == -1 ) {
		str.SetString( "1" );
		return;
	}

	if ( dotIndex != -1 ) {
		str.Delete( dotIndex, 1 );
		if ( digitIndex > dotIndex ) {
			digitIndex--;
		}
	}
	else {
		dotIndex = i;
	}

	if ( up ) {
		if ( str[digitIndex] == '9' ) {
			str.SetAt( digitIndex, '0' );
			if ( digitIndex == 0 ) {
				str.Insert( 0, '1' );
				dotIndex++;
			}
			else {
				str.SetAt( digitIndex-1, '1' );
			}
		}
		else {
			str.SetAt( digitIndex, str[digitIndex] + 1 );
		}
	}
	else {
		if ( str[digitIndex] == '1' ) {
			if ( str[digitIndex+1] == '\0' ) {
				str.SetAt( digitIndex, '0' );
				str.AppendChar( '9' );
			}
			else if ( str[digitIndex+1] == '0' ) {
				str.SetAt( digitIndex, '0' );
				str.SetAt( digitIndex+1, '9' );
			}
			else {
				str.SetAt( digitIndex+1, str[digitIndex+1] - 1 );
			}
		}
		else {
			str.SetAt( digitIndex, str[digitIndex] - 1 );
		}
	}
	if ( dotIndex < str.GetLength() ) {
		str.Insert( dotIndex, '.' );
		// remove trailing zeros
		for ( i = str.GetLength()-1; i >= 0; i-- ) {
			if ( str[i] != '0' && str[i] != '.' ) {
				break;
			}
		}
		if ( i < str.GetLength() - 1 ) {
			str.Delete( i+1, str.GetLength() - i );
		}
	}
	for ( i = 0; str[i]; i++ ) {
		if ( str[i] == '.' ) {
			if ( i > 1 ) {
				str.Delete( 0, i-1 );
			}
			break;
		}
		if ( str[i] != '0' ) {
			if ( i > 0 ) {
				str.Delete( 0, i );
			}
			break;
		}
	}
}

/*
================
EditSpinFloat
================
*/
float EditSpinFloat( CEdit *edit, bool up ) {
	CString str;

	edit->GetWindowText( str );
	SpinFloatString( str, up );
	edit->SetWindowText( str );
	return atof( str );
}

/*
================
SetSafeComboBoxSelection
================
*/
int SetSafeComboBoxSelection( CComboBox *combo, const char *string, int skip ) {
	int index;

	index = combo->FindString( -1, string );
	if ( index == -1 ) {
		index = 0;
	}
	if ( combo->GetCount() != 0 ) {
		if ( index == skip ) {
			index = ( skip + 1 ) % combo->GetCount();
		}
		combo->SetCurSel( index );
	}

	return index;
}

/*
================
GetComboBoxSelection
================
*/
int GetSafeComboBoxSelection( CComboBox *combo, CString &string, int skip ) {
	int index;

	index = combo->GetCurSel();
	if ( index == CB_ERR ) {
		index = 0;
	}
	if ( combo->GetCount() != 0 ) {
		if ( index == skip ) {
			index = ( skip + 1 ) % combo->GetCount();
		}
		combo->GetLBText( index, string );
	}
	else {
		string = "";
	}

	return index;
}

/*
================
UnsetSafeComboBoxSelection
================
*/
int UnsetSafeComboBoxSelection( CComboBox *combo, CString &string ) {
	int skip, index;

	skip = combo->FindString( -1, string );
	index = combo->GetCurSel();
	if ( index == CB_ERR ) {
		index = 0;
	}
	if ( combo->GetCount() != 0 ) {
		if ( index == skip ) {
			index = ( skip + 1 ) % combo->GetCount();
		}
		combo->SetCurSel( index );
	}

	return index;
}
