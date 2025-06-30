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

// DlgPlayerControls.cpp : implementation file
//

#include "stdafx.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDlgPlayerControls dialog

CDlgPlayerControls::CDlgPlayerControls(CControls &ctrlControls, CWnd* pParent /*=NULL*/)
: CDialog(CDlgPlayerControls::IDD, pParent), m_ctrlControls(_pGame->gm_ctrlControlsExtra)
{
  // make copy of the controls, we will change them
  m_ctrlControls = ctrlControls;

  //{{AFX_DATA_INIT(CDlgPlayerControls)
	m_bInvertControler = FALSE;
	m_iRelativeAbsoluteType = -1;
	m_strPressNewButton = _T("");
	//}}AFX_DATA_INIT
}


void CDlgPlayerControls::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);

  INDEX iSelectedAxis = -1;
  BOOL bEnableControls;
  
  // if list control window is created
  if( IsWindow(m_listAxisActions.m_hWnd))
  {
    // if dialog is recieving data
    if( pDX->m_bSaveAndValidate == FALSE)
    {
      iSelectedAxis = m_listAxisActions.GetNextItem( -1, LVNI_SELECTED);
      if( iSelectedAxis != -1)
      {
        // enable combo control
        m_comboControlerAxis.EnableWindow();
        // get curently mounted controler description
        CTString strControlerName = m_listAxisActions.GetItemText( iSelectedAxis, 1);
        // find in combo currently selected mounted axis
        int iComboEntry = m_comboControlerAxis.FindStringExact( -1, strControlerName);
        // and select it 
        m_comboControlerAxis.SetCurSel( iComboEntry);
        // if axis none is selected, disable sensitivity and other additional controls for axis
        if( m_ctrlControls.ctrl_aaAxisActions[ iSelectedAxis].aa_iAxisAction == AXIS_NONE)
        {
          // disable controls for defining controler's attributes
          bEnableControls = FALSE;
        }
        else
        {
          // enable controls for defining controler's attributes
          bEnableControls = TRUE;
          // and get sensitivity slider state
          m_sliderControlerSensitivity.SetPos( 
            m_ctrlControls.ctrl_aaAxisActions[ iSelectedAxis].aa_fSensitivity /
            (100/SENSITIVITY_SLIDER_POSITIONS) );
          // get state of invert axis check box
          m_bInvertControler = m_ctrlControls.ctrl_aaAxisActions[ iSelectedAxis].aa_bInvert;
          // get radio value (relative/absolute type of controler)
          if( m_ctrlControls.ctrl_aaAxisActions[ iSelectedAxis].aa_bRelativeControler)
          {
            m_iRelativeAbsoluteType = 0;
          }
          else
          {
            m_iRelativeAbsoluteType = 1;
          }
        }
      }
      else
      {
        // disable combo control
        m_comboControlerAxis.EnableWindow( FALSE);
        // disable controls for defining controler's attributes
        bEnableControls = FALSE;
      }
      m_sliderControlerSensitivity.EnableWindow( bEnableControls);
      GetDlgItem( IDC_INVERT_CONTROLER)->EnableWindow( bEnableControls);
      GetDlgItem( IDC_RELATIVE_ABSOLUTE_TYPE)->EnableWindow( bEnableControls);
      GetDlgItem( IDC_CONTROLER_SENSITIVITY)->EnableWindow( bEnableControls);
      GetDlgItem( IDC_ABSOLUTE_RADIO)->EnableWindow(bEnableControls);
      GetDlgItem( IDC_CONTROLER_TYPE_T)->EnableWindow(bEnableControls);
      GetDlgItem( IDC_CONTROLER_SENSITIVITY_T)->EnableWindow(bEnableControls);
    }
  }
	
  //{{AFX_DATA_MAP(CDlgPlayerControls)
	DDX_Control(pDX, IDC_AXIS_ACTIONS_LIST, m_listAxisActions);
	DDX_Control(pDX, IDC_BUTTON_ACTIONS_LIST, m_listButtonActions);
	DDX_Control(pDX, IDC_EDIT_SECOND_CONTROL, m_editSecondControl);
	DDX_Control(pDX, IDC_EDIT_FIRST_CONTROL, m_editFirstControl);
	DDX_Control(pDX, IDC_CONTROLER_SENSITIVITY, m_sliderControlerSensitivity);
	DDX_Control(pDX, IDC_CONTROLER_AXIS, m_comboControlerAxis);
	DDX_Check(pDX, IDC_INVERT_CONTROLER, m_bInvertControler);
	DDX_Radio(pDX, IDC_RELATIVE_ABSOLUTE_TYPE, m_iRelativeAbsoluteType);
	DDX_Text(pDX, IDC_PRESS_MESSAGE, m_strPressNewButton);
	//}}AFX_DATA_MAP

  // if dialog is giving data
  if( pDX->m_bSaveAndValidate != FALSE)
  {
    iSelectedAxis = m_listAxisActions.GetNextItem( -1, LVNI_SELECTED);
    // if there is valid selection
    if( iSelectedAxis != -1)
    {
      // apply check box
      m_ctrlControls.ctrl_aaAxisActions[ iSelectedAxis].aa_bInvert = m_bInvertControler;
      // apply relative/absolute radio
      if( m_iRelativeAbsoluteType == 0)
      {
        m_ctrlControls.ctrl_aaAxisActions[ iSelectedAxis].aa_bRelativeControler = TRUE;
      }
      else
      {
        m_ctrlControls.ctrl_aaAxisActions[ iSelectedAxis].aa_bRelativeControler = FALSE;
      }
      // apply sensitivity slider value
      m_ctrlControls.ctrl_aaAxisActions[ iSelectedAxis].aa_fSensitivity = 
        m_sliderControlerSensitivity.GetPos() * (100/SENSITIVITY_SLIDER_POSITIONS);
    }
  }
}


