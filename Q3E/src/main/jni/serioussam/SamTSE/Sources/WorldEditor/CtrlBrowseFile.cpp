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

// CtrlBrowseFile.cpp : implementation file
//

#include "stdafx.h"
#include "CtrlBrowseFile.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCtrlBrowseFile

CCtrlBrowseFile::CCtrlBrowseFile()
{
}

CCtrlBrowseFile::~CCtrlBrowseFile()
{
}

void CCtrlBrowseFile::SetDialogPtr( CPropertyComboBar *pDialog)
{
  m_pDialog = pDialog;
}


BEGIN_MESSAGE_MAP(CCtrlBrowseFile, CButton)
	//{{AFX_MSG_MAP(CCtrlBrowseFile)
	ON_CONTROL_REFLECT(BN_CLICKED, OnClicked)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCtrlBrowseFile message handlers
CTFileName CCtrlBrowseFile::GetIntersectingFile()
{
  // obtain curently active document
  CWorldEditorDoc *pDoc = theApp.GetDocument();
  ASSERT( pDoc != NULL);

  // obtain curently selected property ID
  CPropertyID *ppidProperty = m_pDialog->GetSelectedProperty();
  if( ppidProperty == NULL ||
      !((ppidProperty->pid_eptType == CEntityProperty::EPT_FILENAME) ||
        (ppidProperty->pid_eptType == CEntityProperty::EPT_FILENAMENODEP)) ) return CTString("");

  // lock selection's dynamic container
  pDoc->m_selEntitySelection.Lock();
  // file name to contain selection intersecting file
  CTFileName fnIntersectingFile;
  CTFileNameNoDep fnIntersectingFileNoDep;
  // for each of the selected entities
  FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
  {
    // obtain property ptr
    CEntityProperty *pepProperty = iten->PropertyForName( ppidProperty->pid_strName);
    // if this is first entity in dynamic container
    if( pDoc->m_selEntitySelection.Pointer(0) == iten)
    {
      if( m_bFileNameNoDep)
      {
        fnIntersectingFileNoDep = ENTITYPROPERTY( &*iten, pepProperty->ep_slOffset, CTFileNameNoDep);
      }
      else
      {
        fnIntersectingFile = ENTITYPROPERTY( &*iten, pepProperty->ep_slOffset, CTFileName);
      }
    }
    else
    {
      if( m_bFileNameNoDep)
      {
        CTFileNameNoDep fnCurrentFileNoDep = ENTITYPROPERTY( &*iten, pepProperty->ep_slOffset, CTFileNameNoDep);
        if( fnCurrentFileNoDep != fnIntersectingFileNoDep)
        {
          fnIntersectingFileNoDep = CTString("");
          break;
        }
      }
      else
      {
        CTFileName fnCurrentFile = ENTITYPROPERTY( &*iten, pepProperty->ep_slOffset, CTFileName);
        if( fnCurrentFile != fnIntersectingFile)
        {
          fnIntersectingFile = CTString("");
          break;
        }
      }
    }
  }
  // unlock selection's dynamic container
  pDoc->m_selEntitySelection.Unlock();
  if( m_bFileNameNoDep)
  {
    return fnIntersectingFileNoDep;
  }
  else
  {
    return fnIntersectingFile;
  }
}

void CCtrlBrowseFile::OnClicked()
{
  // don't do anything if document doesn't exist
  if( theApp.GetDocument() == NULL) return;
  // obtain curently active document
  CWorldEditorDoc *pDoc = theApp.GetDocument();

  // obtain curently selected property ID
  CPropertyID *ppidProperty = m_pDialog->GetSelectedProperty();
  if( ppidProperty == NULL) return;

  // file name to contain selection intersecting file
  CTFileName fnIntersectingFile = GetIntersectingFile();

  // call file requester
  CTFileName fnChoosedFile;
  if( fnIntersectingFile.FileExt() == ".mdl")
  {
    fnChoosedFile = _EngineGUI.FileRequester( "Choose file",
    FILTER_MDL FILTER_ALL FILTER_END,
    KEY_NAME_REQUEST_FILE_DIR, fnIntersectingFile.FileDir(),
    fnIntersectingFile.FileName()+fnIntersectingFile.FileExt());
  }
  else if( fnIntersectingFile.FileExt() == ".tex")
  {
    fnChoosedFile = _EngineGUI.FileRequester( "Choose file",
    FILTER_TEX FILTER_ALL FILTER_END,
    KEY_NAME_REQUEST_FILE_DIR, fnIntersectingFile.FileDir(),
    fnIntersectingFile.FileName()+fnIntersectingFile.FileExt());
  }
  else if( fnIntersectingFile.FileExt() == ".wav")
  {
    fnChoosedFile = _EngineGUI.FileRequester( "Choose file",
    FILTER_WAV FILTER_ALL FILTER_END,
    KEY_NAME_REQUEST_FILE_DIR, fnIntersectingFile.FileDir(),
    fnIntersectingFile.FileName()+fnIntersectingFile.FileExt());
  }
  else if( fnIntersectingFile.FileExt() == ".smc")
  {
    fnChoosedFile = _EngineGUI.FileRequester( "Choose file",
    FILTER_SMC FILTER_ALL FILTER_END,
    KEY_NAME_REQUEST_FILE_DIR, fnIntersectingFile.FileDir(),
    fnIntersectingFile.FileName()+fnIntersectingFile.FileExt());
  }
  else
  {
    fnChoosedFile = _EngineGUI.FileRequester( "Choose file",
    FILTER_ALL FILTER_MDL FILTER_TEX FILTER_WAV FILTER_END,
    KEY_NAME_REQUEST_FILE_DIR, fnIntersectingFile.FileDir(),
    fnIntersectingFile.FileName()+fnIntersectingFile.FileExt());
  }

  if( fnChoosedFile == "") return;

  // for each of the selected entities
  FOREACHINDYNAMICCONTAINER(pDoc->m_selEntitySelection, CEntity, iten)
  {
    // obtain property ptr
    CEntityProperty *penpProperty = iten->PropertyForName( ppidProperty->pid_strName);
    // discard old entity settings
    iten->End();
    // set new file name value
    if( m_bFileNameNoDep)
    {
      ENTITYPROPERTY( &*iten, penpProperty->ep_slOffset, CTFileNameNoDep) =
        (CTFileNameNoDep) fnChoosedFile;
    }
    else
    {
      ENTITYPROPERTY( &*iten, penpProperty->ep_slOffset, CTFileName) = fnChoosedFile;
    }
    // apply new entity settings
    iten->Initialize();
  }
  // mark that document is changed
  pDoc->SetModifiedFlag( TRUE);
  pDoc->UpdateAllViews( NULL);
  pDoc->m_chSelections.MarkChanged();
  // reload data to dialog
  m_pDialog->UpdateData( FALSE);
}
