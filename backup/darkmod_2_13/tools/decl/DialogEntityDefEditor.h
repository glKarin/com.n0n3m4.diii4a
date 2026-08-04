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

#ifndef __DIALOGENTITYDEFEDITOR_H__
#define __DIALOGENTITYDEFEDITOR_H__

#pragma once

#include "../radiant/PropertyList.h"

// DialogEntityDefEditor dialog

class DialogEntityDefEditor : public CDialog {

	DECLARE_DYNAMIC(DialogEntityDefEditor)

public:
						DialogEntityDefEditor( CWnd* pParent = NULL );   // standard constructor
	virtual				~DialogEntityDefEditor();

	void				LoadDecl( idDeclEntityDef *decl );

	//{{AFX_VIRTUAL(DialogEntityDefEditor)
	virtual BOOL		OnInitDialog();
	virtual void		DoDataExchange( CDataExchange* pDX );    // DDX/DDV support
	virtual BOOL		PreTranslateMessage( MSG* pMsg );
	//}}AFX_VIRTUAL

protected:
	//{{AFX_MSG(DialogEntityDefEditor)
	afx_msg BOOL		OnToolTipNotify( UINT id, NMHDR *pNMHDR, LRESULT *pResult );
	afx_msg void		OnSetFocus( CWnd *pOldWnd );
	afx_msg void		OnDestroy();
	afx_msg void		OnActivate( UINT nState, CWnd* pWndOther, BOOL bMinimized );
	afx_msg void		OnMove( int x, int y );
	afx_msg void		OnSize( UINT nType, int cx, int cy );
	afx_msg void		OnSizing( UINT nSide, LPRECT lpRect );
    afx_msg LRESULT		OnFindDialogMessage( WPARAM wParam, LPARAM lParam );
	afx_msg void		OnEditChange();
	afx_msg void		OnInheritChange();
	afx_msg void		OnEnInputEdit( NMHDR *pNMHDR, LRESULT *pResult );

	afx_msg void		OnKeyValChange();

	afx_msg void		OnBnClickedAdd();
	afx_msg void		OnBnClickedDelete();

	afx_msg void		OnBnClickedTest();
	afx_msg void		OnBnClickedOk();
	afx_msg void		OnBnClickedCancel();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()

private:

	//{{AFX_DATA(DialogEntityDefEditor)
	enum				{ IDD = IDD_DIALOG_ENTITYEDITOR };
	CStatusBarCtrl		statusBar;
	CEdit				declNameEdit;
	CComboBox			inheritCombo;
	CComboBox			spawnclassCombo;

	CPropertyList		keyValsList;

	CStatic				keyLabel;
	CEdit				keyEdit;
	CButton				addButton;
	CButton				delButton;
	CStatic				line;

	CButton				testButton;
	CButton				okButton;
	CButton				cancelButton;
	//}}AFX_DATA

	static toolTip_t	toolTips[];

	HACCEL				m_hAccel;
	CRect				initialRect;
	idDeclEntityDef *	decl;
	int					firstLine;

private:
	void				PopulateLists(idStr &declText);
	void				SetInherit(idStr &inherit);
	void				BuildDeclText(idStr &declText);
	bool				TestDecl( const idStr &declText );
	void				UpdateStatusBar( void );
};

#endif /* !__DIALOGENTITYDEFEDITOR_H__ */
