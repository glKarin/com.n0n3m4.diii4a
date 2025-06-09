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

// WorldEditorDoc.h : interface of the CWorldEditorDoc class
//
/////////////////////////////////////////////////////////////////////////////
#ifndef WORLDEDITORDOC_H
#define WORLDEDITORDOC_H 1

#define SNAP_DOUBLE_CM 0.015625
#define SNAP_FLOAT_CM 0.015625f
#define SNAP_FLOAT_12 0.125f
#define SNAP_FLOAT_25 0.25f
#define SNAP_ANGLE_GRID (AngleDeg(2.5f))

#define POLYGON_MODE 1
#define SECTOR_MODE 2
#define ENTITY_MODE 3
#define VERTEX_MODE 4
#define TERRAIN_MODE 5
#define CSG_MODE 6

enum ESelectionType
{
  ST_NONE = 0,
  ST_VERTEX,
  ST_ENTITY,
  ST_VOLUME,
  ST_SECTOR,
  ST_POLYGON,
};
  
enum CSGType 
{
  CSG_ILLEGAL = 0,
  CSG_ADD,
  CSG_ADD_REVERSE,
  CSG_REMOVE,
  CSG_REMOVE_REVERSE,
  CSG_SPLIT_SECTORS,
  CSG_JOIN_SECTORS,
  CSG_SPLIT_POLYGONS,
  CSG_JOIN_POLYGONS,
  CSG_JOIN_POLYGONS_KEEP_TEXTURES,
  CSG_JOIN_ALL_POLYGONS,
  CSG_JOIN_ALL_POLYGONS_KEEP_TEXTURES,
  CSG_JOIN_LAYERS,
  CSG_ADD_ENTITIES,
};

class CUndo
{
public:  
  CListNode m_lnListNode;
  CTFileName m_fnmUndoFile;     // name of temporary file used for undo/redo
  /* Constructor. */
  CUndo(void);    // throw char * 
  /* Destructor. */
  ~CUndo(void);
};

class CWorldEditorDoc : public CDocument
{
protected: // create from serialization only
	CWorldEditorDoc();
	DECLARE_DYNCREATE(CWorldEditorDoc)

// Attributes
public:
  CDynamicContainer<CTerrainUndo> m_dcTerrainUndo;
  INDEX m_iCurrentTerrainUndo;
  BOOL m_bAskedToCheckOut;
  SLONG m_slDisplaceTexTime;
  INDEX m_iMirror;
  INDEX m_iTexture;
  CTerrain *m_ptrSelectedTerrain;
  CTextureObject m_toBackdropUp;
  CTextureObject m_toBackdropFt;
  CTextureObject m_toBackdropRt;
  CObject3D m_o3dBackdropObject;
  FLOAT3D m_vCreateBoxVertice0; // vertices that were last used in box creation
  FLOAT3D m_vCreateBoxVertice1;
  FLOAT3D m_avVolumeBoxVertice[ 8]; // volume box vertices 
  // number of vertices used for creating last primitive base polygon (-1 for recreate base)
  BOOL m_bPrimitiveCreatedFirstTime;
  INDEX m_ctLastPrimitiveVertices;
  DOUBLE m_fLastPrimitiveWidth;
  DOUBLE m_fLastPrimitiveLenght;
  BOOL m_bLastIfOuter;
  TriangularisationType m_ttLastTriangularisationType;
  // index of vertice on volume box that user is currently dragging
  INDEX m_iVolumeBoxDragVertice;
  // starting position of volume box vertice before drag started
  FLOAT3D m_vVolumeBoxStartDragVertice;