BEGIN_MESSAGE_MAP(CDlgPlayerControls, CDialog)
	//{{AFX_MSG_MAP(CDlgPlayerControls)
	ON_EN_SETFOCUS(IDC_EDIT_FIRST_CONTROL, OnSetfocusEditFirstControl)
	ON_EN_SETFOCUS(IDC_EDIT_SECOND_CONTROL, OnSetfocusEditSecondControl)
	ON_BN_CLICKED(ID_FIRST_CONTROL_NONE, OnFirstControlNone)
	ON_BN_CLICKED(ID_SECOND_CONTROL_NONE, OnSecondControlNone)
	ON_BN_CLICKED(ID_DEFAULT, OnDefault)
	ON_CBN_SELCHANGE(IDC_CONTROLER_AXIS, OnSelchangeControlerAxis)
	ON_BN_CLICKED(ID_MOVE_CONTROL_UP, OnMoveControlUp)
	ON_BN_CLICKED(ID_MOVE_CONTROL_DOWN, OnMoveControlDown)
	ON_BN_CLICKED(ID_BUTTON_ACTION_ADD, OnButtonActionAdd)
	ON_BN_CLICKED(ID_BUTTON_ACTION_EDIT, OnButtonActionEdit)
	ON_BN_CLICKED(ID_BUTTON_ACTION_REMOVE, OnButtonActionRemove)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgPlayerControls message handlers

void CDlgPlayerControls::FillActionsList(void)
{
  // get selected item
  INDEX iDefaultSelected = m_listButtonActions.GetNextItem( -1, LVNI_SELECTED);
  
  // empty list containing actions
  m_listButtonActions.DeleteAllItems();

  // one item to serve for all actions and mounted buttons
  LV_ITEM itItem;
  // all items will be of text type
  itItem.mask = LVIF_TEXT;
  // index for automatic counting of added items
  INDEX ctItemsAdded = 0;
  // now add all button actions
  FOREACHINLIST( CButtonAction, ba_lnNode, m_ctrlControls.ctrl_lhButtonActions, itButtonAction)
  {
    // macro for adding single button action into list control
    itItem.iItem = ctItemsAdded;
    itItem.iSubItem = 0;
    itItem.pszText = (char *)(const char *) itButtonAction->ba_strName;
    m_listButtonActions.InsertItem( &itItem);
    itItem.iSubItem = 1;
    itItem.pszText = (char *)(const char *)_pInput->GetButtonName( itButtonAction->ba_iFirstKey);
    m_listButtonActions.SetItem( &itItem);
    itItem.iSubItem = 2;
    itItem.pszText = (char *)(const char *)_pInput->GetButtonName( itButtonAction->ba_iSecondKey);
    m_listButtonActions.SetItem( &itItem);
    ctItemsAdded++;
  }


  // try to select same item that was selected before refresh list
  if( iDefaultSelected != -1)
  {
    // select wanted item
    m_listButtonActions.SetItemState( iDefaultSelected, 
      LVIS_FOCUSED|LVIS_SELECTED|LVIS_DROPHILITED, LVIS_FOCUSED|LVIS_SELECTED|LVIS_DROPHILITED);
    m_listButtonActions.EnsureVisible( iDefaultSelected, FALSE);
  }
}

