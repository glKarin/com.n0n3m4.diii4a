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

// DlgAudioQuality.cpp : implementation file
//

#include "stdafx.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgAudioQuality dialog


CDlgAudioQuality::CDlgAudioQuality(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgAudioQuality::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgAudioQuality)
	m_iAudioQualityRadio = -1;
	m_bUseDirectSound3D = FALSE;
	//}}AFX_DATA_INIT
}


void CDlgAudioQuality::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

  // if dialog is recieving data
  if( pDX->m_bSaveAndValidate == FALSE)
  {
    // set current state of audio quality radio button
/*    switch( Flesh.gm_aqAudioQuality)
    {
    case CGame::AQ_LOW:{ m_iAudioQualityRadio = 0; break; }
    case CGame::AQ_MEDIUM:{ m_iAudioQualityRadio = 1; break; }
    case CGame::AQ_HIGH:{ m_iAudioQualityRadio = 2; break; }
    default:{ ASSERTALWAYS( "Illegal audio quality value found!"); }
    }
    */
    m_bUseDirectSound3D = FALSE;
  }

  //{{AFX_DATA_MAP(CDlgAudioQuality)
	DDX_Radio(pDX, IDC_AUDIO_QUALITY_LOW, m_iAudioQualityRadio);
	DDX_Check(pDX, IDC_USE_DSOUND3D, m_bUseDirectSound3D);
	//}}AFX_DATA_MAP
  
  // if dialog is giving data
  if( pDX->m_bSaveAndValidate != FALSE)
  {
    // apply selected state of audio quality radio button
/*    switch( m_iAudioQualityRadio)
    {
    case 0:{ Flesh.gm_aqAudioQuality = CGame::AQ_LOW; break; }
    case 1:{ Flesh.gm_aqAudioQuality = CGame::AQ_MEDIUM; break; }
    case 2:{ Flesh.gm_aqAudioQuality = CGame::AQ_HIGH; break; }
    default:{ ASSERTALWAYS( "Illegal audio quality radio value found!"); }
    }
    */
    //Flesh.gm_bUseDirectSound3D = m_bUseDirectSound3D;
  }
}


BEGIN_MESSAGE_MAP(CDlgAudioQuality, CDialog)
	//{{AFX_MSG_MAP(CDlgAudioQuality)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgAudioQuality message handlers
