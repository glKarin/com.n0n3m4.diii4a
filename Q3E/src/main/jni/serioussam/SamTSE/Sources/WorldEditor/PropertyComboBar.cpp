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

// PropertyComboBar.cpp : implementation file
//

#include "stdafx.h"
#include "PropertyComboBar.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPropertyComboBar dialog

BOOL CPropertyComboBar::Create( CWnd* pParentWnd, UINT nIDTemplate,
                                UINT nStyle, UINT nID, BOOL bChange)
{
  if(!CDialogBar::Create(pParentWnd,nIDTemplate,nStyle,nID))
  {
    return FALSE;
  }

  m_Size = m_sizeDefault;
  
  // subclass property combo box
  m_PropertyComboBox.SubclassDlgItem( IDC_PROPERTYCOMBO, this);
  // subclass enum combo box
  m_EditEnumComboBox.SubclassDlgItem( IDC_EDIT_ENUM, this);
  // subclass edit string control
  m_EditStringCtrl.SubclassDlgItem( IDC_EDIT_STRING, this);
  // subclass edit float control
  m_EditFloatCtrl.SubclassDlgItem( IDC_EDIT_FLOAT, this);
  // for andgle 3D
  m_EditHeading.SubclassDlgItem( IDC_EDIT_HEADING, this);
  m_EditPitch.SubclassDlgItem( IDC_EDIT_PITCH, this);
  m_EditBanking.SubclassDlgItem( IDC_EDIT_BANKING, this);
  // subclass edit BBox control
  m_XCtrlAxisRadio.SubclassDlgItem( IDC_AXIS_X, this);
  m_YCtrlAxisRadio.SubclassDlgItem( IDC_AXIS_Y, this);
  m_ZCtrlAxisRadio.SubclassDlgItem( IDC_AXIS_Z, this);
  m_EditBBoxMinCtrl.SubclassDlgItem( IDC_EDIT_BBOX_MIN, this);
  m_EditBBoxMaxCtrl.SubclassDlgItem( IDC_EDIT_BBOX_MAX, this);
  // subclass edit index control
  m_EditIndexCtrl.SubclassDlgItem( IDC_EDIT_INDEX, this);
  // subclass edit boolean control
  m_EditBoolCtrl.SubclassDlgItem( IDC_EDIT_BOOL, this);
  // subclass edit color control
  m_EditColorCtrl.SetPickerType( CColoredButton::PT_MFC);
  m_EditColorCtrl.SubclassDlgItem( IDC_EDIT_COLOR, this);

  // subclass flag field property ctrl
  m_ctrlEditFlags.SubclassDlgItem( ID_FLAGS_PROPERTY, this);

  // subclass browse file control
  m_BrowseFileCtrl.SubclassDlgItem( IDC_BROWSE_FILE, this);

  // subclass difficulty spawn flags
  m_EditEasySpawn.SubclassDlgItem( IDC_EASY, this);
  m_EditNormalSpawn.SubclassDlgItem( IDC_NORMAL, this);
  m_EditHardSpawn.SubclassDlgItem( IDC_HARD, this);
  m_EditExtremeSpawn.SubclassDlgItem( IDC_EXTREME, this);
  m_EditDifficulty_1.SubclassDlgItem( IDC_DIFFICULTY_1, this);
  m_EditDifficulty_2.SubclassDlgItem( IDC_DIFFICULTY_2, this);
  m_EditDifficulty_3.SubclassDlgItem( IDC_DIFFICULTY_3, this);
  m_EditDifficulty_4.SubclassDlgItem( IDC_DIFFICULTY_4, this);
  m_EditDifficulty_5.SubclassDlgItem( IDC_DIFFICULTY_5, this);

  // subclass game mode spawn flags
  m_EditSingleSpawn.SubclassDlgItem( IDC_SINGLE, this);
  m_EditCooperativeSpawn.SubclassDlgItem( IDC_COOPERATIVE, this);
  m_EditDeathMatchSpawn.SubclassDlgItem( IDC_DEATHMATCH, this);
  m_EditGameMode_1.SubclassDlgItem( IDC_GAME_MODE_1, this);
  m_EditGameMode_2.SubclassDlgItem( IDC_GAME_MODE_2, this);
  m_EditGameMode_3.SubclassDlgItem( IDC_GAME_MODE_3, this);
  m_EditGameMode_4.SubclassDlgItem( IDC_GAME_MODE_4, this);
  m_EditGameMode_5.SubclassDlgItem( IDC_GAME_MODE_5, this);
  m_EditGameMode_6.SubclassDlgItem( IDC_GAME_MODE_6, this);

  // set dialog ptrs to controls
  m_PropertyComboBox.SetDialogPtr( this);
  m_EditEnumComboBox.SetDialogPtr( this);
  m_EditStringCtrl.SetDialogPtr( this);
  m_EditFloatCtrl.SetDialogPtr( this);
  m_EditHeading.SetDialogPtr( this);
  m_EditPitch.SetDialogPtr( this);
  m_EditBanking.SetDialogPtr( this);
  m_EditIndexCtrl.SetDialogPtr( this);
  m_EditBBoxMinCtrl.SetDialogPtr( this);
  m_EditBBoxMaxCtrl.SetDialogPtr( this);
  m_XCtrlAxisRadio.SetDialogPtr( this);
  m_YCtrlAxisRadio.SetDialogPtr( this);
  m_ZCtrlAxisRadio.SetDialogPtr( this);
  m_EditBoolCtrl.SetDialogPtr( this);
  m_BrowseFileCtrl.SetDialogPtr( this);
  
  m_EditEasySpawn.SetDialogPtr( this);
  m_EditNormalSpawn.SetDialogPtr( this);
  m_EditHardSpawn.SetDialogPtr( this);
  m_EditExtremeSpawn.SetDialogPtr( this);
  m_EditDifficulty_1.SetDialogPtr( this);
  m_EditDifficulty_2.SetDialogPtr( this);
  m_EditDifficulty_3.SetDialogPtr( this);
  m_EditDifficulty_4.SetDialogPtr( this);
  m_EditDifficulty_5.SetDialogPtr( this);
  
  m_EditSingleSpawn.SetDialogPtr( this);
  m_EditCooperativeSpawn.SetDialogPtr( this);
  m_EditDeathMatchSpawn.SetDialogPtr( this);
  m_EditGameMode_1.SetDialogPtr( this);
  m_EditGameMode_2.SetDialogPtr( this);
  m_EditGameMode_3.SetDialogPtr( this);
  m_EditGameMode_4.SetDialogPtr( this);
  m_EditGameMode_5.SetDialogPtr( this);
  m_EditGameMode_6.SetDialogPtr( this);
  m_ctrlEditFlags.SetDialogPtr( this);

  // set font for properties combo
  m_PropertyComboBox.SetFont(&theApp.m_Font);
  // set font for enum combo
  m_EditEnumComboBox.SetFont(&theApp.m_Font);

  return TRUE;
}

BEGIN_MESSAGE_MAP(CPropertyComboBar, CDialogBar)
	//{{AFX_MSG_MAP(CPropertyComboBar)
	ON_WM_HSCROLL()
	//}}AFX_MSG_MAP
	ON_UPDATE_COMMAND_UI(IDC_BROWSE_FILE, OnUpdateBrowseFile)
	ON_UPDATE_COMMAND_UI(IDC_NO_FILE, OnUpdateNoFile)
	ON_UPDATE_COMMAND_UI(IDC_NO_TARGET, OnUpdateNoTarget)
	ON_UPDATE_COMMAND_UI(IDC_EDIT_COLOR, OnUpdateEditColor)
	ON_UPDATE_COMMAND_UI(ID_FLAGS_PROPERTY, OnUpdateEditFlags)
	ON_COMMAND(IDC_NO_FILE, OnNoFile)
	ON_COMMAND(IDC_NO_TARGET, OnNoTarget)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPropertyComboBar message handlers

//--------------------------------------------------------------------------------------------
void CPropertyComboBar::SelectAxisRadio(CWnd *pwndToSelect)
{
  // deselect all three axis radios
  m_XCtrlAxisRadio.SetCheck( 0);
  m_YCtrlAxisRadio.SetCheck( 0);
  m_ZCtrlAxisRadio.SetCheck( 0);
  // select right one
  if( pwndToSelect == &m_XCtrlAxisRadio) {m_XCtrlAxisRadio.SetCheck( 1); m_iXYZAxis=1;};
  if( pwndToSelect == &m_YCtrlAxisRadio) {m_YCtrlAxisRadio.SetCheck( 1); m_iXYZAxis=2;};
  if( pwndToSelect == &m_ZCtrlAxisRadio) {m_ZCtrlAxisRadio.SetCheck( 1); m_iXYZAxis=3;};
}

