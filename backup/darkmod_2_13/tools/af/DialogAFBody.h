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

// DialogAFBody dialog

class DialogAFBody : public CDialog {

	DECLARE_DYNAMIC(DialogAFBody)

public:
						DialogAFBody( CWnd* pParent = NULL );   // standard constructor
	virtual				~DialogAFBody();
	void				LoadFile( idDeclAF *af );
	void				SaveFile( void );
	void				LoadBody( const char *name );
	void				SaveBody( void );
	void				UpdateFile( void );

	DialogAFConstraint *constraintDlg;

	enum				{ IDD = IDD_DIALOG_AF_BODY };

protected:
	virtual BOOL		OnInitDialog();
	virtual void		DoDataExchange( CDataExchange* pDX );    // DDX/DDV support
	virtual INT_PTR   OnToolHitTest( CPoint point, TOOLINFO* pTI ) const;
	afx_msg BOOL		OnToolTipNotify( UINT id, NMHDR *pNMHDR, LRESULT *pResult );
	afx_msg void		OnShowWindow( BOOL bShow, UINT nStatus );
	afx_msg void		OnCbnSelchangeComboBodies();
	afx_msg void		OnBnClickedButtonNewbody();
	afx_msg void		OnBnClickedButtonRenamebody();
	afx_msg void		OnBnClickedButtonDeletebody();
	afx_msg void		OnCbnSelchangeComboCmType();
	afx_msg void		OnEnChangeEditCmLength();
	afx_msg void		OnDeltaposSpinCmLength(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void		OnEnChangeEditCmHeight();
	afx_msg void		OnDeltaposSpinCmHeight(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void		OnEnChangeEditCmWidth();
	afx_msg void		OnDeltaposSpinCmWidth(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void		OnEnChangeEditCmNumsides();
	afx_msg void		OnDeltaposSpinCmNumsides(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void		OnCbnSelchangeComboBoneJoint1();
	afx_msg void		OnCbnSelchangeComboBoneJoint2();
	afx_msg void		OnEnChangeEditCmDensity();
	afx_msg void		OnDeltaposSpinCmDensity(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void		OnEnChangeEditCmInertiascale();
	afx_msg void		OnBnClickedRadioOriginCoordinates();
	afx_msg void		OnBnClickedRadioOriginBonecenter();
	afx_msg void		OnBnClickedRadioOriginJoint();
	afx_msg void		OnEnChangeEditAfVectorX();
	afx_msg void		OnDeltaposSpinAfVectorX(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void		OnEnChangeEditAfVectorY();
	afx_msg void		OnDeltaposSpinAfVectorY(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void		OnEnChangeEditAfVectorZ();
	afx_msg void		OnDeltaposSpinAfVectorZ(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void		OnOnCbnSelchangeComboOriginBoneCenterJoint1();
	afx_msg void		OnOnCbnSelchangeComboOriginBoneCenterJoint2();
	afx_msg void		OnOnCbnSelchangeComboOriginJoint();
	afx_msg void		OnEnChangeEditAnglesPitch();
	afx_msg void		OnDeltaposSpinAnglesPitch(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void		OnEnChangeEditAnglesYaw();
	afx_msg void		OnDeltaposSpinAnglesYaw(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void		OnEnChangeEditAnglesRoll();
	afx_msg void		OnDeltaposSpinAnglesRoll(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void		OnBnClickedCheckSelfcollision();
	afx_msg void		OnEnChangeEditContents();
	afx_msg void		OnEnChangeEditClipmask();
	afx_msg void		OnEnChangeEditLinearfriction();
	afx_msg void		OnDeltaposSpinLinearfriction(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void		OnEnChangeEditAngularfriction();
	afx_msg void		OnDeltaposSpinAngularfriction(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void		OnEnChangeEditContactfriction();
	afx_msg void		OnDeltaposSpinContactfriction(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void		OnEnChangeEditFrictionDirection();
	afx_msg void		OnEnChangeEditContactMotorDirection();
	afx_msg void		OnCbnSelchangeComboModifiedjoint();
	afx_msg void		OnBnClickedRadioModifyOrientation();
	afx_msg void		OnBnClickedRadioModifyPosition();
	afx_msg void		OnBnClickedRadioModifyBoth();
	afx_msg void		OnEnChangeEditContainedjoints();

	DECLARE_MESSAGE_MAP()

private:
	idDeclAF *			file;
	idDeclAF_Body *		body;
	int					numJoints;

	//{{AFX_DATA(DialogAFBody)
	CComboBox			bodyList;				// list with bodies
	CComboBox			cm_comboType;
	float				cm_length;
	float				cm_height;
	float				cm_width;
	CComboBox			cm_comboBoneJoint1;
	CComboBox			cm_comboBoneJoint2;
	float				cm_numSides;
	float				cm_density;
	CEdit				cm_inertiaScale;
	float				cm_origin_x;
	float				cm_origin_y;
	float				cm_origin_z;
	CComboBox			cm_originBoneCenterJoint1;
	CComboBox			cm_originBoneCenterJoint2;
	CComboBox			cm_originJoint;
	float				cm_angles_pitch;
	float				cm_angles_yaw;
	float				cm_angles_roll;
	BOOL				m_selfCollision;
	CEdit				m_editContents;
	CEdit				m_editClipMask;
	float				m_linearFriction;
	float				m_angularFriction;
	float				m_contactFriction;
	CEdit				m_frictionDirection;
	CEdit				m_contactMotorDirection;
	CComboBox			m_comboModifiedJoint;
	CEdit				m_editContainedJoints;
	//}}AFX_DATA

	static toolTip_t	toolTips[];

private:
	void				InitBodyList( void );
	void				InitJointLists( void );
	void				InitCollisionModelType( void );
	void				InitModifiedJointList( void );
	void				InitNewRenameDeleteButtons( void );
	void				ValidateCollisionModelLength( bool update );
	void				ValidateCollisionModelHeight( bool update );
	void				ValidateCollisionModelWidth( bool update );
	void				ValidateCollisionModelNumSides( bool update );
	void				ValidateCollisionModelDensity( bool update );
};
