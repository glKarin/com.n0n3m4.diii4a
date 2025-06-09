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

#include "stdafx.h"
#include "SeriousSkaStudio.h"
#include "SeriousSkaStudioDoc.h"
#include "SeriousSkaStudioView.h"

#include "MainFrm.h"
#include "ChildFrm.h"
#include "DlgBarTreeView.h"
#include "SplitterFrame.h"
#include "resource.h"
#include <afxwin.h>

#include <Engine/Ska/ModelInstance.h>
#include <Engine/Templates/Stock_CMesh.h>
#include <Engine/Templates/Stock_CSkeleton.h>
#include <Engine/Templates/Stock_CAnimSet.h>
#include <Engine/Templates/Stock_CTextureData.h>

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define MAKESPACE(x) (x>0?"%*c":""),x,' '

// parser stuff
#include "ParsingSymbols.h" 
INDEX _yy_iLine;
extern int include_stack_ptr;
extern CTFileName strCurentFileName;
extern CDynamicStackArray<CTString> astrText;

CMesh *_yy_pMesh;
CSkeleton *_yy_pSkeleton;
CAnimSet *_yy_pAnimSet;
INDEX _yy_ctAnimSets;

INDEX _yy_iIndex;  // index for parser
INDEX _yy_jIndex;  // index for parser
INDEX _yy_iWIndex; // index for weightmaps in parser
INDEX _yy_iMIndex; // index for mophmaps in parser

// counters for optimization calculation
INDEX ctMeshVxBeforeOpt = 0;
INDEX ctMeshVxAfterOpt = 0;

#define LAMP_MODEL_FILENAME   "Models\\SkaStudio\\Lamp\\Lamp.smc"

/////////////////////////////////////////////////////////////////////////////
// CSeriousSkaStudioApp

BEGIN_MESSAGE_MAP(CSeriousSkaStudioApp, CWinApp)
	//{{AFX_MSG_MAP(CSeriousSkaStudioApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
	ON_COMMAND(ID_FILE_NEW, OnFileNew)
	ON_COMMAND(ID_IMPORT_CONVERT, OnImportConvert)
	//}}AFX_MSG_MAP
	// Standard file based document commands
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSeriousSkaStudioApp construction

CSeriousSkaStudioApp::CSeriousSkaStudioApp()
{
  ctNoRenderRequests = 0;
  bAppInitialized = FALSE;
}

void CSeriousSkaStudioApp::DisableRendering()
{
  ctNoRenderRequests++;
}
void CSeriousSkaStudioApp::EnableRendering()
{
  ctNoRenderRequests--;
  ASSERT(ctNoRenderRequests>=0);
  ctNoRenderRequests = ClampDn(ctNoRenderRequests,(INDEX)0);
}
INDEX CSeriousSkaStudioApp::GetDisableRenderRequests()
{
  return ctNoRenderRequests;
}
/////////////////////////////////////////////////////////////////////////////
// The one and only CSeriousSkaStudioApp object

CSeriousSkaStudioApp theApp;
CModelInstance *pmiSelected = NULL;

BOOL CSeriousSkaStudioApp::InitInstance()
{
  _CrtSetBreakAlloc(55);
  BOOL bResult;
  CTSTREAM_BEGIN {
    bResult = SubInitInstance();
  } CTSTREAM_END;
  return bResult;
}

int CSeriousSkaStudioApp::Run()
{
	int iResult;
  CTSTREAM_BEGIN {
    iResult=CWinApp::Run();
  } CTSTREAM_END;
	return iResult;
}

BOOL CSeriousSkaStudioApp::SubInitInstance()
{
	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

	// Change the registry key under which our settings are stored.
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization.
	SetRegistryKey( L"CroTeam");

	LoadStdProfileSettings(8);  // Load standard INI file options (including MRU)

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views.

  CMultiDocTemplate* pDocTemplate;
  m_pdtDocTemplate = pDocTemplate = new CMultiDocTemplate(
		IDR_SERIOUTYPE,
		RUNTIME_CLASS(CSeriousSkaStudioDoc),
		RUNTIME_CLASS(CChildFrame), // custom MDI child frame
		RUNTIME_CLASS(CSeriousSkaStudioView));
	AddDocTemplate(m_pdtDocTemplate);

  // initialize entire engine
  SE_InitEngine("");
  SE_LoadDefaultFonts();
  // remember both compressed and uncompressed rotations in animations
  RememberUnCompresedRotatations(TRUE);

  // (re)set default display mode
  _pGfx->ResetDisplayMode(GAT_OGL);

  CTFileName fnGroundTexture = (CTString)"Models\\Editor\\Floor.tex";
  // CTFileName fnGroundTexture = (CTString)"ModelsSka\\Enemies\\Grunt\\SoldierAnim.tex";
  // CTFileName fnGroundTexture = (CTString)"Models\\Enemies\\ElementalLava\\Lava04FX.tex";
  try
  {
    toGroundTexture.SetData_t(fnGroundTexture);
  }
  catch(char *strError)
  {
    ErrorMessage(strError);
    return FALSE;
  }
  
  // create main MDI Frame window
	CMainFrame* pMainFrame = new CMainFrame;
	if (!pMainFrame->LoadFrame(IDR_MAINFRAME))
		return FALSE;
	m_pMainWnd = pMainFrame;

  // Enable drag/drop open
	m_pMainWnd->DragAcceptFiles();

	// Enable DDE Execute open
	EnableShellOpen();
	RegisterShellFileTypes(TRUE);

	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	
  // don't start new document automatically
  cmdInfo.m_nShellCommand = CCommandLineInfo::FileNothing;

  ParseCommandLine(cmdInfo);

	// Dispatch commands specified on the command line
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;

  // load startup script
  _pShell->Execute( "include \"Scripts\\SkaStudio_Startup.ini\"");

  // try to load lamp model 
  try
  {
    pmiLight = ParseSmcFile_t(_fnmApplicationPath + LAMP_MODEL_FILENAME);
    pmiLight->StretchModel(FLOAT3D(.5f,.5f,.5f))  ;
  }
  catch(char *strError) 
  {
    if(pmiLight) pmiLight->Clear();
    ErrorMessage("%s",strError);
    // return FALSE;
  }


  // theApp.m_wndSpliterLogFrame.SubclassDlgItem(IDC_SPLITER_LOG_FRAME,&m_dlgErrorList);
  // theApp.m_wndSpliterLogFrame.SetOrientation(SPF_TOP);
  // The main window has been initialized, so show and update it.
	pMainFrame->ShowWindow(m_nCmdShow|SW_SHOWMAXIMIZED);
	pMainFrame->UpdateWindow();
	return TRUE;
}