void CPropertyComboBar::DoDataExchange(CDataExchange* pDX) 
{
  CWorldEditorDoc *pDoc = theApp.GetDocument();
  if( pDoc == NULL)
  {
    return;
  }
  
  // if dialog is recieving data
  if( pDX->m_bSaveAndValidate == FALSE)
  {
    SetIntersectingEntityClassName();
    SetIntersectingFileName();
    // set spawn flags...
    // obtain selected property ID ptr
    CPropertyID *ppidProperty = GetSelectedProperty();
    // if there is valid property selected
    if( ppidProperty != NULL)
    {
      // reset selection entities spawn on and off masks
      ULONG ulSpawnOn = MAX_ULONG;
      ULONG ulSpawnOff = MAX_ULONG;
      // for each of the selected entities
      FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
      {
        // intersect current mask with spawn mask of all selected entities
        ulSpawnOn &= iten->GetSpawnFlags();
        ulSpawnOff&= ~iten->GetSpawnFlags();
      }

#define SET_SPAWN_FLAG( flag, ctrl) \
      if((ulSpawnOn & (flag)) && !(ulSpawnOff & (flag))) ctrl.SetCheck( 1); \
      else if(!(ulSpawnOn & (flag)) && (ulSpawnOff & (flag))) ctrl.SetCheck( 0); \
      else ctrl.SetCheck( 2);

      // set states of all spawn flags using calculated selected entities masks
      SET_SPAWN_FLAG( SPF_EASY, m_EditEasySpawn);
      SET_SPAWN_FLAG( SPF_NORMAL, m_EditNormalSpawn);
      SET_SPAWN_FLAG( SPF_HARD, m_EditHardSpawn);
      SET_SPAWN_FLAG( SPF_EXTREME, m_EditExtremeSpawn);
      SET_SPAWN_FLAG( SPF_EXTREME<<1, m_EditDifficulty_1);
      SET_SPAWN_FLAG( SPF_EXTREME<<2, m_EditDifficulty_2);
      SET_SPAWN_FLAG( SPF_EXTREME<<3, m_EditDifficulty_3);
      SET_SPAWN_FLAG( SPF_EXTREME<<4, m_EditDifficulty_4);
      SET_SPAWN_FLAG( SPF_EXTREME<<5, m_EditDifficulty_5);

      SET_SPAWN_FLAG( SPF_SINGLEPLAYER, m_EditSingleSpawn);
      SET_SPAWN_FLAG( SPF_DEATHMATCH, m_EditDeathMatchSpawn);
      SET_SPAWN_FLAG( SPF_COOPERATIVE, m_EditCooperativeSpawn);
      SET_SPAWN_FLAG( SPF_COOPERATIVE<<1, m_EditGameMode_1);
      SET_SPAWN_FLAG( SPF_COOPERATIVE<<2, m_EditGameMode_2);
      SET_SPAWN_FLAG( SPF_COOPERATIVE<<3, m_EditGameMode_3);
      SET_SPAWN_FLAG( SPF_COOPERATIVE<<4, m_EditGameMode_4);
      SET_SPAWN_FLAG( SPF_COOPERATIVE<<5, m_EditGameMode_5);
      SET_SPAWN_FLAG( SPF_COOPERATIVE<<6, m_EditGameMode_6);
    }
  }
  
  DDX_Text(pDX, IDC_FLOAT_RANGE_T, m_strFloatRange);
  DDX_Text(pDX, IDC_INDEX_RANGE_T, m_strIndexRange);
  DDX_Text(pDX, IDC_CHOOSE_COLOR_T, m_strChooseColor);
  DDX_Text(pDX, IDC_FILE_NAME_T, m_strFileName);
  DDX_Text(pDX, IDC_ENTITY_CLASS, m_strEntityClass);
  DDX_Text(pDX, IDC_ENTITY_NAME, m_strEntityName);
  DDX_Text(pDX, IDC_ENTITY_DESCRIPTION, m_strEntityDescription);
	
  if( m_EditBBoxMinCtrl.IsWindowVisible())
  {
    DDX_SkyFloat(pDX, IDC_EDIT_BBOX_MIN, m_fEditingBBoxMin);
    DDX_SkyFloat(pDX, IDC_EDIT_BBOX_MAX, m_fEditingBBoxMax);
  }
  if( m_EditStringCtrl.IsWindowVisible())
  {
    DDX_Text(pDX, IDC_EDIT_STRING, m_strEditingString);
  }
  if( m_EditFloatCtrl.IsWindowVisible())
  {
    DDX_SkyFloat(pDX, IDC_EDIT_FLOAT, m_fEditingFloat);
  }
  if( m_EditHeading.IsWindowVisible())
  {
    DDX_SkyFloat(pDX, IDC_EDIT_HEADING, m_fEditingHeading);
  }
  if( m_EditPitch.IsWindowVisible())
  {
    DDX_SkyFloat(pDX, IDC_EDIT_PITCH, m_fEditingPitch);
  }
  if( m_EditBanking.IsWindowVisible())
  {
    DDX_SkyFloat(pDX, IDC_EDIT_BANKING, m_fEditingBanking);
  }
  if( m_EditIndexCtrl.IsWindowVisible())
  {
    DDX_Text(pDX, IDC_EDIT_INDEX, m_iEditingIndex);
  }

  // if dialog is giving data
  if( pDX->m_bSaveAndValidate != FALSE)
  {
    // obtain selected property ID ptr
    CPropertyID *ppidProperty = GetSelectedProperty();
    // if there is valid property selected
    if( ppidProperty != NULL)
    {
      // see type of changing property
      switch( ppidProperty->pid_eptType)
      {
      // if we are changing flag field
      case CEntityProperty::EPT_FLAGS:
        {
          // for each of the selected entities
          FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
          {
            // obtain property ptr
            CEntityProperty *penpProperty = iten->PropertyForName( ppidProperty->pid_strName);
            // discard old entity settings
            iten->End();
            // set new flag value
            m_ctrlEditFlags.ApplyChange( ENTITYPROPERTY( &*iten, penpProperty->ep_slOffset, ULONG));
            // apply new entity settings
            iten->Initialize();
          }
          break;
        }
      // if we are changing enum property
      case CEntityProperty::EPT_ENUM:
        {
          // get currently selected combo member
          INDEX iSelectedComboMember = m_EditEnumComboBox.GetCurSel();
          // it must exist
          if( iSelectedComboMember == CB_ERR) return;
          // get enum ID to be set for all entities
          INDEX iSelectedEnumID = m_EditEnumComboBox.GetItemData(iSelectedComboMember);

          // for each of the selected entities
          FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
          {
            // obtain property ptr
            CEntityProperty *penpProperty = iten->PropertyForName( ppidProperty->pid_strName);
            // discard old entity settings
            iten->End();
            // set new enum value
            ENTITYPROPERTY( &*iten, penpProperty->ep_slOffset, INDEX) = iSelectedEnumID;
            // apply new entity settings
            iten->Initialize();
          }
          break;
        }
      case CEntityProperty::EPT_ANIMATION:
        {
          // get currently selected combo member
          INDEX iSelectedComboMember = m_EditEnumComboBox.GetCurSel();
          // it must exist
          if( iSelectedComboMember == CB_ERR) return;
          // get animation to be set for all entities
          INDEX iSelectedAnimation = m_EditEnumComboBox.GetItemData(iSelectedComboMember);

          // for each of the selected entities
          FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
          {
            // obtain property ptr
            CEntityProperty *penpProperty = iten->PropertyForName( ppidProperty->pid_strName);
            // discard old entity settings
            iten->End();
            // set new animation
            ENTITYPROPERTY( &*iten, penpProperty->ep_slOffset, INDEX) = iSelectedAnimation;
            // apply new entity settings
            iten->Initialize();
          }
          break;
        }
      // if we are changing illumination type property
      case CEntityProperty::EPT_ILLUMINATIONTYPE:
        {
          // get currently selected combo member
          INDEX iSelectedComboMember = m_EditEnumComboBox.GetCurSel();
          // it must exist
          if( iSelectedComboMember == CB_ERR) return;
          // get illumination type to be set for all entities
          INDEX iIlluminationType = m_EditEnumComboBox.GetItemData(iSelectedComboMember);

          // for each of the selected entities
          FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
          {
            // obtain property ptr
            CEntityProperty *penpProperty = iten->PropertyForName( ppidProperty->pid_strName);
            // discard old entity settings
            iten->End();
            // set new illumination type value
            ENTITYPROPERTY( &*iten, penpProperty->ep_slOffset, INDEX) = iIlluminationType;
            // apply new entity settings
            iten->Initialize();
          }
          break;
        }
      // if we are changing string property
      case CEntityProperty::EPT_STRING:
      case CEntityProperty::EPT_STRINGTRANS:
        {
          // for each of the selected entities
          FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
          {
            // obtain property ptr
            CEntityProperty *penpProperty = iten->PropertyForName( ppidProperty->pid_strName);
            // discard old entity settings
            iten->End();
            // set new string
            ENTITYPROPERTY( &*iten, penpProperty->ep_slOffset, CTString) = CStringA(m_strEditingString);
            // apply new entity settings
            iten->Initialize();
          }
          // mark that document changed so that OnIdle on CSG destination combo would
          // refresh combo entries (because we could be changing world name)
          pDoc->m_chDocument.MarkChanged();
          break;
        }
      // if we are changing float, range or angle property
      case CEntityProperty::EPT_FLOAT:
      case CEntityProperty::EPT_RANGE:
      case CEntityProperty::EPT_ANGLE:
      case CEntityProperty::EPT_ANGLE3D:
        {
          // for each of the selected entities
          FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
          {
            // discard old entity settings
            iten->End();
            // obtain property ptr
            CEntityProperty *penpProperty = iten->PropertyForName( ppidProperty->pid_strName);
            // if we are editing angle property
            if( ppidProperty->pid_eptType == CEntityProperty::EPT_ANGLE3D)
            {
              // set new angle 3d
              ENTITYPROPERTY( &*iten, penpProperty->ep_slOffset, ANGLE3D) = 
                ANGLE3D(AngleDeg(m_fEditingHeading), AngleDeg(m_fEditingPitch),
                        AngleDeg(m_fEditingBanking));
            }
            else if(ppidProperty->pid_eptType == CEntityProperty::EPT_ANGLE)
            {
              // set new angle
              ENTITYPROPERTY( &*iten, penpProperty->ep_slOffset, FLOAT) = AngleDeg(m_fEditingFloat);
            }
            else
            {
              // set new float or range
              ENTITYPROPERTY( &*iten, penpProperty->ep_slOffset, FLOAT) = m_fEditingFloat;
            }
            // apply new entity settings
            iten->Initialize();
          }
          break;
        }
      case CEntityProperty::EPT_FLOATAABBOX3D:
        {
          // for each of the selected entities
          FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
          {
            // obtain property ptr
            CEntityProperty *penpProperty = iten->PropertyForName( ppidProperty->pid_strName);
            // get old (pre-changed) value for bounding box
            FLOATaabbox3D bboxOld = ENTITYPROPERTY( &*iten, penpProperty->ep_slOffset,
                                                    FLOATaabbox3D);
            // get its min and max vectors
            FLOAT3D vMin = bboxOld.Min();
            FLOAT3D vMax = bboxOld.Max();
            // set new min and max bbox values for currently selected axis
            vMin( m_iXYZAxis) = m_fEditingBBoxMin;
            vMax( m_iXYZAxis) = m_fEditingBBoxMax;
            // create new bbox and set it into property
            FLOATaabbox3D bboxNew( vMin, vMax);
            // discard old entity settings
            iten->End();
            ENTITYPROPERTY( &*iten, penpProperty->ep_slOffset, FLOATaabbox3D) = bboxNew;
            // apply new entity settings
            iten->Initialize();
          }
          break;
        }
      case CEntityProperty::EPT_INDEX:
        {
          // for each of the selected entities
          FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
          {
            // obtain property ptr
            CEntityProperty *penpProperty = iten->PropertyForName( ppidProperty->pid_strName);
            INDEX iNewValue = m_iEditingIndex;
            // discard old entity settings
            iten->End();
            // set new index
            ENTITYPROPERTY( &*iten, penpProperty->ep_slOffset, INDEX) = iNewValue;
            // apply new entity settings
            iten->Initialize();
          }
          break;
        }
      // if we are changing file name property
      case CEntityProperty::EPT_FILENAME:
        {
          // file name changing function is done inside CBrowseFile.cpp
          break;
        }
      case CEntityProperty::EPT_COLOR:
        {
          break;
        }
      // if we are changing bool property
      case CEntityProperty::EPT_BOOL:
        {
          // get state of check box
          INDEX iCheckBox = m_EditBoolCtrl.GetCheck();
          // must be 0 or 1
          if( iCheckBox != 2)
          {
            // for each of the selected entities
            FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
            {
              // obtain property ptr
              CEntityProperty *penpProperty = iten->PropertyForName( ppidProperty->pid_strName);
              // discard old entity settings
              iten->End();
              // set new boolean value
              ENTITYPROPERTY( &*iten, penpProperty->ep_slOffset, BOOL) = iCheckBox;
              // apply new entity settings
              iten->Initialize();
            }
          }
          break;
        }
      // if we are changing entity ptr property
      case CEntityProperty::EPT_ENTITYPTR:
      case CEntityProperty::EPT_PARENT:
        {
          // get currently selected combo member
          INDEX iSelectedComboMember = m_EditEnumComboBox.GetCurSel();
          // it must exist
          if( iSelectedComboMember == CB_ERR) return;
          // get entity ptr to be set for all entities
          CEntity *penEntity = (CEntity *)m_EditEnumComboBox.GetItemData(iSelectedComboMember);

          // for each of the selected entities
          FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
          {
            if( ppidProperty->pid_eptType == CEntityProperty::EPT_PARENT)
            {
              iten->SetParent( penEntity);
            }
            else
            {
              // obtain property ptr
              CEntityProperty *penpProperty = iten->PropertyForName( ppidProperty->pid_strName);
              // discard old entity settings
              iten->End();
              // set new entity ptr value
              ENTITYPROPERTY( &*iten, penpProperty->ep_slOffset, CEntityPointer) = penEntity;
              // apply new entity settings
              iten->Initialize();
            }
          }
          break;
        }
      // if we are changing entity ptr property
      case CEntityProperty::EPT_SPAWNFLAGS:
        {
          // obtain spawn on and off masks
          ULONG ulBitsToClear = MAX_ULONG;
          ULONG ulBitsToSet = 0;

#define GET_SPAWN_MASKS( flag, ctrl) \
          if( ctrl.GetCheck() == 0) ulBitsToClear &= ~(flag); \
          if( ctrl.GetCheck() == 1) ulBitsToSet |= (flag);

          // look at spawn checks and create masks of bits to set and to clear
          GET_SPAWN_MASKS( SPF_EASY, m_EditEasySpawn);
          GET_SPAWN_MASKS( SPF_NORMAL, m_EditNormalSpawn);
          GET_SPAWN_MASKS( SPF_HARD, m_EditHardSpawn);
          GET_SPAWN_MASKS( SPF_EXTREME, m_EditExtremeSpawn);
          GET_SPAWN_MASKS( SPF_EXTREME<<1, m_EditDifficulty_1);
          GET_SPAWN_MASKS( SPF_EXTREME<<2, m_EditDifficulty_2);
          GET_SPAWN_MASKS( SPF_EXTREME<<3, m_EditDifficulty_3);
          GET_SPAWN_MASKS( SPF_EXTREME<<4, m_EditDifficulty_4);
          GET_SPAWN_MASKS( SPF_EXTREME<<5, m_EditDifficulty_5);

          GET_SPAWN_MASKS( SPF_SINGLEPLAYER, m_EditSingleSpawn);
          GET_SPAWN_MASKS( SPF_DEATHMATCH, m_EditDeathMatchSpawn);
          GET_SPAWN_MASKS( SPF_COOPERATIVE, m_EditCooperativeSpawn);
          GET_SPAWN_MASKS( SPF_COOPERATIVE<<1, m_EditGameMode_1);
          GET_SPAWN_MASKS( SPF_COOPERATIVE<<2, m_EditGameMode_2);
          GET_SPAWN_MASKS( SPF_COOPERATIVE<<3, m_EditGameMode_3);
          GET_SPAWN_MASKS( SPF_COOPERATIVE<<4, m_EditGameMode_4);
          GET_SPAWN_MASKS( SPF_COOPERATIVE<<5, m_EditGameMode_5);
          GET_SPAWN_MASKS( SPF_COOPERATIVE<<6, m_EditGameMode_6);

          // for each of the selected entities set spawn flags
          FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
          {
            // clear and set curently selected spawn flags to all selected entities
            iten->SetSpawnFlags(iten->GetSpawnFlags() & ulBitsToClear);
            iten->SetSpawnFlags(iten->GetSpawnFlags() | ulBitsToSet);
          }
          break;
        }
      default:
        {
          ASSERTALWAYS("Unknown property type");
        }
      }
      // mark that document is changed
      pDoc->SetModifiedFlag( TRUE);
      // redraw all views
      pDoc->UpdateAllViews( NULL);
    }
  }
	
	CDialogBar::DoDataExchange(pDX);
}
//--------------------------------------------------------------------------------------------
CSize CPropertyComboBar::CalcDynamicLayout(int nLength, DWORD nMode)
{
  CSize csResult;
  // Return default if it is being docked or floated
  if ((nMode & LM_VERTDOCK) || (nMode & LM_HORZDOCK))
  {
    if (nMode & LM_STRETCH) // if not docked stretch to fit
    {
      csResult = CSize((nMode & LM_HORZ) ? 32767 : m_Size.cx,
                       (nMode & LM_HORZ) ? m_Size.cy : 32767);
    }
    else
    {
      csResult = m_Size;
    }
  }
  else if (nMode & LM_MRUWIDTH)
  {
    csResult = m_Size;
  }
  // In all other cases, accept the dynamic length
  else
  {
    if (nMode & LM_LENGTHY)
    {
      // Note that we don't change m_Size.cy because we disabled vertical sizing
      csResult = CSize( m_Size.cx, m_Size.cy = nLength);
    }
    else
    {
      csResult = CSize( m_Size.cx = nLength, m_Size.cy);
    }
  }
  
#define CTRLS_LINE_H 22
#define CTRLS_RW  40
#define CTRLS_RH  44
#define CTRLS_YS  38+CTRLS_RH
#define CTRLS_YE  (CTRLS_YS+CTRLS_LINE_H)
#define CTRLS_YRS 28+CTRLS_RH
#define CTRLS_YRE (CTRLS_YRS+CTRLS_RH)
#define CTRLS_YTS 44+CTRLS_RH
#define CTRLS_YTE (CTRLS_YTS+CTRLS_LINE_H)
#define CTRLS_YCMRS CTRLS_YS-4        // y color radio mode start
#define CTRLS_YCSS  CTRLS_YCMRS+26    // y color sliders start
#define CTRLS_YNTS 64+CTRLS_RH        // no target button
#define CTRLS_YNTE (CTRLS_YNTS+CTRLS_LINE_H)
#define CTRLS_BUTTONW 40
  // set entity class text position
  GetDlgItem( IDC_ENTITY_CLASS)->MoveWindow( CRect( 8, 4, csResult.cx - 8, 4+14));
  // set entity name text position
  GetDlgItem( IDC_ENTITY_NAME)->MoveWindow( CRect( 8, 18, csResult.cx - 8, 18+14));
  // set entity description text position
  GetDlgItem( IDC_ENTITY_DESCRIPTION)->MoveWindow( CRect( 8, 32, csResult.cx - 8, 32+14));
  // set property combo size and position
  m_PropertyComboBox.MoveWindow( CRect( 8, 5+CTRLS_RH, csResult.cx - 8, 120+CTRLS_RH));
  // set enum combo size and position
  m_EditEnumComboBox.MoveWindow( CRect( 8, CTRLS_YS, csResult.cx - 8, 120+CTRLS_RH));
  // set edit string control size and position
  m_EditStringCtrl.MoveWindow( CRect( 8, CTRLS_YS, csResult.cx - 8, CTRLS_YE));  
  // set edit float control size and position
  m_EditFloatCtrl.MoveWindow( CRect( csResult.cx/2, CTRLS_YS, csResult.cx - 8, CTRLS_YE));
  // set edit BBox controls size and position
  GetDlgItem( IDC_AXIS_X)->MoveWindow( CRect( csResult.cx/2-CTRLS_RW*3/2, CTRLS_YRS,
                                              csResult.cx/2-CTRLS_RW/2, CTRLS_YRE));
  GetDlgItem( IDC_AXIS_Y)->MoveWindow( CRect( csResult.cx/2-CTRLS_RW/2, CTRLS_YRS,
                                              csResult.cx/2+CTRLS_RW/2, CTRLS_YRE) );
  GetDlgItem( IDC_AXIS_Z)->MoveWindow( CRect( csResult.cx/2+CTRLS_RW/2, CTRLS_YRS,
                                              csResult.cx/2+CTRLS_RW*3/2, CTRLS_YRE) );
  m_EditBBoxMinCtrl.MoveWindow( CRect( 8, CTRLS_YRE,
                                       csResult.cx/2-8, CTRLS_YRE+CTRLS_LINE_H) );
  m_EditBBoxMaxCtrl.MoveWindow( CRect( csResult.cx/2+8, CTRLS_YRE,
                                       csResult.cx-8, CTRLS_YRE+CTRLS_LINE_H) );
  // select x axis by default for editing bbox
  SelectAxisRadio( &m_XCtrlAxisRadio);
  // set edit index control size and position
  m_EditIndexCtrl.MoveWindow( CRect( csResult.cx/2, CTRLS_YS, csResult.cx - 8, CTRLS_YE));
  // set float range description message size and position
  GetDlgItem( IDC_FLOAT_RANGE_T)->MoveWindow( CRect( 8, CTRLS_YTS, csResult.cx/2, CTRLS_YTE));
  // set index range description message size and position
  GetDlgItem( IDC_INDEX_RANGE_T)->MoveWindow( CRect( 8, CTRLS_YTS, csResult.cx/2, csResult.cy));
  // set edit boolean control size and position
  m_EditBoolCtrl.MoveWindow( CRect( 8, CTRLS_YS, csResult.cx - 8, CTRLS_YE));

  // set float range description message size and position
  GetDlgItem( IDC_ANGLE3D_T)->MoveWindow( CRect( 8, CTRLS_YTS+4, csResult.cx/4-4, CTRLS_YTE+4));
  GetDlgItem( IDC_EDIT_HEADING)->MoveWindow( CRect( csResult.cx/4-4, CTRLS_YTS, csResult.cx/4*2-4, CTRLS_YTE));
  GetDlgItem( IDC_EDIT_PITCH)->MoveWindow( CRect( csResult.cx/4*2-4, CTRLS_YTS, csResult.cx/4*3-4, CTRLS_YTE));
  GetDlgItem( IDC_EDIT_BANKING)->MoveWindow( CRect( csResult.cx/4*3-4, CTRLS_YTS, csResult.cx-4, CTRLS_YTE));
  
  // set size and position of color picker
  m_EditColorCtrl.MoveWindow( CRect( 8, CTRLS_YCMRS, 128, CTRLS_YCMRS+(CTRLS_LINE_H*1.25)));

  // set size and position of flags array
  m_ctrlEditFlags.MoveWindow( CRect( 8, CTRLS_YCMRS+12, csResult.cx-4, CTRLS_YCMRS+(CTRLS_LINE_H*1.0)+12));

  // set color description message size and position
  GetDlgItem( IDC_CHOOSE_COLOR_T)->MoveWindow( CRect( 4, CTRLS_YCSS-4, 16, csResult.cy-4));
  // set browse file button size and position
  m_BrowseFileCtrl.MoveWindow( CRect( csResult.cx-CTRLS_BUTTONW-16, CTRLS_YS, csResult.cx - 24, CTRLS_YE));
  GetDlgItem( IDC_NO_FILE)->MoveWindow( CRect( csResult.cx-24, CTRLS_YS, csResult.cx - 8, CTRLS_YE));
  GetDlgItem( IDC_NO_TARGET)->MoveWindow( CRect( 8, CTRLS_YNTS, csResult.cx - 8, CTRLS_YNTE));
  // set browse file description message size and position
  GetDlgItem( IDC_FILE_NAME_T)->MoveWindow( CRect( 8, CTRLS_YTS, csResult.cx - CTRLS_BUTTONW-16, CTRLS_YTE));
  return csResult;
}

