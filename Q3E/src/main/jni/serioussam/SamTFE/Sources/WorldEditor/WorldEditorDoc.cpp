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

// WorldEditorDoc.cpp : implementation of the CWorldEditorDoc class
//

#include "stdafx.h"
#include "WorldEditor.h"
#include "WorldEditorDoc.h"

#include <Engine/Base/Profiling.h>
#include <Engine/Build.h>
#include <direct.h>

#ifdef _DEBUG
#undef new
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#pragma optimize("p", on) // this is in effect for entire file!
extern COLOR acol_ColorizePallete[];

/////////////////////////////////////////////////////////////////////////////
// CWorldEditorDoc

IMPLEMENT_DYNCREATE(CWorldEditorDoc, CDocument)

BEGIN_MESSAGE_MAP(CWorldEditorDoc, CDocument)
	//{{AFX_MSG_MAP(CWorldEditorDoc)
	ON_COMMAND(ID_CSG_SPLIT_SECTORS, OnCsgSplitSectors)
	ON_UPDATE_COMMAND_UI(ID_CSG_SPLIT_SECTORS, OnUpdateCsgSplitSectors)
	ON_COMMAND(ID_CSG_CANCEL, OnCsgCancel)
	ON_COMMAND(ID_SHOW_ORIENTATION, OnShowOrientation)
	ON_UPDATE_COMMAND_UI(ID_SHOW_ORIENTATION, OnUpdateShowOrientation)
	ON_COMMAND(ID_EDIT_UNDO, OnEditUndo)
	ON_COMMAND(ID_EDIT_REDO, OnEditRedo)
	ON_UPDATE_COMMAND_UI(ID_EDIT_UNDO, OnUpdateEditUndo)
	ON_UPDATE_COMMAND_UI(ID_EDIT_REDO, OnUpdateEditRedo)
	ON_COMMAND(ID_WORLD_SETTINGS, OnWorldSettings)
	ON_COMMAND(ID_CSG_JOIN_SECTORS, OnCsgJoinSectors)
	ON_UPDATE_COMMAND_UI(ID_CSG_JOIN_SECTORS, OnUpdateCsgJoinSectors)
	ON_COMMAND(ID_AUTO_SNAP, OnAutoSnap)
	ON_COMMAND(ID_CSG_ADD, OnCsgAdd)
	ON_UPDATE_COMMAND_UI(ID_CSG_ADD, OnUpdateCsgAdd)
	ON_COMMAND(ID_CSG_REMOVE, OnCsgRemove)
	ON_UPDATE_COMMAND_UI(ID_CSG_REMOVE, OnUpdateCsgRemove)
	ON_COMMAND(ID_CSG_SPLIT_POLYGONS, OnCsgSplitPolygons)
	ON_UPDATE_COMMAND_UI(ID_CSG_SPLIT_POLYGONS, OnUpdateCsgSplitPolygons)
	ON_COMMAND(ID_CSG_JOIN_POLYGONS, OnCsgJoinPolygons)
	ON_UPDATE_COMMAND_UI(ID_CSG_JOIN_POLYGONS, OnUpdateCsgJoinPolygons)
	ON_COMMAND(ID_CALCULATESHADOWS, OnCalculateShadows)
	ON_COMMAND(ID_BROWSE_ENTITIES_MODE, OnBrowseEntitiesMode)
	ON_UPDATE_COMMAND_UI(ID_BROWSE_ENTITIES_MODE, OnUpdateBrowseEntitiesMode)
	ON_COMMAND(ID_PREVIOUS_SELECTED_ENTITY, OnPreviousSelectedEntity)
	ON_UPDATE_COMMAND_UI(ID_PREVIOUS_SELECTED_ENTITY, OnUpdatePreviousSelectedEntity)
	ON_COMMAND(ID_NEXT_SELECTED_ENTITY, OnNextSelectedEntity)
	ON_UPDATE_COMMAND_UI(ID_NEXT_SELECTED_ENTITY, OnUpdateNextSelectedEntity)
	ON_COMMAND(ID_JOIN_LAYERS, OnJoinLayers)
	ON_UPDATE_COMMAND_UI(ID_AUTO_SNAP, OnUpdateAutoSnap)
	ON_COMMAND(ID_SELECT_BY_CLASS, OnSelectByClass)
	ON_UPDATE_COMMAND_UI(ID_SELECT_BY_CLASS, OnUpdateSelectByClass)
	ON_COMMAND(ID_CSG_JOIN_ALL_POLYGONS, OnCsgJoinAllPolygons)
	ON_UPDATE_COMMAND_UI(ID_CSG_JOIN_ALL_POLYGONS, OnUpdateCsgJoinAllPolygons)
	ON_COMMAND(ID_TEXTURE_1, OnTexture1)
	ON_UPDATE_COMMAND_UI(ID_TEXTURE_1, OnUpdateTexture1)
	ON_COMMAND(ID_TEXTURE_2, OnTexture2)
	ON_UPDATE_COMMAND_UI(ID_TEXTURE_2, OnUpdateTexture2)
	ON_COMMAND(ID_TEXTURE_3, OnTexture3)
	ON_UPDATE_COMMAND_UI(ID_TEXTURE_3, OnUpdateTexture3)
	ON_COMMAND(ID_TEXTURE_MODE_1, OnTextureMode1)
	ON_COMMAND(ID_TEXTURE_MODE_2, OnTextureMode2)
	ON_COMMAND(ID_TEXTURE_MODE_3, OnTextureMode3)
	ON_COMMAND(ID_SAVE_THUMBNAIL, OnSaveThumbnail)
	ON_COMMAND(ID_UPDATE_LINKS, OnUpdateLinks)
	ON_COMMAND(ID_SNAPSHOT, OnSnapshot)
	ON_COMMAND(ID_MIRROR_AND_STRETCH, OnMirrorAndStretch)
	ON_COMMAND(ID_FLIP_LAYER, OnFlipLayer)
	ON_UPDATE_COMMAND_UI(ID_FLIP_LAYER, OnUpdateFlipLayer)
	ON_COMMAND(ID_FILTER_SELECTION, OnFilterSelection)
	ON_COMMAND(ID_UPDATE_CLONES, OnUpdateClones)
	ON_UPDATE_COMMAND_UI(ID_UPDATE_CLONES, OnUpdateUpdateClones)
	ON_COMMAND(ID_HIDE_SELECTED, OnHideSelected)
	ON_UPDATE_COMMAND_UI(ID_HIDE_SELECTED, OnUpdateHideSelected)
	ON_COMMAND(ID_HIDE_UNSELECTED, OnHideUnselected)
	ON_COMMAND(ID_SHOW_ALL, OnShowAll)
	ON_COMMAND(ID_CHECK_EDIT, OnCheckEdit)
	ON_COMMAND(ID_CHECK_ADD, OnCheckAdd)
        ON_COMMAND(ID_CHECK_DELETE, OnCheckDelete)
	ON_UPDATE_COMMAND_UI(ID_CHECK_EDIT, OnUpdateCheckEdit)
	ON_UPDATE_COMMAND_UI(ID_CHECK_ADD, OnUpdateCheckAdd)
        ON_UPDATE_COMMAND_UI(ID_CHECK_DELETE, OnUpdateCheckDelete)
	ON_COMMAND(ID_UPDATE_BRUSHES, OnUpdateBrushes)
	ON_COMMAND(ID_SELECT_BY_CLASS_IMPORTANT, OnSelectByClassImportant)
	ON_COMMAND(ID_INSERT_3D_OBJECT, OnInsert3dObject)
	ON_COMMAND(ID_EXPORT_3D_OBJECT, OnExport3dObject)
	ON_UPDATE_COMMAND_UI(ID_EXPORT_3D_OBJECT, OnUpdateExport3dObject)
	ON_COMMAND(ID_CROSSROAD_FOR_N, OnCrossroadForN)
	ON_COMMAND(ID_POPUP_VTX_ALLIGN, OnPopupVtxAllign)
	ON_COMMAND(ID_POPUP_VTX_FILTER, OnPopupVtxFilter)
	ON_COMMAND(ID_POPUP_VTX_NUMERIC, OnPopupVtxNumeric)
	ON_COMMAND(ID_HIDE_SELECTED_SECTORS, OnHideSelectedSectors)
	ON_COMMAND(ID_HIDE_UNSELECTED_SECTORS, OnHideUnselectedSectors)
	ON_COMMAND(ID_SHOW_ALL_SECTORS, OnShowAllSectors)
	ON_COMMAND(ID_TEXTURE_MODE_4, OnTextureMode4)
	ON_COMMAND(ID_TEXTURE_MODE_5, OnTextureMode5)
	ON_COMMAND(ID_TEXTURE_MODE_6, OnTextureMode6)
	ON_COMMAND(ID_TEXTURE_MODE_7, OnTextureMode7)
	ON_COMMAND(ID_TEXTURE_MODE_8, OnTextureMode8)
	ON_COMMAND(ID_TEXTURE_MODE_9, OnTextureMode9)
	ON_COMMAND(ID_TEXTURE_MODE_10, OnTextureMode10)
	//}}AFX_MSG_MAP
  ON_COMMAND(ID_EXPORT_PLACEMENTS, OnExportPlacements)
  ON_COMMAND(ID_EXPORT_ENTITIES, OnExportEntities)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWorldEditorDoc construction/destruction

CWorldEditorDoc::CWorldEditorDoc()
{
  m_iCurrentTerrainUndo=-1;
  m_ptrSelectedTerrain=NULL;
  m_slDisplaceTexTime=0;
  m_bAskedToCheckOut = FALSE;
  m_pCutLineView = NULL;
  m_iMirror = 0;
  m_bWasEverSaved = FALSE;
  m_iSelectedEntityInVolume = 0;  
  m_iTexture = 0;
  m_ctLastPrimitiveVertices = -1;
  m_bPrimitiveCreatedFirstTime = TRUE;
  m_fLastPrimitiveWidth = 0.0;
  m_fLastPrimitiveLenght = 0.0;
  m_bLastIfOuter = FALSE;
  m_ttLastTriangularisationType = theApp.m_vfpCurrent.vfp_ttTriangularisationType;
  m_bAutoSnap = TRUE;
  m_bPrimitiveMode = FALSE;
  m_pwoSecondLayer = NULL;
  m_penPrimitive = NULL;
  m_bOrientationIcons=AfxGetApp()->GetProfileInt( L"World editor", L"Orientation icons", FALSE);
  m_bBrowseEntitiesMode = FALSE;
  m_bReadOnly = FALSE;

  m_csgtLastUsedCSGOperation = CSG_ILLEGAL;
  m_csgtPreLastUsedCSGOperation = CSG_ILLEGAL;
  m_bPreLastUsedPrimitiveMode = TRUE;
  m_bLastUsedPrimitiveMode = TRUE;
  m_fnLastDroppedTemplate = CTString("");

  // initialize grid placement 
  m_plGrid.pl_PositionVector = FLOAT3D(0.0f,0.0f,0.0f);
  m_plGrid.pl_OrientationAngle = ANGLE3D(0,0,0);
  // initialize delta placement
  m_plDeltaPlacement.pl_PositionVector = FLOAT3D(0.0f,0.0f,0.0f);
  m_plDeltaPlacement.pl_OrientationAngle = ANGLE3D(0,0,0);
  // initialize last placement
  m_plLastPlacement.pl_PositionVector = FLOAT3D(0.0f,0.0f,0.0f);
  m_plLastPlacement.pl_OrientationAngle = ANGLE3D(0,0,0);

  // initialize create box vertices
  char strIni[ 128];
  strcpy( strIni, CStringA(theApp.GetProfileString( L"World editor", L"Volume box min", L"0.0 0.0 0.0")));
  sscanf( strIni, "%f %f %f",
    &m_vCreateBoxVertice0(1), &m_vCreateBoxVertice0(2), &m_vCreateBoxVertice0(3));
  strcpy( strIni, CStringA(theApp.GetProfileString( L"World editor", L"Volume box max", L"1.0 1.0 1.0")));
  sscanf( strIni, "%f %f %f",
    &m_vCreateBoxVertice1(1), &m_vCreateBoxVertice1(2), &m_vCreateBoxVertice1(3));

  // set default editing mode - polygon mode
  INDEX iMode=AfxGetApp()->GetProfileInt( L"World editor", L"Last editing mode", POLYGON_MODE);
  if(iMode==POLYGON_MODE || iMode==VERTEX_MODE  || iMode==SECTOR_MODE || iMode==ENTITY_MODE || iMode==TERRAIN_MODE)
  {
    SetEditingMode( iMode);
  }
  else
  {
    SetEditingMode( POLYGON_MODE);
  }
}

CWorldEditorDoc::~CWorldEditorDoc()
{
  DeleteTerrainUndo(this);

  if(m_iMode==POLYGON_MODE || m_iMode==VERTEX_MODE  || m_iMode==SECTOR_MODE || m_iMode==ENTITY_MODE || m_iMode==TERRAIN_MODE)
  {
    theApp.WriteProfileInt(L"World editor", L"Last editing mode", m_iMode);
  }
  else
  {
    theApp.WriteProfileInt(L"World editor", L"Last editing mode", POLYGON_MODE);
  }

  if( m_pwoSecondLayer != NULL)
    delete m_pwoSecondLayer;

  // delete stored undo members
  FORDELETELIST(CUndo, m_lnListNode, m_lhUndo, itUndo)
  { 
    delete &itUndo.Current();
  }

  // delete redo
  FORDELETELIST(CUndo, m_lnListNode, m_lhRedo, itRedo)
  { 
		delete &itRedo.Current();
  }
}

void CWorldEditorDoc::ClearSelections(ESelectionType stExcept /*=ST_NONE*/)
{
  if( stExcept != ST_VERTEX)  { m_selVertexSelection.Clear(); m_chSelections.MarkChanged();}
  if( stExcept != ST_ENTITY)  { m_selEntitySelection.Clear(); m_chSelections.MarkChanged();}
  if( stExcept != ST_VOLUME)  { m_cenEntitiesSelectedByVolume.Clear(); m_chSelections.MarkChanged();}
  if( stExcept != ST_SECTOR)  { m_selSectorSelection.Clear(); m_chSelections.MarkChanged();}
  if( stExcept != ST_POLYGON) { m_selPolygonSelection.Clear(); m_chSelections.MarkChanged();}
}

/*
 * Sets message about current mode and selected members
 */
void CWorldEditorDoc::SetStatusLineModeInfoMessage( void)
{
  BOOL bAlt = (GetKeyState( VK_MENU)&0x8000) != 0;
  BOOL bCtrl = (GetKeyState( VK_CONTROL)&0x8000) != 0;

  // initialy, we are in polygon edit mode
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  
  HICON hIcon;

  char strModeName[ 32];
  // write mode change on status line
  switch( m_iMode)
  {
  case POLYGON_MODE:
    {
      sprintf( strModeName, "%d polys, layer %d", m_selPolygonSelection.Count(), m_iTexture+1);
      hIcon = theApp.LoadIcon( IDR_ICON_PANE_POLYGON);
      break;
    };
  case SECTOR_MODE:
    {
      sprintf( strModeName, "%d sectors", m_selSectorSelection.Count());
      hIcon = theApp.LoadIcon( IDR_ICON_PANE_SECTOR);
      break;
    };
  case ENTITY_MODE:
    { 
      if( m_bBrowseEntitiesMode)
      {
        sprintf( strModeName, "%d in volume", m_cenEntitiesSelectedByVolume.Count());
      }
      else
      {
        sprintf( strModeName, "%d entities", m_selEntitySelection.Count());
      };
      hIcon = theApp.LoadIcon( IDR_ICON_PANE_ENTITY);
      break;
    }
  case CSG_MODE: 
    {
      sprintf( strModeName, "CSG mode");
      hIcon = theApp.LoadIcon( IDR_ICON_PANE_CSG);
      break;
    };
  case VERTEX_MODE: 
    {
      sprintf( strModeName, "%d vertices", m_selVertexSelection.Count());
      hIcon = theApp.LoadIcon( IDR_ICON_PANE_VERTEX);
      break;
    };
  case TERRAIN_MODE:
    {
      if(bCtrl&&bAlt)
      {
        if(theApp.m_iTerrainEditMode==TEM_HEIGHTMAP)
        {
          sprintf( strModeName, "%s", "Pick altitude");
        }
        else
        {
          sprintf( strModeName, "%s", "Pick layer");
        }
      }
      else if(theApp.m_iTerrainEditMode==TEM_HEIGHTMAP)
      {
        INDEX iIcon;
        CTString strText;
        if(theApp.m_iTerrainBrushMode==TBM_FILTER)
        {
          sprintf( strModeName, "%s", GetFilterName(theApp.m_iFilter));
        }
        else
        {
          GetBrushModeInfo(INDEX(theApp.m_iTerrainBrushMode), iIcon, strText);
          sprintf( strModeName, "%s", strText);
        }
      }
      else
      {
        sprintf( strModeName, "Layer %d", GetLayerIndex());
      }
      hIcon = theApp.LoadIcon( IDR_ICON_PANE_TERRAIN);
      break;
    };
  default: { FatalError("Unknown editing mode."); break;};
  }
  pMainFrame->m_wndStatusBar.GetStatusBarCtrl().SetIcon( EDITING_MODE_ICON_PANE, hIcon);
  pMainFrame->m_wndStatusBar.SetPaneText( EDITING_MODE_PANE, CString(strModeName), TRUE);
}

/*
 * Changes editing mode
 */
void CWorldEditorDoc::SetEditingMode( INDEX iNewMode)
{
#if !ALLOW_TERRAINS
  if(iNewMode==TERRAIN_MODE)
  {
    return;
  }
#endif

  // exit cut mode
  theApp.m_bCutModeOn = FALSE;

  // cancel browse entities mode
  if( m_bBrowseEntitiesMode)
  {
    OnBrowseEntitiesMode();
  }
  // set new mode
  m_iMode = iNewMode;
  
  SetStatusLineModeInfoMessage();

  UpdateAllViews( NULL);
  // send message for mode change
  PostThreadMessage( GetCurrentThreadId(), WM_CHANGE_EDITING_MODE, TRUE, 0);
}

BOOL CWorldEditorDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

  // create the World entity
  CPlacement3D plWorld;
  plWorld.pl_PositionVector = FLOAT3D(0.0f,0.0f,0.0f);
  plWorld.pl_OrientationAngle = ANGLE3D(0,0,0);

  CEntity *penWorldBase;
  try
  {
    penWorldBase = m_woWorld.CreateEntity_t(plWorld, CTFILENAME("Classes\\WorldBase.ecl"));
  }
  catch (const char *err_str)
  {
    AfxMessageBox( CString(err_str));
    return FALSE;
  }
  // prepare the entity
  penWorldBase->Initialize();
  EFirstWorldBase eFirstWorldBase;
  penWorldBase->SendEvent( eFirstWorldBase);
  CEntity::HandleSentEvents();

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CWorldEditorDoc serialization

void CWorldEditorDoc::Serialize(CArchive& ar)
{
  // must not get here
  ASSERT(FALSE);
}

/////////////////////////////////////////////////////////////////////////////
// CWorldEditorDoc diagnostics

#ifdef _DEBUG
void CWorldEditorDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CWorldEditorDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CWorldEditorDoc commands
/////////////////////////////////////////////////////////////////////////////


void CWorldEditorDoc::SetupBackdropTextureObject( CTFileName fnPicture, CTextureObject &to)
{
  CImageInfo iiImageInfo;
  try
  {
    iiImageInfo.LoadAnyGfxFormat_t( fnPicture);
    // both dimension must be potentions of 2
    if( (iiImageInfo.ii_Width  == 1<<((int)Log2( (FLOAT)iiImageInfo.ii_Width))) &&
        (iiImageInfo.ii_Height == 1<<((int)Log2( (FLOAT)iiImageInfo.ii_Height))) )
    {
      CTFileName fnTexture = fnPicture.FileDir()+fnPicture.FileName()+".tex";
      // creates new texture with one frame
      CTextureData tdPicture;
      tdPicture.Create_t( &iiImageInfo, iiImageInfo.ii_Width, 1, FALSE);
      tdPicture.Save_t( fnTexture);
      to.SetData_t( fnTexture);
    }
  }
  catch (const char *strError)
  {
    (void) strError;
  }
}

BOOL CWorldEditorDoc::OnOpenDocument(LPCTSTR lpszPathName) 
{
  CTFileName fnOpenFileName;
  // open the world
  fnOpenFileName = CTString(CStringA(lpszPathName));
  if( fnOpenFileName.FileExt()!=".wld") return FALSE;
  try
  {
    fnOpenFileName.RemoveApplicationPath_t();

    // if the file is read only
    if(IsFileReadOnly(fnOpenFileName))
    {
      // warn user about it
      WarningMessage("'%s' is read only. You have to check it out to be able to save it.",
        (const char*)fnOpenFileName);
      // remember it
      m_bReadOnly = TRUE;
    }

    _pfWorldEditingProfile.Reset();
    m_woWorld.Load_t( fnOpenFileName);
    m_woWorld.ReinitializeEntities();
    _pfWorldEditingProfile.Report( theApp.m_strCSGAndShadowStatistics);
    theApp.m_strCSGAndShadowStatistics.SaveVar(CTString("Temp\\Profile_Open.txt"));

    // try to load textures for backdrops
    if( m_woWorld.wo_strBackdropUp != "")
    {
      SetupBackdropTextureObject( m_woWorld.wo_strBackdropUp, m_toBackdropUp);
    }
    if( m_woWorld.wo_strBackdropFt != "")
    {
      SetupBackdropTextureObject( m_woWorld.wo_strBackdropFt, m_toBackdropFt);
    }
    if( m_woWorld.wo_strBackdropRt != "")
    {
      SetupBackdropTextureObject( m_woWorld.wo_strBackdropRt, m_toBackdropRt);
    }
    POSITION pos = GetFirstViewPosition();
    CWorldEditorView *pWedView = (CWorldEditorView *) GetNextView(pos);
    ASSERT( pWedView != NULL);
    CChildFrame *pWedChild = pWedView->GetChildFrame();
    ASSERT( pWedChild != NULL);
    pWedChild->m_mvViewer.mv_plViewer = m_woWorld.wo_plFocus;
    pWedChild->m_mvViewer.mv_fTargetDistance = m_woWorld.wo_fTargetDistance;
  }
  catch (const char *strError)
  {
    AfxMessageBox( CString(strError));
    return FALSE;
  }

  // try to load object for backdrops
  if( m_woWorld.wo_strBackdropObject != "")
  {
    // try to
    try
    {
      // load 3D lightwawe object
      FLOATmatrix3D mStretch;
      mStretch.Diagonal(1.0f);
      m_o3dBackdropObject.LoadAny3DFormat_t( m_woWorld.wo_strBackdropObject, mStretch);
    }
    // catch and
    catch (const char *strError)
    {
      // report errors
      AfxMessageBox( CString(strError));
    }
  }

  if( theApp.m_Preferences.ap_bShowAllOnOpen)
  {
    OnShowAllEntities();
    OnShowAllSectors();
  }
  // flush stale caches
  _pShell->Execute("FreeUnusedStock();");
	return TRUE;
}

// overridden from mfc
void CWorldEditorDoc::SetModifiedFlag( BOOL bModified /*= TRUE*/ )
{
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  CDocument::SetModifiedFlag(bModified);

  if (!bModified) {
    return;
  }

  try
  {
    if (IsFileReadOnly(m_woWorld.wo_fnmFileName) && !m_bAskedToCheckOut)
    {
      // ask for check out
      if( ::MessageBoxA( pMainFrame->m_hWnd, "Do you want to open the world for edit?",
                      "Warning !", MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON1 | 
                      MB_TASKMODAL | MB_TOPMOST) == IDYES)
      {
        m_bAskedToCheckOut = TRUE;
        OnCheckEdit();
      }
    }
  }
  catch (const char *strError)
  {
    AfxMessageBox( CString(strError));
    return;
  }
}

BOOL CWorldEditorDoc::OnSaveDocument(LPCTSTR lpszPathName) 
{
  CTFileName fnSaveFileName;
  // save the world
  fnSaveFileName = CTString(CStringA(lpszPathName));
  try
  {
    fnSaveFileName.RemoveApplicationPath_t();
    // if the file is read only
    if(IsFileReadOnly(fnSaveFileName))
    {
      // don't allow saving
      WarningMessage( "World file is read-only. You can't save it.");
      return FALSE;
    }

    POSITION pos = GetFirstViewPosition();
    CWorldEditorView *pWedView = (CWorldEditorView *) GetNextView(pos);
    CChildFrame *pWedChild = pWedView->GetChildFrame();
    m_woWorld.wo_plFocus = pWedChild->m_mvViewer.mv_plViewer;
    m_woWorld.wo_fTargetDistance = pWedChild->m_mvViewer.mv_fTargetDistance;

    m_woWorld.Save_t( fnSaveFileName);
    SetModifiedFlag(FALSE);
  }
  catch (const char *strError)
  {
    AfxMessageBox( CString(strError));
    return FALSE;
  }
  // write file's directory into application's .ini file
  theApp.WriteProfileString(L"World editor", L"Open directory",
                            CString(_fnmApplicationPath+fnSaveFileName.FileDir()));
  // save thumbnail
  SaveThumbnail();
  m_bWasEverSaved = TRUE;
  return TRUE;
}

static BOOL _bDontRecalculateBase = FALSE;

// start creating primitive from current application's primitive, don't recreate base
void CWorldEditorDoc::ApplyCurrentPrimitiveSettings(void)
{
  ASSERT( m_pwoSecondLayer != NULL);
  // destroy second layer world
  delete m_pwoSecondLayer;
  m_pwoSecondLayer = NULL;
  INDEX iPreCSGMode = m_iPreCSGMode;
  // force not recreation of primitive base
  _bDontRecalculateBase = TRUE;
  StartPrimitiveCSG( theApp.m_vfpCurrent.vfp_plPrimitive, FALSE);
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  pMainFrame->m_TriangularisationCombo.SetCurSel( (int)theApp.m_vfpCurrent.vfp_ttTriangularisationType);
  m_iPreCSGMode = iPreCSGMode;
}

// start creating primitive
void CWorldEditorDoc::StartPrimitiveCSG( CPlacement3D plPrimitive, BOOL bResetAngles/*=TRUE*/)
{
  ApplyAutoColorize();
  if( theApp.m_ptdActiveTexture == NULL)
  {
    AfxMessageBox( L"You have to select active texture first (double click on texture in browser).");
    return;
  }
  if( !((theApp.m_vfpCurrent.vfp_ptPrimitiveType == PT_CONUS) ||
        (theApp.m_vfpCurrent.vfp_ptPrimitiveType == PT_TORUS) ||
        (theApp.m_vfpCurrent.vfp_ptPrimitiveType == PT_STAIRCASES) ||
        (theApp.m_vfpCurrent.vfp_ptPrimitiveType == PT_SPHERE) ||
        (theApp.m_vfpCurrent.vfp_ptPrimitiveType == PT_TERRAIN) ) )
  {
    WarningMessage( "This type of primitive not yet supported. Please be patient !!!");
    return;
  }
  POSITION pos = GetFirstViewPosition();
  CWorldEditorView *pWedView = (CWorldEditorView *) GetNextView(pos);
  CChildFrame *pWedChild = pWedView->GetChildFrame();
  // remember auto mip brushing flag
  pWedChild->m_bLastAutoMipBrushingOn = pWedChild->m_bAutoMipBrushingOn;
  // turn off auto mip brushing
  pWedChild->m_bAutoMipBrushingOn = FALSE;

  m_pwoSecondLayer = new CWorld;
  m_bPrimitiveMode = TRUE;

  // position the second layer
  m_plSecondLayer = plPrimitive;
  // if angle reset requested
  if( bResetAngles)
  {
    // reset angles so they are alligned to current grid
    m_plSecondLayer.pl_OrientationAngle = m_plGrid.pl_OrientationAngle;
  }

  // create the World entity
  CPlacement3D plPrimitiveEntity;
  plPrimitiveEntity.pl_PositionVector = FLOAT3D(0.0f,0.0f,0.0f);
  plPrimitiveEntity.pl_OrientationAngle = ANGLE3D(0,0,0);
  try
  {
    m_penPrimitive = m_pwoSecondLayer->CreateEntity_t( plPrimitiveEntity,
      CTFILENAME("Classes\\WorldBase.ecl"));
  }
  catch (const char *err_str)
  {
    AfxMessageBox( CString(err_str));
    // discard initialized variables needed for CSG
    delete m_pwoSecondLayer;
    m_pwoSecondLayer = NULL;
    m_bPrimitiveMode = FALSE;
    return;
  }
  // prepare the entity
  m_penPrimitive->Initialize();
  // store current mode
  m_iPreCSGMode = m_iMode;
  // start CSG mode
  SetEditingMode( CSG_MODE);
  // create primitive for the first time
  //m_bPrimitiveCreatedFirstTime = TRUE;
  CreatePrimitive();
  // if preferences say so, show info
  if( theApp.m_Preferences.ap_AutomaticInfo)
  {
    CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
    pMainFrame->ShowInfoWindow();
  }
  // invalidate document (i.e. all views)
  UpdateAllViews( NULL);
}