void CDlgPlayerControls::FillAxisList(void)
{
  // get selected item
  INDEX iDefaultSelected = m_listAxisActions.GetNextItem( -1, LVNI_SELECTED);
  
  // empty list containing actions
  m_listAxisActions.DeleteAllItems();

  // one item to serve for all actions and mounted buttons
  LV_ITEM itItem;
  // all items will be of text type
  itItem.mask = LVIF_TEXT;
  // now add all axis actions
  for(INDEX iAxis = 0; iAxis<AXIS_ACTIONS_CT; iAxis++) {
    itItem.iItem = iAxis;
    itItem.iSubItem = 0;
    itItem.pszText = (char*)(const char*)_pGame->gm_astrAxisNames[iAxis];
    m_listAxisActions.InsertItem( &itItem);
    itItem.iSubItem = 1;
    itItem.pszText = (char *)(const char *)_pInput->GetAxisName(
      m_ctrlControls.ctrl_aaAxisActions[iAxis].aa_iAxisAction);
    m_listAxisActions.SetItem( &itItem);
  }

  // try to select same item that was selected before refresh list
  if( iDefaultSelected != -1)
  {
    m_listAxisActions.SetItemState( iDefaultSelected, 
      LVIS_FOCUSED|LVIS_SELECTED|LVIS_DROPHILITED, LVIS_FOCUSED|LVIS_SELECTED|LVIS_DROPHILITED);
    m_listAxisActions.EnsureVisible( iDefaultSelected, FALSE);
  }
}

// percentage of action's list control window width used for action names
#define BUTTON_ACTION_NAME_PERCENTAGE 50
#define AXIS_ACTION_NAME_PERCENTAGE 50

BOOL CDlgPlayerControls::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
  // initialize controler sensitivity
  m_sliderControlerSensitivity.SetRange( 1, SENSITIVITY_SLIDER_POSITIONS);
  m_sliderControlerSensitivity.SetTicFreq( 1);

  // get action list control's width in pixels
  CRect rectListControl;
  m_listButtonActions.GetClientRect( rectListControl);
  // insert column for action names
  INDEX iMainColumnWidth = rectListControl.Width()*BUTTON_ACTION_NAME_PERCENTAGE/100;
  m_listButtonActions.InsertColumn( 0, "Button action", LVCFMT_LEFT, iMainColumnWidth);
  // insert first control column
  INDEX iFirstSubColumnWidth = (rectListControl.Width()*(100-BUTTON_ACTION_NAME_PERCENTAGE)/2)/100;
  m_listButtonActions.InsertColumn( 1, "First", LVCFMT_LEFT, iFirstSubColumnWidth);
  // insert second control column
  INDEX iSecondSubColumnWidth = 
    rectListControl.Width()-iMainColumnWidth-iFirstSubColumnWidth - 16;
  m_listButtonActions.InsertColumn( 2, "Second", LVCFMT_LEFT, iSecondSubColumnWidth);

  // add all actions into actions list
  FillActionsList();

  // get axis action list control's width in pixels
  m_listAxisActions.GetClientRect( rectListControl);
  // insert column for axis action names
  iMainColumnWidth = rectListControl.Width()*AXIS_ACTION_NAME_PERCENTAGE/100;
  m_listAxisActions.InsertColumn( 0, "Axis action", LVCFMT_LEFT, iMainColumnWidth);
  // insert mounting controls column
  INDEX iAxisMouterNameWidth = rectListControl.Width()*(100-AXIS_ACTION_NAME_PERCENTAGE)/100-1;
  m_listAxisActions.InsertColumn( 1, "Current controler", LVCFMT_LEFT, iAxisMouterNameWidth);

  // add all available axis into axis list
  FillAxisList();
  
  // for all possible axis mounting controlers
  for( INDEX iAxis=0; iAxis<_pInput->GetAvailableAxisCount(); iAxis++)
  {
    m_comboControlerAxis.AddString( _pInput->GetAxisName( iAxis));
  }

  return TRUE;
}

