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

// SeriousSkaStudio.h : main header file for the SERIOUSSKASTUDIO application
//

#if !defined(AFX_SERIOUSSKASTUDIO_H__0D873E27_F3AF_4EC5_B1AE_B0F330DB5848__INCLUDED_)
#define AFX_SERIOUSSKASTUDIO_H__0D873E27_F3AF_4EC5_B1AE_B0F330DB5848__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols

#include "DlgClient.h"
#include "SeriousSkaStudioDoc.h"
#include "SeriousSkaStudioView.h"
#include "DlgBarTreeView.h"

/////////////////////////////////////////////////////////////////////////////
// CSeriousSkaStudioApp:
// See SeriousSkaStudio.cpp for the implementation of this class
//
#define FILTER_ASCII         "ASCII files (*.aal,*.asl,*.aml)\0*.aal;*.aml;*.asl\0"
#define FILTER_MESH          "Mesh ASCII files (*.am)\0*.am;\0"
#define FILTER_SKELETON      "Skeleton ASCII files (*.as)\0*.as;\0"
#define FILTER_ANIMATION     "Animation ASCII files (*.aa)\0*.aa;\0"
#define FILTER_TEXTURE       "Texture files (*.tex)\0*.tex;\0"
#define FILTER_MESH_LIST     "Mesh list files (*.aml)\0*.aml;\0"
#define FILTER_SKELETON_LIST "Skeleton list files (*.asl)\0*.asl;\0"
#define FILTER_ANIMSET_LISTS "AnimSet list files (*.aal)\0*.aal;\0"

#define STRETCH_BUTTON_INDEX 22

#define NT_MODELINSTANCE    0
#define NT_MESHLODLIST      1
#define NT_MESHLOD          2
#define NT_TEXINSTANCE      3
#define NT_SKELETONLODLIST  4
#define NT_SKELETONLOD      5
#define NT_BONE             6
#define NT_ANIMSET          7
#define NT_ANIMATION        8
#define NT_ANIM_BONEENV     9
#define NT_COLISIONBOX     10
#define NT_ALLFRAMESBBOX   11
#define NT_MESHSURFACE     12

class CSeriousSkaStudioApp : public CWinApp
{
public:
  CMultiDocTemplate* m_pdtDocTemplate;
	CSeriousSkaStudioApp();
  BOOL SubInitInstance();
  CSeriousSkaStudioDoc *GetDocument();
  CSeriousSkaStudioView *GetActiveView(void);
  void NotificationMessage(const char *strFormat, ...);
  void ErrorMessage(const char *strFormat, ...);

  void ReloadRootModelInstance();
  void UpdateRootModelInstance();
  void ReselectCurrentItem();
  void DisableRendering();
  void EnableRendering();
  INDEX GetDisableRenderRequests();

  CModelInstance *OnAddNewModelInstance();
  CModelInstance *OnOpenExistingInstance(CTString strModelInstance);
  void AddEmptyListsToModelInstance(CModelInstance &mi);

  // save root model instance with children and all binary files
  void SaveRootModel();
  // save as different smc file
  void SaveRootModelAs();
  // save model instance with children and all binary files
  void SaveModel(CModelInstance &mi);
  BOOL SaveModelAs(CModelInstance *pmi);

  void SaveSmcFile(CModelInstance &mi,BOOL bSaveChildren);
  BOOL SaveMeshListFile(MeshInstance &mshi, BOOL bConvert);
  BOOL SaveSkeletonListFile(CSkeleton &skl, BOOL bConvert);
  BOOL SaveAnimSetFile(CAnimSet &as, BOOL bConvert);

  void SaveShaderParams_t(MeshLOD *pmlod,CTFileName fnShaderParams);
  void SaveModelInstance_t(CModelInstance *pmi,CModelInstance *pmiParent,CTFileStream &ostrFile,BOOL bSaveChildren);
  void SaveMeshInstance_t(MeshInstance &mshi,CTFileStream &ostrFile);
  void SaveSkeletonList_t(CSkeleton &skl,CTFileStream &ostrFile);
  void SaveAnimSet_t(CAnimSet &as,CTFileStream &ostrFile);
  BOOL ConvertAnimationInAnimSet(CAnimSet *pas,Animation *pan);
  
  BOOL ConvertMesh(CTFileName fnMesh);
  BOOL ConvertSkeleton(CTFileName fnSkeleton);
  BOOL ConvertAnimSet(CTFileName fnAnimSet);

  // get pointer to error listctrl
  CListCtrl *GetErrorList();
  BOOL IsErrorDlgVisible();
  SIZE GetLogDlgSize();
  INDEX GetErrorDlgWidth();
  void ShowErrorDlg(BOOL bShow);

  // dockable dialog that with tree
  CDlgBarTreeView m_dlgBarTreeView;
//  CDlgTemplate m_dlgErrorList;
  CDialogBar m_dlgErrorList;
  

  BOOL bAppInitialized;
  // array that holds info about every node in tree view
  CStaticStackArray<struct NodeInfo> aNodeInfo;

  // currently selected bone
  INDEX iSelectedBoneID;
  // is flag for showing colision boxes tured on
  BOOL bShowAnimQueue;
  CTextureObject toGroundTexture;

  // if a child is maximized only it is draw
  BOOL bChildrenMaximized;

  CModelInstance *pmiLight;

private:
  INDEX ctNoRenderRequests;

  
  // Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSeriousSkaStudioApp)
	public:
	virtual BOOL InitInstance();
	virtual BOOL OnIdle(LONG lCount);
  virtual int Run();
	virtual int ExitInstance();
	//}}AFX_VIRTUAL

// Implementation
	//{{AFX_MSG(CSeriousSkaStudioApp)
	afx_msg void OnAppAbout();
	afx_msg void OnFileOpen();
  afx_msg void OnFileNew();
	afx_msg void OnImportConvert();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

extern CSeriousSkaStudioApp theApp;
extern BOOL StartParser(CTString fnParseFile);

// selected model instance in tree view
extern CModelInstance *pmiSelected;


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SERIOUSSKASTUDIO_H__0D873E27_F3AF_4EC5_B1AE_B0F330DB5848__INCLUDED_)
