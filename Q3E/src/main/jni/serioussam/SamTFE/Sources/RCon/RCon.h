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

// RCon.h : main header file for the RCON application
//

#if !defined(AFX_RCON_H__2FCD4617_96D7_11D5_9918_000021211E76__INCLUDED_)
#define AFX_RCON_H__2FCD4617_96D7_11D5_9918_000021211E76__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CRConApp:
// See RCon.cpp for the implementation of this class
//

class CRConApp : public CWinApp
{
public:
	CRConApp();

  ULONG m_ulHost;
  UWORD m_uwPort;
  ULONG m_ulCode;
  CTString m_strPass;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRConApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation
	virtual BOOL SubInitInstance();

	//{{AFX_MSG(CRConApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

extern CRConApp theApp;
void MinimizeApp(void);
void RestoreApp(void);

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_RCON_H__2FCD4617_96D7_11D5_9918_000021211E76__INCLUDED_)