  BOOL m_bPreLastUsedPrimitiveMode;
  BOOL m_bLastUsedPrimitiveMode;
  CTFileName m_fnLastDroppedTemplate;
  enum CSGType m_csgtPreLastUsedCSGOperation;
  enum CSGType m_csgtLastUsedCSGOperation;
  // list head for undo
  CListHead m_lhUndo;
  // list head for redo
  CListHead m_lhRedo;
  BOOL m_bAutoSnap;
  BOOL m_bOrientationIcons;
  BOOL m_bPrimitiveMode;
  BOOL m_bBrowseEntitiesMode;
  CPlacement3D m_plGrid;
  CPlacement3D m_plSecondLayer;
  CPlacement3D m_plDeltaPlacement;
  CPlacement3D m_plLastPlacement;
  CWorld m_woWorld;
  CWorld *m_pwoSecondLayer;   // world for holding second layer
  CEntity *m_penPrimitive;
  // index for holding pre CSG mode
  INDEX m_iPreCSGMode;
  INDEX m_iMode;

  // volume selection
  CDynamicContainer<CEntity> m_cenEntitiesSelectedByVolume;
  INDEX m_iSelectedEntityInVolume;
  // selections
  CEntitySelection m_selEntitySelection;
  CBrushSectorSelection m_selSectorSelection;
  CBrushVertexSelection m_selVertexSelection;
  CStaticArray<DOUBLE3D> m_avStartDragVertices;
  CBrushPolygonSelection m_selPolygonSelection;
  CStaticArray<CPlacement3D> m_aSelectedEntityPlacements;
  CBrushPolygon *m_pbpoLastCentered;

  CChangeableRT m_chSelections;
  CChangeableRT m_chDocument;

  BOOL m_bWasEverSaved;     // set if saved at least once (if not - play from testgame.wld)
  BOOL m_bReadOnly;     // opened file was read-only
  CWorldEditorView *m_pCutLineView;
  FLOAT3D m_vCutLineStart;
  FLOAT3D m_vCutLineEnd;
  FLOAT3D m_vControlLineDragStart;

// Operations
public:
  // Function corects coordinates of vertices that represent box because given vertice is
  // moved and box has invalid geometry
  void CorrectBox(INDEX iMovedVtx, FLOAT3D vNewPosition);
  // Selects entity with given index inside volume
  void SelectGivenEntity( INDEX iEntityToSelect);
  // adds all world's entities that fall inside volume into volume selection
  void SelectEntitiesByVolumeBox(void);
  void ConvertObject3DToBrush(CObject3D &ob, BOOL bApplyProjectedMapping=FALSE,
    INDEX iMipBrush=0, FLOAT fMipFactor=1E6f, BOOL bApplyDefaultPolygonProperties=TRUE);
  // calculate base of primitive (discard vertex draging)
  void CreateConusPrimitive(void);
  void CreateTorusPrimitive(void);
  void CreateStaircasesPrimitive(void);
  void CreateSpherePrimitive(void);
  void CreateTerrainPrimitive(void);
  void CreateTerrainObject3D( CImageInfo *piiDisplace, INDEX iSlicesX, INDEX iSlicesZ, INDEX iMip);
  void CreatePrimitive(void);
  // apply auto colorize function
  void ApplyAutoColorize(void);
  // refresh primitive page
  void RefreshPrimitivePage(void);
  // refresh primitive page
  void RefreshCurrentInfoPage(void);
  // apply now current primitive settings (used in load primitive settings and history)
  void ApplyCurrentPrimitiveSettings(void);
  BOOL IsEntityCSGEnabled(void);
  // start creating primitive
  void StartPrimitiveCSG( CPlacement3D plPrimitive, BOOL bResetAngles = TRUE);
  // start CSG with world template
  void StartTemplateCSG( CPlacement3D plPrimitive, const CTFileName &fnWorld,
    BOOL bResetAngles = TRUE);
  // inform all views that CSG is started
  void AtStartCSG(void);
  // inform all views that CSG is finished
  void AtStopCSG(void);
  // apply current CSG operation
  void PreApplyCSG(enum CSGType CSGType);
  void ApplyCSG(enum CSGType CSGType);
  // cancel current CSG operation
  void CancelCSG(void);
  // clean up after doing a CSG
  void StopCSG(void);
  // does "snap to grid" for given float
  void SnapFloat( FLOAT &fDest, FLOAT fStep = SNAP_FLOAT_GRID);
  // does "snap to grid" for given angle
  void SnapAngle( ANGLE &angDest, ANGLE fStep = SNAP_ANGLE_GRID);
  // does "snap to grid" for given placement
  void SnapToGrid( CPlacement3D &plPlacement, FLOAT fSnapValue);
  // does "snap to grid" for primitive values
  void SnapPrimitiveValuesToGrid(void);
  // saves curent state of the world as tail of give undo/redo list
  void SaveWorldIntoUndoRedoList( CListHead &lhList);
  // restores last operation from given undo/redo object
  void LoadWorldFromUndoRedoList( CUndo *pUndoRedo);
  // remembers last operation into undo buffer
  void RememberUndo(void);
  // undoes last operation
  void Undo(void);
  // redoes last undoed operation
  void Redo(void);
  // retrieves editing mode
  inline INDEX GetEditingMode() { return m_iMode;};
  // selects all entities in volume
  void OnSelectAllInVolume(void);
  // sets editing mode
  void SetEditingMode( INDEX iNewMode);
  // paste given texture over polygon selection
  void PasteTextureOverSelection_t( CTFileName fnFileName);
  // deselect all selected members in current selection mode
  void DeselectAll(void);
  // Sets message about current mode and selected members
  void SetStatusLineModeInfoMessage( void);
  // creates texture from picture and initializes texture object
  void SetupBackdropTextureObject( CTFileName fnPicture, CTextureObject &to);
  // saves thumbnail
  void SaveThumbnail(void);
  // reset primitive settings
  void ResetPrimitive(void);
  void InsertPrimitiveVertex(INDEX iEdge, FLOAT3D vVertexToInsert);
  void DeletePrimitiveVertex(INDEX iVtxToDelete);
  void ApplyMirrorAndStretch(INDEX iMirror, FLOAT fStretch);
  void SetActiveTextureLayer(INDEX iLayer);
  