CPropertyID *CPropertyComboBar::GetSelectedProperty()
{
  INDEX iSelectedProperty = m_PropertyComboBox.GetCurSel();
  if( (iSelectedProperty == CB_ERR) || ( iSelectedProperty < 0) )
  {
    //ASSERTALWAYS( "GetSelectedProperty() must not be called if there are no joined properties");
    return NULL;
  }
  CPropertyID *ppidProperty = (CPropertyID *) 
    m_PropertyComboBox.GetItemData( iSelectedProperty);
  ASSERT( ppidProperty != (CPropertyID *) -1);
  return ppidProperty;
}

void CPropertyComboBar::SetFirstValidEmptyTargetProperty(CEntity *penTarget)
{
  CWorldEditorDoc *pDoc = theApp.GetDocument();
  if( pDoc == NULL) return;

  for( INDEX iItem=0; iItem<m_PropertyComboBox.GetCount(); iItem++)
  {
    CPropertyID *ppidProperty = (CPropertyID *) m_PropertyComboBox.GetItemData( iItem);
    ASSERT( ppidProperty != (CPropertyID *) -1);
    if( ppidProperty->pid_eptType == CEntityProperty::EPT_ENTITYPTR)
    {
      // for each of the selected entities
      {FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
      {
        // obtain property ptr
        CEntityProperty *penpProperty = iten->PropertyForName( ppidProperty->pid_strName);
        // check that all entities with this property target NULL and that supposed target is valid
        CEntity *penOldTarget = ENTITYPROPERTY( &*iten, penpProperty->ep_slOffset, CEntityPointer);
        BOOL bValidTarget = iten->IsTargetValid( penpProperty->ep_slOffset, penTarget);
        // if this ptr is already set
        if( penOldTarget==penTarget)
        {
          // don't do anything
          return;
        }
        // stop checking if ptr isn't NULL or if not valid target
        if( penOldTarget!=NULL || !bValidTarget)
        {
          continue;
        }
        // for each of the selected entities
        {FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
        {
          // discard old entity settings
          iten->End();
          // obtain property ptr
          CEntityProperty *penpProperty = iten->PropertyForName( ppidProperty->pid_strName);
          // set clicked entity as one that selected entity points to
          ENTITYPROPERTY( &*iten, penpProperty->ep_slOffset, CEntityPointer) = penTarget;
          // apply new entity settings
          iten->Initialize();
        }}
        m_PropertyComboBox.SetCurSel(iItem);
        m_PropertyComboBox.SelectProperty();
        pDoc->m_chSelections.MarkChanged();
        return;
      }}
    }
  }
}