// start CSG with world template
void CWorldEditorDoc::StartTemplateCSG( CPlacement3D plTemplate,
                                        const CTFileName &fnWorld, BOOL bResetAngles/*=TRUE*/)
{
  POSITION pos = GetFirstViewPosition();
  CWorldEditorView *pWedView = (CWorldEditorView *) GetNextView(pos);
  CChildFrame *pWedChild = pWedView->GetChildFrame();
  // remember auto mip brushing flag
  pWedChild->m_bLastAutoMipBrushingOn = pWedChild->m_bAutoMipBrushingOn;
  // turn off auto mip brushing
  pWedChild->m_bAutoMipBrushingOn = FALSE;

  m_pwoSecondLayer = new CWorld;
  m_bPrimitiveMode = FALSE;

  // remember name of last template used for CSG
  m_fnLastDroppedTemplate = fnWorld;

  // position the second layer
  m_plSecondLayer = plTemplate;

  // if angle reset was requested
  if( bResetAngles)
  {
    // reset angles so they are alligned to current grid
    m_plSecondLayer.pl_OrientationAngle = m_plGrid.pl_OrientationAngle;
  }

  try
  {
    // load the World
    m_pwoSecondLayer->Load_t( fnWorld);
  }
  catch (const char *err_str)
  {
    AfxMessageBox( CString(err_str));
    delete m_pwoSecondLayer;
    m_pwoSecondLayer = NULL;
    return;
  }
  // invalidate document (i.e. all views)
  UpdateAllViews( NULL);
  // store current mode
  m_iPreCSGMode = m_iMode;
  // start CSG mode
  SetEditingMode( CSG_MODE);
  // update position property page for the first time
  m_chSelections.MarkChanged();

  // if preferences say so, show info
  if( theApp.m_Preferences.ap_AutomaticInfo)
  {
    CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
    pMainFrame->ShowInfoWindow();
  }
  // don't join layers !!!!

  /*
  // if none of entities in dropped world has brush rendering type
  // for all of the world's entities
  {FOREACHINDYNAMICCONTAINER(m_pwoSecondLayer->wo_cenEntities, CEntity, iten)
  {
    CEntity::RenderType rt = iten->GetRenderType();
    // if the entity is brush and it is not empty
    if( (rt==CEntity::RT_BRUSH || rt==CEntity::RT_FIELDBRUSH) && ( !iten->IsEmptyBrush()) &&
        (CTString(iten->GetClass()->ec_pdecDLLClass->dec_strName) == "WorldBase") )
    {
      // if preferences say so, show info
      if( theApp.m_Preferences.ap_AutomaticInfo)
      {
        CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
        pMainFrame->ShowInfoWindow();
      }
      // don't join layers
      return;
    }
  }}
  // call join layers
  OnJoinLayers();
  */
}

#define CT_PRIMITIVES_IN_HISTORY_BUFFER 1024
// apply current CSG operation
void CWorldEditorDoc::ApplyCSG(enum CSGType CSGType)
{
  if((CSGType==CSG_ADD ||
      CSGType==CSG_ADD_REVERSE ||
      CSGType==CSG_REMOVE ||
      CSGType==CSG_REMOVE_REVERSE ||
      CSGType==CSG_ADD_ENTITIES ||
      CSGType==CSG_SPLIT_POLYGONS ||
      CSGType==CSG_JOIN_LAYERS) && (m_pwoSecondLayer==NULL)) return;
  POSITION pos = GetFirstViewPosition();
  CWorldEditorView *pWedView = (CWorldEditorView *) GetNextView(pos);
  CChildFrame *pWedChild = pWedView->GetChildFrame();
  // restore auto mip brushing flag
  pWedChild->m_bAutoMipBrushingOn = pWedChild->m_bLastAutoMipBrushingOn;

  // set wait cursor
  CWaitCursor StartWaitCursor;

  if( theApp.m_bCSGReportEnabled)
  {
    _pfWorldEditingProfile.Reset();
  }

  RememberUndo();
  // remember last used CSG operation
  m_csgtPreLastUsedCSGOperation = m_csgtLastUsedCSGOperation;
  m_csgtLastUsedCSGOperation = CSGType;
  // set flag telling is last used CSG was primitive
  m_bPreLastUsedPrimitiveMode = m_bLastUsedPrimitiveMode;
  m_bLastUsedPrimitiveMode = m_bPrimitiveMode;

  // invalving entities
  CEntity *penThis;
  CEntity *penOther;
  BOOL bThisFound = FALSE;
  BOOL bOtherFound = FALSE;

  // calculate delta placement
	m_plDeltaPlacement = m_plSecondLayer;
  // convert it into absolute space of last used placement (delta calculated)
  m_plDeltaPlacement.AbsoluteToRelative( m_plLastPlacement);
  // remember position of last applied CSG
  m_plLastPlacement = m_plSecondLayer;

  theApp.m_vfpCurrent.vfp_plPrimitive = m_plSecondLayer;
  // calculate width, height, sheer,... delta to be able to use it for next clone CSG
  theApp.m_vfpDelta = theApp.m_vfpCurrent - theApp.m_vfpLast;
  // remember last values for primitive as prelast used ones
  theApp.m_vfpPreLast = theApp.m_vfpLast;
  // remember current values for primitive as last used ones
  theApp.m_vfpLast = theApp.m_vfpCurrent;

  if( m_bPrimitiveMode)
  {
    // if there are too many primitives in history buffer
    if( theApp.m_lhPrimitiveHistory.Count() >= CT_PRIMITIVES_IN_HISTORY_BUFFER)
    {
      // remove last used one
      CPrimitiveInHistoryBuffer *ppihbLast = 
        LIST_TAIL( theApp.m_lhPrimitiveHistory, CPrimitiveInHistoryBuffer, pihb_lnNode);
      theApp.m_lhPrimitiveHistory.RemTail();
      delete ppihbLast;
    }

    // add this primtive into history buffer
    CPrimitiveInHistoryBuffer *ppihbMember = new CPrimitiveInHistoryBuffer;
    ppihbMember->pihb_vfpPrimitive = theApp.m_vfpCurrent;
    ppihbMember->pihb_vfpPrimitive.vfp_csgtCSGOperation = CSGType;
    theApp.m_lhPrimitiveHistory.AddHead( ppihbMember->pihb_lnNode);

    // save primitives history buffer
    CTFileStream strmFile;
    try
    {
      strmFile.Create_t( CTString("Data\\PrimitivesHistory.pri"));
      INDEX ctHistory = theApp.m_lhPrimitiveHistory.Count();
      strmFile << ctHistory;
      // write history primitives list
      FOREACHINLIST( CPrimitiveInHistoryBuffer, pihb_lnNode, theApp.m_lhPrimitiveHistory, itPrim)
      {
        itPrim->pihb_vfpPrimitive.Write_t( strmFile);
      }
    }
    catch (const char *strError)
    {
      WarningMessage( strError);
    }

    // remember used values for primitive to be used as default values for next primitive
    // of same type
    switch( theApp.m_vfpCurrent.vfp_ptPrimitiveType)
    {
    case PT_CONUS:{ theApp.m_vfpConus = theApp.m_vfpCurrent; break;}
    case PT_TORUS:{ theApp.m_vfpTorus = theApp.m_vfpCurrent; break;}
    case PT_STAIRCASES:{ theApp.m_vfpStaircases = theApp.m_vfpCurrent; break;}
    case PT_SPHERE:{ theApp.m_vfpSphere = theApp.m_vfpCurrent; break;}
    case PT_TERRAIN:{ theApp.m_vfpTerrain = theApp.m_vfpCurrent; break;}
    default: ASSERTALWAYS( "Wrong primitive type occured");
    }
  }

  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());

  // join operations are not really CSG operations because we do not have second layer
  BOOL bJoinOperation = (
    CSGType==CSG_JOIN_LAYERS ||
    CSGType==CSG_JOIN_SECTORS ||
    CSGType==CSG_JOIN_POLYGONS ||
    CSGType==CSG_JOIN_POLYGONS_KEEP_TEXTURES ||
    CSGType==CSG_JOIN_ALL_POLYGONS ||
    CSGType==CSG_JOIN_ALL_POLYGONS_KEEP_TEXTURES);

  if( !bJoinOperation)
  {
    // for real CSG operations search for invalving entities
    penThis = pMainFrame->m_CSGDesitnationCombo.GetSelectedBrushEntity();
    if( penThis != NULL)
    {
      bThisFound = TRUE;
    }
    if( !bThisFound) 
    {
      WarningMessage( "Destination for CSG can't be optained, canceling CSG.");
      StopCSG();
      return;
    }

    // find other entity
    {FOREACHINDYNAMICCONTAINER(m_pwoSecondLayer->wo_cenEntities, CEntity, itenOther) {
      if (CTString(itenOther->GetClass()->ec_pdecDLLClass->dec_strName) == "WorldBase") {
        penOther = &itenOther.Current();
        bOtherFound = TRUE;
        break;
      }
    }}
    // if other entity can't be obtained, switch to join layers mode
    if( !bOtherFound)
    {
      CSGType = CSG_JOIN_LAYERS;
    }
  }

  // act acording requested CSG operation
  switch( CSGType)
  {
    case CSG_ADD:
    {
      ClearSelections();
      // apply "add"
      m_woWorld.CSGAdd(*penThis, *m_pwoSecondLayer, *penOther, m_plSecondLayer);
      break;
    }
    case CSG_ADD_REVERSE:
    {
      ClearSelections();
      // apply "add reverse"
      m_woWorld.CSGAddReverse(*penThis, *m_pwoSecondLayer, *penOther, m_plSecondLayer);
      break;
    }
    case CSG_REMOVE:
    {
      ClearSelections();
      // apply "remove"
      m_woWorld.CSGRemove(*penThis, *m_pwoSecondLayer, *penOther, m_plSecondLayer);
      break;
    }
    case CSG_REMOVE_REVERSE:
    {
      //ClearSelections();
      // apply "remove reverse"
      // m_woWorld.CSGRemoveReverse(*penThis, *m_pwoSecondLayer, *penOther, m_plSecondLayer);
      break;
    }
    case CSG_SPLIT_SECTORS:
    {
      // clear all selections except sector seletion
      ClearSelections( ST_SECTOR);
      // apply "split sectors"
      m_woWorld.SplitSectors(*penThis, m_selSectorSelection,
        *m_pwoSecondLayer, *penOther, m_plSecondLayer);
      break;
    }
    case CSG_JOIN_SECTORS:
    {
      // clear all selections except sector seletion
      ClearSelections( ST_SECTOR);
      // join selected sectors
      m_woWorld.JoinSectors( m_selSectorSelection);
      // store current mode
      m_iPreCSGMode = m_iMode;
      break;
    }
    case CSG_SPLIT_POLYGONS:
    {
      // clear all selections except polygon seletion
      ClearSelections( ST_POLYGON);
      // apply "split polygons"
      m_woWorld.SplitPolygons(*penThis, m_selPolygonSelection,
        *m_pwoSecondLayer, *penOther, m_plSecondLayer);
      break;
    }
    case CSG_JOIN_POLYGONS:
    case CSG_JOIN_POLYGONS_KEEP_TEXTURES:
    {
      // clear all selections except polygon seletion
      ClearSelections( ST_POLYGON);
      // join selected polygons
      m_woWorld.JoinPolygons(m_selPolygonSelection);
      // store current mode
      m_iPreCSGMode = m_iMode;
      break;
    }
    case CSG_JOIN_ALL_POLYGONS:
    case CSG_JOIN_ALL_POLYGONS_KEEP_TEXTURES:
    {
      // clear all selections except polygon seletion
      ClearSelections( ST_POLYGON);
      // join selected polygons
      m_woWorld.JoinAllPossiblePolygons( 
        m_selPolygonSelection, CSGType==CSG_JOIN_ALL_POLYGONS_KEEP_TEXTURES, m_iTexture);
      // store current mode
      m_iPreCSGMode = m_iMode;
      break;
    }
    case CSG_JOIN_LAYERS:
    {
      theApp.m_vfpCurrent.vfp_csgtCSGOperation = CSG_JOIN_LAYERS;
      // clear entity selections
      m_selEntitySelection.Clear();
      m_cenEntitiesSelectedByVolume.Clear();
      // mark that selections have been changed
      m_chSelections.MarkChanged();

      // make container of entities to copy
      CDynamicContainer<CEntity> cenToCopy;
      cenToCopy = m_pwoSecondLayer->wo_cenEntities;
      // remove empty brushes from it
      {FOREACHINDYNAMICCONTAINER(m_pwoSecondLayer->wo_cenEntities, CEntity, iten)
      {
        if( iten->IsEmptyBrush() && (iten->GetFlags()&ENF_ZONING))
        {
          cenToCopy.Remove(iten);
        }
      }}
      // copy entities in container
      m_woWorld.CopyEntities( *m_pwoSecondLayer, cenToCopy, 
        m_selEntitySelection, m_plSecondLayer);
      m_iPreCSGMode = ENTITY_MODE;
      break;
    }
    default:
    {
      ASSERTALWAYS( "Illegal CSG operation type requested!");
    }
  }

  // increase auto colorize color's index
  theApp.m_iLastAutoColorizeColor=(theApp.m_iLastAutoColorizeColor+1)%32;

  m_chSelections.MarkChanged();
  SetModifiedFlag(TRUE);
  m_chDocument.MarkChanged();
  StopCSG();

  if( theApp.m_bCSGReportEnabled)
  {
    // create CSG report
    _pfWorldEditingProfile.Report( theApp.m_strCSGAndShadowStatistics);
    theApp.m_strCSGAndShadowStatistics.SaveVar(CTString("Temp\\Profile_CSG.txt"));
  }
  m_iMirror = 0;
}

// clean up after doing a CSG
void CWorldEditorDoc::StopCSG(void)
{
  if( m_pwoSecondLayer == NULL) return;
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  // destroy second layer world
  delete m_pwoSecondLayer;
  m_pwoSecondLayer = NULL;
  m_bPrimitiveMode = FALSE;

  // if preferences say so, hide info
  if( theApp.m_Preferences.ap_AutomaticInfo)
  {
    pMainFrame->HideInfoWindow();
  }
  // restore mode
  SetEditingMode( m_iPreCSGMode);

  // if info frame exist
  if( pMainFrame->m_pInfoFrame != NULL)
  {
    // force immidiate page refilling
    pMainFrame->m_pInfoFrame->m_pInfoSheet->OnIdle( 0);
  }

  // invalidate document (i.e. all views)
  UpdateAllViews( NULL);
}

// cancel current CSG operation
void CWorldEditorDoc::CancelCSG(void)
{
  StopCSG();
}

void CWorldEditorDoc::OnIdle(void)
{
  CValuesForPrimitive &vfp=theApp.m_vfpCurrent;
  if( m_pwoSecondLayer!=NULL && m_bPrimitiveMode &&
      vfp.vfp_ptPrimitiveType==PT_TERRAIN &&
      vfp.vfp_fnDisplacement!="" &&
      theApp.m_Preferences.ap_bAutoUpdateDisplaceMap)
  {
    try
    {
      SLONG slFileTime=GetFileTimeStamp_t(vfp.vfp_fnDisplacement);
      if(slFileTime>m_slDisplaceTexTime)
      {
        CreatePrimitive();
        UpdateAllViews( NULL);
      }
    }
    catch (const char *strError)
    {
      (void) strError;
    }
  }
  
  POSITION pos = GetFirstViewPosition();
  CWorldEditorView *pWedView;
  FOREVER
  {
    pWedView = (CWorldEditorView *) GetNextView(pos);
    if( pWedView == NULL) break;
    pWedView->OnIdle();
  }

  if( GetEditingMode()==TERRAIN_MODE)
  {
    UpdateAllViews( NULL);
  }
}

// does "snap to grid" for given coordinate
void CWorldEditorDoc::SnapFloat( FLOAT &fDest, FLOAT fStep /* SNAP_FLOAT_GRID */)
{
  // this must use floor() to get proper snapping of negative values.
  FLOAT fDiv = fDest/fStep;
  FLOAT fRound = fDiv + 0.5f;
  int iSnap = int( floor(fRound));
  FLOAT fRes = iSnap * fStep;
  fDest = fRes;
}

// does "snap to grid" for given angle
void CWorldEditorDoc::SnapAngle( ANGLE &angDest, ANGLE angStep /* SNAP_ANGLE_GRID */)
{
  /* Watch out for unsigned-signed mixing!
   All sub-expression and arguments must be unsigned for this to work correctly!
   Unfortunately, ANGLE is not an unsigned type by default, so we must cast it.
   Also, angStep must be a divisor of ANGLE_180!
   */
  
  SnapFloat( angDest, angStep);

  /*
  ASSERT(ANGLE_180%angStep == 0);   // don't test with ANGLE_360 ,since it is 0!
  angDest = ANGLE( ((UWORD(angDest)+UWORD(angStep)/2U)/UWORD(angStep))*UWORD(angStep) );
  */
}

// does "snap to grid" for given placement
void CWorldEditorDoc::SnapToGrid( CPlacement3D &plPlacement, FLOAT fSnapValue)
{
  FLOAT fAngleSnap = SNAP_ANGLE_GRID;
  if( fSnapValue < SNAP_FLOAT_CM) fSnapValue = SNAP_FLOAT_CM;
  if( !m_bAutoSnap)
  {
    fSnapValue = SNAP_FLOAT_CM;
    fAngleSnap = ANGLE_SNAP/32.0f;
  }

  // snap X coordinate
  SnapFloat( plPlacement.pl_PositionVector(1), fSnapValue);
  // snap Y coordinate
  SnapFloat( plPlacement.pl_PositionVector(2), fSnapValue);
  // snap Z coordinate
  SnapFloat( plPlacement.pl_PositionVector(3), fSnapValue);
  
  /*
  // snap H angle
  SnapAngle( plPlacement.pl_OrientationAngle(1));
  // snap P angle
  SnapAngle( plPlacement.pl_OrientationAngle(2));
  // snap B angle
  SnapAngle( plPlacement.pl_OrientationAngle(3));
  */
  
  // snap X coordinate
  SnapFloat( plPlacement.pl_PositionVector(1), SNAP_FLOAT_CM);
  // snap Y coordinate
  SnapFloat( plPlacement.pl_PositionVector(2), SNAP_FLOAT_CM);
  // snap Z coordinate
  SnapFloat( plPlacement.pl_PositionVector(3), SNAP_FLOAT_CM);
  
  // snap H angle
  SnapAngle( plPlacement.pl_OrientationAngle(1), fAngleSnap);
  // snap P angle
  SnapAngle( plPlacement.pl_OrientationAngle(2), fAngleSnap);
  // snap B angle
  SnapAngle( plPlacement.pl_OrientationAngle(3), fAngleSnap);
}

// static vars used for polygon creation in primitives
static BOOL _bAutoCreateMipBrushes;
static BOOL _bClosed;
static CObjectMaterial *_pomMaterial;
static CObjectSector *_poscSector;
static CTextureData *_pPrimitiveTexture;
static DOUBLE _fTextureWidth;
static DOUBLE _fTextureHeight;


void DisplaceVertex( DOUBLE3D &vVtx, CImageInfo *pII,
                     DOUBLE fMinX, DOUBLE fMaxX, DOUBLE fMinZ, DOUBLE fMaxZ, 
                     INDEX iSlicesPerW, INDEX iSlicesPerL, FLOAT fAmplitude)
{
  if( pII == NULL) return;

  FLOAT fPix = (fMaxX-fMinX)/(pII->ii_Width-1);
  FLOAT fDelta = (vVtx(1)-fMinX);
  FLOAT fMaxDelta = (fMaxX-fMinX);
  FLOAT fTmp = ((pII->ii_Width-1) / fMaxDelta * fDelta);

  PIX pixX = (PIX)((pII->ii_Width-1) /(fMaxX-fMinX) * (vVtx(1) + fPix/2.0f -fMinX) );
  PIX pixY = (PIX)((pII->ii_Height-1)/(fMaxZ-fMinZ) * (vVtx(3) + fPix/2.0f -fMinZ) );
  if( pixX >= pII->ii_Width)  pixX = pII->ii_Width -1;
  if( pixY >= pII->ii_Height) pixY = pII->ii_Height-1;
  SLONG slPicPosition = (pII->ii_Width*pixY +pixX) * (pII->ii_BitsPerPixel/8);
  vVtx(2) += fAmplitude/256.0f * pII->ii_Picture[slPicPosition];
}


#define HEIGHT_EPSILON 0.001

void AddPolygon(INDEX vtxCt, DOUBLE3D *avVtx, BOOL bInvert,
                DOUBLE3D f3dMappingTranslation = DOUBLE3D(0.0f,0.0f,0.0f),
                CImageInfo *pII=NULL,
                DOUBLE fMinX=0.0f, DOUBLE fMaxX=0.0f,
                DOUBLE fMinZ=0.0f, DOUBLE fMaxZ=0.0f,
                INDEX iSlicesPerW=0, INDEX iSlicesPerL=0,
                FLOAT fAmplitude=0.0f,
                DOUBLE fRaiseHeight=0.0f)
{
  // copy array of vertices
  DOUBLE3D *avVtxCopy = new DOUBLE3D[vtxCt];
  
  for( INDEX iCopy=0; iCopy<vtxCt; iCopy++)
  {
    avVtxCopy[iCopy] = avVtx[iCopy];
  }

  
  // displace all vertices
  if( pII != NULL)
  {
    for( INDEX iVtx=0; iVtx<vtxCt; iVtx++)
    {
      if( Abs(avVtxCopy[iVtx](2)-fRaiseHeight)<HEIGHT_EPSILON)
      {
        DisplaceVertex( avVtxCopy[iVtx], pII, fMinX, fMaxX, fMinZ, fMaxZ,
          iSlicesPerW, iSlicesPerL, fAmplitude);
      }
    }
  }

  /*
  // report it
  _RPT1(_CRT_WARN, "\n%d:", vtxCt);
  for(INDEX ivx=0; ivx<vtxCt; ivx++) {
    // report it
    _RPT3(_CRT_WARN, " (%f, %f, %f)",
      avVtx[ivx](1),
      avVtx[ivx](2),
      avVtx[ivx](3));
  }
  */
  
  switch( theApp.m_vfpCurrent.vfp_ttTriangularisationType)
  {
    case TT_NONE:
    {
      // create polygon
      CObjectPolygon *pObjectPolygon = _poscSector->CreatePolygon( 
        vtxCt, avVtxCopy, *_pomMaterial, NULL, bInvert);
      if( pObjectPolygon != NULL) {
        // set shadow cluster size to 2m
        ((CBrushPolygonProperties&)(pObjectPolygon->opo_ubUserData)).bpp_sbShadowClusterSize=2;
      }
      break;
    }
    case TT_CENTER_VERTEX:
    {
      // calculate center vertex
      DOUBLE3D vCenter = DOUBLE3D( 0.0, 0.0, 0.0);
      INDEX iVtx=0;
      for( ; iVtx<vtxCt; iVtx++)
      {
        vCenter+=avVtxCopy[iVtx];
      }
      vCenter /= vtxCt;

      // create polygons
      DOUBLE3D avPolygon[ 3];
      for( iVtx=0; iVtx<vtxCt; iVtx++)
      {
        INDEX iNextVtx = (iVtx+1)%vtxCt;
        avPolygon[ 0] = avVtxCopy[iVtx];
        avPolygon[ 1] = avVtxCopy[iNextVtx];
        avPolygon[ 2] = vCenter;
        CObjectPolygon *pObjectPolygon = _poscSector->CreatePolygon( 
          3, avPolygon, *_pomMaterial, NULL, bInvert);
        if( pObjectPolygon != NULL) {
          // set shadow cluster size to 2m
          ((CBrushPolygonProperties&)(pObjectPolygon->opo_ubUserData)).bpp_sbShadowClusterSize=2;
        }
      }
      break;
    }
    default:
    {
      INDEX iStartVtx = 0;
      if( theApp.m_vfpCurrent.vfp_ttTriangularisationType == TT_FROM_VTX00) iStartVtx = 0;
      if( theApp.m_vfpCurrent.vfp_ttTriangularisationType == TT_FROM_VTX01) iStartVtx = 1;
      if( theApp.m_vfpCurrent.vfp_ttTriangularisationType == TT_FROM_VTX02) iStartVtx = 2;
      if( theApp.m_vfpCurrent.vfp_ttTriangularisationType == TT_FROM_VTX03) iStartVtx = 3;
      if( theApp.m_vfpCurrent.vfp_ttTriangularisationType == TT_FROM_VTX04) iStartVtx = 4;
      if( theApp.m_vfpCurrent.vfp_ttTriangularisationType == TT_FROM_VTX05) iStartVtx = 5;
      if( theApp.m_vfpCurrent.vfp_ttTriangularisationType == TT_FROM_VTX06) iStartVtx = 6;
      if( theApp.m_vfpCurrent.vfp_ttTriangularisationType == TT_FROM_VTX07) iStartVtx = 7;
      if( theApp.m_vfpCurrent.vfp_ttTriangularisationType == TT_FROM_VTX08) iStartVtx = 8;
      if( theApp.m_vfpCurrent.vfp_ttTriangularisationType == TT_FROM_VTX09) iStartVtx = 9;
      if( theApp.m_vfpCurrent.vfp_ttTriangularisationType == TT_FROM_VTX10) iStartVtx =10;
      if( theApp.m_vfpCurrent.vfp_ttTriangularisationType == TT_FROM_VTX11) iStartVtx =11;
      if( theApp.m_vfpCurrent.vfp_ttTriangularisationType == TT_FROM_VTX12) iStartVtx =12;
      if( theApp.m_vfpCurrent.vfp_ttTriangularisationType == TT_FROM_VTX13) iStartVtx =13;
      if( theApp.m_vfpCurrent.vfp_ttTriangularisationType == TT_FROM_VTX14) iStartVtx =14;
      if( theApp.m_vfpCurrent.vfp_ttTriangularisationType == TT_FROM_VTX15) iStartVtx =15;
      iStartVtx %= vtxCt;
      // create polygons
      DOUBLE3D avPolygon[ 3];
      for( INDEX iVtx=iStartVtx; iVtx<iStartVtx+vtxCt-2; iVtx++)
      {
        INDEX iNextVtx = (iVtx+1)%vtxCt;
        INDEX iNextNextVtx = (iNextVtx+1)%vtxCt;
        avPolygon[ 0] = avVtxCopy[iStartVtx];
        avPolygon[ 1] = avVtxCopy[iNextVtx];
        avPolygon[ 2] = avVtxCopy[iNextNextVtx];
        CObjectPolygon *pObjectPolygon = _poscSector->CreatePolygon( 
          3, avPolygon, *_pomMaterial, NULL, bInvert);
        if( pObjectPolygon != NULL) {
          // set shadow cluster size to 2m
          ((CBrushPolygonProperties&)(pObjectPolygon->opo_ubUserData)).bpp_sbShadowClusterSize=2;
        }
      }
      break;
    }
  }
  delete[] avVtxCopy;
}

void CWorldEditorDoc::ConvertObject3DToBrush(CObject3D &ob, BOOL bApplyProjectedMapping/*=FALSE*/,
                                             INDEX iMipBrush/*=0*/, FLOAT fSwitchFactor/*=1E6f*/,
                                             BOOL bApplyDefaultPolygonProperties/*=TRUE*/)
{
  CObject3D obTmp = ob;
  obTmp.RecalculatePlanes();

  // try to
  try
  {
    // turn this on to dump all primitives
    #ifndef NDEBUG
    //theApp.m_vfpCurrent.vfp_o3dPrimitive.DebugDump();
    #endif //NDEBUG
    if( !obTmp.ArePolygonsPlanar())
    {
      throw( "ERROR: Primitive that You want to use has non planar polygons.\n"
        "Make sure that stretch x and stretch y are same or use triangularisation.");
    }

    if( bApplyProjectedMapping)
    {
      DOUBLE xMin = theApp.m_vfpCurrent.vfp_fXMin;
      DOUBLE xMax = theApp.m_vfpCurrent.vfp_fXMax;
      DOUBLE zMin = theApp.m_vfpCurrent.vfp_fZMin;
      DOUBLE zMax = theApp.m_vfpCurrent.vfp_fZMax;
      // create mapping to be stretched over the top of primitive
      FLOATplane3D plHorizontal(FLOAT3D(0,1,0),0);
      CMappingDefinitionUI mduiHorizontal;
      mduiHorizontal.mdui_aURotation = mduiHorizontal.mdui_aVRotation = AngleDeg(90.0f);
      mduiHorizontal.mdui_fUStretch = FLOAT((xMax-xMin)/_fTextureWidth);
      mduiHorizontal.mdui_fVStretch = FLOAT((zMax-zMin)/_fTextureHeight);
      mduiHorizontal.mdui_fUOffset = FLOAT(xMin)/mduiHorizontal.mdui_fUStretch;
      mduiHorizontal.mdui_fVOffset = FLOAT(zMin)/mduiHorizontal.mdui_fVStretch;
      CMappingDefinition mdHorizontal;
      mdHorizontal.FromUI(mduiHorizontal);

      // for each polygon in primitive
      CObject3D &ob = obTmp;
      ob.ob_aoscSectors.Lock();
      CObjectSector &osc = ob.ob_aoscSectors[0];
      osc.osc_aopoPolygons.Lock();
      FOREACHINDYNAMICARRAY( osc.osc_aopoPolygons, CObjectPolygon, itopo) {
        CObjectPolygon &opo = *itopo;
        // project mapping to the polygon
        opo.opo_amdMappings[0].ProjectMapping(plHorizontal, mdHorizontal,
          DOUBLEtoFLOAT(*opo.opo_Plane));
        opo.opo_amdMappings[1] = opo.opo_amdMappings[0];
        opo.opo_amdMappings[2] = opo.opo_amdMappings[0];
      }
      osc.osc_aopoPolygons.Unlock();
      ob.ob_aoscSectors.Unlock();
    }

    // convert it into brush
    CBrush3D *pbr = m_penPrimitive->GetBrush();
    if( iMipBrush == 0)
    {
      pbr->Clear();
    }

    pbr->AddMipBrushFromObject3D_t(obTmp, fSwitchFactor);
    
    if( bApplyDefaultPolygonProperties)
    {
      // --- Apply default values for primitive polygons ---
      // for each mip in its brush
      FOREACHINLIST(CBrushMip, bm_lnInBrush, pbr->br_lhBrushMips, itbm) {
        // for all sectors in this mip
        FOREACHINDYNAMICARRAY(itbm->bm_abscSectors, CBrushSector, itbsc) {
          // for all polygons in sector
          FOREACHINSTATICARRAY(itbsc->bsc_abpoPolygons, CBrushPolygon, itbpo)
          {
            itbpo->CopyPropertiesWithoutTexture( *theApp.m_pbpoPolygonWithDeafultValues);
          }
        }
      }
    }
    pbr->CalculateBoundingBoxes();
  }
  // report errors
  catch (const char *err_str)
  {
    AfxMessageBox( CString(err_str));
  }
}