BOOL CSeriousSkaStudioApp::OnIdle(LONG lCount) 
{
  POSITION pos = m_pdtDocTemplate->GetFirstDocPosition();
  while (pos!=NULL)
  {
    CSeriousSkaStudioDoc *pmdCurrent = (CSeriousSkaStudioDoc *)m_pdtDocTemplate->GetNextDoc(pos);
    pmdCurrent->OnIdle();
  }
  extern BOOL _bApplicationActive;
  return CWinApp::OnIdle(lCount) || _bApplicationActive;

}

void CSeriousSkaStudioApp::OnFileNew()
{
  // add new model instance
  CModelInstance *pmi = OnAddNewModelInstance();
  if(pmi == NULL)
  {
    //delete pDocument;
    theApp.ErrorMessage("Failed to create model instance");
    return;
  }

  CDocument* pDocument = m_pdtDocTemplate->CreateNewDocument();
  if (pDocument == NULL)
	{
		TRACE0("CDocTemplate::CreateNewDocument returned NULL.\n");
		AfxMessageBox(AFX_IDP_FAILED_TO_CREATE_DOC);
		return;
	}
	ASSERT_VALID(pDocument);
  // View creation
	CFrameWnd* pFrame = m_pdtDocTemplate->CreateNewFrame(pDocument, NULL);
	if (pFrame == NULL)
	{
		AfxMessageBox(AFX_IDP_FAILED_TO_CREATE_DOC);
		delete pDocument;       // explicit delete on error
		return;
	}
	ASSERT_VALID(pFrame);

  // add file to mru
  CTString strFileName = pmi->mi_fnSourceFile.FileName() + pmi->mi_fnSourceFile.FileExt();
  CString strOpenPath;
  strOpenPath = theApp.GetProfileString(L"SeriousSkaStudio", L"Open directory", L"");
  strOpenPath += pmi->mi_fnSourceFile;

  pDocument->SetPathName(CString(strFileName), TRUE);
  pDocument->SetTitle(CString(strFileName));

  pDocument->SetModifiedFlag( FALSE);
	m_pdtDocTemplate->InitialUpdateFrame(pFrame, pDocument, TRUE);
  // set root model instance
  theApp.GetDocument()->m_ModelInstance = pmi;
  // theApp.SaveRootModel();
  // fill tree view with new model instance
  // ReloadRootModelInstance();
  UpdateRootModelInstance();
}

void CSeriousSkaStudioApp::OnFileOpen() 
{
  CTFileName fnSim;
  // get file name  
  fnSim = _EngineGUI.FileRequester( "Select existing Smc file",
    "ASCII model files (*.smc)\0*.smc\0"
    "All files (*.*)\0*.*\0\0",
    "Open directory", "Models\\", "");
  if (fnSim=="") return;

  CTFileName fnFull;
  fnFull = _fnmApplicationPath + fnSim;

  CModelInstance *pmi = OnOpenExistingInstance(fnSim);
  if(pmi == NULL)
  {
    // if faile to open smc
    theApp.ErrorMessage("Failed to open model instance '%s'",(const char*)fnSim);
    return;
  }
  // create new document
  CDocument* pDocument = m_pdtDocTemplate->CreateNewDocument();
  if (pDocument == NULL)
	{
		TRACE0("CDocTemplate::CreateNewDocument returned NULL.\n");
		AfxMessageBox(AFX_IDP_FAILED_TO_CREATE_DOC);
		return;
	}
	ASSERT_VALID(pDocument);
  // View creation
	CFrameWnd* pFrame = m_pdtDocTemplate->CreateNewFrame(pDocument, NULL);
	if (pFrame == NULL)
	{
		AfxMessageBox(AFX_IDP_FAILED_TO_CREATE_DOC);
		delete pDocument;       // explicit delete on error
		return;
	}
	ASSERT_VALID(pFrame);

  pDocument->SetModifiedFlag( FALSE);

  // add file to mru
  CTString strFileName = pmi->mi_fnSourceFile.FileName() + pmi->mi_fnSourceFile.FileExt();
  CString strOpenPath;
  strOpenPath = theApp.GetProfileString(L"SeriousSkaStudio", L"Open directory", L"");
  strOpenPath += pmi->mi_fnSourceFile;

  pDocument->SetPathName(CString(strFileName), TRUE);
  pDocument->SetTitle(CString(strFileName));

  pDocument->SetModifiedFlag( FALSE);
  m_pdtDocTemplate->InitialUpdateFrame(pFrame, pDocument, TRUE);
  // set root model instance
  theApp.GetDocument()->m_ModelInstance = pmi;
  // fill tree view with new model insntance
  ReloadRootModelInstance();
}

CSeriousSkaStudioView* CSeriousSkaStudioApp::GetActiveView(void)
{
  CSeriousSkaStudioView *res;
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  res = DYNAMIC_DOWNCAST(CSeriousSkaStudioView, pMainFrame->GetActiveFrame()->GetActiveView());
  return res;
}

CSeriousSkaStudioDoc *CSeriousSkaStudioApp::GetDocument()
{
  // obtain current view ptr
  CSeriousSkaStudioView *pSKAView = GetActiveView();
  // if view does not exist, return
  if( pSKAView == NULL)
  {
    return NULL;
  }
  // obtain document ptr
  CSeriousSkaStudioDoc *pDoc = pSKAView->GetDocument();
  // return it
  return pDoc;
}

// start pasring fnParseFile file (may include mesh,skeleton,animset,...)
BOOL StartParser(CTString fnParseFile)
{
  CTFileName fnFull;
  fnFull = _fnmApplicationPath + fnParseFile;

  yyin = NULL;
  astrText.PopAll();
  astrText.Clear();
  // initialize pre-parsing variables
  yyin = fopen(fnFull, "r");
  // reset include depth ptr
  include_stack_ptr = 0;
  strCurentFileName = fnFull;

  _yy_iIndex = 0;
  _yy_jIndex = 0;
  _yy_iLine = 1;

  // load data
  try
  {
    if (yyin==NULL) {
      ThrowF_t("Cannot open file '%s'!", (const char*)fnParseFile );
    }

    yyrestart(yyin);
    yyparse();
 
    fclose(yyin);
  }
  // if an error in parsing occured
  catch(char *strError)
  {
    WarningMessage(strError);
    // AfxMessageBox(strError);
    theApp.ErrorMessage(strError);
    if(yyin!=NULL) fclose(yyin);
    return FALSE;
  }
return TRUE;
}

