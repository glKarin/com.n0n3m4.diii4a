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

// DlgSelectPlayer.cpp : implementation file
//

#include "stdafx.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgSelectPlayer dialog


CDlgSelectPlayer::CDlgSelectPlayer(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgSelectPlayer::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgSelectPlayer)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CDlgSelectPlayer::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgSelectPlayer)
	DDX_Control(pDX, IDC_COMBO_AVAILABLE_CONTROLS, m_comboAvailableControls);
	DDX_Control(pDX, IDC_COMBO_AVAILABLE_PLAYERS, m_comboAvailablePlayers);
	//}}AFX_DATA_MAP

  // if dialog is giving data
  if( pDX->m_bSaveAndValidate != FALSE)
  {
    INDEX iSelectedPlayer = m_comboAvailablePlayers.GetCurSel();
    INDEX iSelectedControls = m_comboAvailableControls.GetCurSel();
    // if there is no player selected
    if( (iSelectedPlayer == CB_ERR) || ( iSelectedControls == CB_ERR) )
    {
      // end dialog as if user pressed escape key
      EndDialog( IDCANCEL);
    }
    else
    {
      _pGame->gm_iWEDSinglePlayer = iSelectedPlayer;
      _pGame->SavePlayersAndControls();
    }
  }
}


BEGIN_MESSAGE_MAP(CDlgSelectPlayer, CDialog)
	//{{AFX_MSG_MAP(CDlgSelectPlayer)
	ON_CBN_SELCHANGE(IDC_COMBO_AVAILABLE_PLAYERS, OnSelchangeComboAvailablePlayers)
	ON_CBN_SELCHANGE(IDC_COMBO_AVAILABLE_CONTROLS, OnSelchangeComboAvailableControls)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgSelectPlayer message handlers

BOOL CDlgSelectPlayer::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
  // fill players and controls combo boxes
  for( INDEX iPC=0; iPC<8; iPC++)
  {
    CTString strPlayer = _pGame->gm_apcPlayers[ iPC].pc_strName;
    m_comboAvailablePlayers.AddString( strPlayer);
//    CTString strControls = _pGame->gm_actrlControls[ iPC].ctrl_strName;
    m_comboAvailableControls.AddString( "dummy");
  }
  m_comboAvailablePlayers.SetCurSel( _pGame->gm_iWEDSinglePlayer);
  m_comboAvailableControls.SetCurSel( _pGame->gm_iWEDSinglePlayer);

	return TRUE;
}

void CDlgSelectPlayer::OnSelchangeComboAvailablePlayers() 
{
  INDEX iSelectedPlayer = m_comboAvailablePlayers.GetCurSel();
  if( iSelectedPlayer == CB_ERR) return;
  m_comboAvailableControls.SetCurSel( iSelectedPlayer);
}

void CDlgSelectPlayer::OnSelchangeComboAvailableControls() 
{
  INDEX iSelectedPlayer = m_comboAvailablePlayers.GetCurSel();
  if( iSelectedPlayer == CB_ERR) return;
  INDEX iSelectedControls = m_comboAvailableControls.GetCurSel();
}