void CWorldEditorDoc::ApplyAutoColorize(void)
{
  // if primitive auto colorization is on
  if( theApp.m_Preferences.ap_bAutoColorize)
  {
    theApp.m_vfpCurrent.vfp_colPolygonsColor = acol_ColorizePallete[theApp.m_iLastAutoColorizeColor];
    theApp.m_vfpCurrent.vfp_colSectorsColor = theApp.m_vfpCurrent.vfp_colPolygonsColor;
  }
}

void CWorldEditorDoc::CreateConusPrimitive(void)
{
  /*
  // report it
  _RPT0(_CRT_WARN, "\nConus\n");
  */
  // calculate height
  DOUBLE fHeight = theApp.m_vfpCurrent.vfp_fYMax-theApp.m_vfpCurrent.vfp_fYMin;
  if( fHeight < SNAP_FLOAT_GRID) fHeight = SNAP_FLOAT_GRID;
  // get count of vertices on the base
  INDEX vtxCt = theApp.m_vfpCurrent.vfp_avVerticesOnBaseOfPrimitive.Count();
  // get shear values
  DOUBLE dx = theApp.m_vfpCurrent.vfp_fShearX;
  DOUBLE dz = theApp.m_vfpCurrent.vfp_fShearZ;
  // get stretch value for top-base vertices
  DOUBLE fStretchX = theApp.m_vfpCurrent.vfp_fStretchX;
  DOUBLE fStretchY = theApp.m_vfpCurrent.vfp_fStretchY;

  // base polygon
  DOUBLE3D *avBottomPolygon = new DOUBLE3D[ vtxCt];
  for(INDEX iVtxBottom=0; iVtxBottom<vtxCt; iVtxBottom++)
  {
    avBottomPolygon[ iVtxBottom] = theApp.m_vfpCurrent.vfp_avVerticesOnBaseOfPrimitive[ iVtxBottom];
    avBottomPolygon[ iVtxBottom](2) = theApp.m_vfpCurrent.vfp_fYMin;
  }
  AddPolygon( vtxCt, avBottomPolygon, _bClosed);

  // top polygon
  DOUBLE3D *avTopPolygon = new DOUBLE3D[ vtxCt];
  for(INDEX iVtxTop=0; iVtxTop<vtxCt; iVtxTop++)
  {
    avTopPolygon[ iVtxTop] = DOUBLE3D( 
     theApp.m_vfpCurrent.vfp_avVerticesOnBaseOfPrimitive[ iVtxTop](1) * fStretchX + dx,
     theApp.m_vfpCurrent.vfp_fYMax,
     theApp.m_vfpCurrent.vfp_avVerticesOnBaseOfPrimitive[ iVtxTop](3) * fStretchY + dz);
  }
  if( (fStretchX != 0.0f) && (fStretchY != 0.0f) )
  {
    AddPolygon( vtxCt, avTopPolygon, !_bClosed);
  }

  // side polygons
  DOUBLE3D *avSidePolygon = new DOUBLE3D[ 4];
  for( INDEX iBaseVtx=0; iBaseVtx<vtxCt; iBaseVtx++)
  {
    INDEX iNextVtx = (iBaseVtx+1) % vtxCt;
    avSidePolygon[ 0] = avBottomPolygon[ iBaseVtx];
    avSidePolygon[ 0](2) = theApp.m_vfpCurrent.vfp_fYMin;
    avSidePolygon[ 1] = avTopPolygon[ iBaseVtx];
    avSidePolygon[ 1](2) = theApp.m_vfpCurrent.vfp_fYMax;
    avSidePolygon[ 2] = avTopPolygon[ iNextVtx];
    avSidePolygon[ 2](2) = theApp.m_vfpCurrent.vfp_fYMax;
    avSidePolygon[ 3] = avBottomPolygon[ iNextVtx];
    avSidePolygon[ 3](2) = theApp.m_vfpCurrent.vfp_fYMin;

    // get lenght of base edge
    DOUBLE fEdgeLenght = 
      (theApp.m_vfpCurrent.vfp_avVerticesOnBaseOfPrimitive[ 1] -
       theApp.m_vfpCurrent.vfp_avVerticesOnBaseOfPrimitive[ 0]).Length();
    // calculate how many times we wrapped (tiled) one texture
    INDEX iWrappedTimes = INDEX( (iBaseVtx * fEdgeLenght) / _fTextureWidth);
    // prepare vector of mapping translation (allign to left)
    DOUBLE3D f3dMappingTranslation = avSidePolygon[ 0];
    // add primitive-texture height difference to start from top
    f3dMappingTranslation -= DOUBLE3D( 0.0f, fHeight-_fTextureHeight, 0.0f);
    // calculate last edge's mapping remainings for "continous" mapping
    DOUBLE fMappingRemaining = iBaseVtx*fEdgeLenght - iWrappedTimes*_fTextureWidth;
    // calculate edge vector going from end toward start vertice of base edge
    DOUBLE3D f3dEdge = avSidePolygon[ 3] - avSidePolygon[ 0];
    // normalize it
    f3dEdge.Normalize();
    // multiply it with mapping remaining value
    f3dEdge *= fMappingRemaining;
    // add its influence into mapping translation vector
    f3dMappingTranslation += f3dEdge;

    // create polygons on side of conus
    AddPolygon( 4, avSidePolygon, _bClosed, f3dMappingTranslation);
  }
  delete avBottomPolygon;
  delete avTopPolygon;
  delete avSidePolygon;
  theApp.m_vfpCurrent.vfp_o3dPrimitive.Optimize();
  ConvertObject3DToBrush(theApp.m_vfpCurrent.vfp_o3dPrimitive);
}

void CWorldEditorDoc::CreateTorusPrimitive(void)
{
  /*
  // report it
  _RPT0(_CRT_WARN, "\nTorus\n");
  */

  // get count of vertices that will be used for creating base polygon
  INDEX vtxCt = theApp.m_vfpCurrent.vfp_avVerticesOnBaseOfPrimitive.Count();
  // get torus parameters
  INDEX iSlicesIn360 = theApp.m_vfpCurrent.vfp_iSlicesIn360;
  INDEX iNoOfSlices = theApp.m_vfpCurrent.vfp_iNoOfSlices;
  FLOAT fHeight = theApp.m_vfpCurrent.vfp_fYMax-theApp.m_vfpCurrent.vfp_fYMin;
  // clamp no of slices
  if(iNoOfSlices<1) iNoOfSlices=1;
  //if(iNoOfSlices>iSlicesIn360) iNoOfSlices=iSlicesIn360;

  DOUBLE3D **papvBases = new DOUBLE3D *[iNoOfSlices+1];
  DOUBLE3D vCenter = DOUBLE3D( theApp.m_vfpCurrent.vfp_fRadius, 0.0f, 0.0f);
  BOOL bInvert = _bClosed;

  // rotate base vertices to calculate torus slices
  INDEX iSlice=0;
  for( ; iSlice<iNoOfSlices+1; iSlice++)
  {
    // create rotation matrix
    ANGLE3D angSlice = ANGLE3D( 0, 0, AngleDeg( (360.0f/iSlicesIn360)*iSlice));
    if( theApp.m_vfpCurrent.vfp_fRadius>0.0f)
    {
      angSlice(3) = -angSlice(3);
    }
    DOUBLEmatrix3D matrixRot;
    matrixRot ^= angSlice;
    papvBases[iSlice] = new DOUBLE3D[vtxCt];
    // calculate vertex coordinates for each slice
    for(INDEX iBaseVtx=0; iBaseVtx<vtxCt; iBaseVtx++)
    {
      // create vector from center to rotating vertex
      DOUBLE3D vCT = theApp.m_vfpCurrent.vfp_avVerticesOnBaseOfPrimitive[ iBaseVtx]-vCenter;
      papvBases[iSlice][iBaseVtx] = vCT*matrixRot+vCenter;
      papvBases[iSlice][iBaseVtx](3) += iSlice*fHeight;
    }
  }

  if(iNoOfSlices!=iSlicesIn360 || fHeight!=0)
  {
    // create torus starting polygon
    AddPolygon( vtxCt, papvBases[0], bInvert);
    // create torus ending polygon
    AddPolygon( vtxCt, papvBases[iNoOfSlices], !bInvert);
  }

  DOUBLE3D avPolygonVertices[4];
  // create polygons on sides
  for(iSlice=0; iSlice<iNoOfSlices; iSlice++)
  {
    INDEX iNextSlice = (iSlice+1)%(iNoOfSlices+1);
    for(INDEX iVtx=0; iVtx<vtxCt; iVtx++)         
    {
      INDEX iNextVtx = (iVtx+1)%vtxCt;
      avPolygonVertices[0] = papvBases[iSlice][iVtx];
      avPolygonVertices[1] = papvBases[iSlice][iNextVtx];
      avPolygonVertices[2] = papvBases[iNextSlice][iNextVtx];
      avPolygonVertices[3] = papvBases[iNextSlice][iVtx];
      AddPolygon( 4, avPolygonVertices, !bInvert);
    }
  }
  // free allocated arrays
  for( INDEX iFree=0; iFree<iNoOfSlices+1; iFree++)
  {
    delete papvBases[iFree];
  }
  delete papvBases;

  theApp.m_vfpCurrent.vfp_o3dPrimitive.Optimize();
  ConvertObject3DToBrush(theApp.m_vfpCurrent.vfp_o3dPrimitive);
}

void CWorldEditorDoc::CreateStaircasesPrimitive(void)
{
  /*
  // report it
  _RPT0(_CRT_WARN, "\nStaircases\n");
  */
  // calculate height
  DOUBLE fWidth = theApp.m_vfpCurrent.vfp_fXMax-theApp.m_vfpCurrent.vfp_fXMin;
  if( fWidth < SNAP_FLOAT_GRID) fWidth = SNAP_FLOAT_GRID;
  DOUBLE fHeight = theApp.m_vfpCurrent.vfp_fYMax-theApp.m_vfpCurrent.vfp_fYMin;
  if( fHeight < SNAP_FLOAT_GRID) fHeight = SNAP_FLOAT_GRID;
  DOUBLE fLenght = (theApp.m_vfpCurrent.vfp_fZMax-theApp.m_vfpCurrent.vfp_fZMin);
  if( fLenght < SNAP_FLOAT_12) fLenght = SNAP_FLOAT_12;
  
  // get parameters for staircases 
  INDEX iStairsIn360 = theApp.m_vfpCurrent.vfp_iSlicesIn360;
  INDEX iNoOfStairs = theApp.m_vfpCurrent.vfp_iNoOfSlices;

  DOUBLE fRadius = theApp.m_vfpCurrent.vfp_fRadius;
  DOUBLE angle = 360.0/iStairsIn360;
  DOUBLE angleAdd = 0.0;
  // if base is created inside circle
  if( theApp.m_vfpCurrent.vfp_bOuter && !theApp.m_vfpCurrent.vfp_bLinearStaircases) 
  {
    fRadius = fRadius/Cos(FLOAT(angle/2));
    angleAdd = -angle/2.0;
    fWidth = fWidth/Cos(FLOAT(angle/2));
  }

  BOOL bTopSlope = theApp.m_vfpCurrent.vfp_iTopShape == 1;
  BOOL bTopCeiling = theApp.m_vfpCurrent.vfp_iTopShape == 2;
  BOOL bBottomSlope = theApp.m_vfpCurrent.vfp_iBottomShape == 1;
  BOOL bBottomFloor = theApp.m_vfpCurrent.vfp_iBottomShape == 2;

  DOUBLE3D **papvBases = new DOUBLE3D *[iNoOfStairs];
  DOUBLE3D vCenter = DOUBLE3D( 0.0f, 0.0f, 0.0f);

  BOOL bInvert = _bClosed;

  // rotate base vertices to calculate rotating staircases
  INDEX iStair=0;
  for( ; iStair<iNoOfStairs; iStair++)
  {
    // create rotation matrix
    ANGLE3D angRotation1;
    ANGLE3D angRotation2;
    
    if( fRadius>0.0f)
    {
      angRotation1 = ANGLE3D( -AngleDeg( FLOAT((angle)*iStair+angleAdd)), 0, 0);
      angRotation2 = ANGLE3D( -AngleDeg( FLOAT((angle)*(iStair+1)+angleAdd)), 0, 0);
    }
    else
    {
      angRotation1 = ANGLE3D( AngleDeg( FLOAT((angle)*iStair+angleAdd)), 0, 0);
      angRotation2 = ANGLE3D( AngleDeg( FLOAT((angle)*(iStair+1)+angleAdd)), 0, 0);
    }
    
    DOUBLEmatrix3D matrixRot1;
    matrixRot1 ^= angRotation1;
    DOUBLEmatrix3D matrixRot2;
    matrixRot2 ^= angRotation2;
    papvBases[iStair] = new DOUBLE3D[4];
    if( theApp.m_vfpCurrent.vfp_bLinearStaircases)
    {
      papvBases[iStair][0] = DOUBLE3D( -fWidth/2.0f, fHeight*iStair, -fLenght*iStair);
      papvBases[iStair][1] = DOUBLE3D( -fWidth/2.0f, fHeight*iStair, -fLenght*(iStair+1));
      papvBases[iStair][2] = DOUBLE3D( fWidth/2.0f, fHeight*iStair, -fLenght*(iStair+1));
      papvBases[iStair][3] = DOUBLE3D( fWidth/2.0f, fHeight*iStair, -fLenght*iStair);
    }
    else
    {
      // calculate vertex coordinates for each rotating stair base
      for(INDEX iBaseVtx=0; iBaseVtx<4; iBaseVtx++)
      {
        DOUBLE3D vCT1, vCT2;
        // create vector from center to rotating vertex
        if( fRadius>0.0f)
        {
          vCT1 = DOUBLE3D(-fRadius,0.0,0.0)-vCenter;
          vCT2 = DOUBLE3D(-fRadius+fWidth,0.0,0.0)-vCenter;
        }
        else
        {
          vCT1 = DOUBLE3D(-fRadius-fWidth,0.0,0.0)-vCenter;
          vCT2 = DOUBLE3D(-fRadius,0.0,0.0)-vCenter;
        }

        papvBases[iStair][3] = vCT2*matrixRot1+vCenter;
        papvBases[iStair][3](2) = fHeight*iStair;
        papvBases[iStair][2] = vCT2*matrixRot2+vCenter;
        papvBases[iStair][2](2) = fHeight*iStair;
        papvBases[iStair][1] = vCT1*matrixRot2+vCenter;
        papvBases[iStair][1](2) = fHeight*iStair;
        papvBases[iStair][0] = vCT1*matrixRot1+vCenter;
        papvBases[iStair][0](2) = fHeight*iStair;
      }
    }
  }

  DOUBLE3D avVtx[8];
  // create polygons of stairs
  for(iStair=0; iStair<iNoOfStairs; iStair++)
  {
    // add only first front polygon if rest are eaten by stairs material
    BOOL bAddFrontVerticalPolygon = TRUE;
    if( (bTopSlope || bTopCeiling) && (iStair != 0) )
      bAddFrontVerticalPolygon = FALSE;

    // add only last back polygon if rest are eaten by stairs material
    BOOL bAddBackVerticalPolygon = TRUE;
    if( (bBottomSlope || bBottomFloor) && (iStair != (iNoOfStairs-1)) )
      bAddBackVerticalPolygon = FALSE;

    avVtx[0] = papvBases[iStair][0];
    avVtx[1] = papvBases[iStair][1];
    avVtx[2] = papvBases[iStair][2];
    avVtx[3] = papvBases[iStair][3];
    avVtx[4] = papvBases[iStair][0];
    avVtx[4](2) += fHeight;

    avVtx[5] = papvBases[iStair][1];
    avVtx[5](2) += fHeight;
    avVtx[6] = papvBases[iStair][2];
    avVtx[6](2) += fHeight;
    avVtx[7] = papvBases[iStair][3];
    avVtx[7](2) += fHeight;

    // if bottom shape is slope
    if( bBottomSlope)
    {
      avVtx[1](2) += fHeight;
      avVtx[2](2) += fHeight;
    }
    // if top shape is slope
    if( bTopSlope)
    {
      avVtx[5](2) += fHeight;
      avVtx[6](2) += fHeight;
    }
    DOUBLE3D avPolygon[4];
    avPolygon[0] = avVtx[0];
    avPolygon[1] = avVtx[1];
    avPolygon[2] = avVtx[5];
    avPolygon[3] = avVtx[4];
    // if bottom shape is floor
    if( bBottomFloor)
    {
      avPolygon[0](2) = 0.0;
      avPolygon[1](2) = 0.0;
    }
    // if top shape is ceiling
    if( bTopCeiling)
    {
      avPolygon[2](2) = fHeight*iNoOfStairs;
      avPolygon[3](2) = fHeight*iNoOfStairs;
    }
    AddPolygon( 4, avPolygon, !bInvert);
    avPolygon[0] = avVtx[3];
    avPolygon[1] = avVtx[2];
    avPolygon[2] = avVtx[6];
    avPolygon[3] = avVtx[7];
    // if bottom shape is floor
    if( bBottomFloor)
    {
      avPolygon[0](2) = 0.0;
      avPolygon[1](2) = 0.0;
    }
    // if top shape is ceiling
    if( bTopCeiling)
    {
      avPolygon[2](2) = fHeight*iNoOfStairs;
      avPolygon[3](2) = fHeight*iNoOfStairs;
    }
    AddPolygon( 4, avPolygon, bInvert);
    if( bAddFrontVerticalPolygon)
    {
      avPolygon[0] = avVtx[0];
      avPolygon[1] = avVtx[4];
      avPolygon[2] = avVtx[7];
      avPolygon[3] = avVtx[3];
      // if top shape is ceiling
      if( bTopCeiling)
      {
        avPolygon[1](2) = fHeight*iNoOfStairs;
        avPolygon[2](2) = fHeight*iNoOfStairs;
      }
      AddPolygon( 4, avPolygon, !bInvert);
    }

    // if bottom shape is slope, top is stairs, don't create polygon because its area is 0
    if( !(bBottomSlope && !bTopSlope) && bAddBackVerticalPolygon)
    {
      avPolygon[0] = avVtx[1];
      avPolygon[1] = avVtx[5];
      avPolygon[2] = avVtx[6];
      avPolygon[3] = avVtx[2];
      if( bBottomFloor)
      {
        avPolygon[0](2) = 0.0;
        avPolygon[3](2) = 0.0;
      }
      AddPolygon( 4, avPolygon, bInvert);
    }
    // if top shape is slope
    if( bTopSlope == 1)
    {
      avPolygon[0] = avVtx[4];
      avPolygon[1] = avVtx[6];
      avPolygon[2] = avVtx[7];
      AddPolygon( 3, avPolygon, !bInvert);
      avPolygon[0] = avVtx[4];
      avPolygon[1] = avVtx[5];
      avPolygon[2] = avVtx[6];
      AddPolygon( 3, avPolygon, !bInvert);
    }
    else
    {
      avPolygon[0] = avVtx[4];
      avPolygon[1] = avVtx[5];
      avPolygon[2] = avVtx[6];
      avPolygon[3] = avVtx[7];
      // if top shape is ceiling
      if( bTopCeiling)
      {
        avPolygon[0](2) = fHeight*iNoOfStairs;
        avPolygon[1](2) = fHeight*iNoOfStairs;
        avPolygon[2](2) = fHeight*iNoOfStairs;
        avPolygon[3](2) = fHeight*iNoOfStairs;
      }
      AddPolygon( 4, avPolygon, !bInvert);
    }
    // if bottom shape is slope
    if( bBottomSlope == 1)
    {
      avPolygon[0] = avVtx[0];
      avPolygon[1] = avVtx[2];
      avPolygon[2] = avVtx[3];
      AddPolygon( 3, avPolygon, bInvert);
      avPolygon[0] = avVtx[0];
      avPolygon[1] = avVtx[1];
      avPolygon[2] = avVtx[2];
      AddPolygon( 3, avPolygon, bInvert);
    }
    else
    {
      avPolygon[0] = avVtx[0];
      avPolygon[1] = avVtx[1];
      avPolygon[2] = avVtx[2];
      avPolygon[3] = avVtx[3];
      // if bottom shape is floor
      if( bBottomFloor)
      {
        avPolygon[0](2) = 0.0;
        avPolygon[1](2) = 0.0;
        avPolygon[2](2) = 0.0;
        avPolygon[3](2) = 0.0;
      }
      AddPolygon( 4, avPolygon, bInvert);
    }
  }
  // free allocated arrays
  for( INDEX iFree=0; iFree<iNoOfStairs; iFree++)
  {
    delete papvBases[iFree];
  }
  delete papvBases;

  theApp.m_vfpCurrent.vfp_o3dPrimitive.Optimize();
  ConvertObject3DToBrush(theApp.m_vfpCurrent.vfp_o3dPrimitive);
}

void CWorldEditorDoc::CreateSpherePrimitive(void)
{
  // calculate width, lenght and height but as radiuses !!! (divided by 2)
  DOUBLE fWidth = (theApp.m_vfpCurrent.vfp_fXMax-theApp.m_vfpCurrent.vfp_fXMin)/2.0;
  if( fWidth < SNAP_FLOAT_GRID) fWidth = SNAP_FLOAT_GRID;
  DOUBLE fHeight = (theApp.m_vfpCurrent.vfp_fYMax-theApp.m_vfpCurrent.vfp_fYMin)/2.0;
  if( fHeight < SNAP_FLOAT_GRID) fHeight = SNAP_FLOAT_GRID;
  DOUBLE fLenght = (theApp.m_vfpCurrent.vfp_fZMax-theApp.m_vfpCurrent.vfp_fZMin)/2.0;
  if( fLenght < SNAP_FLOAT_12) fLenght = SNAP_FLOAT_12;
  // get parameters for staircases 
  INDEX iMeridians = theApp.m_vfpCurrent.vfp_iMeridians;
  INDEX iParalels = theApp.m_vfpCurrent.vfp_iParalels;
  BOOL bInvert = _bClosed;
  
  // calculate bases
  DOUBLE3D **papvSlices = new DOUBLE3D *[iParalels+1];
  ANGLE angleSliceDelta = AngleDeg(180.0f)/iParalels;
  ANGLE angleSlice = -90.0f;
  // calculate all slices on sphere
  INDEX iSlice=0;
  for( ; iSlice<iParalels+1; iSlice++)
  {
    DOUBLE fSliceHeight, dA, dB;
    // if equal slices
    if( theApp.m_vfpCurrent.vfp_bLinearStaircases)
    {
      fSliceHeight = -fHeight + (fHeight*2/iParalels*iSlice);
    }
    else
    {
      fSliceHeight = Sin( angleSlice) * fHeight;
    }

    // snap X coordinate (1 cm)
    //Snap(fSliceHeight, SNAP_DOUBLE_CM);
    // calculate width and lenght of elipse for current slice
    dA =  sqrt((fWidth*fWidth*fHeight*fHeight-
                             fWidth*fWidth*fSliceHeight*fSliceHeight)/(fHeight*fHeight));
    dB =  sqrt((fLenght*fLenght*fHeight*fHeight-
                             fLenght*fLenght*fSliceHeight*fSliceHeight)/(fHeight*fHeight));

    
    // array for vertices of this slice
    papvSlices[iSlice] = new DOUBLE3D[iMeridians];
    ANGLE angle = AngleDeg(360.0f)/iMeridians;
    ANGLE angleCt = 0;
    for( INDEX iVtx=0; iVtx<iMeridians; iVtx++)
    {
      DOUBLE x = Cos( angleCt) * dA + (theApp.m_vfpCurrent.vfp_fXMin+theApp.m_vfpCurrent.vfp_fXMax)/2.0f;
      DOUBLE z = Sin( angleCt) * dB + (theApp.m_vfpCurrent.vfp_fZMin+theApp.m_vfpCurrent.vfp_fZMax)/2.0f;
      // snap X coordinate (1 cm)
      //Snap(x, SNAP_DOUBLE_CM);
      // snap Y coordinate (1 cm)
      //Snap(z, SNAP_DOUBLE_CM);
      papvSlices[iSlice][iVtx] = DOUBLE3D( x, fSliceHeight, z);
      angleCt += angle;
    }
    angleSlice += angleSliceDelta;
  }
  DOUBLE3D avPolygon[4];
  // create polygons
  for( iSlice=0; iSlice<iParalels; iSlice++)
  {
    INDEX iNextSlice = iSlice+1;
    for( INDEX iVtx=0; iVtx<iMeridians; iVtx++)
    {
      INDEX iNextVtx = (iVtx+1)%iMeridians;
      if (iSlice == 0) {
        avPolygon[0] = papvSlices[iNextSlice][iVtx];
        avPolygon[1] = papvSlices[iNextSlice][iNextVtx];
        avPolygon[2] = papvSlices[iSlice][0];
        AddPolygon( 3, avPolygon, bInvert);
      } else if (iSlice == iParalels-1) {
        avPolygon[0] = papvSlices[iNextSlice][0];
        avPolygon[1] = papvSlices[iSlice][iNextVtx];
        avPolygon[2] = papvSlices[iSlice][iVtx];
        AddPolygon( 3, avPolygon, bInvert);
      } else {
        avPolygon[0] = papvSlices[iNextSlice][iVtx];
        avPolygon[1] = papvSlices[iNextSlice][iNextVtx];
        avPolygon[2] = papvSlices[iSlice][iNextVtx];
        avPolygon[3] = papvSlices[iSlice][iVtx];
        AddPolygon( 4, avPolygon, bInvert);
      }
    }
  }

  // free allocated arrays
  for( INDEX iFree=0; iFree<iParalels+1; iFree++)
  {
    delete papvSlices[iFree];
  }
  delete papvSlices;

  theApp.m_vfpCurrent.vfp_o3dPrimitive.Optimize();
  ConvertObject3DToBrush(theApp.m_vfpCurrent.vfp_o3dPrimitive);
}


void InitializeObject3DForPrimitive(void)
{
  // clear Object3D that will be used for creating primitive
  theApp.m_vfpCurrent.vfp_o3dPrimitive.Clear();
  // create sector
  _poscSector = theApp.m_vfpCurrent.vfp_o3dPrimitive.ob_aoscSectors.New(1);
  _poscSector->osc_colColor = theApp.m_vfpCurrent.vfp_colSectorsColor;
  // create material
  _pomMaterial = _poscSector->osc_aomtMaterials.New(1);
  _pPrimitiveTexture = theApp.m_ptdActiveTexture;
  _fTextureWidth = METERS_MEX( _pPrimitiveTexture->GetWidth());
  _fTextureHeight = METERS_MEX( _pPrimitiveTexture->GetHeight());
  *_pomMaterial = CObjectMaterial( _pPrimitiveTexture->GetName());
  _pomMaterial->SetColor( theApp.m_vfpCurrent.vfp_colPolygonsColor);
  // Pick up primitive-related variables
  _bClosed = theApp.m_vfpCurrent.vfp_bClosed;
  _bAutoCreateMipBrushes = theApp.m_vfpCurrent.vfp_bAutoCreateMipBrushes;
}

void GetTerrainPolygonEdges(CObjectSector &osec, INDEX iPolygon, INDEX iSlicesX, INDEX iSlicesZ, 
                            CObjectEdge *&poe0, CObjectEdge *&poe1, CObjectEdge *&poe2,
                            CObjectEdge *&poe3, CObjectEdge *&poe4);

void CWorldEditorDoc::CreateTerrainPrimitive(void)
{
  CImageInfo iiDisplace;
  CImageInfo *piiDisplace = &iiDisplace;
  if( theApp.m_vfpCurrent.vfp_fnDisplacement != "")
  {
    try
    {
      iiDisplace.LoadAnyGfxFormat_t( theApp.m_vfpCurrent.vfp_fnDisplacement);
      m_slDisplaceTexTime=GetFileTimeStamp_t(theApp.m_vfpCurrent.vfp_fnDisplacement);
    }
    catch (const char *strError)
    {
      (void) strError;
      piiDisplace = NULL;
    }
  }
  else
  {
    piiDisplace = NULL;
  }
  
  // get parameters for slices
  INDEX iSlicesX = theApp.m_vfpCurrent.vfp_iSlicesPerWidth;
  if( iSlicesX < 1) iSlicesX = 1;
  INDEX iSlicesZ = theApp.m_vfpCurrent.vfp_iSlicesPerHeight;
  if( iSlicesZ < 1) iSlicesZ = 1;

  INDEX iMip=0;
  // auto create mip brushes
  while( iSlicesX>=1 && iSlicesZ>=1 && ((iMip==0)||_bAutoCreateMipBrushes))
  {
    CreateTerrainObject3D( piiDisplace, iSlicesX, iSlicesZ, iMip);
    ConvertObject3DToBrush(theApp.m_vfpCurrent.vfp_o3dPrimitive, TRUE, iMip, 5.0f+iMip*1.5, FALSE);
    iSlicesX /= 2;
    iSlicesZ /= 2;
    iMip++;
  }
}