void CPropertyComboBar::SelectPreviousEmptyTarget(void)
{
  CircleTargetProperties( -1, TRUE);
}

void CPropertyComboBar::SelectPreviousProperty(void)
{
  CircleTargetProperties( -1, FALSE);
}

void CPropertyComboBar::SelectNextEmptyTarget(void)
{
  CircleTargetProperties( 1, TRUE);
}

void CPropertyComboBar::SelectNextProperty(void)
{
  CircleTargetProperties( 1, FALSE);
}

void CPropertyComboBar::CircleTargetProperties(INDEX iDirection, BOOL bOnlyEmptyTargets)
{
  CWorldEditorDoc *pDoc = theApp.GetDocument();
  if( pDoc == NULL)
  {
    return;
  }
  INDEX ctProperties = m_PropertyComboBox.GetCount();
  // if no properties
  if( ctProperties == 0) return;
  // if selected is invalid
  INDEX iSelectedProperty = m_PropertyComboBox.GetCurSel();
  if( (iSelectedProperty == CB_ERR) || ( iSelectedProperty < 0) ) return;
  // set invalid result
  INDEX iToSelect = -1;
  // browse trough entities
  for( INDEX iItem=1; iItem<ctProperties; iItem++)
  {
    // we will use next/previos item for selecting
    INDEX iToTest = (iSelectedProperty+ctProperties+iDirection*iItem)%ctProperties;
    // if there is valid property selected
    CPropertyID *ppidProperty = (CPropertyID *) m_PropertyComboBox.GetItemData( iToTest);
    if( ppidProperty == NULL)
    {
      ASSERTALWAYS("Null property found!");
      return;
    }
      // if null-ptr target is requested
    if( bOnlyEmptyTargets)
    {
      // if this is entity pointer property
      if( ppidProperty->pid_eptType == CEntityProperty::EPT_ENTITYPTR)
      {
        // obtain property ptr
        CEntity *pen = pDoc->m_selEntitySelection.GetFirstInSelection();
        CEntityProperty *penpProperty = ppidProperty->pid_penpProperty;
        CEntity *penTarget = ENTITYPROPERTY( pen, penpProperty->ep_slOffset, CEntityPointer);
        // if it is NULL
        if( penTarget == NULL)
        {
          // set this property as one to select
          iToSelect = iToTest;
          continue;
        }
      }
    }
    else
    {
      // if null-ptr target isn't requested, each property is valid
      iToSelect = iToTest;
      continue;
    }
  }

  // if we have valid result
  if( iToSelect != -1)
  {
    // select it
    m_PropertyComboBox.SetCurSel( iToSelect);
    m_PropertyComboBox.SelectProperty();
  }
}

void CPropertyComboBar::SetIntersectingFileName()
{
  if( m_BrowseFileCtrl.IsWindowVisible())
  {
    // get entity selection's intesecting file name
    CTFileName fnIntersectingFile = m_BrowseFileCtrl.GetIntersectingFile();
    if( fnIntersectingFile != "")
    {
      m_strFileName = "...\\" + fnIntersectingFile.FileName() + fnIntersectingFile.FileExt();
    }
    else
    {
      m_strFileName = "";
    }
  }
}

void CPropertyComboBar::SetColorPropertyToEntities( COLOR colNewColor)
{
  CWorldEditorDoc *pDoc = theApp.GetDocument();
  // obtain curently selected property ID
  CPropertyID *ppidProperty = GetSelectedProperty();
  if( ppidProperty == NULL) return;
  // change curently selected color property in the selected entities
  // for each of the selected entities
  FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
  {
    // obtain property ptr
    CEntityProperty *penpProperty = iten->PropertyForName( ppidProperty->pid_strName);
    // discard old entity settings
    iten->End();
    // set new color value
    ENTITYPROPERTY( &*iten, penpProperty->ep_slOffset, COLOR) = colNewColor;
    // apply new entity settings
    iten->Initialize();
  }
  // mark that document is changed
  pDoc->SetModifiedFlag( TRUE);
}

BOOL CPropertyComboBar::OnIdle(LONG lCount)
{
  CWorldEditorDoc *pDoc = theApp.GetDocument();
  m_PropertyComboBox.OnIdle( lCount);
  
  // if we are curently changing color
  if( m_EditColorCtrl.IsWindowVisible())
  {
    COLOR colCurrentColor = m_EditColorCtrl.GetColor();
    if( colCurrentColor != m_colLastColor)
    {
      SetColorPropertyToEntities( colCurrentColor);
      m_colLastColor = colCurrentColor;
      // update all views
      pDoc->UpdateAllViews( NULL);
    }
  }

  return TRUE;
}


/*
 * Arranges (using show/hide) controls depending upon editing property type
 */
