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

// DlgInfoSheet.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDlgInfoSheet

#define IM_NONE    0
#define IM_NORMAL  1
#define IM_MAPPING 2

class CDlgInfoSheet : public CPropertySheet
{
// Construction
public:
	CUpdateable m_Updateable;
  CModelerView *m_LastViewUpdated;
  INDEX m_InfoMode;
	BOOL OnIdle(LONG lCount);
	DECLARE_DYNAMIC(CDlgInfoSheet)
	CDlgInfoSheet(CWnd* pWndParent);
  void CustomSetActivePage( CPropertyPage *pppToActivate);

	CDlgInfoPgNone m_PgInfoNone;
  CDlgInfoPgRendering m_PgInfoRendering;
	CDlgInfoPgGlobal m_PgInfoGlobal;
	CDlgInfoPgMip m_PgInfoMip;
	CDlgInfoPgPos m_PgInfoPos;
	CDlgInfoPgAnim m_PgInfoAnim;
	CDlgPgCollision m_PgInfoCollision;
	CDlgPgInfoAttachingPlacement m_PgAttachingPlacement;
	CDlgPgInfoAttachingSound m_PgAttachingSound;
	CDlgInfoSheet(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	CDlgInfoSheet(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgInfoSheet)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CDlgInfoSheet();

	// Generated message map functions
protected:
	//{{AFX_MSG(CDlgInfoSheet)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
