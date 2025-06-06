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

// WorldEditor.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "WorldEditor.h"
#include "DlgTipOfTheDay.h"
#include <Engine/Templates/Stock_CTextureData.h>
#include <Engine/Templates/Stock_CModelData.h>

#include <sys/stat.h>
#include <sys/utime.h>
#include <process.h>


#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

UINT _uiMessengerMsg=-1;
UINT _uiMessengerForcePopup=-1;

extern FLOAT _fFlyModeSpeedMultiplier = 1.0f;
FLOAT _fLastMipBrushingOptionUsed = -10000.0f;
extern INDEX wed_iMaxFPSActive = 500;
extern FLOAT wed_fFrontClipDistance = 0.5f;
extern struct GameGUI_interface *_pGameGUI = NULL;
extern INDEX wed_bUseGenericTextureReplacement = FALSE;

CTFileName fnmPersistentSymbols = CTString("Scripts\\PersistentSymbols.ini");

// Macros used for ini i/o operations
#define INI_PRIMITIVE_READ( strname, default_val)                               \
  strcpy( strIni, CStringA(theApp.GetProfileString( L"World editor prefs", CString(strPrimitiveType+" "+ strname ), CString( default_val ))))
#define INI_PRIMITIVE_WRITE( strname)                                   \
  theApp.WriteProfileString( L"World editor prefs", CString(strPrimitiveType+" "+strname), CString(strIni))

#define INI_READ( strname, default_val)                               \
  strcpy( strIni, CStringA(theApp.GetProfileString( L"World editor prefs", CString( strname ), CString(default_val))))
#define GET_FLAG( var)                                        \
  if( strcmp( strIni, "YES") == 0)   var = TRUE;              \
  else                          var = FALSE;
#define GET_COLOR( var)                                       \
  sscanf( strIni, "0X%08x", &var);
#define GET_INDEX( var)                                       \
  sscanf( strIni, "%d", &var);
#define GET_FLOAT( var)                                       \
  sscanf( strIni, "%f", &var);
#define GET_STRING( var)                                      \
  var = CTString( strIni);

#define SET_FLAG( var)                                        \
  if( var) strcpy( strIni, "YES");                            \
  else     strcpy( strIni, "NO");
#define SET_COLOR( var)                                       \
  sprintf( strIni, "0x%08x", var);                            \
  _strupr( strIni);
#define SET_INDEX( var)                                       \
  sprintf( strIni, "%d", var);                                \
  _strupr( strIni);
#define SET_FLOAT( var)                                       \
  sprintf( strIni, "%f", var);                                \
  _strupr( strIni);
#define SET_STRING( var)                                      \
  sprintf( strIni, "%s", var);

#define INI_WRITE( strname)                                   \
  theApp.WriteProfileString( L"World editor prefs", CString( strname ), CString(strIni))

void InitializeGame(void)
{
  try {
    #ifndef NDEBUG 
      #define GAMEDLL _fnmApplicationExe.FileDir()+"GameGUI"+_strModExt+"D.dll"
    #else
      #define GAMEDLL _fnmApplicationExe.FileDir()+"GameGUI"+_strModExt+".dll"
    #endif
    CTFileName fnmExpanded;
    ExpandFilePath(EFP_READ, CTString(GAMEDLL), fnmExpanded);

    HMODULE hGame = LoadLibraryA(fnmExpanded);
    if (hGame==NULL) {
      ThrowF_t("%s", GetWindowsError(GetLastError()));
    }
    GameGUI_interface* (*GAMEGUI_Create)(void) = (GameGUI_interface* (*)(void))GetProcAddress(hGame, "GAMEGUI_Create");
    if (GAMEGUI_Create==NULL) {
      ThrowF_t("%s", GetWindowsError(GetLastError()));
    }
    _pGameGUI = GAMEGUI_Create();

  } catch (const char *strError) {
    FatalError("%s", strError);
  }
  _pGameGUI->Initialize(CTString("Data\\WorldEditor.gms"));
}

/////////////////////////////////////////////////////////////////////////////
// CWorldEditorApp

BEGIN_MESSAGE_MAP(CWorldEditorApp, CWinApp)
	//{{AFX_MSG_MAP(CWorldEditorApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
	ON_COMMAND(ID_FILE_PREFERENCES, OnFilePreferences)
	ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
	ON_COMMAND(ID_IMPORT_3D_OBJECT, OnImport3DObject)
	ON_COMMAND(ID_DECADIC_GRID, OnDecadicGrid)
	ON_UPDATE_COMMAND_UI(ID_DECADIC_GRID, OnUpdateDecadicGrid)
	ON_COMMAND(ID_CONVERT_WORLDS, OnConvertWorlds)
	ON_COMMAND(ID_SET_AS_DEFAULT, OnSetAsDefault)
	ON_COMMAND(ID_HELP_SHOWTIPOFTHEDAY, OnHelpShowTipOfTheDay)
	//}}AFX_MSG_MAP
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWorldEditorApp construction

void SetRenderingPrefs( CViewPrefs vp)
{
  // store current shadows on/off state
  CWorldRenderPrefs::ShadowsType sht=_wrpWorldRenderPrefs.GetShadowsType();
  _wrpWorldRenderPrefs = vp.m_wrpWorldRenderPrefs;
  _mrpModelRenderPrefs = vp.m_mrpModelRenderPrefs;

  _wrpWorldRenderPrefs.SetSelectedEntityModel( theApp.m_pEntityMarkerModelObject);
  _wrpWorldRenderPrefs.SetSelectedPortalModel( theApp.m_pPortalMarkerModelObject);
  _wrpWorldRenderPrefs.SetEmptyBrushModel( theApp.m_pEmptyBrushModelObject);

  // restore current shadows on/off state
  _wrpWorldRenderPrefs.SetShadowsType( sht);
}

void WED_ApplyChildSettings(INDEX iChildCfg)
{
  CChildConfiguration &cc = theApp.m_ccChildConfigurations[ iChildCfg];
  // find perspective view
  for( INDEX iView=0; iView<cc.m_iHorizontalSplitters*cc.m_iHorizontalSplitters; iView++)
  {
    if( cc.m_ptProjectionType[ iView] == CSlaveViewer::PT_PERSPECTIVE)
    {
      SetRenderingPrefs( cc.m_vpViewPrefs[ iView]);
    }
  }
}

void WED_ApplyChildSettings0(void) { WED_ApplyChildSettings(0);}
void WED_ApplyChildSettings1(void) { WED_ApplyChildSettings(1);}
void WED_ApplyChildSettings2(void) { WED_ApplyChildSettings(2);}
void WED_ApplyChildSettings3(void) { WED_ApplyChildSettings(3);}
void WED_ApplyChildSettings4(void) { WED_ApplyChildSettings(4);}
void WED_ApplyChildSettings5(void) { WED_ApplyChildSettings(5);}
void WED_ApplyChildSettings6(void) { WED_ApplyChildSettings(6);}
void WED_ApplyChildSettings7(void) { WED_ApplyChildSettings(7);}
void WED_ApplyChildSettings8(void) { WED_ApplyChildSettings(8);}
void WED_ApplyChildSettings9(void) { WED_ApplyChildSettings(9);}

void WED_ApplyRenderingSettings0(void) { SetRenderingPrefs(theApp.m_vpViewPrefs[0]);}
void WED_ApplyRenderingSettings1(void) { SetRenderingPrefs(theApp.m_vpViewPrefs[1]);}
void WED_ApplyRenderingSettings2(void) { SetRenderingPrefs(theApp.m_vpViewPrefs[2]);}
void WED_ApplyRenderingSettings3(void) { SetRenderingPrefs(theApp.m_vpViewPrefs[3]);}
void WED_ApplyRenderingSettings4(void) { SetRenderingPrefs(theApp.m_vpViewPrefs[4]);}
void WED_ApplyRenderingSettings5(void) { SetRenderingPrefs(theApp.m_vpViewPrefs[5]);}
void WED_ApplyRenderingSettings6(void) { SetRenderingPrefs(theApp.m_vpViewPrefs[6]);}
void WED_ApplyRenderingSettings7(void) { SetRenderingPrefs(theApp.m_vpViewPrefs[7]);}
void WED_ApplyRenderingSettings8(void) { SetRenderingPrefs(theApp.m_vpViewPrefs[8]);}
void WED_ApplyRenderingSettings9(void) { SetRenderingPrefs(theApp.m_vpViewPrefs[9]);}

void FindEmptyBrushes( void);
void WED_FindEmptyBrushes(void) { FindEmptyBrushes(); }

// Create primitive default values
CValuesForPrimitive::CValuesForPrimitive()
{
  vfp_csgtCSGOperation = CSG_ILLEGAL;
  vfp_ptPrimitiveType = PT_CONUS;
  vfp_avVerticesOnBaseOfPrimitive.Clear();
  vfp_avVerticesOnBaseOfPrimitive.New(4);
  vfp_avVerticesOnBaseOfPrimitive[0] = DOUBLE3D( -8.0f, 0.0f, -8.0f);
  vfp_avVerticesOnBaseOfPrimitive[1] = DOUBLE3D(  8.0f, 0.0f, -8.0f);
  vfp_avVerticesOnBaseOfPrimitive[2] = DOUBLE3D(  8.0f, 0.0f,  8.0f);
  vfp_avVerticesOnBaseOfPrimitive[3] = DOUBLE3D( -8.0f, 0.0f,  8.0f);
  vfp_bClosed       = TRUE;
  vfp_ttTriangularisationType = TT_NONE;
  vfp_bDummy = FALSE;
  vfp_bAutoCreateMipBrushes = FALSE;
  vfp_fXMin         = -8.0f;
  vfp_fXMax         =  8.0f;
  vfp_fYMin         = -8.0f;
  vfp_fYMax         =  8.0f;
  vfp_fZMin         = -8.0f;
  vfp_fZMax         = 8.0f;
  vfp_fShearX       = 0.0f;
  vfp_fShearZ       = 0.0f;
  vfp_fStretchX     = 1.0f;
  vfp_fStretchY     = 1.0f;
  vfp_plPrimitive.pl_PositionVector = FLOAT3D( 0.0f, 0.0f, 0.0f);
  vfp_plPrimitive.pl_OrientationAngle = ANGLE3D(0,0,0);
  vfp_colSectorsColor  = C_BLUE;
  vfp_colPolygonsColor = C_RED;
  vfp_fRadius       = 32.0f;
  vfp_bLinearStaircases = FALSE;
  vfp_bOuter = TRUE;
  vfp_iSlicesIn360 = 12;
  vfp_iNoOfSlices = 6;
  vfp_iMeridians  = 6;
  vfp_iParalels   = 6;
  vfp_iSlicesPerWidth = 6;
  vfp_iSlicesPerHeight= 6;
  vfp_iTopShape = 0;
  vfp_iBottomShape = 0;
  vfp_fAmplitude = 50.0f;
  vfp_fMipStart = 6.0f;
  vfp_fMipStep = 1.5f;
}

CWorldEditorApp::CWorldEditorApp()
{
  // register message
  _uiMessengerMsg = RegisterWindowMessageA("Croteam Messenger: Incoming Message");
  _uiMessengerForcePopup = RegisterWindowMessageA("Croteam Messenger: Force Popup Message");

  m_fnClassForDropMarker = CTFILENAME("Classes\\Marker.ecl");
  m_bChangeDisplayModeInProgress = FALSE;
  m_bDisableDataExchange = FALSE;
  m_pLastActivatedDocument = NULL;
//#ifdef NDEBUG
  m_bShowStatusInfo = TRUE;
//#else
//  m_bShowStatusInfo = FALSE;
//#endif
  m_bDocumentChangeOn = FALSE;
  m_bCSGReportEnabled = TRUE;
  m_bRememberUndo = TRUE;
  m_bMeasureModeOn = FALSE;
  m_bCutModeOn = FALSE;
  m_pfntSystem = NULL;
  m_ptdError = NULL;
  m_ptoError = NULL;
  m_ptdIconsTray = NULL;
  m_ptdActiveTexture = NULL;
  
  m_vLastTerrainHit=FLOAT3D(0,0,0);
  m_penLastTerrainHit=NULL;
  m_fCurrentTerrainBrush=4.0f;
  m_fTerrainBrushPressure=1024.0f*3.0f/4.0f; //75%
  m_iTerrainEditMode=TEM_HEIGHTMAP;
  m_iTerrainBrushMode=0;
  m_fTerrainBrushPressureEnum=-1.0f;
  m_fnDistributionNoiseTexture=CTFILENAME("Textures\\Editor\\RandomNoise.tex");
  m_fnContinousNoiseTexture=CTFILENAME("Textures\\Editor\\RandomNoise.tex");

  m_iFBMOctaves=4;
  m_fFBMHighFrequencyStep=1.0f;
  m_fFBMStepFactor=2.2f;
  m_fFBMMaxAmplitude=64.0f;
  m_fFBMfAmplitudeDecreaser=0.5f;
  m_bFBMAddNegativeValues=TRUE;
  m_bFBMRandomOffset=FALSE;

  m_uwEditAltitude=0;
  m_fPaintPower=1.0f;
  m_fSmoothPower=1.0f;
  m_iFilter=FLT_SHARPEN;
  m_fFilterPower=1.0f;
  m_fPosterizeStep=2.0f;
  m_fNoiseAltitude=1.0f;
  m_iRNDSubdivideAndDisplaceItterations=2;
  m_iTerrainGenerationMethod=0;
  
  m_pbpoClipboardPolygon = new CBrushPolygon;
  m_pbpoPolygonWithDeafultValues = new CBrushPolygon;
  m_bFirstTimeStarted = FALSE;

  // no copy operations performed yet
  m_ctLastCopyType = CT_NONE;
  m_colSectorAmbientClipboard = C_BLACK;
  m_bDecadicGrid = FALSE;

  // give personality to each values for primitive class
  m_vfpConus.vfp_ptPrimitiveType = PT_CONUS;
  m_vfpTorus.vfp_ptPrimitiveType = PT_TORUS;
  m_vfpStaircases.vfp_ptPrimitiveType = PT_STAIRCASES;
  m_vfpSphere.vfp_ptPrimitiveType = PT_SPHERE;
  m_vfpTerrain.vfp_ptPrimitiveType = PT_TERRAIN;
  m_bTexture1 = TRUE;
  m_bTexture2 = TRUE;
  m_bTexture3 = TRUE;
  m_fTerrainSwitchStart = 100.0f;
  m_fTerrainSwitchStep = 1.0f;

  m_iLastClassSortAplied = 0;
  m_bInvertClassSort = FALSE;
  m_iLastAutoColorizeColor = 0;

  m_plClipboard1 = CPlacement3D( FLOAT3D(0.0f,0.0f,0.0f), ANGLE3D(0,0,0));
  m_plClipboard2 = CPlacement3D( FLOAT3D(0.0f,0.0f,0.0f), ANGLE3D(0,0,0));
  m_tmStartStatusLineInfo=0;
}

// default constructor
CViewPrefs::CViewPrefs( void)
{
  // set default values
  SetDefaultValues();
}

/*
 * Set default values for view preferences
 */