void CWorldEditorDoc::CreateTerrainObject3D( CImageInfo *piiDisplace, INDEX iSlicesX, INDEX iSlicesZ, INDEX iMip)
{
  // calculate width, lenght and heigth
  DOUBLE fWidth = (theApp.m_vfpCurrent.vfp_fXMax-theApp.m_vfpCurrent.vfp_fXMin);
  if( fWidth < SNAP_FLOAT_GRID) fWidth = SNAP_FLOAT_GRID;
  DOUBLE fHeight = (theApp.m_vfpCurrent.vfp_fYMax-theApp.m_vfpCurrent.vfp_fYMin);
  if( fHeight < SNAP_FLOAT_GRID) fHeight = SNAP_FLOAT_GRID;
  DOUBLE fLenght = (theApp.m_vfpCurrent.vfp_fZMax-theApp.m_vfpCurrent.vfp_fZMin);
  if( fLenght < SNAP_FLOAT_12) fLenght = SNAP_FLOAT_12;
  
  DOUBLE fMinX = theApp.m_vfpCurrent.vfp_fXMin;
  DOUBLE fMaxX = theApp.m_vfpCurrent.vfp_fXMax;
  DOUBLE fMinY = theApp.m_vfpCurrent.vfp_fYMin;
  DOUBLE fMaxY = theApp.m_vfpCurrent.vfp_fYMax;
  DOUBLE fMinZ = theApp.m_vfpCurrent.vfp_fZMin;
  DOUBLE fMaxZ = theApp.m_vfpCurrent.vfp_fZMax;

  DOUBLE fDX = fWidth/iSlicesX;
  DOUBLE fDZ = fLenght/iSlicesZ;

  FLOAT fAmplitude = theApp.m_vfpCurrent.vfp_fAmplitude;

  ULONG ulNonFllorPolygonFlags = BPOF_FULLBRIGHT|BPOF_DETAILPOLYGON|BPOF_PORTAL;
  if( !_bClosed)
  {
    // swap min and max y coordinates
    FLOAT fTemp = fMinY;
    fMinY = fMaxY;
    fMaxY = fTemp;
    ulNonFllorPolygonFlags = BPOF_FULLBRIGHT|BPOF_DETAILPOLYGON;
  }

  // initialize object 3D
  InitializeObject3DForPrimitive();
  // get sector reference
  CObjectSector &osec = *_poscSector;

  INDEX ctVertices = (iSlicesX+1)*(iSlicesZ+1)+4;
  osec.osc_aovxVertices.New(ctVertices);
  osec.osc_aovxVertices.Lock();

  // create 'floor' vertices
  {for( INDEX iz=0; iz<=iSlicesZ; iz++) {
    {for( INDEX ix=0; ix<=iSlicesX; ix++) {
      INDEX iVtx = iz*(iSlicesX+1)+ix;
      CObjectVertex &ov = osec.osc_aovxVertices[iVtx];
      ov = DOUBLE3D(fMinX+fDX*ix, fMinY, fMinZ+fDZ*iz);
      DisplaceVertex( ov, piiDisplace, fMinX, fMaxX, fMinZ, fMaxZ, iSlicesX, iSlicesZ, fAmplitude);
    }}
  }}
  // create four 'ceiling' vertices
#define START_OF_CEILING_VERTICES ((iSlicesX+1)*(iSlicesZ+1))
  INDEX iVtx = START_OF_CEILING_VERTICES;
  osec.osc_aovxVertices[iVtx+0] = DOUBLE3D(fMinX, fMaxY, fMaxZ);
  osec.osc_aovxVertices[iVtx+1] = DOUBLE3D(fMaxX, fMaxY, fMaxZ);
  osec.osc_aovxVertices[iVtx+2] = DOUBLE3D(fMaxX, fMaxY, fMinZ);
  osec.osc_aovxVertices[iVtx+3] = DOUBLE3D(fMinX, fMaxY, fMinZ);

  // allocate edges
  INDEX ctEdges = iSlicesX*(iSlicesZ+1)+iSlicesZ*(iSlicesX+1)+iSlicesX*iSlicesZ+8;
  osec.osc_aoedEdges.New(ctEdges);
  
  // create edges from vertices
  osec.osc_aoedEdges.Lock();
  // create horizontal edges
  {for( INDEX iz=0; iz<iSlicesZ+1; iz++)
  {
    {for( INDEX ix=0; ix<iSlicesX; ix++)
    {
      INDEX iVtx1 = iz*(iSlicesX+1)+ix;
      INDEX iEdg = iz*iSlicesX+ix;
      CObjectEdge &oedg = osec.osc_aoedEdges[iEdg];
      oedg.oed_Vertex0 = &osec.osc_aovxVertices[iVtx1];
      oedg.oed_Vertex1 = &osec.osc_aovxVertices[iVtx1+1];
    }
  }
  }}

  // create vertical edges
#define START_OF_VERTICAL_EDGES (iSlicesX*(iSlicesZ+1))
  {for( INDEX iz=0; iz<iSlicesZ; iz++)
  {
    {for( INDEX ix=0; ix<iSlicesX+1; ix++)
    {
      INDEX iVtx1 = iz*(iSlicesX+1)+ix;
      INDEX iEdg = START_OF_VERTICAL_EDGES + iz*(iSlicesX+1)+ix;
      CObjectEdge &oedg = osec.osc_aoedEdges[iEdg];
      oedg.oed_Vertex0 = &osec.osc_aovxVertices[iVtx1];
      oedg.oed_Vertex1 = &osec.osc_aovxVertices[iVtx1+(iSlicesX+1)];
    }
  }
  }}

  // create slope edges
#define START_OF_SLOPE_EDGES (iSlicesX*(iSlicesZ+1)+(iSlicesX+1)*iSlicesZ)
  {for( INDEX iz=0; iz<iSlicesZ; iz++)
  {
    {for( INDEX ix=0; ix<iSlicesX; ix++)
    {
      INDEX iVtx1 = iz*(iSlicesX+1)+ix;
      INDEX iEdg = START_OF_SLOPE_EDGES + iz*iSlicesX+ix;
      CObjectEdge &oedg = osec.osc_aoedEdges[iEdg];
      oedg.oed_Vertex0 = &osec.osc_aovxVertices[iVtx1];
      oedg.oed_Vertex1 = &osec.osc_aovxVertices[iVtx1+iSlicesX+1+1];
    }
  }
  }}

  // create border edges
#define START_OF_BORDER_EDGES (iSlicesX*(iSlicesZ+1)+(iSlicesX+1)*iSlicesZ+iSlicesX*iSlicesZ)
  {
    // 0
    INDEX iEdg = START_OF_BORDER_EDGES + 0;
    CObjectEdge &oedg = osec.osc_aoedEdges[iEdg];
    INDEX iVtx0 = (iSlicesX+1)*iSlicesZ;
    oedg.oed_Vertex0 = &osec.osc_aovxVertices[iVtx0];
    oedg.oed_Vertex1 = &osec.osc_aovxVertices[START_OF_CEILING_VERTICES+0];
  }
  {
    // 1
    INDEX iEdg = START_OF_BORDER_EDGES + 1;
    CObjectEdge &oedg = osec.osc_aoedEdges[iEdg];
    INDEX iVtx0 = (iSlicesX+1)*(iSlicesZ+1)-1;
    oedg.oed_Vertex0 = &osec.osc_aovxVertices[iVtx0];
    oedg.oed_Vertex1 = &osec.osc_aovxVertices[START_OF_CEILING_VERTICES+1];
  }
  {
    // 2
    INDEX iEdg = START_OF_BORDER_EDGES + 2;
    CObjectEdge &oedg = osec.osc_aoedEdges[iEdg];
    INDEX iVtx0 = iSlicesX;
    oedg.oed_Vertex0 = &osec.osc_aovxVertices[iVtx0];
    oedg.oed_Vertex1 = &osec.osc_aovxVertices[START_OF_CEILING_VERTICES+2];
  }
  {
    // 3
    INDEX iEdg = START_OF_BORDER_EDGES + 3;
    CObjectEdge &oedg = osec.osc_aoedEdges[iEdg];
    INDEX iVtx0 = 0;
    oedg.oed_Vertex0 = &osec.osc_aovxVertices[iVtx0];
    oedg.oed_Vertex1 = &osec.osc_aovxVertices[START_OF_CEILING_VERTICES+3];
  }

  // create ceiling edges
#define START_OF_CEILING_EDGES (START_OF_BORDER_EDGES + 4)
  {
    // 0
    INDEX iEdg = START_OF_CEILING_EDGES + 0;
    CObjectEdge &oedg = osec.osc_aoedEdges[iEdg];
    oedg.oed_Vertex0 = &osec.osc_aovxVertices[START_OF_CEILING_VERTICES];
    oedg.oed_Vertex1 = &osec.osc_aovxVertices[START_OF_CEILING_VERTICES+1];
  }
  {
    // 1
    INDEX iEdg = START_OF_CEILING_EDGES + 1;
    CObjectEdge &oedg = osec.osc_aoedEdges[iEdg];
    oedg.oed_Vertex0 = &osec.osc_aovxVertices[START_OF_CEILING_VERTICES+1];
    oedg.oed_Vertex1 = &osec.osc_aovxVertices[START_OF_CEILING_VERTICES+2];
  }
  {
    // 2
    INDEX iEdg = START_OF_CEILING_EDGES + 2;
    CObjectEdge &oedg = osec.osc_aoedEdges[iEdg];
    oedg.oed_Vertex0 = &osec.osc_aovxVertices[START_OF_CEILING_VERTICES+2];
    oedg.oed_Vertex1 = &osec.osc_aovxVertices[START_OF_CEILING_VERTICES+3];
  }
  {
    // 3
    INDEX iEdg = START_OF_CEILING_EDGES + 3;
    CObjectEdge &oedg = osec.osc_aoedEdges[iEdg];
    oedg.oed_Vertex0 = &osec.osc_aovxVertices[START_OF_CEILING_VERTICES+3];
    oedg.oed_Vertex1 = &osec.osc_aovxVertices[START_OF_CEILING_VERTICES+0];
  }

	// get material
  CObjectMaterial &omat = *_pomMaterial;

  // allocate polygons and their planes
  INDEX ctPolygons = iSlicesX*iSlicesZ*2+4+1;
  osec.osc_aopoPolygons.New(ctPolygons);
  osec.osc_aoplPlanes.New(ctPolygons);
  
  osec.osc_aopoPolygons.Lock();
  osec.osc_aoplPlanes.Lock();

  // create floor polygons and their planes
  for( INDEX iPolygon=0; iPolygon<iSlicesX*iSlicesZ; iPolygon++)
  {
    // obtain edges of one broken checked polygon
    CObjectEdge *poe0, *poe1, *poe2, *poe3, *poe4;
    GetTerrainPolygonEdges(osec, iPolygon, iSlicesX, iSlicesZ, poe0, poe1, poe2, poe3, poe4);

    {
      // create upper polygon
      CObjectPlane &opl = osec.osc_aoplPlanes[iPolygon*2+0];
      opl = DOUBLEplane3D( *poe3->oed_Vertex1, *poe3->oed_Vertex0, *poe0->oed_Vertex0);

      CObjectPolygon &opo = osec.osc_aopoPolygons[iPolygon*2+0];
      opo.opo_Plane = &opl;
      // set polygon edges
      opo.opo_PolygonEdges.New(3);
      opo.opo_PolygonEdges.Lock();
      opo.opo_PolygonEdges[0].ope_Edge = poe0;
      opo.opo_PolygonEdges[0].ope_Backward = TRUE;
      opo.opo_PolygonEdges[1].ope_Edge = poe4;
      opo.opo_PolygonEdges[1].ope_Backward = FALSE;
      opo.opo_PolygonEdges[2].ope_Edge = poe3;
      opo.opo_PolygonEdges[2].ope_Backward = TRUE;
      opo.opo_PolygonEdges.Unlock();
      // set other polygon properties
      opo.opo_Material = &omat;
      opo.opo_ulFlags = BPOF_FULLBRIGHT|BPOF_DETAILPOLYGON;
      opo.opo_colorColor = theApp.m_vfpCurrent.vfp_colPolygonsColor;
    }

    {
      // create lower polygon
      CObjectPlane &opl = osec.osc_aoplPlanes[iPolygon*2+1];
      opl = DOUBLEplane3D( *poe1->oed_Vertex0, *poe1->oed_Vertex1, *poe2->oed_Vertex1);

      CObjectPolygon &opo = osec.osc_aopoPolygons[iPolygon*2+1];
      opo.opo_Plane = &opl;
      // set polygon edges
      opo.opo_PolygonEdges.New(3);
      opo.opo_PolygonEdges.Lock();
      opo.opo_PolygonEdges[0].ope_Edge = poe1;
      opo.opo_PolygonEdges[0].ope_Backward = FALSE;
      opo.opo_PolygonEdges[1].ope_Edge = poe2;
      opo.opo_PolygonEdges[1].ope_Backward = FALSE;
      opo.opo_PolygonEdges[2].ope_Edge = poe4;
      opo.opo_PolygonEdges[2].ope_Backward = TRUE;
      opo.opo_PolygonEdges.Unlock();
      // set other polygon properties
      opo.opo_Material = &omat;
      opo.opo_ulFlags = BPOF_FULLBRIGHT|BPOF_DETAILPOLYGON;
      opo.opo_colorColor = theApp.m_vfpCurrent.vfp_colPolygonsColor;
    }
  }

  // create side polygons and their planes
#define START_OF_SIDE_POLYGONS (iSlicesX*iSlicesZ*2)
  {
    // side polygon 0
    CObjectPolygon &opo = osec.osc_aopoPolygons[START_OF_SIDE_POLYGONS+0];
    CObjectEdge &oe0 = osec.osc_aoedEdges[ START_OF_BORDER_EDGES+0];
    CObjectEdge &oe1 = osec.osc_aoedEdges[ START_OF_CEILING_EDGES+0];

    CObjectPlane &opl = osec.osc_aoplPlanes[START_OF_SIDE_POLYGONS+0];
    opl = DOUBLEplane3D( *oe0.oed_Vertex0, *oe0.oed_Vertex1, *oe1.oed_Vertex1);
    opo.opo_Plane = &opl;
    opo.opo_PolygonEdges.New(iSlicesX+3);
    opo.opo_PolygonEdges.Lock();
    INDEX iEdg=0;
    for( ; iEdg<iSlicesX; iEdg++)
    {
      INDEX iEdgeIdx = iSlicesX*iSlicesZ+iEdg;
      opo.opo_PolygonEdges[iEdg].ope_Edge = &osec.osc_aoedEdges[ iEdgeIdx];
      opo.opo_PolygonEdges[iEdg].ope_Backward = TRUE;
    }
    opo.opo_PolygonEdges[iEdg+0].ope_Edge = &osec.osc_aoedEdges[ START_OF_BORDER_EDGES+0];
    opo.opo_PolygonEdges[iEdg+0].ope_Backward = FALSE;
    opo.opo_PolygonEdges[iEdg+1].ope_Edge = &osec.osc_aoedEdges[ START_OF_CEILING_EDGES+0];
    opo.opo_PolygonEdges[iEdg+1].ope_Backward = FALSE;
    opo.opo_PolygonEdges[iEdg+2].ope_Edge = &osec.osc_aoedEdges[ START_OF_BORDER_EDGES+1];
    opo.opo_PolygonEdges[iEdg+2].ope_Backward = TRUE;
    opo.opo_Material = &omat;
    opo.opo_ulFlags = ulNonFllorPolygonFlags;
    opo.opo_colorColor = theApp.m_vfpCurrent.vfp_colPolygonsColor;
    opo.opo_PolygonEdges.Unlock();
  }
  {
    // side polygon 1
    CObjectPolygon &opo = osec.osc_aopoPolygons[START_OF_SIDE_POLYGONS+1];
    CObjectEdge &oe0 = osec.osc_aoedEdges[ START_OF_BORDER_EDGES+1];
    CObjectEdge &oe1 = osec.osc_aoedEdges[ START_OF_CEILING_EDGES+1];

    CObjectPlane &opl = osec.osc_aoplPlanes[START_OF_SIDE_POLYGONS+1];
    opl = DOUBLEplane3D( *oe0.oed_Vertex0, *oe0.oed_Vertex1, *oe1.oed_Vertex1);
    opo.opo_Plane = &opl;
    opo.opo_PolygonEdges.New(iSlicesZ+3);
    opo.opo_PolygonEdges.Lock();
    INDEX iEdg=0;
    for( ; iEdg<iSlicesZ; iEdg++)
    {
      opo.opo_PolygonEdges[iEdg].ope_Edge = &osec.osc_aoedEdges[ START_OF_VERTICAL_EDGES+iSlicesX+iEdg*(iSlicesX+1)];
      opo.opo_PolygonEdges[iEdg].ope_Backward = FALSE;
    }
    opo.opo_PolygonEdges[iEdg+0].ope_Edge = &osec.osc_aoedEdges[ START_OF_BORDER_EDGES+1];
    opo.opo_PolygonEdges[iEdg+0].ope_Backward = FALSE;
    opo.opo_PolygonEdges[iEdg+1].ope_Edge = &osec.osc_aoedEdges[ START_OF_CEILING_EDGES+1];
    opo.opo_PolygonEdges[iEdg+1].ope_Backward = FALSE;
    opo.opo_PolygonEdges[iEdg+2].ope_Edge = &osec.osc_aoedEdges[ START_OF_BORDER_EDGES+2];
    opo.opo_PolygonEdges[iEdg+2].ope_Backward = TRUE;
    opo.opo_Material = &omat;
    opo.opo_ulFlags = ulNonFllorPolygonFlags;
    opo.opo_colorColor = theApp.m_vfpCurrent.vfp_colPolygonsColor;
    opo.opo_PolygonEdges.Unlock();
  }
  {
    // side polygon 2
    CObjectPolygon &opo = osec.osc_aopoPolygons[START_OF_SIDE_POLYGONS+2];
    CObjectEdge &oe0 = osec.osc_aoedEdges[ START_OF_BORDER_EDGES+2];
    CObjectEdge &oe1 = osec.osc_aoedEdges[ START_OF_CEILING_EDGES+2];

    CObjectPlane &opl = osec.osc_aoplPlanes[START_OF_SIDE_POLYGONS+2];
    opl = DOUBLEplane3D( *oe0.oed_Vertex0, *oe0.oed_Vertex1, *oe1.oed_Vertex1);
    opo.opo_Plane = &opl;
    opo.opo_PolygonEdges.New(iSlicesX+3);
    opo.opo_PolygonEdges.Lock();
    INDEX iEdg=0;
    for( ; iEdg<iSlicesX; iEdg++)
    {
      opo.opo_PolygonEdges[iEdg].ope_Edge = &osec.osc_aoedEdges[ iEdg];
      opo.opo_PolygonEdges[iEdg].ope_Backward = FALSE;
    }
    opo.opo_PolygonEdges[iEdg+0].ope_Edge = &osec.osc_aoedEdges[ START_OF_BORDER_EDGES+2];
    opo.opo_PolygonEdges[iEdg+0].ope_Backward = FALSE;
    opo.opo_PolygonEdges[iEdg+1].ope_Edge = &osec.osc_aoedEdges[ START_OF_CEILING_EDGES+2];
    opo.opo_PolygonEdges[iEdg+1].ope_Backward = FALSE;
    opo.opo_PolygonEdges[iEdg+2].ope_Edge = &osec.osc_aoedEdges[ START_OF_BORDER_EDGES+3];
    opo.opo_PolygonEdges[iEdg+2].ope_Backward = TRUE;
    opo.opo_Material = &omat;
    opo.opo_ulFlags = ulNonFllorPolygonFlags;
    opo.opo_colorColor = theApp.m_vfpCurrent.vfp_colPolygonsColor;
    opo.opo_PolygonEdges.Unlock();
  }
  {
    // side polygon 3
    CObjectPolygon &opo = osec.osc_aopoPolygons[START_OF_SIDE_POLYGONS+3];
    CObjectEdge &oe0 = osec.osc_aoedEdges[ START_OF_BORDER_EDGES+3];
    CObjectEdge &oe1 = osec.osc_aoedEdges[ START_OF_CEILING_EDGES+3];

    CObjectPlane &opl = osec.osc_aoplPlanes[START_OF_SIDE_POLYGONS+3];
    opl = DOUBLEplane3D( *oe0.oed_Vertex0, *oe0.oed_Vertex1, *oe1.oed_Vertex1);
    opo.opo_Plane = &opl;
    opo.opo_PolygonEdges.New(iSlicesZ+3);
    opo.opo_PolygonEdges.Lock();
    INDEX iEdg=0;
    for( ; iEdg<iSlicesZ; iEdg++)
    {
      opo.opo_PolygonEdges[iEdg].ope_Edge = &osec.osc_aoedEdges[ START_OF_VERTICAL_EDGES+iEdg*(iSlicesX+1)];
      opo.opo_PolygonEdges[iEdg].ope_Backward = TRUE;
    }
    opo.opo_PolygonEdges[iEdg+0].ope_Edge = &osec.osc_aoedEdges[ START_OF_BORDER_EDGES+3];
    opo.opo_PolygonEdges[iEdg+0].ope_Backward = FALSE;
    opo.opo_PolygonEdges[iEdg+1].ope_Edge = &osec.osc_aoedEdges[ START_OF_CEILING_EDGES+3];
    opo.opo_PolygonEdges[iEdg+1].ope_Backward = FALSE;
    opo.opo_PolygonEdges[iEdg+2].ope_Edge = &osec.osc_aoedEdges[ START_OF_BORDER_EDGES+0];
    opo.opo_PolygonEdges[iEdg+2].ope_Backward = TRUE;
    opo.opo_Material = &omat;
    opo.opo_ulFlags = ulNonFllorPolygonFlags;
    opo.opo_colorColor = theApp.m_vfpCurrent.vfp_colPolygonsColor;
    opo.opo_PolygonEdges.Unlock();
  }

  // create ceiling polygon and its plane
#define CEILING_POLYGON (iSlicesX*iSlicesZ*2+4)
  {
    // ceiling polygon
    CObjectPolygon &opo = osec.osc_aopoPolygons[CEILING_POLYGON];
    CObjectEdge &oe0 = osec.osc_aoedEdges[ START_OF_CEILING_EDGES+0];
    CObjectEdge &oe1 = osec.osc_aoedEdges[ START_OF_CEILING_EDGES+1];

    CObjectPlane &opl = osec.osc_aoplPlanes[CEILING_POLYGON];
    opl = DOUBLEplane3D( *oe1.oed_Vertex1, *oe1.oed_Vertex0, *oe0.oed_Vertex0);
    opo.opo_Plane = &opl;
    opo.opo_PolygonEdges.New(4);
    opo.opo_PolygonEdges.Lock();
    opo.opo_PolygonEdges[0].ope_Edge = &osec.osc_aoedEdges[ START_OF_CEILING_EDGES+0];
    opo.opo_PolygonEdges[0].ope_Backward = TRUE;
    opo.opo_PolygonEdges[1].ope_Edge = &osec.osc_aoedEdges[ START_OF_CEILING_EDGES+1];
    opo.opo_PolygonEdges[1].ope_Backward = TRUE;
    opo.opo_PolygonEdges[2].ope_Edge = &osec.osc_aoedEdges[ START_OF_CEILING_EDGES+2];
    opo.opo_PolygonEdges[2].ope_Backward = TRUE;
    opo.opo_PolygonEdges[3].ope_Edge = &osec.osc_aoedEdges[ START_OF_CEILING_EDGES+3];
    opo.opo_PolygonEdges[3].ope_Backward = TRUE;
    opo.opo_Material = &omat;
    opo.opo_ulFlags = ulNonFllorPolygonFlags;
    opo.opo_colorColor = theApp.m_vfpCurrent.vfp_colPolygonsColor;
    opo.opo_PolygonEdges.Unlock();
  }

  osec.osc_aovxVertices.Unlock();
  osec.osc_aoedEdges.Unlock();
  osec.osc_aoplPlanes.Unlock();
  osec.osc_aopoPolygons.Unlock();

  theApp.m_vfpCurrent.vfp_o3dPrimitive.Optimize();
}

void GetTerrainPolygonEdges(CObjectSector &osec, INDEX iPolygon,  INDEX iSlicesX, INDEX iSlicesZ,
                            CObjectEdge *&poe0, CObjectEdge *&poe1, CObjectEdge *&poe2,
                            CObjectEdge *&poe3, CObjectEdge *&poe4)
{
  INDEX iPolX = iPolygon%iSlicesX;
  INDEX iPolZ = iPolygon/iSlicesX;
  
  INDEX iEdge0 = iPolZ*iSlicesX+iPolX;
  poe0 = &osec.osc_aoedEdges[ iEdge0];
  
  INDEX iEdge1 = START_OF_VERTICAL_EDGES+iPolZ*(iSlicesX+1)+iPolX;
  poe1 = &osec.osc_aoedEdges[ iEdge1];
  
  INDEX iEdge2 = iEdge0+iSlicesX;
  poe2 = &osec.osc_aoedEdges[ iEdge2];
  
  INDEX iEdge3 = iEdge1+1;
  poe3 = &osec.osc_aoedEdges[ iEdge3];

  INDEX iEdge4 = START_OF_SLOPE_EDGES+iPolZ*iSlicesX+iPolX;
  poe4 = &osec.osc_aoedEdges[ iEdge4];
}

void CWorldEditorDoc::CreatePrimitive(void)
{
  // this is patch because deleting of primitive property page can call this
  if( m_iMode != CSG_MODE) return;
  // activ texture must exist
  ASSERT( theApp.m_ptdActiveTexture != NULL);

  InitializeObject3DForPrimitive();

  switch( theApp.m_vfpCurrent.vfp_ptPrimitiveType)
  {
  case PT_CONUS:
  case PT_TORUS:
    {
      // calculate width, height and lenght
      DOUBLE fWidth = theApp.m_vfpCurrent.vfp_fXMax-theApp.m_vfpCurrent.vfp_fXMin;
      DOUBLE fHeight = theApp.m_vfpCurrent.vfp_fYMax-theApp.m_vfpCurrent.vfp_fYMin;
      DOUBLE fLenght = theApp.m_vfpCurrent.vfp_fZMax-theApp.m_vfpCurrent.vfp_fZMin;
      // some values must be valid, so if they are not, coorect them
      if( fWidth < SNAP_FLOAT_GRID) fWidth = SNAP_FLOAT_GRID;
      if( fHeight < SNAP_FLOAT_GRID) fHeight = SNAP_FLOAT_GRID;
      if( fLenght < SNAP_FLOAT_GRID) fLenght = SNAP_FLOAT_GRID;
      // divide width and lenght by two because these values are used as radiuses
      fWidth /= 2.0f;
      fLenght/= 2.0f;
      // get count of vertices that will be used for creating base polygon
      INDEX vtxCt = theApp.m_vfpCurrent.vfp_avVerticesOnBaseOfPrimitive.Count();

      // if currently used number of vertices is not same as one used for last create primitive,
      // or if change occure in width or lenght
      if( (m_bPrimitiveCreatedFirstTime) ||
          (m_ctLastPrimitiveVertices != vtxCt) ||
          (m_fLastPrimitiveWidth != fWidth) ||
          (m_fLastPrimitiveLenght != fLenght) ||
          (m_bLastIfOuter != theApp.m_vfpCurrent.vfp_bOuter) ||
          (m_ttLastTriangularisationType != theApp.m_vfpCurrent.vfp_ttTriangularisationType) )
      {
        // recreate base vertices (discard vertex dragging)
        if( !_bDontRecalculateBase)
        {
          theApp.m_vfpCurrent.CalculatePrimitiveBase();
        }
        _bDontRecalculateBase = FALSE;
        // and remember new values for base, wait with recreating before any
        // change occure or until CSG operaton is applyed
        m_bPrimitiveCreatedFirstTime = FALSE;
        m_ctLastPrimitiveVertices = vtxCt;
        m_fLastPrimitiveWidth = fWidth;
        m_fLastPrimitiveLenght = fLenght;
        m_bLastIfOuter = theApp.m_vfpCurrent.vfp_bOuter;
        m_ttLastTriangularisationType = theApp.m_vfpCurrent.vfp_ttTriangularisationType;
      }
      // create primitive for the first time
      if( theApp.m_vfpCurrent.vfp_ptPrimitiveType == PT_CONUS)
      {
        CreateConusPrimitive();
      }
      else
      {
        CreateTorusPrimitive();
      }
      break;
    }
  case PT_STAIRCASES:
    {
      CreateStaircasesPrimitive();
      break;
    }
  case PT_SPHERE:
    {
      CreateSpherePrimitive();
      break;
    }
  case PT_TERRAIN:
    {
      CreateTerrainPrimitive();
      break;
    }
  default:
    {
      ASSERTALWAYS( "Invalid primitive type found!");
    }
  }

  // update position property page 
  m_chSelections.MarkChanged();
}

/*
 * refresh primitive page
 */
void CWorldEditorDoc::RefreshPrimitivePage(void)
{
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  // if info exists and active page is primitive page
  if( (pMainFrame->m_pInfoFrame != NULL) &&
      (pMainFrame->m_pInfoFrame->m_pInfoSheet->GetActivePage() == 
       &pMainFrame->m_pInfoFrame->m_pInfoSheet->m_PgPrimitive) )
  {
    // refresh primitive page
    pMainFrame->m_pInfoFrame->m_pInfoSheet->m_PgPrimitive.UpdateData( FALSE);
  }
}
/*
 * refresh position page
 */
void CWorldEditorDoc::RefreshCurrentInfoPage(void)
{
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  // if info exists 
  if( pMainFrame->m_pInfoFrame != NULL)
  {
    // if active page is position page
    if( pMainFrame->m_pInfoFrame->m_pInfoSheet->GetActivePage() == 
         &pMainFrame->m_pInfoFrame->m_pInfoSheet->m_PgPosition)
    {
      // refresh position page
      pMainFrame->m_pInfoFrame->m_pInfoSheet->m_PgPosition.UpdateData( FALSE);
    }
    // if active page is texture page
    else if( pMainFrame->m_pInfoFrame->m_pInfoSheet->GetActivePage() == 
              &pMainFrame->m_pInfoFrame->m_pInfoSheet->m_PgTexture)
    {
      // refresh polygon page
      pMainFrame->m_pInfoFrame->m_pInfoSheet->m_PgTexture.UpdateData( FALSE);
    }
  }
}

/*
 * snaps parameters for creating primitive to grid
 */
void CWorldEditorDoc::SnapPrimitiveValuesToGrid(void)
{
  /*
  SnapFloat( theApp.m_vfpCurrent.vfp_fShearX);
  SnapFloat( theApp.m_vfpCurrent.vfp_fShearZ);
  SnapFloat( theApp.m_vfpCurrent.vfp_fXMin);
  SnapFloat( theApp.m_vfpCurrent.vfp_fXMax);
  SnapFloat( theApp.m_vfpCurrent.vfp_fYMin);
  SnapFloat( theApp.m_vfpCurrent.vfp_fYMax);
  SnapFloat( theApp.m_vfpCurrent.vfp_fZMin);
  SnapFloat( theApp.m_vfpCurrent.vfp_fZMax);
  */
}

/*
 * Constructor.
 */
CUndo::CUndo(void)    // throw char * 
{
  static INDEX iUndoFile = 0;   // counter for undo files
  static char achUndoFileName[256];

  // create a temporary file name
  sprintf(achUndoFileName, "Temp\\WED_Undo%d.tmp", iUndoFile);
  m_fnmUndoFile = CTString(achUndoFileName);

  // increment the counter of undo files
  iUndoFile++;
}

