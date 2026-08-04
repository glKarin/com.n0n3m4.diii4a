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

// DialogAFProperties dialog

class DialogAFProperties : public CDialog {

	DECLARE_DYNAMIC(DialogAFProperties)

public:
						DialogAFProperties( CWnd* pParent = NULL );   // standard constructor
	virtual				~DialogAFProperties();
	void				LoadFile( idDeclAF *af );
	void				SaveFile( void );

	DialogAFBody *		bodyDlg;
	DialogAFConstraint *constraintDlg;

	enum				{ IDD = IDD_DIALOG_AF_PROPERTIES };

protected:
	virtual void		DoDataExchange( CDataExchange* pDX );    // DDX/DDV support
	virtual INT_PTR   OnToolHitTest( CPoint point, TOOLINFO* pTI ) const;
	afx_msg BOOL		OnToolTipNotify( UINT id, NMHDR *pNMHDR, LRESULT *pResult );
	afx_msg void		OnEnChangeEditModel();
	afx_msg void		OnEnChangeEditSkin();
	afx_msg void		OnBnClickedButtonBrowseModel();
	afx_msg void		OnBnClickedButtonBrowseSkin();
	afx_msg void		OnBnClickedCheckSelfcollision();
	afx_msg void		OnEnChangeEditContents();
	afx_msg void		OnEnChangeEditClipmask();
	afx_msg void		OnEnChangeEditLinearfriction();
	afx_msg void		OnDeltaposSpinLinearfriction(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void		OnEnChangeEditAngularfriction();
	afx_msg void		OnDeltaposSpinAngularfriction(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void		OnEnChangeEditContactfriction();
	afx_msg void		OnDeltaposSpinContactfriction(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void		OnEnChangeEditConstraintfriction();
	afx_msg void		OnDeltaposSpinConstraintfriction(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void		OnEnChangeEditTotalmass();
	afx_msg void		OnDeltaposSpinTotalmass(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void		OnEnChangeEditLinearvelocity();
	afx_msg void		OnEnChangeEditAngularvelocity();
	afx_msg void		OnEnChangeEditLinearacceleration();
	afx_msg void		OnEnChangeEditAngularacceleration();
	afx_msg void		OnEnChangeEditNomovetime();
	afx_msg void		OnEnChangeEditMinimummovetime();
	afx_msg void		OnEnChangeEditMaximummovetime();
	afx_msg void		OnEnChangeEditLineartolerance();
	afx_msg void		OnEnChangeEditAngulartolerance();

	DECLARE_MESSAGE_MAP()

private:
	idDeclAF *			file;

	//{{AFX_DATA(DialogAFProperties)
	CEdit				m_editModel;
	CEdit				m_editSkin;
	BOOL				m_selfCollision;
	CEdit				m_editContents;
	CEdit				m_editClipMask;
	float				m_linearFriction;
	float				m_angularFriction;
	float				m_contactFriction;
	float				m_constraintFriction;
	float				m_totalMass;
	float				m_suspendLinearVelocity;
	float				m_suspendAngularVelocity;
	float				m_suspendLinearAcceleration;
	float				m_suspendAngularAcceleration;
	float				m_noMoveTime;
	float				m_minMoveTime;
	float				m_maxMoveTime;
	float				m_linearTolerance;
	float				m_angularTolerance;
	//}}AFX_DATA

	static toolTip_t	toolTips[];

private:
	void				UpdateFile( void );
	void				ClearFile( void );
};