void CPropertyComboBar::ArrangeControls()
{
  // array to receive description message
  char strMessage[ 128];

  // spawn flags are grayed
  BOOL bEnableSpawn = FALSE;

  // mark all controls for hiding
  int iEnum = SW_HIDE;
  int iString = SW_HIDE;
  int iFloat = SW_HIDE;
  int iBBox = SW_HIDE;
  int iIndex = SW_HIDE;
  int iBool = SW_HIDE;
  int iColor = SW_HIDE;
  int iBrowse = SW_HIDE;
  int iX = SW_HIDE;
  int iNoTarget = SW_HIDE;
  int iFloatRangeText = SW_HIDE;
  int iAngle3D = SW_HIDE;
  int iIndexRangeText = SW_HIDE;
  int iChooseColorText = SW_HIDE;
  int iFileNameText = SW_HIDE;
  int iFlags = SW_HIDE;

  // get active document 
  CWorldEditorDoc* pDoc = theApp.GetActiveDocument();
  // if view does not exist, return
  if( pDoc != NULL)
  {
    m_strFloatRange = "";
    m_strChooseColor = "";
    m_strFileName = "";

    // obtain selected property ID ptr
    INDEX iSelectedProperty = m_PropertyComboBox.GetCurSel();
    CPropertyID *ppidProperty = (CPropertyID *) m_PropertyComboBox.GetItemData( iSelectedProperty);
    // if there is valid property selected
    if( ppidProperty != NULL)
    {
      // show controls acording to property type
      switch( ppidProperty->pid_eptType)
      {
      case CEntityProperty::EPT_FLAGS:
        {
          iFlags = SW_SHOW;
          CEntity *pen = pDoc->m_selEntitySelection.GetFirstInSelection();
          CEntityProperty *penpProperty = pen->PropertyForName( ppidProperty->pid_strName);
          m_ctrlEditFlags.SetFlags( ENTITYPROPERTY( pen, penpProperty->ep_slOffset, ULONG));

          // obtain enum property description object
          CEntityPropertyEnumType *epEnum = penpProperty->ep_pepetEnumType;
          // create mask of editable bits
          ULONG ulEditable=0;
          // for all enumerated members
          for( INDEX iEnum = 0; iEnum<epEnum->epet_ctValues; iEnum++)
          {
            if( epEnum->epet_aepevValues[ iEnum].epev_strName!="")
            {
              ulEditable|=(1UL)<<epEnum->epet_aepevValues[ iEnum].epev_iValue;
              CTString strBitName=epEnum->epet_aepevValues[ iEnum].epev_strName;
              m_ctrlEditFlags.SetBitDescription(iEnum, strBitName);
            }
          }
          m_ctrlEditFlags.SetEditableMask(ulEditable);

          // for each of the selected entities
          FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
          {
            // obtain property ptr
            CEntityProperty *penpProperty = iten->PropertyForName( ppidProperty->pid_strName);
            // it is, get string as one that others will compare with
            m_ctrlEditFlags.MergeFlags( ENTITYPROPERTY( &*iten, penpProperty->ep_slOffset, ULONG));
          }
          break;
        }
      case CEntityProperty::EPT_ENUM:
      case CEntityProperty::EPT_ENTITYPTR:
      case CEntityProperty::EPT_PARENT:
      case CEntityProperty::EPT_ANIMATION:
      case CEntityProperty::EPT_ILLUMINATIONTYPE:
        {
          // obtain property ptr
          CEntityProperty *penpProperty = ppidProperty->pid_penpProperty;
          // remove all combo entries
          m_EditEnumComboBox.ResetContent();
          // combo control is to be shown
          iEnum = SW_SHOW;
          // for enum type of property
          if( ppidProperty->pid_eptType == CEntityProperty::EPT_ENUM)
          {
            // obtain enum property description object
            CEntityPropertyEnumType *epEnum = penpProperty->ep_pepetEnumType;

            // lock selection's dynamic container
            pDoc->m_selEntitySelection.Lock();

            BOOL bAllEntitiesHaveSameEnum = TRUE;
            INDEX iCurrentEnumID;

            // for each of the selected entities
            FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten) 
            {
              // if this is first entity in dynamic container
              if( pDoc->m_selEntitySelection.Pointer(0) == iten)
              {
                // it is, set its value as one that others will compare with
                iCurrentEnumID = ENTITYPROPERTY( &*iten, penpProperty->ep_slOffset, INDEX);
              }
              else
              {
                // if value of entity enum variable is different from first one
                if( iCurrentEnumID != ENTITYPROPERTY( &*iten, penpProperty->ep_slOffset, INDEX) )
                {
                  // mark that all entities do not have same enum
                  bAllEntitiesHaveSameEnum = FALSE;
                  break;
                }
              }
            }
            // unlock selection's dynamic container
            pDoc->m_selEntitySelection.Unlock();

            // invalid choosed enum ID
            INDEX iSelectedEnumID = -1;
            // for all enumerated members
            for( INDEX iEnum = 0; iEnum< epEnum->epet_ctValues; iEnum++)
            {
              // add enum member into combo box
              // add descriptive string
              INDEX iAddedAs = m_EditEnumComboBox.AddString( 
                CString(epEnum->epet_aepevValues[ iEnum].epev_strName));
              // get looping enum id
              INDEX iLoopingEnumID = epEnum->epet_aepevValues[ iEnum].epev_iValue;
              // connect descriptive string with enum value itself
              m_EditEnumComboBox.SetItemData( iAddedAs, iLoopingEnumID);
            
              // if all entities have same enum and this is joint enumerated value
              if( bAllEntitiesHaveSameEnum && (iCurrentEnumID == iLoopingEnumID) )
              {
                // remember its index to set as selected combo value
                iSelectedEnumID = iCurrentEnumID;
              }
            }
            // if entities have same enum there must be valid selected enum ID
            if( bAllEntitiesHaveSameEnum)
            {
              ASSERT(iSelectedEnumID != -1);
            }

            // if all entities have same enum
            if( bAllEntitiesHaveSameEnum)
            {
              // after inserting combo members, they can be sorted so adding order does not work
              // any more, combo member indices are not same any more
              // mark invalid combo entry as selected
              INDEX iSelectedCombo = -1;
              // for all combo members
              for( INDEX iCombo = 0; iCombo<m_EditEnumComboBox.GetCount(); iCombo++)
              {
                // if this entry has same enum ID as selected one
                if( m_EditEnumComboBox.GetItemData( iCombo) == (DWORD) iSelectedEnumID)
                {
                  // set its index as one to be selected
                  iSelectedCombo = iCombo;
                  break;
                }
              }
              // there must be selected entry
              ASSERT( iSelectedCombo != -1);
              // select combo member
              m_EditEnumComboBox.SetCurSel( iSelectedCombo);
            }
          }
          // else if this is entity ptr property
          else if( (ppidProperty->pid_eptType == CEntityProperty::EPT_ENTITYPTR) ||
                   (ppidProperty->pid_eptType == CEntityProperty::EPT_PARENT) )
          {
            BOOL bParentProperty = ppidProperty->pid_eptType == CEntityProperty::EPT_PARENT;
            // for all entities in the world
            {FOREACHINDYNAMICCONTAINER(pDoc->m_woWorld.wo_cenEntities, CEntity, iten)
            {
              CTString strEntityName = iten->GetName();
              CBrushSector *pbscSector = iten->GetFirstSector();
              BOOL bSectorVisible = (pbscSector == NULL) ||
                                   !(pbscSector->bsc_ulFlags & BSCF_HIDDEN);
              
              BOOL bValidTarget = TRUE;
              // for each entity in selection
              {FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, itenSel)
              {
                CEntityProperty *penpProperty = ppidProperty->pid_penpProperty;
                if(penpProperty != NULL)
                {
                  // see if target is valid
                  if( !itenSel->IsTargetValid( penpProperty->ep_slOffset, iten))
                  {
                    bValidTarget = FALSE;
                    break;
                  }
                }
              }}

              // if entity is targetable, not hidden, and is not selected
              if( (iten->IsTargetable() || bParentProperty) &&
                  (bSectorVisible) &&
                  bValidTarget &&
                  //(!iten->IsSelected(ENF_SELECTED)) &&
                  !(iten->en_ulFlags&ENF_HIDDEN))
              {
                INDEX iAddedAs;
                if( strEntityName != "")
                {
                  // add it to combo
                  iAddedAs = m_EditEnumComboBox.AddString( CString(strEntityName));
                }
                else
                {
                  // add it to combo
                  iAddedAs = m_EditEnumComboBox.AddString( L"Unnamed");
                }
                // set entity ptr as item's data
                m_EditEnumComboBox.SetItemData( iAddedAs, (DWORD_PTR) &*iten);
              }
            }}

            // set NULL entity name
            INDEX iAddedAs = m_EditEnumComboBox.InsertString( 0, L"None");
            // set NULL entity ptr as item's data
            m_EditEnumComboBox.SetItemData( iAddedAs, NULL);

            // lock selection's dynamic container
            pDoc->m_selEntitySelection.Lock();

            // to hold intersecting entity ptr
            CEntity *penEntity;
            // for each of the selected entities
            FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
            {
              // if this is first entity in dynamic container
              if( pDoc->m_selEntitySelection.Pointer(0) == iten)
              {
                // it is, set its value as one that others will compare with
                if( bParentProperty)
                  penEntity = iten->GetParent();
                else
                  penEntity = ENTITYPROPERTY( &*iten, penpProperty->ep_slOffset, CEntityPointer);
                
              }
              else
              {
                // get current entity ptr
                CEntity *penCurrent;
                if( bParentProperty) penCurrent = iten->GetParent();
                else penCurrent = ENTITYPROPERTY( &*iten, penpProperty->ep_slOffset, CEntityPointer);

                // if value of entity ptr is different from first one
                if( penEntity != penCurrent)
                {
                  // mark that all entities do not have same entity ptr
                  penEntity = (CEntity *) -1;
                  break;
                }
              }
            }
            // unlock selection's dynamic container
            pDoc->m_selEntitySelection.Unlock();
          
            if( penEntity != (CEntity *)-1)
            {
              INDEX iSelectedCombo = 0;
              // for all combo members
              for( INDEX iCombo = 0; iCombo<m_EditEnumComboBox.GetCount(); iCombo++)
              {
                // if this entry has same entity ptr as selected one
                if( penEntity == (CEntity *) m_EditEnumComboBox.GetItemData( iCombo) )
                {
                  // set its index as one to be selected
                  iSelectedCombo = iCombo;
                  break;
                }
              }
              // select combo member
              m_EditEnumComboBox.SetCurSel( iSelectedCombo);
            }
            
            // show x button
            iNoTarget = SW_SHOW;
          }
          // else if we should initialize animation property
          else if( ppidProperty->pid_eptType == CEntityProperty::EPT_ANIMATION)
          {
            // get first selected entity
            pDoc->m_selEntitySelection.Lock();
            CEntity *penFirst = pDoc->m_selEntitySelection.Pointer(0);
            pDoc->m_selEntitySelection.Unlock();
            CAnimData *pAD = penFirst->GetAnimData( penpProperty->ep_slOffset);
            if( pAD != NULL)
            {
              // add all animations into combo box
              for( INDEX iAnimation=0;iAnimation<pAD->GetAnimsCt();iAnimation++)
              {
                // obtain information about animation
                CAnimInfo aiInfo;
                pAD->GetAnimInfo(iAnimation, aiInfo);
                // add animation to combo
                INDEX iAddedAs = m_EditEnumComboBox.AddString( CString(aiInfo.ai_AnimName));
                // set animation number as item's data
                m_EditEnumComboBox.SetItemData( iAddedAs, (DWORD_PTR) iAnimation);
              }
            }

            // lock selection's dynamic container
            pDoc->m_selEntitySelection.Lock();
            INDEX iJointAnimation = -1;
            // for each of the selected entities
            FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
            {
              // obtain property ptr
              CEntityProperty *penpProperty = iten->PropertyForName( ppidProperty->pid_strName);
              // if this is first entity in dynamic container
              if( pDoc->m_selEntitySelection.Pointer(0) == iten)
              {
                // it is, get light animation as one that others will compare with
                iJointAnimation = ENTITYPROPERTY( &*iten, penpProperty->ep_slOffset, INDEX);
              }
              else
              {
                if( iJointAnimation!=ENTITYPROPERTY( &*iten, penpProperty->ep_slOffset, INDEX) )
                {
                  iJointAnimation = -1;
                }
              }
            }
            // unlock selection's dynamic container
            pDoc->m_selEntitySelection.Unlock();
          
            if( iJointAnimation != -1)
            {
              INDEX iSelectedCombo = -1;
              // for all combo members
              for( INDEX iCombo = 0; iCombo<m_EditEnumComboBox.GetCount(); iCombo++)
              {
                // if this entry has same entity ptr as selected one
                if( iJointAnimation == (INDEX) m_EditEnumComboBox.GetItemData( iCombo) )
                {
                  // set its index as one to be selected
                  iSelectedCombo = iCombo;
                  break;
                }
              }
              // if there is joint animation but is not found in combo
              if( iSelectedCombo == -1)
              {
                iSelectedCombo = 0;
              }
              // select combo member
              m_EditEnumComboBox.SetCurSel( iSelectedCombo);
            }
          }
          // else if we should initialize illumination type property
          else if( ppidProperty->pid_eptType == CEntityProperty::EPT_ILLUMINATIONTYPE)
          {
            // add all illumination types into combo box
            for( INDEX iIllumination=0;iIllumination<255; iIllumination++)
            {
              // get illumination name
              CTString strIlluminationName = 
                pDoc->m_woWorld.wo_aitIlluminationTypes[iIllumination].it_strName;
              // name must not be <none> except for first illumination
              if(strIlluminationName == "")
              {
                break;
              }
              // add illumination type to combo
              INDEX iAddedAs = m_EditEnumComboBox.AddString( CString(strIlluminationName));
              // set illumination type number as item's data
              m_EditEnumComboBox.SetItemData( iAddedAs, (DWORD_PTR) iIllumination);
            }

            // lock selection's dynamic container
            pDoc->m_selEntitySelection.Lock();

            INDEX iJointIllumination = -1;
            // for each of the selected entities
            FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
            {
              // obtain property ptr
              CEntityProperty *penpProperty = iten->PropertyForName( ppidProperty->pid_strName);
              // if this is first entity in dynamic container
              if( pDoc->m_selEntitySelection.Pointer(0) == iten)
              {
                // it is, get illumination as one that others will compare with
                iJointIllumination = ENTITYPROPERTY( &*iten, penpProperty->ep_slOffset, INDEX);
              }
              else
              {
                if( iJointIllumination!=ENTITYPROPERTY( &*iten, penpProperty->ep_slOffset, INDEX) )
                {
                  iJointIllumination = -1;
                }
              }
            }
            // unlock selection's dynamic container
            pDoc->m_selEntitySelection.Unlock();
          
            if( iJointIllumination != -1)
            {
              INDEX iSelectedCombo = -1;
              // for all combo members
              for( INDEX iCombo = 0; iCombo<m_EditEnumComboBox.GetCount(); iCombo++)
              {
                // if this entry has same entity ptr as selected one
                if( iJointIllumination == (INDEX) m_EditEnumComboBox.GetItemData( iCombo) )
                {
                  // set its index as one to be selected
                  iSelectedCombo = iCombo;
                  break;
                }
              }
              // if there is joint illumination but is not found in combo
              if( iSelectedCombo == -1)
              {
                iSelectedCombo = 0;
              }
              // select combo member
              m_EditEnumComboBox.SetCurSel( iSelectedCombo);
            }
          }
          else
          {
            ASSERTALWAYS("Illegal entity \"combo\"-type property found !");
          }
          break;
        }
      case CEntityProperty::EPT_STRING:
      case CEntityProperty::EPT_STRINGTRANS:
        {
          // edit string control is to be shown
          iString = SW_SHOW;
          // lock selection's dynamic container
          pDoc->m_selEntitySelection.Lock();
          // for each of the selected entities
          FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
          {
            // obtain property ptr
            CEntityProperty *penpProperty = iten->PropertyForName( ppidProperty->pid_strName);
            // if this is first entity in dynamic container
            if( pDoc->m_selEntitySelection.Pointer(0) == iten)
            {
              // it is, get string as one that others will compare with
              m_strEditingString = ENTITYPROPERTY( &*iten, penpProperty->ep_slOffset, CTString);
            }
            else
            {
              CTString strString = ENTITYPROPERTY( &*iten, penpProperty->ep_slOffset, CTString);
              // if string is different from first one
              if( CTString( CStringA(m_strEditingString)) != strString)
              {
                // all selected entities do not share same string
                m_strEditingString = "";
              }
            }
          }
          // unlock selection's dynamic container
          pDoc->m_selEntitySelection.Unlock();
          break;
        }
      case CEntityProperty::EPT_FLOAT:
      case CEntityProperty::EPT_RANGE:
      case CEntityProperty::EPT_ANGLE:
        {
          iFloat = SW_SHOW;
          iFloatRangeText = SW_SHOW;
        
          pDoc->m_selEntitySelection.Lock();
          FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
          {
            CEntityProperty *penpProperty = iten->PropertyForName( ppidProperty->pid_strName);
            if( pDoc->m_selEntitySelection.Pointer(0) == iten)
            {
              if( ppidProperty->pid_eptType == CEntityProperty::EPT_ANGLE)
              {
                m_fEditingFloat = DegAngle( ENTITYPROPERTY( 
                                            &*iten, penpProperty->ep_slOffset, ANGLE));
              }
              else
              {
                m_fEditingFloat = ENTITYPROPERTY( &*iten, penpProperty->ep_slOffset, FLOAT);
              }
            }
            else
            {
              FLOAT fCurrentFloat;
              if( ppidProperty->pid_eptType == CEntityProperty::EPT_ANGLE)
              {
                fCurrentFloat = DegAngle( ENTITYPROPERTY( 
                                          &*iten, penpProperty->ep_slOffset, ANGLE));
              }
              else
              {
                fCurrentFloat = ENTITYPROPERTY( &*iten, penpProperty->ep_slOffset, FLOAT);
              }
              if( m_fEditingFloat != fCurrentFloat )
              {
                m_fEditingFloat = 0.0f;
                break;
              }
            }
          }
          pDoc->m_selEntitySelection.Unlock();

          sprintf( strMessage, "Float");
          m_strFloatRange = strMessage;
          if( ppidProperty->pid_eptType == CEntityProperty::EPT_RANGE)
          {
            pDoc->UpdateAllViews( NULL);
          }
          break;
        }
      case CEntityProperty::EPT_ANGLE3D:
        {
          iAngle3D = SW_SHOW;
          pDoc->m_selEntitySelection.Lock();
          FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
          {
            CEntityProperty *penpProperty = iten->PropertyForName( ppidProperty->pid_strName);
            if( pDoc->m_selEntitySelection.Pointer(0) == iten)
            {
              ANGLE3D aAngle = ENTITYPROPERTY( &*iten, penpProperty->ep_slOffset, ANGLE3D);
              m_fEditingHeading = DegAngle( aAngle(1));
              m_fEditingPitch = DegAngle( aAngle(2));
              m_fEditingBanking = DegAngle( aAngle(3));
            }
            else
            {
              ANGLE3D aCurrent = ENTITYPROPERTY( &*iten, penpProperty->ep_slOffset, ANGLE3D);
              if( m_fEditingHeading != DegAngle( aCurrent(1))) m_fEditingHeading = 0.0f;
              if( m_fEditingPitch != DegAngle( aCurrent(2))) m_fEditingPitch = 0.0f;
              if( m_fEditingBanking != DegAngle( aCurrent(3))) m_fEditingBanking = 0.0f;
            }
          }
          pDoc->m_selEntitySelection.Unlock();
          break;
        }
      case CEntityProperty::EPT_FLOATAABBOX3D:
        {
          iBBox = SW_SHOW;
          // mark that all entities have same value for current bbox axis
          BOOL bAllHaveSameBBoxValue = TRUE;

          // lock selection's dynamic container
          pDoc->m_selEntitySelection.Lock();
          // for each of the selected entities
          FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
          {
            // obtain property ptr
            CEntityProperty *penpProperty = iten->PropertyForName( ppidProperty->pid_strName);
            // if this is first entity in dynamic container
            if( pDoc->m_selEntitySelection.Pointer(0) == iten)
            {
              // get first entity's bbox min and max values for selected axis
              FLOATaabbox3D bboxCurrent = ENTITYPROPERTY( &*iten, penpProperty->ep_slOffset,
                                                          FLOATaabbox3D);
              // get its min and max vectors
              FLOAT3D vMin = bboxCurrent.Min();
              FLOAT3D vMax = bboxCurrent.Max();
              // remember min and max ranges for current axis for other entities in selection
              // to compare with
              m_fEditingBBoxMin = vMin( m_iXYZAxis);
              m_fEditingBBoxMax = vMax( m_iXYZAxis);
            }
            else
            {
              // get first entity's bbox min and max values for selected axis
              FLOATaabbox3D bboxCurrent = ENTITYPROPERTY( &*iten, penpProperty->ep_slOffset,
                                                          FLOATaabbox3D);
              // get its min and max vectors
              FLOAT3D vMin = bboxCurrent.Min();
              FLOAT3D vMax = bboxCurrent.Max();
              // compare min and max ranges for current axis with those from first entity
              if( (m_fEditingBBoxMin != vMin( m_iXYZAxis)) ||
                  (m_fEditingBBoxMax != vMax( m_iXYZAxis)) )
              {
                // all selected entities do not share same number so mark it
                bAllHaveSameBBoxValue = FALSE;
                m_fEditingBBoxMin = 0.0f;
                m_fEditingBBoxMax = 0.0f;
                break;
              }
            }
          }
          // unlock selection's dynamic container
          pDoc->m_selEntitySelection.Unlock();
          // refresh views
          pDoc->UpdateAllViews( NULL);
          break;
        }
      case CEntityProperty::EPT_INDEX:
        {
          iIndex = SW_SHOW;
          iIndexRangeText = SW_SHOW;
        
          // mark that all entities have same index property value
          BOOL bAllHaveSameIndex = TRUE;

          // lock selection's dynamic container
          pDoc->m_selEntitySelection.Lock();
          // for each of the selected entities
          FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
          {
            // obtain property ptr
            CEntityProperty *penpProperty = iten->PropertyForName( ppidProperty->pid_strName);
            // if this is first entity in dynamic container
            if( pDoc->m_selEntitySelection.Pointer(0) == iten)
            {
              // it is, get this number as one that others will compare with
              m_iEditingIndex = ENTITYPROPERTY( &*iten, penpProperty->ep_slOffset, INDEX);
            }
            else
            {
              // if number is different from first one
              if( m_iEditingIndex != ENTITYPROPERTY( &*iten, penpProperty->ep_slOffset, INDEX) )
              {
                // all selected entities do not share same number so mark it
                bAllHaveSameIndex = FALSE;
                m_iEditingIndex = 0;
                break;
              }
            }
          }
          // unlock selection's dynamic container
          pDoc->m_selEntitySelection.Unlock();
          sprintf( strMessage, "Integer");
          m_strIndexRange = strMessage;
          break;
        }
      case CEntityProperty::EPT_BOOL:
        {
          m_EditBoolCtrl.SetWindowText( CString(ppidProperty->pid_strName));
          iBool = SW_SHOW;
          // variable to receive state of check box
          INDEX iCheckBox;
          // for each of the selected entities
          FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
          {
            // obtain property ptr
            CEntityProperty *penpProperty = iten->PropertyForName( ppidProperty->pid_strName);
            // if this is first entity in dynamic container
            if( pDoc->m_selEntitySelection.Pointer(0) == iten)
            {
              // it is, get boolean as one that others will compare with
              iCheckBox = ENTITYPROPERTY( &*iten, penpProperty->ep_slOffset, BOOL);
            }
            else
            {
              // if this boolean is different from first one
              if( iCheckBox != ENTITYPROPERTY( &*iten, penpProperty->ep_slOffset, BOOL) )
              {
                // selected entities do not share same boolean, set undefined state
                iCheckBox = 2;
                break;
              }
            }
          }
          // set calculated boolean state (or undefined)
          m_EditBoolCtrl.SetCheck( iCheckBox);
          break;
        }
      case CEntityProperty::EPT_COLOR:
        {
          // intersecting color
          COLOR colIntersected;
          // lock selection's dynamic container
          pDoc->m_selEntitySelection.Lock();
          // for each of the selected entities
          FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
          {
            // obtain property ptr
            CEntityProperty *penpProperty = iten->PropertyForName( ppidProperty->pid_strName);
            // if this is first entity in dynamic container
            if( pDoc->m_selEntitySelection.Pointer(0) == iten)
            {
              // it is, get this color as one that others will compare with
              colIntersected = ENTITYPROPERTY( &*iten, penpProperty->ep_slOffset, COLOR);
              // set button's color
              m_EditColorCtrl.SetColor( colIntersected);
              // remember this color as the last
              m_colLastColor = colIntersected;
            }
            else
            {
              // if color is different from first one
              if( colIntersected != ENTITYPROPERTY( &*iten, penpProperty->ep_slOffset, COLOR) )
              {
                // all selected entities do not share same color
                m_EditColorCtrl.SetMixedColor();
                break;
              }
            }
          }
          // unlock selection's dynamic container
          pDoc->m_selEntitySelection.Unlock();
          iChooseColorText = SW_SHOW;
          iColor = SW_SHOW;
          break;
        }
      case CEntityProperty::EPT_FILENAME:
      case CEntityProperty::EPT_FILENAMENODEP:
        {
          SetIntersectingFileName();
          iFileNameText = SW_SHOW;
          iBrowse = SW_SHOW;
          iX = SW_SHOW;
          m_BrowseFileCtrl.m_bFileNameNoDep = 
            (ppidProperty->pid_eptType == CEntityProperty::EPT_FILENAMENODEP);
          break;
        }
      case CEntityProperty::EPT_SPAWNFLAGS:
        {
          bEnableSpawn = TRUE;
          break;
        }
      default:
        {
          ASSERTALWAYS("Unknown property type");
        }
      }  
    }
    // update all views
    pDoc->UpdateAllViews( NULL);
  }

  m_EditEasySpawn.EnableWindow( bEnableSpawn);
  m_EditNormalSpawn.EnableWindow( bEnableSpawn);
  m_EditHardSpawn.EnableWindow( bEnableSpawn);
  m_EditExtremeSpawn.EnableWindow( bEnableSpawn);
  m_EditDifficulty_1.EnableWindow( bEnableSpawn);
  m_EditDifficulty_2.EnableWindow( bEnableSpawn);
  m_EditDifficulty_3.EnableWindow( bEnableSpawn);
  m_EditDifficulty_4.EnableWindow( bEnableSpawn);
  m_EditDifficulty_5.EnableWindow( bEnableSpawn);

  m_EditSingleSpawn.EnableWindow( bEnableSpawn);
  m_EditCooperativeSpawn.EnableWindow( bEnableSpawn);
  m_EditDeathMatchSpawn.EnableWindow( bEnableSpawn);
  m_EditGameMode_1.EnableWindow( bEnableSpawn);
  m_EditGameMode_2.EnableWindow( bEnableSpawn);
  m_EditGameMode_3.EnableWindow( bEnableSpawn);
  m_EditGameMode_4.EnableWindow( bEnableSpawn);
  m_EditGameMode_5.EnableWindow( bEnableSpawn);
  m_EditGameMode_6.EnableWindow( bEnableSpawn);

  GetDlgItem( IDC_EASY_T)->EnableWindow( bEnableSpawn);
  GetDlgItem( IDC_NORMAL_T)->EnableWindow( bEnableSpawn);
  GetDlgItem( IDC_HARD_T)->EnableWindow( bEnableSpawn);
  GetDlgItem( IDC_EXTREME_T)->EnableWindow( bEnableSpawn);
  GetDlgItem( IDC_DIFFICULTY_1_T)->EnableWindow( bEnableSpawn);
  GetDlgItem( IDC_DIFFICULTY_2_T)->EnableWindow( bEnableSpawn);
  GetDlgItem( IDC_DIFFICULTY_3_T)->EnableWindow( bEnableSpawn);
  GetDlgItem( IDC_DIFFICULTY_4_T)->EnableWindow( bEnableSpawn);
  GetDlgItem( IDC_DIFFICULTY_5_T)->EnableWindow( bEnableSpawn);

  GetDlgItem( IDC_SINGLE_T)->EnableWindow( bEnableSpawn);
  GetDlgItem( IDC_COOPERATIVE_T)->EnableWindow( bEnableSpawn);
  GetDlgItem( IDC_DEATHMATCH_T)->EnableWindow( bEnableSpawn);
  GetDlgItem( IDC_GAME_MODE_1_T)->EnableWindow( bEnableSpawn);
  GetDlgItem( IDC_GAME_MODE_2_T)->EnableWindow( bEnableSpawn);
  GetDlgItem( IDC_GAME_MODE_3_T)->EnableWindow( bEnableSpawn);
  GetDlgItem( IDC_GAME_MODE_4_T)->EnableWindow( bEnableSpawn);
  GetDlgItem( IDC_GAME_MODE_5_T)->EnableWindow( bEnableSpawn);
  GetDlgItem( IDC_GAME_MODE_6_T)->EnableWindow( bEnableSpawn);

  m_EditEasySpawn.ShowWindow( bEnableSpawn);
  m_EditNormalSpawn.ShowWindow( bEnableSpawn);
  m_EditHardSpawn.ShowWindow( bEnableSpawn);
  m_EditExtremeSpawn.ShowWindow( bEnableSpawn);
  m_EditDifficulty_1.ShowWindow( bEnableSpawn);
  m_EditDifficulty_2.ShowWindow( bEnableSpawn);
  m_EditDifficulty_3.ShowWindow( bEnableSpawn);
  m_EditDifficulty_4.ShowWindow( bEnableSpawn);
  m_EditDifficulty_5.ShowWindow( bEnableSpawn);

  m_EditSingleSpawn.ShowWindow( bEnableSpawn);
  m_EditCooperativeSpawn.ShowWindow( bEnableSpawn);
  m_EditDeathMatchSpawn.ShowWindow( bEnableSpawn);
  m_EditGameMode_1.ShowWindow( bEnableSpawn);
  m_EditGameMode_2.ShowWindow( bEnableSpawn);
  m_EditGameMode_3.ShowWindow( bEnableSpawn);
  m_EditGameMode_4.ShowWindow( bEnableSpawn);
  m_EditGameMode_5.ShowWindow( bEnableSpawn);
  m_EditGameMode_6.ShowWindow( bEnableSpawn);


  GetDlgItem( IDC_EASY_T)->ShowWindow( bEnableSpawn);
  GetDlgItem( IDC_NORMAL_T)->ShowWindow( bEnableSpawn);
  GetDlgItem( IDC_HARD_T)->ShowWindow( bEnableSpawn);
  GetDlgItem( IDC_EXTREME_T)->ShowWindow( bEnableSpawn);
  GetDlgItem( IDC_DIFFICULTY_1_T)->ShowWindow( bEnableSpawn);
  GetDlgItem( IDC_DIFFICULTY_2_T)->ShowWindow( bEnableSpawn);
  GetDlgItem( IDC_DIFFICULTY_3_T)->ShowWindow( bEnableSpawn);
  GetDlgItem( IDC_DIFFICULTY_4_T)->ShowWindow( bEnableSpawn);
  GetDlgItem( IDC_DIFFICULTY_5_T)->ShowWindow( bEnableSpawn);

  GetDlgItem( IDC_SINGLE_T)->ShowWindow( bEnableSpawn);
  GetDlgItem( IDC_DEATHMATCH_T)->ShowWindow( bEnableSpawn);
  GetDlgItem( IDC_COOPERATIVE_T)->ShowWindow( bEnableSpawn);
  GetDlgItem( IDC_GAME_MODE_1_T)->ShowWindow( bEnableSpawn);
  GetDlgItem( IDC_GAME_MODE_2_T)->ShowWindow( bEnableSpawn);
  GetDlgItem( IDC_GAME_MODE_3_T)->ShowWindow( bEnableSpawn);
  GetDlgItem( IDC_GAME_MODE_4_T)->ShowWindow( bEnableSpawn);
  GetDlgItem( IDC_GAME_MODE_5_T)->ShowWindow( bEnableSpawn);
  GetDlgItem( IDC_GAME_MODE_6_T)->ShowWindow( bEnableSpawn);

  // show controls that are necessary to edit current property
  m_EditEnumComboBox.ShowWindow( iEnum);
  m_EditStringCtrl.ShowWindow( iString);
  m_EditFloatCtrl.ShowWindow( iFloat);
  m_EditBBoxMinCtrl.ShowWindow( iBBox);
  m_EditBBoxMaxCtrl.ShowWindow( iBBox);
  GetDlgItem( IDC_AXIS_X)->ShowWindow( iBBox);
  GetDlgItem( IDC_AXIS_Y)->ShowWindow( iBBox);
  GetDlgItem( IDC_AXIS_Z)->ShowWindow( iBBox);
  m_EditIndexCtrl.ShowWindow( iIndex);
  m_EditBoolCtrl.ShowWindow( iBool);
  m_BrowseFileCtrl.ShowWindow( iBrowse);
  GetDlgItem( IDC_NO_FILE)->ShowWindow( iX);
  GetDlgItem( IDC_NO_TARGET)->ShowWindow( iNoTarget);

  m_EditColorCtrl.ShowWindow( iColor);
  m_ctrlEditFlags.ShowWindow( iFlags);

  GetDlgItem( IDC_FLOAT_RANGE_T)->ShowWindow( iFloatRangeText);
  GetDlgItem( IDC_INDEX_RANGE_T)->ShowWindow( iIndexRangeText);
  GetDlgItem( IDC_FILE_NAME_T)->ShowWindow( iFileNameText);
  
  GetDlgItem( IDC_ANGLE3D_T)->ShowWindow( iAngle3D);
  GetDlgItem( IDC_EDIT_HEADING)->ShowWindow( iAngle3D);
  GetDlgItem( IDC_EDIT_PITCH)->ShowWindow( iAngle3D);
  GetDlgItem( IDC_EDIT_BANKING)->ShowWindow( iAngle3D);

  GetDlgItem( IDC_ENTITY_CLASS)->ShowWindow( SW_SHOW);
  GetDlgItem( IDC_ENTITY_NAME)->ShowWindow( SW_SHOW);
  GetDlgItem( IDC_ENTITY_DESCRIPTION)->ShowWindow( SW_SHOW);

  GetDlgItem( IDC_ENTITY_CLASS)->EnableWindow( m_PropertyComboBox.IsWindowEnabled());
  GetDlgItem( IDC_ENTITY_NAME)->EnableWindow( m_PropertyComboBox.IsWindowEnabled());
  GetDlgItem( IDC_ENTITY_DESCRIPTION)->EnableWindow( m_PropertyComboBox.IsWindowEnabled());

  // update dialog data (so new data could be loaded into dialog)
  UpdateData( FALSE);
}