/*
 * Destructor.
 */
CUndo::~CUndo(void)
{
  // delete the temporary file
  RemoveFile(m_fnmUndoFile);
}

/*
 * Loads state of the world from given undo/redo object
 */
void CWorldEditorDoc::LoadWorldFromUndoRedoList( CUndo *pUndoRedo)
{
  // try to
  try
  {
    // load new world from the undo file
    m_woWorld.Load_t(pUndoRedo->m_fnmUndoFile);
//    m_woWorld.ReinitializeEntities();
    // flush stale caches
    _pShell->Execute("FreeUnusedStock();");
    // invalidate document (i.e. all views)
    UpdateAllViews( NULL);
  }
  // report errors
  catch (const char *err_str)
  {
    AfxMessageBox( CString(err_str));
  }
}

/*
 * Saves current state of the world as tail of give undo/redo list
 */
void CWorldEditorDoc::SaveWorldIntoUndoRedoList( CListHead &lhList)
{
  // try to
  try
  {
    // allocate new undo/redo object
    CUndo *pUndoRedo = new CUndo;
    // save the world to the undo file
    m_woWorld.Save_t(pUndoRedo->m_fnmUndoFile);
    // add new undo as tail into undo list
    lhList.AddTail( pUndoRedo->m_lnListNode);
  }
  // report errors
  catch (const char *err_str)
  {
    AfxMessageBox( CString(err_str));
  }
}

/*
 * Remebers last operation into undo buffer
 */
void CWorldEditorDoc::RememberUndo(void)
{
  // if undo remembering is disabled
  if( !theApp.m_bRememberUndo)
  {
    return;
  }
  // delete redo list
  FORDELETELIST(CUndo, m_lnListNode, m_lhRedo, itRedo)
  { 
		delete &itRedo.Current();
  }

  // while there are more members in undo buffer than allowed or list isn't empty
  while( (m_lhUndo.Count() >= theApp.m_Preferences.ap_iUndoLevels) &&
         (!m_lhUndo.IsEmpty()) )
  {
    // get first member in undo list
    CUndo *pUndo = LIST_HEAD( m_lhUndo, CUndo, m_lnListNode);
    // remove it
    pUndo->m_lnListNode.Remove();
    // and delete it
    delete pUndo;
  }
  
  // if undo level is 0, don't remember any undo
  if( theApp.m_Preferences.ap_iUndoLevels == 0)
  {
    return;
  }

  // save current state of level into undo list
  SaveWorldIntoUndoRedoList( m_lhUndo);
}

/*
 * Undoes last operation
 */
void CWorldEditorDoc::Undo(void)
{
  // if undo level is 0, or undo list is empty, don't do any undo
  if( (theApp.m_Preferences.ap_iUndoLevels == 0) ||
      (m_lhUndo.IsEmpty()) )
  {
    return;
  }

  // clear selections
  ClearSelections();

  // get tail member from undo buffer
  CUndo *pUndo = LIST_TAIL( m_lhUndo, CUndo, m_lnListNode);
  // remove it from undo buffer
  pUndo->m_lnListNode.Remove();
  // save current state of level into redo list
  SaveWorldIntoUndoRedoList( m_lhRedo);
  // restore last saved state from undo list
  LoadWorldFromUndoRedoList( pUndo);
  // delete just used undo member
  delete pUndo;
}

/*
 * Redoes last operation
 */
void CWorldEditorDoc::Redo(void)
{
  // if redo list is empty, don't do any redo
  if( m_lhRedo.IsEmpty() )
  {
    return;
  }
  
  // clear selections
  ClearSelections();

  // get tail member from redo buffer
  CUndo *pRedo = LIST_TAIL( m_lhRedo, CUndo, m_lnListNode);
  // remove it from redo buffer
  pRedo->m_lnListNode.Remove();
  
  // save current state of level into undo list
  SaveWorldIntoUndoRedoList( m_lhUndo);
  // restore last saved state from redo list
  LoadWorldFromUndoRedoList( pRedo);
  // delete just used redo member
  delete pRedo;
}

void CWorldEditorDoc::OnEditUndo() 
{
  if( GetEditingMode()==TERRAIN_MODE)
  {
    if( m_iCurrentTerrainUndo>=0)
    {
      ApplyTerrainUndo(&m_dcTerrainUndo[m_iCurrentTerrainUndo]);
    }
  }
  else
  {
    Undo();
  }

  m_chDocument.MarkChanged();
  UpdateAllViews( NULL);
}

void CWorldEditorDoc::OnUpdateEditUndo(CCmdUI* pCmdUI) 
{
  if( GetEditingMode()==TERRAIN_MODE)
  {
    pCmdUI->Enable( m_iCurrentTerrainUndo>=0);
  }
  else
  {
    pCmdUI->Enable( !m_lhUndo.IsEmpty());
  }
}

void CWorldEditorDoc::OnEditRedo() 
{
  if( GetEditingMode()==TERRAIN_MODE)
  {
    INDEX ctRedos=m_dcTerrainUndo.Count()-1-m_iCurrentTerrainUndo;
    if( ctRedos>0)
    {
      ApplyTerrainRedo(&m_dcTerrainUndo[m_iCurrentTerrainUndo+1]);
    }
  }
  else
  {
    Redo();
  }
  m_chDocument.MarkChanged();
  UpdateAllViews( NULL);
}


void CWorldEditorDoc::OnUpdateEditRedo(CCmdUI* pCmdUI) 
{
  if( GetEditingMode()==TERRAIN_MODE)
  {
    INDEX ctRedos=m_dcTerrainUndo.Count()-1-m_iCurrentTerrainUndo;
    pCmdUI->Enable( ctRedos>0);
  }
  else
  {
    pCmdUI->Enable( !m_lhRedo.IsEmpty());
  }
}

// paste given texture over polygon selection
void CWorldEditorDoc::PasteTextureOverSelection_t( CTFileName fnTexName)
{
  // for each of the selected polygons
  FOREACHINDYNAMICCONTAINER( m_selPolygonSelection, CBrushPolygon, itbpo)
  {
    CTextureData *pTD = (CTextureData *) itbpo->bpo_abptTextures[m_iTexture].bpt_toTexture.GetData();
    if( (pTD == NULL) || (pTD->GetName() != fnTexName) )
    {
      itbpo->bpo_abptTextures[m_iTexture].bpt_toTexture.SetData_t( fnTexName); 
      // mark that document has been modified
      SetModifiedFlag( TRUE);
      // mark that selections have been changed
      m_chSelections.MarkChanged();
    }
  }
  // update all views
  UpdateAllViews( NULL);
}


FLOAT _fLastTimeDeselectAllUsed = -10000.0f;
// delete all selected members in current selection mode
void CWorldEditorDoc::DeselectAll(void)
{
  // if browse entities mode is on
  if( m_bBrowseEntitiesMode)
  {
    // cancel browse entities mode
    OnBrowseEntitiesMode();
  }
  else
  {
    FLOAT fCurrentTime = _pTimer->GetRealTimeTick();
    if( (fCurrentTime-_fLastTimeDeselectAllUsed)<1.0f)
    {
      ClearSelections();
      _fLastTimeDeselectAllUsed = fCurrentTime;
      return;
    }
    _fLastTimeDeselectAllUsed = fCurrentTime;

    // according to current selection mode clear selected members
    switch( GetEditingMode())
    {
    case POLYGON_MODE:
      { 
        m_selPolygonSelection.Clear();
        break;
      };
    case SECTOR_MODE:
      {
        m_selSectorSelection.Clear();
        break;
      };
    case ENTITY_MODE:
      {
        m_selEntitySelection.Clear();
        break;
      };
    case VERTEX_MODE:
      {
        m_selVertexSelection.Clear();
        break;
      };
    case CSG_MODE:
      { 
        break;
      };
    case TERRAIN_MODE:
      {
        m_ptrSelectedTerrain=NULL;
        theApp.m_ctTerrainPage.MarkChanged();
      break;
      };
    default:
      { 
        FatalError("Unknown editing mode.");
        break;
      };
    }
  }
  // mark that selections have been changed
  m_chSelections.MarkChanged();
  // redraw all viewes
  UpdateAllViews( NULL);
}

void CWorldEditorDoc::OnWorldSettings() 
{
  CDlgWorldSettings dlgWorldSettings;
  dlgWorldSettings.SetupBcgSettings( FALSE);
  if( dlgWorldSettings.DoModal() != IDOK) return;
  try
  {
// !!!!    m_woWorld.SetBackgroundTexture_t(CTString(dlgWorldSettings.m_fnBackgroundPicture));
  }
  catch (const char *strError)
  {
    AfxMessageBox( CString(strError));
  }
  m_woWorld.SetBackgroundColor( dlgWorldSettings.m_BackgroundColor.GetColor());
  m_woWorld.SetDescription( CTString(CStringA(dlgWorldSettings.m_strMissionDescription)));
  SetModifiedFlag( TRUE);
  UpdateAllViews( NULL);
}

/*
 * CSG Add
 */
void CWorldEditorDoc::OnCsgAdd() 
{
  BOOL bShift = (GetKeyState( VK_SHIFT)&0x8000) != 0;
  if( bShift) PreApplyCSG( CSG_ADD_REVERSE); // reverse priorities
  else        PreApplyCSG( CSG_ADD);
}

void CWorldEditorDoc::PreApplyCSG(enum CSGType CSGType) 
{
  if( GetEditingMode() != CSG_MODE)
  {
    // search for destination entity
    CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
    CEntity *penTarget = pMainFrame->m_CSGDesitnationCombo.GetSelectedBrushEntity();
    if( (penTarget == NULL) || (penTarget->IsSelected( ENF_SELECTED)) )
    {
      AfxMessageBox( L"Illegal CSG operands (target must not be selected) !");
      return;
    }
    // create temporary world
    CWorld woDummyWorld;

    // create zero placement
    CPlacement3D plZeroPlacement;
    plZeroPlacement.pl_PositionVector = FLOAT3D(0.0f,0.0f,0.0f);
    plZeroPlacement.pl_OrientationAngle = ANGLE3D(0,0,0);

    CDynamicContainer<CEntity> dcenDummy;
    // for all still selected brush entities
    {FOREACHINDYNAMICCONTAINER(m_selEntitySelection, CEntity, iten)
    {
      CEntity::RenderType rt = iten->GetRenderType();
      // if the entity is brush and it is not empty
      if( rt==CEntity::RT_BRUSH || rt==CEntity::RT_FIELDBRUSH)
      {
        // copy entity into dummy world
        woDummyWorld.CopyOneEntity( *iten, plZeroPlacement);
      }
      // deselect entities that are not brushes
      else
      {
        // deselect clicked sector
        dcenDummy.Add( &iten.Current());
      }
    }}
    
    // for entities that should be deselected
    {FOREACHINDYNAMICCONTAINER(dcenDummy, CEntity, iten)
    {
      m_selEntitySelection.Deselect( *iten);
    }}
    dcenDummy.Clear();
    
    // remember undo before doing CSG operations
    RememberUndo();

    // clear all selections except entity
    ClearSelections( ST_ENTITY);

    // delete all brush entities that are still selected
    m_woWorld.DestroyEntities( m_selEntitySelection);

    // set wait cursor
    CWaitCursor StartWaitCursor;
    m_csgtLastUsedCSGOperation = CSG_ADD_ENTITIES;
    m_bPreLastUsedPrimitiveMode = m_bLastUsedPrimitiveMode;
    m_bLastUsedPrimitiveMode = FALSE;
    // for all of the dummy world's entities
    {FOREACHINDYNAMICCONTAINER(woDummyWorld.wo_cenEntities, CEntity, iten)
    {
      // create another dummy world
      CWorld woOneBrush;
      // copy entity from dummy world to world containing only one brush
      CEntity *penOnlyBrush = woOneBrush.CopyOneEntity( *iten, plZeroPlacement);
      // ----------- Do CSG beetween current entity and destination combo's entity
      switch( CSGType)
      {
      case CSG_ADD:
      {
        m_woWorld.CSGAdd(*penTarget, woOneBrush, *penOnlyBrush, plZeroPlacement);
        break;
      }
      case CSG_ADD_REVERSE:
      {
        m_woWorld.CSGAddReverse(*penTarget, woOneBrush, *penOnlyBrush, plZeroPlacement);
        break;
      }
      case CSG_REMOVE:
      {
        m_woWorld.CSGRemove(*penTarget, woOneBrush, *penOnlyBrush, plZeroPlacement);
        break;
      }
      case CSG_SPLIT_SECTORS:
      {
        m_woWorld.SplitSectors(*penTarget, m_selSectorSelection, woOneBrush, *penOnlyBrush, plZeroPlacement);
        break;
      }
      case CSG_SPLIT_POLYGONS:
      {
        m_woWorld.SplitPolygons(*penTarget, m_selPolygonSelection, woOneBrush, *penOnlyBrush, plZeroPlacement);
        break;
      }
      default:
      {
        ASSERTALWAYS("PreAplyCSG() function called with illegal CSG operation.");
        return;
      }
      }
    }}
    // mark that selections have been changed
    SetModifiedFlag(TRUE);
    m_chSelections.MarkChanged();
    m_chDocument.MarkChanged();
  }
  else
  {
    ApplyCSG( CSGType);
  }
  UpdateAllViews( NULL);
}

BOOL CWorldEditorDoc::IsEntityCSGEnabled(void) 
{
  if(m_pwoSecondLayer != NULL) return TRUE;
  // if we are in entity mode
  if( GetEditingMode() != CSG_MODE)
  {
    // for all selected entities
    FOREACHINDYNAMICCONTAINER(m_selEntitySelection, CEntity, iten)
    {
      CEntity::RenderType rt = iten->GetRenderType();
      if( rt==CEntity::RT_BRUSH || rt==CEntity::RT_FIELDBRUSH)
      {
        return TRUE;
      }
    }
  }
  return FALSE;
}

void CWorldEditorDoc::OnUpdateCsgAdd(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable( IsEntityCSGEnabled());
}

/*
 * CSG Remove
 */
void CWorldEditorDoc::OnCsgRemove() 
{
  PreApplyCSG( CSG_REMOVE);
}
void CWorldEditorDoc::OnUpdateCsgRemove(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable( IsEntityCSGEnabled());
}

/*
 * CSG Split Sectors.
 */
void CWorldEditorDoc::OnCsgSplitSectors() 
{
  PreApplyCSG( CSG_SPLIT_SECTORS);
}

void CWorldEditorDoc::OnUpdateCsgSplitSectors(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable( IsEntityCSGEnabled());
}

/*
 * CSG Join Sectors.
 */
void CWorldEditorDoc::OnCsgJoinSectors() 
{
  ApplyCSG( CSG_JOIN_SECTORS);
}

void CWorldEditorDoc::OnUpdateCsgJoinSectors(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable( m_woWorld.CanJoinSectors( m_selSectorSelection));
}

/*
 * CSG Split Polygons.
 */
void CWorldEditorDoc::OnCsgSplitPolygons() 
{
  PreApplyCSG( CSG_SPLIT_POLYGONS);
}
void CWorldEditorDoc::OnUpdateCsgSplitPolygons(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable( IsEntityCSGEnabled() && m_selPolygonSelection.Count() > 0);
}

/*
 * CSG Join Polygons.
 */
void CWorldEditorDoc::OnCsgJoinPolygons() 
{
  BOOL bCtrl = (GetKeyState( VK_CONTROL)&0x8000) != 0;
  if (bCtrl) {
    ApplyCSG(CSG_JOIN_POLYGONS_KEEP_TEXTURES);   // keep textures if control pressed
  } else {
    ApplyCSG(CSG_JOIN_POLYGONS);
  }
}
void CWorldEditorDoc::OnUpdateCsgJoinPolygons(CCmdUI* pCmdUI) 
{
  // check for polygon mode and count (crashed here right after merging vertices)
  if( (m_iMode == POLYGON_MODE) && (m_selPolygonSelection.Count() != 0) )
  {
    pCmdUI->Enable( m_woWorld.CanJoinPolygons(m_selPolygonSelection));
  }
  else
  {
    pCmdUI->Enable( FALSE);
  }
}

void CWorldEditorDoc::OnCsgJoinAllPolygons() 
{
  BOOL bCtrl = (GetKeyState( VK_CONTROL)&0x8000) != 0;
  if (bCtrl) {
    ApplyCSG(CSG_JOIN_ALL_POLYGONS_KEEP_TEXTURES);   // keep textures if control pressed
  } else {
    ApplyCSG(CSG_JOIN_ALL_POLYGONS);
  }
}

void CWorldEditorDoc::OnUpdateCsgJoinAllPolygons(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable( m_woWorld.CanJoinAllPossiblePolygons(m_selPolygonSelection));
}

/*
 * Cancel CSG.
 */
void CWorldEditorDoc::OnCsgCancel() 
{
  if( m_iMode == CSG_MODE)
  {
    CancelCSG();	
  }
  else
  {
    SetEditingMode( VERTEX_MODE);
  }
}


void CWorldEditorDoc::OnShowOrientation() 
{
  m_bOrientationIcons = !m_bOrientationIcons;
  theApp.WriteProfileInt(L"World editor", L"Orientation icons", m_bOrientationIcons);
  UpdateAllViews( NULL);
}

void CWorldEditorDoc::OnUpdateShowOrientation(CCmdUI* pCmdUI) 
{
  pCmdUI->SetCheck( m_bOrientationIcons);
}

void CWorldEditorDoc::OnAutoSnap() 
{
  m_bAutoSnap = !m_bAutoSnap;		
}

void CWorldEditorDoc::OnUpdateAutoSnap(CCmdUI* pCmdUI) 
{
  pCmdUI->SetCheck( m_bAutoSnap);
}

void CWorldEditorDoc::OnCalculateShadows() 
{
  RememberUndo();   // this is before wait cursor, so that we can see if it gets blocked here

  // set wait cursor
  CWaitCursor StartWaitCursor;

  // reset profile
  _pfWorldEditingProfile.Reset();
  m_woWorld.CalculateDirectionalShadows();

  if( GetEditingMode()==TERRAIN_MODE)
  {
    CTerrain *ptTerrain=GetTerrain();
    if(ptTerrain!=NULL) ptTerrain->UpdateShadowMap();
  }

  // create shadows report
  _pfWorldEditingProfile.Report( theApp.m_strCSGAndShadowStatistics);
  theApp.m_strCSGAndShadowStatistics.SaveVar(CTString("Temp\\Profile_Shadows.txt"));

  // mark that document has changed
  SetModifiedFlag(TRUE);
  m_chDocument.MarkChanged();
  // invalidate document (i.e. all views)
  UpdateAllViews( NULL);
}

void CWorldEditorDoc::OnHideSelected() 
{
  if( m_iMode == ENTITY_MODE)
  {
    OnHideSelectedEntities();
  }
  else
  {
    OnHideSelectedSectors();
  }
}

void CWorldEditorDoc::OnHideUnselected() 
{
  if( m_iMode == ENTITY_MODE)
  {
    OnHideUnselectedEntities();
  }
  if( m_iMode == SECTOR_MODE)
  {
    OnHideUnselectedSectors();
  }
  if( m_iMode == TERRAIN_MODE)
  {
    theApp.m_iTerrainBrushMode=TBM_MAXIMUM;
    theApp.m_ctTerrainPageCanvas.MarkChanged();
    SetStatusLineModeInfoMessage();
  }
}

void CWorldEditorDoc::OnShowAll() 
{
  if( m_iMode == ENTITY_MODE)
  {
    OnShowAllEntities();
  }
  if( m_iMode == SECTOR_MODE)
  {
    OnShowAllSectors();
  }
}

void CWorldEditorDoc::OnUpdateHideSelected(CCmdUI* pCmdUI) 
{
	// enable button if selection is not empty
  if( m_iMode == ENTITY_MODE)
  {
    pCmdUI->Enable( m_selEntitySelection.Count() != 0);
  }
  else
  {
    pCmdUI->Enable( m_selSectorSelection.Count() != 0);
  }	
}

void CWorldEditorDoc::OnHideSelectedSectors() 
{
	// hide selected sectors
	m_woWorld.HideSelectedSectors( m_selSectorSelection);
  // update all views
  UpdateAllViews( NULL);
}

void CWorldEditorDoc::OnHideUnselectedSectors() 
{
	// hide unselected sectors
  m_woWorld.HideUnselectedSectors();
  // update all views
  UpdateAllViews( NULL);
}

void CWorldEditorDoc::OnShowAllSectors() 
{
	// hide unselected sectors
  m_woWorld.ShowAllSectors();
  // update all views
  UpdateAllViews( NULL);
}

void CWorldEditorDoc::OnHideSelectedEntities() 
{
	// hide selected entities
	m_woWorld.HideSelectedEntities( m_selEntitySelection);
  // update all views
  UpdateAllViews( NULL);
}

void CWorldEditorDoc::OnHideUnselectedEntities() 
{
	// hide unselected entities
  m_woWorld.HideUnselectedEntities();
  // update all views
  UpdateAllViews( NULL);
}

void CWorldEditorDoc::OnShowAllEntities() 
{
	// hide unselected entities
  m_woWorld.ShowAllEntities();
  // update all views
  UpdateAllViews( NULL);
}

// How box is created from 2 vertices: indices of first or second vertice, one that gives
// current coordinate for one of box's vertices (we have two source points, T0 and T1. 
// Coordinate for box vertice 0 is B0(xT0, yT0, zT0), vertice 1 is B1(xT1, yT0, zT0) ...)
  static INDEX _aiBoxCreation[8][3] = {
  0,0,0,
  1,0,0,
  1,0,1,
  0,0,1,
  0,1,0,
  1,1,0,
  1,1,1,
  0,1,1
};

// Array of indices to vertices that need to be corrected if one of box's vertices is moved
// (if vertice 0 moved, copy its x coordinate to vertices 3,7,4, copy y coordinate to
//  vertices 1,2,3, z to 1,5,4...)
static INDEX _aiCorrectVertices[8][3*3] = {
  3,7,4, 1,2,3, 1,5,4,
  2,5,6, 0,2,3, 0,4,5,
  1,5,6, 0,1,3, 3,6,7,
  0,4,7, 0,1,2, 2,6,7,
  0,3,7, 5,6,7, 0,1,5,
  1,2,6, 4,6,7, 0,1,4,
  1,2,5, 4,5,7, 2,3,7,
  0,3,4, 4,5,6, 2,3,6
};

// Selects entity with given index inside volume
void CWorldEditorDoc::SelectGivenEntity( INDEX iEntityToSelect)
{
  // clear normal entity selection
  m_selEntitySelection.Clear();
  // if there is any entity in volume container
  if( m_cenEntitiesSelectedByVolume.Count() != 0)
  {
    // clip requested entity
    if( iEntityToSelect >= m_cenEntitiesSelectedByVolume.Count())
    {
      iEntityToSelect = 0;
    }
    m_iSelectedEntityInVolume = iEntityToSelect;
    // lock the selection
    m_cenEntitiesSelectedByVolume.Lock();
    // get requested entity
    CEntity *penEntity = m_cenEntitiesSelectedByVolume.Pointer(m_iSelectedEntityInVolume);
    // unlock the selection
    m_cenEntitiesSelectedByVolume.Unlock();
    // add entity into normal selection
    m_selEntitySelection.Select( *penEntity);
    // center entity
    POSITION pos = GetFirstViewPosition();
    CWorldEditorView *pWedView = (CWorldEditorView *) GetNextView(pos);
    pWedView->OnCenterEntity();
  }
  // mark that selections have been changed
  m_chSelections.MarkChanged();
  // obtain main frame ptr
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  // and refresh property combo manualy calling on idle
  pMainFrame->m_PropertyComboBar.m_PropertyComboBox.OnIdle( 0);
  // update all views
  UpdateAllViews( NULL);
}

/*
 * Function puts world's entites occupied by volume box to volume box selection
 */
void CWorldEditorDoc::SelectEntitiesByVolumeBox(void) 
{
  m_iSelectedEntityInVolume = 0;  
  // create volume box (for browsing entities)
  FLOAT3D vMinMax[2];
  vMinMax[0] = m_vCreateBoxVertice0;
  vMinMax[1] = m_vCreateBoxVertice1;
  // create coordinates for box's vertices
  for( INDEX iBoxVertice=0;iBoxVertice<8;iBoxVertice++)
  {
    m_avVolumeBoxVertice[iBoxVertice] = FLOAT3D( vMinMax[_aiBoxCreation[iBoxVertice][0]](1),
                                           vMinMax[_aiBoxCreation[iBoxVertice][1]](2),
                                           vMinMax[_aiBoxCreation[iBoxVertice][2]](3) );
  }
  // create bbox from requested volume
  FLOATaabbox3D bboxVolume( m_vCreateBoxVertice0, m_vCreateBoxVertice1);
  // clear entity volume container
  m_cenEntitiesSelectedByVolume.Clear();
  // clear normal entity selection
  m_selEntitySelection.Clear();
  // for all of the world's entities
  FOREACHINDYNAMICCONTAINER(m_woWorld.wo_cenEntities, CEntity, iten)
  {
    CPlacement3D plEntityPlacement = iten->GetPlacement();
    // if entity handle is inside volume box
    if( bboxVolume.HasContactWith( FLOATaabbox3D(plEntityPlacement.pl_PositionVector)) )
    {
      // add entity into volume container
      m_cenEntitiesSelectedByVolume.Add( iten);
    }
  }
  SetStatusLineModeInfoMessage();
}

/*
 * Function corects coordinates of vertices that represent box because given vertice is
 * moved and box has invalid geometry
 */
void CWorldEditorDoc::CorrectBox(INDEX iMovedVtx, FLOAT3D vNewPosition) 
{
  for( INDEX iCoordinate=0;iCoordinate<3; iCoordinate++)
  {
    for( INDEX iVtxToCorrect=0;iVtxToCorrect<3; iVtxToCorrect++)
    {
      // copy coordinate
      m_avVolumeBoxVertice[ _aiCorrectVertices[iMovedVtx][iCoordinate*3+iVtxToCorrect]]
        (iCoordinate+1) = vNewPosition(iCoordinate+1);
    }
  }
  // copy new moved vertice's position
  m_avVolumeBoxVertice[ iMovedVtx] = vNewPosition;
  // set new values for next creation of volume box
  m_vCreateBoxVertice0 = m_avVolumeBoxVertice[ 0];
  m_vCreateBoxVertice1 = m_avVolumeBoxVertice[ 6];
}

void CWorldEditorDoc::OnBrowseEntitiesMode() 
{
  m_bBrowseEntitiesMode = !m_bBrowseEntitiesMode;
  // if we should start select by volume (or browse entities) mode
  if( m_bBrowseEntitiesMode)
  {
    SelectEntitiesByVolumeBox();
  }
  // stop select by volume mode
  else
  {
    // write to ini last vertices used for volume box creation
    char strIni[ 128];
    sprintf( strIni, "%f %f %f",
      m_vCreateBoxVertice0(1), m_vCreateBoxVertice0(2), m_vCreateBoxVertice0(3));
    theApp.WriteProfileString( L"World editor", L"Volume box min", CString(strIni));
    sprintf( strIni, "%f %f %f",
      m_vCreateBoxVertice1(1), m_vCreateBoxVertice1(2), m_vCreateBoxVertice1(3));
    theApp.WriteProfileString( L"World editor", L"Volume box max", CString(strIni));
    m_chSelections.MarkChanged();
  }
  // update all views
  UpdateAllViews( NULL);
}
void CWorldEditorDoc::OnUpdateBrowseEntitiesMode(CCmdUI* pCmdUI) 
{
  pCmdUI->SetCheck(	m_bBrowseEntitiesMode);
  pCmdUI->Enable(	GetEditingMode() == ENTITY_MODE);
}

void CWorldEditorDoc::OnPreviousSelectedEntity() 
{
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  INDEX iEntityToSelect = (m_iSelectedEntityInVolume+
    m_cenEntitiesSelectedByVolume.Count()-1)%m_cenEntitiesSelectedByVolume.Count();
  
  // clip entity index
  if( iEntityToSelect >= m_cenEntitiesSelectedByVolume.Count())
  {
    iEntityToSelect = 0;
  }

  CTString strMessage;
  strMessage.PrintF("Entity %d/%d", iEntityToSelect, m_cenEntitiesSelectedByVolume.Count());
  pMainFrame->SetStatusBarMessage( strMessage, STATUS_LINE_PANE, 2);
  
  SelectGivenEntity( iEntityToSelect);
}

void CWorldEditorDoc::OnNextSelectedEntity() 
{
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  INDEX iEntityToSelect = (m_iSelectedEntityInVolume+1)%m_cenEntitiesSelectedByVolume.Count();
  // clip entity index
  if( iEntityToSelect >= m_cenEntitiesSelectedByVolume.Count() )
  {
    iEntityToSelect = 0;
  }
  
  CTString strMessage;
  strMessage.PrintF("Entity %d/%d", iEntityToSelect, m_cenEntitiesSelectedByVolume.Count());
  pMainFrame->SetStatusBarMessage( strMessage, STATUS_LINE_PANE, 2);
  
  SelectGivenEntity( iEntityToSelect);
}

void CWorldEditorDoc::OnUpdatePreviousSelectedEntity(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable(	m_cenEntitiesSelectedByVolume.Count()>0);
}
void CWorldEditorDoc::OnUpdateNextSelectedEntity(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable(	m_cenEntitiesSelectedByVolume.Count()>0);
}

void CWorldEditorDoc::OnSelectAllInVolume( void)
{
  // for each of the entities selected by volume
  FOREACHINDYNAMICCONTAINER( m_cenEntitiesSelectedByVolume, CEntity, iten)
  {
    if( !iten->IsSelected( ENF_SELECTED))
    {
      // add entity into normal selection
      m_selEntitySelection.Select( *iten);
    }
  }
  // clear volume container
  //m_cenEntitiesSelectedByVolume.Clear();
  // go out of browse by volume mode
  if( m_bBrowseEntitiesMode) OnBrowseEntitiesMode();
  // mark that selections have been changed
  m_chSelections.MarkChanged();
  // obtain main frame ptr
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  // and refresh property combo manualy calling on idle
  pMainFrame->m_PropertyComboBar.m_PropertyComboBox.OnIdle( 0);
  // update all views
  UpdateAllViews( NULL);
}