void CViewPrefs::SetDefaultValues( void)
{
  // set view's defaults
  m_bAutoRenderingRange = TRUE;
  m_fRenderingRange = 100.0f;
  m_PaperColor = C_WHITE;
  m_SelectionColor = C_RED;
  m_GridColor = 0xAFBDFE;
  m_bMeasurementTape = FALSE;
  // set defaults for world
  m_wrpWorldRenderPrefs.SetHiddenLinesOn( TRUE);
  m_wrpWorldRenderPrefs.SetEditorModelsOn( TRUE);
  m_wrpWorldRenderPrefs.SetFieldBrushesOn( TRUE);
  m_wrpWorldRenderPrefs.SetBackgroundTextureOn( FALSE);
  m_wrpWorldRenderPrefs.SetVerticesFillType( CWorldRenderPrefs::FT_NONE);
  m_wrpWorldRenderPrefs.SetVerticesInkColor( C_RED);
  m_wrpWorldRenderPrefs.SetEdgesFillType( CWorldRenderPrefs::FT_POLYGONCOLOR);
  m_wrpWorldRenderPrefs.SetEdgesInkColor( C_BLACK);
  m_wrpWorldRenderPrefs.SetPolygonsFillType( CWorldRenderPrefs::FT_NONE);
  m_wrpWorldRenderPrefs.SetPolygonsInkColor( C_GRAY);
  m_wrpWorldRenderPrefs.SetLensFlaresType( CWorldRenderPrefs::LFT_NONE);
  m_wrpWorldRenderPrefs.SetFogOn( FALSE);
  m_wrpWorldRenderPrefs.SetHazeOn( FALSE);
  m_wrpWorldRenderPrefs.SetMirrorsOn( FALSE);
  m_wrpWorldRenderPrefs.SetShowTargetsOn( FALSE);
  m_wrpWorldRenderPrefs.SetShowEntityNamesOn( FALSE);

  // set defaults for models
  m_mrpModelRenderPrefs.SetRenderType( RT_TEXTURE);
  m_mrpModelRenderPrefs.SetShadingType( RT_SHADING_PHONG);
  m_mrpModelRenderPrefs.SetShadowQuality( 0);
  m_mrpModelRenderPrefs.SetInkColor( C_BLACK);
  m_mrpModelRenderPrefs.BBoxFrameShow( FALSE);
  m_mrpModelRenderPrefs.BBoxAllShow( FALSE);
  m_mrpModelRenderPrefs.SetWire( FALSE);
  sprintf( m_achrBcgPicture, "");
}

void CViewPrefs::ClearInvalidConfigPointers(void)
{
  m_wrpWorldRenderPrefs.SetSelectedEntityModel(NULL);
  m_wrpWorldRenderPrefs.SetSelectedPortalModel(NULL);
  m_wrpWorldRenderPrefs.SetEmptyBrushModel(NULL);
}

CAppPrefs::~CAppPrefs()
{
}

CWorldEditorApp::~CWorldEditorApp()
{
}

CWorldEditorDoc* CWorldEditorApp::GetActiveDocument(void)
{
  // if document change process is on (document view switching)
  if( m_bDocumentChangeOn)
  {
    return NULL;
  }
  else
  {
    // obtain active view
    CWorldEditorView *pWorldEditorView = GetActiveView();
    // if there is active view
    if( pWorldEditorView != NULL)
    {
      // return document that view represents
      return GetActiveView()->GetDocument();
    }
    // otherwise
    else
    {
      return NULL;
    }
  }
}

