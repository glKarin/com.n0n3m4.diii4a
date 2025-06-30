/* Copyright (c) 2002-2012 Croteam Ltd. 
This program is free software; you can redistribute it and/or modify
it under the terms of version 2 of the GNU General Public License as published by
the Free Software Foundation


This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA. */

// RConDlg.cpp : implementation file
//

#include "stdafx.h"
#include <Engine/Network/CommunicationInterface.h>
#include "RCon.h"
#include "RConDlg.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRConDlg dialog

CRConDlg::CRConDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CRConDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CRConDlg)
	m_strLog = _T("");
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CRConDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CRConDlg)
	DDX_Text(pDX, IDC_LOG, m_strLog);
	//}}AFX_DATA_MAP

  // keep the last line visible
  CEdit *pctrlLog = (CEdit *) (GetDlgItem(IDC_LOG));
  if( pctrlLog != NULL)
  {
    int iLines=pctrlLog->GetLineCount();
    pctrlLog->LineScroll(iLines);
  }
}

BEGIN_MESSAGE_MAP(CRConDlg, CDialog)
	//{{AFX_MSG_MAP(CRConDlg)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_CLOSE()
	ON_WM_TIMER()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRConDlg message handlers

BOOL CRConDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	// TODO: Add extra initialization here

  SetTimer(0, 10, NULL);
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CRConDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CRConDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

BOOL CRConDlg::PreTranslateMessage(MSG* pMsg) 
{
	// if we caught key down message
  if( pMsg->message==WM_KEYDOWN)
  {
    if((int)pMsg->wParam==VK_RETURN)
    {
      UpdateData(TRUE);
      CWnd *pwndCommand = GetDlgItem(IDC_COMMAND);
      if(pwndCommand==CWnd::GetFocus()) {
        CString strCommand;
        pwndCommand->GetWindowText(strCommand);

        // send chat string to user(s)
        m_strLog += ">"+strCommand+"\r\n";
        pwndCommand->SetWindowText(L"");
        UpdateData(FALSE);

        CNetworkMessage nm(MSG_EXTRA);
        nm<<CTString(0, "rcmd %u \"%s\" %s\n", theApp.m_ulCode, (const char*)theApp.m_strPass, (const char*)CStringA(strCommand));
        _pNetwork->SendBroadcast(nm, theApp.m_ulHost, theApp.m_uwPort);
        _cmiComm.Client_Update();
      }
    }
  }
	
	return CDialog::PreTranslateMessage(pMsg);
}

void CRConDlg::OnCancel() 
{
  MinimizeApp();
}

void CRConDlg::OnOK() 
{
}

void CRConDlg::OnClose() 
{
  PostMessage(WM_QUIT);
	CDialog::OnClose();
}

void CRConDlg::OnTimer(UINT_PTR nIDEvent) 
{
  // repeat
  BOOL bChanged = FALSE;
  FOREVER {
    CNetworkMessage nmReceived;

    _cmiComm.Client_Update();
    ULONG ulFrom;
    UWORD uwPort;
    BOOL bHasMsg = _pNetwork->ReceiveBroadcast(nmReceived, ulFrom, uwPort);
    // if there are no more messages
    if (!bHasMsg) {
      // finish
      break;
    }

    // if this message is not valid rcon message
    if (nmReceived.GetType()!=MSG_EXTRA) {
      // skip it
      continue;
    }
    // get the string from the message
    CTString strMsg;
    nmReceived>>strMsg;

    // accept logs only
    if (!strMsg.RemovePrefix("log ")) {
      continue;
    }
    ULONG ulCode;
    INDEX iLine;
    char strLine[256];
    strMsg.ScanF("%u %d %256[^\n]", &ulCode, &iLine, strLine);
    if (ulCode!=theApp.m_ulCode) {
      continue;
    }

    m_strLog += (const char*)strLine;
    m_strLog += "\r\n";
    bChanged = TRUE;
  }
  if (bChanged) {
    UpdateData(FALSE);
  }
	
	CDialog::OnTimer(nIDEvent);
}