// convert ascii mesh into binary 
BOOL CSeriousSkaStudioApp::ConvertMesh(CTFileName fnMesh)
{
  CMesh mesh;
  _yy_pMesh = &mesh;
  // parse fnMesh
  if(!StartParser(fnMesh))
  {
    // if failed clear mesh and return
    mesh.Clear();              
    return FALSE;
  }
  // count optimization results
  int ctmlods = mesh.msh_aMeshLODs.Count();
  int imlod=0;
  for(;imlod<ctmlods;imlod++)
  {
    ctMeshVxBeforeOpt += mesh.msh_aMeshLODs[imlod].mlod_aVertices.Count();
  }
  // optimize mesh
  mesh.Optimize();
  mesh.NormalizeWeights();
  // count optimization results
  for(imlod=0;imlod<ctmlods;imlod++)
  {
    ctMeshVxAfterOpt += mesh.msh_aMeshLODs[imlod].mlod_aVertices.Count();
  }
  // save binary mesh
  try
  {
    mesh.Save_t(fnMesh.NoExt() + ".bm");
  }
  catch(char *strErr)
  {
    ErrorMessage(strErr);
  }
  // clear from memory
  mesh.Clear();
  _yy_pMesh = NULL;
  
  // reload mesh in stock
  CMesh *pMesh;
  try
  {
    // load mesh
    pMesh = _pMeshStock->Obtain_t(fnMesh.NoExt() + ".bm");
    // reload it
    pMesh->Reload();
    // release mesh
    _pMeshStock->Release(pMesh);
  }
  catch(char *strError)
  {
    if(strError != NULL) ErrorMessage(strError);
    return FALSE;
  }
  return TRUE;
}

// convert ascii skeleton into binary 
BOOL CSeriousSkaStudioApp::ConvertSkeleton(CTFileName fnSkeleton)
{
  CSkeleton skeleton;
  _yy_pSkeleton = &skeleton;
  if(!StartParser(fnSkeleton))
  {
    //if failed clear skeleton and return
    skeleton.Clear();
    return FALSE;
  }
  // sort bones
  skeleton.SortSkeleton();
  try
  {
    // save binary skeleton
    skeleton.Save_t(fnSkeleton.NoExt() + ".bs");
  }
  catch(char *strErr)
  {
    ErrorMessage(strErr);
  }
  // clear skeleton
  skeleton.Clear();
  _yy_pSkeleton = NULL;

  // reload skeleton in stock
  CSkeleton *pSkeleton;
  try
  {
    // load skeleton
    pSkeleton = _pSkeletonStock->Obtain_t(fnSkeleton.NoExt() + ".bs");
    // reload skeleton
    pSkeleton->Reload();
    // release skeleton
    _pSkeletonStock->Release(pSkeleton);
  }
  catch(char *strError)
  {
    if(strError != NULL) ErrorMessage("%s",strError);
    return FALSE;
  }
  return TRUE;
}

// convert ascii anim set into binary 
BOOL CSeriousSkaStudioApp::ConvertAnimSet(CTFileName fnAnimSet)
{
  CAnimSet animset;
  _yy_pAnimSet = &animset;
  if(!StartParser(fnAnimSet))
  {
    //if failed clear animset and return
    animset.Clear();
    return FALSE;
  }
  animset.Optimize();
  try
  {
    // save animset as binary
    animset.Save_t(fnAnimSet.NoExt() + ".ba");
  }
  catch(char *strErr)
  {
    ErrorMessage(strErr);
  }
  // clear from memory
  animset.Clear();
  _yy_pAnimSet = NULL;

  // reload animset in stock
  CAnimSet *pAnimSet;
  try
  {
    // load animset
    pAnimSet = _pAnimSetStock->Obtain_t(fnAnimSet.NoExt() + ".ba");
    // reload animset
    pAnimSet->Reload();
    // release animset
    _pAnimSetStock->Release(pAnimSet);
  }
  catch(char *strError)
  {
    if(strError != NULL) ErrorMessage("%s",strError);
    return FALSE;
  }
  return TRUE;
}

// Save smc file
void CSeriousSkaStudioApp::SaveSmcFile(CModelInstance &mi,BOOL bSaveChildren)
{
	CSeriousSkaStudioDoc *pDoc = GetDocument();

  // first get first model instance that has its file
  CModelInstance *pmiParent=NULL;
  CModelInstance *pmiFirst=&mi;
  CTFileName fnSmc = mi.mi_fnSourceFile;
  CModelInstance *pfmi = mi.GetFirstNonReferencedParent(GetDocument()->m_ModelInstance);
  if(pfmi!=NULL) {
    pmiParent = pfmi->GetParent(pDoc->m_ModelInstance);
    pmiFirst = pfmi;
    fnSmc = pfmi->mi_fnSourceFile;
  }

  DisableRendering();
  try
  {
    fnSmc.RemoveApplicationPath_t();
  }
  catch(char *){}

  CTFileStream ostrFile;
  // try to save model instance
  try {
    ostrFile.Create_t(fnSmc,CTStream::CM_TEXT);
    SaveModelInstance_t(pmiFirst,pmiParent,ostrFile,bSaveChildren);
    ostrFile.Close();
    NotificationMessage("Smc '%s' saved.",pmiFirst->mi_fnSourceFile); 
  } catch(char *strError) {
    ErrorMessage(strError);
  }
  EnableRendering();
}

// save mesh list file
BOOL CSeriousSkaStudioApp::SaveMeshListFile(MeshInstance &mshi, BOOL bConvert)
{
  DisableRendering();
  // get mesh list filename
  CTFileName fnMeshList = mshi.mi_pMesh->GetName();
  fnMeshList = fnMeshList.NoExt() + ".aml";
  try {
    fnMeshList.RemoveApplicationPath_t();
  } catch(char *){}
  CTString strBackUp;
  try {
    // back up current mesh list file
    strBackUp.Load_t(fnMeshList);
  } catch(char*){}
  // save mesh instance in new mesh list file
  CTFileStream ostrFile;
  try {
    ostrFile.Create_t(fnMeshList,CTStream::CM_TEXT);
    SaveMeshInstance_t(mshi,ostrFile);
    ostrFile.Close();
  } catch(char *strError) {
    ErrorMessage(strError);
    EnableRendering();
    return FALSE;
  }

  // if new mesh list file needs to be converted
  if(bConvert) {
    if(!ConvertMesh(fnMeshList)) {
      // convert failed
      if(strBackUp.Length()>0) {
        // try returning old mesh list file
        try {
          strBackUp.Save_t(fnMeshList);
        } catch(char*){}
      }
    }
  }
  EnableRendering();
  return TRUE;
}

// save skeleton list file 
BOOL CSeriousSkaStudioApp::SaveSkeletonListFile(CSkeleton &skl, BOOL bConvert)
{
  DisableRendering();
  CTFileName fnSkeletonList = skl.GetName();
  fnSkeletonList = fnSkeletonList.NoExt() + ".asl";
  try {
    fnSkeletonList.RemoveApplicationPath_t();
  }
  catch(char *){}

  // back up current skeleton list file
  CTString strBackUp;
  try {
    strBackUp.Load_t(fnSkeletonList);
  }
  catch(char*){}

  CTFileStream ostrFile;
  try {
    ostrFile.Create_t(fnSkeletonList,CTStream::CM_TEXT);
    SaveSkeletonList_t(skl,ostrFile);
    ostrFile.Close();
  } catch(char *strError) {
    ErrorMessage(strError);
    EnableRendering();
    return FALSE;
  }

  if(bConvert) {
    if(!ConvertSkeleton(fnSkeletonList)) {
      // convert failed
      if(strBackUp.Length()>0) {
        // try returning old mesh list file
        try {
          strBackUp.Save_t(fnSkeletonList);
        }
        catch(char*){}
      }
    }
  }
  EnableRendering();
  return TRUE;
}

