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



#include "../../sys/win32/rc/Common_resource.h"

#include "DialogGoToLine.h"

#if defined(ID_DEBUG_MEMORY) && defined(ID_REDIRECT_NEWDELETE)
#undef new
#undef DEBUG_NEW
#define DEBUG_NEW new
#endif


IMPLEMENT_DYNAMIC(DialogGoToLine, CDialog)

/*
================
DialogGoToLine::DialogGoToLine
================
*/
DialogGoToLine::DialogGoToLine( CWnd* pParent /*=NULL*/ )
	: CDialog(DialogGoToLine::IDD, pParent)
	, firstLine(0)
	, lastLine(0)
	, line(0)
{
}

/*
================
DialogGoToLine::~DialogGoToLine
================
*/
DialogGoToLine::~DialogGoToLine() {
}

/*
================
DialogGoToLine::DoDataExchange
================
*/
void DialogGoToLine::DoDataExchange(CDataExchange* pDX) {
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(DialogGoToLine)
	DDX_Control( pDX, IDC_GOTOLINE_EDIT, numberEdit);
	//}}AFX_DATA_MAP
}

/*
================
DialogGoToLine::SetRange
================
*/
void DialogGoToLine::SetRange( int firstLine, int lastLine ) {
    this->firstLine = firstLine;
	this->lastLine = lastLine;
}

/*
================
DialogGoToLine::GetLine
================
*/
int DialogGoToLine::GetLine( void ) const {
	return line;
}

/*
================
DialogGoToLine::OnInitDialog
================
*/
BOOL DialogGoToLine::OnInitDialog()  {

	CDialog::OnInitDialog();

	GetDlgItem( IDC_GOTOLINE_STATIC )->SetWindowText( va( "&Line number (%d - %d):", firstLine, lastLine ) );

	numberEdit.SetWindowText( va( "%d", firstLine ) );
	numberEdit.SetSel( 0, -1 );
	numberEdit.SetFocus();

	return FALSE; // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


BEGIN_MESSAGE_MAP(DialogGoToLine, CDialog)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
END_MESSAGE_MAP()


// DialogGoToLine message handlers

/*
================
DialogGoToLine::OnBnClickedOk
================
*/
void DialogGoToLine::OnBnClickedOk() {
	CString text;
	numberEdit.GetWindowText( text );
	line = idMath::ClampInt( firstLine, lastLine, atoi( text ) );
	OnOK();
}
