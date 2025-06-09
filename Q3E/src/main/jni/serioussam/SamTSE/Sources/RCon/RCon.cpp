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

// RCon.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "RCon.h"
#include "RConDlg.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRConApp

BEGIN_MESSAGE_MAP(CRConApp, CWinApp)
	//{{AFX_MSG_MAP(CRConApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CRConApp construction

CRConApp::CRConApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CRConApp object

CRConApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CRConApp initialization

BOOL CRConApp::SubInitInstance()
{
  // initialize engine
  SE_InitEngine( "Serious Sam");

  CTString strCmdLine = CStringA(m_lpCmdLine);
  char strHost[80], strPass[80];

  strHost[0] = 0;
  strPass[0] = 0;

  ULONG ulPort;
  strCmdLine.ScanF("%80s %u \"%80[^\"]\"", strHost, &ulPort, strPass); 
  
  
  CNetworkProvider np;
  np.np_Description = "TCP/IP Client";
  _pNetwork->StartProvider_t(np);

  m_ulHost = StringToAddress(strHost);
  m_uwPort = ulPort;
  m_ulCode = rand()*rand();
  m_strPass = strPass;

  CRConDlg dlg;
	m_pMainWnd = &dlg;
  dlg.m_strLog = (const char*)CTString(0, 
    "Serious Sam RCON v1.0\r\nServer: %s:%d\r\nReady for commands...\r\n", strHost, ulPort);
	int nResponse = dlg.DoModal();

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}

BOOL CRConApp::InitInstance()
{
  BOOL bResult;
  CTSTREAM_BEGIN {
    bResult = SubInitInstance();
  } CTSTREAM_END;

  return bResult;
}

void MinimizeApp(void)
{
  theApp.m_pMainWnd->ShowWindow(SW_SHOWMINIMIZED);
//  theApp.m_pMainWnd->ShowWindow(SW_HIDE);
}

void RestoreApp(void)
{
  theApp.m_pMainWnd->EnableWindow(TRUE);
  theApp.m_pMainWnd->SetActiveWindow();
  theApp.m_pMainWnd->ShowWindow(SW_SHOW);
  theApp.m_pMainWnd->SetFocus();
  theApp.m_pMainWnd->SetForegroundWindow();
}