// save anim set file
BOOL CSeriousSkaStudioApp::SaveAnimSetFile(CAnimSet &as, BOOL bConvert)
{
  DisableRendering();
  CTFileName fnAnimSet = as.GetName();
  fnAnimSet = fnAnimSet.NoExt() + ".aal";
  try {
    fnAnimSet.RemoveApplicationPath_t();
  } catch(char *){}

  // back up current skeleton list file
  CTString strBackUp;
  try {
    strBackUp.Load_t(fnAnimSet); 
  } catch(char*){}

  CTFileStream ostrFile;
  try {
    ostrFile.Create_t(fnAnimSet,CTStream::CM_TEXT);
    SaveAnimSet_t(as,ostrFile);
    ostrFile.Close();
  } catch(char *strError) {
    ErrorMessage(strError);
    EnableRendering();
    return FALSE;
  }

  if(bConvert) {
    ConvertAnimSet(fnAnimSet);
  }

  EnableRendering();
  return TRUE;
}
// convert only one animation in animset 
BOOL CSeriousSkaStudioApp::ConvertAnimationInAnimSet(CAnimSet *pas,Animation *pan)
{
  DisableRendering();
  CTFileName fnTemp = (CTString)"Temp/animset";
  
  // try to save model instance
  CTString strAnimSet;
  CTString strCustomSpeed;
  CTString strCompresion = "FALSE";
  if(pan->an_bCompresed) strCompresion = "TRUE";
  if(pan->an_bCustomSpeed) strCustomSpeed.PrintF("  ANIMSPEED %g;",pan->an_fSecPerFrame);
  strAnimSet.PrintF("ANIMSETLIST\n{\n  TRESHOLD %g;\n  COMPRESION %s;\n%s\n  #INCLUDE \"%s\"\n}\n",
    pan->an_fTreshold,(const char*)strCompresion,(const char*)strCustomSpeed,(const char*)pan->an_fnSourceFile);

  try
  {
    strAnimSet.Save_t(fnTemp + ".aal");
    if(!ConvertAnimSet(fnTemp + ".aal")) {
      EnableRendering();
      return FALSE;
    }
    CAnimSet as;
    // load new animset
    as.Load_t(fnTemp + ".ba");
    if(as.as_Anims.Count()>0)
    {
      Animation &anNew = as.as_Anims[0];
      // overwrite old animation with new one
      *pan = anNew;
    }
    // clear new animset
    as.Clear();
  }
  catch(char *strErr)
  {
    ErrorMessage(strErr);
    EnableRendering();
    return FALSE;
  }
  EnableRendering();
  return TRUE;
}
// update root model instance
void CSeriousSkaStudioApp::UpdateRootModelInstance()
{
  CSeriousSkaStudioDoc *pDoc = GetDocument();
  if(pDoc!=NULL) {
    CModelInstance *pmi = pDoc->m_ModelInstance;
    if(pmi!=NULL) {
      m_dlgBarTreeView.UpdateModelInstInfo(pmi);
    }
  }
}
// reload root model instance for this document
void CSeriousSkaStudioApp::ReloadRootModelInstance()
{
  DisableRendering();
	CSeriousSkaStudioDoc *pDoc = GetDocument();
  CModelInstance *pmi=NULL;

  if(pDoc==NULL) pmi = NULL;
  else pmi = pDoc->m_ModelInstance;
  // flag for parser to remember source file names
  CModelInstance::EnableSrcRememberFN(TRUE);
  // if model instance is valid
  if(pmi != NULL)
  {
    // clear current model instance
    pDoc->m_ModelInstance->Clear();
    // try parsing smc file
    try {
      pDoc->m_ModelInstance = ParseSmcFile_t(pDoc->m_ModelInstance->mi_fnSourceFile);
    } catch(char *strError) {
      // error in parsing occured
      ErrorMessage("%s",strError);
      if(pDoc->m_ModelInstance != NULL) pDoc->m_ModelInstance->Clear();
      pDoc->m_ModelInstance = NULL;
    }
    UpdateRootModelInstance();
  }
  else
  {
    m_dlgBarTreeView.ResetControls();
    pmiSelected = NULL;
  }
//  NotificationMessage("Root model instance updated"); 
  EnableRendering();
}

void CSeriousSkaStudioApp::ReselectCurrentItem()
{
  HTREEITEM hSelected = m_dlgBarTreeView.m_TreeCtrl.GetSelectedItem();
  if(hSelected != NULL) {
    m_dlgBarTreeView.SelItemChanged(hSelected);
  }
}