void CPropertyComboBar::OnUpdateBrowseFile(CCmdUI* pCmdUI)
{
  pCmdUI->Enable( TRUE);
}

void CPropertyComboBar::OnUpdateEditColor(CCmdUI* pCmdUI)
{
  pCmdUI->Enable( TRUE);
}

void CPropertyComboBar::OnUpdateEditFlags(CCmdUI* pCmdUI)
{
  pCmdUI->Enable( TRUE);
}

void CPropertyComboBar::SetIntersectingEntityClassName(void)
{
  CWorldEditorDoc *pDoc = theApp.GetDocument();
  // lock selection's dynamic container
  pDoc->m_selEntitySelection.Lock();
  // string to contain intersecting class name
  CTString strIntersectingClass = "No entity class";
  CTString strIntersectingName = "No name";
  CTString strIntersectingDescription = "No description";
  // for each of the selected entities
  FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
  {
    // get class of this entity
    CEntityClass *pencEntityClass = iten->GetClass();
    // get file name of clas file
    CTFileName fnClassFileName = pencEntityClass->GetName().FileName();
    // get name
    CTString strEntityName = iten->GetName();
    // get description
    CTString strEntityDescription = iten->GetDescription();
    // if this is first entity in dynamic container
    if( pDoc->m_selEntitySelection.Pointer(0) == iten)
    {
      // it is, get file name as one that others will compare with
      strIntersectingClass = fnClassFileName;
      // remember name
      strIntersectingName = strEntityName;
      // and description
      strIntersectingDescription = strEntityDescription;
    }
    else
    {
      // if any of the selected entities has different class
      if( strIntersectingClass != fnClassFileName)
      {
        // set multi class message
        strIntersectingClass = "Multi class";
      }
      if( strIntersectingName != iten->GetName())
      {
        // set multi class message
        strIntersectingName = "Multi name";
      }
      // if any of the selected entities has different class description
      if( strIntersectingDescription != iten->GetDescription())
      {
        // set multi class description message
        strIntersectingDescription = "Multi description";
      }
    }
  }
  m_strEntityClass = strIntersectingClass;
  m_strEntityName = strIntersectingName;
  m_strEntityDescription = strIntersectingDescription;
  // unlock selection's dynamic container
  pDoc->m_selEntitySelection.Unlock();
}

