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

#include <afxcview.h>
#include "MaterialEditor.h"
#include "MaterialTreeView.h"
#include "MaterialPropTreeView.h"
#include "MaterialPreviewView.h"
#include "StageView.h"
#include "MaterialPreviewPropView.h"
#include "MEOptions.h"
#include "ConsoleView.h"
#include "FindDialog.h"
#include "../common/PropTree/PropTreeView.h"
#include "MaterialDocManager.h"
#include "MaterialEditView.h"

/**
* The main window for the material editor.
*/
class MEMainFrame : public CFrameWnd, public MaterialView
{
	
public:
	MEMainFrame();
	virtual ~MEMainFrame();

	//Public Operations
	void						PrintConsoleMessage(const char *msg);

protected: 
	DECLARE_DYNAMIC(MEMainFrame)

	// Overrides
	virtual BOOL 				PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL 				OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);

	//Message Handlers
	afx_msg int					OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void 				OnDestroy();
	afx_msg void 				OnSize(UINT nType, int cx, int cy);
	afx_msg void 				OnTcnSelChange(NMHDR *pNMHDR, LRESULT *pResult);

	//Menu Message Handlers
	afx_msg void 				OnFileExit();
	afx_msg void 				OnFileSaveMaterial();
	afx_msg void 				OnFileSaveFile();
	afx_msg void 				OnFileSaveAll();
	afx_msg void 				OnFileSaveMaterialUpdate(CCmdUI *pCmdUI);
	afx_msg void 				OnFileSaveFileUpdate(CCmdUI *pCmdUI);
	afx_msg void 				OnFileSaveAllUpdate(CCmdUI *pCmdUI);

	afx_msg void 				OnApplyMaterial();
	afx_msg void 				OnApplyFile();
	afx_msg void 				OnApplyAll();
	afx_msg void 				OnApplyMaterialUpdate(CCmdUI *pCmdUI);
	afx_msg void 				OnApplyFileUpdate(CCmdUI *pCmdUI);
	afx_msg void 				OnApplyAllUpdate(CCmdUI *pCmdUI);

	afx_msg void 				OnEditCut();
	afx_msg void 				OnEditCopy();
	afx_msg void 				OnEditPaste();
	afx_msg void 				OnEditDelete();
	afx_msg void 				OnEditRename();
	afx_msg void 				OnEditCutUpdate(CCmdUI *pCmdUI);
	afx_msg void 				OnEditCopyUpdate(CCmdUI *pCmdUI);
	afx_msg void 				OnEditPasteUpdate(CCmdUI *pCmdUI);
	afx_msg void 				OnEditDeleteUpdate(CCmdUI *pCmdUI);
	afx_msg void 				OnEditRenameUpdate(CCmdUI *pCmdUI);

	afx_msg void 				OnEditFind();
	afx_msg void 				OnEditFindNext();

	afx_msg void 				OnEditUndo();
	afx_msg void 				OnEditRedo();
	afx_msg void 				OnEditUndoUpdate(CCmdUI *pCmdUI);
	afx_msg void 				OnEditRedoUpdate(CCmdUI *pCmdUI);
	
	afx_msg void 				OnViewIncludeFile();
	afx_msg void 				OnReloadArbPrograms();
	afx_msg void 				OnReloadImages();
	
	DECLARE_MESSAGE_MAP()

	//Methods for Find interactions
	friend						FindDialog;
	void						CloseFind();
	void						FindNext(MaterialSearchData_t* search);

	//MaterialView Interface
	virtual void	MV_OnMaterialSelectionChange(MaterialDoc* pMaterial);
	
protected:
	//Status and Toolbars
	CStatusBar  				m_wndStatusBar;
	CToolBar    				m_wndToolBar;

	//Splitter windows
	CTabCtrl					m_tabs;
	CSplitterWnd				m_splitterWnd;
	CSplitterWnd				m_editSplitter;
	CSplitterWnd				m_previewSplitter;
	CSplitterWnd*				m_materialEditSplitter;

	//Child Views
	MaterialTreeView*			m_materialTreeView;
	StageView*					m_stageView;
	MaterialPropTreeView*		m_materialPropertyView;
	MaterialPreviewView*		m_materialPreviewView;
	MaterialPreviewPropView*	m_previewPropertyView;
	ConsoleView*				m_consoleView;

	MaterialEditView*			m_materialEditView;

	//Find Data
	FindDialog*					m_find;
	MaterialSearchData_t		searchData;

	//Document Management
	MaterialDocManager			materialDocManager;
	MaterialDoc*				currentDoc;

	//Options
	MEOptions					options;

};