void CSeriousSkaStudioApp::OnImportConvert() 
{
	CSeriousSkaStudioDoc *pDoc = GetDocument();
  CDynamicArray<CTFileName> afnCovert;
  _EngineGUI.FileRequester( "Open ASCII intermediate files",
    FILTER_ASCII FILTER_MESH_LIST FILTER_SKELETON_LIST FILTER_ANIMSET_LISTS FILTER_ALL,
    "Open directory", "Models\\", "", &afnCovert);
  INDEX ctMeshes = 0;
  INDEX ctSkeletons = 0;
  INDEX ctAnimSets = 0;
  CTimerValue tvStartOptimizer = _pTimer->GetHighPrecisionTimer();
  ctMeshVxBeforeOpt = 0;
  ctMeshVxAfterOpt = 0;


  FOREACHINDYNAMICARRAY( afnCovert, CTFileName, itConvert)
  {
    char fnCurent[256];
    strcpy(fnCurent,itConvert.Current());
    char *pchDot = strrchr(fnCurent, '.');
    if (pchDot==NULL) 
      continue;
    else if(strcmp((_strlwr(pchDot)),".aml")==0) 
    {
      if(ConvertMesh(itConvert.Current()))
      {
        ctMeshes++;
      }
    }
    else if(strcmp((_strlwr(pchDot)),".asl")==0)
    {
      if(ConvertSkeleton(itConvert.Current()))
        ctSkeletons++;
    }
    else if(strcmp((_strlwr(pchDot)),".aal")==0)
    {
      if(ConvertAnimSet(itConvert.Current()))
        ctAnimSets++;
    }
  }
  
  CTimerValue tvStop = _pTimer->GetHighPrecisionTimer();

  if(ctMeshes+ctSkeletons+ctAnimSets == 0) return;
  char strText[256];
  char strOptimizeText[128];
  sprintf(strText,"Convert results:\n\n  Meshes    \t%d\n  Skeletons \t%d\n  AnimSets  \t%d\n\nTime to convert: \t%d ms",ctMeshes,ctSkeletons,ctAnimSets,(int)((tvStop-tvStartOptimizer).GetSeconds()*1000));
  sprintf(strOptimizeText,"\n\nOptimization results:\n  Vertices before \t%d\n  Vertices after \t%d",ctMeshVxBeforeOpt,ctMeshVxAfterOpt);
  strcat(strText,strOptimizeText);
  AfxMessageBox(CString(strText));
  UpdateRootModelInstance();
}
// save model instance to smc file
static INDEX iCurSpaces=0;
void CSeriousSkaStudioApp::SaveModelInstance_t(CModelInstance *pmi,CModelInstance *pmiParent,CTFileStream &ostrFile,BOOL bSaveChildren)
{
  ASSERT(pmi!=NULL);

  FLOATmatrix3D mat;
  FLOAT3D vPos = pmi->mi_qvOffset.vPos;
  ANGLE3D aRot;
  BOOL bSaveOffSet = TRUE;
  // if model instance have parent
  if(pmiParent!=NULL) {
    // if source file names are same, save offsets in file
    bSaveOffSet = (pmi->mi_fnSourceFile == pmiParent->mi_fnSourceFile);
  }
  
  if(bSaveOffSet)
  {
    pmi->mi_qvOffset.qRot.ToMatrix(mat);
    DecomposeRotationMatrix(aRot,mat);
    // if offset exists
    if((vPos(1)) || (vPos(2)) || (vPos(3)) || (aRot(1)) || (aRot(2)) || (aRot(3))) {
      ostrFile.FPrintF_t(MAKESPACE(iCurSpaces));
      ostrFile.FPrintF_t("OFFSET    \t%g,%g,%g,%g,%g,%g;\n",vPos(1),vPos(2),vPos(3),aRot(1),aRot(2),aRot(3));
    }
  }
  
  ostrFile.FPrintF_t(MAKESPACE(iCurSpaces));
  ostrFile.FPrintF_t("NAME \"%s\";\n",(const char*)pmi->GetName());
  ostrFile.FPrintF_t(MAKESPACE(iCurSpaces));
  ostrFile.FPrintF_t("{\n");
  iCurSpaces+=2;
  INDEX ctmi=pmi->mi_aMeshInst.Count();
  // for each mesh instance
  for(INDEX imi=0;imi<ctmi;imi++) {
    MeshInstance &mshi = pmi->mi_aMeshInst[imi];
    ostrFile.FPrintF_t(MAKESPACE(iCurSpaces));
    CTString fnMesh = mshi.mi_pMesh->GetName();
    ostrFile.FPrintF_t("MESH       \tTFNM \"%s\";\n",(const char*)fnMesh);
    INDEX ctti=mshi.mi_tiTextures.Count();
    // write textures
    ostrFile.FPrintF_t(MAKESPACE(iCurSpaces));
    ostrFile.FPrintF_t("TEXTURES   \n");
    ostrFile.FPrintF_t(MAKESPACE(iCurSpaces));
    ostrFile.FPrintF_t("{\n");
    // for each texture instance
    for(INDEX iti=0;iti<ctti;iti++) {
      TextureInstance &ti = mshi.mi_tiTextures[iti];
      ostrFile.FPrintF_t(MAKESPACE(iCurSpaces+2));
      ostrFile.FPrintF_t("\"%s\"\tTFNM \"%s\";\n",(const char*)ska_GetStringFromTable(ti.GetID()),(const char*)ti.ti_toTexture.GetName());
    }
    ostrFile.FPrintF_t(MAKESPACE(iCurSpaces));
    ostrFile.FPrintF_t("}\n");
  }

  // write skeleton
  if(pmi->mi_psklSkeleton != NULL) {
    ostrFile.FPrintF_t(MAKESPACE(iCurSpaces));
    ostrFile.FPrintF_t("SKELETON  \tTFNM \"%s\";\n",(const char*)pmi->mi_psklSkeleton->GetName());
  }
  INDEX ctas=pmi->mi_aAnimSet.Count();
  // write animset
  for(INDEX ias=0;ias<ctas;ias++)
  {
    ostrFile.FPrintF_t(MAKESPACE(iCurSpaces));
    ostrFile.FPrintF_t("ANIMSET   \tTFNM \"%s\";\n",(const char*)pmi->mi_aAnimSet[ias].GetName());
  }

  // write all frames bouning box
  ostrFile.FPrintF_t(MAKESPACE(iCurSpaces));
  FLOAT3D vMin = pmi->mi_cbAllFramesBBox.Min();
  FLOAT3D vMax = pmi->mi_cbAllFramesBBox.Max();
  ostrFile.FPrintF_t("ALLFRAMESBBOX\t%g,%g,%g,%g,%g,%g;\n",vMin(1),vMin(2),vMin(3),vMax(1),vMax(2),vMax(3));

  // write colision boxes
  ostrFile.FPrintF_t(MAKESPACE(iCurSpaces));
  ostrFile.FPrintF_t("COLISION\n");
  ostrFile.FPrintF_t(MAKESPACE(iCurSpaces));
  ostrFile.FPrintF_t("{\n");
  // write each colison box
  INDEX ctcb = pmi->mi_cbAABox.Count();
  for(INDEX icb=0;icb<ctcb;icb++)
  {
    ostrFile.FPrintF_t(MAKESPACE(iCurSpaces+2));
    ColisionBox &cb = pmi->mi_cbAABox[icb];
    ostrFile.FPrintF_t("\"%s\"  {%g,%g,%g,%g,%g,%g;}\n",cb.GetName(),
      cb.Min()(1),cb.Min()(2),cb.Min()(3),
      cb.Max()(1),cb.Max()(2),cb.Max()(3));
  }
  ostrFile.FPrintF_t(MAKESPACE(iCurSpaces));
  ostrFile.FPrintF_t("}\n");
  
  // write model color
  // ostrFile.FPrintF_t(MAKESPACE(iCurSpaces));
  // ostrFile.FPrintF_t("COLOR\t\t0x%X;\n",pmi->GetModelColor());

  // write children
  INDEX ctmic = pmi->mi_cmiChildren.Count();
  for(INDEX imic=0;imic<ctmic;imic++)
  {
    CModelInstance *pcmi = &pmi->mi_cmiChildren[imic];

    FLOATmatrix3D mat;
    FLOAT3D vPos = pcmi->mi_qvOffset.vPos;
    ANGLE3D aRot;
    pcmi->mi_qvOffset.qRot.ToMatrix(mat);
    DecomposeRotationMatrix(aRot,mat);

    ostrFile.FPrintF_t("\n");
    ostrFile.FPrintF_t(MAKESPACE(iCurSpaces));
    ostrFile.FPrintF_t("PARENTBONE\t\"%s\";\n",(const char*)ska_GetStringFromTable(pcmi->mi_iParentBoneID));

    // attachment was in same file
    if(pcmi->mi_fnSourceFile == pmi->mi_fnSourceFile)
    {
      // continue writing in this file
      SaveModelInstance_t(pcmi,pmi,ostrFile,bSaveChildren);
    }
    else // attachment has its own file
    {
      // write child offset
      if((vPos(1)) || (vPos(2)) || (vPos(3)) || (aRot(1)) || (aRot(2)) || (aRot(3)))
      {
        ostrFile.FPrintF_t(MAKESPACE(iCurSpaces));
        ostrFile.FPrintF_t("OFFSET    \t%g,%g,%g,%g,%g,%g;\n",vPos(1),vPos(2),vPos(3),aRot(1),aRot(2),aRot(3));
      }

      CTFileName fnCmiSourceFile = pcmi->mi_fnSourceFile;
      try{
        fnCmiSourceFile.RemoveApplicationPath_t();
      } catch(char*) {}
      ostrFile.FPrintF_t(MAKESPACE(iCurSpaces));
      // include that file
      ostrFile.FPrintF_t("#INCLUDE\t\"%s\"\n",(const char*)fnCmiSourceFile);
      if(bSaveChildren) 
      {
        INDEX itmpSpaces = iCurSpaces;
        iCurSpaces = 0;
        // save child model instance
        SaveSmcFile(*pcmi,bSaveChildren);
        iCurSpaces = itmpSpaces;
      }
    }
  }
  iCurSpaces-=2;
  ostrFile.FPrintF_t(MAKESPACE(iCurSpaces));
  ostrFile.FPrintF_t("}\n");
}
// save shader params for all surfaces in one mesh lod
void CSeriousSkaStudioApp::SaveShaderParams_t(MeshLOD *pmlod,CTFileName fnShaderParams)
{
  CTFileStream ostrFile;
  ostrFile.Create_t(fnShaderParams,CTStream::CM_TEXT);
  INDEX ctsrf=pmlod->mlod_aSurfaces.Count();
  ostrFile.FPrintF_t("SHADER_PARAMS 1.0;\nSHADER_SURFACES %d\n{\n",ctsrf);
  for(INDEX isrf=0;isrf<ctsrf;isrf++)
  {
    MeshSurface &msrf = pmlod->mlod_aSurfaces[isrf];
    ShaderParams *pShdParams = &msrf.msrf_ShadingParams;
    CTString strShaderName;
    if(msrf.msrf_pShader!=NULL) strShaderName = msrf.msrf_pShader->GetName();
    CTString strSurfName = ska_GetStringFromTable(msrf.msrf_iSurfaceID);
    
    ostrFile.FPrintF_t("  SHADER_SURFACE \"%s\";\n  {\n",(const char*)strSurfName);
    ostrFile.FPrintF_t("    SHADER_NAME \"%s\";\n",(const char*)strShaderName);
    // write texture names
    INDEX cttx = pShdParams->sp_aiTextureIDs.Count();
    ostrFile.FPrintF_t("    SHADER_TEXTURES %d\n    {\n",cttx);
    for(INDEX itx=0;itx<cttx;itx++)
    {
      CTString strTextID = ska_GetStringFromTable(pShdParams->sp_aiTextureIDs[itx]);
      ostrFile.FPrintF_t("      \"%s\";\n",(const char*)strTextID);
    }
    ostrFile.FPrintF_t("    };\n");
    // write uvmaps
    INDEX cttxc = pShdParams->sp_aiTexCoordsIndex.Count();
    ostrFile.FPrintF_t("    SHADER_UVMAPS %d\n    {\n",cttxc);
    for(INDEX itxc=0;itxc<cttxc;itxc++)
    {
      ostrFile.FPrintF_t("      %d;\n",pShdParams->sp_aiTexCoordsIndex[itxc]);
    }
    ostrFile.FPrintF_t("    };\n");
    // write colors
    INDEX ctcol=pShdParams->sp_acolColors.Count();
    ostrFile.FPrintF_t("    SHADER_COLORS %d\n    {\n",ctcol);
    for(INDEX icol=0;icol<ctcol;icol++)
    {
      CTString strColor = CTString(0,"%.8X",pShdParams->sp_acolColors[icol]);
      ostrFile.FPrintF_t("      0x%s;\n",(const char*)strColor);
    }
    ostrFile.FPrintF_t("    };\n");
    //write floats
    INDEX ctfl=pShdParams->sp_afFloats.Count();
    ostrFile.FPrintF_t("    SHADER_FLOATS %d\n    {\n",ctfl);
    for(INDEX ifl=0;ifl<ctfl;ifl++)
    {
      ostrFile.FPrintF_t("      %g;\n",pShdParams->sp_afFloats[ifl]);
    }
    ostrFile.FPrintF_t("    };\n");
    // write flags
    CTString strFlags = CTString(0,"%.8X",pShdParams->sp_ulFlags);
    ostrFile.FPrintF_t("    SHADER_FLAGS 0x%s;\n",(const char*)strFlags);

    // close surface 
    ostrFile.FPrintF_t("  };\n");
  }
  ostrFile.FPrintF_t("};\nSHADER_PARAMS_END\n");
  ostrFile.Close();
}
// write mesh lod list to stream
void CSeriousSkaStudioApp::SaveMeshInstance_t(MeshInstance &mshi,CTFileStream &ostrFile)
{
  CMesh &msh = *mshi.mi_pMesh;
  ostrFile.FPrintF_t("MESHLODLIST\n{\n");

  INDEX ctmlod = msh.msh_aMeshLODs.Count();
  for(INDEX imlod=0;imlod<ctmlod;imlod++)
  {
    MeshLOD &mlod = msh.msh_aMeshLODs[imlod];
    ostrFile.FPrintF_t("  MAX_DISTANCE %g;\n",mlod.mlod_fMaxDistance);
    // try to remove app path from source
    CTFileName fnSource = mlod.mlod_fnSourceFile;
    CTFileName fnShaderParams = fnSource.FileDir()+fnSource.FileName()+".shp";
    try { fnSource.RemoveApplicationPath_t(); }
    catch(char *){}
    ostrFile.FPrintF_t("  #INCLUDE \"%s\"\n",(const char*)fnShaderParams);
    ostrFile.FPrintF_t("  #INCLUDE \"%s\"\n",(const char*)fnSource);
    SaveShaderParams_t(&mlod,fnShaderParams);
  }
  ostrFile.FPrintF_t("}\n");
}
// write anim set list to stream
void CSeriousSkaStudioApp::SaveAnimSet_t(CAnimSet &as,CTFileStream &ostrFile)
{
  ostrFile.FPrintF_t("ANIMSETLIST\n{\n");

  INDEX ctan = as.as_Anims.Count();
  for(INDEX ian=0;ian<ctan;ian++)
  {
    Animation &an = as.as_Anims[ian];
    // try to remove app path from source
    CTFileName fnSource = an.an_fnSourceFile;
    try { fnSource.RemoveApplicationPath_t(); }
    catch(char *){}
    ostrFile.FPrintF_t("  TRESHOLD %g;\n",an.an_fTreshold);
    if(an.an_bCompresed) ostrFile.FPrintF_t("  COMPRESION TRUE;\n");
    else ostrFile.FPrintF_t("  COMPRESION FALSE;\n");
    if(an.an_bCustomSpeed) ostrFile.FPrintF_t("  ANIMSPEED %g;\n",an.an_fSecPerFrame);
    ostrFile.FPrintF_t("  #INCLUDE \"%s\"\n",(const char*)fnSource);
  }
  ostrFile.FPrintF_t("}\n");
}
// write skeleton lod list to stream
void CSeriousSkaStudioApp::SaveSkeletonList_t(CSkeleton &skl,CTFileStream &ostrFile)
{
  ostrFile.FPrintF_t("SKELETONLODLIST\n{\n");

  INDEX ctslod = skl.skl_aSkeletonLODs.Count();
  for(INDEX islod=0;islod<ctslod;islod++)
  {
    SkeletonLOD &slod = skl.skl_aSkeletonLODs[islod];
    ostrFile.FPrintF_t("  MAX_DISTANCE %g;\n",slod.slod_fMaxDistance);
    // try to remove app path from source
    CTFileName fnSource = slod.slod_fnSourceFile;
    try { fnSource.RemoveApplicationPath_t(); }
    catch(char *){}
    ostrFile.FPrintF_t("  #INCLUDE \"%s\"\n",(const char*)fnSource);
  }
  ostrFile.FPrintF_t("}\n");
}

