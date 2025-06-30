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

// DlgConsole.cpp : implementation file
//

#include "stdafx.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgConsole dialog

CDlgConsole::CDlgConsole(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgConsole::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgConsole)
	m_strConsoleOutput = _T("");
	//}}AFX_DATA_INIT
}


void CDlgConsole::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	//{{AFX_DATA_MAP(CDlgConsole)
	DDX_Control(pDX, IDC_CONSOLE_SYMBOLS, m_ctrConsoleSymbolsCombo);
	DDX_Control(pDX, IDC_CONSOLE_INPUT, m_ctrlEditConsole);
	DDX_Text(pDX, IDC_CONSOLE_OUTPUT, m_strConsoleOutput);
	//}}AFX_DATA_MAP

  // if dialog is reciving data
  if( pDX->m_bSaveAndValidate == FALSE)
  {
    //m_strConsoleOutput = "Default output string";
//    m_strConsoleOutput = _pConsole->GetBuffer();
    m_ctrlEditConsole.SetTextFromConsole();
  }
}


BEGIN_MESSAGE_MAP(CDlgConsole, CDialog)
	//{{AFX_MSG_MAP(CDlgConsole)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgConsole message handlers

BOOL CDlgConsole::OnInitDialog() 
{
	CDialog::OnInitDialog();

  // set default console text
  //m_ctrlEditConsole.SetWindowText( "Default input string");
  m_ctrlEditConsole.SetWindowText( CString(_pGame->gam_strConsoleInputBuffer));

  /*
  //  create application windows font for console
	LOGFONT logFont;
  CFont fntFont;
	memset(&logFont, 0, sizeof(logFont));
	if (!::GetSystemMetrics(SM_DBCSENABLED))
	{
		logFont.lfHeight = -11;
		logFont.lfWeight = FW_REGULAR;
		logFont.lfPitchAndFamily = FF_ROMAN|FIXED_PITCH;
    logFont.lfOrientation = 10;
    logFont.lfQuality = PROOF_QUALITY;
    logFont.lfItalic = TRUE;
		// prepare default font name
    CString strDefaultFont;
		strDefaultFont.LoadString(IDS_DEFAULT_ARIAL);
		lstrcpy(logFont.lfFaceName, strDefaultFont);
    // try to create font
    if( !fntFont.CreateFontIndirect(&logFont))
			TRACE0("Could Not create font for console\n");
	}
	else
	{
    fntFont.Attach(::GetStockObject(SYSTEM_FONT));
	}
  m_ctrlEditConsole.SetFont( &fntFont);
  */
  
  // fill symbols combo box
  m_ctrConsoleSymbolsCombo.ResetContent();
  // for each of symbols in the shell
  FOREACHINDYNAMICARRAY(_pShell->sh_assSymbols, CShellSymbol, itss)
  {
    // if it is not visible to user
    if (!(itss->ss_ulFlags&SSF_USER)) {
      // skip it
      continue;
    }
    // get completion name for that symbol
    CTString strSymbol = itss->GetCompletionString();
    // add string to console
    m_ctrConsoleSymbolsCombo.AddString( CString(strSymbol));
  }
  // select first combo member
  m_ctrConsoleSymbolsCombo.SetCurSel( 0);

  m_ctrlEditConsole.SetSel( -1, 60000);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
