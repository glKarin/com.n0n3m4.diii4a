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
#if !defined(AFX_DIALOGSOUNDGROUP_H__3503E935_1F86_4484_903F_021CBDAB7729__INCLUDED_)
#define AFX_DIALOGSOUNDGROUP_H__3503E935_1F86_4484_903F_021CBDAB7729__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DialogSoundGroup.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDialogSoundGroup dialog

class CDialogSoundGroup : public CDialog
{
// Construction
public:
	idStrList list;
	CDialogSoundGroup(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDialogSoundGroup)
	enum { IDD = IDD_DIALOG_SOUNDGROUP };
	CListBox	lstGroups;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDialogSoundGroup)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDialogSoundGroup)
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DIALOGSOUNDGROUP_H__3503E935_1F86_4484_903F_021CBDAB7729__INCLUDED_)
