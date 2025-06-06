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

// DlgVideoQuality.cpp : implementation file
//

#include "stdafx.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgVideoQuality dialog


CDlgVideoQuality::CDlgVideoQuality(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgVideoQuality::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgVideoQuality)
	m_radioObjectShadowQuality = -1;
	m_radioTextureQuality = -1;
	m_radioWorldShadowQuality = -1;
	//}}AFX_DATA_INIT
}


void CDlgVideoQuality::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
  
  // if dialog is recieving data
  if( pDX->m_bSaveAndValidate == FALSE)
  {
    // set current state of texture quality radio button
/*    switch( Flesh.gm_tqTextureQuality)
    {
    case CGame::TQ_LOW:     { m_radioTextureQuality = 0; break; }
    case CGame::TQ_MEDIUM:  { m_radioTextureQuality = 1; break; }
    case CGame::TQ_HIGH:    { m_radioTextureQuality = 2; break; }
    default:                 { ASSERTALWAYS( "Illegal texture quality value found!"); }
    }
    // set current state of object shadow quality radio button
    switch( Flesh.gm_osqObjectShadowQuality)
    {
    case CGame::OSQ_OFF:    { m_radioObjectShadowQuality = 0; break; }
    case CGame::OSQ_LOW:    { m_radioObjectShadowQuality = 1; break; }
    case CGame::OSQ_HIGH:   { m_radioObjectShadowQuality = 2; break; }
    default:                 { ASSERTALWAYS( "Illegal object shadow quality value found!"); }
    }
    // set current state of world shadow quality radio button
    switch( Flesh.gm_wsqWorldShadowQuality)
    {
    case CGame::WSQ_LOW:    { m_radioWorldShadowQuality = 0; break; }
    case CGame::WSQ_MEDIUM: { m_radioWorldShadowQuality = 1; break; }
    case CGame::WSQ_HIGH:   { m_radioWorldShadowQuality = 2; break; }
    default:                 { ASSERTALWAYS( "Illegal world shadow quality value found!"); }
    }
    */
  }

	//{{AFX_DATA_MAP(CDlgVideoQuality)
	DDX_Radio(pDX, IDC_OBJECT_SHADOW_QUALITY, m_radioObjectShadowQuality);
	DDX_Radio(pDX, IDC_TEXTURE_QUALITY, m_radioTextureQuality);
	DDX_Radio(pDX, IDC_WORLD_SHADOW_QUALITY, m_radioWorldShadowQuality);
	//}}AFX_DATA_MAP
  
  // if dialog is giving data
  if( pDX->m_bSaveAndValidate != FALSE)
  {
    // pick up states of texture quality radio button
/*    switch( m_radioTextureQuality)
    {
    case 0:    { Flesh.gm_tqTextureQuality = CGame::TQ_LOW; break; }
    case 1:    { Flesh.gm_tqTextureQuality = CGame::TQ_MEDIUM; break; }
    case 2:    { Flesh.gm_tqTextureQuality = CGame::TQ_HIGH; break; }
    default:   { ASSERTALWAYS( "Illegal texture quality radio value found!"); }
    }
    // pick up states of object shadow quality radio button
    switch( m_radioObjectShadowQuality)
    {
    case 0:    { Flesh.gm_osqObjectShadowQuality = CGame::OSQ_OFF; break; }
    case 1:    { Flesh.gm_osqObjectShadowQuality = CGame::OSQ_LOW; break; }
    case 2:    { Flesh.gm_osqObjectShadowQuality = CGame::OSQ_HIGH; break; }
    default:   { ASSERTALWAYS( "Illegal object shadow quality radio value found!"); }
    }
    // pick up states of world shadow quality radio button
    switch( m_radioWorldShadowQuality)
    {
    case 0:    { Flesh.gm_wsqWorldShadowQuality = CGame::WSQ_LOW; break; }
    case 1:    { Flesh.gm_wsqWorldShadowQuality = CGame::WSQ_MEDIUM; break; }
    case 2:    { Flesh.gm_wsqWorldShadowQuality = CGame::WSQ_HIGH; break; }
    default:   { ASSERTALWAYS( "Illegal world shadow quality radio value found!"); }
    }
    */
  }
}


BEGIN_MESSAGE_MAP(CDlgVideoQuality, CDialog)
	//{{AFX_MSG_MAP(CDlgVideoQuality)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgVideoQuality message handlers