void CSeriousSkaStudioApp::AddEmptyListsToModelInstance(CModelInstance &mi)
{
  // remove app path from model instance
  CTFileName fnSmcSource = mi.mi_fnSourceFile;
  try {
    fnSmcSource.RemoveApplicationPath_t();
  } catch(char *) {
  }

  CTFileName fnMeshList = fnSmcSource.NoExt() + ".aml";
  CTFileName fnSkeletonList = fnSmcSource.NoExt() + ".asl";
  CTFileName fnAnimSet = fnSmcSource.NoExt() + ".aal";

  // if mesh list with that name does not exists
  if(!FileExists(fnMeshList)) {
    // create new empty one
    CTFileStream ostrFile;
    try {
      // create new file
      ostrFile.Create_t(fnMeshList,CTStream::CM_TEXT);
      // write empty header in file
      ostrFile.FPrintF_t("MESHLODLIST\n{\n}\n");
      // close file
      ostrFile.Close();
      // convert it to binary (just to exist)
      if(theApp.ConvertMesh(fnMeshList)) {
        fnMeshList = fnMeshList.NoExt() + ".bm";
        // add it to selected model instance
          mi.AddMesh_t(fnMeshList);
      }
    } catch(char *strError) {
      ErrorMessage("%s",strError);
    }
  }
  // if skeleton list with that name does not exists
  if(!FileExists(fnSkeletonList)) {
    // create new empty one
    CTFileStream ostrFile;
    try {
      // create new file
      ostrFile.Create_t(fnSkeletonList,CTStream::CM_TEXT);
      // write empty header in file
      ostrFile.FPrintF_t("SKELETONLODLIST\n{\n}\n");
      // close file
      ostrFile.Close();
      // convert it to binary (just to exist)
      if(theApp.ConvertSkeleton(fnSkeletonList)) {
        fnSkeletonList = fnSkeletonList.NoExt() + ".bs";
        // add it to selected model instance
          mi.AddSkeleton_t(fnSkeletonList);
      }
    } catch(char *strError) {
      ErrorMessage("%s",strError);
    }
  }
  // if animset with that name does not exists
  if(!FileExists(fnAnimSet)) {
    // create new empty one
    CTFileStream ostrFile;
    try {
      // create new file
      ostrFile.Create_t(fnAnimSet,CTStream::CM_TEXT);
      // write empty header in file
      ostrFile.FPrintF_t("ANIMSETLIST\n{\n}\n");
      // close file
      ostrFile.Close();
      // convert it to binary (just to exist)
      if(theApp.ConvertAnimSet(fnAnimSet)) {
        fnAnimSet = fnAnimSet.NoExt() + ".ba";
        // add it to selected model instance
          mi.AddAnimSet_t(fnAnimSet);
      }
    } catch(char *strError) {
      ErrorMessage("%s",strError);
    }
  }
}

