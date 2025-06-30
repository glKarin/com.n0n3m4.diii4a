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

// DlgInfoPgGlobal.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDlgInfoPgGlobal dialog

class CDlgInfoPgGlobal : public CPropertyPage
{
	DECLARE_DYNCREATE(CDlgInfoPgGlobal)

// Construction
public:
	CUpdateable m_udAllValues;
	BOOL OnIdle(LONG lCount);
	CDlgInfoPgGlobal();
	~CDlgInfoPgGlobal();

  void SetGlobalPageFromView(CModelerView* pModelerView);
  void SetViewFromGlobalPage(CModelerView* pModelerView);

// Dialog Data
	//{{AFX_DATA(CDlgInfoPgGlobal)
	enum { IDD = IDD_INFO_GLOBAL };
	CColoredButton	m_colorDiffuse;
	CColoredButton	m_colorBump;
	CColoredButton	m_colorSpecular;
	CColoredButton	m_colorReflection;
	CString	m_strFlat;
	CString	m_strTextureSize;
	CString	m_strWndSize;
	CString	m_strReflections;
	CString	m_strDifuse;
	CString	m_strHighQuality;
	CString	m_strSpecular;
	CString	m_strMaxShadow;
	CString	m_strBump;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CDlgInfoPgGlobal)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CDlgInfoPgGlobal)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};
