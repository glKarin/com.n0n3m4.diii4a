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

// DialogAFView dialog

class DialogAFView : public CDialog {

	DECLARE_DYNAMIC(DialogAFView)

public:
						DialogAFView(CWnd* pParent = NULL);   // standard constructor
	virtual				~DialogAFView();

	enum				{ IDD = IDD_DIALOG_AF_VIEW };

protected:
	virtual void		DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual INT_PTR   OnToolHitTest( CPoint point, TOOLINFO* pTI ) const;
	afx_msg BOOL		OnToolTipNotify( UINT id, NMHDR *pNMHDR, LRESULT *pResult );
	afx_msg void		OnBnClickedCheckViewBodies();
	afx_msg void		OnBnClickedCheckViewBodynames();
	afx_msg void		OnBnClickedCheckViewBodyMass();
	afx_msg void		OnBnClickedCheckViewTotalMass();
	afx_msg void		OnBnClickedCheckViewInertiatensor();
	afx_msg void		OnBnClickedCheckViewVelocity();
	afx_msg void		OnBnClickedCheckViewConstraints();
	afx_msg void		OnBnClickedCheckViewConstraintnames();
	afx_msg void		OnBnClickedCheckViewPrimaryonly();
	afx_msg void		OnBnClickedCheckViewLimits();
	afx_msg void		OnBnClickedCheckViewConstrainedBodies();
	afx_msg void		OnBnClickedCheckViewTrees();
	afx_msg void		OnBnClickedCheckMd5Skeleton();
	afx_msg void		OnBnClickedCheckMd5Skeletononly();
	afx_msg void		OnBnClickedCheckLinesDepthtest();
	afx_msg void		OnBnClickedCheckLinesUsearrows();
	afx_msg void		OnBnClickedCheckPhysicsNofriction();
	afx_msg void		OnBnClickedCheckPhysicsNolimits();
	afx_msg void		OnBnClickedCheckPhysicsNogravity();
	afx_msg void		OnBnClickedCheckPhysicsNoselfcollision();
	afx_msg void		OnBnClickedCheckPhysicsTiming();
	afx_msg void		OnBnClickedCheckPhysicsDragEntities();
	afx_msg void		OnBnClickedCheckPhysicsShowDragSelection();

	DECLARE_MESSAGE_MAP()

private:
	//{{AFX_DATA(DialogAFView)
	BOOL				m_showBodies;
	BOOL				m_showBodyNames;
	BOOL				m_showMass;
	BOOL				m_showTotalMass;
	BOOL				m_showInertia;
	BOOL				m_showVelocity;
	BOOL				m_showConstraints;
	BOOL				m_showConstraintNames;
	BOOL				m_showPrimaryOnly;
	BOOL				m_showLimits;
	BOOL				m_showConstrainedBodies;
	BOOL				m_showTrees;
	BOOL				m_showSkeleton;
	BOOL				m_showSkeletonOnly;
	BOOL				m_debugLineDepthTest;
	BOOL				m_debugLineUseArrows;
	BOOL				m_noFriction;
	BOOL				m_noLimits;
	BOOL				m_noGravity;
	BOOL				m_noSelfCollision;
	BOOL				m_showTimings;
	BOOL				m_dragEntity;
	BOOL				m_dragShowSelection;
	//}}AFX_DATA

	float				m_gravity;

	static toolTip_t	toolTips[];
};