// add new root model instance to an empty project 
CModelInstance *CSeriousSkaStudioApp::OnAddNewModelInstance() 
{
  CModelInstance *pmi=NULL;
  CSeriousSkaStudioDoc *pDoc = GetDocument();
  CTFileName fnSim;
  // get file name  
  fnSim = _EngineGUI.FileRequester( "Type name for new Smc file or select existing one",
    "ASCII model files (*.smc)\0*.smc\0"
    "All files (*.*)\0*.*\0\0",
    "Open directory", "Models\\", "");
  if (fnSim=="") return NULL;
  CTFileName fnFull;
  fnFull = _fnmApplicationPath + fnSim;
  CModelInstance::EnableSrcRememberFN(TRUE);

  // check if file allready exist
  if(FileExists(fnSim))
  {
    CTString strText = CTString(0,"'%s' already exists.\nDo you want to overwrite it?",fnSim);
    int iRet = AfxMessageBox(CString(strText),MB_YESNO);
    if(iRet == IDNO) {
      return NULL;
    }
  }
  // if file does not exist create new one
  CTFileStream ostrFile;
  try
  {
    // create new file
    ostrFile.Create_t(fnSim,CTStream::CM_TEXT);
    // write empty header in file
    ostrFile.FPrintF_t("NAME \"%s\";\n{\n}\n",(const char*)fnSim.FileName());
    // close file
    ostrFile.Close();
    // load new smc file
    pmi = ParseSmcFile_t(fnFull);
  }
  catch(char *strError)
  {
    ErrorMessage("%s",strError);
    if(pmi != NULL) pmi->Clear();
    return NULL;
  }

  // Add empty lists in they files do not exists on disk

  AddEmptyListsToModelInstance(*pmi);
  return pmi;
}
// load existing smc file
CModelInstance *CSeriousSkaStudioApp::OnOpenExistingInstance(CTString strModelInstance)
{
  CModelInstance *pmi=NULL;

  CModelInstance::EnableSrcRememberFN(TRUE);
  // check if file exist
  if(FileExists(strModelInstance))
  {
    // start parsing smc file
    try
    {
      pmi = ParseSmcFile_t(_fnmApplicationPath + strModelInstance);
    }
    catch(char *strError)
    {
      // error in parsing occured
      ErrorMessage("%s",strError);
      if(pmi != NULL) pmi->Clear();
      return NULL;
    }
  }
  else
  {
    ErrorMessage("File '%s' does not exist",strModelInstance);
    return NULL;
  }
  return pmi;
}