void CWorldEditorDoc::OnJoinLayers() 
{
  ApplyCSG( CSG_JOIN_LAYERS);
}

void CWorldEditorDoc::OnUpdateSelectByClass(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable(	TRUE);
}

void CWorldEditorDoc::OnSelectByClass() 
{
  CWorldEditorView *pWorldEditorView = theApp.GetActiveView();
  CDlgBrowseByClass dlgBrowseByClass;
  INDEX ctEntities = m_cenEntitiesSelectedByVolume.Count();
  // auto start in all entities mode if no entities are selected
  dlgBrowseByClass.m_bShowVolume = (ctEntities != 0);
  if( dlgBrowseByClass.DoModal() == IDOK)
  {
    m_chSelections.MarkChanged();
    SetEditingMode( ENTITY_MODE);
    UpdateAllViews( NULL);
    if( (pWorldEditorView != NULL) && 
        (dlgBrowseByClass.m_bCenterSelected) )
    {                                                                    
      pWorldEditorView->CenterSelected();
    }
  }
}

void CWorldEditorDoc::OnSelectByClassImportant() 
{
  CWorldEditorView *pWorldEditorView = theApp.GetActiveView();
  CDlgBrowseByClass dlgBrowseByClass;
  dlgBrowseByClass.m_bShowImportants = TRUE;
  if( dlgBrowseByClass.DoModal() == IDOK)
  {
    m_chSelections.MarkChanged();
    SetEditingMode( ENTITY_MODE);
    UpdateAllViews( NULL);
    if( (pWorldEditorView != NULL) && 
        (dlgBrowseByClass.m_bCenterSelected) )
    {
      pWorldEditorView->CenterSelected();
    }
  }
}

void CWorldEditorDoc::OnCrossroadForN() 
{
  if( m_iMode == VERTEX_MODE)
  {
    CDlgSnapVertex dlg;
    dlg.DoModal();
  }
  else
  {
    OnSelectByClassAll();
  }
}

void CWorldEditorDoc::OnSelectByClassAll() 
{
  CWorldEditorView *pWorldEditorView = theApp.GetActiveView();
  CDlgBrowseByClass dlgBrowseByClass;
  dlgBrowseByClass.m_bShowVolume = FALSE;
  if( dlgBrowseByClass.DoModal() == IDOK)
  {
    m_chSelections.MarkChanged();
    SetEditingMode( ENTITY_MODE);
    UpdateAllViews( NULL);
    if( (pWorldEditorView != NULL) && 
        (dlgBrowseByClass.m_bCenterSelected) )
    {
      pWorldEditorView->CenterSelected();
    }
  }
}

void CWorldEditorDoc::OnTexture1() 
{
  theApp.m_bTexture1 = !theApp.m_bTexture1;
  UpdateAllViews( NULL);
}

void CWorldEditorDoc::OnUpdateTexture1(CCmdUI* pCmdUI) 
{
  pCmdUI->SetCheck( theApp.m_bTexture1);
}

void CWorldEditorDoc::OnTexture2() 
{
  theApp.m_bTexture2 = !theApp.m_bTexture2;
  UpdateAllViews( NULL);
}

void CWorldEditorDoc::OnUpdateTexture2(CCmdUI* pCmdUI) 
{
  pCmdUI->SetCheck( theApp.m_bTexture2);
}

void CWorldEditorDoc::OnTexture3() 
{
  theApp.m_bTexture3 = !theApp.m_bTexture3;
  UpdateAllViews( NULL);
}

void CWorldEditorDoc::OnUpdateTexture3(CCmdUI* pCmdUI) 
{
  pCmdUI->SetCheck( theApp.m_bTexture3);
}

void CWorldEditorDoc::SetActiveTextureLayer(INDEX iLayer)
{
  if( GetEditingMode()==TERRAIN_MODE)
  {
    CTerrain *ptTerrain=GetTerrain();
    if(ptTerrain==NULL) return;
    if(iLayer>=ptTerrain->tr_atlLayers.Count()) return;
    SelectLayer(iLayer);
    m_chSelections.MarkChanged();
    theApp.m_ctTerrainPageCanvas.MarkChanged();
  }
  else if(iLayer<3)
  {
    m_iTexture = iLayer;
    m_chSelections.MarkChanged();
    UpdateAllViews( NULL);
  }
}

void CWorldEditorDoc::OnTextureMode1() 
{
  SetActiveTextureLayer(0);
}

void CWorldEditorDoc::OnTextureMode2() 
{
  SetActiveTextureLayer(1);
}

void CWorldEditorDoc::OnTextureMode3() 
{
  SetActiveTextureLayer(2);
}

void CWorldEditorDoc::OnTextureMode4() 
{
  SetActiveTextureLayer(3);
}

void CWorldEditorDoc::OnTextureMode5() 
{
  SetActiveTextureLayer(4);
}

void CWorldEditorDoc::OnTextureMode6() 
{
  SetActiveTextureLayer(5);
}

void CWorldEditorDoc::OnTextureMode7() 
{
  SetActiveTextureLayer(6);
}

void CWorldEditorDoc::OnTextureMode8() 
{
  SetActiveTextureLayer(7);
}

void CWorldEditorDoc::OnTextureMode9() 
{
  SetActiveTextureLayer(8);
}

void CWorldEditorDoc::OnTextureMode10() 
{
  SetActiveTextureLayer(9);
}

void CWorldEditorDoc::OnSaveThumbnail( void) 
{
  // remember current position for thumbnail saving into world
  CWorldEditorView *pViewForThumbnail = theApp.GetActiveView();
  if( pViewForThumbnail == NULL) return;
  CChildFrame *pChild = pViewForThumbnail->GetChildFrame();
  // set new viewer settings
  m_woWorld.wo_plThumbnailFocus = pChild->m_mvViewer.mv_plViewer;
  m_woWorld.wo_fThumbnailTargetDistance = pChild->m_mvViewer.mv_fTargetDistance;
  // save thumbnail
  SaveThumbnail();
}

void CWorldEditorDoc::SaveThumbnail() 
{
  CDrawPort *pDrawPort;
  CImageInfo II;
  CTextureData TD;
  CAnimData AD;
  ULONG flags = NONE;

  // if document isn't saved, call save as
  if( GetPathName() == "")
  {
    // if failed
    if( !DoFileSave()) return;
  }

  // try to find perspective view
  POSITION pos = GetFirstViewPosition();
  CWorldEditorView *pViewForThumbnail = theApp.GetActiveView();
  CWorldEditorView *pWedView;
  FOREVER
  {
    pWedView = (CWorldEditorView *) GetNextView(pos);
    if( pWedView == NULL) return;
    if( pWedView->m_ptProjectionType == CSlaveViewer::PT_PERSPECTIVE)
    {
      pViewForThumbnail = pWedView;
      break;
    }
  }
  // if perspective view can't be found, don't do anything
  if( pViewForThumbnail == NULL) return;
  CChildFrame *pChild = pViewForThumbnail->GetChildFrame();

  // create canvas to render picture
  _pGfx->CreateWorkCanvas( 128, 128, &pDrawPort);
  if( pDrawPort != NULL)
  {
    if( pDrawPort->Lock())
    {
      // remember old viewer settings
      CPlacement3D plOrgPlacement = pChild->m_mvViewer.mv_plViewer;
      FLOAT fOldTargetDistance = pChild->m_mvViewer.mv_fTargetDistance;
      // set new viewer settings
      pChild->m_mvViewer.mv_plViewer = m_woWorld.wo_plThumbnailFocus;
      pChild->m_mvViewer.mv_fTargetDistance = m_woWorld.wo_fThumbnailTargetDistance;
      // render vew from thumbnail position
      pViewForThumbnail->RenderView( pDrawPort);
      // restore orgiginal position
      pChild->m_mvViewer.mv_plViewer = plOrgPlacement;
      pChild->m_mvViewer.mv_fTargetDistance = fOldTargetDistance;
      pDrawPort->Unlock();
    }
    
    CTFileName fnDocument = CTString( CStringA(GetPathName()));
    CTFileName fnThumbnail = fnDocument.FileDir() + fnDocument.FileName() + CTString(".tbn");

    pDrawPort->GrabScreen(II);
    // try to
    try {
      // remove application path
      fnThumbnail.RemoveApplicationPath_t();
      // create image info from texture
      TD.Create_t( &II, 128, MAX_MEX_LOG2, FALSE);
      // save the thumbnail
      CTFileStream File;
      File.Create_t( fnThumbnail);
      TD.Write_t( &File);
      File.Close();
    }
    // if failed
    catch (const char *strError) {
      // report error
      AfxMessageBox(CString(strError));
    }
    _pGfx->DestroyWorkCanvas( pDrawPort);
    pDrawPort = NULL;
  }
  CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
  // refresh browser (open and close current virtual directory)
  pMainFrame->m_Browser.CloseSelectedDirectory();
  pMainFrame->m_Browser.OpenSelectedDirectory();
}

void CWorldEditorDoc::ResetPrimitive() 
{
  if( !m_bPrimitiveMode) return;
  FLOAT fDX = (theApp.m_vfpCurrent.vfp_fXMax+theApp.m_vfpCurrent.vfp_fXMin)/2.0f;
  FLOAT fDY = theApp.m_vfpCurrent.vfp_fYMin;
  FLOAT fDZ = (theApp.m_vfpCurrent.vfp_fZMax+theApp.m_vfpCurrent.vfp_fZMin)/2.0f;

  FLOAT fWidth = theApp.m_vfpCurrent.vfp_fXMax-theApp.m_vfpCurrent.vfp_fXMin;
  FLOAT fHeight = theApp.m_vfpCurrent.vfp_fYMax-theApp.m_vfpCurrent.vfp_fYMin;
  FLOAT fLenght = (theApp.m_vfpCurrent.vfp_fZMax-theApp.m_vfpCurrent.vfp_fZMin);

  FLOAT3D vDelta = FLOAT3D( fDX, fDY, fDZ);
  m_plSecondLayer.pl_PositionVector += vDelta;
  theApp.m_vfpCurrent.vfp_plPrimitive = m_plSecondLayer;

  theApp.m_vfpCurrent.vfp_fXMin = -fWidth/2.0f;
  theApp.m_vfpCurrent.vfp_fXMax = fWidth/2.0f;
  theApp.m_vfpCurrent.vfp_fYMin = 0.0f;
  theApp.m_vfpCurrent.vfp_fYMax = fHeight;
  theApp.m_vfpCurrent.vfp_fZMin = -fLenght/2.0f;
  theApp.m_vfpCurrent.vfp_fZMax = fLenght/2.0f;
  
  RefreshPrimitivePage();
  m_bPrimitiveCreatedFirstTime = TRUE;
  _bDontRecalculateBase = FALSE;
  CreatePrimitive();
  UpdateAllViews( NULL);
}

void CWorldEditorDoc::DeletePrimitiveVertex(INDEX iVtxToDelete)
{
  // get count of vertices on the base
  INDEX vtxCt = theApp.m_vfpCurrent.vfp_avVerticesOnBaseOfPrimitive.Count();
  if( vtxCt < 4) return;

  CStaticArray<DOUBLE3D> avDecreased;
  avDecreased.New(vtxCt-1);
  INDEX iVtxNew = 0;
  for( INDEX iVtxOld = 0; iVtxOld<vtxCt; iVtxOld++)
  {
    if( iVtxOld != iVtxToDelete)
    {
      avDecreased[iVtxNew] = theApp.m_vfpCurrent.vfp_avVerticesOnBaseOfPrimitive[iVtxOld];
      iVtxNew++;
    }
  }
  // copy new array back to primitive
  theApp.m_vfpCurrent.vfp_avVerticesOnBaseOfPrimitive.Clear();
  theApp.m_vfpCurrent.vfp_avVerticesOnBaseOfPrimitive.New(vtxCt-1);
  for( INDEX iVtx=0; iVtx<vtxCt-1; iVtx++)
  {
    theApp.m_vfpCurrent.vfp_avVerticesOnBaseOfPrimitive[iVtx] = avDecreased[iVtx];
  }
  m_ctLastPrimitiveVertices = vtxCt-1;
  RefreshPrimitivePage();
  CreatePrimitive();
  UpdateAllViews( NULL);
}

void CWorldEditorDoc::InsertPrimitiveVertex(INDEX iEdge, FLOAT3D vVertexToInsert)
{
  // get count of vertices on the base
  INDEX vtxCt = theApp.m_vfpCurrent.vfp_avVerticesOnBaseOfPrimitive.Count();
  CStaticArray<DOUBLE3D> avIncreased;
  avIncreased.New(vtxCt+1);
  INDEX iVtxNew = 0;
  for( INDEX iVtxOld = 0; iVtxOld<vtxCt; iVtxOld++)
  {
    avIncreased[iVtxNew] = theApp.m_vfpCurrent.vfp_avVerticesOnBaseOfPrimitive[iVtxOld];
    iVtxNew++;
    if( iVtxOld == iEdge)
    {
      avIncreased[iVtxNew] = FLOATtoDOUBLE(vVertexToInsert);
      avIncreased[iVtxNew](2) = 0.0f;
      iVtxNew++;
    }
  }
  // copy new array back to primitive
  theApp.m_vfpCurrent.vfp_avVerticesOnBaseOfPrimitive.Clear();
  theApp.m_vfpCurrent.vfp_avVerticesOnBaseOfPrimitive.New(vtxCt+1);
  for( INDEX iVtx=0; iVtx<vtxCt+1; iVtx++)
  {
    theApp.m_vfpCurrent.vfp_avVerticesOnBaseOfPrimitive[iVtx] = avIncreased[iVtx];
  }
  m_ctLastPrimitiveVertices = vtxCt+1;
  RefreshPrimitivePage();
  CreatePrimitive();
  UpdateAllViews( NULL);
}

void CWorldEditorDoc::OnUpdateLinks() 
{
  CWaitCursor wc;
  m_woWorld.RebuildLinks();
}

void CWorldEditorDoc::OnSnapshot() 
{
  RememberUndo();
}

void CWorldEditorDoc::OnMirrorAndStretch() 
{
  if (!m_bPrimitiveMode) {
    CDlgMirrorAndStretch dlg;
    // set dialog name
    if (m_pwoSecondLayer != NULL) {
      dlg.m_strName = "Mirror and stretch second layer";
    } else {
      dlg.m_strName = "Mirror and stretch entire world";
    }
    if( dlg.DoModal() == IDOK)
    {
      ApplyMirrorAndStretch( dlg.m_iMirror, dlg.m_fStretch);
    }
  } else {
    ASSERT(FALSE);
  }
}

void CWorldEditorDoc::OnFlipLayer() 
{
  if (!m_bPrimitiveMode) {
    ApplyMirrorAndStretch( m_iMirror, 1.0f);
    m_iMirror = (m_iMirror+1)%4;
    ApplyMirrorAndStretch( m_iMirror, 1.0f);
  }
}

void CWorldEditorDoc::OnUpdateFlipLayer(CCmdUI* pCmdUI) 
{
  // enable only for second layer
  pCmdUI->Enable( m_pwoSecondLayer != NULL && !m_bPrimitiveMode);
}

void CWorldEditorDoc::ApplyMirrorAndStretch(INDEX iMirror, FLOAT fStretch) 
{
  try
  {
    CWorld woDummy;
    if( m_pwoSecondLayer != NULL)
    {
      woDummy.MirrorAndStretch( *m_pwoSecondLayer, fStretch, 
        (enum WorldMirrorType)iMirror);
      woDummy.Save_t(CTString("Temp\\MirrorAndStretch.wld"));
      m_pwoSecondLayer->Clear();
      m_pwoSecondLayer->Load_t(CTString("Temp\\MirrorAndStretch.wld"));
    }
    else
    {
      int iRes = AfxMessageBox(L"Are you sure you want to mirror/stretch entire world?", MB_ICONEXCLAMATION|MB_YESNO|MB_DEFBUTTON2);
      if (iRes!=IDYES) {
        return;
      }
      CWaitCursor wcWait;
      RememberUndo();

      ClearSelections();

      woDummy.MirrorAndStretch( m_woWorld, fStretch, 
        (enum WorldMirrorType)iMirror);
      woDummy.Save_t(CTString("Temp\\MirrorAndStretch.wld"));
      m_woWorld.Clear();
      m_woWorld.Load_t(CTString("Temp\\MirrorAndStretch.wld"));
      m_woWorld.CalculateNonDirectionalShadows();

      m_chDocument.MarkChanged();
      SetModifiedFlag();
    }
  }
  catch (const char *strError)
  {
    AfxMessageBox(CString(strError));
  }
  UpdateAllViews( NULL);                                        
}

void CWorldEditorDoc::OnFilterSelection() 
{
  CDlgFilterPolygonSelection dlgFilterPolygonSelection;
  dlgFilterPolygonSelection.DoModal();
}

BOOL CWorldEditorDoc::IsCloneUpdatingAllowed(void) 
{
  INDEX ctEntities = m_selEntitySelection.Count();

  // if only one entity is selected
  if( ctEntities == 1)
  {
    // get only selected entity
    m_selEntitySelection.Lock();
    CEntity *penOnlySelected = &m_selEntitySelection[0];
    m_selEntitySelection.Unlock();

    // if entity doesn't have parent
    if( penOnlySelected->GetParent() == NULL)
    {
      // allow clone updating
      return TRUE;
    }
  }
  // disable clone updating
  return	FALSE;
}

void CWorldEditorDoc::OnUpdateUpdateClones(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable(	IsCloneUpdatingAllowed() );
}

// delete selected entity with all descendents
void DeleteEntityWithDescendents( CWorld &woWorld, CEntity *penParent)
{
  FORDELETELIST( CEntity, en_lnInParent, penParent->en_lhChildren, itenChild)
  {
    DeleteEntityWithDescendents( woWorld, &*itenChild);
  }
  woWorld.DestroyOneEntity( penParent);
}

void CWorldEditorDoc::OnUpdateClones() 
{
  // for each case
  if( m_selEntitySelection.Count() == 0)
  {
    return;
  }
  
  // get only selected entity
  m_selEntitySelection.Lock();
  CEntity *penOnlySelected = &m_selEntitySelection[0];
  m_selEntitySelection.Unlock();

  // clear selections before destroying some entities
  ClearSelections();

  // remember placements and delete all clones (entities with same name)
  CTString strName = penOnlySelected->GetName();
  if( strName == "World Base")
  {
    CMainFrame* pMainFrame = STATIC_DOWNCAST(CMainFrame, AfxGetMainWnd());
    if( ::MessageBoxA( pMainFrame->m_hWnd, "Are you sure that you want to execute update clones\n"
      "on entity named: \"World Base\"?", "Warning !", MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON1|
               MB_SYSTEMMODAL | MB_TOPMOST) != IDYES)
    {
      return;
    }
  }

  RememberUndo();

  CDynamicContainer<CEntity> apenClones;
  {FOREACHINDYNAMICCONTAINER( m_woWorld.wo_cenEntities, CEntity, iten)
  {
    // if this is clone (by name), it is not original and it is not child of some other entity
    if( (strName == iten->GetName()) && ( &*iten != penOnlySelected) && (iten->GetParent() == NULL) )
    {
      apenClones.Add( &*iten);
    }
  }}

  apenClones.Lock();
  // remember placements of clones
  CStaticArray<CPlacement3D> aplClones;
  INDEX ctEntities = apenClones.Count();
  aplClones.New( ctEntities);
  {for( INDEX iEntity=0; iEntity<ctEntities; iEntity++)
  {
    aplClones[ iEntity] = apenClones[ iEntity].GetPlacement();
  }}

  // delete clones with their descendents
  {for( INDEX iEntity=0; iEntity<ctEntities; iEntity++)
  {
    DeleteEntityWithDescendents( m_woWorld, &apenClones[ iEntity]);
  }}

  // clone entity(ies) for each remembered placement
  {for( INDEX iEntity=0; iEntity<ctEntities; iEntity++)
  {
    m_woWorld.CopyEntityInWorld( *penOnlySelected, aplClones[ iEntity]);
  }}
  apenClones.Unlock();

  m_chSelections.MarkChanged();

  m_chDocument.MarkChanged();
  SetModifiedFlag();
  UpdateAllViews( NULL);                                        
}

void CWorldEditorDoc::SetCutMode( CWorldEditorView *pwedView)
{
  POSITION pos = GetFirstViewPosition();
  CWorldEditorView *pwedTemp;
  FOREVER
  {
    pwedTemp = (CWorldEditorView *) GetNextView(pos);
    if( pwedTemp == NULL) return;
    if( pwedTemp == pwedView)
    {
      pwedTemp->m_bCutMode = TRUE;
    }
  }
}

void CreateCuttingWorld( FLOATplane3D &plPolygon, FLOATaabbox3D &box, CObject3D &o3d)
{
  // ------------------- Create polygon that will be used for cutting
  FLOAT3D v0, v1, v2, v3;
  v0=v1=v2=v3= FLOAT3D( 0.0f, 0.0f, 0.0f);

  // find major axes of the polygon plane
  INDEX iMajorAxis1, iMajorAxis2;
  GetMajorAxesForPlane( plPolygon, iMajorAxis1, iMajorAxis2);

  FLOAT3D vCenter = box.Center();
  FLOAT fSize = box.Size().Length();
  v0(iMajorAxis1) = vCenter(iMajorAxis1)-fSize;  v0(iMajorAxis2) = vCenter(iMajorAxis2)+fSize;
  v1(iMajorAxis1) = vCenter(iMajorAxis1)+fSize;  v1(iMajorAxis2) = vCenter(iMajorAxis2)+fSize;
  v2(iMajorAxis1) = vCenter(iMajorAxis1)+fSize;  v2(iMajorAxis2) = vCenter(iMajorAxis2)-fSize;
  v3(iMajorAxis1) = vCenter(iMajorAxis1)-fSize;  v3(iMajorAxis2) = vCenter(iMajorAxis2)-fSize;

  // project coordinates back to plane
  FLOAT3D vp0, vp1, vp2, vp3;
  vp0 = plPolygon.ProjectPoint( v0);
  vp1 = plPolygon.ProjectPoint( v1);
  vp2 = plPolygon.ProjectPoint( v2);
  vp3 = plPolygon.ProjectPoint( v3);


  // add vertices
  CObjectSector *pos = o3d.ob_aoscSectors.New(1);
  pos->osc_aovxVertices.New(4);
  pos->osc_aovxVertices.Lock();
  pos->osc_aovxVertices[0] = FLOATtoDOUBLE(vp0);
  pos->osc_aovxVertices[1] = FLOATtoDOUBLE(vp1);
  pos->osc_aovxVertices[2] = FLOATtoDOUBLE(vp2);
  pos->osc_aovxVertices[3] = FLOATtoDOUBLE(vp3);

  // add edges
  pos->osc_aoedEdges.New(4);
  pos->osc_aoedEdges.Lock();
  pos->osc_aoedEdges[0].oed_Vertex0 = &pos->osc_aovxVertices[0];
  pos->osc_aoedEdges[0].oed_Vertex1 = &pos->osc_aovxVertices[1];
  pos->osc_aoedEdges[1].oed_Vertex0 = &pos->osc_aovxVertices[1];
  pos->osc_aoedEdges[1].oed_Vertex1 = &pos->osc_aovxVertices[2];
  pos->osc_aoedEdges[2].oed_Vertex0 = &pos->osc_aovxVertices[2];
  pos->osc_aoedEdges[2].oed_Vertex1 = &pos->osc_aovxVertices[3];
  pos->osc_aoedEdges[3].oed_Vertex0 = &pos->osc_aovxVertices[3];
  pos->osc_aoedEdges[3].oed_Vertex1 = &pos->osc_aovxVertices[0];
  
  // add plane
  CObjectPlane *popl = pos->osc_aoplPlanes.New(1);
  *popl = FLOATtoDOUBLE(plPolygon);

  // add material
  CObjectMaterial *pom = pos->osc_aomtMaterials.New(1);
  // set must-exist texture
  pom->omt_Name = "Textures\\Editor\\Default.tex";
  // add polygon
  CObjectPolygon *pop = pos->osc_aopoPolygons.New(1);
  pop->opo_Plane = popl;
  pop->opo_Material = pom;
  pop->opo_PolygonEdges.New(4);
  pop->opo_PolygonEdges.Lock();
  pop->opo_PolygonEdges[0].ope_Edge = &pos->osc_aoedEdges[0];
  pop->opo_PolygonEdges[1].ope_Edge = &pos->osc_aoedEdges[1];
  pop->opo_PolygonEdges[2].ope_Edge = &pos->osc_aoedEdges[2];
  pop->opo_PolygonEdges[3].ope_Edge = &pos->osc_aoedEdges[3];
  pop->opo_PolygonEdges.Unlock();

  // unlock locked arrays
  pos->osc_aovxVertices.Unlock();
  pos->osc_aoedEdges.Unlock();
}

void CWorldEditorDoc::ApplyCut( void)
{
  // find the view that defines cut plane
  POSITION posView = GetFirstViewPosition();

  if( m_pCutLineView == NULL)
  {
    ASSERTALWAYS( "Cut line view wasn't set properly!");
    // don't allow calling without cutting view ptr set
    return;
  }

  CWorldEditorView *pwedView;
  FOREVER
  {
    pwedView = (CWorldEditorView *) GetNextView(posView);
    // if we didn't find it in list of views
    if( pwedView == NULL)
    {
      // don't do anything
      return;
    }
    // if we found it
    if( pwedView == m_pCutLineView)
    {
      // stop looping
      break;
    }
  }

  // calculate cutting plane
  FLOAT3D &vcl0 = m_vCutLineStart;
  FLOAT3D &vcl1 = m_vCutLineEnd;
  // get cutting edge vector
  FLOAT3D vCutLine = vcl1-vcl0;

  // cross product with viewer direction vector will give us cutting plane normal
  // so calculate viewer direction vector
  CAnyProjection3D prProjection;
  // create a slave viewer
  CSlaveViewer svViewer(
    m_pCutLineView->GetChildFrame()->m_mvViewer,
    m_pCutLineView->m_ptProjectionType,
    m_plGrid,
    m_pCutLineView->m_pdpDrawPort);
  svViewer.MakeProjection(prProjection);
  prProjection->Prepare();
  // make the ray from viewer point through the dummy point, in current projection
  CPlacement3D plRay;
  prProjection->RayThroughPoint( FLOAT3D(0.0f, 0.0f, 0.0f), plRay);
  // get viewer's direction vector
  FLOAT3D vDirection;
  AnglesToDirectionVector( plRay.pl_OrientationAngle, vDirection);
  
  // make cross product
  FLOAT3D vNormal = vCutLine * vDirection;
  
  // create plane from normal vector and one of cutting edge points
  FLOATplane3D plPolygon(vNormal, vcl0);

  // exit cut mode
  theApp.m_bCutModeOn = FALSE;

  // we need to obtain brush for CSG
  CBrush3D *pbrBrush = NULL;

  // if we are in polygon mode
  if( GetEditingMode() == POLYGON_MODE)
  {
    CBrushPolygon *pbpo = m_selPolygonSelection.GetFirstInSelection();
    if( pbpo == NULL)
    {
      ASSERTALWAYS( "Apply cut called in polygon mode, but none polygon is selected.");
      return;
    }
    pbrBrush = pbpo->bpo_pbscSector->bsc_pbmBrushMip->bm_pbrBrush;
  }
  else if( GetEditingMode() == ENTITY_MODE)
  {
    // do nothing
  }
  else
  {
    // we must be in sector mode
    ASSERT( GetEditingMode() == SECTOR_MODE);

    CBrushSector *pbsc = m_selSectorSelection.GetFirstInSelection();
    if( pbsc == NULL)
    {
      ASSERTALWAYS( "Apply cut called in sector mode, but none sector is selected.");
      return;
    }
    pbrBrush = pbsc->bsc_pbmBrushMip->bm_pbrBrush;
  }
  // brush containing polygon or sector will be our target entity
  CEntity *penTarget = NULL;
  if( pbrBrush != NULL)
  {
    penTarget=pbrBrush->br_penEntity;
  }
  RememberUndo();

  // ------------------- Create cutting world
  // obtain child frame
  CChildFrame *pWedChild = pwedView->GetChildFrame();  
  // remember auto mip brushing flag
  BOOL bAutoMipBrushingOn = pWedChild->m_bAutoMipBrushingOn;
  // turn off auto mip brushing
  pWedChild->m_bAutoMipBrushingOn = FALSE;

  // create world cutter
  CWorld woCutter;
  // create zero-placement
  CPlacement3D plOrigin;
  plOrigin.pl_PositionVector = FLOAT3D(0.0f,0.0f,0.0f);
  plOrigin.pl_OrientationAngle = ANGLE3D(0,0,0);
  // create main brush entity
  CEntity *penCutter = NULL;
  try
  {
    penCutter = woCutter.CreateEntity_t( plOrigin, CTFILENAME("Classes\\WorldBase.ecl"));
  }
  catch (const char *err_str)
  {
    AfxMessageBox( CString(err_str));
    return;
  }
  // prepare the entity
  penCutter->Initialize();

  if( GetEditingMode() == ENTITY_MODE)
  {
  }
  else
  {
    // ------------------- Create brush mip and sector
    CBrush3D *pbr = penCutter->GetBrush();
    // brush must exist
    if( pbr == NULL)
    {
      ASSERTALWAYS( "Brush not properly initialized!");
      return;
    }

    FLOATaabbox3D box;
    // obtain bounding box of selected polygons/sectors
    if( GetEditingMode() == POLYGON_MODE)
    {
      FOREACHINDYNAMICCONTAINER(m_selPolygonSelection, CBrushPolygon, itbpo)
      {
        box |= itbpo->bpo_boxBoundingBox;
      }
    }
    else if( GetEditingMode() == SECTOR_MODE)
    {
      FOREACHINDYNAMICCONTAINER(m_selSectorSelection, CBrushSector, itbsc)
      {
        box |= itbsc->bsc_boxBoundingBox;
      }
    }

    // create object3d to hold plane-cutter primitive
    CObject3D o3d;
    CreateCuttingWorld( plPolygon, box, o3d);
  
    // convert object 3d to brush
    try
    {
      pbr->FromObject3D_t( o3d);
      pbr->CalculateBoundingBoxes();
    }
    // report errors
    catch (const char *err_str)
    {
      AfxMessageBox( CString(err_str));
    }


    // -------- Apply BSP cut operation
    if( GetEditingMode() == POLYGON_MODE)
    {
      // clear all selections except polygon seletion
      ClearSelections( ST_POLYGON);
      // apply "split polygons"
      m_woWorld.SplitPolygons(*penTarget, m_selPolygonSelection, woCutter, *penCutter, plOrigin);
    }
    else
    {
      // clear all selections except sector seletion
      ClearSelections( ST_SECTOR);
      // apply "split sectors"
      m_woWorld.SplitSectors(*penTarget, m_selSectorSelection, woCutter, *penCutter, plOrigin);
    }
  }

  // restore auto mip brushing
  pWedChild->m_bAutoMipBrushingOn = bAutoMipBrushingOn;
  m_chSelections.MarkChanged();
  SetModifiedFlag(TRUE);
  m_chDocument.MarkChanged();
  UpdateAllViews( NULL);
}

