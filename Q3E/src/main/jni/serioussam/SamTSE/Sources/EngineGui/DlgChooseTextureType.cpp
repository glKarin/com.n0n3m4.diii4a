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

// DlgChooseTextureType.cpp : implementation file
//

#include "EngineGui/StdH.h"
#include "DlgChooseTextureType.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgChooseTextureType dialog


CDlgChooseTextureType::CDlgChooseTextureType(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgChooseTextureType::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgChooseTextureType)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CDlgChooseTextureType::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgChooseTextureType)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgChooseTextureType, CDialog)
	//{{AFX_MSG_MAP(CDlgChooseTextureType)
	ON_BN_CLICKED(ID_ANIMATED_TEXTURE, OnAnimatedTexture)
	ON_BN_CLICKED(ID_EFFECT_TEXTURE, OnEffectTexture)
	ON_BN_CLICKED(ID_NORMAL_TEXTURE, OnNormalTexture)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgChooseTextureType message handlers

void CDlgChooseTextureType::OnNormalTexture() 
{
  EndDialog( 0);
}

void CDlgChooseTextureType::OnAnimatedTexture() 
{
  EndDialog( 1);
}

void CDlgChooseTextureType::OnEffectTexture() 
{
  EndDialog( 2);
}

void CDlgChooseTextureType::OnCancel() 
{
  EndDialog( -1);
}
