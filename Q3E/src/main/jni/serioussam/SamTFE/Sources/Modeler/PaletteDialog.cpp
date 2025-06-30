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

// PaletteDialog.cpp : implementation file
//

#include "stdafx.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPaletteDialog dialog


CPaletteDialog::CPaletteDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CPaletteDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CPaletteDialog)
	m_ColorName = _T("");
	m_ModeString = _T("");
	//}}AFX_DATA_INIT
  m_LastViewUpdated = NULL;
}

void CPaletteDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
  
  CModelerView *pModelerView = CModelerView::GetActiveView();
  if( !pDX->m_bSaveAndValidate)
  {
    if( pModelerView != NULL)
    {
      if( pModelerView->m_bOnColorMode)
      {
        m_ModeString = "Mode: On";
      }
      else
      {
        m_ModeString = "Mode: Off";
      }

      m_ColorName = pModelerView->m_ModelObject.GetColorName( pModelerView->m_iChoosedColor);
    }
    else
    {
      m_ColorName = "";
    }
  }

	//{{AFX_DATA_MAP(CPaletteDialog)
	DDX_Control(pDX, IDC_CURRENT_COLOR, m_ChoosedColorButton);
	DDX_Control(pDX, IDC_PICK_COLOR, m_PickColor);
	DDX_Control(pDX, IDC_COLOR_PALETTE_BUTTON9, m_ColorPaletteButton9);
	DDX_Control(pDX, IDC_COLOR_PALETTE_BUTTON8, m_ColorPaletteButton8);
	DDX_Control(pDX, IDC_COLOR_PALETTE_BUTTON7, m_ColorPaletteButton7);
	DDX_Control(pDX, IDC_COLOR_PALETTE_BUTTON6, m_ColorPaletteButton6);
	DDX_Control(pDX, IDC_COLOR_PALETTE_BUTTON5, m_ColorPaletteButton5);
	DDX_Control(pDX, IDC_COLOR_PALETTE_BUTTON4, m_ColorPaletteButton);
	DDX_Control(pDX, IDC_COLOR_PALETTE_BUTTON32, m_ColorPaletteButton32);
	DDX_Control(pDX, IDC_COLOR_PALETTE_BUTTON31, m_ColorPaletteButton31);
	DDX_Control(pDX, IDC_COLOR_PALETTE_BUTTON30, m_ColorPaletteButton30);
	DDX_Control(pDX, IDC_COLOR_PALETTE_BUTTON3, m_ColorPaletteButton3);
	DDX_Control(pDX, IDC_COLOR_PALETTE_BUTTON29, m_ColorPaletteButton29);
	DDX_Control(pDX, IDC_COLOR_PALETTE_BUTTON28, m_ColorPaletteButton28);
	DDX_Control(pDX, IDC_COLOR_PALETTE_BUTTON27, m_ColorPaletteButton27);
	DDX_Control(pDX, IDC_COLOR_PALETTE_BUTTON26, m_ColorPaletteButton26);
	DDX_Control(pDX, IDC_COLOR_PALETTE_BUTTON25, m_ColorPaletteButton25);
	DDX_Control(pDX, IDC_COLOR_PALETTE_BUTTON24, m_ColorPaletteButton24);
	DDX_Control(pDX, IDC_COLOR_PALETTE_BUTTON23, m_ColorPaletteButton23);
	DDX_Control(pDX, IDC_COLOR_PALETTE_BUTTON22, m_ColorPaletteButton22);
	DDX_Control(pDX, IDC_COLOR_PALETTE_BUTTON21, m_ColorPaletteButton21);
	DDX_Control(pDX, IDC_COLOR_PALETTE_BUTTON20, m_ColorPaletteButton20);
	DDX_Control(pDX, IDC_COLOR_PALETTE_BUTTON2, m_ColorPaletteButton2);
	DDX_Control(pDX, IDC_COLOR_PALETTE_BUTTON19, m_ColorPaletteButton19);
	DDX_Control(pDX, IDC_COLOR_PALETTE_BUTTON18, m_ColorPaletteButton18);
	DDX_Control(pDX, IDC_COLOR_PALETTE_BUTTON17, m_ColorPaletteButton17);
	DDX_Control(pDX, IDC_COLOR_PALETTE_BUTTON16, m_ColorPaletteButton16);
	DDX_Control(pDX, IDC_COLOR_PALETTE_BUTTON15, m_ColorPaletteButton15);
	DDX_Control(pDX, IDC_COLOR_PALETTE_BUTTON14, m_ColorPaletteButton14);
	DDX_Control(pDX, IDC_COLOR_PALETTE_BUTTON13, m_ColorPaletteButton13);
	DDX_Control(pDX, IDC_COLOR_PALETTE_BUTTON12, m_ColorPaletteButton12);
	DDX_Control(pDX, IDC_COLOR_PALETTE_BUTTON11, m_ColorPaletteButton11);
	DDX_Control(pDX, IDC_COLOR_PALETTE_BUTTON10, m_ColorPaletteButton10);
	DDX_Control(pDX, IDC_COLOR_PALETTE_BUTTON1, m_ColorPaletteButton1);
	DDX_Text(pDX, IDC_EDIT_COLOR_NAME, m_ColorName);
	DDV_MaxChars(pDX, m_ColorName, 32);
	DDX_Text(pDX, IDC_MODE, m_ModeString);
	//}}AFX_DATA_MAP

  if( pDX->m_bSaveAndValidate)
  {
    if( pModelerView != NULL)
    {
      pModelerView->m_ModelObject.SetColorName( pModelerView->m_iChoosedColor, 
                                                CTString(CStringA(m_ColorName)));
      pModelerView->GetDocument()->SetModifiedFlag();
    }
  }
}

BEGIN_MESSAGE_MAP(CPaletteDialog, CDialog)
	//{{AFX_MSG_MAP(CPaletteDialog)
	ON_EN_CHANGE(IDC_EDIT_COLOR_NAME, OnChangeEditColorName)
	//}}AFX_MSG_MAP
  ON_COMMAND_RANGE(IDC_COLOR_PALETTE_BUTTON_BASE, IDC_COLOR_PALETTE_BUTTON_END, OnColorPalleteButton)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPaletteDialog message handlers

BOOL CPaletteDialog::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
  HICON hIcon = AfxGetApp()->LoadIcon(IDI_PICK);
  ASSERT( hIcon != NULL);
  m_PickColor.SetIcon( hIcon);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CPaletteDialog::OnColorPalleteButton(UINT nID)
{
  INDEX m_iChoosedColor = nID - IDC_COLOR_PALETTE_BUTTON_BASE;
}

BOOL CPaletteDialog::OnIdle(LONG lCount)
{
  CModelerView *pModelerView = CModelerView::GetActiveView();
  if( pModelerView != m_LastViewUpdated)
  {
    if( (pModelerView == NULL) || (m_LastViewUpdated == NULL) )
    {
      UpdateData( FALSE);
      Invalidate( FALSE);
    }
    else
    {
      UpdateData( FALSE);
    }
    m_LastViewUpdated = pModelerView;
  }
  return TRUE;
}

void CPaletteDialog::OnChangeEditColorName() 
{
	UpdateData(TRUE);	
}