// get pointer to error listctrl
CListCtrl *CSeriousSkaStudioApp::GetErrorList()
{
  return (CListCtrl*)m_dlgErrorList.GetDlgItem(IDC_LC_ERROR_LIST);
}

void CSeriousSkaStudioApp::ShowErrorDlg(BOOL bShow)
{
  // return;
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  pMainFrame->ShowControlBar(&m_dlgErrorList, bShow, FALSE);
  // m_wndSpliterLogFrame.
  // m_wndSpliterLogFrame.SetWindowPos(0,0,0,100,5,SWP_NOZORDER);
}

BOOL CSeriousSkaStudioApp::IsErrorDlgVisible()
{
  return m_dlgErrorList.IsWindowVisible();
}

// returns size of log window
SIZE CSeriousSkaStudioApp::GetLogDlgSize()
{
  SIZE size;
  CRect rc;

  m_dlgErrorList.GetClientRect(rc);
  size.cx = rc.right;
  size.cy = rc.bottom;
  return size;
}

void CSeriousSkaStudioApp::ErrorMessage(const char *strFormat, ...)
{
  // format the message in buffer
  va_list arg;
  va_start(arg, strFormat);
  CTString strBuffer;
  strBuffer.VPrintF(strFormat, arg);

  CTString strText;
  CTime tm = CTime::GetCurrentTime();
  // set time and error
  strText.PrintF("%.2d:%.2d:%.2d %s",tm.GetHour(),tm.GetMinute(),tm.GetSecond(),(const char*)strBuffer);
  if(bAppInitialized) {
    CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
    pMainFrame->m_wndStatusBar.SetPaneText(0,CString(strText),TRUE);
    // add message in log list
    GetErrorList()->InsertItem(0,CString(strText),14);
    GetErrorList()->EnsureVisible(0,FALSE);
    // show log list
    ShowErrorDlg(TRUE);
  } else {
    FatalError((const char*)strText);
  }

}
void CSeriousSkaStudioApp::NotificationMessage(const char *strFormat, ...)
{
  // format the message in buffer
  va_list arg;
  va_start(arg, strFormat);
  CTString strBuffer;
  strBuffer.VPrintF(strFormat, arg);

  CTString strText;
  CTime tm = CTime::GetCurrentTime();
  // set time and error
  strText.PrintF("%.2d:%.2d:%.2d %s",tm.GetHour(),tm.GetMinute(),tm.GetSecond(),(const char*)strBuffer);
  if(bAppInitialized) {
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  pMainFrame->m_wndStatusBar.SetPaneText(0,CString(strText),TRUE);
  // add message in log list
  GetErrorList()->InsertItem(0,CString(strText),15);
  GetErrorList()->EnsureVisible(0,FALSE);
  } else {
    WarningMessage((const char*)strText);
  }
}

int CSeriousSkaStudioApp::ExitInstance() 
{
  SE_EndEngine();
	return CWinApp::ExitInstance();
}

// save root model instance with children and all binary files
void CSeriousSkaStudioApp::SaveRootModel()
{
  CSeriousSkaStudioDoc *pDoc = GetDocument();
  SaveModel(*pDoc->m_ModelInstance);
  pDoc->m_bModelInstanceChanged = FALSE;
}
// save as different smc file
void CSeriousSkaStudioApp::SaveRootModelAs()
{
  CSeriousSkaStudioDoc *pDoc = GetDocument();
  CModelInstance *pmi = pDoc->m_ModelInstance;
  if(pmi != NULL) {
    SaveModelAs(pmi);
    // update tile name
    pDoc->SetTitle(CString(pmi->mi_fnSourceFile.FileName() + pmi->mi_fnSourceFile.FileExt()));
  }
}

BOOL CSeriousSkaStudioApp::SaveModelAs(CModelInstance *pmi)
{
  ASSERT(pmi!=NULL);
  if(pmi==NULL) return FALSE;
  CModelInstance *pmiParent = NULL;
  CString fnOldSmcFile;

  // get smc file name  
  CTFileName fnSim;
  fnSim = _EngineGUI.FileRequester( "Select existing Smc file",
    "ASCII model files (*.smc)\0*.smc\0"
    "All files (*.*)\0*.*\0\0",
    "Open directory", "Models\\", "",NULL,FALSE);
  if (fnSim=="") return FALSE;

  CTFileName fnFull;
  fnFull = _fnmApplicationPath + fnSim;
  pmi->mi_fnSourceFile = fnSim;
  // save model instance
  SaveModel(*pmi);

  return TRUE;
}

// save model instance with children and all binary files
void CSeriousSkaStudioApp::SaveModel(CModelInstance &mi)
{
  BOOL bSaved = TRUE;
  // save smc file 
  SaveSmcFile(mi,FALSE);
  // count mesh instances
  INDEX ctmshi = mi.mi_aMeshInst.Count();
  // for each mesh instance
  for(INDEX imshi=0;imshi<ctmshi;imshi++) {
    MeshInstance &mshi = mi.mi_aMeshInst[imshi];
    CMesh *pMesh = mshi.mi_pMesh;
    // try to save mesh
    try {
      pMesh->Save_t(pMesh->ser_FileName);
    } catch(char *strErr) {
      ErrorMessage(strErr);
      bSaved = FALSE;
    }
    // save mesh instance as ascii file (do not convert to binary)
    SaveMeshListFile(mshi,FALSE);
  }
  CSkeleton *psklSkeleton = mi.mi_psklSkeleton;
  if(psklSkeleton!=NULL) {
    // try to save skeleton
    try {
      psklSkeleton->Save_t(psklSkeleton->ser_FileName);
    } catch(char *strErr) {
      ErrorMessage(strErr);
      bSaved = FALSE;
    }
    // save skeleton as ascii file (do not convert to binary)
    SaveSkeletonListFile(*psklSkeleton,FALSE);
  }
  // count animsets
  INDEX ctas = mi.mi_aAnimSet.Count();
  // for each animset 
  for(INDEX ias=0;ias<ctas;ias++)
  {
    CAnimSet *pas = &mi.mi_aAnimSet[ias];
    // try to save animset
    try {
      pas->Save_t(pas->ser_FileName);
    } catch(char *strErr) {
      ErrorMessage(strErr);
      bSaved = FALSE;
    }
    // save animset as ascii file (do not convert to binary)
    SaveAnimSetFile(*pas,FALSE);
  }
  // count children
  INDEX ctcmi = mi.mi_cmiChildren.Count();
  // for each child model instance
  for(INDEX icmi=0;icmi<ctcmi;icmi++) {
    CModelInstance &cmi = mi.mi_cmiChildren[icmi];
    // save child model
    SaveModel(cmi);
  }
}

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// App command to run the dialog
void CSeriousSkaStudioApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}
