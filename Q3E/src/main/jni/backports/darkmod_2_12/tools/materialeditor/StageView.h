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
#include <afxole.h>

#include "MaterialEditor.h"
#include "ToggleListView.h"
#include "MaterialView.h"
#include "MaterialPropTreeView.h"

/**
* View that handles managing the material stages.
*/
class StageView : public ToggleListView, public MaterialView
{

public:
	virtual ~StageView();

	/** 
	* Defines the type of stages
	*/
	enum {
		STAGE_TYPE_MATERIAL,
		STAGE_TYPE_STAGE,
		STAGE_TYPE_SPECIAL_MAP_STAGE
	};

	//Associates a property view with this stage view
	void					SetMaterialPropertyView(MaterialPropTreeView* propView) { m_propView = propView; };

	//MaterialView Interface
	virtual void			MV_OnMaterialSelectionChange(MaterialDoc* pMaterial);
	virtual void			MV_OnMaterialStageAdd(MaterialDoc* pMaterial, int stageNum);
	virtual void			MV_OnMaterialStageDelete(MaterialDoc* pMaterial, int stageNum);
	virtual void			MV_OnMaterialStageMove(MaterialDoc* pMaterial, int from, int to);
	virtual void			MV_OnMaterialAttributeChanged(MaterialDoc* pMaterial, int stage, const char* attribName);
	virtual void			MV_OnMaterialSaved(MaterialDoc* pMaterial);

	//Edit Operation Tests
	bool					CanCopy();
	bool					CanPaste();
	bool					CanCut();
	bool					CanDelete();
	bool					CanRename();

	//Refresh the stage list
	void					RefreshStageList();

protected:
	StageView();
	DECLARE_DYNCREATE(StageView)

	afx_msg int				OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void 			OnLvnItemchanged(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void 			OnLvnDeleteallitems(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void 			OnLvnBegindrag(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void 			OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void 			OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void 			OnNMRclick(NMHDR *pNMHDR, LRESULT *pResult);

	afx_msg void 			OnRenameStage();
	afx_msg void 			OnDeleteStage();
	afx_msg void 			OnDeleteAllStages();
	afx_msg void 			OnAddStage();
	afx_msg void 			OnAddBumpmapStage();
	afx_msg void 			OnAddDiffuseStage();
	afx_msg void 			OnAddSpecualarStage();

	afx_msg void 			OnCopy();
	afx_msg void 			OnPaste();
	
	afx_msg void 			OnLvnBeginlabeledit(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void 			OnLvnEndlabeledit(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void 			OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	DECLARE_MESSAGE_MAP()
	
	//Overrides
	virtual BOOL			PreTranslateMessage(MSG* pMsg);
	virtual BOOL			PreCreateWindow(CREATESTRUCT& cs);

	//Toggle List View Interface
	virtual void			OnStateChanged(int index, int toggleState);

	void					PopupMenu(CPoint* pt);

	void					DropItemOnList();

protected:

	MaterialPropTreeView*	m_propView;
	MaterialDoc*			currentMaterial;
	
	//Manual handing of the row dragging
	CImageList*				dragImage;
	bool					bDragging;
	int						dragIndex;
	int						dropIndex;
	CWnd*					dropWnd;
	CPoint					dropPoint;

	bool					internalChange;
};


