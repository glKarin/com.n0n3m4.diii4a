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

// Modeler.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include <Engine/Templates/Stock_CModelData.h>
#include <Engine/Templates/Stock_CTextureData.h>

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// Macros used for ini i/o operations
#define INI_READ( strname, def)                               \
  wcscpy( strIni, theApp.GetProfileString( L"Modeler prefs", CString(strname), CString(def)))
#define GET_FLAG( var)                                        \
  if( wcscmp( strIni, L"YES") == 0)   var = TRUE;              \
  else                          var = FALSE;
#define GET_COLOR( var)                                       \
  swscanf( strIni, L"0X%08x", &var);
#define GET_INDEX( var)                                       \
  swscanf( strIni, L"%d", &var);
#define GET_FLOAT( var)                                       \
  swscanf( strIni, L"%f", &var);

#define SET_FLAG( var)                                        \
  if( var) wcscpy( strIni, L"YES");                            \
  else     wcscpy( strIni, L"NO");
#define SET_COLOR( var)                                       \
  swprintf( strIni, L"0x%08x", var);                            \
  _wcsupr( strIni);
#define SET_INDEX( var)                                       \
  swprintf( strIni, L"%d", var);                                \
  _wcsupr( strIni);
#define SET_FLOAT( var)                                       \
  swprintf( strIni, L"%f", var);                                \
  _wcsupr( strIni);
#define INI_WRITE( strname)                                   \
  theApp.WriteProfileString( L"Modeler prefs", CString(strname), strIni)

BOOL GetFlagFromProfile( CTString strVarName, BOOL bDefault)
{
  CTString strDefault;
  if( bDefault) strDefault = "YES";
  else          strDefault = "NO";
  CTString strTemp = CStringA(theApp.GetProfileString( L"Modeler prefs", CString(strVarName), CString(strDefault)));
  if( strTemp == "YES") return TRUE;
  return FALSE;
};

void SetFlagToProfile( CTString strVarName, BOOL bValue)
{
  if( bValue) theApp.WriteProfileString( L"Modeler prefs", CString(strVarName), L"YES");
  else        theApp.WriteProfileString( L"Modeler prefs", CString(strVarName), L"NO");
};

INDEX GetIndexFromProfile( CTString strVarName, INDEX iDefault)
{
  CTString strDefault;
  strDefault.PrintF("%d", iDefault);
  CTString strTemp = CStringA(theApp.GetProfileString( L"Modeler prefs", CString(strVarName), CString(strDefault)));
  INDEX iValue;
  sscanf( strTemp, "%d", &iValue);
  return iValue;
};

void SetIndexToProfile( CTString strVarName, INDEX iValue)
{
  CTString strTemp;
  strTemp.PrintF("%d", iValue);
  theApp.WriteProfileString( L"Modeler prefs", CString(strVarName), CString(strTemp));
};

COLOR GetColorFromProfile( CTString strVarName, COLOR colDefault)
{
  CTString strDefault;
  strDefault.PrintF("0x%08x", colDefault);
  CTString strTemp = CStringA(theApp.GetProfileString( L"Modeler prefs", CString(strVarName), CString(strDefault)));
  COLOR colValue;
  sscanf( strTemp, "0x%08x", &colValue);
  return colValue;
};

void SetColorToProfile( CTString strVarName, COLOR colValue)
{
  CTString strTemp;
  strTemp.PrintF("0x%08x", colValue);
  theApp.WriteProfileString( L"Modeler prefs", CString(strVarName), CString(strTemp));
};

/////////////////////////////////////////////////////////////////////////////
// CModelerApp

BEGIN_MESSAGE_MAP(CModelerApp, CWinApp)
	//{{AFX_MSG_MAP(CModelerApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	ON_COMMAND(ID_FILE_NEW, OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
	ON_COMMAND(ID_FILE_PREFERENCES, OnFilePreferences)
	//}}AFX_MSG_MAP
	// Standard file based document commands
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////

UINT APIENTRY ModelerFileRequesterHook( HWND hdlg, UINT uiMsg, WPARAM wParam,	LPARAM lParam)
{
  if( uiMsg == WM_INITDIALOG)
  {
    CRect rectMainFrame, rectFileReqPos, rectFileReqNewPos;
    CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
    LONG left, top;
    
    GetWindowRect( GetParent( hdlg), rectFileReqPos);
    pMainFrame->GetWindowRect( rectMainFrame);
    
    left = (rectMainFrame.right - rectMainFrame.left)/2
                             - rectFileReqPos.Width()/2;
    top = (rectMainFrame.bottom - rectMainFrame.top)/2
                             - rectFileReqPos.Height()/2;
    MoveWindow( GetParent( hdlg), left, top,
                rectFileReqPos.Width(), rectFileReqPos.Height(), TRUE);
  }
  return 0;
}

