/* Copyright (c) 2002-2012 Croteam Ltd. 
This program is free software; you can redistribute it and/or modify
it under the terms of version 2 of the GNU General Public License as published by
the Free Software Foundation


This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA. */

// CtrlAxisRadio.cpp : implementation file
//

#include "stdafx.h"
#include "WorldEditor.h"
#include "CtrlAxisRadio.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCtrlAxisRadio

CCtrlAxisRadio::CCtrlAxisRadio()
{
}

CCtrlAxisRadio::~CCtrlAxisRadio()
{
}

void CCtrlAxisRadio::SetDialogPtr( CPropertyComboBar *pDialog)
{
  m_pDialog = pDialog;
}

BEGIN_MESSAGE_MAP(CCtrlAxisRadio, CButton)
	//{{AFX_MSG_MAP(CCtrlAxisRadio)
	ON_CONTROL_REFLECT(BN_CLICKED, OnClicked)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCtrlAxisRadio message handlers

void CCtrlAxisRadio::OnClicked() 
{
  // don't do anything if document doesn't exist
  if( theApp.GetDocument() == NULL) return;
  // select clicked axis radio
  m_pDialog->SelectAxisRadio( this);
  // show that second axis have been selected
	m_pDialog->ArrangeControls();
}
