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
#include "afxcmn.h"

#include "entitydlg.h"
#include "ConsoleDlg.h"
#include "TabsDlg.h"


// CInspectorDialog dialog

class CInspectorDialog : public CTabsDlg
{
	//DECLARE_DYNAMIC(CInspectorDialog)w

public:
	CInspectorDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~CInspectorDialog();

// Dialog Data
	enum { IDD = IDD_DIALOG_INSPECTORS };

protected:
	bool initialized;
	unsigned int dockedTabs;

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	void AssignModel ();
	CTabCtrl tabInspector;
	//idGLConsoleWidget consoleWnd;
	CConsoleDlg consoleWnd;
	CNewTexWnd texWnd;
	CDialogTextures mediaDlg;
	CEntityDlg entityDlg;
	void SetMode(int mode, bool updateTabs = true);
	void UpdateEntitySel(eclass_t *ent);
	void UpdateSelectedEntity();
	void FillClassList();
	bool GetSelectAllCriteria(idStr &key, idStr &val);

	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDestroy();
	afx_msg void OnClose();
	virtual BOOL PreTranslateMessage(MSG* pMsg);

	void SetDockedTabs ( bool docked , int ID );	
};

extern CInspectorDialog *g_Inspectors;