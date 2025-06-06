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

// ModelerDoc.cpp : implementation of the CModelerDoc class
//

#include "stdafx.h"
#include <Engine/Templates/Stock_CAnimData.h>
#include <Engine/Templates/Stock_CTextureData.h>
#include <Engine/Templates/Stock_CSoundData.h>
#include <Engine/Templates/Stock_CModelData.h>

#include <Engine/Build.h>

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CModelerDoc

IMPLEMENT_DYNCREATE(CModelerDoc, CDocument)

BEGIN_MESSAGE_MAP(CModelerDoc, CDocument)
	//{{AFX_MSG_MAP(CModelerDoc)
	ON_COMMAND(ID_FILE_ADD_TEXTURE, OnFileAddTexture)
	ON_UPDATE_COMMAND_UI(ID_FILE_ADD_TEXTURE, OnUpdateFileAddTexture)
	ON_COMMAND(ID_NEXT_SURFACE, OnNextSurface)
	ON_COMMAND(ID_PREV_SURFACE, OnPrevSurface)
	ON_COMMAND(ID_LINK_SURFACES, OnLinkSurfaces)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CModelerDoc construction/destruction

CModelerDoc::CModelerDoc()
{
  // Default clipboard variables used for storing copy/paste data
  m_f3ClipboardCenter = FLOAT3D( 0.0f, 0.0f, 0.0f);
  m_fClipboardZoom = FLOAT( 0.0f);
  m_f3ClipboardHPB = FLOAT3D( 0.0f, 0.0f, 0.0f);
  m_iCurrentMip = 0;
}

CModelerDoc::~CModelerDoc()
{
  _pAnimStock->FreeUnused();
  _pTextureStock->FreeUnused();
  _pModelStock->FreeUnused();
  _pSoundStock->FreeUnused();
}

BOOL CModelerDoc::CreateModelFromScriptFile( CTFileName fnScrFileName, char *strError)
{
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  try
  {
    CWaitCursor StartWaitCursor;
   	
    CRect rectMainFrameSize;
    CRect rectProgress, rectProgressNew;
    
    pMainFrame->GetWindowRect( &rectMainFrameSize);
    pMainFrame->m_NewProgress.Create( IDD_NEW_PROGRESS, pMainFrame);
    pMainFrame->m_NewProgress.GetWindowRect( &rectProgress);
    
    rectProgressNew.left = rectMainFrameSize.Width()/2 - rectProgress.Width()/2;
    rectProgressNew.top = rectMainFrameSize.Height()/2 - rectProgress.Height()/2;
    rectProgressNew.right = rectProgressNew.left + rectProgress.Width();
    rectProgressNew.bottom = rectProgressNew.top + rectProgress.Height();

    pMainFrame->m_NewProgress.MoveWindow( rectProgressNew);
    pMainFrame->m_NewProgress.ShowWindow(SW_SHOW);

    m_emEditModel.LoadFromScript_t( fnScrFileName);
  }
  catch( char *err_str)
  {
    strcpy( strError, err_str);
    pMainFrame->m_NewProgress.DestroyWindow();
    return FALSE;
  }
  pMainFrame->m_NewProgress.DestroyWindow();
	m_bDocLoadedOk = TRUE;
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CModelerDoc serialization

void CModelerDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
	}
	else
	{
	}
}

/////////////////////////////////////////////////////////////////////////////
// CModelerDoc diagnostics

#ifdef _DEBUG
void CModelerDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CModelerDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CModelerDoc commands