  void ClearSelections(ESelectionType stExcept=ST_NONE);
  BOOL IsCloneUpdatingAllowed(void);

  void SetCutMode( CWorldEditorView *pwedView);
  void ApplyCut( void);

  // show/hide functoins
  void OnHideSelectedEntities(void);
	void OnHideUnselectedEntities(void);
	void OnShowAllEntities(void);
  void OnHideSelectedSectors(void);
	void OnHideUnselectedSectors(void);
	void OnShowAllSectors(void);
  void SetModifiedFlag( BOOL bModified = TRUE);

  void ReloadWorld(void);
  BOOL IsReadOnly(void);
  BOOL IsBrushUpdatingAllowed(void);

  void OnIdle(void);
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWorldEditorDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	virtual BOOL OnSaveDocument(LPCTSTR lpszPathName);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CWorldEditorDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
public:
	//{{AFX_MSG(CWorldEditorDoc)
	afx_msg void OnCsgSplitSectors();
	afx_msg void OnUpdateCsgSplitSectors(CCmdUI* pCmdUI);
	afx_msg void OnCsgCancel();
	afx_msg void OnShowOrientation();
	afx_msg void OnUpdateShowOrientation(CCmdUI* pCmdUI);
	afx_msg void OnEditUndo();
	afx_msg void OnEditRedo();
	afx_msg void OnUpdateEditUndo(CCmdUI* pCmdUI);
	afx_msg void OnUpdateEditRedo(CCmdUI* pCmdUI);
	afx_msg void OnWorldSettings();
	afx_msg void OnCsgJoinSectors();
	afx_msg void OnUpdateCsgJoinSectors(CCmdUI* pCmdUI);
	afx_msg void OnAutoSnap();
	afx_msg void OnCsgAdd();
	afx_msg void OnUpdateCsgAdd(CCmdUI* pCmdUI);
	afx_msg void OnCsgRemove();
	afx_msg void OnUpdateCsgRemove(CCmdUI* pCmdUI);
	afx_msg void OnCsgSplitPolygons();
	afx_msg void OnUpdateCsgSplitPolygons(CCmdUI* pCmdUI);
	afx_msg void OnCsgJoinPolygons();
	afx_msg void OnUpdateCsgJoinPolygons(CCmdUI* pCmdUI);
	afx_msg void OnCalculateShadows();
	afx_msg void OnBrowseEntitiesMode();
	afx_msg void OnUpdateBrowseEntitiesMode(CCmdUI* pCmdUI);
	afx_msg void OnPreviousSelectedEntity();
	afx_msg void OnUpdatePreviousSelectedEntity(CCmdUI* pCmdUI);
	afx_msg void OnNextSelectedEntity();
	afx_msg void OnUpdateNextSelectedEntity(CCmdUI* pCmdUI);
	afx_msg void OnJoinLayers();
	afx_msg void OnUpdateAutoSnap(CCmdUI* pCmdUI);
	afx_msg void OnSelectByClass();
	afx_msg void OnUpdateSelectByClass(CCmdUI* pCmdUI);
	afx_msg void OnCsgJoinAllPolygons();
	afx_msg void OnUpdateCsgJoinAllPolygons(CCmdUI* pCmdUI);
	afx_msg void OnTexture1();
	afx_msg void OnUpdateTexture1(CCmdUI* pCmdUI);
	afx_msg void OnTexture2();
	afx_msg void OnUpdateTexture2(CCmdUI* pCmdUI);
	afx_msg void OnTexture3();
	afx_msg void OnUpdateTexture3(CCmdUI* pCmdUI);
	afx_msg void OnTextureMode1();
	afx_msg void OnTextureMode2();
	afx_msg void OnTextureMode3();
	afx_msg void OnSaveThumbnail();
	afx_msg void OnUpdateLinks();
	afx_msg void OnSnapshot();
	afx_msg void OnMirrorAndStretch();
	afx_msg void OnFlipLayer();
	afx_msg void OnUpdateFlipLayer(CCmdUI* pCmdUI);
	afx_msg void OnFilterSelection();
	afx_msg void OnUpdateClones();
	afx_msg void OnUpdateUpdateClones(CCmdUI* pCmdUI);
	afx_msg void OnSelectByClassAll();
	afx_msg void OnHideSelected();
	afx_msg void OnUpdateHideSelected(CCmdUI* pCmdUI);
	afx_msg void OnHideUnselected();
	afx_msg void OnShowAll();
	afx_msg void OnCheckEdit();
	afx_msg void OnCheckAdd();
        afx_msg void OnCheckDelete();
	afx_msg void OnUpdateCheckEdit(CCmdUI* pCmdUI);
	afx_msg void OnUpdateCheckAdd(CCmdUI* pCmdUI);
        afx_msg void OnUpdateCheckDelete(CCmdUI* pCmdUI);
	afx_msg void OnUpdateBrushes();
	afx_msg void OnSelectByClassImportant();
	afx_msg void OnInsert3dObject();
	afx_msg void OnExport3dObject();
	afx_msg void OnUpdateExport3dObject(CCmdUI* pCmdUI);
	afx_msg void OnCrossroadForN();
	afx_msg void OnPopupVtxAllign();
	afx_msg void OnPopupVtxFilter();
	afx_msg void OnPopupVtxNumeric();
	afx_msg void OnTextureMode4();
	afx_msg void OnTextureMode5();
	afx_msg void OnTextureMode6();
	afx_msg void OnTextureMode7();
	afx_msg void OnTextureMode8();
	afx_msg void OnTextureMode9();
	afx_msg void OnTextureMode10();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
  afx_msg void OnExportPlacements();
  afx_msg void OnExportEntities();
};

/////////////////////////////////////////////////////////////////////////////
#endif // WORLDEDITORDOC_H
