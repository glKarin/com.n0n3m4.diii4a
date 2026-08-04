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
#if !defined(AFX_TEXTUREBAR_H__86220273_B656_11D1_B59F_00AA00A410FC__INCLUDED_)
#define AFX_TEXTUREBAR_H__86220273_B656_11D1_B59F_00AA00A410FC__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// TextureBar.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CTextureBar dialog

class CTextureBar : public CDialogBar
{
// Construction
public:
	void GetSurfaceAttributes();
	void SetSurfaceAttributes();
	CTextureBar();

// Dialog Data
	//{{AFX_DATA(CTextureBar)
	enum { IDD = IDD_TEXTUREBAR };
	CSpinButtonCtrl	m_spinRotate;
	CSpinButtonCtrl	m_spinVScale;
	CSpinButtonCtrl	m_spinVShift;
	CSpinButtonCtrl	m_spinHScale;
	CSpinButtonCtrl	m_spinHShift;
	int	m_nHShift;
	int	m_nHScale;
	int	m_nRotate;
	int	m_nVShift;
	int	m_nVScale;
	int		m_nRotateAmt;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTextureBar)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CTextureBar)
	afx_msg void OnDeltaposSpinHshift(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeltaposSpinVshift(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeltaposSpinHScale(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeltaposSpinVScale(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDeltaposSpinRotate(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSelectionPrint();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnBtnApplytexturestuff();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TEXTUREBAR_H__86220273_B656_11D1_B59F_00AA00A410FC__INCLUDED_)