BOOL CModelerDoc::OnOpenDocument(LPCTSTR lpszPathName) 
{
	m_bDocLoadedOk = FALSE;
  CTFileName fnModelFile = CTString(CStringA(lpszPathName));

  try
  {
    fnModelFile.RemoveApplicationPath_t();
  }
  catch( char *err_str)
  {
    AfxMessageBox( CString(err_str));
    return FALSE;
  }

  try
  {
    m_emEditModel.Load_t( fnModelFile);
  }
  catch( char *err_str)
  {
    CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
    MessageBoxA(pMainFrame->m_hWnd, err_str, "Warning! Model load failed.", MB_OK|MB_ICONHAND|MB_SYSTEMMODAL);
    return FALSE;
  }

  INDEX ctLoadedModels=m_emEditModel.edm_aamAttachedModels.Count();
  INDEX ctAttachmentPositions=m_emEditModel.edm_md.md_aampAttachedPosition.Count();
  
  if(ctLoadedModels<ctAttachmentPositions)
  {
    for( INDEX iPos=ctLoadedModels; iPos<ctAttachmentPositions; iPos++)
    {
      m_emEditModel.edm_aamAttachedModels.New();
      m_emEditModel.edm_aamAttachedModels.Lock();
      CAttachedModel &am=m_emEditModel.edm_aamAttachedModels[iPos];
      DECLARE_CTFILENAME( fnAxis, "Models\\Editor\\Axis.mdl");
      am.am_bVisible=TRUE;
      am.am_strName="Not loaded";
      am.am_iAnimation=0;
      try
      {
        am.SetModel_t(fnAxis);
      }
      catch( char *strError)
      {
        WarningMessage( strError);
      }
      m_emEditModel.edm_aamAttachedModels.Unlock();
    }
  }

	m_bDocLoadedOk = TRUE;
  // flush stale caches
  _pShell->Execute("FreeUnusedStock();");
  SelectSurface( 0, TRUE);
  return TRUE;
}

BOOL CModelerDoc::OnSaveDocument(LPCTSTR lpszPathName) 
{
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
	//return CDocument::OnSaveDocument(lpszPathName);
  CTFileName fnModelFile = CTString(CStringA(lpszPathName));
  try
  {
    fnModelFile.RemoveApplicationPath_t();
  }
  catch( char *err_str)
  {
    AfxMessageBox( CString(err_str));
    return FALSE;
  }

  try
  {
  	m_emEditModel.Save_t( fnModelFile);
    m_emEditModel.SaveMapping_t( fnModelFile.NoExt()+".map", 0);
  }
  catch( char *err_str)
  {
    MessageBoxA(pMainFrame->m_hWnd, err_str, "Warning! Model Save failed.", MB_OK|MB_ICONHAND|MB_SYSTEMMODAL);
    return FALSE;
  }
  SetModifiedFlag( FALSE);

  CModelerView *pModelerView = CModelerView::GetActiveView();
  if( pModelerView != NULL)
  {
    pModelerView->SaveThumbnail();
  }

  // reload attachments of all loaded models 
  POSITION pos = theApp.m_pdtModelDocTemplate->GetFirstDocPosition();
  while (pos!=NULL)
  {
    CModelerDoc *pmdCurrent = (CModelerDoc *)theApp.m_pdtModelDocTemplate->GetNextDoc(pos);
    if( pmdCurrent != this)
    {
      BOOL bUpdateAttachments = TRUE;
      
      // if document is modified
      if( pmdCurrent->IsModified())
      {
        CTString strMessage;
        CTFileName fnDoc = CTString(CStringA(pmdCurrent->GetPathName()));
        strMessage.PrintF("Do you want to save model \"%s\" before reloading its attachments?", fnDoc.FileName() );
        if( ::MessageBoxA( pMainFrame->m_hWnd, strMessage,
                        "Warning !", MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON1 | 
                        MB_TASKMODAL | MB_TOPMOST) != IDYES)
        {
          bUpdateAttachments = FALSE;
        }
        // save document
        else
        {
          pmdCurrent->OnSaveDocument(pmdCurrent->GetPathName());
        }
      }

      if( bUpdateAttachments)
      {
        POSITION pos = pmdCurrent->GetFirstViewPosition();
        while (pos != NULL)
        {
          CView* pView = GetNextView(pos);
          ((CModelerView *)pView)->m_ModelObject.AutoSetAttachments();
          
          //CModelData *pmd = (CModelData *) ((CModelerView *)pView)->m_ModelObject.GetData();
          //pmd->Reload();
        }
      }
    }
  }


  return TRUE;
}