void CPropertyComboBar::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	CDialogBar::OnHScroll(nSBCode, nPos, pScrollBar);
  // copy color to selected entities
  SetColorPropertyToEntities( m_EditColorCtrl.GetColor());

  // refresh views
  CWorldEditorDoc *pDoc = theApp.GetDocument();
  // mark that document is changed
  pDoc->SetModifiedFlag( TRUE);
  // redraw all views
  pDoc->UpdateAllViews( NULL);
}

CEntity *CPropertyComboBar::GetSelectedEntityPtr(void) 
{
  // obtain selected property ID ptr
  CPropertyID *ppidProperty = GetSelectedProperty();
  // if there is valid property selected
  if( (ppidProperty == NULL) || 
	 ((ppidProperty->pid_eptType != CEntityProperty::EPT_ENTITYPTR) &&
	  (ppidProperty->pid_eptType != CEntityProperty::EPT_PARENT)) )
  {
    return NULL;
  }
  // get currently selected combo member
  INDEX iSelectedComboMember = m_EditEnumComboBox.GetCurSel();
  // it must exist
  ASSERT( iSelectedComboMember != CB_ERR);
  // get entity ptr to be set for all entities
  CEntity *penEntity = (CEntity *)m_EditEnumComboBox.GetItemData(iSelectedComboMember);
  return penEntity;
}

