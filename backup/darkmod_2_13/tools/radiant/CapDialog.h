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
#if !defined(AFX_CAPDIALOG_H__10637162_2BD2_11D2_B030_00AA00A410FC__INCLUDED_)
#define AFX_CAPDIALOG_H__10637162_2BD2_11D2_B030_00AA00A410FC__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// CapDialog.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CCapDialog dialog

class CCapDialog : public CDialog
{
// Construction
public:
  enum {BEVEL = 0, ENDCAP, IBEVEL, IENDCAP};
	CCapDialog(CWnd* pParent = NULL);   // standard constructor

  int getCapType() {return m_nCap;};
// Dialog Data
	//{{AFX_DATA(CCapDialog)
	enum { IDD = IDD_DIALOG_CAP };
	int		m_nCap;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCapDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CCapDialog)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CAPDIALOG_H__10637162_2BD2_11D2_B030_00AA00A410FC__INCLUDED_)
