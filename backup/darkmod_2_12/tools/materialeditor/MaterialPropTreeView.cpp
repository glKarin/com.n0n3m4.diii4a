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
#include "precompiled.h"
#pragma hdrstop



#include "MaterialPropTreeView.h"

#if defined(ID_DEBUG_MEMORY) && defined(ID_REDIRECT_NEWDELETE)
#undef new
#undef DEBUG_NEW
#define DEBUG_NEW new
#endif

#define PROP_TREE_VIEW "PropTreeView"


IMPLEMENT_DYNCREATE(MaterialPropTreeView, CPropTreeView)

BEGIN_MESSAGE_MAP(MaterialPropTreeView, CPropTreeView)
	ON_NOTIFY( PTN_ITEMCHANGED, IDC_PROPERTYTREE, OnPropertyChangeNotification )
	ON_NOTIFY( PTN_ITEMEXPANDING, IDC_PROPERTYTREE, OnPropertyItemExpanding )
END_MESSAGE_MAP()


/**
* Constructor for MaterialPropTreeView.
*/
MaterialPropTreeView::MaterialPropTreeView() {
	registry.Init("Software\\id Software\\DOOM3\\Tools\\MaterialEditor\\PropertySettings");
	internalChange = false;
}

/**
* Destructor for MaterialPropTreeView.
*/
MaterialPropTreeView::~MaterialPropTreeView() {
}

/**
* Initializes the list of properties based on the type (material, stage, special stage).
* @param listType The type of list (material, stage, special stage)
* @param stageNum The stage from which to get the attributes.
*/
void MaterialPropTreeView::SetPropertyListType(int listType, int stageNum) {

	currentListType = listType;
	currentStage = stageNum;

	m_Tree.DeleteAllItems();

	//idList<MaterialProp_t*>* propList = NULL;
	MaterialDefList* propList = MaterialDefManager::GetMaterialDefs(currentListType);
	currentPropDefs = propList;

	if(!propList)
		return;

	CPropTreeItem* pCurrentGroup = NULL;
	CPropTreeItem* pCurrentNode = NULL;

	for(int i = 0; i < propList->Num(); i++) {
		switch((*propList)[i]->type) {
			case MaterialDef::MATERIAL_DEF_TYPE_GROUP:
				{
					pCurrentGroup = m_Tree.InsertItem(new CPropTreeItem());
					pCurrentNode = pCurrentGroup;

					if(!registry.GetBool(va("Expand%d%s", currentListType, (*propList)[i]->displayName.c_str())))
						pCurrentGroup->Expand();
				}
				break;
			case MaterialDef::MATERIAL_DEF_TYPE_BOOL:
				{
					CPropTreeItemCheck* pCheck;
					pCheck = (CPropTreeItemCheck*)m_Tree.InsertItem(new CPropTreeItemCheck(), pCurrentGroup);
					pCheck->CreateCheckBox();
					pCurrentNode = pCheck;
				}
				break;
			case MaterialDef::MATERIAL_DEF_TYPE_STRING:
				{
					//pCurrentNode = m_Tree.InsertItem(new CPropTreeItemEdit(), pCurrentGroup);	
					pCurrentNode = m_Tree.InsertItem(new CPropTreeItemFileEdit(), pCurrentGroup);

				}
				break;
		}

		if(pCurrentNode) {
			(*propList)[i]->SetViewData(PROP_TREE_VIEW, pCurrentNode->GetCtrlID());
			pCurrentNode->SetLabelText((*propList)[i]->displayName);
			pCurrentNode->SetInfoText((*propList)[i]->displayInfo);
		}
	}

	RefreshProperties();
}

/**
* Loads the property view settings from the registry.
*/
void MaterialPropTreeView::LoadSettings() {
	registry.Load();
}

/**
* Saves the property view settings to the registry.
*/
void MaterialPropTreeView::SaveSettings() {
	registry.Save();
}