CWorldEditorView* CWorldEditorApp::GetActiveView(void)
{
  CWorldEditorView *res;
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  res = DYNAMIC_DOWNCAST(CWorldEditorView, pMainFrame->GetActiveFrame()->GetActiveView());
  return res;
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CWorldEditorApp object

CWorldEditorApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CWorldEditorApp initialization

BOOL CWorldEditorApp::InitInstance()
{
  _CrtSetBreakAlloc(55);
  BOOL bResult;
  CTSTREAM_BEGIN {
    bResult = SubInitInstance();
  } CTSTREAM_END;
  return bResult;
}

static CTString _strCmd;
static CString _strCmdW;
static CTString cmd_strOutput;
static CTString cmd_strMod;

// get first next word or quoted string
static CTString GetNextParam(void)
{
  // strip leading spaces/tabs
  _strCmd.TrimSpacesLeft();
  // if nothing left
  if (_strCmd=="") {
    // no word to return
    return "";
  }

  // if the first char is quote
  if (_strCmd[0]=='"') {
    // find first next quote
    const char *pchClosingQuote = strchr(_strCmd+1, '"');
    // if not found
    if (pchClosingQuote==NULL) {
      // error in command line
      cmd_strOutput+=CTString(0, TRANS("Command line error!\n"));
      // finish parsing
      _strCmd = "";
      return "";
    }
    INDEX iQuote = pchClosingQuote-_strCmd;

    // get the quoted string
    CTString strWord;
    CTString strRest;
    _strCmd.Split(iQuote, strWord, strRest);
    // remove the quotes
    strWord.DeleteChar(0);
    strRest.DeleteChar(0);
    // get the word
    _strCmd = strRest;
    return strWord;

  // if the first char is not quote
  } else {
    // find first next space
    size_t iSpace;
    size_t ctChars = strlen(_strCmd);
    for(iSpace=0; iSpace<ctChars; iSpace++) {
      if (isspace(_strCmd[iSpace])) {
        break;
      }
    }
    // get the word string
    CTString strWord;
    CTString strRest;
    _strCmd.Split(iSpace, strWord, strRest);
    // remove the space
    strRest.DeleteChar(0);
    // get the word
    _strCmd = strRest;
    return strWord;
  }
}

// check for custom parameters
void CWorldEditorApp::MyParseCommandLine(void)
{
  _strCmd = CStringA(m_lpCmdLine);
  cmd_strOutput = "";
  cmd_strOutput+=CTString(0, TRANS("Command line: '%s'\n"), _strCmd);
  // if no command line
  if (strlen(_strCmd) == 0) {
    // do nothing
    return;
  }

  FOREVER {
    CTString strWord = GetNextParam();
    if (strWord=="") {
      cmd_strOutput+="\n";
      _strCmdW = CString(_strCmd);
      m_lpCmdLine = (LPWSTR)(LPCWSTR)_strCmdW;
      return;
    } else if (strWord=="+game") {
      CTString strMod = GetNextParam();
      if (strMod!="SeriousSam") { // (we ignore default mod - always use base dir in that case)
        cmd_strMod = strMod;
        _fnmMod = "Mods\\"+strMod+"\\";
      }
    } else {
      _strCmdW = CString(_strCmd);
      m_lpCmdLine = (LPWSTR)(LPCWSTR)_strCmdW;
      return;
    }
  }
  
}

BOOL CWorldEditorApp::SubInitInstance()
{
  // required for visual styles
  InitCommonControls();

 	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

	// Initialize OLE 2.0 libraries
	if (!AfxOleInit())
	{
    AfxMessageBox(L"ERROR: Failed to initialize OLE 2.0 libraries");
		return FALSE;
	}
	AfxEnableControlContainer();

  // check for custom parameters
  MyParseCommandLine();

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

  // initialize entire engine
  SE_InitEngine("SeriousEditor");
  SE_LoadDefaultFonts();

  // settings will be saved into registry instead of ini file
  if (_strModExt=="") {
    SetRegistryKey( CString("CroTeam"));
  } else {
    SetRegistryKey( CString("CroTeam\\"+_strModExt));
  }

  CPrintF("%s", cmd_strOutput);

  // if the registry is not set yet
  CString strDefaultTexture = GetProfileString( L"World editor prefs", L"Default primitive texture", L"");
  if (strDefaultTexture == L"") {
    // load registry from the ini file
    CTString strCommand;
    strCommand.PrintF("regedit.exe -s \"%s%s\"",
      (const CTString&)_fnmApplicationPath,
      (const CTString&)CTString("Data\\Defaults\\WorldEditor.reg"));
    system(strCommand);
/*    _spawnlp(_P_WAIT, "regedit.exe", 
      "-s",
      (const CTString&)(_fnmApplicationPath+CTString("Data\\Defaults\\WorldEditor.reg")),
      NULL);
    */
  }

	LoadStdProfileSettings(8);  // Load standard INI file options (including MRU)

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views.

	m_pDocTemplate = new CMultiDocTemplate(
		IDR_WEDTYPE,
		RUNTIME_CLASS(CWorldEditorDoc),
		RUNTIME_CLASS(CChildFrame), // custom MDI child frame
		RUNTIME_CLASS(CWorldEditorView));
	AddDocTemplate(m_pDocTemplate);

  //  create application windows font
	LOGFONT logFont;
	memset(&logFont, 0, sizeof(logFont));
	if( !::GetSystemMetrics(SM_DBCSENABLED)){
		logFont.lfHeight = -11;
		logFont.lfWeight = FW_REGULAR;
		logFont.lfPitchAndFamily = FF_ROMAN;
    logFont.lfOrientation = 10;
    logFont.lfQuality = PROOF_QUALITY;
    logFont.lfItalic = TRUE;
		lstrcpy(logFont.lfFaceName, L"Arial");
    if( !m_Font.CreateFontIndirect(&logFont))
			TRACE0("Could Not create font for combo\n");
	}	else {
    m_Font.Attach(::GetStockObject(SYSTEM_FONT));
	}

	memset(&logFont, 0, sizeof(logFont));
	if( !::GetSystemMetrics(SM_DBCSENABLED)){
		logFont.lfHeight = -11;
		logFont.lfWeight = FW_REGULAR;
		logFont.lfPitchAndFamily = FF_MODERN;
    logFont.lfOrientation = 10;
    logFont.lfQuality = PROOF_QUALITY;
    logFont.lfItalic = FALSE;
		lstrcpy(logFont.lfFaceName, L"Courier New");
    if( !m_FixedFont.CreateFontIndirect(&logFont))
			TRACE0("Could Not create fixed font\n");
	}	else {
    m_FixedFont.Attach(::GetStockObject(SYSTEM_FONT));
	}

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

  // add console variables
  extern INDEX wed_bSaveTestGameFirstTime;
  _pShell->DeclareSymbol("user INDEX wed_bSaveTestGameFirstTime;", &wed_bSaveTestGameFirstTime);
  _pShell->DeclareSymbol("persistent user INDEX wed_iMaxFPSActive;", &wed_iMaxFPSActive);
  _pShell->DeclareSymbol("persistent user FLOAT wed_fFrontClipDistance;", &wed_fFrontClipDistance);
  _pShell->DeclareSymbol("persistent user INDEX wed_bUseGenericTextureReplacement;", &wed_bUseGenericTextureReplacement);

  // functions that are used to change rendering preferences while testing game
  _pShell->DeclareSymbol("user void WED_ApplyChildSettings0(void);", &WED_ApplyChildSettings0);
  _pShell->DeclareSymbol("user void WED_ApplyChildSettings1(void);", &WED_ApplyChildSettings1);
  _pShell->DeclareSymbol("user void WED_ApplyChildSettings2(void);", &WED_ApplyChildSettings2);
  _pShell->DeclareSymbol("user void WED_ApplyChildSettings3(void);", &WED_ApplyChildSettings3);
  _pShell->DeclareSymbol("user void WED_ApplyChildSettings4(void);", &WED_ApplyChildSettings4);
  _pShell->DeclareSymbol("user void WED_ApplyChildSettings5(void);", &WED_ApplyChildSettings5);
  _pShell->DeclareSymbol("user void WED_ApplyChildSettings6(void);", &WED_ApplyChildSettings6);
  _pShell->DeclareSymbol("user void WED_ApplyChildSettings7(void);", &WED_ApplyChildSettings7);
  _pShell->DeclareSymbol("user void WED_ApplyChildSettings8(void);", &WED_ApplyChildSettings8);
  _pShell->DeclareSymbol("user void WED_ApplyChildSettings9(void);", &WED_ApplyChildSettings9);

  _pShell->DeclareSymbol("user void WED_ApplyRenderingSettings0(void);", &WED_ApplyRenderingSettings0);
  _pShell->DeclareSymbol("user void WED_ApplyRenderingSettings1(void);", &WED_ApplyRenderingSettings1);
  _pShell->DeclareSymbol("user void WED_ApplyRenderingSettings2(void);", &WED_ApplyRenderingSettings2);
  _pShell->DeclareSymbol("user void WED_ApplyRenderingSettings3(void);", &WED_ApplyRenderingSettings3);
  _pShell->DeclareSymbol("user void WED_ApplyRenderingSettings4(void);", &WED_ApplyRenderingSettings4);
  _pShell->DeclareSymbol("user void WED_ApplyRenderingSettings5(void);", &WED_ApplyRenderingSettings5);
  _pShell->DeclareSymbol("user void WED_ApplyRenderingSettings6(void);", &WED_ApplyRenderingSettings6);
  _pShell->DeclareSymbol("user void WED_ApplyRenderingSettings7(void);", &WED_ApplyRenderingSettings7);
  _pShell->DeclareSymbol("user void WED_ApplyRenderingSettings8(void);", &WED_ApplyRenderingSettings8);
  _pShell->DeclareSymbol("user void WED_ApplyRenderingSettings9(void);", &WED_ApplyRenderingSettings9);
  
  _pShell->DeclareSymbol("user void WED_FindEmptyBrush(void);", &WED_FindEmptyBrushes);

  // load persistent symbols
  _pShell->Execute(CTString("include \""+fnmPersistentSymbols+"\";"));

  // prepare full screen mode
  _EngineGUI.GetFullScreenModeFromRegistry( "Display modes", m_dmFullScreen, m_gatFullScreen);
  _EngineGUI.SetFullScreenModeToRegistry(   "Display modes", m_dmFullScreen, m_gatFullScreen);

  m_iApi=GAT_OGL;
  m_iApi=AfxGetApp()->GetProfileInt(L"Display modes", L"SED Gfx API", GAT_OGL);

  // (re)set default display mode
  _pGfx->ResetDisplayMode((enum GfxAPIType) m_iApi);

  // initialize game itself (GameShell interface) and load settings
  InitializeGame();
  // load startup script
  _pShell->Execute( "include \"Scripts\\WorldEditor_startup.ini\"");

  // read all data from ini file
  ReadFromIniFileOnInit();
  ReadDefaultPolygonValues();

  // load primitives history buffer
  CTString strPrimitives("Data\\PrimitivesHistory.pri");
  if (FileExists(strPrimitives)) {
    CTFileStream strmFile;
    try
    {
      strmFile.Open_t(strPrimitives);
      INDEX ctHistory;
      strmFile >> ctHistory;
      for( INDEX iPrim=0; iPrim<ctHistory; iPrim++)
      {
        CPrimitiveInHistoryBuffer *ppihbMember = new CPrimitiveInHistoryBuffer;
        ppihbMember->pihb_vfpPrimitive.Read_t( strmFile);
        m_lhPrimitiveHistory.AddTail( ppihbMember->pihb_lnNode);
      }
    }
    catch (const char *strError)
    {
      WarningMessage( strError);
    }
  }

  m_bDecadicGrid = !m_Preferences.ap_BinaryGrid;

  // don't start new document automatically
  cmdInfo.m_nShellCommand = CCommandLineInfo::FileNothing;

  // create temporary directory to contain undo files
  CreateDirectoryA( _fnmApplicationPath + "Temp\\", NULL);

  // try to
  try
  {
    // load error texture
  	DECLARE_CTFILENAME( fnErrorTexture, "Textures\\Editor\\Error.tex");
    m_ptdError = _pTextureStock->Obtain_t( fnErrorTexture);
    // load error texture
  	DECLARE_CTFILENAME( fnViewIcons, "Models\\Editor\\ViewIcons.tex");
    m_pViewIconsTD = _pTextureStock->Obtain_t( fnViewIcons);
    // load icon tray texture
  	DECLARE_CTFILENAME( fnIconTrayTexture, "Textures\\Editor\\IconsTray.tex");
    m_ptdIconsTray = _pTextureStock->Obtain_t( fnIconTrayTexture);
  }
  catch (const char *err_str)
  {
    AfxMessageBox( CString(err_str));
		return FALSE;
  }
  m_ptoError = new CTextureObject;
  m_ptoError->SetData( m_ptdError);

  // assign system font
  m_pfntSystem = _pfdDisplayFont;
  
  try
  {
    // load entity selection marker model
  	DECLARE_CTFILENAME( fnEntityMarker, "Models\\Editor\\EntityMarker.mdl");
    m_pEntityMarkerModelData = _pModelStock->Obtain_t( fnEntityMarker);
    m_pEntityMarkerModelObject = new CModelObject;
    m_pEntityMarkerModelObject->SetData(m_pEntityMarkerModelData);
    m_pEntityMarkerModelObject->SetAnim( 0);
    // load entity selection marker model's texture
    DECLARE_CTFILENAME( fnEntityMarkerTex, "Models\\Editor\\EntityMarker.tex");
    m_ptdEntityMarkerTexture = _pTextureStock->Obtain_t( fnEntityMarkerTex);
    m_pEntityMarkerModelObject->mo_toTexture.SetData( m_ptdEntityMarkerTexture);

    // load portal selection marker model
  	DECLARE_CTFILENAME( fnPortalMarker, "Models\\Editor\\PortalMarker.mdl");
    m_pPortalMarkerModelData = _pModelStock->Obtain_t( fnPortalMarker);
    m_pPortalMarkerModelObject = new CModelObject;
    m_pPortalMarkerModelObject->SetData(m_pPortalMarkerModelData);
    m_pPortalMarkerModelObject->SetAnim( 0);
    // load portal selection marker model's texture
    DECLARE_CTFILENAME( fnPortalMarkerTex, "Models\\Editor\\PortalMarker.tex");
    m_ptdPortalMarkerTexture = _pTextureStock->Obtain_t( fnPortalMarkerTex);
    m_pPortalMarkerModelObject->mo_toTexture.SetData( m_ptdPortalMarkerTexture);

    // load empty brush model
  	DECLARE_CTFILENAME( fnEmptyBrush, "Models\\Editor\\EmptyBrush.mdl");
    m_pEmptyBrushModelData = _pModelStock->Obtain_t( fnEmptyBrush);
    m_pEmptyBrushModelObject = new CModelObject;
    m_pEmptyBrushModelObject->SetData(m_pEmptyBrushModelData);
    m_pEmptyBrushModelObject->SetAnim( 0);
    // load empty brush model's texture
    DECLARE_CTFILENAME( fnEmptyBrushTex, "Models\\Editor\\EmptyBrush.tex");
    m_ptdEmptyBrushTexture = _pTextureStock->Obtain_t( fnEmptyBrushTex);
    m_pEmptyBrushModelObject->mo_toTexture.SetData( m_ptdEmptyBrushTexture);

    // load range sphere
  	DECLARE_CTFILENAME( fnRangeSphere, "Models\\Editor\\RangeSphere.mdl");
    m_pRangeSphereModelData = _pModelStock->Obtain_t( fnRangeSphere);
    m_pRangeSphereModelObject = new CModelObject;
    m_pRangeSphereModelObject->SetData(m_pRangeSphereModelData);
    m_pRangeSphereModelObject->SetAnim( 0);
    // load range sphere model's texture
    DECLARE_CTFILENAME( fnRangeSphereTex, "Models\\Editor\\RangeSphere.tex");
    m_ptdRangeSphereTexture = _pTextureStock->Obtain_t( fnRangeSphereTex);
    m_pRangeSphereModelObject->mo_toTexture.SetData( m_ptdRangeSphereTexture);

    // load angle 3d model
  	DECLARE_CTFILENAME( fnAngle3D, "Models\\Editor\\AngleVector.mdl");
    m_pAngle3DModelData = _pModelStock->Obtain_t( fnAngle3D);
    m_pAngle3DModelObject = new CModelObject;
    m_pAngle3DModelObject->SetData(m_pAngle3DModelData);
    m_pAngle3DModelObject->SetAnim( 0);
    // load angle 3d model's texture
    DECLARE_CTFILENAME( fnAngle3DTex, "Models\\Editor\\Vector.tex");
    m_ptdAngle3DTexture = _pTextureStock->Obtain_t( fnAngle3DTex);
    m_pAngle3DModelObject->mo_toTexture.SetData( m_ptdAngle3DTexture);

    // load bounding box
  	DECLARE_CTFILENAME( fnBoundingBox, "Models\\Editor\\BoundingBox.mdl");
    m_pBoundingBoxModelData = _pModelStock->Obtain_t( fnBoundingBox);
    m_pBoundingBoxModelObject = new CModelObject;
    m_pBoundingBoxModelObject->SetData(m_pBoundingBoxModelData);
    m_pBoundingBoxModelObject->SetAnim( 0);
    // load bounding box model's texture
    DECLARE_CTFILENAME( fnBoundingBoxTex, "Models\\Editor\\BoundingBox.tex");
    m_ptdBoundingBoxTexture = _pTextureStock->Obtain_t( fnBoundingBoxTex);
    m_pBoundingBoxModelObject->mo_toTexture.SetData( m_ptdBoundingBoxTexture);
  }
  catch (const char *error)
  {
    FatalError("Cannot load one of entity selection model components: %s", error);
    return FALSE;
  }

  // initialize browsing window's view port
  CBrowseWindow *pBrowseWindow = &pMainFrame->m_Browser.m_BrowseWindow;
  _pGfx->CreateWindowCanvas( pMainFrame->m_Browser.m_BrowseWindow.m_hWnd, &pBrowseWindow->m_pViewPort,
                             &pBrowseWindow->m_pDrawPort);
  pMainFrame->m_Browser.OpenSelectedDirectory();

  // assure that terrain brushes exist
  GenerateNonExistingTerrainEditBrushes();

  // Dispatch commands specified on the command line
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;

	// The main window has been initialized, so show and update it.
	pMainFrame->ShowWindow(SW_SHOWMAXIMIZED);
	pMainFrame->UpdateWindow();

  // show tip of the day
  if (m_bShowTipOfTheDay) {
    OnHelpShowTipOfTheDay();
  }

  // if we should automatically call preferences dialog (because WED is first time started)
  if( m_bFirstTimeStarted) OnFilePreferences();

  CTString strCmdLine=CTString(CStringA(m_lpCmdLine));
  if(strCmdLine[0]=='\"' && strCmdLine[strCmdLine.Length()-1]=='\"')
  {
    strCmdLine.DeleteChar(0);
    strCmdLine.DeleteChar(strCmdLine.Length()-1);
  }

  if (strCmdLine != "" && !strCmdLine.RemovePrefix("/dde"))
  {
    // Open a file passed as the first command line parameter.
    OpenDocumentFile(m_lpCmdLine);
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
void CWorldEditorApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// CWorldEditorApp commands

BOOL CWorldEditorApp::SaveAllModified()
{
	CMainFrame* pMainFrame = (CMainFrame *) m_pMainWnd;

	if( (pMainFrame != NULL) && (pMainFrame->m_Browser.m_bVirtualTreeChanged) )
  {
    switch(::MessageBoxA( pMainFrame->m_hWnd,
      "Virtual tree changed but not saved. Do You want to save it?", "Warning !",
      MB_YESNOCANCEL | MB_ICONWARNING | MB_DEFBUTTON1 | MB_TASKMODAL | MB_TOPMOST) )
    {
    case IDYES:
      {
        pMainFrame->m_Browser.SaveVirtualTree(pMainFrame->m_fnLastVirtualTree, &pMainFrame->m_Browser.m_VirtualTree);
        break;
      }
    case IDCANCEL:
      {
        return FALSE;
      }
    }
  }

	return CWinApp::SaveAllModified();
}

void CWorldEditorApp::ReadFromIniFileOnInit(void)
{
  char strIni[ 256];

  // read data that can be saved multiple times to ini file
  ReadFromIniFile();

  // obtain texture for primitive
  CString strTexture = GetProfileString( L"World editor prefs", L"Default primitive texture", L"Textures\\Editor\\Default.tex");
  // if exists in ini file
  if( strTexture != L"")
  {
    theApp.SetNewActiveTexture( _fnmApplicationPath + CTString(CStringA(strTexture)));
  }

  INI_READ( "Paint power", "1.0");
  GET_FLOAT( m_fPaintPower);

  INI_READ( "Smooth power", "1.0");
  GET_FLOAT( m_fSmoothPower);

  INI_READ( "Current filter", "1");
  GET_INDEX( m_iFilter);

  INI_READ( "Posterize step", "2.0");
  GET_FLOAT( m_fPosterizeStep);

  INI_READ( "Terrain generation method", "0");
  GET_INDEX( m_iTerrainGenerationMethod);

  INI_READ( "Subdivade and displace itterations", "2");
  GET_INDEX( m_iRNDSubdivideAndDisplaceItterations);

  INI_READ( "Noise altitude", "1.0");
  GET_FLOAT( m_fNoiseAltitude);  

  INI_READ( "Distribution noise texture", CTFILENAME("Textures\\Editor\\RandomNoise.tex"));
  GET_STRING( m_fnDistributionNoiseTexture);
  
  INI_READ( "Continous noise texture", CTFILENAME("Textures\\Editor\\RandomNoise.tex"));
  GET_STRING( m_fnContinousNoiseTexture);

  INI_READ( "FBM Octaves", "4");
  GET_INDEX( m_iFBMOctaves);

  INI_READ( "FBM High frequency step", "1.0");
  GET_FLOAT( m_fFBMHighFrequencyStep);

  INI_READ( "FBM Step factor", "2.0");
  GET_FLOAT( m_fFBMStepFactor);

  INI_READ( "FBM Max amplitude", "64.0");
  GET_FLOAT( m_fFBMMaxAmplitude);

  INI_READ( "FBM Amplitude decreaser", "0.5");
  GET_FLOAT( m_fFBMfAmplitudeDecreaser);

  INI_READ( "FBM Add negative values", "NO");
  GET_FLAG( m_bFBMAddNegativeValues);  

  INI_READ( "FBM Random offset", "NO");
  GET_FLAG( m_bFBMRandomOffset);  

  m_bShowTipOfTheDay = GetProfileInt(L"World editor", L"Show Tip of the Day", TRUE);
  m_iCurrentTipOfTheDay = GetProfileInt(L"World editor", L"Current Tip of the Day", 0);
  _fFlyModeSpeedMultiplier=m_Preferences.ap_fDefaultFlyModeSpeed;
}

void CWorldEditorApp::ReadFromIniFile()
{
  // if loading of world and model's rendering preferences file fails
  if( !LoadRenderingPreferences())
  {
    CViewPrefs tempVP;
    // for all view's rendering preferences
    for( INDEX i=0; i< VIEW_PREFERENCES_CT; i++)
    {
      // clear possibly wrong loaded values
      m_vpViewPrefs[ i] = tempVP;
      // set default values
      m_vpViewPrefs[ i].SetDefaultValues();
    }

    // BUFFER 0: -default view
    // BUFFER 1: -no REM
    m_vpViewPrefs[ 1].m_wrpWorldRenderPrefs.SetEditorModelsOn( FALSE);
    m_vpViewPrefs[ 1].m_wrpWorldRenderPrefs.SetFieldBrushesOn( FALSE);
    m_vpViewPrefs[ 1].m_wrpWorldRenderPrefs.SetLensFlaresType( CWorldRenderPrefs::LFT_REFLECTIONS_AND_GLARE);
    // BUFFER 2: -polygon color, edges ink, no REM
    m_vpViewPrefs[ 2].m_wrpWorldRenderPrefs.SetPolygonsFillType( CWorldRenderPrefs::FT_POLYGONCOLOR);
    m_vpViewPrefs[ 2].m_wrpWorldRenderPrefs.SetEdgesFillType( CWorldRenderPrefs::FT_INKCOLOR);
    m_vpViewPrefs[ 2].m_wrpWorldRenderPrefs.SetEditorModelsOn( FALSE);
    m_vpViewPrefs[ 2].m_wrpWorldRenderPrefs.SetFieldBrushesOn( FALSE);
    m_vpViewPrefs[ 2].m_wrpWorldRenderPrefs.SetLensFlaresType( CWorldRenderPrefs::LFT_REFLECTIONS_AND_GLARE);
    // BUFFER 3: -wire frame (lines in color of polygons)
    m_vpViewPrefs[ 3].m_wrpWorldRenderPrefs.SetPolygonsFillType( CWorldRenderPrefs::FT_NONE);
    m_vpViewPrefs[ 3].m_wrpWorldRenderPrefs.SetEditorModelsOn( FALSE);
    m_vpViewPrefs[ 3].m_wrpWorldRenderPrefs.SetFieldBrushesOn( FALSE);
    m_vpViewPrefs[ 3].m_mrpModelRenderPrefs.BBoxAllShow( TRUE);
    // BUFFER 4: -polygons show sector color, no REM
    m_vpViewPrefs[ 4].m_wrpWorldRenderPrefs.SetEdgesFillType( CWorldRenderPrefs::FT_INKCOLOR);
    m_vpViewPrefs[ 4].m_wrpWorldRenderPrefs.SetPolygonsFillType( CWorldRenderPrefs::FT_SECTORCOLOR);
    m_vpViewPrefs[ 4].m_wrpWorldRenderPrefs.SetEditorModelsOn( FALSE);
    m_vpViewPrefs[ 4].m_wrpWorldRenderPrefs.SetFieldBrushesOn( FALSE);
    // BUFFER 5: -polygons in gray color, no REM
    m_vpViewPrefs[ 5].m_wrpWorldRenderPrefs.SetEdgesFillType( CWorldRenderPrefs::FT_NONE);
    m_vpViewPrefs[ 5].m_wrpWorldRenderPrefs.SetPolygonsFillType( CWorldRenderPrefs::FT_INKCOLOR);
    m_vpViewPrefs[ 5].m_wrpWorldRenderPrefs.SetEditorModelsOn( FALSE);
    m_vpViewPrefs[ 5].m_wrpWorldRenderPrefs.SetFieldBrushesOn( FALSE);
    m_vpViewPrefs[ 5].m_wrpWorldRenderPrefs.SetPolygonsInkColor( C_GRAY);
    // BUFFER 6: -polygons color, no REM
    m_vpViewPrefs[ 6].m_wrpWorldRenderPrefs.SetPolygonsFillType( CWorldRenderPrefs::FT_POLYGONCOLOR);
    m_vpViewPrefs[ 6].m_wrpWorldRenderPrefs.SetEdgesFillType( CWorldRenderPrefs::FT_NONE);
    m_vpViewPrefs[ 6].m_wrpWorldRenderPrefs.SetEditorModelsOn( FALSE);
    m_vpViewPrefs[ 6].m_wrpWorldRenderPrefs.SetFieldBrushesOn( FALSE);
    // BUFFER 7: -polygons in white color
    m_vpViewPrefs[ 7].m_wrpWorldRenderPrefs.SetPolygonsFillType( CWorldRenderPrefs::FT_INKCOLOR);
    m_vpViewPrefs[ 7].m_wrpWorldRenderPrefs.SetEdgesFillType( CWorldRenderPrefs::FT_INKCOLOR);
    m_vpViewPrefs[ 7].m_wrpWorldRenderPrefs.SetPolygonsInkColor( C_WHITE);
    // BUFFER 8: -polygons use texture, edges ink
    m_vpViewPrefs[ 8].m_wrpWorldRenderPrefs.SetPolygonsFillType( CWorldRenderPrefs::FT_TEXTURE);
    m_vpViewPrefs[ 8].m_wrpWorldRenderPrefs.SetEdgesFillType( CWorldRenderPrefs::FT_INKCOLOR);
    // BUFFER 9: -game view with background texture
    m_vpViewPrefs[ VIEW_PREFERENCES_CT-1].m_wrpWorldRenderPrefs.SetPolygonsFillType( CWorldRenderPrefs::FT_TEXTURE);
    m_vpViewPrefs[ VIEW_PREFERENCES_CT-1].m_wrpWorldRenderPrefs.SetEdgesFillType( CWorldRenderPrefs::FT_NONE);
    m_vpViewPrefs[ VIEW_PREFERENCES_CT-1].m_wrpWorldRenderPrefs.SetEditorModelsOn( FALSE);
    m_vpViewPrefs[ VIEW_PREFERENCES_CT-1].m_wrpWorldRenderPrefs.SetFieldBrushesOn( FALSE);
    m_vpViewPrefs[ VIEW_PREFERENCES_CT-1].m_wrpWorldRenderPrefs.SetLensFlaresType( CWorldRenderPrefs::LFT_REFLECTIONS_AND_GLARE);
    m_vpViewPrefs[ VIEW_PREFERENCES_CT-1].m_wrpWorldRenderPrefs.SetBackgroundTextureOn( TRUE);
    m_vpViewPrefs[ VIEW_PREFERENCES_CT-1].m_wrpWorldRenderPrefs.SetFogOn( TRUE);
    m_vpViewPrefs[ VIEW_PREFERENCES_CT-1].m_wrpWorldRenderPrefs.SetHazeOn( TRUE);
    m_vpViewPrefs[ VIEW_PREFERENCES_CT-1].m_wrpWorldRenderPrefs.SetMirrorsOn( TRUE);
  }

  // if loading of child configuration file fails
  if( !LoadChildConfigurations())
  {
    // for all child configurations
    for( INDEX i=0; i<CHILD_CONFIGURATIONS_CT; i++)
    {
      // set default values
      m_ccChildConfigurations[ i].SetDefaultValues();
    }
    // CONFIGURATION 0: 3+1 defaults, perspective has no wire frame, but has texture
    m_ccChildConfigurations[ 0].m_vpViewPrefs[ 1] = theApp.m_vpViewPrefs[ 0];
    m_ccChildConfigurations[ 0].m_vpViewPrefs[ 1].m_wrpWorldRenderPrefs.SetEdgesFillType( CWorldRenderPrefs::FT_NONE);
    m_ccChildConfigurations[ 0].m_vpViewPrefs[ 1].m_wrpWorldRenderPrefs.SetPolygonsFillType( CWorldRenderPrefs::FT_TEXTURE);
    // CONFIGURATION 1: 3+1, same as 0 but no editor models
    m_ccChildConfigurations[ 1].m_vpViewPrefs[ 0] = theApp.m_vpViewPrefs[ 1];
    m_ccChildConfigurations[ 1].m_vpViewPrefs[ 1] = theApp.m_vpViewPrefs[ 1];
    m_ccChildConfigurations[ 1].m_vpViewPrefs[ 2] = theApp.m_vpViewPrefs[ 1];
    m_ccChildConfigurations[ 1].m_vpViewPrefs[ 3] = theApp.m_vpViewPrefs[ 1];
    m_ccChildConfigurations[ 1].m_vpViewPrefs[ 1].m_wrpWorldRenderPrefs.SetEdgesFillType( CWorldRenderPrefs::FT_NONE);
    m_ccChildConfigurations[ 1].m_vpViewPrefs[ 1].m_wrpWorldRenderPrefs.SetPolygonsFillType( CWorldRenderPrefs::FT_TEXTURE);
    // CONFIGURATION 2: 3+1, polygon color, edges ink, no REM
    m_ccChildConfigurations[ 2].m_vpViewPrefs[ 0] = theApp.m_vpViewPrefs[ 1];
    m_ccChildConfigurations[ 2].m_vpViewPrefs[ 1] = theApp.m_vpViewPrefs[ 2];
    m_ccChildConfigurations[ 2].m_vpViewPrefs[ 2] = theApp.m_vpViewPrefs[ 1];
    m_ccChildConfigurations[ 2].m_vpViewPrefs[ 3] = theApp.m_vpViewPrefs[ 1];
    // CONFIGURATION 3: 3+1, texture fill in all views, no wire
    m_ccChildConfigurations[ 3].m_vpViewPrefs[ 0] = theApp.m_vpViewPrefs[ 0];
    m_ccChildConfigurations[ 3].m_vpViewPrefs[ 1] = theApp.m_vpViewPrefs[ 0];
    m_ccChildConfigurations[ 3].m_vpViewPrefs[ 2] = theApp.m_vpViewPrefs[ 0];
    m_ccChildConfigurations[ 3].m_vpViewPrefs[ 3] = theApp.m_vpViewPrefs[ 0];
    m_ccChildConfigurations[ 3].m_vpViewPrefs[ 0].m_wrpWorldRenderPrefs.SetEdgesFillType( CWorldRenderPrefs::FT_NONE);
    m_ccChildConfigurations[ 3].m_vpViewPrefs[ 0].m_wrpWorldRenderPrefs.SetPolygonsFillType( CWorldRenderPrefs::FT_TEXTURE);
    m_ccChildConfigurations[ 3].m_vpViewPrefs[ 1].m_wrpWorldRenderPrefs.SetEdgesFillType( CWorldRenderPrefs::FT_NONE);
    m_ccChildConfigurations[ 3].m_vpViewPrefs[ 1].m_wrpWorldRenderPrefs.SetPolygonsFillType( CWorldRenderPrefs::FT_TEXTURE);
    m_ccChildConfigurations[ 3].m_vpViewPrefs[ 2].m_wrpWorldRenderPrefs.SetEdgesFillType( CWorldRenderPrefs::FT_NONE);
    m_ccChildConfigurations[ 3].m_vpViewPrefs[ 2].m_wrpWorldRenderPrefs.SetPolygonsFillType( CWorldRenderPrefs::FT_TEXTURE);
    m_ccChildConfigurations[ 3].m_vpViewPrefs[ 3].m_wrpWorldRenderPrefs.SetEdgesFillType( CWorldRenderPrefs::FT_NONE);
    m_ccChildConfigurations[ 3].m_vpViewPrefs[ 3].m_wrpWorldRenderPrefs.SetPolygonsFillType( CWorldRenderPrefs::FT_TEXTURE);
    // CONFIGURATION 4: 1, shadows (gray), no REM
    m_ccChildConfigurations[ 4].m_iHorizontalSplitters = 1;
    m_ccChildConfigurations[ 4].m_iVerticalSplitters = 1;
    m_ccChildConfigurations[ 4].m_ptProjectionType[ 0] = CSlaveViewer::PT_PERSPECTIVE;
    m_ccChildConfigurations[ 4].m_vpViewPrefs[ 0] = theApp.m_vpViewPrefs[ 5];
    // CONFIGURATION 5: 1, wire frame
    m_ccChildConfigurations[ 5].m_iHorizontalSplitters = 1;
    m_ccChildConfigurations[ 5].m_iVerticalSplitters = 1;
    m_ccChildConfigurations[ 5].m_ptProjectionType[ 0] = CSlaveViewer::PT_PERSPECTIVE;
    m_ccChildConfigurations[ 5].m_vpViewPrefs[ 0] = theApp.m_vpViewPrefs[ 3];
    // CONFIGURATION 6: 1, clean, game view
    m_ccChildConfigurations[ 6].m_iHorizontalSplitters = 1;
    m_ccChildConfigurations[ 6].m_iVerticalSplitters = 1;
    m_ccChildConfigurations[ 6].m_ptProjectionType[ 0] = CSlaveViewer::PT_PERSPECTIVE;
    m_ccChildConfigurations[ 6].m_vpViewPrefs[ 0] = theApp.m_vpViewPrefs[ VIEW_PREFERENCES_CT-1];
    // CONFIGURATION 7: 1, shadows + black wire frame
    m_ccChildConfigurations[ 7].m_iHorizontalSplitters = 1;
    m_ccChildConfigurations[ 7].m_iVerticalSplitters = 1;
    m_ccChildConfigurations[ 7].m_ptProjectionType[ 0] = CSlaveViewer::PT_PERSPECTIVE;
    m_ccChildConfigurations[ 7].m_vpViewPrefs[ 0] = theApp.m_vpViewPrefs[ 5];
    m_ccChildConfigurations[ 7].m_vpViewPrefs[ 0].m_wrpWorldRenderPrefs.SetEdgesFillType( CWorldRenderPrefs::FT_INKCOLOR);
    // CONFIGURATION 8: 1, clean, edges ink
    m_ccChildConfigurations[ 8].m_iHorizontalSplitters = 1;
    m_ccChildConfigurations[ 8].m_iVerticalSplitters = 1;
    m_ccChildConfigurations[ 8].m_ptProjectionType[ 0] = CSlaveViewer::PT_PERSPECTIVE;
    m_ccChildConfigurations[ 8].m_vpViewPrefs[ 0] = theApp.m_vpViewPrefs[ 8];
    m_ccChildConfigurations[ 8].m_vpViewPrefs[ 0].m_wrpWorldRenderPrefs.SetEditorModelsOn( FALSE);
    m_ccChildConfigurations[ 8].m_vpViewPrefs[ 0].m_wrpWorldRenderPrefs.SetFieldBrushesOn( FALSE);
    // CONFIGURATION 9: 1, polygons use texture, edges ink
    m_ccChildConfigurations[ 9].m_iHorizontalSplitters = 1;
    m_ccChildConfigurations[ 9].m_iVerticalSplitters = 1;
    m_ccChildConfigurations[ 9].m_ptProjectionType[ 0] = CSlaveViewer::PT_PERSPECTIVE;
    m_ccChildConfigurations[ 9].m_vpViewPrefs[ 0] = theApp.m_vpViewPrefs[ 8];
    m_ccChildConfigurations[ 9].m_vpViewPrefs[ 0].m_wrpWorldRenderPrefs.SetEdgesFillType( CWorldRenderPrefs::FT_NONE);
  }

  // read values for preferences
  m_Preferences.ReadFromIniFile();

// read from INI primitive settings
  m_vfpPreLast.ReadFromIniFile("PreLast");
  m_vfpLast.ReadFromIniFile("Last");
  m_vfpCurrent.ReadFromIniFile("Current");
  m_vfpConus.ReadFromIniFile("Conus");
  m_vfpConus.vfp_ptPrimitiveType = PT_CONUS;
  m_vfpTorus.ReadFromIniFile("Torus");
  m_vfpTorus.vfp_ptPrimitiveType = PT_TORUS;
  m_vfpStaircases.ReadFromIniFile("Staircases");
  m_vfpStaircases.vfp_ptPrimitiveType = PT_STAIRCASES;
  m_vfpSphere.ReadFromIniFile("Sphere");
  m_vfpSphere.vfp_ptPrimitiveType = PT_SPHERE;
  m_vfpTerrain.ReadFromIniFile("Terrain");
  m_vfpTerrain.vfp_ptPrimitiveType = PT_TERRAIN;
}

// World editor ini read function for preferences
void CAppPrefs::ReadFromIniFile()
{
  char strIni[ 256];

  INI_READ( "Copy existing window preferences", "NO");
  GET_FLAG( ap_CopyExistingWindowPrefs);
  INI_READ( "Auto maximize window", "YES");
  GET_FLAG( ap_AutoMaximizeWindow);
  INI_READ( "Set default colors", "YES");
  GET_FLAG( ap_SetDefaultColors);
  INI_READ( "Automatic info", "YES");
  GET_FLAG( ap_AutomaticInfo);
  INI_READ( "Update allways", "NO");
  GET_FLAG( ap_UpdateAllways);
  INI_READ( "Binary grid", "YES");
  GET_FLAG( ap_BinaryGrid);
  INI_READ( "Save undo for delete", "YES");
  GET_FLAG( ap_bSaveUndoForDelete);
  INI_READ( "Auto colorize primitives", "YES");
  GET_FLAG( ap_bAutoColorize);
  INI_READ( "Show all on open", "YES");
  GET_FLAG( ap_bShowAllOnOpen);
  INI_READ( "Hide shadows on open", "NO");
  GET_FLAG( ap_bHideShadowsOnOpen);
  INI_READ( "Auto update displace map", "YES");
  GET_FLAG( ap_bAutoUpdateDisplaceMap);  

  INI_READ( "Undo levels", "10");
  GET_INDEX( ap_iUndoLevels);
  INI_READ( "Startup window setup", "0");
  GET_INDEX( ap_iStartupWindowSetup);

  INI_READ( "Ink color", "0X00000000");
  GET_COLOR( ap_DefaultInkColor);
  INI_READ( "Paper color", "0XAAAAAAAA");
  GET_COLOR( ap_DefaultPaperColor);
  INI_READ( "Current selection color", "0XFFFFFFFF");
  GET_COLOR( ap_DefaultSelectionColor);
  INI_READ( "Current grid color", "0XFF000000");
  GET_COLOR( ap_DefaultGridColor);

  INI_READ( "Source safe project", "$/Flesh/");
  GET_STRING( ap_strSourceSafeProject);

  INI_READ( "Default fly mode speed", "1.0");
  GET_FLOAT( ap_fDefaultFlyModeSpeed);  

  INI_READ( "Terrain selection visible", "0");
  GET_INDEX( ap_iTerrainSelectionVisible);
  INI_READ( "Terrain selection hidden", "1");
  GET_INDEX( ap_iTerrainSelectionHidden);

  INI_READ( "Memory for terrain undo", "32");
  GET_INDEX( ap_iMemoryForTerrainUndo);

  INI_READ( "Auto generate distribution", "YES");
  GET_FLAG( ap_bAutoUpdateTerrainDistribution);  
  
}

// read from INI last values for primitive
void CValuesForPrimitive::ReadFromIniFile(CTString strPrimitiveType)
{
  CSetFPUPrecision FPUPrecision(FPT_53BIT);
  char strIni[ 256];

  INI_PRIMITIVE_READ( "primitive type", "1");
  GET_INDEX( vfp_ptPrimitiveType);
  INI_PRIMITIVE_READ( "x", "0.0");
  GET_FLOAT( vfp_plPrimitive.pl_PositionVector(1));
  INI_PRIMITIVE_READ( "y", "0.0");
  GET_FLOAT( vfp_plPrimitive.pl_PositionVector(2));
  INI_PRIMITIVE_READ( "z", "0.0");
  GET_FLOAT( vfp_plPrimitive.pl_PositionVector(3));
  INI_PRIMITIVE_READ( "heading", "0.0");
  GET_FLOAT( vfp_plPrimitive.pl_OrientationAngle(1));
  INI_PRIMITIVE_READ( "pitch", "0.0");
  GET_FLOAT( vfp_plPrimitive.pl_OrientationAngle(2));
  INI_PRIMITIVE_READ( "banking", "0.0");
  GET_FLOAT( vfp_plPrimitive.pl_OrientationAngle(3));

  INI_PRIMITIVE_READ( "triangularisation type", "0");
  GET_INDEX( vfp_ttTriangularisationType);

  INI_PRIMITIVE_READ( "closed", "NO");
  GET_FLAG( vfp_bClosed);
  INI_PRIMITIVE_READ( "gouraud shadows", "NO");
  GET_FLAG( vfp_bDummy);
  INI_PRIMITIVE_READ( "auto create mip brushes", "NO");
  GET_FLAG( vfp_bAutoCreateMipBrushes);
  INI_PRIMITIVE_READ( "sectors color", "0X0000FF00");
  GET_COLOR( vfp_colSectorsColor);
  INI_PRIMITIVE_READ( "polygons color", "0XFF000000");
  GET_COLOR( vfp_colPolygonsColor);

  INI_PRIMITIVE_READ( "x min", "-8");
  GET_FLOAT( vfp_fXMin);
  INI_PRIMITIVE_READ( "x max", "8");
  GET_FLOAT( vfp_fXMax);
  INI_PRIMITIVE_READ( "y min", "0");
  GET_FLOAT( vfp_fYMin);
  INI_PRIMITIVE_READ( "y max", "8");
  GET_FLOAT( vfp_fYMax);
  INI_PRIMITIVE_READ( "z min", "-8");
  GET_FLOAT( vfp_fZMin);
  INI_PRIMITIVE_READ( "z max", "8");
  GET_FLOAT( vfp_fZMax);
  INI_PRIMITIVE_READ( "shear x", "0.0");
  GET_FLOAT( vfp_fShearX);
  INI_PRIMITIVE_READ( "shear z", "0.0");
  GET_FLOAT( vfp_fShearZ);
  INI_PRIMITIVE_READ( "stretch x", "1.0");
  GET_FLOAT( vfp_fStretchX);
  INI_PRIMITIVE_READ( "stretch y", "1.0");
  GET_FLOAT( vfp_fStretchY);

  INI_PRIMITIVE_READ( "linear staircases", "NO");
  GET_FLAG( vfp_bLinearStaircases);
  INI_PRIMITIVE_READ( "outer", "YES");
  GET_FLAG( vfp_bOuter);

  INI_PRIMITIVE_READ( "slices in 360", "12");
  GET_INDEX( vfp_iSlicesIn360);
  INI_PRIMITIVE_READ( "no of slices", "6");
  GET_INDEX( vfp_iNoOfSlices);
  INI_PRIMITIVE_READ( "radius", "32.0");
  GET_FLOAT( vfp_fRadius);

  INI_PRIMITIVE_READ( "Primitive no of slices", "6");
  GET_INDEX( vfp_iNoOfSlices);

  INI_PRIMITIVE_READ( "meridians", "6");
  GET_INDEX( vfp_iMeridians);
  INI_PRIMITIVE_READ( "paralels", "6");
  GET_INDEX( vfp_iParalels);
  INI_PRIMITIVE_READ( "slices per width", "6");
  GET_INDEX( vfp_iSlicesPerWidth);
  INI_PRIMITIVE_READ( "slices per height", "6");
  GET_INDEX( vfp_iSlicesPerHeight);
  INI_PRIMITIVE_READ( "top shape", "0");
  GET_INDEX( vfp_iTopShape);
  INI_PRIMITIVE_READ( "bottom shape", "0");
  GET_INDEX( vfp_iBottomShape);
  INI_PRIMITIVE_READ( "csg operation", "0");
  GET_INDEX( vfp_csgtCSGOperation);

  INI_PRIMITIVE_READ( "amplitude", "50.0");
  GET_FLOAT( vfp_fAmplitude);
  INI_PRIMITIVE_READ( "mip start", "6.0");
  GET_FLOAT( vfp_fMipStart);
  INI_PRIMITIVE_READ( "mip step", "1.5");
  GET_FLOAT( vfp_fMipStep);
  INI_PRIMITIVE_READ( "displacement picture", "");
  GET_STRING( vfp_fnDisplacement);

  INDEX ctPrimitiveBaseVertices;
  INI_PRIMITIVE_READ( "base vertices", "4");
  GET_INDEX( ctPrimitiveBaseVertices);
  vfp_avVerticesOnBaseOfPrimitive.Clear();
  vfp_avVerticesOnBaseOfPrimitive.New( ctPrimitiveBaseVertices);
  CalculatePrimitiveBase();
}
// write to INI last used values for primitive
void CValuesForPrimitive::WriteToIniFile(CTString strPrimitiveType)
{
  char strIni[ 256];

  SET_INDEX( vfp_ptPrimitiveType);
  INI_PRIMITIVE_WRITE( "primitive type");

  SET_INDEX( vfp_avVerticesOnBaseOfPrimitive.Count());
  INI_PRIMITIVE_WRITE( "base vertices");

  SET_FLOAT( vfp_plPrimitive.pl_PositionVector(1));
  INI_PRIMITIVE_WRITE( "x");
  SET_FLOAT( vfp_plPrimitive.pl_PositionVector(2));
  INI_PRIMITIVE_WRITE( "y");
  SET_FLOAT( vfp_plPrimitive.pl_PositionVector(3));
  INI_PRIMITIVE_WRITE( "z");
  SET_FLOAT( vfp_plPrimitive.pl_OrientationAngle(1));
  INI_PRIMITIVE_WRITE( "heading");
  SET_FLOAT( vfp_plPrimitive.pl_OrientationAngle(2));
  INI_PRIMITIVE_WRITE( "pitch");
  SET_FLOAT( vfp_plPrimitive.pl_OrientationAngle(3));
  INI_PRIMITIVE_WRITE( "banking");

  SET_INDEX( vfp_ttTriangularisationType);
  INI_PRIMITIVE_WRITE( "triangularisation type");

  SET_FLAG( vfp_bClosed);
  INI_PRIMITIVE_WRITE( "closed");
  SET_FLAG( vfp_bDummy);
  INI_PRIMITIVE_WRITE( "gouraud shadows");
  SET_FLAG( vfp_bAutoCreateMipBrushes);
  INI_PRIMITIVE_WRITE( "auto create mip brushes");
  SET_COLOR( vfp_colSectorsColor);
  INI_PRIMITIVE_WRITE( "sectors color");
  SET_COLOR( vfp_colPolygonsColor);
  INI_PRIMITIVE_WRITE( "polygons color");

  SET_FLOAT( vfp_fXMin);
  INI_PRIMITIVE_WRITE( "x min");
  SET_FLOAT( vfp_fXMax);
  INI_PRIMITIVE_WRITE( "x max");
  SET_FLOAT( vfp_fYMin);
  INI_PRIMITIVE_WRITE( "y min");
  SET_FLOAT( vfp_fYMax);
  INI_PRIMITIVE_WRITE( "y max");
  SET_FLOAT( vfp_fZMin);
  INI_PRIMITIVE_WRITE( "z min");
  SET_FLOAT( vfp_fZMax);
  INI_PRIMITIVE_WRITE( "z max");
  SET_FLOAT( vfp_fShearX);
  INI_PRIMITIVE_WRITE( "shear x");
  SET_FLOAT( vfp_fShearZ);
  INI_PRIMITIVE_WRITE( "shear z");
  SET_FLOAT( vfp_fStretchX);
  INI_PRIMITIVE_WRITE( "stretch x");
  SET_FLOAT( vfp_fStretchY);
  INI_PRIMITIVE_WRITE( "stretch y");

  SET_FLAG( vfp_bLinearStaircases);
  INI_PRIMITIVE_WRITE( "linear staircases");
  SET_FLAG( vfp_bOuter);
  INI_PRIMITIVE_WRITE( "outer");

  SET_INDEX( vfp_iSlicesIn360);
  INI_PRIMITIVE_WRITE( "slices in 360");
  SET_INDEX( vfp_iNoOfSlices);
  INI_PRIMITIVE_WRITE( "no of slices");
  SET_FLOAT( vfp_fRadius);
  INI_PRIMITIVE_WRITE( "radius");

  SET_INDEX( vfp_iNoOfSlices);
  INI_PRIMITIVE_WRITE( "Primitive no of slices");

  SET_INDEX( vfp_iMeridians);
  INI_PRIMITIVE_WRITE( "meridians");
  SET_INDEX( vfp_iParalels);
  INI_PRIMITIVE_WRITE( "paralels");
  SET_INDEX( vfp_iSlicesPerWidth);
  INI_PRIMITIVE_WRITE( "slices per width");
  SET_INDEX( vfp_iSlicesPerHeight);
  INI_PRIMITIVE_WRITE( "slices per height");
  SET_INDEX( vfp_iTopShape);
  INI_PRIMITIVE_WRITE( "top shape");
  SET_INDEX( vfp_iBottomShape);
  INI_PRIMITIVE_WRITE( "bottom shape");
  SET_INDEX( vfp_csgtCSGOperation);
  INI_PRIMITIVE_WRITE( "csg operation");
  SET_FLOAT( vfp_fAmplitude);
  INI_PRIMITIVE_WRITE( "amplitude");
  SET_FLOAT( vfp_fMipStart);
  INI_PRIMITIVE_WRITE( "mip start");
  SET_FLOAT( vfp_fMipStep);
  INI_PRIMITIVE_WRITE( "mip step");
  SET_STRING( vfp_fnDisplacement);
  INI_PRIMITIVE_WRITE( "displacement picture");
}

void CValuesForPrimitive::CalculatePrimitiveBase(void)
{
  ASSERT(GetFPUPrecision()==FPT_53BIT);

  // pick up number of vertices for base polygon
  INDEX vtxCt = vfp_avVerticesOnBaseOfPrimitive.Count();
  // calculate width and lenght
  DOUBLE fWidth = (vfp_fXMax-vfp_fXMin)/2.0f;
  DOUBLE fLenght = (vfp_fZMax-vfp_fZMin)/2.0f;
  // some values must be valid, if they are not, coorect them
  if( fWidth < SNAP_FLOAT_12) fWidth = SNAP_FLOAT_12;
  if( fLenght < SNAP_FLOAT_12) fLenght = SNAP_FLOAT_12;

  // We calculate vertices as this is box-type primitive
  // Step, ammount of angle ct increasement
  ANGLE angle = AngleDeg(360.0)/vtxCt;
  ANGLE angleCt = -angle/2;
  // Radius of circle surrounding polygon if height of polygon's basic triangle is 1 m
  DOUBLE dA = fWidth/Cos(angle/2);
  DOUBLE dB = dA*fLenght/fWidth;

  // if base is created inside circle
  if( !vfp_bOuter)
  {
    angleCt = 0.0;
    dA = fWidth;
    dB = fLenght;
  }
  // for all of the base vertices
  for(INDEX iVtx=0; iVtx<vtxCt; iVtx++)
  {
    DOUBLE x = Cos( angleCt) * dA + (vfp_fXMin+vfp_fXMax)/2.0f;
    DOUBLE z = Sin( angleCt) * dB + (vfp_fZMin+vfp_fZMax)/2.0f;
    // snap X coordinate (1 cm)
    //Snap(x, SNAP_DOUBLE_CM);
    // snap Y coordinate (1 cm)
    //Snap(z, SNAP_DOUBLE_CM);
    // calculate vertice on base polygon of the primitive
    vfp_avVerticesOnBaseOfPrimitive[ iVtx] = DOUBLE3D( x, 0.0, z);
    angleCt += angle;
  }
}

void CValuesForPrimitive::Write_t(CTStream &strmFile)
{
  strmFile.WriteID_t( CChunkID(VALUES_FOR_PRIMITIVE_VERSION3));

  strmFile << (INDEX) vfp_ptPrimitiveType;
  INDEX ctBaseVtx = vfp_avVerticesOnBaseOfPrimitive.Count();
  strmFile << ctBaseVtx;
  for( INDEX iBaseVtx=0; iBaseVtx<ctBaseVtx; iBaseVtx++)
  {
    strmFile << vfp_avVerticesOnBaseOfPrimitive[iBaseVtx];
  }
  strmFile << vfp_plPrimitive;
  strmFile << (INDEX) vfp_ttTriangularisationType;
  strmFile << vfp_bClosed;
  strmFile << vfp_bDummy;

  INDEX iPrimitiveType = (INDEX) vfp_ptPrimitiveType;
  strmFile << iPrimitiveType;

  strmFile << vfp_bAutoCreateMipBrushes;
  strmFile << vfp_colSectorsColor;
  strmFile << vfp_colPolygonsColor;
  strmFile << vfp_fXMin;
  strmFile << vfp_fXMax;
  strmFile << vfp_fYMin;
  strmFile << vfp_fYMax;
  strmFile << vfp_fZMin;
  strmFile << vfp_fZMax;
  strmFile << vfp_fShearX;
  strmFile << vfp_fShearZ;
  strmFile << vfp_fStretchX;
  strmFile << vfp_fStretchY;
  strmFile << vfp_bLinearStaircases;
  strmFile << vfp_bOuter;
  strmFile << vfp_iSlicesIn360;
  strmFile << vfp_iNoOfSlices;
  strmFile << vfp_fRadius;
  strmFile << vfp_iMeridians;
  strmFile << vfp_iParalels;
  strmFile << vfp_iSlicesPerWidth;
  strmFile << vfp_iSlicesPerHeight;
  strmFile << vfp_iTopShape;
  strmFile << vfp_iBottomShape;

  strmFile << vfp_fAmplitude;
  strmFile << vfp_fnDisplacement;
  strmFile << vfp_fMipStart;
  strmFile << vfp_fMipStep;
}

void CValuesForPrimitive::Read_t(CTStream &strmFile)
{

  CChunkID cidVersion = strmFile.GetID_t();
  if( !((cidVersion == CChunkID(VALUES_FOR_PRIMITIVE_VERSION2)) ||
        (cidVersion == CChunkID(VALUES_FOR_PRIMITIVE_VERSION3))) )
  {
    throw( "Only versions 2 and 3 of primitive value files is supported!");
  }

  INDEX iPrimitiveType;
  strmFile >> iPrimitiveType;
  vfp_ptPrimitiveType = (enum PrimitiveType) iPrimitiveType;

  INDEX ctBaseVtx;
  strmFile >> ctBaseVtx;
  vfp_avVerticesOnBaseOfPrimitive.Clear();
  vfp_avVerticesOnBaseOfPrimitive.New( ctBaseVtx);
  for( INDEX iBaseVtx=0; iBaseVtx<ctBaseVtx; iBaseVtx++)
  {
    strmFile >> vfp_avVerticesOnBaseOfPrimitive[iBaseVtx];
  }
  strmFile >> vfp_plPrimitive;
  INDEX iTriangularisationType;
  strmFile >> iTriangularisationType;
  vfp_ttTriangularisationType = (enum TriangularisationType) iTriangularisationType;
  strmFile >> vfp_bClosed;
  strmFile >> vfp_bDummy;

  INDEX iCSGOperation;
  strmFile >> iCSGOperation;
  vfp_csgtCSGOperation = (enum CSGType) iCSGOperation;

  strmFile >> vfp_bAutoCreateMipBrushes;
  strmFile >> vfp_colSectorsColor;
  strmFile >> vfp_colPolygonsColor;
  strmFile >> vfp_fXMin;
  strmFile >> vfp_fXMax;
  strmFile >> vfp_fYMin;
  strmFile >> vfp_fYMax;
  strmFile >> vfp_fZMin;
  strmFile >> vfp_fZMax;
  strmFile >> vfp_fShearX;
  strmFile >> vfp_fShearZ;
  strmFile >> vfp_fStretchX;
  strmFile >> vfp_fStretchY;
  strmFile >> vfp_bLinearStaircases;
  strmFile >> vfp_bOuter;
  strmFile >> vfp_iSlicesIn360;
  strmFile >> vfp_iNoOfSlices;
  strmFile >> vfp_fRadius;
  strmFile >> vfp_iMeridians;
  strmFile >> vfp_iParalels;
  strmFile >> vfp_iSlicesPerWidth;
  strmFile >> vfp_iSlicesPerHeight;
  strmFile >> vfp_iTopShape;
  strmFile >> vfp_iBottomShape;

  strmFile >> vfp_fAmplitude;
  strmFile >> vfp_fnDisplacement;
  if( cidVersion == CChunkID(VALUES_FOR_PRIMITIVE_VERSION3))
  {
    strmFile >> vfp_fMipStart;
    strmFile >> vfp_fMipStep;
  }
}

void CWorldEditorApp::WriteToIniFileOnEnd(void)
{
  char strIni[ 256];

  // write data that can be saved multiple times to ini file
  WriteToIniFile();

  if( theApp.m_ptdActiveTexture != NULL)
  {
    CTFileName fnTextureForPrimitive( theApp.m_ptdActiveTexture->GetName());
    fnTextureForPrimitive.SetAbsolutePath();
    strcpy( strIni, fnTextureForPrimitive);
    WriteProfileString( L"World editor prefs", L"Default primitive texture", CString(strIni));
  }

  SET_FLOAT( m_fPaintPower);
  INI_WRITE( "Paint power");

  SET_FLOAT( m_fSmoothPower);
  INI_WRITE( "Smooth power");

  SET_INDEX( m_iFilter);
  INI_WRITE( "Current filter");

  SET_FLOAT( m_fPosterizeStep);
  INI_WRITE( "Posterize step");

  SET_INDEX( m_iTerrainGenerationMethod);
  INI_WRITE( "Terrain generation method");

  SET_INDEX( m_iRNDSubdivideAndDisplaceItterations);
  INI_WRITE( "Subdivade and displace itterations");

  SET_FLOAT( m_fNoiseAltitude);  
  INI_WRITE( "Noise altitude");

  SET_STRING( m_fnDistributionNoiseTexture);
  INI_WRITE( "Distribution noise texture");
  
  SET_STRING( m_fnContinousNoiseTexture);
  INI_WRITE( "Continous noise texture");

  SET_INDEX( m_iFBMOctaves);
  INI_WRITE( "FBM Octaves");

  SET_FLOAT( m_fFBMHighFrequencyStep);
  INI_WRITE( "FBM High frequency step");

  SET_FLOAT( m_fFBMStepFactor);
  INI_WRITE( "FBM Step factor");

  SET_FLOAT( m_fFBMMaxAmplitude);
  INI_WRITE( "FBM Max amplitude");

  SET_FLOAT( m_fFBMfAmplitudeDecreaser);
  INI_WRITE( "FBM Amplitude decreaser");

  SET_FLAG( m_bFBMAddNegativeValues);  
  INI_WRITE( "FBM Add negative values");

  SET_FLAG( m_bFBMRandomOffset);  
  INI_WRITE( "FBM Random offset");

  WriteProfileInt(L"World editor", L"Show Tip of the Day", m_bShowTipOfTheDay);
  WriteProfileInt(L"World editor", L"Current Tip of the Day", m_iCurrentTipOfTheDay);
  WriteProfileInt(L"Display modes", L"SED Gfx API", m_iApi);
}

void CWorldEditorApp::WriteToIniFile()
{
  // write values for preferences
  m_Preferences.WriteToIniFile();

  // write to INI file last values used for primitive
  m_vfpPreLast.WriteToIniFile("PreLast");
  m_vfpLast.WriteToIniFile("Last");
  m_vfpCurrent.WriteToIniFile("Current");
  // write to INI last values for each type of primitive
  m_vfpConus.WriteToIniFile("Conus");
  m_vfpTorus.WriteToIniFile("Torus");
  m_vfpStaircases.WriteToIniFile("Staircases");
  m_vfpSphere.WriteToIniFile("Sphere");
  m_vfpTerrain.WriteToIniFile("Terrain");

  // save world and model rendering preferences
  SaveRenderingPreferences();

  // save child configurations
  SaveChildConfigurations();
}

// World editor ini write function for preferences
void CAppPrefs::WriteToIniFile()
{
  char strIni[ 256];

  SET_FLAG( ap_CopyExistingWindowPrefs);
  INI_WRITE( "Copy existing window preferences");
  SET_FLAG( ap_AutoMaximizeWindow);
  INI_WRITE( "Auto maximize window");
  SET_FLAG( ap_SetDefaultColors);
  INI_WRITE( "Set default colors");
  SET_FLAG( ap_AutomaticInfo);
  INI_WRITE( "Automatic info");
  SET_FLAG( ap_UpdateAllways);
  INI_WRITE( "Update allways");
  SET_FLAG( ap_BinaryGrid);
  INI_WRITE( "Binary grid");
  SET_FLAG( ap_bSaveUndoForDelete);
  INI_WRITE( "Save undo for delete");
  SET_FLAG( ap_bAutoColorize);
  INI_WRITE( "Auto colorize primitives");
  SET_FLAG( ap_bShowAllOnOpen);
  INI_WRITE( "Show all on open");
  SET_FLAG( ap_bHideShadowsOnOpen);
  INI_WRITE( "Hide shadows on open");
  SET_FLAG( ap_bAutoUpdateDisplaceMap);
  INI_WRITE( "Auto update displace map");
  
  SET_INDEX( ap_iUndoLevels);
  INI_WRITE( "Undo levels");
  SET_INDEX( ap_iStartupWindowSetup);
  INI_WRITE( "Startup window setup");

  SET_COLOR( ap_DefaultInkColor);
  INI_WRITE( "Ink color");
  SET_COLOR( ap_DefaultPaperColor);
  INI_WRITE( "Paper color");
  SET_COLOR( ap_DefaultSelectionColor);
  INI_WRITE( "Current selection color");
  SET_COLOR( ap_DefaultGridColor);
  INI_WRITE( "Current grid color");

  SET_STRING( ap_strSourceSafeProject);
  INI_WRITE( "Source safe project");

  SET_FLOAT( ap_fDefaultFlyModeSpeed);  
  INI_WRITE( "Default fly mode speed");

  SET_INDEX( ap_iTerrainSelectionVisible);
  INI_WRITE( "Terrain selection visible");
  SET_INDEX( ap_iTerrainSelectionHidden);
  INI_WRITE( "Terrain selection hidden");

  SET_INDEX( ap_iMemoryForTerrainUndo);
  INI_WRITE( "Memory for terrain undo");

  SET_FLAG( ap_bAutoUpdateTerrainDistribution);  
  INI_WRITE( "Auto generate distribution");
}


BOOL CWorldEditorApp::LoadRenderingPreferences()
{
  CTFileName fnRenderingPrefs = CTString("Data\\WEDRenderingPrefs.bin");

  // if rendering preferences file does not exist
  if (!FileExists(fnRenderingPrefs)) {
    // just reset to defaults without a note
    return FALSE;
  }

  // load world and model rendering preferences
  CTFileStream strmFile;
  try
  {
    // open binary file to read rendering preferences
    strmFile.Open_t( fnRenderingPrefs);
    // read file ID
    strmFile.ExpectID_t( CChunkID( "RPRF"));  // rendering preferences
    // check version number
    if( !(CChunkID( VIEW_PREFERENCES_VER) == strmFile.GetID_t()) )
    {
      throw( "Invalid version of rendering preferences, switching to defaults.");
    }
    // read view rendering preferences
    strmFile.Read_t( &m_vpViewPrefs, sizeof( m_vpViewPrefs));
    for(INDEX i=0; i<ARRAYCOUNT(m_vpViewPrefs); i++) {
      m_vpViewPrefs[i].ClearInvalidConfigPointers();
    }
    // read ID for end of rendering prefs
    strmFile.ExpectID_t( CChunkID( "RPED"));  // rendering preferences end
  }
  catch (const char *err_str)
  {
    char achrMessage[ 256];
    sprintf( achrMessage, "%s\nWorld editor's rendering preferences will be switched "
             "to defaults.", err_str);
    AfxMessageBox( CString(achrMessage));
    return FALSE;
  }
  return TRUE;
}

void CWorldEditorApp::SaveRenderingPreferences(void)
{
  // save world and model rendering preferences
  CTFileStream strmFile;
  try
  {
    // open binary file to save rendering preferences
  	CTFileName fnRenderingPrefs = CTString("Data\\WEDRenderingPrefs.bin");
    strmFile.Create_t( fnRenderingPrefs, CTStream::CM_BINARY);
    // write file ID
    strmFile.WriteID_t( CChunkID( "RPRF"));  // child configurations
    // write version number
    strmFile.WriteID_t( CChunkID( VIEW_PREFERENCES_VER));
    // write child configurations array
    strmFile.Write_t( &m_vpViewPrefs, sizeof( m_vpViewPrefs));
    // write ID for end of rendering prefs
    strmFile.WriteID_t( CChunkID( "RPED"));  // rendering preferences end
  }
  catch (const char *err_str)
  {
    AfxMessageBox( CString(err_str));
  }
}

/*
 * Set default values for view preferences
 */
void CChildConfiguration::SetDefaultValues(void)
{
  m_iHorizontalSplitters = 2;
  m_iVerticalSplitters = 2;
  m_fPercentageLeft = 0.480064f;
  m_fPercentageTop = 0.476654f;
  m_bGridOn = TRUE;
  m_vpViewPrefs[ 0] = theApp.m_vpViewPrefs[ 0];
  m_vpViewPrefs[ 1] = theApp.m_vpViewPrefs[ 0];
  m_vpViewPrefs[ 2] = theApp.m_vpViewPrefs[ 0];
  m_vpViewPrefs[ 3] = theApp.m_vpViewPrefs[ 0];
  m_ptProjectionType[ 0] = CSlaveViewer::PT_ISOMETRIC_TOP;
  m_ptProjectionType[ 1] = CSlaveViewer::PT_PERSPECTIVE;
  m_ptProjectionType[ 2] = CSlaveViewer::PT_ISOMETRIC_FRONT;
  m_ptProjectionType[ 3] = CSlaveViewer::PT_ISOMETRIC_RIGHT;
}
void CChildConfiguration::ClearInvalidConfigPointers(void)
{
  m_vpViewPrefs[0].ClearInvalidConfigPointers();
  m_vpViewPrefs[1].ClearInvalidConfigPointers();
  m_vpViewPrefs[2].ClearInvalidConfigPointers();
  m_vpViewPrefs[3].ClearInvalidConfigPointers();
}

BOOL CWorldEditorApp::LoadChildConfigurations(void)
{
  CTFileName fnChildConfigurations = CTString("Data\\WEDChildConfigurations.bin");

  // if child configuration file does not exist
  if (!FileExists(fnChildConfigurations)) {
    // just reset to defaults without a note
    return FALSE;
  }

  // load child configurations
  CTFileStream strmFile;
  try
  {
    // create binary file to receive child configurations
    strmFile.Open_t( fnChildConfigurations);
    // read file ID
    strmFile.ExpectID_t( CChunkID( "CCFG"));  // child configurations
    // check version number
    if( !( CChunkID(CHILD_CONFIGURATION_VER) == strmFile.GetID_t()) )
    {
      throw( "Invalid version of child configurations file, switching to defaults.");
    }
    // clear configurations
    m_ccChildConfigurations->SetDefaultValues();
    // read child configurations array
    strmFile.Read_t( &m_ccChildConfigurations, sizeof( m_ccChildConfigurations));
    // read end of file ID
    strmFile.ExpectID_t( CChunkID( "CCED"));  // end of child configurations ID
  }
  catch (const char *err_str)
  {
    char achrMessage[ 256];
    sprintf( achrMessage, "%s\nSplit window configurations will be switched to defaults.", err_str);
    AfxMessageBox( CString(achrMessage));
    ClearInvalidConfigPointers();
    return FALSE;
  }
  ClearInvalidConfigPointers();
  return TRUE;
}

void CWorldEditorApp::SaveChildConfigurations(void)
{
  ClearInvalidConfigPointers();
  // save child configurations
  CTFileStream strmFile;
  try
  {
  	CTFileName fnChildConfigurations = CTString("Data\\WEDChildConfigurations.bin");
    // create binary file to receive child configurations
    strmFile.Create_t( fnChildConfigurations, CTStream::CM_BINARY);
    // write file ID
    strmFile.WriteID_t( CChunkID( "CCFG"));  // child configurations
    // write version number
    strmFile.WriteID_t( CChunkID( CHILD_CONFIGURATION_VER));
    // write child configurations array
    strmFile.Write_t( &m_ccChildConfigurations, sizeof( m_ccChildConfigurations));
    // write end of file ID
    strmFile.WriteID_t( CChunkID( "CCED"));  // end of child configurations ID
  }
  catch (const char *err_str)
  {
    AfxMessageBox( CString(err_str));
  }
}

// clear possibly invalid pointers in view configurations
void CWorldEditorApp::ClearInvalidConfigPointers(void)
{
  for(INDEX i=0; i<ARRAYCOUNT(m_ccChildConfigurations); i++) {
    m_ccChildConfigurations[i].ClearInvalidConfigPointers();
  }
}

int CWorldEditorApp::ExitInstance()
{
  // cleanup game library
  _pGameGUI->End();

  // delete clipboard file
  RemoveFile( CTString("Temp\\ClipboardWorld.wld"));

  WriteToIniFileOnEnd();
  WriteDefaultPolygonValues();

  // release entity marker texture
  if( m_ptdEntityMarkerTexture != NULL)
  {
    _pTextureStock->Release( m_ptdEntityMarkerTexture);
    m_ptdEntityMarkerTexture = NULL;
  }
  // and entity marker model data
  if( m_pEntityMarkerModelData != NULL)
  {
    _pModelStock->Release( m_pEntityMarkerModelData);
    delete m_pEntityMarkerModelObject;
    m_pEntityMarkerModelObject = NULL;
  }

  // release portal marker texture
  if( m_ptdPortalMarkerTexture != NULL)
  {
    _pTextureStock->Release( m_ptdPortalMarkerTexture);
    m_ptdPortalMarkerTexture = NULL;
  }
  // and portal marker model data
  if( m_pPortalMarkerModelData != NULL)
  {
    _pModelStock->Release( m_pPortalMarkerModelData);
    delete m_pPortalMarkerModelObject;
    m_pPortalMarkerModelObject = NULL;
  }

  // release empty brush texture
  if( m_ptdEmptyBrushTexture != NULL)
  {
    _pTextureStock->Release( m_ptdEmptyBrushTexture);
    m_ptdEmptyBrushTexture = NULL;
  }
  // and empty brush model data
  if( m_pEmptyBrushModelData != NULL)
  {
    _pModelStock->Release( m_pEmptyBrushModelData);
    delete m_pEmptyBrushModelObject;
    m_pEmptyBrushModelObject = NULL;
  }

  // release range sphere texture
  if( m_ptdRangeSphereTexture != NULL)
  {
    _pTextureStock->Release( m_ptdRangeSphereTexture);
    m_ptdRangeSphereTexture = NULL;
  }
  // and range sphere model data
  if( m_pRangeSphereModelData != NULL)
  {
    _pModelStock->Release( m_pRangeSphereModelData);
    delete m_pRangeSphereModelObject;
    m_pRangeSphereModelObject = NULL;
  }

  // release angle3d texture
  if( m_ptdAngle3DTexture != NULL)
  {
    _pTextureStock->Release( m_ptdAngle3DTexture);
    m_ptdAngle3DTexture = NULL;
  }
  // and angle3d model data
  if( m_pAngle3DModelData != NULL)
  {
    _pModelStock->Release( m_pAngle3DModelData);
    delete m_pAngle3DModelObject;
    m_pAngle3DModelObject = NULL;
  }

  // release bounding box texture
  if( m_ptdBoundingBoxTexture != NULL)
  {
    _pTextureStock->Release( m_ptdBoundingBoxTexture);
    m_ptdBoundingBoxTexture = NULL;
  }
  // and range bounding box model data
  if( m_pBoundingBoxModelData != NULL)
  {
    _pModelStock->Release( m_pBoundingBoxModelData);
    delete m_pBoundingBoxModelObject;
    m_pBoundingBoxModelObject = NULL;
  }

  // release error texture object
  if( m_ptoError != NULL)
  {
    delete m_ptoError;
  }
  // release error texture
  if( m_ptdError != NULL)
  {
    _pTextureStock->Release( m_ptdError);
  }
  // release icons tray texture
  if( m_ptdIconsTray != NULL)
  {
    _pTextureStock->Release( m_ptdIconsTray);
  }

  // release orientation icons
  if( m_pViewIconsTD != NULL)
  {
    _pTextureStock->Release( m_pViewIconsTD);
  }

  // release default primitive texture
  if( m_ptdActiveTexture != NULL)
  {
    _pTextureStock->Release( m_ptdActiveTexture);
  }

  /*
  FORDELETELIST( CDisplayMode, dm_Node, m_AvailableModes, litDM)
  {
    delete &litDM.Current();
  } */

  FORDELETELIST( CPrimitiveInHistoryBuffer, pihb_lnNode, theApp.m_lhPrimitiveHistory, itPrim)
  {
    delete &itPrim.Current();
  }

  delete m_pbpoClipboardPolygon;

  // end entire engine
  SE_EndEngine();

  int iResult = CWinApp::ExitInstance();
  return iResult;
}


void CWorldEditorApp::OnFilePreferences()
{
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  CDlgPreferences dlg;

  dlg.m_dmFullScreen  = m_dmFullScreen;
  dlg.m_gatFullScreen = m_gatFullScreen;
  if( dlg.DoModal() == IDOK)
  { // remember new full screen mode
    m_dmFullScreen  = dlg.m_dmFullScreen;
    m_gatFullScreen = dlg.m_gatFullScreen;
  }
}

BOOL CWorldEditorApp::OnIdle(LONG lCount)
{
  // if game is on
  if( _pInput->IsInputEnabled())
  {
    ASSERT(FALSE); //!!!!!
    return FALSE;
  }
  // if game is on, everithing else is off (don't execute this)
  else if( !(lCount&0xF))
  {
    POSITION pos = m_pDocTemplate->GetFirstDocPosition();
    while (pos!=NULL) {
      CWorldEditorDoc *pdocCurrent = (CWorldEditorDoc *)m_pDocTemplate->GetNextDoc(pos);
      pdocCurrent->OnIdle();
    }

    // remember the world pointer of current document
    CWorldEditorView *pvCurrent = GetActiveView();
    if (pvCurrent!=NULL) {
      CWorldEditorDoc *pdocCurrent = pvCurrent->GetDocument();
      if (pdocCurrent!=NULL) {
        _pShell->SetCurrentWorld(&pdocCurrent->m_woWorld);
      }
    }

    ((CMainFrame *)m_pMainWnd)->OnIdle( lCount);
  }
  BOOL bLMB = (GetKeyState( VK_LBUTTON)&0x8000) != 0;
  BOOL bRMB = (GetKeyState( VK_RBUTTON)&0x8000) != 0;

  BOOL bResult=CWinApp::OnIdle(lCount);
  return bResult||bLMB||bRMB;
}

// force all documents to repaint thir views
void CWorldEditorApp::RefreshAllDocuments( void)
{
  POSITION pos = m_pDocTemplate->GetFirstDocPosition();

  while (pos!=NULL)
  {
    CWorldEditorDoc *pdocCurrent = (CWorldEditorDoc *)m_pDocTemplate->GetNextDoc(pos);
    pdocCurrent->UpdateAllViews( NULL);
  }
}

// sets new active texture for primitive's default material
void CWorldEditorApp::SetNewActiveTexture( CTFileName fnFullTexName)
{
  CMainFrame *pMainFrame = (CMainFrame *)m_pMainWnd;
  // to hold new texture
  CTextureData *pdtNewTexture = NULL;
  // to hold short texture name
  CTFileName fnTexName = fnFullTexName;
  // try to
  try
  {
    // obtain the new texture
    fnTexName.RemoveApplicationPath_t();
    pdtNewTexture = _pTextureStock->Obtain_t( fnTexName);
  }
  // if failed
  catch (const char *err_str)
  {
    pdtNewTexture = _pTextureStock->Obtain_t( CTFILENAME("Textures\\Editor\\Default.tex") );
    (void)err_str;
    // report error
    //AfxMessageBox( CString(err_str));
    //return;
  }
  ASSERT(pdtNewTexture != NULL);
  // if there is old texture
  if (m_ptdActiveTexture!=NULL) {
    // release it
    _pTextureStock->Release(m_ptdActiveTexture);
  }
  // remember the new texture
  m_ptdActiveTexture = pdtNewTexture;
  // if info frame exists, update it
  if( pMainFrame->m_pInfoFrame != NULL)
  {
    CInfoSheet *pSheet = pMainFrame->m_pInfoFrame->m_pInfoSheet;
    // redraw info frame
    pMainFrame->m_pInfoFrame->Invalidate(FALSE);
    // info's page is pg global
    if( (pSheet->GetActivePage() == &pSheet->m_PgGlobal) )
    {
      // update texture description
      pMainFrame->m_pInfoFrame->m_pInfoSheet->m_PgGlobal.UpdateData( FALSE);
    }

    // get active view
    CWorldEditorView *pWorldEditorView = theApp.GetActiveView();
    // get active document
    CWorldEditorDoc* pDoc = theApp.GetActiveDocument();
    // if view and second layer exist (CSG is on), and primitive mode is on
    if( (pWorldEditorView != NULL) &&
        (pDoc->m_pwoSecondLayer != NULL) &&
        (pDoc->m_bPrimitiveMode) )
    {
      // apply texture change for CSG primitive
      pSheet->m_PgPrimitive.ApplySCGChange();
    }
  }
}

// opens directory wich contains given item
void CWorldEditorApp::FindItemInBrowser( CTFileName fnItemFileName)
{
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  pMainFrame->m_Browser.SelectItemDirectory( fnItemFileName);
}

void CWorldEditorApp::TexturizeSelection(void)
{
  // get active document
  CWorldEditorDoc* pDoc = theApp.GetActiveDocument();

  // if it exists
  if( pDoc != NULL)
  {
    // try to apply new texture to current selection
    try
    {
      // if polygon mode
      if( (pDoc->m_iMode == POLYGON_MODE) && (m_ptdActiveTexture != NULL) )
      {
        // get name from serial object
        CTFileName fnTextureName = m_ptdActiveTexture->GetName();
        pDoc->PasteTextureOverSelection_t( fnTextureName);
      }
    }
    // if failed
    catch (const char *err_str)
    {
      // report error
      AfxMessageBox( CString(err_str));
      return;
    }
  }
}

void CWorldEditorApp::OnFileOpen()
{
  // call file requester for opening documents
  CDynamicArray<CTFileName> afnOpenedWorlds;
  _EngineGUI.FileRequester( "Choose worlds to open", FILTER_WLD FILTER_ALL FILTER_END,
    "Open world directory", "Worlds\\", "", &afnOpenedWorlds);
  FOREACHINDYNAMICARRAY( afnOpenedWorlds, CTFileName, itWorld)
  {
    // try to load document
    m_pDocTemplate->OpenDocumentFile( CString(_fnmApplicationPath+itWorld.Current()));
  }
}

void CWorldEditorApp::OnConvertWorlds()
{
  _pShell->Execute( CTString("con_bNoWarnings=1;"));

  // call file requester for list containing worlds to convert
  CTFileName fnFileList = _EngineGUI.FileRequester( "Choose list file for conversion",
                          FILTER_LST FILTER_TXT FILTER_ALL FILTER_END, "List files directory", "");
  if( fnFileList == "") return;

  INDEX ctLines = 0;
  char achrLine[256];
  CTFileStream fsFileList;

  // count lines in list file
  try {
    fsFileList.Open_t( fnFileList);
    while( !fsFileList.AtEOF()) {
      fsFileList.GetLine_t( achrLine, 256);
      // increase counter only for lines that are not blank
      if( achrLine != "") ctLines++;
    }
    fsFileList.Close();
  }
  // if the list file can't be opened
  catch (const char *strError) {
    _pShell->Execute( CTString("con_bNoWarnings=0;"));
    WarningMessage( "Error reading list file: %s", strError);
    return;
  }

  // process list file
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  CDlgProgress dlgProgressDialog( pMainFrame, TRUE);
  dlgProgressDialog.Create( IDD_PROGRESS_DIALOG); // create progress dialog
  dlgProgressDialog.SetWindowText( L"Convert files");
  dlgProgressDialog.ShowWindow( SW_SHOW); // show progress window
  dlgProgressDialog.CenterWindow(); // center window
  dlgProgressDialog.m_ctrlProgres.SetRange( 0, (short)ctLines); // set progress range

  // prepare error file
  CTFileStream fsErrorFile;
  CTFileName   fnErrorFile = fnFileList.NoExt() + ".err";
  // reopen list file
  fsFileList.Open_t( fnFileList);

  INDEX iCurrent = 0;
  BOOL bConvertError = FALSE;
  CTString strReport;
  CTFileName fnmFile, fnmExt, fnmFileFull;
  // loop thru lines
  while( !fsFileList.AtEOF())
  {
    try {
      if( dlgProgressDialog.m_bCancelPressed) break;

      // read one line from list file
      fsFileList.GetLine_t( achrLine, 256);
      // ignore blank lines
      if( achrLine == "") continue;

      // set message and progress position
      char achrProgressMessage[256];
      sprintf( achrProgressMessage, "Converting files ... (%d / %d)", iCurrent+1, ctLines);
      dlgProgressDialog.SetProgressMessageAndPosition( achrProgressMessage, iCurrent+1);

      // convert needed type of object
      fnmFile = CTString( achrLine);
      fnmFileFull = _fnmApplicationPath+fnmFile;
      fnmExt  = fnmFile.FileExt();
      struct _stat    FileStat;
      struct _utimbuf FileTime;

      // convert world?
      if( fnmExt == ".wld")
      {
        // load the world
        CWorld woWorld;
        woWorld.Load_t(fnmFile);
        // reinitialize all entities
        woWorld.ReinitializeEntities();
        // flush stale caches
        _pShell->Execute("FreeUnusedStock();");
        // show all sectors and entities
        woWorld.ShowAllSectors();
        woWorld.ShowAllEntities();
        // recalculate shadows on all brush polygons in the world
        woWorld.DiscardAllShadows();
        woWorld.CalculateDirectionalShadows();
        woWorld.CalculateNonDirectionalShadows();
        // get original file date
        if( _stat( fnmFileFull, &FileStat)) throw "Error getting file date.";
        // save world in new format
        woWorld.Save_t(fnmFile);
        _pShell->Execute( CTString( "bReinitializeShadowLayers=0;"));
        woWorld.Clear();
        // revert to original file date
        FileTime.actime  = FileStat.st_atime;
        FileTime.modtime = FileStat.st_mtime;
        if( _utime( fnmFileFull, &FileTime)) throw "Error setting file date.";
      }
      // convert texture?
      else if( fnmExt == ".tex" || fnmExt == ".tbn")
      {
        // convert textures (but keep the file date)
        CTextureData tdTex;
        tdTex.Load_t( fnmFile);
        // if old texture has been loaded
        if( tdTex.td_ulFlags & TEX_WASOLD) {
          // cannost convert mangled textures
          if( tdTex.td_ptegEffect==NULL && tdTex.IsModified()) throw( TRANS("Cannot write texture that has modified frames."));
          // get original file date
          if( _stat( fnmFileFull, &FileStat)) throw "Error getting file date.";
          // save texture in new format
          tdTex.Save_t( fnmFile);
          // revert to original file date
          FileTime.actime  = FileStat.st_atime;
          FileTime.modtime = FileStat.st_mtime;
          if( _utime( fnmFileFull, &FileTime)) throw "Error setting file date.";
        }
      }
      // convert WHAT?
      else if( fnmFile=="")
      {
        continue;
      }
      else
      {
        ThrowF_t( "Unsupported file format: %s", (CTString&)fnmExt);
      }
    }

    // if file can't be opened
    catch (const char *strError) {
      try {
        // on first error
        if( !bConvertError) {
          // (re)create error file
          fsErrorFile.Create_t( fnErrorFile);
          bConvertError = TRUE;
        }
        // report error to file
        strReport.PrintF( "File: %s\nHad error: %s\n", (CTString)fnmFile, strError);

        fsErrorFile.PutString_t( strReport);
        fsErrorFile.PutLine_t( "-----------------------------------------------");
      }
      // here should be no errors,
      catch (const char *strError) {
        // otherwise ...
        (void) strError;
        WarningMessage( "Fatal error occured while working with error file!");
        break;
      }
    }

    // advance to next line
    iCurrent++;
  }

  fsFileList.Close();
  dlgProgressDialog.DestroyWindow(); // destroy progress dialog
  _pShell->Execute( CTString("con_bNoWarnings=0;"));

  // report error situation (if any)
  if( bConvertError) {
    fsErrorFile.PutLine_t( "DONE.");
    fsErrorFile.Close();
    WarningMessage( "There were some errors in conversion. They are listed in file:\n %s", fnErrorFile);
  }
}

CEntity *CWorldEditorApp::CreateWorldBaseEntity(CWorld &woWorld, BOOL bZoning,
                                                CPlacement3D plWorld/*=CPlacement3D(FLOAT3D(0,0,0),ANGLE3D(0,0,0))*/)
{
  CEntity *penwb;
  // try to
  try
  {
    // create world base entity
    penwb = woWorld.CreateEntity_t(plWorld, CTFILENAME("Classes\\WorldBase.ecl"));
  }
  // catch and
  catch (const char *err_str)
  {
    // report errors
    AfxMessageBox( CString(err_str));
    return NULL;
  }
  // prepare the entity
  penwb->Initialize();
  
  if( bZoning)
  {
    EFirstWorldBase eFirstWorldBase;
    penwb->SendEvent( eFirstWorldBase);
    CEntity::HandleSentEvents();
  }
  return penwb;
}

BOOL CWorldEditorApp::Add3DObject(CWorldEditorDoc *pDoc, CEntity *penwb, CTFileName fnFile, BOOL bAdd)
{
  CObject3D o3d;
  // temporary world
  CWorld woWorld;
  CEntity *penwb2=CreateWorldBaseEntity(woWorld, FALSE);
  if( penwb2==NULL)
  {
    return FALSE;
  }

  // try to
  try
  {
    // load 3D object
    FLOATmatrix3D mStretch;
    mStretch.Diagonal(1.0f);
    if (fnFile.FileExt()==".obj") { // Maya Obj has different orientation
      mStretch.Diagonal(FLOAT3D(-1.0f, 1.0f, -1.0f));
    }
    o3d.LoadAny3DFormat_t( fnFile, mStretch);

    FOREACHINDYNAMICARRAY(o3d.ob_aoscSectors, CObjectSector, itosc)
    {
      // for each material in object3D
      FOREACHINDYNAMICARRAY(itosc->osc_aomtMaterials, CObjectMaterial, itom)
      {
        if( !FileExists(itom->omt_Name))
        {
          itom->omt_Name = "Textures\\Editor\\Default.tex";
        }
      }
    }
    // create world base's brush from object 3D
    CBrush3D *pbr = penwb2->GetBrush();
    pbr->FromObject3D_t( o3d);
    pbr->CalculateBoundingBoxes();

    CPlacement3D plDummy;
    plDummy.pl_PositionVector = FLOAT3D(0.0f,0.0f,0.0f);
    plDummy.pl_OrientationAngle = ANGLE3D(0,0,0);
    // if should apply CSG add
    if(bAdd)
    {
      penwb->en_pwoWorld->CSGAdd(*penwb, woWorld, *penwb2, plDummy);
    }
    // if should perform join layers
    else
    {
      // copy entities
      penwb->en_pwoWorld->CopyEntities( woWorld, woWorld.wo_cenEntities, pDoc->m_selEntitySelection, plDummy);
    }
  }
  // catch and
  catch (const char *err_str)
  {
    // report errors
    AfxMessageBox( CString(err_str));
    return FALSE;
  }
  return TRUE;
}

/*
 * Import 3d object(s), create new document
 */
void CWorldEditorApp::OnImport3DObject()
{
  // try to load document
  CWorldEditorDoc *pDoc = (CWorldEditorDoc *) m_pDocTemplate->CreateNewDocument();

  // create the World entity
  CEntity *pwb=CreateWorldBaseEntity(pDoc->m_woWorld, TRUE);
  if( pwb==NULL)
  {
    delete pDoc;
    return;
  }

  // create 3d objects
  INDEX ctImported=Insert3DObjects(pDoc);
  if( ctImported==0)
  {
    delete pDoc;
    return;
  }

  // finish creating document
 	if (pDoc == NULL)
	{
		TRACE0("CDocTemplate::CreateNewDocument returned NULL.\n");
		AfxMessageBox(AFX_IDP_FAILED_TO_CREATE_DOC);
		return;
	}
	ASSERT_VALID(pDoc);

  BOOL bAutoDelete = pDoc->m_bAutoDelete;
	pDoc->m_bAutoDelete = FALSE;   // don't destroy if something goes wrong
	CFrameWnd* pFrame = m_pDocTemplate->CreateNewFrame(pDoc, NULL);
	pDoc->m_bAutoDelete = bAutoDelete;
	if (pFrame == NULL)
	{
		AfxMessageBox(AFX_IDP_FAILED_TO_CREATE_DOC);
		delete pDoc;       // explicit delete on error
		return;
	}
	ASSERT_VALID(pFrame);

  pDoc->SetModifiedFlag();
  // set document name and don't add it into MRU
  //pDoc->SetPathName( _fnmApplicationPath+"Worlds\\"+"Untitled.wld", FALSE);
  //pDoc->SetTitle( fn3D.FileName() + ".wld");
	m_pDocTemplate->InitialUpdateFrame(pFrame, pDoc, TRUE);
  pDoc->SetModifiedFlag( TRUE);
}

INDEX CWorldEditorApp::Insert3DObjects(CWorldEditorDoc *pDoc)
{
  INDEX ctInserted=0;
  CDynamicArray<CTFileName> afnFiles;
  CTFileName fn3D = _EngineGUI.FileRequester( "Import 3D object series",
    FILTER_3DOBJ FILTER_LWO FILTER_OBJ FILTER_3DS FILTER_ALL FILTER_END,
    "Import 3D object directory", "", "", &afnFiles);

  if( afnFiles.Count() == 0) return 0;

  // get first file (when strings are sorted)
  CTString strMin="a";
  ((char *)(const char *) strMin)[0]=char(255);
  CTFileName *pfn=NULL;
  {FOREACHINDYNAMICARRAY(afnFiles, CTFileName, itfn)
  {
    CTString str=itfn->FileName();
    if( strcmp(str, strMin)<0)
    {
      strMin=str;
      pfn=&*itfn;
    }
  }}
  ASSERT(pfn!=NULL);
  if(pfn==NULL) return ctInserted;
  
  // create main World entity
  CEntity *pwb=CreateWorldBaseEntity(pDoc->m_woWorld, TRUE);
  if( pwb==NULL)
  {
    return 0;
  }
  // add first 3D file as zoning base
  if( !Add3DObject(pDoc, pwb, *pfn, TRUE))
  {
    return 0;
  }
  ctInserted++;

  // remove added file from dynamic container
  afnFiles.Delete(pfn);
  // add other files
  {FOREACHINDYNAMICARRAY(afnFiles, CTFileName, itfn)
  {
    CTString &str=*itfn;
    CTString strName=itfn->FileName();
    if( ((char *)(const char *)strName)[strlen(strName)-1] == 'E')
    {
      // join layers
      Add3DObject(pDoc, pwb, *itfn, FALSE);
    }
    else
    {
      // CSG add
      Add3DObject(pDoc, pwb, *itfn, TRUE);
    }
    ctInserted++;
  }}
  return ctInserted;
}

CWorldEditorDoc* CWorldEditorApp::GetLastActivatedDocument(void)
{
  return m_pLastActivatedDocument;
}

void CWorldEditorApp::ActivateDocument(CWorldEditorDoc *pDocToActivate)
{
  // remember document to be activated as last activated
  m_pLastActivatedDocument = pDocToActivate;
}

CWorldEditorDoc *CWorldEditorApp::GetDocument()
{
  // obtain current view ptr
  CWorldEditorView *pWorldEditorView = GetActiveView();
  // if view does not exist, return
  if( pWorldEditorView == NULL)
  {
    return NULL;
  }
  // obtain document ptr
  CWorldEditorDoc *pDoc = pWorldEditorView->GetDocument();
  // return it
  return pDoc;
}

void CWorldEditorApp::OnFileNew()
{
  CWinApp::OnFileNew();
  // obtain new document ptr
  CWorldEditorDoc *pDoc = GetDocument();

  CTFileName fnDefaultBcg = CTString(CStringA(theApp.GetProfileString(
    L"World editor prefs", L"Default background picture",
    CString("Textures\\Editor\\Default.tex"))));

  try
  {
//!!!!    pDoc->m_woWorld.SetBackgroundTexture_t( fnDefaultBcg);
  }
  catch (const char *strError)
  {
    (void) strError;
  }

  try
  {
//!!!!    pDoc->m_woWorld.SetBackgroundTexture_t( CTString("Textures\\Editor\\Default.tex"));
  }
  catch (const char *strError)
  {
    AfxMessageBox( CString(strError));
  }

  char chrColor[ 16];
  COLOR colBackground;
  // obtain background color form INI file
  strcpy( chrColor, CStringA(theApp.GetProfileString( L"World editor prefs",
                                             L"Default background color", L"0XFF000000")));
  sscanf( chrColor, "0X%08x", &colBackground);
  // set background color to color button
  pDoc->m_woWorld.SetBackgroundColor( colBackground);
  pDoc->m_woWorld.SetDescription( "No mission description");
  pDoc->SetModifiedFlag( TRUE);

  CString strOpenPath;
  strOpenPath = CStringA(theApp.GetProfileString(L"World editor", L"Open directory", L""));
  // set default document's name and don't set it into MRU
  pDoc->SetPathName( strOpenPath + "Untitled.wld", FALSE);
  pDoc->SetTitle( L"Untitled.wld");
}

int CWorldEditorApp::Run()
{
	int iResult;
  CTSTREAM_BEGIN {
    iResult=CWinApp::Run();
  } CTSTREAM_END;
	return iResult;
}

BOOL CWorldEditorApp::PreTranslateMessage(MSG* pMsg)
{
	return CWinApp::PreTranslateMessage(pMsg);
}

CTString CWorldEditorApp::GetNameForVirtualTreeNode( CVirtualTreeNode *pvtnNode)
{
  ASSERT( pvtnNode != NULL);
  // start from selected directory
  CVirtualTreeNode *pvtnCurrentDir = pvtnNode;
  // reset curently opened virtual directory name
  CTString strResult = "";
  // compile full path name
  while( pvtnCurrentDir->vnt_pvtnParent != NULL)
  {
    strResult = pvtnCurrentDir->vtn_strName+"\\"+ strResult;
    pvtnCurrentDir = pvtnCurrentDir->vnt_pvtnParent;
  }
  // if given is root directory
  if( strResult == "")
  {
    strResult = "<ROOT>";
  }
  return strResult;
}

void CWorldEditorApp::OnDecadicGrid()
{
  m_bDecadicGrid = !m_bDecadicGrid;
  // refresh all documents
  RefreshAllDocuments();
}

void CWorldEditorApp::OnUpdateDecadicGrid(CCmdUI* pCmdUI)
{
  pCmdUI->SetCheck( m_bDecadicGrid);
}

void CCustomToolTip::ManualOn( PIX pixManualX, PIX pixManualY, TTCFunction_type *pCallBack, void *pThis)
{
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  cct_pCallback = pCallBack;
  cct_pThis = pThis;

  if( pMainFrame->m_pwndToolTip != NULL)
  {
    ManualOff();
  }

  pMainFrame->ManualToolTipOn( pixManualX, pixManualY);
}

void CCustomToolTip::ManualOff( void)
{
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  pMainFrame->m_pwndToolTip->ManualOff();
  pMainFrame->m_pwndToolTip = NULL;
}

void CCustomToolTip::ManualUpdate(void)
{
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  pMainFrame->ManualToolTipUpdate();
}

void CCustomToolTip::MouseMoveNotify( HWND hwndCaller, ULONG ulTime, TTCFunction_type *pCallBack, void *pThis)
{
  cct_hwndCaller = hwndCaller;
  cct_pCallback = pCallBack;
  cct_pThis = pThis;

  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  pMainFrame->KillTimer( 0);
  pMainFrame->SetTimer( 0, ulTime, NULL);
}

void CWorldEditorApp::OnSetAsDefault() 
{
  CWorldEditorDoc *pDoc = GetDocument();
  ASSERT( pDoc->m_selPolygonSelection.Count() == 1);
  
  pDoc->m_selPolygonSelection.Lock();
  m_pbpoPolygonWithDeafultValues->CopyPropertiesWithoutTexture( pDoc->m_selPolygonSelection[0]);
  pDoc->m_selPolygonSelection.Unlock();
}

void CWorldEditorApp::ReadDefaultPolygonValues() 
{
  char strIni[ 256];
  INI_READ( "Default polygon flags", "0");
  GET_INDEX( m_pbpoPolygonWithDeafultValues->bpo_ulFlags);
  INI_READ( "Default polygon shadow color", "0XFFFFFFFF");
  GET_COLOR( m_pbpoPolygonWithDeafultValues->bpo_colShadow);
  INI_READ( "Default polygon surface", "0");
  GET_INDEX( m_pbpoPolygonWithDeafultValues->bpo_bppProperties.bpp_ubSurfaceType);
  INI_READ( "Default polygon illumination", "0");
  GET_INDEX( m_pbpoPolygonWithDeafultValues->bpo_bppProperties.bpp_ubIlluminationType);
  INI_READ( "Default polygon blend", "1");
  GET_INDEX( m_pbpoPolygonWithDeafultValues->bpo_bppProperties.bpp_ubShadowBlend);
  INI_READ( "Default polygon mirror", "0");
  GET_INDEX( m_pbpoPolygonWithDeafultValues->bpo_bppProperties.bpp_ubMirrorType);
  INI_READ( "Default polygon cluster size", "2");
  GET_INDEX( m_pbpoPolygonWithDeafultValues->bpo_bppProperties.bpp_sbShadowClusterSize);
}

void CWorldEditorApp::WriteDefaultPolygonValues() 
{
  char strIni[ 256];
  SET_INDEX( m_pbpoPolygonWithDeafultValues->bpo_ulFlags);
  INI_WRITE( "Default polygon flags");
  SET_COLOR( m_pbpoPolygonWithDeafultValues->bpo_colShadow);
  INI_WRITE( "Default polygon shadow color");
  SET_INDEX( m_pbpoPolygonWithDeafultValues->bpo_bppProperties.bpp_ubSurfaceType);
  INI_WRITE( "Default polygon surface");
  SET_INDEX( m_pbpoPolygonWithDeafultValues->bpo_bppProperties.bpp_ubIlluminationType);
  INI_WRITE( "Default polygon illumination");
  SET_INDEX( m_pbpoPolygonWithDeafultValues->bpo_bppProperties.bpp_ubShadowBlend);
  INI_WRITE( "Default polygon blend");
  SET_INDEX( m_pbpoPolygonWithDeafultValues->bpo_bppProperties.bpp_ubMirrorType);
  INI_WRITE( "Default polygon mirror");
  SET_INDEX( m_pbpoPolygonWithDeafultValues->bpo_bppProperties.bpp_sbShadowClusterSize);
  INI_WRITE( "Default polygon cluster size");
}

void CWorldEditorApp::OnHelpShowTipOfTheDay() 
{
  CDlgTipOfTheDay dlgTips;
  dlgTips.DoModal();
  m_bShowTipOfTheDay = dlgTips.m_bShowTipsAtStartup;
}

void FindEmptyBrushes( void)
{
  // remember the world pointer of current document
  CWorldEditorView *pvCurrent = theApp.GetActiveView();
  if (pvCurrent!=NULL)
  {
    CWorldEditorDoc *pDoc = pvCurrent->GetDocument();
    if (pDoc!=NULL)
    {
      // for each entity in the world
      FOREACHINDYNAMICCONTAINER(pDoc->m_woWorld.wo_cenEntities, CEntity, iten) {
        FLOAT3D vPos = iten->GetPlacement().pl_PositionVector;
        // if it is brush entity
        if (iten->en_RenderType == CEntity::RT_BRUSH) {
          INDEX iMip = 0;
          // for each mip in its brush
          FOREACHINLIST(CBrushMip, bm_lnInBrush, iten->en_pbrBrush->br_lhBrushMips, itbm)
          {
            if( itbm->bm_abscSectors.Count() == 0)
            {
              CPrintF("Found brush named %s, without sectors in mip %d at coordinates: (%g, %g, %g)\n",
                iten->GetName(), iMip, vPos(1), vPos(2), vPos(3));
            }
            iMip++;
          }
        }
      }
    }
  }
}

void CWorldEditorApp::WinHelp(DWORD dwData, UINT nCmd) 
{
	// TODO: Add your specialized code here and/or call the base class

  if (nCmd == HELP_CONTEXT) {
    DisplayHelp(CTFILENAME("Help\\SeriousEditorContext.hlk"), HH_HELP_CONTEXT, dwData);
  } else {
  	CWinApp::WinHelp(dwData, nCmd);
  }
}

void CWorldEditorApp::DisplayHelp(const CTFileName &fnHlk, UINT uCommand, DWORD dwData)
{
  CTString strHelpPath;
  BOOL bHlkFound = TRUE;
  try
  {
    strHelpPath.Load_t(fnHlk.NoExt()+".hlk");
  }
  catch (const char *strError)
  {
    (void) strError;
    if (fnHlk.FileExt()==".ecl") {
      WarningMessage("No help available for class: %s", fnHlk.FileName());
    }
    bHlkFound = FALSE;
  }
  
  // extract prefix
  CTString strHelpFormatID = strHelpPath;
  strHelpFormatID.OnlyFirstLine();
  strHelpPath.RemovePrefix(strHelpFormatID);
  strHelpPath.DeleteChar(0);
  strHelpPath.OnlyFirstLine();

  if( bHlkFound)
  {
    if( strHelpFormatID=="HTM" || strHelpFormatID=="HTML" || strHelpFormatID=="TXT")
    {
      // obtain iexplore path
      CTString strKey = "HKEY_CLASSES_ROOT\\.htm\\";
      CTString strString;
      REG_GetString(strKey, strString);
      strKey = "HKEY_CLASSES_ROOT\\"+strString+"\\shell\\open\\command\\";
      REG_GetString(strKey, strString);

      // now extract file path between two "
      char aExePath[PATH_MAX];
      sscanf( strString, "\"%1024[^\"]\"", aExePath);

      CTString strCommand = "\""+CTString(aExePath)+"\"";
      CTString strInputParam = "\""+_fnmApplicationPath+strHelpPath+"\"";
      const char *argv[4];
      argv[0] = strCommand;
      argv[1] = strInputParam;
      argv[2] = NULL;
      _spawnvp(_P_NOWAIT, aExePath, argv);
      return;
    }
    else if( strHelpFormatID=="CHM")
    {
      HtmlHelp(dwData);
      //HtmlHelp(NULL, 
      //  _fnmApplicationPath+strHelpPath, uCommand, dwData);
      return;
    }
    else
    {
      WarningMessage("Expected TXT, HTM, HTML, or CHM help format indentifier.");
    }
  }
  HtmlHelp(dwData);
  //HtmlHelp(NULL, 
  //  _fnmApplicationPath+"Help\\ToolsHelp.chm::/SeriousEditor/Overview.htm", uCommand, dwData);
}

CEntity *GetTerrainEntity(void)
{
  CTerrain *ptTerrain=GetTerrain();
  if(ptTerrain!=NULL)
  {
    return ptTerrain->tr_penEntity;
  }
  return NULL;
}

CTerrain *GetTerrain(void)
{
  CWorldEditorDoc* pDoc = theApp.GetActiveDocument();
  if(pDoc==NULL) return NULL;
  return pDoc->m_ptrSelectedTerrain;
}

CTerrainLayer *GetLayer(INDEX iLayer)
{
  CTerrain *ptTerrain=GetTerrain();
  if(ptTerrain==NULL) return NULL;
  if(!(ptTerrain->tr_atlLayers.Count()>0) || iLayer>=ptTerrain->tr_atlLayers.Count()) return NULL;
  return &ptTerrain->tr_atlLayers[iLayer];
}

CTerrainLayer *GetLayer(void)
{
  CTerrain *ptTerrain=GetTerrain();
  if(ptTerrain==NULL) return NULL;
  if(!(ptTerrain->tr_atlLayers.Count()>0)) return NULL;
  if(ptTerrain->tr_iSelectedLayer>=ptTerrain->tr_atlLayers.Count())
  {
    ptTerrain->tr_iSelectedLayer=0;
  }
  return &ptTerrain->tr_atlLayers[ptTerrain->tr_iSelectedLayer];
}

INDEX GetLayerIndex(void)
{
  CTerrain *ptTerrain=GetTerrain();
  if(ptTerrain==NULL) return 0;
  if(ptTerrain->tr_atlLayers.Count()<=0 || 
     ptTerrain->tr_iSelectedLayer>=ptTerrain->tr_atlLayers.Count())
  {
    ptTerrain->tr_iSelectedLayer=0;
  }
  return ptTerrain->tr_iSelectedLayer;
}

void SelectLayer(INDEX iLayer)
{
  CTerrain *ptrTerrain=GetTerrain();
  if(ptrTerrain==NULL) return;

  if(ptrTerrain->tr_atlLayers.Count()<=iLayer || iLayer<0)
  {
    ptrTerrain->tr_iSelectedLayer=0;
  }
  else
  {
    ptrTerrain->tr_iSelectedLayer=iLayer;
  }
}
