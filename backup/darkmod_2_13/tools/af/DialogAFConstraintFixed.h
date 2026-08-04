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

// DialogAFConstraintFixed dialog

class DialogAFConstraintFixed : public CDialog {

	DECLARE_DYNAMIC(DialogAFConstraintFixed)

public:
						DialogAFConstraintFixed(CWnd* pParent = NULL);   // standard constructor
	virtual				~DialogAFConstraintFixed();
	void				LoadFile( idDeclAF *af );
	void				SaveFile( void );
	void				LoadConstraint( idDeclAF_Constraint *c );
	void				SaveConstraint( void );
	void				UpdateFile( void );

						enum { IDD = IDD_DIALOG_AF_CONSTRAINT_FIXED };

protected:
	virtual void		DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual INT_PTR   OnToolHitTest( CPoint point, TOOLINFO* pTI ) const;
	afx_msg BOOL		OnToolTipNotify( UINT id, NMHDR *pNMHDR, LRESULT *pResult );

	DECLARE_MESSAGE_MAP()

private:
	idDeclAF *			file;
	idDeclAF_Constraint*constraint;

	//{{AFX_DATA(DialogAFConstraintHinge)
	//}}AFX_DATA

	static toolTip_t	toolTips[];

private:
	void				InitJointLists( void );
};
