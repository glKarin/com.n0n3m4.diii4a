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



#include "qe3.h"
#include "WaitDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWaitDlg dialog


CWaitDlg::CWaitDlg(CWnd* pParent, const char *msg)
	: CDialog(CWaitDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CWaitDlg)
	waitStr = msg;
	//}}AFX_DATA_INIT
	cancelPressed = false;
	Create(CWaitDlg::IDD);
	//g_pParentWnd->SetBusy(true);
}

CWaitDlg::~CWaitDlg() {
	g_pParentWnd->SetBusy(false);
}

void CWaitDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CWaitDlg)
	DDX_Text(pDX, IDC_WAITSTR, waitStr);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CWaitDlg, CDialog)
	//{{AFX_MSG_MAP(CWaitDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWaitDlg message handlers

BOOL CWaitDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	//GetDlgItem(IDC_WAITSTR)->SetWindowText(waitStr);
	GetDlgItem(IDC_WAITSTR)->SetFocus();
	UpdateData(FALSE);
	ShowWindow(SW_SHOW);
	
	// cancel disabled by default
	AllowCancel( false );

	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CWaitDlg::SetText(const char *msg, bool append) {
	if (append) {
		waitStr = text;
		waitStr += "\r\n";
		waitStr += msg;
	} else {
		waitStr = msg;
		text = msg;
	}
	UpdateData(FALSE);
	Invalidate();
	UpdateWindow();
	ShowWindow (SW_SHOWNORMAL);
}

void CWaitDlg::AllowCancel( bool enable ) {
	// this shows or hides the Cancel button
	CWnd* pCancelButton = GetDlgItem (IDCANCEL);
	ASSERT (pCancelButton);
	if ( enable ) {
		pCancelButton->ShowWindow (SW_NORMAL);
	} else {
		pCancelButton->ShowWindow (SW_HIDE);
	}
}

bool CWaitDlg::CancelPressed( void ) {
#if _MSC_VER >= 1300
	MSG *msg = AfxGetCurrentMessage();			// TODO Robert fix me!!
#else
	MSG *msg = &m_msgCur;
#endif

	while( ::PeekMessage(msg, NULL, NULL, NULL, PM_NOREMOVE) ) {
		// pump message
		if ( !AfxGetApp()->PumpMessage() ) {
		}
	}

	return cancelPressed;
}

void CWaitDlg::OnCancel() {
	cancelPressed = true;
}
