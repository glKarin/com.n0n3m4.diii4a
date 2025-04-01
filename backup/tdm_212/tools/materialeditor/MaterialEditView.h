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

#include "MaterialEditor.h"
#include "MaterialPropTreeView.h"
#include "StageView.h"

#include "../comafx/CSyntaxRichEditCtrl.h"

/**
* View that contains the material edit controls. These controls include
* the stage view, the properties view and the source view.
*/
class MaterialEditView : public CFormView, public MaterialView, SourceModifyOwner {

public:
	enum{ IDD = IDD_MATERIALEDIT_FORM };

	CEdit						m_nameEdit;
	CSplitterWnd				m_editSplitter;

	StageView*					m_stageView;
	MaterialPropTreeView*		m_materialPropertyView;
	CTabCtrl					m_tabs;
	CSyntaxRichEditCtrl			m_textView;
	
public:
	virtual			~MaterialEditView();
	
	//MaterialView Interface
	virtual void	MV_OnMaterialSelectionChange(MaterialDoc* pMaterial);
	virtual void	MV_OnMaterialNameChanged(MaterialDoc* pMaterial, const char* oldName);

	//SourceModifyOwner Interface
	virtual idStr GetSourceText();
	
protected:
	MaterialEditView();
	DECLARE_DYNCREATE(MaterialEditView)

	void			GetMaterialSource();
	void			ApplyMaterialSource();

	//CFormView Overrides
	virtual void	DoDataExchange(CDataExchange* pDX);
	virtual void	OnInitialUpdate();
	
	//Message Handlers
	afx_msg int		OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void	OnSize(UINT nType, int cx, int cy);
	afx_msg void 	OnTcnSelChange(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void	OnEnChangeEdit( NMHDR *pNMHDR, LRESULT *pResult );
	DECLARE_MESSAGE_MAP()	

protected:
	bool initHack;
	bool sourceInit;

	bool	sourceChanged;
	idStr	currentMaterialName;
};