void CModelerDoc::OnIdle(void)
{
  POSITION pos = GetFirstViewPosition();

  while ( pos !=NULL) {
    CModelerView *pmvCurrent = (CModelerView *) GetNextView(pos);
    pmvCurrent->OnIdle();
  }
}

// overridden from mfc to discard rendering precalculations
void CModelerDoc::SetModifiedFlag( BOOL bModified /*= TRUE*/ )
{
  CDocument::SetModifiedFlag(bModified);
}

extern UINT APIENTRY ModelerFileRequesterHook( HWND hdlg, UINT uiMsg, WPARAM wParam,	LPARAM lParam);

void CModelerDoc::OnFileAddTexture() 
{
  CModelerView *pModelerView = CModelerView::GetActiveView();
  if( pModelerView == NULL)
    return;

  // call file requester for adding textures
  CDynamicArray<CTFileName> afnTextures;
  CTFileName fnDocName = CTString(CStringA(GetPathName()));
  theApp.WriteProfileString( L"Scape", L"Add texture directory", CString(fnDocName.FileDir()));
  _EngineGUI.FileRequester( "Choose textures to add", FILTER_TEX FILTER_END,
    "Add texture directory", "Textures\\", fnDocName.FileName()+".tex", &afnTextures);
  MEX mexWidth, mexHeight;
  m_emEditModel.edm_md.GetTextureDimensions( mexWidth, mexHeight);
  // add selected textures
  FOREACHINDYNAMICARRAY( afnTextures, CTFileName, itTexture)
  {
    CTextureDataInfo *pNewTDI;
    // add texture
    CTFileName fnTexName = itTexture.Current();
    try
    {
      pNewTDI =m_emEditModel.AddTexture_t( fnTexName, mexWidth, mexHeight);
    }
    catch( char *err_str)
    {
      pNewTDI = NULL;
      AfxMessageBox( CString(err_str));
    }
    if( pNewTDI != NULL)
    {
      SetModifiedFlag();
      pModelerView->m_ptdiTextureDataInfo = pNewTDI;
      // switch to texture mode
      pModelerView->OnRendUseTexture();
      UpdateAllViews( NULL);
    }
  }
}

void CModelerDoc::OnUpdateFileAddTexture(CCmdUI* pCmdUI) 
{
  CModelerView *pModelerView = CModelerView::GetActiveView();
  pCmdUI->Enable( pModelerView != NULL);
}

CTString CModelerDoc::GetModelDirectory( void)
{
  CTFileName fnResult = CTFileName( CTString(CStringA(GetPathName()))).FileDir();
  return CTString( fnResult);
}

CTString CModelerDoc::GetModelName( void)
{
  CTFileName fnResult = CTFileName( CTString(CStringA(GetPathName()))).FileName();
  return CTString( fnResult);
}

void CModelerDoc::SelectMipModel( INDEX iMipToSelect)
{
  m_iCurrentMip = iMipToSelect;
  POSITION pos = GetFirstViewPosition();

  while ( pos !=NULL) {
    CModelerView *pmvCurrent = (CModelerView *) GetNextView(pos);
    pmvCurrent->m_ModelObject.SetManualMipLevel( iMipToSelect);
  }
  SpreadSurfaceSelection();
  theApp.m_chGlobal.MarkChanged();
}

void CModelerDoc::ClearAttachments( void)
{
  POSITION pos = GetFirstViewPosition();
  while ( pos !=NULL)
  {
    CModelerView *pmvView = (CModelerView *) GetNextView(pos);
    pmvView->m_ModelObject.RemoveAllAttachmentModels();
  }
}

