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

#include "../common/PropTree/PropTreeView.h"
#include "MaterialView.h"

#include "../common/registryoptions.h"
#include "MaterialDef.h"


/**
* View that displays material and stage properties and allows the user to edit the properties.
*/
class MaterialPropTreeView : public CPropTreeView, public MaterialView {
	
public:
	virtual				~MaterialPropTreeView();
	
	void				SetPropertyListType(int listType, int stageNum = -1);

	void				LoadSettings();
	void				SaveSettings();

	//Material Interface
	virtual void		MV_OnMaterialChange(MaterialDoc* pMaterial);
	
protected:
	MaterialPropTreeView();
	DECLARE_DYNCREATE(MaterialPropTreeView)

	afx_msg void		OnPropertyChangeNotification( NMHDR *nmhdr, LRESULT *lresult );
	afx_msg void		OnPropertyItemExpanding( NMHDR *nmhdr, LRESULT *lresult );
	DECLARE_MESSAGE_MAP()
	
	MaterialDef*		FindDefForTreeID(UINT treeID);
	void				RefreshProperties();
	
protected:
	
	MaterialDoc*		currentMaterial;
	int					currentListType;
	int					currentStage;
	MaterialDefList*	currentPropDefs;
	rvRegistryOptions	registry;
	bool				internalChange;
};