/////////////////////////////////////////////////////////////////////////////
// CModelerApp construction

CModelerApp::CModelerApp()
{
  m_bRefreshPatchPalette = FALSE;
  m_OnIdlePaused = FALSE;
  m_pLampModelData = NULL;
  m_pFloorModelData = NULL;
  m_pCollisionBoxModelData = NULL;
  m_ptdCollisionBoxTexture = NULL;
  m_ptdLamp = NULL;
  m_ptdFloorTexture = NULL;
  m_pfntFont = NULL;
}

CBcgTexture::CBcgTexture()
{
  wt_TextureData = NULL;
}

CBcgTexture::~CBcgTexture()
{
}

CModelerApp::~CModelerApp()
{
  if( m_pLampModelData != NULL)
  {
    _pModelStock->Release( m_pLampModelData);
    delete m_LampModelObject;
  }
  
  if( m_pCollisionBoxModelData != NULL)
  {
    _pModelStock->Release( m_pCollisionBoxModelData);
    delete m_pCollisionBoxModelObject;
    m_pCollisionBoxModelObject = NULL;
  }

  if( m_pFloorModelData != NULL)
  {
    _pModelStock->Release( m_pFloorModelData);
    delete m_pFloorModelObject;
    m_pFloorModelObject = NULL;
  }
  
  if( m_ptdCollisionBoxTexture != NULL)
  {
    _pTextureStock->Release( m_ptdCollisionBoxTexture);
    m_ptdCollisionBoxTexture = NULL;
  }

  if( m_ptdLamp != NULL)
  {
    _pTextureStock->Release( m_ptdLamp);
    m_ptdLamp = NULL;
  }
  
  if( m_ptdFloorTexture != NULL)
  {
    _pTextureStock->Release( m_ptdFloorTexture);
    m_ptdFloorTexture = NULL;
  }
  

  FORDELETELIST( CBcgTexture, wt_ListNode, m_WorkingTextures, litTex)
  {
    ASSERT( litTex->wt_TextureData != NULL);
    _pTextureStock->Release( litTex->wt_TextureData);
    delete &litTex.Current();
  }

  FORDELETELIST( CWorkingPatch, wp_ListNode, m_WorkingPatches, litPatch)
  {
    CTextureData *pTD = litPatch->wp_TextureData;
    _pTextureStock->Release( litPatch->wp_TextureData);
    delete &litPatch.Current();
  }

  SE_EndEngine();
}


/////////////////////////////////////////////////////////////////////////////
// The one and only CModelerApp object

CModelerApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CModelerApp initialization

BOOL CModelerApp::InitInstance()
{
  BOOL bResult;
  CTSTREAM_BEGIN {
    bResult = SubInitInstance();
  } CTSTREAM_END;
  return bResult;
}