void CPropertyComboBar::OnUpdateNoFile(CCmdUI* pCmdUI)
{
  pCmdUI->Enable( TRUE);
}

void CPropertyComboBar::OnUpdateNoTarget(CCmdUI* pCmdUI)
{
  pCmdUI->Enable( TRUE);
}

void CPropertyComboBar::OnNoFile(void)
{
  CWorldEditorDoc *pDoc = theApp.GetDocument();
  if( pDoc == NULL) return;

  // obtain curently selected property ID
  CPropertyID *ppidProperty = GetSelectedProperty();
  if( ppidProperty == NULL) return;
  // for each of the selected entities
  FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
  {
    // obtain property ptr
    CEntityProperty *penpProperty = iten->PropertyForName( ppidProperty->pid_strName);
    // discard old entity settings
    iten->End();
    switch( ppidProperty->pid_eptType)
    {
    case CEntityProperty::EPT_FILENAMENODEP:
      ENTITYPROPERTY( &*iten, penpProperty->ep_slOffset, CTFileNameNoDep) = CTFileNameNoDep("");
      break;
    case CEntityProperty::EPT_FILENAME:
      ENTITYPROPERTY( &*iten, penpProperty->ep_slOffset, CTFileName) = CTFileName(CTString(""));
      break;
    }
    // apply new entity settings
    iten->Initialize();
  }
  // mark that document is changed
  pDoc->SetModifiedFlag( TRUE);
  pDoc->UpdateAllViews( NULL);
  pDoc->m_chSelections.MarkChanged();
  // reload data to dialog
  UpdateData( FALSE);
}

void CPropertyComboBar::ClearAllTargets(CEntity *penClicked)
{
  CWorldEditorDoc *pDoc = theApp.GetDocument();
  if( pDoc == NULL) return;
  if( penClicked == NULL) return;
  
  // if it is selected
  if( penClicked->IsSelected( ENF_SELECTED))
  {
    // for each of the selected entities
    FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
    {
      // discard old entity settings
      iten->End();
      // obtain entity class ptr
      CDLLEntityClass *pdecDLLClass = iten->GetClass()->ec_pdecDLLClass;
      // for all classes in hierarchy of this entity
      for(; pdecDLLClass!=NULL; pdecDLLClass = pdecDLLClass->dec_pdecBase)
      {
        // for all properties
        for(INDEX iProperty=0; iProperty<pdecDLLClass->dec_ctProperties; iProperty++)
        {
          CEntityProperty &epProperty = pdecDLLClass->dec_aepProperties[iProperty];
          if( epProperty.ep_eptType == CEntityProperty::EPT_ENTITYPTR)
          {
            // clear entity ptr
            ENTITYPROPERTY( &*iten, epProperty.ep_slOffset, CEntityPointer) = NULL;
          }
        }
      }
      // apply new entity settings
      iten->Initialize();
    }
  }
  // if is not selected
  else
  {
    // discard old entity settings
    penClicked->End();
    // obtain entity class ptr
    CDLLEntityClass *pdecDLLClass = penClicked->GetClass()->ec_pdecDLLClass;
    // for all classes in hierarchy of this entity
    for(; pdecDLLClass!=NULL; pdecDLLClass = pdecDLLClass->dec_pdecBase)
    {
      // for all properties
      for(INDEX iProperty=0; iProperty<pdecDLLClass->dec_ctProperties; iProperty++)
      {
        CEntityProperty &epProperty = pdecDLLClass->dec_aepProperties[iProperty];
        if( epProperty.ep_eptType == CEntityProperty::EPT_ENTITYPTR)
        {
          // clear entity ptr
          ENTITYPROPERTY( &*penClicked, epProperty.ep_slOffset, CEntityPointer) = NULL;
        }
      }
    }
    // apply new entity settings
    penClicked->Initialize();
  }
  pDoc->m_chSelections.MarkChanged();
  UpdateData( FALSE);
}

void CPropertyComboBar::SelectProperty(CEntityProperty *penpToMatch)
{
  CWorldEditorDoc *pDoc = theApp.GetDocument();
  if( pDoc == NULL) return;

  if( pDoc->m_selEntitySelection.Count() != 1)return;

  CEntity *pen = pDoc->m_selEntitySelection.GetFirstInSelection();
  if( pen == NULL) return;

  // note selection change
  m_PropertyComboBox.OnIdle( 0);

  for( INDEX iItem=0; iItem<m_PropertyComboBox.GetCount(); iItem++)
  {
    CPropertyID *ppid = (CPropertyID *) m_PropertyComboBox.GetItemData( iItem);
    CEntityProperty *penpProperty = pen->PropertyForName( ppid->pid_strName);
    if( penpProperty == penpToMatch)
    {
      m_PropertyComboBox.SetCurSel(iItem);
      m_PropertyComboBox.SelectProperty();
      return;
    }
  }
}

void CPropertyComboBar::OnNoTarget() 
{
  CWorldEditorDoc *pDoc = theApp.GetDocument();
  if( pDoc == NULL) return;

  // obtain curently selected property ID
  CPropertyID *ppidProperty = GetSelectedProperty();
  if( ppidProperty == NULL) return;
  // for each of the selected entities
  FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
  {
    // obtain property ptr
    CEntityProperty *penpProperty = iten->PropertyForName( ppidProperty->pid_strName);
    // discard old entity settings
    iten->End();
    switch( ppidProperty->pid_eptType)
    {
    case CEntityProperty::EPT_ENTITYPTR:
      ENTITYPROPERTY( &*iten, penpProperty->ep_slOffset, CEntityPointer) = NULL;
      break;
    case CEntityProperty::EPT_PARENT:
      iten->SetParent( NULL);
      break;
    }
    // apply new entity settings
    iten->Initialize();
  }
  // mark that document is changed
  pDoc->SetModifiedFlag( TRUE);
  pDoc->UpdateAllViews( NULL);
  pDoc->m_chSelections.MarkChanged();
  // reload data to dialog
  UpdateData( FALSE);
}
