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

// DlgPgRenderingStatistics.cpp : implementation file
//

#include "stdafx.h"
#include "DlgPgRenderingStatistics.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgPgRenderingStatistics property page

IMPLEMENT_DYNCREATE(CDlgPgRenderingStatistics, CPropertyPage)

CDlgPgRenderingStatistics::CDlgPgRenderingStatistics() : CPropertyPage(CDlgPgRenderingStatistics::IDD)
{
	//{{AFX_DATA_INIT(CDlgPgRenderingStatistics)
	m_strRenderingStatistics = _T("");
	//}}AFX_DATA_INIT
}

CDlgPgRenderingStatistics::~CDlgPgRenderingStatistics()
{
}

void CDlgPgRenderingStatistics::DoDataExchange(CDataExchange* pDX)
{
  if( theApp.m_bDisableDataExchange) return;

	CPropertyPage::DoDataExchange(pDX);

  // if dialog is recieving data
  if( pDX->m_bSaveAndValidate == FALSE)
  {
    CTString strRenderingProfile = "No rendering profile curently available (use F11 in game mode)\r\n";
    CTString strCSGProfile = "\r\n";
    // try to load rendering profile file ("WorldEditor.profile")
    try
    {
      theApp.m_strCSGAndShadowStatistics.Save_t(CTString("Temp\\CSGProfile.txt"));
      strCSGProfile.LoadKeepCRLF_t(CTString("Temp\\CSGProfile.txt"));

      CTFileStream fileProfile;
      fileProfile.Open_t(CTString("WorldEditor.profile"));
      // get size of profile file
      ULONG ulProfileFileSize = fileProfile.GetStreamSize();
      char *pchrFile = new char[ ulProfileFileSize+1];
      // set eol character
      pchrFile[ ulProfileFileSize] = 0;
      fileProfile.Read_t( pchrFile, ulProfileFileSize);
      strRenderingProfile = CTString( pchrFile);
      delete pchrFile;
    }
    // catch errors
    catch (const char *strError)
    {
      // and do nothing
      (void) strError;
    }
    // set new report
    m_strRenderingStatistics = strRenderingProfile + strCSGProfile;
    // notice that statistics have been updated
    m_udStatsUpdated.MarkUpdated();
  }
  
	//{{AFX_DATA_MAP(CDlgPgRenderingStatistics)
	DDX_Text(pDX, IDC_RENDERING_STATISTICS, m_strRenderingStatistics);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgPgRenderingStatistics, CPropertyPage)
	//{{AFX_MSG_MAP(CDlgPgRenderingStatistics)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgPgRenderingStatistics message handlers

BOOL CDlgPgRenderingStatistics::OnIdle(LONG lCount)
{
  // get active view 
  CWorldEditorView *pWorldEditorView = theApp.GetActiveView();

  // if we do have a window and a view and last rendered view is newer than rendering info
  if( (IsWindow(m_hWnd)) &&
      (pWorldEditorView != NULL) &&
      (!pWorldEditorView->m_chViewChanged.IsUpToDate( m_udStatsUpdated)) )
  {
    // change statistics
    UpdateData( FALSE);
  }
  return TRUE;
}