void CWorldEditorDoc::ReloadWorld(void)
{
  // clear selections
  ClearSelections();
  m_chDocument.MarkChanged();
  // try to
  try
  {
    // load new world from the undo file
    m_woWorld.Load_t(m_woWorld.wo_fnmFileName);
    // flush stale caches
    _pShell->Execute("FreeUnusedStock();");
    // invalidate document (i.e. all views)
    UpdateAllViews( NULL);
  }
  // report errors
  catch (const char *err_str)
  {
    AfxMessageBox( CString(err_str));
  }
}

void CWorldEditorDoc::OnCheckEdit(void)
{
  CTFileName fnmFileName;
  ExpandFilePath(EFP_READ, m_woWorld.wo_fnmFileName, fnmFileName);

  CTString strCommand;
  strCommand.PrintF("p4 edit %s", fnmFileName);

  INDEX iResult = system(strCommand);
  if(iResult != 0) {
    WarningMessage( "Unable to perform open for edit!");
    return;
  }

  ReloadWorld();

  CTString strMessage;
  strMessage.PrintF("Opened for edit: %s", (const char *)m_woWorld.wo_fnmFileName);
  AfxMessageBox( CString(strMessage));
}

void CWorldEditorDoc::OnCheckAdd() 
{
  CTFileName fnmFileName;
  ExpandFilePath(EFP_READ, m_woWorld.wo_fnmFileName, fnmFileName);

  CTString strCommand;
  strCommand.PrintF("p4 add %s", fnmFileName);

  INDEX iResult = system(strCommand);
  if(iResult != 0) {
    WarningMessage( "Unable to perform open for add!");
    return;
  }

  ReloadWorld();

  CTString strMessage;
  strMessage.PrintF( "Marked for add: %s", (const char *)m_woWorld.wo_fnmFileName);
  AfxMessageBox( CString(strMessage));
}

void CWorldEditorDoc::OnCheckDelete() 
{
  CTFileName fnmFileName;
  ExpandFilePath(EFP_READ, m_woWorld.wo_fnmFileName, fnmFileName);

  CTString strCommand;
  strCommand.PrintF("p4 delete %s", fnmFileName);

  INDEX iResult = system(strCommand);
  if(iResult != 0) {
    WarningMessage( "Unable to perform open for delete!");
    return;
  }

  ReloadWorld();

  CTString strMessage;
  strMessage.PrintF( "Marked for delete: %s", (const char *)m_woWorld.wo_fnmFileName);
  AfxMessageBox( CString(strMessage));
}


BOOL CWorldEditorDoc::IsReadOnly(void) 
{
  return IsFileReadOnly(m_woWorld.wo_fnmFileName);
}

void CWorldEditorDoc::OnUpdateCheckEdit(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable(TRUE);
}

void CWorldEditorDoc::OnUpdateCheckAdd(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable(TRUE);
}

void CWorldEditorDoc::OnUpdateCheckDelete(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable(TRUE);
}

BOOL CWorldEditorDoc::IsBrushUpdatingAllowed(void) 
{
  // if only one entity is selected
  if( m_selEntitySelection.Count()==1)
  {
    // get only selected entity
    CEntity *pen = m_selEntitySelection.GetFirstInSelection();
    // if it is brush entity
    if (pen->en_RenderType == CEntity::RT_BRUSH && pen->en_pbrBrush!=NULL)
    {
      // allow updating
      return TRUE;
    }
  }
  // disable updating
  return	FALSE;
}

void CWorldEditorDoc::OnUpdateBrushes() 
{
  POSITION pos = GetFirstViewPosition();
  CWorldEditorView *pWedView = (CWorldEditorView *) GetNextView(pos);
  ASSERT( pWedView != NULL);
  RememberUndo();
  // get only selected entity
  CEntity *pen = m_selEntitySelection.GetFirstInSelection();
  CTString strClone=pen->GetName();
  FOREACHINDYNAMICCONTAINER(m_woWorld.wo_cenEntities, CEntity, iten)
  {
    if(iten!=pen &&
       iten->GetName()==strClone &&
       iten->en_RenderType==CEntity::RT_BRUSH)
    {
      iten->en_pbrBrush->Copy(*pen->en_pbrBrush, 1.0f, FALSE);
      pWedView->DiscardShadows( &*iten);
    }
  }
  UpdateAllViews( NULL);
}


void CWorldEditorDoc::OnInsert3dObject() 
{
  theApp.Insert3DObjects(this);
}

void CWorldEditorDoc::OnExport3dObject() 
{
  CTFileName fnName = _EngineGUI.FileRequester( "Export polygons as ...",
    "Raw 3D object\0*.raw\0" FILTER_ALL FILTER_END, "Export geometry directory", "Worlds\\");
  if( fnName == "") return;
  
  try
  {
    CTFileStream strmFile;
    strmFile.Create_t( fnName);  
    strmFile.PutLine_t("No name");
    // for each of the selected polygons
    FOREACHINDYNAMICCONTAINER( m_selPolygonSelection, CBrushPolygon, itbpo)
    {
      CBrushPolygon &bpo = *itbpo;
      for( INDEX iVtx=0; iVtx<bpo.bpo_aiTriangleElements.Count(); iVtx+=3)
      {
        CBrushVertex &vtx1=*bpo.bpo_apbvxTriangleVertices[bpo.bpo_aiTriangleElements[iVtx+0]];
        CBrushVertex &vtx2=*bpo.bpo_apbvxTriangleVertices[bpo.bpo_aiTriangleElements[iVtx+1]];
        CBrushVertex &vtx3=*bpo.bpo_apbvxTriangleVertices[bpo.bpo_aiTriangleElements[iVtx+2]];
        CTString strTemp;
        strTemp.PrintF("%g %g %g  %g %g %g  %g %g %g",
          vtx1.bvx_vAbsolute(1), vtx1.bvx_vAbsolute(2), vtx1.bvx_vAbsolute(3),
          vtx2.bvx_vAbsolute(1), vtx2.bvx_vAbsolute(2), vtx2.bvx_vAbsolute(3),
          vtx3.bvx_vAbsolute(1), vtx3.bvx_vAbsolute(2), vtx3.bvx_vAbsolute(3));
        strmFile.PutLine_t( strTemp);
      }
    }
    strmFile.Close();
  }
  catch (const char *err_str)
  {
    AfxMessageBox( CString(err_str));
  }
}

void CWorldEditorDoc::OnUpdateExport3dObject(CCmdUI* pCmdUI) 
{
  pCmdUI->Enable( m_selPolygonSelection.Count()!=0);
}

void CWorldEditorDoc::OnPopupVtxAllign() 
{
  CDlgAllignVertices dlg;
  dlg.DoModal();
}

void CWorldEditorDoc::OnPopupVtxFilter() 
{
  CDlgFilterVertexSelection dlg;
  dlg.DoModal();
}

void CWorldEditorDoc::OnPopupVtxNumeric() 
{
  CDlgSnapVertex dlg;
  dlg.DoModal();
}


void CWorldEditorDoc::OnExportPlacements()
{
  CStaticStackArray<CTString> astrNeddedSmc;
  try
  {
    CTFileName fnWorld=m_woWorld.wo_fnmFileName;
    // "entity placement and names"
    CTFileName fnExport=fnWorld.FileDir()+fnWorld.FileName()+".epn";
    // open text file
    CTFileStream strmFile;
    strmFile.Create_t( fnExport, CTStream::CM_TEXT);
    // for each entity in world
    FOREACHINDYNAMICCONTAINER(m_woWorld.wo_cenEntities, CEntity, iten)
    {
      CEntity &en=*iten;
      // obtain entity class ptr
      CDLLEntityClass *pdecDLLClass = en.GetClass()->ec_pdecDLLClass;

      // obtain position
      FLOAT3D vPos=en.GetPlacement().pl_PositionVector;
      FLOAT3D vRot=en.GetPlacement().pl_OrientationAngle;

      // dump class name and placement
      CTString strLine;
      CTString strName=en.GetName();
      if(strName=="") {
        strName="Dummy name";
      }
      strLine.PrintF("Class: \"%s\", Name: \"%s\", Position: (%f, %f, %f), Rotation: (%f, %f, %f)",
        pdecDLLClass->dec_strName, strName, vPos(1), vPos(2), vPos(3), vRot(1), vRot(2), vRot(3));
      strmFile.PutLine_t(strLine);

      // if this is model holder 3 class, we should also dump model path
      if(CTString(pdecDLLClass->dec_strName)=="ModelHolder3")
      {
        CTFileName fnmFile=CTString("Unknown");
        FLOAT3D vStretch=FLOAT3D(1.0f,1.0f,1.0f);
        // for all classes in hierarchy of this entity
        for(;pdecDLLClass!=NULL; pdecDLLClass = pdecDLLClass->dec_pdecBase)
        {
          // for all properties
          for(INDEX iProperty=0; iProperty<pdecDLLClass->dec_ctProperties; iProperty++)
          {
            CEntityProperty *pepProperty = &pdecDLLClass->dec_aepProperties[iProperty];
            if( pepProperty->ep_eptType == CEntityProperty::EPT_FILENAME &&
                CTString(pepProperty->ep_strName) == "Model file (.smc)")
            {
              // obtain file name
              fnmFile = ENTITYPROPERTY( &en, pepProperty->ep_slOffset, CTFileName);
              BOOL bExistsInList=FALSE;
              for(INDEX iSmc=0; iSmc<astrNeddedSmc.Count(); iSmc++)
              {
                if(astrNeddedSmc[iSmc]==CTString(fnmFile))
                {
                  bExistsInList=TRUE;
                  break;
                }
              }
              if(!bExistsInList)
              {
                CTString &strNew=astrNeddedSmc.Push();
                strNew=CTString(fnmFile);
              }
            }
            if( pepProperty->ep_eptType == CEntityProperty::EPT_FLOAT &&
                CTString(pepProperty->ep_strName) == "StretchAll")
            {
              FLOAT fStretchAll = ENTITYPROPERTY( &en, pepProperty->ep_slOffset, FLOAT);
              vStretch(1)*=fStretchAll;
              vStretch(2)*=fStretchAll;
              vStretch(3)*=fStretchAll;
            }
            if( pepProperty->ep_eptType == CEntityProperty::EPT_ANGLE3D &&
                CTString(pepProperty->ep_strName) == "StretchXYZ")
            {
              ANGLE3D vStretchXYZ = ENTITYPROPERTY( &en, pepProperty->ep_slOffset, ANGLE3D);
              vStretch(1)*=vStretchXYZ(1);
              vStretch(2)*=vStretchXYZ(2);
              vStretch(3)*=vStretchXYZ(3);
            }
          }
        }
        CTString strLine;
        strLine.PrintF("Smc: \"%s\" Stretch: (%f, %f, %f)", CTString(fnmFile), vStretch(1), vStretch(2), vStretch(3));
        strmFile.PutLine_t(strLine);
      }
    }
    
    // "entity placement and names"
    CTFileName fnSml=fnWorld.FileDir()+fnWorld.FileName()+".sml";
    // open text file
    CTFileStream strmSmlFile;
    strmSmlFile.Create_t( fnSml, CTStream::CM_TEXT);
    // save needed smc's
    for(INDEX iSmc=0; iSmc<astrNeddedSmc.Count(); iSmc++)
    {
      strmSmlFile.PutLine_t(astrNeddedSmc[iSmc]);
    }

    AfxMessageBox(L"Placements exported!", MB_OK|MB_ICONINFORMATION);
  }
  catch (const char *err_str)
  {
    AfxMessageBox( CString(err_str));
  }
}

CTFileName CorrectSlashes(const CTFileName &fnmFile)
{
  char afnmSlash[1024];
  for(INDEX iChar=0; iChar<fnmFile.Length(); iChar++) {
    afnmSlash[iChar] = fnmFile[iChar];
    if(afnmSlash[iChar]=='\\') {
      afnmSlash[iChar] = '/';
    }
  }
  // end the string
  afnmSlash[fnmFile.Length()] = 0;
  return CTString(afnmSlash);
}

// Detects detail texture and replaces it with normal map texture
CTFileName RemapDetailTexturePath(CTFileName &fnmFile)
{
  if(fnmFile.FindSubstr("/Detail/") >=0) {
    return fnmFile.FileDir() + fnmFile.FileName() + "_NM.tex";
  }
  return fnmFile;
}

CTString FixQuotes(const CTString &strOrg)
{
  char achrFixed[1024];
  INDEX iFixedChar = 0;
  for(INDEX iChar=0; iChar<strOrg.Length(); iChar++) {
    // if we found a quote
    if(strOrg[iChar]=='\"') {
      // replace it with \"
      achrFixed[iFixedChar++] = '\\';
      achrFixed[iFixedChar++] = '\"';
    } else {
      achrFixed[iFixedChar++] = strOrg[iChar];
    }
  }
  // end the string
  achrFixed[iFixedChar] = 0;
  return CTString(achrFixed);
}

// Class used to represent polygon during brush to .amf exporting
class CAmfNGon {
public:
  CDynamicContainer<CBrushVertex> ang_cbpoVertices;
public:
  void FromBrushPolygon(CBrushPolygon *pbpo);
};

// Class used to represent polygon during brush to .amf exporting
class CAmfPolygon {
public:
  CStaticStackArray<CAmfNGon> amfp_aangNgons;
public:
  void FromBrushPolygon(CBrushPolygon *pbpo);
};

// Creates polygon consisting of vertex loop from brush polygon
void CAmfPolygon::FromBrushPolygon(CBrushPolygon *pbpo)
{
    // make copy of the index array
  CStaticArray<INDEX> aiTriangles;
  aiTriangles.CopyArray( pbpo->bpo_aiTriangleElements);
_nextNgon:
  // copy loop into n-gon
  CAmfNGon &aNgon = amfp_aangNgons.Push();
  // find first triangle that is not handled
  INDEX iFirstNGonTriangle = -1;
  {for(INDEX iTri=0; iTri<aiTriangles.Count()/3; iTri++) {
    if(aiTriangles[iTri*3] != -1) {
      iFirstNGonTriangle=iTri;
      break;
    }
  }}
  // triangle must be found
  if(iFirstNGonTriangle==-1) {
    return;
  }
  
  // and add it to the loop and mark them as handled
  aNgon.ang_cbpoVertices.Add( pbpo->bpo_apbvxTriangleVertices[aiTriangles[iFirstNGonTriangle*3+0]]); aiTriangles[iFirstNGonTriangle*3+0] = -1;
  aNgon.ang_cbpoVertices.Add( pbpo->bpo_apbvxTriangleVertices[aiTriangles[iFirstNGonTriangle*3+1]]); aiTriangles[iFirstNGonTriangle*3+1] = -1;
  aNgon.ang_cbpoVertices.Add( pbpo->bpo_apbvxTriangleVertices[aiTriangles[iFirstNGonTriangle*3+2]]); aiTriangles[iFirstNGonTriangle*3+2] = -1;
  
  // re-entry point for expanding loop
_nextLoopEdge:;
  // for each loop's edge
  for(INDEX iLoopEdge=0; iLoopEdge<aNgon.ang_cbpoVertices.Count(); iLoopEdge++) {
    // get edge vertices
    CBrushVertex *pbvLoop0 = &aNgon.ang_cbpoVertices[iLoopEdge];
    CBrushVertex *pbvLoop1 = &aNgon.ang_cbpoVertices[(iLoopEdge+1)%aNgon.ang_cbpoVertices.Count()];
    // find triangle that shares edge
    for(INDEX iTri=0; iTri<aiTriangles.Count()/3; iTri++) {
      // for each edge in triangle
      for(INDEX iTriEdge=0; iTriEdge<3; iTriEdge++) {
        // fetch edge vertex indices
        INDEX iTriVtx0 = aiTriangles[iTri*3+iTriEdge];
        INDEX iTriVtx1 = aiTriangles[iTri*3+(iTriEdge+1)%3];
        // if triangle is already  handled
        if(iTriVtx0==-1 || iTriVtx1==-1) {
          break;
        }
        CBrushVertex *pbvEdg0 = pbpo->bpo_apbvxTriangleVertices[iTriVtx0];
        CBrushVertex *pbvEdg1 = pbpo->bpo_apbvxTriangleVertices[iTriVtx1];
        // if this edge is the same as the loop edge
        if(pbvLoop0==pbvEdg1 && pbvLoop1==pbvEdg0) {
          // find index of vertex to insert (third vertex)
          INDEX iThirdVtxNo;
          if(iTriEdge==0) { iThirdVtxNo=2;}
          else if(iTriEdge==1) { iThirdVtxNo=0;}
          else { iThirdVtxNo=1;}
          INDEX iThirdVertex = aiTriangles[iTri*3+iThirdVtxNo];
          // mark that triangle is integrated into the loop
          aiTriangles[iTri*3+0] = -1;
          aiTriangles[iTri*3+1] = -1;
          aiTriangles[iTri*3+2] = -1;
          CBrushVertex *pbvThird = pbpo->bpo_apbvxTriangleVertices[iThirdVertex];
          aNgon.ang_cbpoVertices.Insert(pbvThird, iLoopEdge+1);
          goto _nextLoopEdge;
        }
      }
    }
  }

  // test if all triangles are cleared
  {for(INDEX iTri=0; iTri<aiTriangles.Count()/3; iTri++) {
    // if not all are cleared
    if(aiTriangles[iTri*3] != -1) {
      // add another ngon
      goto _nextNgon;
    }
  }}
}

// Types of exported types
enum ExportType {
  ET_RENDERING,
  ET_VISIBILITY,
};

// Class used to collect surface data for brush to .amf exporting
class CAmfSurface {
public:
  CDynamicContainer<CBrushPolygon> sf_cbpoPolygons;
  CAnimData *sf_padAnimData; // surface's texture
  UBYTE sf_ubMaterial; // surface's material
public:
  CAmfSurface(void) {
    sf_padAnimData = NULL;
  };
  
  // Calculates count of ngons
  INDEX GetNGonCount(void) {
    INDEX ctNgons = 0;
    for(INDEX iPlg=0; iPlg<sf_cbpoPolygons.Count(); iPlg++) {
      CBrushPolygon &bpo = sf_cbpoPolygons[iPlg];
      CAmfPolygon *amfp = (CAmfPolygon *)bpo.bpo_pspoScreenPolygon;
      ctNgons += amfp->amfp_aangNgons.Count();
    }
    return ctNgons;
  };

  // Calculates count of ngon vertices
  INDEX GetNGonVertexCount(void) {
    INDEX ctVertices = 0;
    for(INDEX iPlg=0; iPlg<sf_cbpoPolygons.Count(); iPlg++) {
      CBrushPolygon &bpo = sf_cbpoPolygons[iPlg];
      CAmfPolygon *amfp = (CAmfPolygon *)bpo.bpo_pspoScreenPolygon;
      for(INDEX iNgon=0; iNgon<amfp->amfp_aangNgons.Count(); iNgon++) {
        CAmfNGon &aNgon = amfp->amfp_aangNgons[iNgon];
        ctVertices += aNgon.ang_cbpoVertices.Count();
      }
    }
    return ctVertices;
  };
};

// Tests if given polygon is visible
BOOL IsPolygonVisible(const CBrushPolygon &bpo)
{
  // if is invisible
  if(bpo.bpo_ulFlags&BPOF_INVISIBLE) {
    return FALSE;
  }

  // if is portal
  if(bpo.bpo_ulFlags&BPOF_PORTAL) {
    // if is translucent portal
    if(bpo.bpo_ulFlags&BPOF_TRANSLUCENT) {
      // it is visible
      return TRUE;
    }
    return FALSE;
  }
  return TRUE;
}

