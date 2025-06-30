/* Copyright (c) 2002-2012 Croteam Ltd. 
This program is free software; you can redistribute it and/or modify
it under the terms of version 2 of the GNU General Public License as published by
the Free Software Foundation


This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA. */

// SeriousSkaStudioView.h : interface of the CSeriousSkaStudioView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_SERIOUSSKASTUDIOVIEW_H__49B41950_735D_4A07_AAF6_29D528FBF35C__INCLUDED_)
#define AFX_SERIOUSSKASTUDIOVIEW_H__49B41950_735D_4A07_AAF6_29D528FBF35C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ChildFrm.h"

class CSeriousSkaStudioView : public CView
{
protected: // create from serialization only
	CSeriousSkaStudioView();
	DECLARE_DYNCREATE(CSeriousSkaStudioView)

// Attributes
public:
	CSeriousSkaStudioDoc* GetDocument();

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSeriousSkaStudioView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnInitialUpdate();
	protected:
	virtual void OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CSeriousSkaStudioView();
  void RenderView(CDrawPort *pdp);
  void RenderLightModels(CDrawPort *pdp,CPlacement3D &pl);
  INDEX TestRayCastHit(CPoint &pt);
  void SetProjectionData( CPerspectiveProjection3D &prProjection, CDrawPort *pDP);
  void FastZoomIn();
  void FastZoomOut();
  void AddTexture(CTFileName &fnFull);
  void BrowseTexture(CTString strTextureDir);


public:
	void OnIdle(void);
  CDrawPort *m_pdpDrawPort;
  CViewPort *m_pvpViewPort;
  CPoint m_pntLastMouse;
  FLOAT m_fFOV;
  ANGLE3D m_angViewerOrientation;
  FLOAT   m_fTargetDistance;
  FLOAT3D m_vTarget;
  ANGLE3D m_angModelAngle;
  FLOAT   m_fLightDistance;
  CPlacement3D m_plLightPlacement;

  INDEX   m_iViewSize;
  
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
private:
	//{{AFX_MSG(CSeriousSkaStudioView)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDestroy();
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnResetView();
	afx_msg void OnShowWireframe();
	afx_msg void OnShowSkeleton();
	afx_msg void OnShowTexture();
	afx_msg void OnAddMeshlod();
	afx_msg void OnAddAnimation();
	afx_msg void OnAddSkeletonlod();
	afx_msg void OnDeleteselected();
	afx_msg void OnAddAnimset();
	afx_msg BOOL OnAddMeshlist();
	afx_msg void OnAddSkeletonlist();
	afx_msg void OnAddTexture();
	afx_msg void OnAddChildModelInstance();
	afx_msg void OnAddColisionbox();
	afx_msg void OnAnimStop();
	afx_msg void OnAnimSync();
	afx_msg void OnAutoMiping();
	afx_msg void OnUpdateAutoMiping(CCmdUI* pCmdUI);
	afx_msg void OnShowGround();
	afx_msg void OnUpdateShowGround(CCmdUI* pCmdUI);
	afx_msg void OnUpdateShowSkeleton(CCmdUI* pCmdUI);
	afx_msg void OnUpdateShowTexture(CCmdUI* pCmdUI);
	afx_msg void OnUpdateShowWireframe(CCmdUI* pCmdUI);
	afx_msg void OnShowAnimQueue();
	afx_msg void OnUpdateShowAnimQueue(CCmdUI* pCmdUI);
	afx_msg void OnFileSaveModel();
	afx_msg void OnShowNormals();
	afx_msg void OnUpdateShowNormals(CCmdUI* pCmdUI);
	afx_msg void OnShowLights();
	afx_msg void OnUpdateShowLights(CCmdUI* pCmdUI);
	afx_msg void OnChangeAmbientcolor();
	afx_msg void OnChangeLightcolor();
	afx_msg void OnAnimLoop();
	afx_msg void OnUpdateAnimLoop(CCmdUI* pCmdUI);
	afx_msg void OnAnimPause();
	afx_msg void OnUpdateAnimPause(CCmdUI* pCmdUI);
	afx_msg void OnShowColision();
	afx_msg void OnUpdateShowColision(CCmdUI* pCmdUI);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnFileSavemiAs();
	afx_msg void OnFileRecreatetexture();
	afx_msg void OnUpdateFileRecreatetexture(CCmdUI* pCmdUI);
	afx_msg void OnVkDown();
	afx_msg void OnVkUp();
	afx_msg void OnVkEscape();
	afx_msg void OnCreateAddTexture();
	afx_msg void OnAddTextureBump();
	afx_msg void OnAddTextureReflection();
	afx_msg void OnAddTextureSpecular();
	afx_msg void OnShowActiveSkeleton();
	afx_msg void OnUpdateShowActiveSkeleton(CCmdUI* pCmdUI);
	afx_msg void OnShowAllFramesBbox();
	afx_msg void OnUpdateShowAllFramesBbox(CCmdUI* pCmdUI);
	afx_msg void OnModelinstanceSavewithoffset();
	afx_msg void OnVkLeft();
	afx_msg void OnVkRight();
	afx_msg void OnVkLeftWithCtrl();
	afx_msg void OnVkRightWithCtrl();
	afx_msg void OnConvertSelected();
	afx_msg void OnResetColisionbox();
	afx_msg void OnAllFramesRecalc();
	afx_msg void OnReloadTexture();
	afx_msg void OnRecreateTexture();
	afx_msg void OnBrowseTexture();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in SeriousSkaStudioView.cpp
inline CSeriousSkaStudioDoc* CSeriousSkaStudioView::GetDocument()
   { return (CSeriousSkaStudioDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SERIOUSSKASTUDIOVIEW_H__49B41950_735D_4A07_AAF6_29D528FBF35C__INCLUDED_)
