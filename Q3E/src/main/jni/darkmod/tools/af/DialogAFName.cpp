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



#include "../../sys/win32/rc/AFEditor_resource.h"

#include "DialogAF.h"
#include "DialogAFName.h"

// DialogAFName dialog

IMPLEMENT_DYNAMIC(DialogAFName, CDialog)

/*
================
DialogAFName::DialogAFName
================
*/
DialogAFName::DialogAFName(CWnd* pParent /*=NULL*/)
	: CDialog(DialogAFName::IDD, pParent)
	, m_combo(NULL)
{
}

/*
================
DialogAFName::~DialogAFName
================
*/
DialogAFName::~DialogAFName() {
}

/*
================
DialogAFName::DoDataExchange
================
*/
void DialogAFName::DoDataExchange(CDataExchange* pDX) {
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_AF_NAME, m_editName);
}

/*
================
DialogAFName::SetName
================
*/
void DialogAFName::SetName( CString &str ) {
	m_editName = str;
}

/*
================
DialogAFName::GetName
================
*/
void DialogAFName::GetName( CString &str ) {
	str = m_editName;
}

/*
================
DialogAFName::SetComboBox
================
*/
void DialogAFName::SetComboBox( CComboBox *combo ) {
	m_combo = combo;
}

/*
================
DialogAFName::OnInitDialog
================
*/
BOOL DialogAFName::OnInitDialog()  {
	CEdit *edit;
	CString str;

	CDialog::OnInitDialog();

	edit = (CEdit *)GetDlgItem( IDC_EDIT_AF_NAME );
	edit->SetFocus();
	edit->GetWindowText( str );
	edit->SetSel( 0, str.GetLength() );

	return FALSE;
}

/*
================
EditVerifyName
================
*/
void EditVerifyName( CEdit *edit ) {
	CString strIn, strOut;
	int start, end;
	static bool entered = false;

	if ( entered ) {
		return;
	}
	entered = true;

	edit->GetSel( start, end );
	edit->GetWindowText( strIn );
	for ( int i = 0; i < strIn.GetLength(); i++ ) {
		if ( ( strIn[i] >= 'a' && strIn[i] <= 'z' ) ||
				( strIn[i] >= 'A' && strIn[i] <= 'Z' ) ||
					( strIn[i] == '_' ) || ( strIn[i] >= '0' && strIn[i] <= '9' ) ) {
			strOut.AppendChar( strIn[i] );
		}
	}
	edit->SetWindowText( strOut );
	edit->SetSel( start, end );

	entered = false;
}


BEGIN_MESSAGE_MAP(DialogAFName, CDialog)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_EN_CHANGE(IDC_EDIT_AF_NAME, OnEnChangeEditAfName)
END_MESSAGE_MAP()


// DialogAFName message handlers

void DialogAFName::OnBnClickedOk() {

	UpdateData( TRUE );
	if ( m_combo && m_combo->FindStringExact( -1, m_editName ) != -1 ) {
		MessageBox( va( "The name %s is already used.", m_editName.GetBuffer() ), "Name", MB_OK );
	}
	else {
		OnOK();
	}
}

void DialogAFName::OnEnChangeEditAfName() {
	EditVerifyName( (CEdit *) GetDlgItem( IDC_EDIT_AF_NAME ) );
}
