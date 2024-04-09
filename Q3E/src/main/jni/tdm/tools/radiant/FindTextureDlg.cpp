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
#include "FindTextureDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFindTextureDlg dialog

CFindTextureDlg g_TexFindDlg;
CFindTextureDlg& g_dlgFind = g_TexFindDlg;
static bool g_bFindActive = true;

void CFindTextureDlg::updateTextures(const char *p)
{
  if (isOpen())
  {
    if (g_bFindActive)
    {
      setFindStr(p);
    }
    else
    {
      setReplaceStr(p);
    }
  }
}

CFindTextureDlg::CFindTextureDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CFindTextureDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CFindTextureDlg)
	m_bSelectedOnly = FALSE;
	m_strFind = _T("");
	m_strReplace = _T("");
	m_bForce = FALSE;
	m_bLive = TRUE;
	//}}AFX_DATA_INIT
}


void CFindTextureDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFindTextureDlg)
	DDX_Check(pDX, IDC_CHECK_SELECTED, m_bSelectedOnly);
	DDX_Text(pDX, IDC_EDIT_FIND, m_strFind);
	DDX_Text(pDX, IDC_EDIT_REPLACE, m_strReplace);
	DDX_Check(pDX, IDC_CHECK_FORCE, m_bForce);
	DDX_Check(pDX, IDC_CHECK_LIVE, m_bLive);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CFindTextureDlg, CDialog)
	//{{AFX_MSG_MAP(CFindTextureDlg)
	ON_BN_CLICKED(IDC_BTN_APPLY, OnBtnApply)
	ON_EN_SETFOCUS(IDC_EDIT_FIND, OnSetfocusEditFind)
	ON_EN_SETFOCUS(IDC_EDIT_REPLACE, OnSetfocusEditReplace)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CFindTextureDlg::OnBtnApply() 
{
	UpdateData(TRUE);
	CRect rct;
	GetWindowRect(rct);
	SaveRegistryInfo("Radiant::TextureFindWindow", &rct, sizeof(rct));
	FindReplaceTextures( m_strFind, m_strReplace, ( m_bSelectedOnly != FALSE ), ( m_bForce != FALSE ) );
}

void CFindTextureDlg::OnOK() 
{
	UpdateData(TRUE);
	CRect rct;
	GetWindowRect(rct);
	SaveRegistryInfo("Radiant::TextureFindWindow", &rct, sizeof(rct));
	FindReplaceTextures( m_strFind, m_strReplace, ( m_bSelectedOnly != FALSE ), ( m_bForce != FALSE ) );
	CDialog::OnOK();
}

void CFindTextureDlg::show()
{
  if (g_dlgFind.GetSafeHwnd() == NULL || IsWindow(g_dlgFind.GetSafeHwnd()) == FALSE)
  {
    g_dlgFind.Create(IDD_DIALOG_FINDREPLACE);
    g_dlgFind.ShowWindow(SW_SHOW);
  }
  else
  {
    g_dlgFind.ShowWindow(SW_SHOW);
  }
  CRect rct;
  LONG lSize = sizeof(rct);
  if (LoadRegistryInfo("Radiant::TextureFindWindow", &rct, &lSize))
    g_dlgFind.SetWindowPos(NULL, rct.left, rct.top, 0,0, SWP_NOSIZE | SWP_SHOWWINDOW);
}		


bool CFindTextureDlg::isOpen()
{
  return (g_dlgFind.GetSafeHwnd() == NULL || ::IsWindowVisible(g_dlgFind.GetSafeHwnd()) == FALSE) ? false : true;
}

void CFindTextureDlg::setFindStr(const char * p)
{
  g_dlgFind.UpdateData(TRUE);
  if (g_dlgFind.m_bLive)
  {
    g_dlgFind.m_strFind = p;
    g_dlgFind.UpdateData(FALSE);
  }
}

void CFindTextureDlg::setReplaceStr(const char * p)
{
  g_dlgFind.UpdateData(TRUE);
  if (g_dlgFind.m_bLive)
  {
    g_dlgFind.m_strReplace = p;
    g_dlgFind.UpdateData(FALSE);
  }
}


void CFindTextureDlg::OnCancel() 
{
  CRect rct;
  GetWindowRect(rct);
  SaveRegistryInfo("Radiant::TextureFindWindow", &rct, sizeof(rct));
	CDialog::OnCancel();
}

BOOL CFindTextureDlg::DestroyWindow() 
{
	return CDialog::DestroyWindow();
}

void CFindTextureDlg::OnSetfocusEditFind() 
{
  g_bFindActive = true;
}

void CFindTextureDlg::OnSetfocusEditReplace() 
{
  g_bFindActive = false;
}
