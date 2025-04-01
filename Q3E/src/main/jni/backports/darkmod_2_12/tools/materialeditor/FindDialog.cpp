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



#include "FindDialog.h"

#include "MEMainFrame.h"

IMPLEMENT_DYNAMIC(FindDialog, CDialog)

BEGIN_MESSAGE_MAP(FindDialog, CDialog)
	ON_BN_CLICKED(ID_FIND_NEXT, OnBnClickedFindNext)
END_MESSAGE_MAP()

/**
* Constructor for FindDialog.
*/
FindDialog::FindDialog(CWnd* pParent)
:	CDialog(FindDialog::IDD, pParent) {
		registry.Init("Software\\id Software\\DOOM3\\Tools\\MaterialEditor\\Find");
	parent = (MEMainFrame*)pParent;
}

/**
* Destructor for FindDialog.
*/
FindDialog::~FindDialog() {
}

/**
* Creates and instance of the find dialog.
*/
BOOL FindDialog::Create() {
	return CDialog::Create(FindDialog::IDD, parent);
}

/**
* Transfers data to and from the controls in the find dialog.
*/
void FindDialog::DoDataExchange(CDataExchange* pDX) {
	CDialog::DoDataExchange(pDX);

	CString temp = searchData.searchText;
	DDX_Text(pDX, IDC_EDIT_FINDTEXT, temp);
	DDX_Check(pDX, IDC_CHECK_NAME_ONLY, searchData.nameOnly);
	DDX_Radio(pDX, IDC_RADIO_SEARCHFILE, searchData.searchScope);

	searchData.searchText = temp;
}

/**
* Called while the dialog is being initialized to load the find parameters 
* from the registry and set the focus to the correct control.
*/
BOOL FindDialog::OnInitDialog() {
	CDialog::OnInitDialog();

	LoadFindSettings();

	GetDlgItem(IDC_EDIT_FINDTEXT)->SetFocus();

	return FALSE;
}

/**
* Triggers a search based on the parameters in the dialog.
*/
void FindDialog::OnBnClickedFindNext() {

	UpdateData();
	searchData.searched = false;
	parent->FindNext(&searchData);
}

/**
* Saves the search parameters and closes the find dialog.
*/
void FindDialog::OnCancel()
{
	SaveFindSettings();

	parent->CloseFind();
	DestroyWindow();
}

/**
* Loads the search parameters from the registry and makes sure the controls are properly
* initialized.
*/
void FindDialog::LoadFindSettings() {
	registry.Load();

	searchData.searchText = registry.GetString("searchText");
	searchData.nameOnly = (int)registry.GetFloat("nameOnly");
	searchData.searchScope = (int)registry.GetFloat("searchScope");

	registry.GetWindowPlacement("findDialog", GetSafeHwnd());
	
	UpdateData(FALSE);
}

/**
* Saves the search parameters to the registry.
*/
void FindDialog::SaveFindSettings() {

	UpdateData();

	registry.SetString("searchText", searchData.searchText);
	registry.SetFloat("nameOnly", searchData.nameOnly);
	registry.SetFloat("searchScope", searchData.searchScope);
	
	registry.SetWindowPlacement("findDialog", GetSafeHwnd());

	registry.Save();
}


