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

// DlgInfoPgPos.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDlgInfoPgPos dialog

class CDlgInfoPgPos : public CPropertyPage
{
	DECLARE_DYNCREATE(CDlgInfoPgPos)

// Construction
public:
	CUpdateable m_udAllValues;
	BOOL OnIdle(LONG lCount);
	CDlgInfoPgPos();
	~CDlgInfoPgPos();

// Dialog Data
	//{{AFX_DATA(CDlgInfoPgPos)
	enum { IDD = IDD_INFO_POSITION };
	float	m_fLightDist;
	float	m_fHeading;
	float	m_fPitch;
	float	m_fBanking;
	float	m_fX;
	float	m_fY;
	float	m_fZ;
	float	m_fFOW;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CDlgInfoPgPos)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CDlgInfoPgPos)
	afx_msg void OnChangeEditHeading();
	afx_msg void OnChangeEditBanking();
	afx_msg void OnChangeEditPitch();
	afx_msg void OnChangeEditX();
	afx_msg void OnChangeEditY();
	afx_msg void OnChangeEditZ();
	afx_msg void OnChangeEditLightDistance();
	afx_msg void OnChangeEditFow();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};