void CModelerDoc::SetupAttachments( void)
{
  POSITION pos = GetFirstViewPosition();
  while ( pos !=NULL)
  {
    CModelerView *pmvView = (CModelerView *) GetNextView(pos);
    INDEX iAttachment = 0;
    FOREACHINDYNAMICARRAY( m_emEditModel.edm_aamAttachedModels, CAttachedModel, itam)
    {
      m_emEditModel.edm_aamAttachedModels.Lock();
      CAttachedModel *pamAttachedModel = &m_emEditModel.edm_aamAttachedModels[ iAttachment];
      if( pamAttachedModel->am_bVisible)
      {
        CAttachmentModelObject *pamo = pmvView->m_ModelObject.AddAttachmentModel( iAttachment);
        CModelData *pMD = (CModelData *) pamAttachedModel->am_moAttachedModel.GetData();
        ASSERT(pMD != NULL);
        pamo->amo_moModelObject.SetData( pMD);
        pamo->amo_moModelObject.AutoSetTextures();
        pamo->amo_moModelObject.AutoSetAttachments();
        pamo->amo_moModelObject.StartAnim( itam->am_iAnimation);
      }
      m_emEditModel.edm_aamAttachedModels.Unlock();
      iAttachment++;
    }
  }
}

void CModelerDoc::ClearSurfaceSelection(void)
{
  ModelMipInfo &mmi = m_emEditModel.edm_md.md_MipInfos[ m_iCurrentMip];
  for( INDEX iSurface=0; iSurface<mmi.mmpi_MappingSurfaces.Count(); iSurface++) {
    MappingSurface &ms = mmi.mmpi_MappingSurfaces[ iSurface];
    ms.ms_ulRenderingFlags &= ~SRF_SELECTED;
  }
}

void CModelerDoc::SelectAllSurfaces(void)
{
  ModelMipInfo &mmi = m_emEditModel.edm_md.md_MipInfos[ m_iCurrentMip];
  for( INDEX iSurface=0; iSurface<mmi.mmpi_MappingSurfaces.Count(); iSurface++) {
    MappingSurface &ms = mmi.mmpi_MappingSurfaces[ iSurface];
    ms.ms_ulRenderingFlags |= SRF_SELECTED;
  }
}

INDEX CModelerDoc::GetCountOfSelectedSurfaces(void)
{
  INDEX ctSelectedSurfaces = 0;
  ModelMipInfo &mmi = m_emEditModel.edm_md.md_MipInfos[ m_iCurrentMip];
  for( INDEX iSurface=0; iSurface<mmi.mmpi_MappingSurfaces.Count(); iSurface++)
  {
    MappingSurface &ms = mmi.mmpi_MappingSurfaces[ iSurface];
    if( ms.ms_ulRenderingFlags&SRF_SELECTED) ctSelectedSurfaces++;
  }
  return ctSelectedSurfaces;
}

INDEX CModelerDoc::GetOnlySelectedSurface(void)
{
  if( GetCountOfSelectedSurfaces() != 1) return -1;

  ModelMipInfo &mmi = m_emEditModel.edm_md.md_MipInfos[ m_iCurrentMip];
  INDEX iSurface=0;
  for( ; iSurface<mmi.mmpi_MappingSurfaces.Count(); iSurface++)
  {
    MappingSurface &ms = mmi.mmpi_MappingSurfaces[ iSurface];
    if( ms.ms_ulRenderingFlags&SRF_SELECTED) break;
  }
  return iSurface;
}

void CModelerDoc::OnPrevSurface() 
{
  SelectPreviousSurface();
}

void CModelerDoc::OnNextSurface() 
{
  SelectNextSurface();
}

void CModelerDoc::SelectPreviousSurface(void)
{
  ModelMipInfo &mmi = m_emEditModel.edm_md.md_MipInfos[ m_iCurrentMip];
  INDEX iFirstSelected = 1;
  for( INDEX iSurface=0; iSurface<mmi.mmpi_MappingSurfaces.Count(); iSurface++) {
    MappingSurface &ms = mmi.mmpi_MappingSurfaces[ iSurface];
    if( ms.ms_ulRenderingFlags&SRF_SELECTED) {
      iFirstSelected = iSurface;
      break;
    }
  }
  ClearSurfaceSelection();
  iFirstSelected = (iFirstSelected+mmi.mmpi_MappingSurfaces.Count()-1)%mmi.mmpi_MappingSurfaces.Count();
  MappingSurface &ms = mmi.mmpi_MappingSurfaces[ iFirstSelected];
  ms.ms_ulRenderingFlags |= SRF_SELECTED;
  theApp.m_chGlobal.MarkChanged();
}