// Exports one layer of given type
void ExportLayer_t(CWorldEditorDoc *pDoc, CEntity &en, ExportType etExportType, CBrushMip *pbmMip, CTFileStream &strmAmf,
                   const CString &strLayerName, INDEX iLayerNo, BOOL bFieldBrush, BOOL bCollisionOnlyBrush)
{
  // sort brush polygons for their textures
  CDynamicContainer<CAmfSurface> cbpoSurfaces;

  // assume that there will not be any portals nor occluders
  CStaticStackArray<INDEX> ciPortals;
  CStaticStackArray<INDEX> ciOccluders;
  CStaticStackArray<INDEX> ciClassifiers;
  
  // for each sector in the brush mip
  INDEX iPlgGlobal=0;
  {for(INDEX iSector=0; iSector<pbmMip->bm_abscSectors.Count(); iSector++) {
    CBrushSector &bs = pbmMip->bm_abscSectors[iSector];
    // for each polygon in the sector
    for(INDEX iPlg=0; iPlg<bs.bsc_abpoPolygons.Count(); iPlg++) {
      CBrushPolygon &bpo = bs.bsc_abpoPolygons[iPlg];
      // if we are exporting collision (e.g. for empty brushes)
      if(etExportType==ET_RENDERING) {
        if(!bFieldBrush && (!IsPolygonVisible(bpo) && !bCollisionOnlyBrush) ) {
          continue;
        }
        CAnimData *pad = bpo.bpo_abptTextures[0].bpt_toTexture.GetData();
        UBYTE ubMaterial = bpo.bpo_bppProperties.bpp_ubSurfaceType;
        BOOL bFound = FALSE;
        for(INDEX iSurf=0; iSurf<cbpoSurfaces.Count(); iSurf++) {
          CAmfSurface &asSurf = cbpoSurfaces[iSurf];
          // if this surface for the texture-surface pair is already defined
          if( (asSurf.sf_padAnimData==pad) && (asSurf.sf_ubMaterial==ubMaterial) ) {
            // add polygon to existing surface
            asSurf.sf_cbpoPolygons.Add(&bpo);
            bFound = TRUE;
            break;
          }
        }
        // if surface with current texture and material is not yet defined
        if(!bFound) {
          CAmfSurface *pSurf = (CAmfSurface *) new(CAmfSurface);
          cbpoSurfaces.Add(pSurf);
          pSurf->sf_cbpoPolygons.Add(&bpo);
          pSurf->sf_padAnimData = pad;
          pSurf->sf_ubMaterial = ubMaterial;
        }
      } else if(etExportType==ET_VISIBILITY) {
        BOOL bClassifier = FALSE;
        BOOL bOccluder = FALSE;
        BOOL bPortal = FALSE;
        // if this polygon is not involved in visibility
        if(bpo.bpo_ulFlags&BPOF_DETAILPOLYGON) {
          bClassifier = TRUE;
        }
        else if(bpo.bpo_ulFlags&BPOF_OCCLUDER) {
          bOccluder = TRUE;
        }
        else if(bpo.bpo_ulFlags&BPOF_PORTAL) {
          bPortal = TRUE;
        }
        // if surface is not yet defined
        if(cbpoSurfaces.Count()==0) {
          // add one
          CAmfSurface *pSurf = (CAmfSurface *) new(CAmfSurface);
          cbpoSurfaces.Add(pSurf);
        }

        CAmfPolygon *amfp = (CAmfPolygon *)bpo.bpo_pspoScreenPolygon;
        for(INDEX iNgon=0; iNgon<amfp->amfp_aangNgons.Count(); iNgon++) {
          if(bClassifier) { ciClassifiers.Push() = iPlgGlobal; }
          if(bOccluder) { ciOccluders.Push() = iPlgGlobal; }
          if(bPortal) { ciPortals.Push() = iPlgGlobal; }
          iPlgGlobal++;
        }
        cbpoSurfaces[0].sf_cbpoPolygons.Add(&bpo);
      }
    }
  }}

  // count total surface polygons and vertices
  INDEX ctTotalPolygons = 0;
  INDEX ctTotalVertices = 0;
  {for(INDEX iSurf=0; iSurf<cbpoSurfaces.Count(); iSurf++) {
    CAmfSurface &amfs = cbpoSurfaces[iSurf];
    ctTotalPolygons+=amfs.GetNGonCount();
    ctTotalVertices+=amfs.GetNGonVertexCount();
  }}

  // if there is no polygons to export
  if(ctTotalPolygons==0) {
    return;
  }
  
  strmAmf.FPrintF_t("  LAYER_NAME \"%s\"\n", strLayerName);
  strmAmf.FPrintF_t("  LAYER_INDEX %d\n", iLayerNo);
  strmAmf.PutLine_t("  {");
  INDEX ctVertexMaps = etExportType==ET_RENDERING ? 4 : 1;
  strmAmf.FPrintF_t("    VERTEX_MAPS %d\n", ctVertexMaps);
  strmAmf.PutLine_t("    {");
  strmAmf.PutLine_t("      VERTEX_MAP \"morph.position\"");
  strmAmf.PutLine_t("      {");
  strmAmf.FPrintF_t("        ELEMENTS %d\n", ctTotalVertices);
  strmAmf.PutLine_t("        {");
  {for(INDEX iSurf=0; iSurf<cbpoSurfaces.Count(); iSurf++) {
    CAmfSurface &asSurf = cbpoSurfaces[iSurf];
    for(INDEX iPlg=0; iPlg<asSurf.sf_cbpoPolygons.Count(); iPlg++) {
      CBrushPolygon &bpo = asSurf.sf_cbpoPolygons[iPlg];
      CAmfPolygon *amfp = (CAmfPolygon *)bpo.bpo_pspoScreenPolygon;
      for(INDEX iNgon=0; iNgon<amfp->amfp_aangNgons.Count(); iNgon++) {
        CAmfNGon &aNgon = amfp->amfp_aangNgons[iNgon];
        for(INDEX iVtx=0; iVtx<aNgon.ang_cbpoVertices.Count(); iVtx++) {
          CBrushVertex &bVtx = aNgon.ang_cbpoVertices[iVtx];
          strmAmf.FPrintF_t("          { %f, %f, %f; }\n", bVtx.bvx_vdPreciseRelative(1), bVtx.bvx_vdPreciseRelative(2), bVtx.bvx_vdPreciseRelative(3));
        }
      }
    }
  }}
  strmAmf.PutLine_t("        }");
  strmAmf.PutLine_t("      }");
  if(etExportType==ET_RENDERING) {
    for(INDEX iTextureLayer=0; iTextureLayer<3; iTextureLayer++) {
      strmAmf.FPrintF_t("      VERTEX_MAP \"texcoord.Texture %d\"", iTextureLayer+1);
      strmAmf.PutLine_t("      {");
      strmAmf.FPrintF_t("        ELEMENTS %d\n", ctTotalVertices);
      strmAmf.PutLine_t("        {");
      {for(INDEX iSurf=0; iSurf<cbpoSurfaces.Count(); iSurf++) {
        CAmfSurface &asSurf = cbpoSurfaces[iSurf];
        for(INDEX iPlg=0; iPlg<asSurf.sf_cbpoPolygons.Count(); iPlg++) {
          CBrushPolygon &bpo = asSurf.sf_cbpoPolygons[iPlg];
          // fetch mapping parameters
          CMappingVectors mvDefault;
          mvDefault.FromPlane_DOUBLE(bpo.bpo_pbplPlane->bpl_pldPreciseRelative);
          // calculate mapping transformation vectors
          CMappingDefinition &md = bpo.bpo_abptTextures[iTextureLayer].bpt_mdMapping;
          // if there is no texture
          MEX mexTexSizeU, mexTexSizeV;
          if(bpo.bpo_abptTextures[iTextureLayer].bpt_toTexture.GetData()==NULL) {
            mexTexSizeU = 1024;
            mexTexSizeV = 1024;
          } else {
            mexTexSizeU = bpo.bpo_abptTextures[iTextureLayer].bpt_toTexture.GetWidth();
            mexTexSizeV = bpo.bpo_abptTextures[iTextureLayer].bpt_toTexture.GetHeight();
          }
          const FLOAT fMulU = 1024.0f /mexTexSizeU;  // (no need to do shift-opt, because it won't speed up much!)
          const FLOAT fMulV = 1024.0f /mexTexSizeV;

          CMappingVectors mvTransform;
          md.MakeMappingVectors(mvDefault, mvTransform);
          CAmfPolygon *amfp = (CAmfPolygon *)bpo.bpo_pspoScreenPolygon;
          for(INDEX iNgon=0; iNgon<amfp->amfp_aangNgons.Count(); iNgon++) {
            CAmfNGon &aNgon = amfp->amfp_aangNgons[iNgon];
            for(INDEX iVtx=0; iVtx<aNgon.ang_cbpoVertices.Count(); iVtx++) {
              CBrushVertex &bVtx = aNgon.ang_cbpoVertices[iVtx];
              // calculate mapping coordinates
              FLOAT3D vUV = bVtx.bvx_vRelative-mvTransform.mv_vO;
              FLOAT fU = vUV % mvTransform.mv_vU;
              FLOAT fV = vUV % mvTransform.mv_vV;
              fU *= fMulU;
              fV *= fMulV;
              // fix qnans
              if(!_finite(fU)) { fU=0;}
              if(!_finite(fV)) { fV=0;}
              strmAmf.FPrintF_t("          { %f, %f; }\n", fU, fV);
            }
          }
        }
      }}
      strmAmf.PutLine_t("        }");
      strmAmf.PutLine_t("      }");
    }
  }
  strmAmf.PutLine_t("    }");
  strmAmf.FPrintF_t("    VERTICES %d\n", ctTotalVertices);
  strmAmf.PutLine_t("    {");
  for(INDEX iVtx=0; iVtx<ctTotalVertices; iVtx++) {
    if(etExportType==ET_RENDERING) {
      strmAmf.FPrintF_t("      { 4: 0[%d], 1[%d], 2[%d], 3[%d];}\n", iVtx, iVtx, iVtx, iVtx);
    } else {
      strmAmf.FPrintF_t("      { 1: 0[%d]; }\n", iVtx);
    }
  }
  strmAmf.PutLine_t("    }");
  strmAmf.FPrintF_t("    POLYGONS %d\n", ctTotalPolygons);
  strmAmf.PutLine_t("    {");
  INDEX iPlgVtx = 0;
  {for(INDEX iSurf=0; iSurf<cbpoSurfaces.Count(); iSurf++) {
    CAmfSurface &asSurf = cbpoSurfaces[iSurf];
    for(INDEX iPlg=0; iPlg<asSurf.sf_cbpoPolygons.Count(); iPlg++) {
      CBrushPolygon &bpo = asSurf.sf_cbpoPolygons[iPlg];
      CAmfPolygon *amfp = (CAmfPolygon *)bpo.bpo_pspoScreenPolygon;
      for(INDEX iNgon=0; iNgon<amfp->amfp_aangNgons.Count(); iNgon++) {
        CAmfNGon &aNgon = amfp->amfp_aangNgons[iNgon];
        INDEX ctPlgVertices = aNgon.ang_cbpoVertices.Count();
        if(ctPlgVertices==0) {
          strmAmf.FPrintF_t("      { 3: 0, 0, 0; }\n");
        } else {
          strmAmf.FPrintF_t("      { %d: ", ctPlgVertices);
          for(INDEX iVtx=0; iVtx<ctPlgVertices; iVtx++) {
            if(iVtx==ctPlgVertices-1) {
              strmAmf.FPrintF_t("%d; }\n", iPlgVtx++);
            } else {
              strmAmf.FPrintF_t("%d, ", iPlgVtx++);
            }
          }
        }
      }
    }
  }}
  strmAmf.PutLine_t("    }");

  // for rendering
  if(etExportType==ET_RENDERING) {
    strmAmf.FPrintF_t("    POLYGON_MAPS %d\n", cbpoSurfaces.Count());
    strmAmf.PutLine_t("    {");
    // dump surfaces
    INDEX iPlgGlobal=0;
    {for(INDEX iSurf=0; iSurf<cbpoSurfaces.Count(); iSurf++) {
      CAmfSurface &asSurf = cbpoSurfaces[iSurf];
      CBrushPolygon &bpo = asSurf.sf_cbpoPolygons[0];
      strmAmf.FPrintF_t("      POLYGON_MAP_NAME \"surface.Default_%d_%d\"\n", en.en_ulID, iSurf);
#if 1
      // dump surface data
      if(bCollisionOnlyBrush) {
        strmAmf.PutLine_t("      POLYGON_MAP_SHADER \"\"");
      } else {
        strmAmf.PutLine_t("      POLYGON_MAP_SHADER \"Bin/Shaders.module|Standard\"");
      }
      strmAmf.PutLine_t("      {");
      // export material info
      CString strMaterial = pDoc->m_woWorld.wo_astSurfaceTypes[asSurf.sf_ubMaterial].st_strName;
      strmAmf.FPrintF_t("        Material \"%s\";\n", strMaterial);
      // export first layer data
      CTFileName strPath;
      strPath = bpo.bpo_abptTextures[0].bpt_toTexture.GetName();
      strPath = CorrectSlashes(strPath);
      strPath = RemapDetailTexturePath(strPath);
      if(bFieldBrush) {
        strmAmf.FPrintF_t("        \"base color\" Color %d;\n", C_GREEN|128);
        strmAmf.FPrintF_t("        \"blend type\" BlendType \"%translucent\";\n");
        strmAmf.FPrintF_t("        \"double sided\" Bool \"TRUE\";\n");
      } else {
        // setup blend type for translucent portals
        if( (bpo.bpo_ulFlags&BPOF_PORTAL) && (bpo.bpo_ulFlags&BPOF_TRANSLUCENT) ) {
          strmAmf.FPrintF_t("        \"blend type\" BlendType \"%translucent\";\n");
        }
        //strPath.SetAbsolutePath();
        //strPath.ReplaceSubstr("\\", "\\\\");
        //strmAmf.FPrintF_t("        \"base texture\" Texture \"%s\";\n", strPath);
        //strmAmf.FPrintF_t("        \"base uvmap\" UVMap \"Texture 1\";\n");
        strmAmf.FPrintF_t("        \"base color\" Color %d;\n", bpo.bpo_abptTextures[0].s.bpt_colColor);
        // export second layer data
        //strPath = bpo.bpo_abptTextures[1].bpt_toTexture.GetName();
        //strPath = CorrectSlashes(strPath);
        //strPath = RemapDetailTexturePath(strPath);
        //strmAmf.FPrintF_t("        \"blend mask\" Texture \"%s\";\n", strPath);
        //strmAmf.FPrintF_t("        \"mask uvmap\" UVMap \"Texture 2\";\n");
        // export third layer data
        //strPath = bpo.bpo_abptTextures[2].bpt_toTexture.GetName();
        //strPath = CorrectSlashes(strPath);
        //strPath = RemapDetailTexturePath(strPath);
        //strmAmf.FPrintF_t("        \"detail normalmap\" Texture \"%s\";\n", strPath);
        //strmAmf.FPrintF_t("        \"detail uvmap\" UVMap \"Texture 3\";\n");
        //strmAmf.FPrintF_t("        \"tangent uvmap\" UVMap \"Texture 3\";\n");
      }
      strmAmf.PutLine_t("      }");
#else
      strmAmf.PutLine_t("      POLYGON_MAP_SHADER \"Bin/GameSamHD.module|Architecture\"");
      strmAmf.PutLine_t("      {");
      // export material info
      CString strMaterial = pDoc->m_woWorld.wo_astSurfaceTypes[asSurf.sf_ubMaterial].st_strName;
      strmAmf.FPrintF_t("        Material \"%s\";\n", strMaterial);
      // export first layer data
      CTFileName strPath;
      strPath = bpo.bpo_abptTextures[0].bpt_toTexture.GetName();
      strPath = CorrectSlashes(strPath);
      strPath = RemapDetailTexturePath(strPath);
      strmAmf.FPrintF_t("        \"diffuse 1 texture\" Texture \"%s\";\n", strPath);
      strmAmf.FPrintF_t("        \"diffuse 1 uvmap\" UVMap \"Texture 1\";\n");
      // export second layer data
      strPath = bpo.bpo_abptTextures[1].bpt_toTexture.GetName();
      strPath = CorrectSlashes(strPath);
      strPath = RemapDetailTexturePath(strPath);
      strmAmf.FPrintF_t("        \"shade\" Texture \"%s\";\n", strPath);
      strmAmf.FPrintF_t("        \"shade uvmap\" UVMap \"Texture 2\";\n");
      // export third layer data
      strPath = bpo.bpo_abptTextures[2].bpt_toTexture.GetName();
      strPath = CorrectSlashes(strPath);
      strPath = RemapDetailTexturePath(strPath);
      strmAmf.FPrintF_t("        \"normal map 1\" Texture \"%s\";\n", strPath);
      strmAmf.FPrintF_t("        \"normal 1 uvmap\" UVMap \"Texture 3\";\n");
      strmAmf.FPrintF_t("        \"tangent uvmap\" UVMap \"Texture 3\";\n");
      strmAmf.PutLine_t("      }");
#endif
      strmAmf.PutLine_t("      POLYGON_MAP_SMOOTHING_ANGLE 30");
      INDEX ctSurfacePolygons = asSurf.GetNGonCount();
      strmAmf.FPrintF_t("      POLYGONS_COUNT %d\n", ctSurfacePolygons);
      strmAmf.PutLine_t("      {");

      for(INDEX iPlg=0; iPlg<ctSurfacePolygons; iPlg++) {
        strmAmf.FPrintF_t("        %d;\n", iPlgGlobal++);
      }
      strmAmf.PutLine_t("      }");
    }}
    strmAmf.PutLine_t("    }");
  } else if(etExportType==ET_VISIBILITY) {
    // for visibility
    INDEX ctSectors = pbmMip->bm_abscSectors.Count();
    INDEX iOccluderPolyMaps = ciOccluders.Count()>0 ? 1 : 0;
    INDEX iPortalPolyMaps = ciPortals.Count()>0 ? 1 : 0;
    INDEX iClassifierPolyMaps = ciClassifiers.Count()>0 ? 1 : 0;    
    strmAmf.FPrintF_t("    POLYGON_MAPS %d\n", ctSectors+iOccluderPolyMaps+iPortalPolyMaps+iClassifierPolyMaps);
    strmAmf.PutLine_t("    {");
    // dump sectors as separate polygon maps
    {for(INDEX iSector=0; iSector<pbmMip->bm_abscSectors.Count(); iSector++) {
      // count sector polygons
      INDEX ctSectorPolygons = 0;
      CBrushSector *pbs = &pbmMip->bm_abscSectors[iSector];
      {for(INDEX iSurf=0; iSurf<cbpoSurfaces.Count(); iSurf++) {
        CAmfSurface &asSurf = cbpoSurfaces[iSurf];
        for(INDEX iPlg=0; iPlg<asSurf.sf_cbpoPolygons.Count(); iPlg++) {
          CBrushPolygon &bpo = asSurf.sf_cbpoPolygons[iPlg];
          if(bpo.bpo_pbscSector==pbs) {
            CAmfPolygon *amfp = (CAmfPolygon *)bpo.bpo_pspoScreenPolygon;
            INDEX ctNgons = amfp->amfp_aangNgons.Count();
            ctSectorPolygons += ctNgons;
          }
        }
      }}
      strmAmf.FPrintF_t("      POLYGON_MAP_NAME \"sector.Sector_%d\"\n", iSector);
      strmAmf.FPrintF_t("      POLYGONS_COUNT %d\n", ctSectorPolygons);
      strmAmf.PutLine_t("      {");
      // dump polygon indices
      INDEX iPlgGlobal = 0;
      {for(INDEX iSurf=0; iSurf<cbpoSurfaces.Count(); iSurf++) {
        CAmfSurface &asSurf = cbpoSurfaces[iSurf];
        for(INDEX iPlg=0; iPlg<asSurf.sf_cbpoPolygons.Count(); iPlg++) {
          CBrushPolygon &bpo = asSurf.sf_cbpoPolygons[iPlg];
          CAmfPolygon *amfp = (CAmfPolygon *)bpo.bpo_pspoScreenPolygon;
          for(INDEX iNgon=0; iNgon<amfp->amfp_aangNgons.Count(); iNgon++) {
            if(bpo.bpo_pbscSector==pbs) {
              strmAmf.FPrintF_t("        %d;\n", iPlgGlobal);
            }
            iPlgGlobal++;
          }
        }
      }}
      strmAmf.PutLine_t("      }");
    }}

    // dump portals
    if(ciPortals.Count()>0) {
      strmAmf.PutLine_t("      POLYGON_MAP_NAME \"portal.VisPortal\"");
      strmAmf.FPrintF_t("      POLYGONS_COUNT %d\n", ciPortals.Count());
      strmAmf.PutLine_t("      {");
      // dump polygon indices
      {for(INDEX iPlg=0; iPlg<ciPortals.Count(); iPlg++) {
          strmAmf.FPrintF_t("        %d;\n", ciPortals[iPlg]);
      }}
      strmAmf.PutLine_t("      }");
    }

    // dump occluders
    if(ciOccluders.Count()>0) {
      strmAmf.PutLine_t("      POLYGON_MAP_NAME \"portal.VisOccluder\"");
      strmAmf.FPrintF_t("      POLYGONS_COUNT %d\n", ciOccluders.Count());
      strmAmf.PutLine_t("      {");
      // dump polygon indices
      {for(INDEX iPlg=0; iPlg<ciOccluders.Count(); iPlg++) {
          strmAmf.FPrintF_t("        %d;\n", ciOccluders[iPlg]);
      }}
      strmAmf.PutLine_t("      }");
    }

    // dump classifiers
    if(ciClassifiers.Count()>0) {
      strmAmf.PutLine_t("      POLYGON_MAP_NAME \"portal.VisClassifier\"");
      strmAmf.FPrintF_t("      POLYGONS_COUNT %d\n", ciClassifiers.Count());
      strmAmf.PutLine_t("      {");
      // dump polygon indices
      {for(INDEX iPlg=0; iPlg<ciClassifiers.Count(); iPlg++) {
          strmAmf.FPrintF_t("        %d;\n", ciClassifiers[iPlg]);
      }}
      strmAmf.PutLine_t("      }");
    }
    strmAmf.PutLine_t("    }");
  }
  strmAmf.PutLine_t("  }");
}

// Tests if entity brushes are valid
BOOL IsBrushVisible(CEntity &en)
{
  // fetch first mip
  CBrushMip *pbmMip = en.en_pbrBrush->GetFirstMip();
  if(pbmMip==NULL) {
    return FALSE;
  }
  INDEX ctPolygons = 0;
  for(INDEX iSector=0; iSector<pbmMip->bm_abscSectors.Count(); iSector++) {
    CBrushSector &bs = pbmMip->bm_abscSectors[iSector];
    // for each polygon in the sector
    for(INDEX iPlg=0; iPlg<bs.bsc_abpoPolygons.Count(); iPlg++) {
      CBrushPolygon &bpo = bs.bsc_abpoPolygons[iPlg];
      if(!IsPolygonVisible(bpo)) {
        continue;
      }
      ctPolygons++;
    }
  }
  // if there was no polygons to export
  return ctPolygons>0;
}

// Tests if entity brush is empty
BOOL IsBrushEmpty(CEntity &en)
{
  // fetch first mip
  CBrushMip *pbmMip = en.en_pbrBrush->GetFirstMip();
  if(pbmMip==NULL) {
    return FALSE;
  }
  INDEX ctPolygons = 0;
  for(INDEX iSector=0; iSector<pbmMip->bm_abscSectors.Count(); iSector++) {
    CBrushSector &bs = pbmMip->bm_abscSectors[iSector];
    if(bs.bsc_abpoPolygons.Count()>0) {
      return FALSE;
    }
  }
  return TRUE;
}

// Exports given brush mip into .amf format
void ExportEntityToAMF_t(CWorldEditorDoc *pDoc, CEntity &en, const CTFileName &fnAmf, BOOL bFieldBrush, BOOL bInvisibleBrush, BOOL bEmptyBrush)
{
  // fetch first mip
  CBrushMip *pbmMip = en.en_pbrBrush->GetFirstMip();

  // convert all of the brush polygons into ngons
  // for each sector in the brush mip
  {for(INDEX iSector=0; iSector<pbmMip->bm_abscSectors.Count(); iSector++) {
    CBrushSector &bs = pbmMip->bm_abscSectors[iSector];
    // for each polygon in the sector
    for(INDEX iPlg=0; iPlg<bs.bsc_abpoPolygons.Count(); iPlg++) {
      CBrushPolygon &bpo = bs.bsc_abpoPolygons[iPlg];
      // convert it into ngons
      CAmfPolygon *pap = new(CAmfPolygon);
      pap->FromBrushPolygon(&bpo);
      bpo.bpo_pspoScreenPolygon = (CScreenPolygon *) pap;
    }
  }}

  try
  {
    // open .amf file
    CTFileStream strmAmf;
    strmAmf.Create_t( fnAmf, CTStream::CM_TEXT);
    strmAmf.PutLine_t("SE_MESH 1.01");
    strmAmf.PutLine_t("");
    // export visibility for zoning brushes
    INDEX ctLayers = en.en_ulFlags&ENF_ZONING ? 2 : 1;
    if(bEmptyBrush) {
      ctLayers = 0;
    }
    strmAmf.FPrintF_t("LAYERS %d\n", ctLayers);
    strmAmf.PutLine_t("{");
    if(bInvisibleBrush) {
      ExportLayer_t(pDoc, en, ET_RENDERING, pbmMip, strmAmf, "Collision", 0, bFieldBrush, TRUE);
    } else {
      ExportLayer_t(pDoc, en, ET_RENDERING, pbmMip, strmAmf, "Rendering", 0, bFieldBrush, FALSE);
    }
    if(ctLayers>1) {
      ExportLayer_t(pDoc, en, ET_VISIBILITY, pbmMip, strmAmf, "Visibility", 1, bFieldBrush, FALSE);
    }
    strmAmf.PutLine_t("}");
  }
  catch (const char *err_str) {
    AfxMessageBox( CString(err_str));
  }
}

void CWorldEditorDoc::OnExportEntities()
{
  CStaticStackArray<CTString> astrNeddedSmc;
  try
  {
    CTFileName fnWorld=m_woWorld.wo_fnmFileName;
    // "entity placement and names"
    CTFileName fnExport=fnWorld.FileDir()+fnWorld.FileName()+".awf";
    // open text file
    CTFileStream strmFile;
    strmFile.Create_t( fnExport, CTStream::CM_TEXT);

    // prepare container of entities to export
    CDynamicContainer<CEntity> dcEntitiesToExport;

    // for each entity in world
    {FOREACHINDYNAMICCONTAINER(m_woWorld.wo_cenEntities, CEntity, iten)
    {
      CEntity &en=*iten;
      dcEntitiesToExport.Add(&en);
    }}

    // write count of entities
    CTString strLine;
    strLine.PrintF("ENTITIES %d {", dcEntitiesToExport.Count());
    strmFile.PutLine_t(strLine);

    // for each entity in world
    FOREACHINDYNAMICCONTAINER(dcEntitiesToExport, CEntity, iten)
    {
      CEntity &en=*iten;
      // obtain entity class ptr
      CDLLEntityClass *pdecDLLClass = en.GetClass()->ec_pdecDLLClass;

      // obtain position
      FLOAT3D vPos=en.GetPlacement().pl_PositionVector;
      FLOAT3D vRot=en.GetPlacement().pl_OrientationAngle;

      // count entity attributes
      INDEX ctEntityAttributes = 0;
      // for all classes in hierarchy of this entity
      CDLLEntityClass *pdecDLLClassCount = pdecDLLClass;
      for(;pdecDLLClassCount!=NULL; pdecDLLClassCount = pdecDLLClassCount->dec_pdecBase) {
        // for all properties
        for(INDEX iProperty=0; iProperty<pdecDLLClassCount->dec_ctProperties; iProperty++) {
          CEntityProperty *pepProperty = &pdecDLLClassCount->dec_aepProperties[iProperty];
          if(pepProperty->ep_strName!=CTString("")) {
            ctEntityAttributes++;
          }
        }
      }

      // if render type is brush
      if( (en.en_RenderType==CEntity::RT_BRUSH || en.en_RenderType==CEntity::RT_FIELDBRUSH) && en.en_pbrBrush!=NULL) {
        // add one more property because we will add one that will hint "InvisibleBrush"
        ctEntityAttributes++;
      }

      // write count of entity attributes (add 5 fixed ones, for class, ID, spawn flags, parent, name, pos, rot)
      strLine.PrintF("  ENTITY_ATTRIBUTES %d {", ctEntityAttributes+7);
      strmFile.PutLine_t(strLine);

      // entity class
      strLine.PrintF("    \"ENTITYCLASS\" = string(\"SS1 %s\");", pdecDLLClass->dec_strName);
      strmFile.PutLine_t(strLine);
      // entity ID
      strLine.PrintF("    \"ID\" = long(%d);", en.en_ulID);
      strmFile.PutLine_t(strLine);
      // entity spawn flags
      strLine.PrintF("    \"SS1_SPAWN_FLAGS\" = long(%d);", en.en_ulSpawnFlags);
      strmFile.PutLine_t(strLine);
      // entity name
      CTString strName=en.GetName();
      if(strName=="") {
        strName="<unnamed>";
      }
      SLONG idParent=-1;
      CEntity *penParent = en.GetParent();
      if(penParent!=NULL) {
        idParent = penParent->en_ulID;
      }
      strLine.PrintF("    \"PARENT\" = long(%d);", idParent);
      strmFile.PutLine_t(strLine);
      strLine.PrintF("    \"NAME\" = string(\"%s\");", strName);
      strmFile.PutLine_t(strLine);
      // position
      strLine.PrintF("    \"POS\" = float3(%f, %f, %f);", vPos(1), vPos(2), vPos(3));
      strmFile.PutLine_t(strLine);
      // rotation
      strLine.PrintF("    \"ROT\" = float3(%f, %f, %f);", vRot(1), vRot(2), vRot(3));
      strmFile.PutLine_t(strLine);

      // for all classes in hierarchy of this entity
      for(;pdecDLLClass!=NULL; pdecDLLClass = pdecDLLClass->dec_pdecBase) {
        // for all properties
        for(INDEX iProperty=0; iProperty<pdecDLLClass->dec_ctProperties; iProperty++) {
          CEntityProperty *pepProperty = &pdecDLLClass->dec_aepProperties[iProperty];
          if(pepProperty->ep_strName==CTString("")) {
            continue;
          }
          // enumerator
          if( pepProperty->ep_eptType == CEntityProperty::EPT_ENUM) {
            INDEX iEnumValue = ENTITYPROPERTY( &en, pepProperty->ep_slOffset, INDEX);
            strLine.PrintF("    \"%s\" = long(%d);", pepProperty->ep_strName, iEnumValue);
            strmFile.PutLine_t(strLine);
          }
          // boolean
          if( pepProperty->ep_eptType == CEntityProperty::EPT_BOOL) {
            INDEX iBooleanValue = ENTITYPROPERTY( &en, pepProperty->ep_slOffset, BOOL);
            strLine.PrintF("    \"%s\" = long(%d);", pepProperty->ep_strName, iBooleanValue);
            strmFile.PutLine_t(strLine);
          }
          // float value
          if( pepProperty->ep_eptType == CEntityProperty::EPT_FLOAT) {
            FLOAT fFloat = ENTITYPROPERTY( &en, pepProperty->ep_slOffset, FLOAT);
            strLine.PrintF("    \"%s\" = float(%f);", pepProperty->ep_strName, fFloat);
            strmFile.PutLine_t(strLine);
          }
          // color
          if( pepProperty->ep_eptType == CEntityProperty::EPT_COLOR) {
            COLOR colValue = ENTITYPROPERTY( &en, pepProperty->ep_slOffset, COLOR);
            strLine.PrintF("    \"%s\" = long(%d);", pepProperty->ep_strName, colValue);
            strmFile.PutLine_t(strLine);
          }
          // string
          if( pepProperty->ep_eptType == CEntityProperty::EPT_STRING) {
            CTString strString = FixQuotes(ENTITYPROPERTY( &en, pepProperty->ep_slOffset, CTString));
            strLine.PrintF("    \"%s\" = string(\"%s\");", pepProperty->ep_strName, strString);
            strmFile.PutLine_t(strLine);
          }
          // range
          if( pepProperty->ep_eptType == CEntityProperty::EPT_RANGE) {
            FLOAT fFloat = ENTITYPROPERTY( &en, pepProperty->ep_slOffset, FLOAT);
            strLine.PrintF("    \"%s\" = float(%f);", pepProperty->ep_strName, fFloat);
            strmFile.PutLine_t(strLine);
          }
          // entity ptr
          if( pepProperty->ep_eptType == CEntityProperty::EPT_ENTITYPTR) {
            // get the pointer
            CEntityPointer &penPointed = ENTITYPROPERTY( &en, pepProperty->ep_slOffset, CEntityPointer);
            SLONG ulID = penPointed==NULL ? -1 : penPointed->en_ulID;
            strLine.PrintF("    \"%s\" = long(%d);", pepProperty->ep_strName, ulID);
            strmFile.PutLine_t(strLine);
          }          
          // file name
          if( pepProperty->ep_eptType == CEntityProperty::EPT_FILENAME || 
              pepProperty->ep_eptType == CEntityProperty::EPT_FILENAMENODEP) {
            CTFileName fnmFile = CorrectSlashes(ENTITYPROPERTY( &en, pepProperty->ep_slOffset, CTFileName));
            strLine.PrintF("    \"%s\" = string(\"%s\");", pepProperty->ep_strName, fnmFile);
            strmFile.PutLine_t(strLine);
          }
          // index value
          if( pepProperty->ep_eptType == CEntityProperty::EPT_INDEX) {
            INDEX iValue = ENTITYPROPERTY( &en, pepProperty->ep_slOffset, INDEX);
            strLine.PrintF("    \"%s\" = long(%d);", pepProperty->ep_strName, iValue);
            strmFile.PutLine_t(strLine);
          }
          // animation value
          if( pepProperty->ep_eptType == CEntityProperty::EPT_ANIMATION) {
            INDEX iValue = ENTITYPROPERTY( &en, pepProperty->ep_slOffset, INDEX);
            strLine.PrintF("    \"%s\" = long(%d);", pepProperty->ep_strName, iValue);
            strmFile.PutLine_t(strLine);
          }
          // illumination type
          if( pepProperty->ep_eptType == CEntityProperty::EPT_ILLUMINATIONTYPE) {
            INDEX iValue = ENTITYPROPERTY( &en, pepProperty->ep_slOffset, INDEX);
            strLine.PrintF("    \"%s\" = long(%d);", pepProperty->ep_strName, iValue);
            strmFile.PutLine_t(strLine);
          }
          // angle
          if( pepProperty->ep_eptType == CEntityProperty::EPT_ANGLE) {
            INDEX iValue = ENTITYPROPERTY( &en, pepProperty->ep_slOffset, INDEX);
            strLine.PrintF("    \"%s\" = long(%d);", pepProperty->ep_strName, iValue);
            strmFile.PutLine_t(strLine);
          }
          // float 3D
          if( pepProperty->ep_eptType == CEntityProperty::EPT_FLOAT3D) {
            FLOAT3D vValue = ENTITYPROPERTY( &en, pepProperty->ep_slOffset, FLOAT3D);
            strLine.PrintF("    \"%s\" = float3(%f, %f, %f);", pepProperty->ep_strName, vValue(1), vValue(2), vValue(3));
            strmFile.PutLine_t(strLine);
          }
          // angle 3D
          if( pepProperty->ep_eptType == CEntityProperty::EPT_ANGLE3D) {
            ANGLE3D vAngle3D = ENTITYPROPERTY( &en, pepProperty->ep_slOffset, ANGLE3D);
            strLine.PrintF("    \"%s\" = float3(%f, %f, %f);", pepProperty->ep_strName, vAngle3D(1), vAngle3D(2), vAngle3D(3));
            strmFile.PutLine_t(strLine);
          }
          // string trans
          if( pepProperty->ep_eptType == CEntityProperty::EPT_STRINGTRANS) {
            CTString strString = FixQuotes(ENTITYPROPERTY( &en, pepProperty->ep_slOffset, CTString));
            strLine.PrintF("    \"%s\" = string(\"%s\");", pepProperty->ep_strName, strString);
            strmFile.PutLine_t(strLine);
          }          
          // flags
          if( pepProperty->ep_eptType == CEntityProperty::EPT_FLAGS) {
            ULONG ulValue = ENTITYPROPERTY( &en, pepProperty->ep_slOffset, ULONG);
            strLine.PrintF("    \"%s\" = long(%d);", pepProperty->ep_strName, ulValue);
            strmFile.PutLine_t(strLine);
          }
          // EPT_FLOATAABBOX3D - bounding box
          if( pepProperty->ep_eptType == CEntityProperty::EPT_FLOATAABBOX3D) {
            // get value for bounding box
            FLOATaabbox3D bboxOld = ENTITYPROPERTY( &en, pepProperty->ep_slOffset, FLOATaabbox3D);
            FLOAT3D vMin = bboxOld.Min();
            FLOAT3D vMax = bboxOld.Max();
            strLine.PrintF("    \"%s\" = box(%f, %f, %f),(%f, %f, %f);", pepProperty->ep_strName, 
              vMin(1), vMin(2), vMin(3), vMax(1), vMax(2), vMax(3));
            strmFile.PutLine_t(strLine);
          }
        }
      }

      // if render type is brush
      if( (en.en_RenderType==CEntity::RT_BRUSH || en.en_RenderType==CEntity::RT_FIELDBRUSH) && en.en_pbrBrush!=NULL) {
        // add one "fake" property that will hint "invisible brush" status
        BOOL bInvisibleBrush = !IsBrushVisible(en);
        BOOL bEmptyBrush = IsBrushEmpty(en);
        strLine.PrintF("    \"Hint: Invisible brush\" = long(%d);", bInvisibleBrush);
        strmFile.PutLine_t(strLine);

        // ".amf" file name
        CTString strEntityID;
        strEntityID.PrintF("%d", en.en_ulID);
        CTFileName fnAmf;
        fnAmf.PrintF("%s_%s.amf", fnWorld.FileDir()+fnWorld.FileName(), strEntityID);
        BOOL bFieldBrush = en.en_RenderType==CEntity::RT_FIELDBRUSH;
        ExportEntityToAMF_t(this, en, fnAmf, bFieldBrush, bInvisibleBrush, bEmptyBrush);
      }

      // close entity attributes section
      strLine.PrintF("  }");
      strmFile.PutLine_t(strLine);
    }

    // close entity section
    strLine.PrintF("}");
    strmFile.PutLine_t(strLine);
    
    // "entity placement and names"
    CTFileName fnSml=fnWorld.FileDir()+fnWorld.FileName()+".sml";
    // open text file
    CTFileStream strmSmlFile;
    strmSmlFile.Create_t( fnSml, CTStream::CM_TEXT);
    // save needed smc's
    for(INDEX iSmc=0; iSmc<astrNeddedSmc.Count(); iSmc++)
    {
      strmSmlFile.PutLine_t(astrNeddedSmc[iSmc]);
    }

    AfxMessageBox(L"Entities exported!", MB_OK|MB_ICONINFORMATION);
  }
  catch (const char *err_str)
  {
    AfxMessageBox( CString(err_str));
  }
}