/**
* Called when the material has changed but not applied.
* @param pMaterial The selected material.
*/
void MaterialPropTreeView::MV_OnMaterialChange(MaterialDoc* pMaterial) {
	
	if(materialDocManager->GetCurrentMaterialDoc()) {
		idStr currentName = materialDocManager->GetCurrentMaterialDoc()->name;
		if(!internalChange && !pMaterial->name.Icmp(currentName)) {
			RefreshProperties();
		}
	}
}

/**
* Updated the material when an attribute has been changed.
*/
void MaterialPropTreeView::OnPropertyChangeNotification( NMHDR *nmhdr, LRESULT *lresult ) {

	NMPROPTREE	*nmProp = (NMPROPTREE *)nmhdr;
	CPropTreeItem	*item = nmProp->pItem;

	internalChange = true;

	MaterialDef* propItem = FindDefForTreeID(item->GetCtrlID());
	if(propItem) {
		MaterialDoc* materialDoc = materialDocManager->GetCurrentMaterialDoc();

		switch(propItem->type) {
			case MaterialDef::MATERIAL_DEF_TYPE_BOOL:
				{
					BOOL val = item->GetItemValue();
					materialDoc->SetAttributeBool(currentStage, propItem->dictName, val ? true : false);
				}
				break;
			case MaterialDef::MATERIAL_DEF_TYPE_STRING:
				{
					idStr val = (LPCTSTR)item->GetItemValue();								
					materialDoc->SetAttribute(currentStage, propItem->dictName, val);
				}
				break;
		}
	}

	internalChange = false;

	*lresult = 0;
}

/**
* Changes the property setting of a group when is expanding.
*/
void MaterialPropTreeView::OnPropertyItemExpanding( NMHDR *nmhdr, LRESULT *lresult ) {

	NMPROPTREE	*nmProp = (NMPROPTREE *)nmhdr;
	CPropTreeItem	*item = nmProp->pItem;

	//The item isn't toggled till after this returns so use the opposite of the current state.
	registry.SetBool(va("Expand%d%s", currentListType, item->GetLabelText()), item->IsExpanded() ? true : false);
	registry.Save();

	*lresult = 0;
}

/**
* Returns the MeterialDef for a given property tree item.
* @param treeID The id of the tree item in question.
*/
MaterialDef* MaterialPropTreeView::FindDefForTreeID(UINT treeID) {

	int c = currentPropDefs->Num();
	for(int i = 0; i < c; i++) {
		if((*currentPropDefs)[i]->GetViewData(PROP_TREE_VIEW) == treeID)
			return (*currentPropDefs)[i];
	}

	return NULL;
}

/**
* Initializes the property tree with the data from the currently selected material.
*/
void MaterialPropTreeView::RefreshProperties() {
	
	MaterialDefList* propList = MaterialDefManager::GetMaterialDefs(currentListType);

	if(!propList)
		return;

 	MaterialDoc* materialDoc = materialDocManager->GetCurrentMaterialDoc();

	for(int i = 0; i < propList->Num(); i++) {
		switch((*propList)[i]->type) {
			case MaterialDef::MATERIAL_DEF_TYPE_BOOL:
				{
					bool val = materialDoc->GetAttributeBool(currentStage, (*propList)[i]->dictName);
					CPropTreeItemCheck* item = (CPropTreeItemCheck*)m_Tree.FindItem((*propList)[i]->GetViewData(PROP_TREE_VIEW));
					item->SetCheckState(val ? TRUE:FALSE);
				}
				break;
			case MaterialDef::MATERIAL_DEF_TYPE_STRING:
				{
					idStr val = materialDoc->GetAttribute(currentStage, (*propList)[i]->dictName);
					CPropTreeItemEdit* item = (CPropTreeItemEdit*)m_Tree.FindItem((*propList)[i]->GetViewData(PROP_TREE_VIEW));
					item->SetItemValue((LPARAM)val.c_str());
				}
				break;
		}
	}

	Invalidate();
}