void CModelerDoc::SelectSurface(INDEX iSurface, BOOL bClearRest)
{
  ModelMipInfo &mmi = m_emEditModel.edm_md.md_MipInfos[ m_iCurrentMip];
  if( iSurface >= mmi.mmpi_MappingSurfaces.Count()) return;
  if( bClearRest) ClearSurfaceSelection();
  MappingSurface &ms = mmi.mmpi_MappingSurfaces[ iSurface];
  ms.ms_ulRenderingFlags |= SRF_SELECTED;
  theApp.m_chGlobal.MarkChanged();
}

void CModelerDoc::SelectNextSurface(void)
{
  ModelMipInfo &mmi = m_emEditModel.edm_md.md_MipInfos[ m_iCurrentMip];
  INDEX iFirstSelected = 1;
  for( INDEX iSurface=0; iSurface<mmi.mmpi_MappingSurfaces.Count(); iSurface++) {
    MappingSurface &ms = mmi.mmpi_MappingSurfaces[ iSurface];
    if( ms.ms_ulRenderingFlags&SRF_SELECTED) {
      iFirstSelected = iSurface;
      break;
    }
  }
  ClearSurfaceSelection();
  iFirstSelected = (iFirstSelected+1)%mmi.mmpi_MappingSurfaces.Count();
  MappingSurface &ms = mmi.mmpi_MappingSurfaces[ iFirstSelected];
  ms.ms_ulRenderingFlags |= SRF_SELECTED;
  theApp.m_chGlobal.MarkChanged();
}

void CModelerDoc::ToggleSurfaceSelection( INDEX iSurfaceToToggle)
{
  BOOL bAllreadyExisted = FALSE;

  ClearSurfaceSelection();
  ModelMipInfo &mmi = m_emEditModel.edm_md.md_MipInfos[ m_iCurrentMip];
  MappingSurface &ms = mmi.mmpi_MappingSurfaces[ iSurfaceToToggle];
  ms.ms_ulRenderingFlags ^= SRF_SELECTED;
  theApp.m_chGlobal.MarkChanged();
}

void CModelerDoc::SpreadSurfaceSelection( void)
{
  ModelMipInfo &mmiSrc = m_emEditModel.edm_md.md_MipInfos[ 0];
  for( INDEX iMip=1; iMip<m_emEditModel.edm_md.md_MipCt; iMip++) {
    ModelMipInfo &mmiDst = m_emEditModel.edm_md.md_MipInfos[ iMip];
    // copy selected flag for surfaces with same names
    for( INDEX iSurfaceSrc=0; iSurfaceSrc<mmiSrc.mmpi_MappingSurfaces.Count(); iSurfaceSrc++) {
      MappingSurface &msSrc = mmiSrc.mmpi_MappingSurfaces[ iSurfaceSrc];
      for( INDEX iSurfaceDst=0; iSurfaceDst<mmiDst.mmpi_MappingSurfaces.Count(); iSurfaceDst++) {
        MappingSurface &msDst = mmiDst.mmpi_MappingSurfaces[ iSurfaceDst];
        if( CTString(msSrc.ms_Name) == msDst.ms_Name) {
          msDst.ms_ulRenderingFlags &= ~SRF_SELECTED;
          msDst.ms_ulRenderingFlags |= msSrc.ms_ulRenderingFlags & SRF_SELECTED;
        }
      }
    }
  }
  theApp.m_chGlobal.MarkChanged();
}

void CModelerDoc::OnLinkSurfaces() 
{
  CDlgMarkLinkedSurfaces dlgMarkLinkedSurfaces;
  dlgMarkLinkedSurfaces.DoModal();
  UpdateAllViews( NULL);
}
