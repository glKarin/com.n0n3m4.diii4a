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
#include "../common/PropTree/PropTreeView.h"
#include "MaterialEditor.h"
#include "MaterialView.h"

/**
* Structure used associate a material name with a tree item.
*/
typedef struct {
	idStr		materialName;
	HTREEITEM	treeItem;
} MaterialTreeItem_t;

/**
* A tree view of all the materials that have been defined.
*/
class MaterialTreeView : public CTreeView, public MaterialView {

public:
	virtual			~MaterialTreeView();

	void			InitializeMaterialList(bool includeFile = true, const char* filename = NULL);
	void			BuildMaterialList(bool includeFile = true, const char* filename = NULL);
	
	//Material Interface
	virtual void	MV_OnMaterialChange(MaterialDoc* pMaterial);
	virtual void	MV_OnMaterialApply(MaterialDoc* pMaterial);
	virtual void	MV_OnMaterialSaved(MaterialDoc* pMaterial);
	virtual void	MV_OnMaterialAdd(MaterialDoc* pMaterial);
	virtual void	MV_OnMaterialDelete(MaterialDoc* pMaterial);
	virtual void	MV_OnMaterialNameChanged(MaterialDoc* pMaterial, const char* oldName);
	virtual void	MV_OnFileReload(const char* filename);
	
	bool			CanCopy();
	bool			CanPaste();
	bool			CanCut();
	bool			CanDelete();
	bool			CanRename();
	bool			CanSaveFile();
	idStr			GetSaveFilename();

	bool			FindNextMaterial(MaterialSearchData_t* searchData);
	HTREEITEM		FindNextMaterial(HTREEITEM item, MaterialSearchData_t* searchData);
	HTREEITEM		GetNextSeachItem(HTREEITEM item, bool stayInFile);

	void			DeleteFolder(HTREEITEM item, bool addUndo = true);
	HTREEITEM		AddFolder(const char* name, HTREEITEM parent);
	void			RenameFolder(HTREEITEM item, const char* name);
	

protected:
	MaterialTreeView();
	DECLARE_DYNCREATE(MaterialTreeView)

	/**
	* List of tree item types
	*/
	enum {
		TYPE_ROOT = 0,
		TYPE_FOLDER,
		TYPE_FILE,
		TYPE_MATERIAL_FOLDER,
		TYPE_MATERIAL
	};

	//Overrides
	virtual BOOL	PreTranslateMessage(MSG* pMsg);

	//Window Messages
	afx_msg int		OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void 	OnTvnSelchanged(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void 	OnTvnBeginlabeledit(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void 	OnTvnEndlabeledit(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void 	OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void 	OnNMRclick(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void	OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void 	OnTvnBegindrag(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void 	OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void 	OnLButtonUp(UINT nFlags, CPoint point);

	//Menu Commands
	afx_msg void 	OnApplyMaterial();
	afx_msg void 	OnApplyFile();
	afx_msg void 	OnApplyAll();
	afx_msg void 	OnSaveMaterial();
	afx_msg void 	OnSaveFile();
	afx_msg void 	OnSaveAll();
	afx_msg	void 	OnRenameMaterial();
	afx_msg	void 	OnAddMaterial();
	afx_msg	void 	OnAddFolder();
	afx_msg	void 	OnDeleteMaterial();
	afx_msg	void 	OnReloadFile();
	afx_msg	void 	OnCut();
	afx_msg	void 	OnCopy();
	afx_msg	void 	OnPaste();

	//Internal Messages
	afx_msg LRESULT OnRenameFolderComplete(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnRenameMaterialComplete(WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
	

	//Utility methods
	void			RenameMaterial(HTREEITEM item, const char* originalName);
	bool			GetFileName(HTREEITEM item, idStr& out);
	idStr			GetMediaPath(HTREEITEM item, DWORD type);
	void			GetMaterialPaths(HTREEITEM item, idList<MaterialTreeItem_t>* list);
	void			AddStrList(const char *root, idStrList *list, bool includeFile);
	void			PopupMenu(CPoint* pt);
	void			SetItemImage(HTREEITEM item, bool mod, bool apply, bool children);
	

	//Methods for working with the quicktree
	void			CleanLookupTrees(HTREEITEM item);
	void			BuildLookupTrees(HTREEITEM item);
	idStr			GetQuicktreePath(HTREEITEM item);


protected:
	CImageList				m_image;
	bool					treeWithFile;

	//Hashtables for quick lookups
	idHashTable<HTREEITEM>	quickTree;
	idHashTable<HTREEITEM>	materialToTree;
	idHashTable<HTREEITEM>	fileToTree;


	//Member variables for renaming folders
	HTREEITEM				renamedFolder;
	idList<MaterialTreeItem_t> affectedMaterials;

	CImageList*				dragImage;
	bool					bDragging;
	CPoint					dropPoint;
	HTREEITEM				dragItem;

	//Hover Expand
	HTREEITEM				hoverItem;
	DWORD					hoverStartTime;

	bool					internalChange;
};


