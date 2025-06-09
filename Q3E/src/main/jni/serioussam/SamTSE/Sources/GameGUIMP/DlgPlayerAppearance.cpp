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

// DlgPlayerAppearance.cpp : implementation file
//

#include "stdafx.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgPlayerAppearance dialog


CDlgPlayerAppearance::CDlgPlayerAppearance(CPlayerCharacter &pcPlayerCharacter,
                                           CWnd* pParent /*=NULL*/)
	: CDialog(CDlgPlayerAppearance::IDD, pParent)
{
	// make a copy of player's appearance, we will change it
  m_pcPlayerCharacter = pcPlayerCharacter;
  //{{AFX_DATA_INIT(CDlgPlayerAppearance)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CDlgPlayerAppearance::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgPlayerAppearance)
	DDX_Control(pDX, IDC_AVAILABLE_APPEARANCES, m_comboAvailableAppearances);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgPlayerAppearance, CDialog)
	//{{AFX_MSG_MAP(CDlgPlayerAppearance)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgPlayerAppearance message handlers
