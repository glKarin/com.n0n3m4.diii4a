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



#include "qe3.h"
#include "Radiant.h"
#include "EditViewDlg.h"


// CEditViewDlg dialog

IMPLEMENT_DYNAMIC(CEditViewDlg, CDialog)
CEditViewDlg::CEditViewDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CEditViewDlg::IDD, pParent)
{
	findDlg = NULL;
}

CEditViewDlg::~CEditViewDlg() {
}

void CEditViewDlg::DoDataExchange(CDataExchange* pDX) {
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_INFO, editInfo);
}


static UINT FindDialogMessage = ::RegisterWindowMessage(FINDMSGSTRING);

BEGIN_MESSAGE_MAP(CEditViewDlg, CDialog)
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_BUTTON_OPEN, OnBnClickedButtonOpen)
	ON_BN_CLICKED(IDC_BUTTON_SAVE, OnBnClickedButtonSave)
	ON_WM_DESTROY()
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_BUTTON_GOTO, OnBnClickedButtonGoto)
     ON_REGISTERED_MESSAGE(FindDialogMessage, OnFindDialogMessage)
END_MESSAGE_MAP()


// CEditViewDlg message handlers

void CEditViewDlg::OnSize(UINT nType, int cx, int cy) {
	CDialog::OnSize(nType, cx, cy);
	if (GetSafeHwnd() == NULL) {
		return;
	}
	CRect rect, crect;
	GetClientRect(rect);
	CWnd *wnd = GetDlgItem(IDC_BUTTON_OPEN);
	if (wnd == NULL || (wnd && wnd->GetSafeHwnd() == NULL)) {
		return;
	}
	wnd->GetWindowRect(crect);
	wnd->SetWindowPos(NULL, 4, 4, crect.Width(), crect.Height(), SWP_SHOWWINDOW);
	wnd = GetDlgItem(IDC_BUTTON_SAVE);
	int left = 8 + crect.Width();
	wnd->SetWindowPos(NULL, left, 4, crect.Width(), crect.Height(), SWP_SHOWWINDOW);
	wnd = GetDlgItem(IDOK);
	wnd->SetWindowPos(NULL, rect.Width() - crect.Width() - 4, 4, crect.Width(), crect.Height(), SWP_SHOWWINDOW);
	editInfo.SetWindowPos(NULL, 4, 8 + crect.Height(), rect.Width() - 8, rect.Height() - crect.Height() * 2 - 16, SWP_SHOWWINDOW);
	wnd = GetDlgItem(IDC_BUTTON_GOTO);
	wnd->SetWindowPos(NULL, 4, rect.Height() - 4 - crect.Height(), crect.Width(), crect.Height(), SWP_SHOWWINDOW);
	wnd = GetDlgItem(IDC_EDIT_GOTO);
	wnd->SetWindowPos(NULL, 8 + crect.Width(), rect.Height() - 3 - crect.Height(), crect.Width() + 8, crect.Height() - 3, SWP_SHOWWINDOW);
	wnd = GetDlgItem(IDC_STATIC_LINE);
	wnd->SetWindowPos(NULL, 30 + crect.Width() * 2, rect.Height() - crect.Height(), crect.Width() * 2, crect.Height(), SWP_SHOWWINDOW);
	wnd = GetDlgItem(IDC_EDIT_LINE);
	wnd->SetWindowPos(NULL, 40 + crect.Width() * 3, rect.Height() - crect.Height(), crect.Width() + 8, crect.Height(), SWP_SHOWWINDOW);
}

void CEditViewDlg::ShowFindDlg() {
	if (findDlg) {
		return;
	}
	findDlg = new CFindReplaceDialog();
	findDlg->Create(TRUE, findStr, NULL, FR_DOWN, this);

}

void CEditViewDlg::OnBnClickedButtonOpen() {
	CPreviewDlg *dlg = NULL;
	dlg = ((mode == MATERIALS) ? CEntityDlg::ShowMaterialChooser() : CEntityDlg::ShowGuiChooser());
	if (dlg) {
		if (mode == MATERIALS) {
			const idMaterial *mat = declManager->FindMaterial(dlg->mediaName);
			SetMaterialInfo(mat->GetName(), mat->GetFileName(), mat->GetLineNum());
		} else {
			SetGuiInfo(dlg->mediaName);
		}
	}
}

void CEditViewDlg::OnBnClickedButtonSave() {
	if (fileName.Length()) {
		CString text;
        editInfo.GetWindowText(text);
		fileSystem->WriteFile(fileName, text.GetBuffer(0), text.GetLength(), "fs_devpath", "");
		if (mode == MATERIALS) {
			declManager->Reload( false );
		} else {
			uiManager->Reload(false);
		}
	}
}

void CEditViewDlg::UpdateEditPreview() {
	if (GetSafeHwnd() && editInfo.GetSafeHwnd()) {
		editInfo.SetWindowText(editText);
		editInfo.LineScroll(line);
		int cindex = editInfo.LineIndex(line);
		int len = editInfo.LineLength(line);
		editInfo.SetSel(cindex, cindex);
		mediaPreview.SetMode((mode == MATERIALS) ? CMediaPreviewDlg::MATERIALS : CMediaPreviewDlg::GUIS);
		mediaPreview.SetMedia((mode == MATERIALS) ? matName : fileName);
		SetWindowText(va("Editing %s in file <%s>", (mode == MATERIALS) ? matName.c_str() : fileName.c_str(), fileName.c_str()));
		editInfo.SetFocus();
	}
}

