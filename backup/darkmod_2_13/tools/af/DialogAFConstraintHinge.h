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

// DialogAFConstraintHinge dialog

class DialogAFConstraintHinge : public CDialog {

	DECLARE_DYNAMIC(DialogAFConstraintHinge)

public:
						DialogAFConstraintHinge(CWnd* pParent = NULL);   // standard constructor
	virtual				~DialogAFConstraintHinge();
	void				LoadFile( idDeclAF *af );
	void				SaveFile( void );
	void				LoadConstraint( idDeclAF_Constraint *c );
	void				SaveConstraint( void );
	void				UpdateFile( void );

						enum { IDD = IDD_DIALOG_AF_CONSTRAINT_HINGE };

protected:
	virtual void		DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual INT_PTR   OnToolHitTest( CPoint point, TOOLINFO* pTI ) const;
	afx_msg BOOL		OnToolTipNotify( UINT id, NMHDR *pNMHDR, LRESULT *pResult );
	afx_msg void		OnBnClickedRadioAnchorJoint();
	afx_msg void		OnBnClickedRadioAnchorCoordinates();
	afx_msg void		OnCbnSelchangeComboAnchorJoint();
	afx_msg void		OnEnChangeEditAnchorX();
	afx_msg void		OnEnChangeEditAnchorY();
	afx_msg void		OnEnChangeEditAnchorZ();
	afx_msg void		OnDeltaposSpinAnchorX(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void		OnDeltaposSpinAnchorY(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void		OnDeltaposSpinAnchorZ(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void		OnBnClickedRadioHingeAxisBone();
	afx_msg void		OnBnClickedRadioHingeAxisAngles();
	afx_msg void		OnCbnSelchangeComboHingeAxisJoint1();
	afx_msg void		OnCbnSelchangeComboHingeAxisJoint2();
	afx_msg void		OnEnChangeEditHingeAxisPitch();
	afx_msg void		OnDeltaposSpinHingeAxisPitch(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void		OnEnChangeEditHingeAxisYaw();
	afx_msg void		OnDeltaposSpinHingeAxisYaw(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void		OnBnClickedRadioHingeLimitNone();
	afx_msg void		OnBnClickedRadioHingeLimitAngles();
	afx_msg void		OnEnChangeEditHingeLimitAngle1();
	afx_msg void		OnDeltaposSpinHingeLimitAngle1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void		OnEnChangeEditHingeLimitAngle2();
	afx_msg void		OnDeltaposSpinHingeLimitAngle2(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void		OnEnChangeEditHingeLimitAngle3();
	afx_msg void		OnDeltaposSpinHingeLimitAngle3(NMHDR *pNMHDR, LRESULT *pResult);

	DECLARE_MESSAGE_MAP()

private:
	idDeclAF *			file;
	idDeclAF_Constraint*constraint;

	//{{AFX_DATA(DialogAFConstraintHinge)
	CComboBox			m_comboAnchorJoint;
	float				m_anchor_x;
	float				m_anchor_y;
	float				m_anchor_z;
	CComboBox			m_comboAxisJoint1;
	CComboBox			m_comboAxisJoint2;
	float				m_axisPitch;
	float				m_axisYaw;
	float				m_limitAngle1;
	float				m_limitAngle2;
	float				m_limitAngle3;
	//}}AFX_DATA

	static toolTip_t	toolTips[];

private:
	void				InitJointLists( void );
};