BOOL CModelerApp::SubInitInstance()
{
  wchar_t strIni[ 128];
	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

  // settings will be saved into registry instead of ini file
  SetRegistryKey( L"CroTeam");

	LoadStdProfileSettings(8);  // Load standard INI file options (including MRU)

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views.

	CMultiDocTemplate* pDocTemplate;
	m_pdtModelDocTemplate = pDocTemplate = new CMultiDocTemplate(
		IDR_MDLDOCTYPE,
		RUNTIME_CLASS(CModelerDoc),
		RUNTIME_CLASS(CChildFrame), // custom MDI child frame
		RUNTIME_CLASS(CModelerView));
	AddDocTemplate(pDocTemplate);
  
  m_pdtScriptTemplate = pDocTemplate = new CMultiDocTemplate(
		IDR_SCRIPTDOCTYPE,
		RUNTIME_CLASS(CScriptDoc),
		RUNTIME_CLASS(CChildFrame), // custom MDI child frame
		RUNTIME_CLASS(CScriptView));
	AddDocTemplate(pDocTemplate);

  // initialize engine, without network
  SE_InitEngine("");  // DO NOT SPECIFY NAME HERE!
  SE_LoadDefaultFonts();

	// create main MDI Frame window
	CMainFrame* pMainFrame = new CMainFrame;
	if( !pMainFrame->LoadFrame(IDR_MAINFRAME)) return FALSE;
  m_pMainWnd = pMainFrame;
  
  // set main window for engine
  SE_UpdateWindowHandle( m_pMainWnd->m_hWnd);

	// Enable drag/drop open
	m_pMainWnd->DragAcceptFiles();

	// Enable DDE Execute open
	EnableShellOpen();
	RegisterShellFileTypes(TRUE);

	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

  // load startup script
  _pShell->Execute( "include \"Scripts\\Modeler_startup.ini\"");
  
  m_iApi=GAT_OGL;
  m_iApi=GetProfileInt(L"Display modes", L"SED Gfx API", GAT_OGL);
  // (re)set default display mode
  _pGfx->ResetDisplayMode((enum GfxAPIType) m_iApi);

  m_Preferences.ReadFromIniFile();

  // load background textures
  INDEX iWorkingTexturesCt = theApp.GetProfileInt( L"Modeler prefs", 
                                                   L"Modeler working textures count", -1);
  if( iWorkingTexturesCt != -1) {
    char strWTName[ 128];
    for( INDEX i=0; i<iWorkingTexturesCt; i++) {
      sprintf( strWTName, "Working texture %02d", i);
      INI_READ( strWTName, "Error in INI .file!");
      AddModelerWorkingTexture( CTString(CStringA(strIni)));
    }
  }
  // load working patches
  INDEX iWorkingPatchesCt = theApp.GetProfileInt( L"Modeler prefs", 
                                                 L"Modeler working patches count", -1);
  char strWPName[ 128];
  for( INDEX i=0; i<iWorkingPatchesCt; i++) {
    sprintf( strWPName, "Working patch %02d", i);
    INI_READ( strWPName, "Error in INI .file!");
    AddModelerWorkingPatch( CTString(CStringA(strIni)));
  }
  pMainFrame->m_StainsComboBox.Refresh();

  // don't start new document automatically
  cmdInfo.m_nShellCommand = CCommandLineInfo::FileNothing;
  
  // create temporary directory to contain copy/paste mapping all
  CreateDirectoryA( _fnmApplicationPath + "Temp\\", NULL);
  
  // try to
  try
  { // load lamp model
    DECLARE_CTFILENAME( fnLampName, "Models\\Editor\\Lamp.mdl");
    m_pLampModelData = _pModelStock->Obtain_t( fnLampName);
    m_LampModelObject = new CModelObject;
    m_LampModelObject->SetData(m_pLampModelData);
    m_LampModelObject->SetAnim( 0);
    // load lamp's texture
    DECLARE_CTFILENAME( fnLampTex, "Models\\Editor\\SpotLight.tex");
    m_ptdLamp = _pTextureStock->Obtain_t( fnLampTex);
    m_LampModelObject->mo_toTexture.SetData( m_ptdLamp); 

    // load collision box model
    DECLARE_CTFILENAME( fnCollisionBox, "Models\\Editor\\CollisionBox.mdl");
    m_pCollisionBoxModelData = _pModelStock->Obtain_t( fnCollisionBox);
    m_pCollisionBoxModelObject = new CModelObject;
    m_pCollisionBoxModelObject->SetData(m_pCollisionBoxModelData);
    m_pCollisionBoxModelObject->SetAnim( 0);
    // load collision box's texture
    DECLARE_CTFILENAME( fnCollisionBoxTex, "Models\\Editor\\CollisionBox.tex");
    m_ptdCollisionBoxTexture = _pTextureStock->Obtain_t( fnCollisionBoxTex);
    m_pCollisionBoxModelObject->mo_toTexture.SetData( m_ptdCollisionBoxTexture); 

    // load floor model
    DECLARE_CTFILENAME( fnFloor, "Models\\Editor\\Floor.mdl");
    m_pFloorModelData = _pModelStock->Obtain_t( fnFloor);
    m_pFloorModelObject = new CModelObject;
    m_pFloorModelObject->SetData(m_pFloorModelData);
    m_pFloorModelObject->SetAnim( 0);
    // load collision box's texture
    DECLARE_CTFILENAME( fnFloorTex, "Models\\Editor\\Floor.tex");
    m_ptdFloorTexture = _pTextureStock->Obtain_t( fnFloorTex);
    m_pFloorModelObject->mo_toTexture.SetData( m_ptdFloorTexture); 

    DECLARE_CTFILENAME( fnShadowTex, "Textures\\Effects\\Shadow\\SimpleModelShadow.tex");
    // setup simple model shadow texture
    _toSimpleModelShadow.SetData_t( fnShadowTex);
  }
  catch( char *err_str)
  { // report error and continue without models
    AfxMessageBox( CString(err_str));

    // if we allocated model object for collision box
    if( m_pCollisionBoxModelObject != NULL) {
      // delete it
      delete m_pCollisionBoxModelObject;
      m_pCollisionBoxModelObject = NULL;
    }
    // if we loaded collision box's texture
    if( m_ptdCollisionBoxTexture != NULL) {
      // release it and
      _pTextureStock->Release( m_ptdCollisionBoxTexture);
      m_ptdCollisionBoxTexture = NULL;
    }

    // if we loaded lamp's texture
    if( m_ptdLamp != NULL) {
      // release it and
      _pTextureStock->Release( m_ptdLamp);
      m_ptdLamp = NULL;
    }

    // if we allocated model object for floor
    if( m_pFloorModelObject != NULL) {
      // delete it
      delete m_pFloorModelObject;
      m_pFloorModelObject = NULL;
    }
    // if we loaded floor's texture
    if( m_ptdFloorTexture != NULL) {
      // release it and
      _pTextureStock->Release( m_ptdFloorTexture);
      m_ptdFloorTexture = NULL;
    }
  }

  // assign system font
  m_pfntFont = _pfdDisplayFont;

  // Dispatch commands specified on the command line
	if( !ProcessShellCommand(cmdInfo)) return FALSE;

	// The main window has been initialized, so show and update it.
  m_nCmdShow = SW_SHOWMAXIMIZED; // maximize main frame !!!
	pMainFrame->ShowWindow(m_nCmdShow);
	pMainFrame->UpdateWindow();

  // if stating modeler for the first time
  if( m_bFirstTimeStarted) {
    // call preferences
    OnFilePreferences();
  }
  
  return TRUE;
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
		// No message handlers
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
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// App command to run the dialog
void CModelerApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// CModelerApp commands

static TIME timeLastTick=TIME(0);
BOOL CModelerApp::OnIdle(LONG lCount) 
{
  if( _pTimer != NULL)
  {
    TIME timeCurrentTick = _pTimer->GetRealTimeTick();
    if( (timeCurrentTick > timeLastTick) && !m_OnIdlePaused)
    {
      _pTimer->SetCurrentTick( timeCurrentTick);
      timeLastTick = timeCurrentTick;
      POSITION pos = m_pdtModelDocTemplate->GetFirstDocPosition();

      while (pos!=NULL) {
        CModelerDoc *pmdCurrent = (CModelerDoc *)m_pdtModelDocTemplate->GetNextDoc(pos);
        pmdCurrent->OnIdle();
      }
  
      ((CMainFrame *)m_pMainWnd)->OnIdle( lCount);
    }
  }

  // if application is active
  extern BOOL _bApplicationActive;
  if (_bApplicationActive) {
    // never release idle
    return CWinApp::OnIdle(lCount) || TRUE;
  // if application is inactive
  } else {
    // release idle when not needed
    return CWinApp::OnIdle(lCount);
  }
}

/////////////////////////////////////////////////////////////////////////////
void CModelerApp::CreateNewDocument( CTFileName fnRequestedFile)
{
  CTFileName fnMdlFile    = fnRequestedFile.FileDir() + fnRequestedFile.FileName() + ".mdl";
  CTFileName fnScriptFile = fnRequestedFile.FileDir() + fnRequestedFile.FileName() + ".scr";
  
  if( fnRequestedFile.FileExt() != ".scr")
  {
    if( GetFileAttributesA( _fnmApplicationPath + fnScriptFile) != -1)
    {
      if( MessageBoxA( m_pMainWnd->m_hWnd, "Script file allready exists, "
                                          "do you want to overwrite it?",
                 "Warning !", MB_YESNO | MB_ICONWARNING |
                 MB_DEFBUTTON1| MB_SYSTEMMODAL | MB_TOPMOST) == IDYES)
      {
        DeleteFileA( _fnmApplicationPath + fnScriptFile);
      }
      else
      {
        return;
      }
    }
    CEditModel em;
    
    try
    {
      em.CreateScriptFile_t( fnRequestedFile);
    }
    catch( char *err_str)
    {
      AfxMessageBox( CString(err_str));
      return;
    }
  }
  else if( GetFileAttributesA( _fnmApplicationPath + fnMdlFile) != -1)
  {
    if( MessageBoxA( m_pMainWnd->m_hWnd, "Model file allready exists, do you want to "
                    "overwrite it and loose all possible data describing mapping, "
                    "colored polygons, patch positions, ... ?",
                    "Warning !", MB_YESNO | MB_ICONWARNING | 
                    MB_DEFBUTTON1| MB_SYSTEMMODAL | MB_TOPMOST) != IDYES)
      return;
  }

  // Now we create document instance of type CModelerDoc
  CDocument* pDocument = m_pdtModelDocTemplate->CreateNewDocument();
 	if (pDocument == NULL)
	{
		TRACE0("CDocTemplate::CreateNewDocument returned NULL.\n");
		AfxMessageBox(AFX_IDP_FAILED_TO_CREATE_DOC);
		return;
	}
	ASSERT_VALID(pDocument);
	
  BOOL bAutoDelete = pDocument->m_bAutoDelete;
	pDocument->m_bAutoDelete = FALSE;   // don't destroy if something goes wrong
	CFrameWnd* pFrame = m_pdtModelDocTemplate->CreateNewFrame(pDocument, NULL);
	pDocument->m_bAutoDelete = bAutoDelete;
	if (pFrame == NULL)
	{
		AfxMessageBox(AFX_IDP_FAILED_TO_CREATE_DOC);
		delete pDocument;       // explicit delete on error
		return;
	}
	ASSERT_VALID(pFrame);

  pDocument->SetModifiedFlag();
  pDocument->SetPathName( CString(_fnmApplicationPath + fnMdlFile), FALSE);
  pDocument->SetTitle( CString(fnMdlFile.FileName() + ".mdl"));       

  char strError[ 256];
  if( !((CModelerDoc *)pDocument)->CreateModelFromScriptFile( fnScriptFile, strError))
  {
  	pDocument->OnCloseDocument();       // explicit delete on error
    AfxMessageBox( CString(strError));
		return;
  }
	m_pdtModelDocTemplate->InitialUpdateFrame(pFrame, pDocument, TRUE);
}

void CModelerApp::OnFileNew()
{
  // call file requester for opening documents
  CDynamicArray<CTFileName> afnCreateModel;
  _EngineGUI.FileRequester( "Create new model from 3D or script file",
    "3D objects and scripts\0*.lwo;*.obj;*.3ds;*.scr\0"
    FILTER_3DOBJ FILTER_LWO FILTER_OBJ FILTER_3DS FILTER_SCR FILTER_ALL FILTER_END,
    "Create model directory", "Models\\", "", &afnCreateModel);
  // create new models
  FOREACHINDYNAMICARRAY( afnCreateModel, CTFileName, itModel)
  {
    // create new models
    CreateNewDocument( itModel.Current());
  }
}
/////////////////////////////////////////////////////////////////////////////
void CModelerApp::OnFileOpen() 
{
  // call file requester for opening documents
  CDynamicArray<CTFileName> afnOpenModel;
  _EngineGUI.FileRequester( "Open model or script file",
    "Model files (*.mdl)\0*.mdl\0"
    "Script files (*.scr)\0*.scr\0"
    "Model or script files (*.mdl; *.scr)\0*.mdl;*.scr\0"
    "All files (*.*)\0*.*\0\0",
    "Open model directory", "Models\\", "", &afnOpenModel);

  // create new models
  FOREACHINDYNAMICARRAY( afnOpenModel, CTFileName, itModel)
  {
    // we will use full file name to call OnOpenDocument()
    CTFileName fnFullRequestedFile = _fnmApplicationPath + itModel.Current();
    // choose right document template using file's extension
    CDocTemplate *pDocTemplate = m_pdtModelDocTemplate;
    BOOL bScriptDocument = FALSE;
    if( fnFullRequestedFile.FileExt() == ".scr")
    {
      pDocTemplate = m_pdtScriptTemplate;
      bScriptDocument = TRUE;
    }

    // Now we create document instance 
    CDocument* pDocument = pDocTemplate->CreateNewDocument();
 	  if (pDocument == NULL)
	  {
		  TRACE0("CDocTemplate::CreateNewDocument returned NULL.\n");
		  AfxMessageBox(AFX_IDP_FAILED_TO_CREATE_DOC);
		  return;
	  }
	  ASSERT_VALID(pDocument);
	  
    // Model documents must be opened before view creation
    if( !bScriptDocument)
    {
      if( !pDocument->OnOpenDocument( CString(fnFullRequestedFile)))
      {
		    AfxMessageBox(AFX_IDP_FAILED_TO_CREATE_DOC);
		    //delete pDocument;       // explicit delete on error
		    return;
      }
    }
  
    // View creation
    BOOL bAutoDelete = pDocument->m_bAutoDelete;
	  pDocument->m_bAutoDelete = FALSE;   // don't destroy if something goes wrong
	  CFrameWnd* pFrame = pDocTemplate->CreateNewFrame(pDocument, NULL);
	  pDocument->m_bAutoDelete = bAutoDelete;
	  if (pFrame == NULL)
	  {
		  AfxMessageBox(AFX_IDP_FAILED_TO_CREATE_DOC);
		  delete pDocument;       // explicit delete on error
		  return;
	  }
	  ASSERT_VALID(pFrame);

    // Script documents must be opened after view creation
    if( bScriptDocument)
    {
      if( !pDocument->OnOpenDocument( CString(fnFullRequestedFile)))
      {
		    AfxMessageBox(AFX_IDP_FAILED_TO_CREATE_DOC);
		    delete pDocument;       // explicit delete on error
		    return;
      }
    }

    pDocument->SetModifiedFlag( FALSE);
    pDocument->SetPathName( CString(fnFullRequestedFile), TRUE);
    pDocument->SetTitle( CString(fnFullRequestedFile.FileName() + fnFullRequestedFile.FileExt()));
	  pDocTemplate->InitialUpdateFrame(pFrame, pDocument, TRUE);
  }
}

void CModelerApp::OnFilePreferences() 
{
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  CDlgPreferences dlg;
  if( dlg.DoModal() == IDOK)
  { 
    m_Preferences = dlg.m_Prefs;
    // change wiew's background texture
    CModelerView *pModelerView = CModelerView::GetActiveView();
    if( pModelerView != NULL) {
      pModelerView->m_fnBcgTexture = dlg.m_Prefs.ap_DefaultWinBcgTexture;
    }
    pModelerView = CModelerView::GetActiveMappingNormalView();
    if( pModelerView != NULL) {
      CModelerDoc *pDoc = pModelerView->GetDocument();
      pDoc->UpdateAllViews( NULL);
    }
  }
}

int CModelerApp::ExitInstance() 
{
  m_Preferences.WriteToIniFile();
  WriteProfileInt(L"Display modes", L"SED Gfx API", m_iApi);
	return CWinApp::ExitInstance();
}

BOOL CModelerApp::AddModelerWorkingTexture( CTFileName fnTexName)
{
  CBcgTexture *pNewWT = new CBcgTexture;

  pNewWT->wt_FileName = fnTexName;
  try
  {
    pNewWT->wt_TextureData = _pTextureStock->Obtain_t( fnTexName);
  }
  catch( char *err_str)
  {
    MessageBoxA( m_pMainWnd->m_hWnd, err_str, "Warning!", MB_OK|MB_ICONHAND|MB_SYSTEMMODAL);
    if( pNewWT != NULL) delete pNewWT;
    return FALSE;
  }
  pNewWT->wt_toTexture.SetData( pNewWT->wt_TextureData);

  m_WorkingTextures.AddTail( pNewWT->wt_ListNode);
  return TRUE;
}

BOOL CModelerApp::AddModelerWorkingPatch( CTFileName fnPatchName)
{
  FOREACHINLIST( CWorkingPatch, wp_ListNode, m_WorkingPatches, itPatch)
  {
    if( itPatch->wp_FileName == fnPatchName)
    {
      char achrMessage[ 256];
      sprintf( achrMessage, "Working patch \"%s\" already exists.", (CTString&)fnPatchName);
      AfxMessageBox( CString(achrMessage));
      return FALSE;
    }
  }

  CWorkingPatch *pNewWP = new CWorkingPatch;
  pNewWP->wp_FileName = fnPatchName;
  try
  {
    pNewWP->wp_TextureData = _pTextureStock->Obtain_t( pNewWP->wp_FileName);
  }
  catch( char *err_str)
  {
    MessageBoxA( m_pMainWnd->m_hWnd, err_str, "Warning!", MB_OK|MB_ICONHAND|MB_SYSTEMMODAL);
    delete pNewWP;
    return FALSE;
  }
  m_WorkingPatches.AddTail( pNewWP->wp_ListNode);
  return TRUE;
}

const CTextureObject *CModelerApp::GetValidBcgTexture( CTFileName fnTexName)
{
	const CTextureObject *ptoResult = NULL;
  FOREACHINLIST( CBcgTexture, wt_ListNode, m_WorkingTextures, it_wt)
  {
    if( it_wt->wt_FileName == fnTexName)
    {
      return &it_wt->wt_toTexture;
    }
  }
  if( !m_WorkingTextures.IsEmpty())
  {
    ptoResult = &(LIST_HEAD( m_WorkingTextures, CBcgTexture, wt_ListNode)->wt_toTexture);
  }
  return ptoResult;
}

const CTFileName CModelerApp::NextPrevBcgTexture( CTFileName fnTexName, INDEX iNextPrev)
{
  INDEX ctTextures = m_WorkingTextures.Count();
  ASSERT( ctTextures > 1);

  CStaticArray<CTFileName> afnTemp;
  afnTemp.New( ctTextures);
  
  INDEX iCurrent = -1;
  INDEX iIter = 0;
  // add textures to static array and remember current texture's index by name
  FOREACHINLIST( CBcgTexture, wt_ListNode, m_WorkingTextures, it_wt)
  {
    afnTemp[iIter] = it_wt->wt_FileName;
    if( it_wt->wt_FileName == fnTexName)
    {
      iCurrent = iIter;
    }
    iIter++;
  }
  // texture must be found
  ASSERT( iCurrent != -1);
  // return 
  return( afnTemp[(iCurrent+ctTextures+iNextPrev) % ctTextures]);
}

/////////////////////////////////////////////////////////////////////////////
// CAppPrefs routines

CAppPrefs::~CAppPrefs()
{
}

// Modeler ini read function for preferences
void CAppPrefs::ReadFromIniFile()
{
  wchar_t strIni[ 128];
  
  INI_READ( "Copy existing window preferences", "NO");
  GET_FLAG( ap_CopyExistingWindowPrefs);
  INI_READ( "Floor is visible by default", "YES");
  GET_FLAG( ap_bIsFloorVisibleByDefault);
  INI_READ( "Bcg is visible by default", "YES");
  GET_FLAG( ap_bIsBcgVisibleByDefault);
  INI_READ( "Set default colors", "YES");
  GET_FLAG( ap_SetDefaultColors);
  INI_READ( "Auto maximize window", "YES");
  GET_FLAG( ap_AutoMaximizeWindow);
  INI_READ( "Auto window fit", "YES");
  GET_FLAG( ap_AutoWindowFit);
  INI_READ( "Allways see lamp", "NO");
  GET_FLAG( ap_AllwaysSeeLamp);
  INI_READ( "Allow sound lock", "NO");
  GET_FLAG( ap_bAllowSoundLock);
  
  INI_READ( "Default ambient color", "0X3F3F3FFF");
  GET_COLOR( ap_colDefaultAmbientColor);

  INI_READ( "Default model heading", "180.0");
  GET_FLOAT( ap_fDefaultHeading);
  INI_READ( "Default model pitch", "0.0");
  GET_FLOAT( ap_fDefaultHeading);
  INI_READ( "Default model banking", "0.0");
  GET_FLOAT( ap_fDefaultBanking);
  INI_READ( "Default FOW", "90.0");
  GET_FLOAT( ap_fDefaultFOW);

  INI_READ( "Normal view ink color", "0X00000000");
  GET_COLOR( ap_DefaultInkColor);
  INI_READ( "Normal view paper color", "0XAAAAAAAA");
  GET_COLOR( ap_DefaultPaperColor);
  INI_READ( "Mapping view active surface color", "0X00000000");
  GET_COLOR( ap_MappingActiveSurfaceColor);
  INI_READ( "Mapping view inactive surface color", "0X80808000");
  GET_COLOR( ap_MappingInactiveSurfaceColor);
  INI_READ( "Mapping view paper color", "0XFFFFFFFF");
  GET_COLOR( ap_MappingPaperColor);
  INI_READ( "Mapping view win bcg color", "0XAAAAAAAA");
  GET_COLOR( ap_MappingWinBcgColor);

  INI_READ( "Default background texture", "");
  ap_DefaultWinBcgTexture = CTString(CStringA(strIni));
  
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
}

// Modeler ini write function for preferences
void CAppPrefs::WriteToIniFile()
{
  wchar_t strIni[ 128];
  
  SET_FLAG( ap_CopyExistingWindowPrefs);
  INI_WRITE( "Copy existing window preferences");
  SET_FLAG( ap_bIsFloorVisibleByDefault);
  INI_WRITE( "Floor is visible by default");
  SET_FLAG( ap_bIsBcgVisibleByDefault);
  INI_WRITE( "Bcg is visible by default");
  SET_FLAG( ap_SetDefaultColors);
  INI_WRITE( "Set default colors");
  SET_FLAG( ap_AutoMaximizeWindow);
  INI_WRITE( "Auto maximize window");
  SET_FLAG( ap_AutoWindowFit);
  INI_WRITE( "Auto window fit");
  SET_FLAG( ap_AllwaysSeeLamp);
  INI_WRITE( "Allways see lamp");
  SET_FLAG( ap_bAllowSoundLock);
  INI_WRITE( "Allow sound lock");

  SET_COLOR( ap_colDefaultAmbientColor);
  INI_WRITE( "Default ambient color");

  SET_FLOAT( ap_fDefaultHeading);
  INI_WRITE( "Default model heading");
  SET_FLOAT( ap_fDefaultHeading);
  INI_WRITE( "Default model pitch");
  SET_FLOAT( ap_fDefaultBanking);
  INI_WRITE( "Default model banking");
  SET_FLOAT( ap_fDefaultFOW);
  INI_WRITE( "Default FOW");

  SET_COLOR( ap_DefaultInkColor);
  INI_WRITE( "Normal view ink color");
  SET_COLOR( ap_DefaultPaperColor);
  INI_WRITE( "Normal view paper color");
  SET_COLOR( ap_MappingActiveSurfaceColor);
  INI_WRITE( "Mapping view active surface color");
  SET_COLOR( ap_MappingInactiveSurfaceColor);
  INI_WRITE( "Mapping view inactive surface color");
  SET_COLOR( ap_MappingPaperColor);
  INI_WRITE( "Mapping view paper color");
  SET_COLOR( ap_MappingWinBcgColor);
  INI_WRITE( "Mapping view win bcg color");

  wcscpy( strIni, CString(ap_DefaultWinBcgTexture));
  INI_WRITE( "Default background texture");
  
  // Now for working textures
  INDEX iWorkingTexturesCt = theApp.m_WorkingTextures.Count();
  theApp.WriteProfileInt( L"Modeler prefs", L"Modeler working textures count",
                         iWorkingTexturesCt);
  INDEX iWTCt = 0;
  FOREACHINLIST( CBcgTexture, wt_ListNode, theApp.m_WorkingTextures, it_wt)
  {
    char strWTName[ 128];
    sprintf( strWTName, "Working texture %02d", iWTCt);
    theApp.WriteProfileString( L"Modeler prefs", CString(strWTName), CString(it_wt->wt_FileName));
    iWTCt++;
  }
  
  // And now for patches....
  INDEX iWorkingPatchesCt = theApp.m_WorkingPatches.Count();
  theApp.WriteProfileInt( L"Modeler prefs", L"Modeler working patches count",
                         iWorkingPatchesCt);
  INDEX iWPCt = 0;
  FOREACHINLIST( CWorkingPatch, wp_ListNode, theApp.m_WorkingPatches, it_wp)
  {
    char strWPName[ 128];
    sprintf( strWPName, "Working patch %02d", iWPCt);
    theApp.WriteProfileString( L"Modeler prefs", CString(strWPName), CString(it_wp->wp_FileName));
    iWPCt++;
  }
}

int CModelerApp::Run() 
{
  int iResult;
  CTSTREAM_BEGIN {
    iResult=CWinApp::Run();
  } CTSTREAM_END;
	return CWinApp::Run();
}

CModelerView* CModelerApp::GetActiveView(void)
{
  CModelerView *res;
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  res = DYNAMIC_DOWNCAST(CModelerView, pMainFrame->GetActiveFrame()->GetActiveView());
  return res;
}

CModelerDoc *CModelerApp::GetDocument()
{
  // obtain current view ptr
  CModelerView *pModelerView = GetActiveView();
  // if view does not exist, return
  if( pModelerView == NULL)
  {
    return NULL;
  }
  // obtain document ptr
  CModelerDoc *pDoc = pModelerView->GetDocument();
  // return it
  return pDoc;
}

