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
#pragma once

// DialogAFName dialog

class DialogAFName : public CDialog {

	DECLARE_DYNAMIC(DialogAFName)

public:
						DialogAFName(CWnd* pParent = NULL);   // standard constructor
	virtual				~DialogAFName();
	void				SetName( CString &str );
	void				GetName( CString &str );
	void				SetComboBox( CComboBox *combo );

						enum { IDD = IDD_DIALOG_AF_NAME };

protected:
	virtual BOOL		OnInitDialog();
	virtual void		DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	afx_msg void		OnBnClickedOk();
	afx_msg void		OnEnChangeEditAfName();

	DECLARE_MESSAGE_MAP()

private:
	CString				m_editName;
	CComboBox *			m_combo;
};
