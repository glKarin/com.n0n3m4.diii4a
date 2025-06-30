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

// SeriousSkaStudioDoc.cpp : implementation of the CSeriousSkaStudioDoc class
//

#include "stdafx.h"
#include "SeriousSkaStudio.h"

#include "SeriousSkaStudioDoc.h"

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSeriousSkaStudioDoc

IMPLEMENT_DYNCREATE(CSeriousSkaStudioDoc, CDocument)

BEGIN_MESSAGE_MAP(CSeriousSkaStudioDoc, CDocument)
	//{{AFX_MSG_MAP(CSeriousSkaStudioDoc)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSeriousSkaStudioDoc construction/destruction

CSeriousSkaStudioDoc::CSeriousSkaStudioDoc()
{
  m_ModelInstance = NULL;
  m_bModelInstanceChanged = FALSE;
  m_fSpeedZ = 0;
  m_fLoopSecends = 5;
  bAutoMiping = TRUE;
  bShowGround = TRUE;
  bAnimLoop = TRUE;
  fCustomMeshLodDist = 0;
  fCustomSkeletonLodDist = 0;
  m_tvStart=(-1I64);
  m_tvPauseStart=(-1I64);
  m_tvPauseTime=(0I64);
  m_bViewPaused = FALSE;
  bShowColisionBox = FALSE;
  bShowAllFramesBBox = FALSE;

  m_colAmbient = 0x404040FF;
  m_colLight = 0x505050FF;
  m_vLightDir = FLOAT3D(145,-45,0);
  bShowLights = FALSE;
}

CSeriousSkaStudioDoc::~CSeriousSkaStudioDoc()
{
}

// set flag that this document has changed and need to be saved
void CSeriousSkaStudioDoc::MarkAsChanged()
{
  // theApp.ErrorMessage("MarkAsChanged");
  m_bModelInstanceChanged = TRUE;
}

void CSeriousSkaStudioDoc::OnIdle(void)
{
  POSITION pos = GetFirstViewPosition();
  while ( pos !=NULL)
  {
    CSeriousSkaStudioView *pmvCurrent = (CSeriousSkaStudioView *) GetNextView(pos);
    // if children are maximize
    if(theApp.bChildrenMaximized)
    {
      // draw only front window
      if(pmvCurrent->m_iViewSize == SIZE_MAXIMIZED) pmvCurrent->OnIdle();
    }
    else
    {
      pmvCurrent->OnIdle();
    }
  }
}

// Set timer for this document
void CSeriousSkaStudioDoc::SetTimerForDocument()
{
  if( _pTimer != NULL)
  {
    CTimerValue tvNow = _pTimer->GetHighPrecisionTimer();
    if (m_tvStart.tv_llValue==-1I64) {
      m_tvStart = tvNow;
    }
    // if paused
    if(m_bViewPaused) {
      // set current time as time when paused
      tvNow = m_tvPauseStart - m_tvPauseTime;
    } else {
      // substract time in pause from timer to get continues animation
      tvNow -= m_tvPauseTime;
    }
    CTimerValue tvDelta = tvNow-m_tvStart;
    double dSecs = tvDelta.GetSeconds();
    INDEX ctTicks = floor(dSecs/_pTimer->TickQuantum);
    TIME tmTick = ctTicks*_pTimer->TickQuantum;
    FLOAT fFactor = (dSecs-tmTick)/_pTimer->TickQuantum;
    _pTimer->SetCurrentTick( tmTick );
    _pTimer->SetLerp( fFactor );
  }
}

BOOL CSeriousSkaStudioDoc::OnOpenDocument(LPCTSTR lpszPathName) 
{
  CTFileName fnModelFile = CTString(CStringA(lpszPathName));
  try {
    fnModelFile.RemoveApplicationPath_t();
  } catch( char *err_str) {
    AfxMessageBox( CString(err_str));
    return FALSE;
  }

  CModelInstance *pmi = theApp.OnOpenExistingInstance(fnModelFile);
  if(pmi == NULL) {
    // if failed to open smc
    theApp.ErrorMessage("Failed to open model instance '%s'",(const char*)fnModelFile);
    return FALSE;
  }
  // set root model instance
  m_ModelInstance = pmi;

  // flush stale caches
  _pShell->Execute("FreeUnusedStock();");
  return TRUE;
}

BOOL CSeriousSkaStudioDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument()) return FALSE;
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CSeriousSkaStudioDoc serialization
void CSeriousSkaStudioDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}

/////////////////////////////////////////////////////////////////////////////
// CSeriousSkaStudioDoc diagnostics
#ifdef _DEBUG
void CSeriousSkaStudioDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CSeriousSkaStudioDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

INDEX CSeriousSkaStudioDoc::BeforeDocumentClose()
{
  // if model instance was changed
  if(m_ModelInstance != NULL && m_bModelInstanceChanged) {
    CTString strText = CTString(0,"Save changes for '%s'",(const char*)m_ModelInstance->mi_fnSourceFile);
    // ask to save changes
    int iRet = AfxMessageBox(CString(strText),MB_YESNOCANCEL);
    // do not close doc
    if(iRet == IDCANCEL) {
      return FALSE;
    // save model instance
    } else if(iRet == IDYES) {
      // get current error count
      INDEX ctErrors = theApp.GetErrorList()->GetItemCount();
      // save root model instance
      theApp.SaveRootModel();
      // if new errors exists
      if(theApp.GetErrorList()->GetItemCount() != ctErrors) {
        // do not close document
        // return FALSE;
      }
      // else close doc
      return TRUE;
    // IDNO - close doc without saving
    } else {
      m_bModelInstanceChanged = FALSE;
      return TRUE;
    }
  }
  return TRUE;
}

// save current model instnce and clear tree view
void CSeriousSkaStudioDoc::OnCloseDocument() 
{
  if(!BeforeDocumentClose()) {
    return;
  }

  theApp.m_dlgBarTreeView.UpdateModelInstInfo(NULL);
  theApp.m_dlgBarTreeView.ResetControls();
  if(m_ModelInstance!=NULL) m_ModelInstance->Clear();
  m_ModelInstance = NULL;
  // flush stale caches
  _pShell->Execute("FreeUnusedStock();");
  theApp.ReloadRootModelInstance();
	CDocument::OnCloseDocument();
}