void CDlgPlayerControls::SetFirstAndSecondButtonNames(void)
{
  BOOL bEnablePressKeyControls;
  CButtonAction *pbaCurrent = GetSelectedButtonAction();
  if( pbaCurrent != NULL)
  {
    // type first currently mounted button's name
    m_editFirstControl.SetWindowText( (char *)(const char *)
      _pInput->GetButtonName( pbaCurrent->ba_iFirstKey) );
    // type second currently mounted button's name
    m_editSecondControl.SetWindowText( (char *)(const char *)
      _pInput->GetButtonName( pbaCurrent->ba_iSecondKey) );
    // enable edit key and "none" controls
    bEnablePressKeyControls = TRUE;
  }
  else
  {
    bEnablePressKeyControls = FALSE;
  }
  // enable/disable press key controls (edit boxes and "none" buttons)
  m_editFirstControl.EnableWindow( bEnablePressKeyControls);
  m_editSecondControl.EnableWindow( bEnablePressKeyControls);
  GetDlgItem( ID_FIRST_CONTROL_NONE)->EnableWindow( bEnablePressKeyControls);
  GetDlgItem( ID_SECOND_CONTROL_NONE)->EnableWindow( bEnablePressKeyControls);    
}

void CDlgPlayerControls::ActivatePressKey( char *strFirstOrSecond)
{
  // get selected action
  m_iSelectedAction = m_listButtonActions.GetNextItem( -1, LVNI_SELECTED);
  // if there is valid selection
  if( m_iSelectedAction == -1)
  {
    // activate actions list
    m_listButtonActions.SetFocus();
    return;
  }

  // leave left mouse button !
  while( (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0) NOTHING;
  
  char achrMessage[ 256];
  // prepare message
  sprintf( achrMessage, "Press new %s button for action:\n\"%s\"", strFirstOrSecond,
    m_listButtonActions.GetItemText( m_iSelectedAction, 0));
  // set message string to dialog
  m_strPressNewButton = achrMessage;
  // activate string in text control
	UpdateData( FALSE);
  
  // enable application
  AfxGetMainWnd()->EnableWindow();
	AfxGetMainWnd()->SetActiveWindow();

  // enable direct input
  _pInput->EnableInput(m_hWnd);
  // initial reading of all available inputs
  _pInput->GetInput(FALSE);
  // as long as direct input is enabled
  while( _pInput->IsInputEnabled())
  {
    // for all possible buttons
    for( INDEX iButton=0; iButton<_pInput->GetAvailableButtonsCount(); iButton++)
    {
      // if pressed
      if( _pInput->GetButtonState( iButton)
        || (iButton==KID_MOUSE1 && (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0)
        || (iButton==KID_MOUSE2 && (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0)
        || (iButton==KID_MOUSE3 && (GetAsyncKeyState(VK_MBUTTON) & 0x8000) != 0))
      {
        // disable direct input
        _pInput->DisableInput();
        // if new button is mounted allready, set owner's action mounting to "key none"
        FOREACHINLIST( CButtonAction, ba_lnNode, m_ctrlControls.ctrl_lhButtonActions, itButtonAction)
        {
          if( itButtonAction->ba_iFirstKey  == iButton) itButtonAction->ba_iFirstKey  = KID_NONE;
          if( itButtonAction->ba_iSecondKey == iButton) itButtonAction->ba_iSecondKey = KID_NONE;
        }

        CButtonAction *pbaCurrent = GetSelectedButtonAction();
        if( pbaCurrent != NULL)
        {
          if( strFirstOrSecond == CTString("first") )
          {
            pbaCurrent->ba_iFirstKey = iButton;
          }
          else
          {
            pbaCurrent->ba_iSecondKey = iButton;
          }
        }
        // refresh list control
        FillActionsList();
      	// refresh first button edit control
        SetFirstAndSecondButtonNames();
  	    
        // disable application
        AfxGetMainWnd()->EnableWindow( FALSE);
        // activate dialog
        SetActiveWindow();
        EnableWindow();
        // activate actions list
        m_listButtonActions.SetFocus();
        // prevent reselecting edit control
        MSG message;
        // peek and remove all mouse messages from message queue
        while( PeekMessage( &message, m_hWnd, WM_MOUSEFIRST, WM_MOUSELAST, PM_REMOVE)) NOTHING;
        // peek and remove all keyboard messages from message queue
        while( PeekMessage( &message, m_hWnd, WM_KEYFIRST, WM_KEYLAST, PM_REMOVE)) NOTHING;
      }
    }
  }
  ASSERT( !_pInput->IsInputEnabled());
  // try to select same item that was selected before key binding
  if( m_iSelectedAction != -1)
  {
    m_listButtonActions.SetItemState( m_iSelectedAction, 
      LVIS_FOCUSED|LVIS_SELECTED|LVIS_DROPHILITED, LVIS_FOCUSED|LVIS_SELECTED|LVIS_DROPHILITED);
    m_listButtonActions.EnsureVisible( m_iSelectedAction, FALSE);
  }
  // delete press button message
  m_strPressNewButton = "";
  // notice change
	UpdateData( FALSE);
}

void CDlgPlayerControls::OnSetfocusEditFirstControl() 
{
  // activates press key behaviour (rather complicated one) for first button
  ActivatePressKey( "first");
}

void CDlgPlayerControls::OnSetfocusEditSecondControl() 
{
  // activates press key behaviour (rather complicated one) for second button
  ActivatePressKey( "second");
}

CButtonAction *CDlgPlayerControls::GetSelectedButtonAction() 
{
  // get selected action
  m_iSelectedAction = m_listButtonActions.GetNextItem( -1, LVNI_SELECTED);
  // if there is valid selection
  if( m_iSelectedAction != -1)
  {
    INDEX iCurrent = 0;
    FOREACHINLIST( CButtonAction, ba_lnNode, m_ctrlControls.ctrl_lhButtonActions, itButtonAction)
    {
      if( iCurrent == m_iSelectedAction) return itButtonAction;
      iCurrent++;
    }
  }
  return NULL;
}

void CDlgPlayerControls::OnFirstControlNone() 
{
  CButtonAction *pbaCurrent = GetSelectedButtonAction();
  if( pbaCurrent != NULL)
  {
    // set none action key
    pbaCurrent->ba_iFirstKey = KID_NONE;
    // refresh list control
    FillActionsList();
    // refresh first button edit control
    SetFirstAndSecondButtonNames();
    // activate actions list
    m_listButtonActions.SetFocus();
  }
}

void CDlgPlayerControls::OnSecondControlNone() 
{
  CButtonAction *pbaCurrent = GetSelectedButtonAction();
  if( pbaCurrent != NULL)
  {
    // set none action key
    pbaCurrent->ba_iSecondKey = KID_NONE;
    // refresh list control
    FillActionsList();
    // refresh first button edit control
    SetFirstAndSecondButtonNames();
    // activate actions list
    m_listButtonActions.SetFocus();
  }
}

void CDlgPlayerControls::OnDefault() 
{
	// switch controls to default
  m_ctrlControls.SwitchToDefaults();


  // refresh list control
  FillActionsList();
  // refresh first button edit control
  SetFirstAndSecondButtonNames();
  // activate actions list
  m_listButtonActions.SetFocus();

  // refresh axis list
  FillAxisList();
  // set axis attributes
  UpdateData( FALSE);
}

void CDlgPlayerControls::OnSelchangeControlerAxis() 
{
  // get newly selected controler axis
  INDEX iNewControler = m_comboControlerAxis.GetCurSel();
  // must be valid
  ASSERT( iNewControler != CB_ERR);
    
  // get selected item
  INDEX iActiveAxis = m_listAxisActions.GetNextItem( -1, LVNI_SELECTED);
  // change mounting controler for selected axis
  m_ctrlControls.ctrl_aaAxisActions[ iActiveAxis].aa_iAxisAction = iNewControler;
  // refresh axis list control
  FillAxisList();

  // pickup current values of controler attributes
  UpdateData( TRUE);
  // refresh axis-conected controls (dialog recives data)
  UpdateData( FALSE);
}

void CDlgPlayerControls::OnMoveControlUp() 
{
  // get selected item
  INDEX iSelectedButton = m_listButtonActions.GetNextItem( -1, LVNI_SELECTED);
  if( iSelectedButton == 0) return;
  // find member to move up
  INDEX iCurrent = 0;
  CButtonAction *pbaButtonToMove = NULL;

  {FOREACHINLIST( CButtonAction, ba_lnNode, m_ctrlControls.ctrl_lhButtonActions, itButtonAction)
  {
    if( iCurrent == iSelectedButton)
    {
      // remove it from list
      itButtonAction->ba_lnNode.Remove();
      pbaButtonToMove = &itButtonAction.Current();
      break;
    }
    iCurrent++;
  }}
  ASSERT( pbaButtonToMove != NULL);

  // insert removed member again but before its predcessor
  iCurrent = 0;
  {FOREACHINLIST( CButtonAction, ba_lnNode, m_ctrlControls.ctrl_lhButtonActions, itButtonAction)
  {
    if( iCurrent == (iSelectedButton-1) )
    {
      itButtonAction->ba_lnNode.AddBefore( pbaButtonToMove->ba_lnNode);
      break;
    }
    iCurrent++;
  }}
  // refresh list control
  FillActionsList();
  // get no of items
  INDEX iButtonsCt = m_listButtonActions.GetItemCount();
  // get no of items
  for( INDEX iListItem=0; iListItem<iButtonsCt; iListItem++)
  {
    m_listButtonActions.SetItemState( iListItem, 0, LVIS_FOCUSED|LVIS_SELECTED|LVIS_DROPHILITED);
  }

  // select wanted item
  m_listButtonActions.SetItemState( iSelectedButton-1, 
    LVIS_FOCUSED|LVIS_SELECTED|LVIS_DROPHILITED, LVIS_FOCUSED|LVIS_SELECTED|LVIS_DROPHILITED);
  m_listButtonActions.EnsureVisible( iSelectedButton-1, FALSE);
  m_listButtonActions.SetFocus();
}

void CDlgPlayerControls::OnMoveControlDown() 
{
  // get no of items
  INDEX iButtonsCt = m_listButtonActions.GetItemCount();
  // get selected item
  INDEX iSelectedButton = m_listButtonActions.GetNextItem( -1, LVNI_SELECTED);
  if( iSelectedButton >= (iButtonsCt-1) ) return;
  // find member to move down
  INDEX iCurrent = 0;
  CButtonAction *pbaButtonToMove = NULL;
  {FOREACHINLIST( CButtonAction, ba_lnNode, m_ctrlControls.ctrl_lhButtonActions, itButtonAction)
  {
    if( iCurrent == iSelectedButton)
    {
      // remove it from list
      itButtonAction->ba_lnNode.Remove();
      pbaButtonToMove = &itButtonAction.Current();
      break;
    }
    iCurrent++;
  }}
  ASSERT( pbaButtonToMove != NULL);

  // insert removed member again but before its predcessor
  iCurrent = 0;
  {FOREACHINLIST( CButtonAction, ba_lnNode, m_ctrlControls.ctrl_lhButtonActions, itButtonAction)
  {
    if( iCurrent == (iSelectedButton) )
    {
      itButtonAction->ba_lnNode.AddAfter( pbaButtonToMove->ba_lnNode);
      break;
    }
    iCurrent++;
  }}
  // refresh list control
  FillActionsList();
  // get no of items
  for( INDEX iListItem=0; iListItem<iButtonsCt; iListItem++)
  {
    m_listButtonActions.SetItemState( iListItem, 0, LVIS_FOCUSED|LVIS_SELECTED|LVIS_DROPHILITED);
  }

  // select wanted item
  m_listButtonActions.SetItemState( iSelectedButton+1, 
    LVIS_FOCUSED|LVIS_SELECTED|LVIS_DROPHILITED, LVIS_FOCUSED|LVIS_SELECTED|LVIS_DROPHILITED);
  m_listButtonActions.EnsureVisible( iSelectedButton+1, FALSE); 
  m_listButtonActions.SetFocus();
}

void CDlgPlayerControls::OnButtonActionAdd() 
{
  m_listButtonActions.OnButtonActionAdd();
}

void CDlgPlayerControls::OnButtonActionEdit() 
{
  m_listButtonActions.OnButtonActionEdit();
}

void CDlgPlayerControls::OnButtonActionRemove() 
{
  m_listButtonActions.OnButtonActionRemove();
}
