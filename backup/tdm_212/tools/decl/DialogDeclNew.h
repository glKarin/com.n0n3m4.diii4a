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

#ifndef __DIALOGDECLNEW_H__
#define __DIALOGDECLNEW_H__

#pragma once


// DialogDeclNew dialog

class DialogDeclNew : public CDialog {

	DECLARE_DYNAMIC(DialogDeclNew)

public:
						DialogDeclNew( CWnd* pParent = NULL );   // standard constructor
	virtual				~DialogDeclNew();

	void				SetDeclTree( CPathTreeCtrl *tree ) { declTree = tree; }
	void				SetDefaultType( const char *type ) { defaultType = type; }
	void				SetDefaultName( const char *name ) { defaultName = name; }
	void				SetDefaultFile( const char *file ) { defaultFile = file; }
	idDecl *			GetNewDecl( void ) const { return newDecl; }

	//{{AFX_VIRTUAL(DialogDeclNew)
	virtual BOOL		OnInitDialog();
	virtual void		DoDataExchange( CDataExchange* pDX );    // DDX/DDV support
	//}}AFX_VIRTUAL

protected:
	//{{AFX_MSG(DialogDeclNew)
	afx_msg BOOL		OnToolTipNotify( UINT id, NMHDR *pNMHDR, LRESULT *pResult );
	afx_msg void		OnSetFocus( CWnd *pOldWnd );
	afx_msg void		OnDestroy();
	afx_msg void		OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
	afx_msg void		OnBnClickedFile();
	afx_msg void		OnBnClickedOk();
	afx_msg void		OnBnClickedCancel();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()

private:

	//{{AFX_DATA(DialogDeclNew)
	enum				{ IDD = IDD_DIALOG_DECLNEW };
	CComboBox			typeList;
	CEdit				nameEdit;
	CEdit				fileEdit;
	CButton				fileButton;
	CButton				okButton;
	CButton				cancelButton;
	//}}AFX_DATA

	static toolTip_t	toolTips[];

	CPathTreeCtrl *		declTree;
	idStr				defaultType;
	idStr				defaultName;
	idStr				defaultFile;
	idDecl *			newDecl;

private:
	void				InitTypeList( void );
};

#endif /* !__DIALOGDECLNEW_H__ */