BOOL CEditViewDlg::OnInitDialog() {
	CDialog::OnInitDialog();

	mediaPreview.Create(IDD_DIALOG_EDITPREVIEW, this);
	mediaPreview.ShowWindow(SW_SHOW);

	CRect rct;
	LONG lSize = sizeof(rct);
	if (LoadRegistryInfo("Radiant::EditViewWindow", &rct, &lSize))  {
		SetWindowPos(NULL, rct.left, rct.top, rct.Width(), rct.Height(), SWP_SHOWWINDOW);
	}

	editInfo.SetTabStops();
	editInfo.SetLimitText(1024 * 1024);

	UpdateEditPreview();

	SetTimer(1, 250, NULL);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CEditViewDlg::OnDestroy() {
	if (GetSafeHwnd()) {
		CRect rct;
		GetWindowRect(rct);
		SaveRegistryInfo("Radiant::EditViewWindow", &rct, sizeof(rct));
	}

	CDialog::OnDestroy();
}

void CEditViewDlg::SetMaterialInfo(const char *name, const char *file, int _line) {
	idStr str;
	void *buf;
	fileName = "";
	matName = "";
	line = 0;
	str = fileSystem->OSPathToRelativePath( file );
	int size = fileSystem->ReadFile( str, &buf );
	if (size > 0) {
		fileName = str;
		matName = name;
		line = _line - 1;
		if (line < 0) {
			line = 0;
		}
		editText = (char*)buf;
		fileSystem->FreeFile(buf);
	}
	UpdateEditPreview();
}

void CEditViewDlg::SetGuiInfo(const char *name) {
	fileName = "";
	line = 0;
	void *buf;
	int size = fileSystem->ReadFile(name, &buf, NULL);
	if (size > 0) {
		fileName = name;
		editText = (char*)buf;
		fileSystem->FreeFile(buf);
	}
	UpdateEditPreview();
}

void CEditViewDlg::OnTimer(UINT_PTR nIDEvent) {
	CDialog::OnTimer(nIDEvent);
	CWnd *wnd = GetDlgItem(IDC_EDIT_LINE);
	if (wnd) {
		int start, end;
		editInfo.GetSel(start, end);
		wnd->SetWindowText(va("%i",editInfo.LineFromChar(start)));
	}

}

void CEditViewDlg::OnBnClickedButtonGoto() {
	CWnd *wnd = GetDlgItem(IDC_EDIT_GOTO);
	if (wnd) {
		CString str;
		wnd->GetWindowText(str);
		if (str.GetLength()) {
			int l = atoi(str);
			editInfo.SetSel(0, 0);
			editInfo.LineScroll(l);
			int cindex = editInfo.LineIndex(l);
			int len = editInfo.LineLength(l);
			editInfo.SetSel(cindex, cindex);
			editInfo.RedrawWindow();
			editInfo.SetFocus();
		}
	}
}

BOOL CEditViewDlg::PreTranslateMessage(MSG* pMsg) {

	if (pMsg->message == WM_KEYDOWN && (pMsg->wParam == 's' || pMsg->wParam == 'S') && GetAsyncKeyState(VK_CONTROL) & 0x8000) {
		OnBnClickedButtonSave();
		return TRUE;
	}

	if (pMsg->message == WM_KEYDOWN && (pMsg->wParam == 'o' || pMsg->wParam == 'O') && GetAsyncKeyState(VK_CONTROL) & 0x8000) {
		OnBnClickedButtonOpen();
		return TRUE;
	}

	if (pMsg->message == WM_KEYDOWN && (pMsg->wParam == 'f' || pMsg->wParam == 'F') && GetAsyncKeyState(VK_CONTROL) & 0x8000) {
		ShowFindDlg();
		return TRUE;
	}

	if (pMsg->hwnd == editInfo.GetSafeHwnd() && (pMsg->message == WM_KEYDOWN) && (pMsg->wParam == VK_TAB)) {
		// get the char index of the caret position
		int nPos = LOWORD(editInfo.CharFromPos(editInfo.GetCaretPos()));
		// select zero chars
		editInfo.SetSel(nPos, nPos);
		// then replace that selection with a TAB
		editInfo.ReplaceSel("\t", TRUE);
		return TRUE;
	} 

	return CDialog::PreTranslateMessage(pMsg);
}

LRESULT CEditViewDlg::OnFindDialogMessage(WPARAM wParam, LPARAM lParam) {
	if (findDlg == NULL) {
		return 0;
	}

	if (findDlg->IsTerminating()) {
        findDlg = NULL;
        return 0;
    }

	// If the FR_FINDNEXT flag is set,
	// call the application-defined search routine
	// to search for the requested string.
	if(findDlg->FindNext()) {
		//read data from dialog
        findStr = findDlg->GetFindString().GetBuffer(0);
		CString str;
		editInfo.GetWindowText(str);
		editText = str; 
		int start, end;
		editInfo.GetSel(start, end);
		start = editText.Find(findStr, false, end);
		if (start >= 0) {
			editInfo.SetSel(start, start + findStr.Length());
			editInfo.Invalidate();
			editInfo.RedrawWindow();
		}
    }
	return 0;
}
