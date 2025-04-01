/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#ifndef __RENDERWORLDLOCAL_H__
#define __RENDERWORLDLOCAL_H__

#include "idlib/containers/HashMap.h"
#include "idlib/containers/BitArray.h"

// assume any lightDef or entityDef index above this is an internal error
#define LUDICROUS_INDEX	65537		// (2 ** 16) + 1;


typedef struct portal_s {
	int						intoArea;		// area this portal leads to
	idWinding 				w;				// winding points have counter clockwise ordering seen this area
	idPlane					plane;			// view must be on the positive side of the plane to cross
	//struct portal_s *		next;			// next portal of the area
	struct doublePortal_s *	doublePortal;
} portal_t;


typedef struct doublePortal_s {
	struct portal_s			portals[2];
	int						blockingBits;	// PS_BLOCK_VIEW, PS_BLOCK_AIR, etc, set by doors that shut them off

	float					lossPlayer;		// grayman #3042 - amount of Player sound loss (in dB)

	// A portal will be considered closed if it is past the
	// fog-out point in a fog volume.  We only support a single
	// fog volume over each portal.
	idRenderLightLocal *	fogLight;
	struct doublePortal_s *	nextFoggedPortal;
	int						portalViewCount;		// For r_showPortals. Keep track whether the player's view flows through 
											// individual portals, not just whole visleafs.  -- SteveL #4162

	doublePortal_s() { // zero fill
		blockingBits = 0;
		lossPlayer = 0;
		fogLight = 0;
		nextFoggedPortal = 0;
		portalViewCount = 0;
	}
} doublePortal_t;


typedef struct portalArea_s {
	int				areaNum;
	int				connectedAreaNum[NUM_PORTAL_ATTRIBUTES];	// if two areas have matching connectedAreaNum, they are
									// not separated by a portal with the apropriate PS_BLOCK_* blockingBits
	int				areaViewCount;		// set by R_FindViewLightsAndEntities. Marks whether anything in this area has been drawn this frame for r_showPortals
	idScreenRect	areaScreenRect;

	idList<portal_t*> areaPortals;		// never changes after load

	// stgatilov #6296: indexes of entities/light present in this area
	// these arrays are designed for fastest area->entity/light iteration
	idList<int>		entityRefs;
	idList<int>		lightRefs;
	// xxxBackRefs[k] is a linked list node explaining how to remove xxxRefs[k]
	// this is needed for O(1) additions/removals, is not very fast, and should not be used for iteration
	idList<areaReference_t*> entityBackRefs;
	idList<areaReference_t*> lightBackRefs;

	idList<int>		forceShadowsBehindOpaqueEntityRefs;

	portalArea_s() { // zero fill
		areaNum = 0;
		memset(connectedAreaNum, 0, sizeof(connectedAreaNum));
		areaViewCount = 0;
		memset(&areaScreenRect, 0, sizeof(areaScreenRect));
	}
} portalArea_t;


static const int	CHILDREN_HAVE_MULTIPLE_AREAS = -2;
static const int	AREANUM_SOLID = -1;
typedef struct {
	idPlane			plane;
	int				children[2];		// negative numbers are (-1 - areaNumber), 0 = solid
	int				commonChildrenArea;	// if all children are either solid or a single area,
										// this is the area number, else CHILDREN_HAVE_MULTIPLE_AREAS
} areaNode_t;


//used when r_useInteractionTable = 2
struct InterTableHashFunction {
	ID_FORCE_INLINE int operator() (int idx) const {
		//note: f(x) = (A*x % P), where P = 2^31-1 is prime
		static const unsigned MOD = ((1U<<31) - 1);
		uint64_t prod = 0x04738F51ULL * (unsigned)idx;
		unsigned mod = (prod >> 31) + (prod & MOD);
		unsigned modm = mod - MOD;
		mod = mod < MOD ? mod : modm;
		return (int)mod;
	}
};

//this table stores all interactions ever generated and still actual
class idInteractionTable {
public:
	idInteractionTable();
	~idInteractionTable();
	void Init();
	void Shutdown();
	idInteraction *Find(idRenderWorldLocal *world, int lightIdx, int entityIdx) const;
	bool Add(idInteraction *interaction);
	bool Remove(idInteraction *interaction);
	idStr Stats() const;

private:
	int useInteractionTable = -1;
	//r_useInteractionTable = 1: Single Matrix  (light x entity)
	idInteraction** SM_matrix;
	//r_useInteractionTable = 2: Single Hash Table
	idHashMap<int, idInteraction*> SHT_table;
};

class LightQuerySystem;

class idRenderWorldLocal : public idRenderWorld {
public:
							idRenderWorldLocal();
	virtual					~idRenderWorldLocal() override;

	virtual	qhandle_t		AddEntityDef( const renderEntity_t *re ) override;
	virtual	void			UpdateEntityDef( qhandle_t entityHandle, const renderEntity_t *re ) override;
	virtual	void			FreeEntityDef( qhandle_t entityHandle ) override;
	virtual const renderEntity_t *GetRenderEntity( qhandle_t entityHandle ) const override;

	virtual	qhandle_t		AddLightDef( const renderLight_t *rlight ) override;
	virtual	void			UpdateLightDef( qhandle_t lightHandle, const renderLight_t *rlight ) override; 
	virtual	void			FreeLightDef( qhandle_t lightHandle ) override;
	virtual const renderLight_t *GetRenderLight( qhandle_t lightHandle ) const override;

	virtual bool			CheckAreaForPortalSky( int areaNum ) override;

	virtual	void			GenerateAllInteractions() override;
	virtual void			RegenerateWorld() override;

	virtual void			ProjectDecalOntoWorld( const idFixedWinding &winding, const idVec3 &projectionOrigin, const bool parallel, const float fadeDepth, const idMaterial *material, const int startTime ) override;
	virtual void			ProjectDecal( qhandle_t entityHandle, const idFixedWinding &winding, const idVec3 &projectionOrigin, const bool parallel, const float fadeDepth, const idMaterial *material, const int startTime ) override;
	virtual void			ProjectOverlay( qhandle_t entityHandle, const idPlane localTextureAxis[2], const idMaterial *material ) override;
	virtual void			RemoveDecals( qhandle_t entityHandle ) override;

	virtual void			SetRenderView( const renderView_t *renderView ) override;
	virtual	void			RenderScene( const renderView_t &renderView ) override;

	virtual	int				NumAreas( void ) const override;
	virtual int				GetAreaAtPoint( const idVec3 &point ) const override;
	virtual int				GetPointInArea( int areaNum, idVec3 &point ) const override;
	virtual int				FindAreasInBounds( const idBounds &bounds, int *areas, int maxAreas ) const override;
	virtual	int				NumPortalsInArea( int areaNum ) override;
	// grayman #3042 - set portal sound loss (in dB)
	virtual void			SetPortalPlayerLoss( qhandle_t portal, float loss ) override;

	virtual exitPortal_t	GetPortal( int areaNum, int portalNum ) override;

#if 0
	virtual	guiPoint_t		GuiTrace( qhandle_t entityHandle, const idVec3 start, const idVec3 end ) const override;
#endif
	virtual bool			ModelTrace( modelTrace_t &trace, qhandle_t entityHandle, const idVec3 &start, const idVec3 &end, const float radius ) const override;
	virtual bool			Trace( modelTrace_t &trace, const idVec3 &start, const idVec3 &end, const float radius, bool skipDynamic = true, bool skipPlayer = false ) const override;
	virtual bool			FastWorldTrace( modelTrace_t &trace, const idVec3 &start, const idVec3 &end ) const override;
	virtual bool			MaterialTrace( const idVec3 &p, const idMaterial *mat, idStr &matName ) const override;
	virtual bool			TraceAll( modelTrace_t &trace, const idVec3 &start, const idVec3 &end, bool fastWorld = false, float radius = 0.0f, TraceFilterFunc filterCallback = nullptr, void *context = nullptr ) const override;

	// stgatilov #6546: querying light value at various points in space
	virtual lightQuery_t	LightAtPointQuery_AddQuery( qhandle_t onEntity, const samplePointOnModel_t &point, const idList<qhandle_t> &ignoredEntities ) override;
	virtual bool			LightAtPointQuery_CheckResult( lightQuery_t query, idVec3 &outputValue, idVec3& outputPosition ) const override;
	virtual void			LightAtPointQuery_Forget( lightQuery_t query ) override;

	virtual void			DebugClearLines( int time ) override;
	virtual void			DebugLine( const idVec4 &color, const idVec3 &start, const idVec3 &end, const int lifetime = 0, const bool depthTest = false ) override;
	virtual void			DebugArrow( const idVec4 &color, const idVec3 &start, const idVec3 &end, int size, const int lifetime = 0 ) override;
	virtual void			DebugWinding( const idVec4 &color, const idWinding &w, const idVec3 &origin, const idMat3 &axis, const int lifetime = 0, const bool depthTest = false ) override;
	virtual void			DebugCircle( const idVec4 &color, const idVec3 &origin, const idVec3 &dir, const float radius, const int numSteps, const int lifetime = 0, const bool depthTest = false ) override;
	virtual void			DebugSphere( const idVec4 &color, const idSphere &sphere, const int lifetime = 0, bool depthTest = false ) override;
	virtual void			DebugBounds( const idVec4 &color, const idBounds &bounds, const idVec3 &org = vec3_origin, const int lifetime = 0 ) override;
	virtual void			DebugBox( const idVec4 &color, const idBox &box, const int lifetime = 0 ) override;
	virtual void			DebugFilledBox( const idVec4 &color, const idBox &box, const int lifetime = 0, const bool depthTest = false ) override;
	virtual void			DebugFrustum( const idVec4 &color, const idFrustum &frustum, const bool showFromOrigin = false, const int lifetime = 0 ) override;
	virtual void			DebugCone( const idVec4 &color, const idVec3 &apex, const idVec3 &dir, float radius1, float radius2, const int lifetime = 0 ) override;
	void					DebugScreenRect( const idVec4 &color, const idScreenRect &rect, const viewDef_t *viewDef, const int lifetime = 0 );
	virtual void			DebugAxis( const idVec3 &origin, const idMat3 &axis ) override;

	virtual void			DebugClearPolygons( int time ) override;
	virtual void			DebugPolygon( const idVec4 &color, const idWinding &winding, const int lifeTime = 0, const bool depthTest = false ) override;

	virtual void			DebugText( const char *text, const idVec3 &origin, float scale, const idVec4 &color, const idMat3 &viewAxis, const int align = 1, const int lifetime = 0, bool depthTest = false ) override;

	//-----------------------

	idStr					mapName;				// ie: maps/tim_dm2.proc, written to demoFile
	ID_TIME_T				mapTimeStamp;			// for fast reloads of the same level

	areaNode_t *			areaNodes;
	int						numAreaNodes;

	idList<portalArea_t> portalAreas;
	//int						numPortalAreas;
	int						connectedAreaNum;		// incremented every time a door portal state changes

	idList<doublePortal_t>	doublePortals;
	//int						numInterAreaPortals;

	idList<idRenderModel *>	localModels;

	idList<idRenderEntityLocal*>	entityDefs;
	idList<idRenderLightLocal*>		lightDefs;
	// stgatilov #6296: duplicate members of entityDefs/lightDefs in compact arrays for faster access
	idBitArrayDefault entityDefsInView;			// 1 when viewCount == tr.viewCount
	idList<idSphere> entityDefsBoundingSphere;	// computed by referenceBounds

	idBlockAlloc<areaReference_t, 1024> areaReferenceAllocator;
	idBlockAlloc<idInteraction, 256>	interactionAllocator;

	// all light / entity interactions are referenced here for fast lookup without
	// having to crawl the doubly linked lists.  EnntityDefs are sequential for better
	// cache access, because the table is accessed by light in idRenderWorldLocal::CreateLightDefInteractions()
	// Growing this table is time consuming, so we add a pad value to the number
	// of entityDefs and lightDefs
	idInteractionTable		interactionTable;


	bool					generateAllInteractionsCalled;

	LightQuerySystem *		lightQuerySystem;

	typedef idFlexList<int, 128> AreaList;

	//-----------------------
	// RenderWorld_load.cpp

	idRenderModel *			ParseModel( idLexer *src );
	idRenderModel *			ParseShadowModel( idLexer *src );
	void					SetupAreaRefs();
	void					ParseInterAreaPortals( idLexer *src );
	void					ParseNodes( idLexer *src );
	int						CommonChildrenArea_r( areaNode_t *node );
	void					FreeWorld();
	void					ClearWorld();
	void					FreeDefs();
	void					TouchWorldModels( void );
	void					AddWorldModelEntities();
	void					ClearPortalStates();
	virtual	bool			InitFromMap( const char *mapName ) override;

	//--------------------------
	// RenderWorld_portals.cpp

	idScreenRect			ScreenRectFromWinding( const idWinding *w, viewEntity_t *space );
	bool					PortalIsFoggedOut( const portal_t *p );
	void					FloodViewThroughArea_r( const idVec3 origin, int areaNum, const struct portalStack_s *ps );
	void					FlowViewThroughPortals( const idVec3 origin, int numPlanes, const idPlane *planes );
	struct FlowLightThroughPortalsContext;
	void					FloodLightThroughArea_r( FlowLightThroughPortalsContext &context, int areaNum, const struct portalStack_s *ps ) const;
	void					FlowLightThroughPortals( const idRenderLightLocal *light, const AreaList &startingAreaIds, AreaList *areaIds, lightPortalFlow_t *portalFlow ) const;
	bool					CullEntityByPortals( const idRenderEntityLocal *entity, const struct portalStack_s *ps );
	void					AddAreaEntityRefs( int areaNum, const struct portalStack_s *ps );
	bool					CullLightByPortals( const idRenderLightLocal *light, const struct portalStack_s *ps );
	void					AddAreaLightRefs( int areaNum, const struct portalStack_s *ps );
	void					AddAreaRefs( int areaNum, const struct portalStack_s *ps );
	void					BuildConnectedAreas_r( int areaNum );
	void					BuildConnectedAreas( void );
	void					FindViewLightsAndEntities( void );

	struct FloodShadowFrustumContext;
	bool					FloodShadowFrustumThroughArea_r( FloodShadowFrustumContext &context, const idBounds &bounds ) const;
	void					FlowShadowFrustumThroughPortals( idScreenRect &scissorRect, const idFrustum &frustum, const int *startAreas, int startAreasNum ) const;

	virtual int				NumPortals( void ) const override;
	virtual qhandle_t		FindPortal( const idBounds &b ) const override;
	static bool				DoesVisportalContactBox( const idWinding &visportalWinding, const idBounds &box );	//stgatilov #5354
	virtual void			SetPortalState( qhandle_t portal, int blockingBits ) override;
	virtual int				GetPortalState( qhandle_t portal ) override;
	virtual idPlane			GetPortalPlane( qhandle_t portal ) override;	//stgatilov #5462

	virtual bool			AreasAreConnected( int areaNum1, int areaNum2, portalConnection_t connection ) override;
	void					FloodConnectedAreas( portalArea_t *area, int portalAttributeIndex );
	const idScreenRect &	GetAreaScreenRect( int areaNum ) const { return portalAreas[areaNum].areaScreenRect; }
	void					ShowPortals();

	//--------------------------
	// RenderWorld_demo.cpp

	virtual void			StartWritingDemo( idDemoFile *demo ) override;
	virtual void			StopWritingDemo() override;
	virtual bool			ProcessDemoCommand( idDemoFile *readDemo, renderView_t *demoRenderView, int *demoTimeOffset ) override;

	void					WriteLoadMap();
	void					WriteRenderView( const renderView_t &renderView );
	void					WriteVisibleDefs( const viewDef_t *viewDef );
	void					WriteFreeLight( qhandle_t handle );
	void					WriteFreeEntity( qhandle_t handle );
	void					WriteRenderLight( qhandle_t handle, const renderLight_t *light );
	void					WriteRenderEntity( qhandle_t handle, const renderEntity_t *ent );
	void					ReadRenderEntity();
	void					ReadRenderLight();
	

	//--------------------------
	// RenderWorld.cpp

	int						AllocateEntityDefHandle();
	void					AddEntityRefToArea( idRenderEntityLocal *def, portalArea_t *area );
	void					AddLightRefToArea( idRenderLightLocal *light, portalArea_t *area );

	void					RecurseProcBSP_r( modelTrace_t *results, int *areas, int *numAreas, int maxAreas, int parentNodeNum, int nodeNum, float p1f, float p2f, const idVec3 &p1, const idVec3 &p2 ) const;
	struct LineIntersectionPoint {
		float param;
		int areaBefore;
	};
	void					RecurseFullLineIntersectionBSP_r( idList<LineIntersectionPoint> &result, int nodeNum, int parentNodeNum, float paramMin, float paramMax, const idVec3 &origin, const idVec3 &dir ) const;

	void					BoundsInAreas_r( int nodeNum, const idBounds &bounds, int *areas, int *numAreas, int maxAreas ) const;

	float					DrawTextLength( const char *text, float scale, int len = 0 );

	void					PutAllInteractionsIntoTable( bool resetTable );
	void					FreeInteractions();

	
	struct FrustumCoveredContext;
	void					GetFrustumCoveredAreas_r(FrustumCoveredContext &context, int nodeNum) const;
	void					GetFrustumCoveredAreas(idRenderEntityLocal* def, AreaList &areaIds) const;
	void					GetFrustumCoveredAreas(idRenderLightLocal* light, AreaList &areaIds) const;
	void					GetAreasOfLightFrustumEnterFaces(idRenderLightLocal* light, AreaList &areaIds) const;

	void					AddLightRefToArea( idRenderLightLocal *def, int areaIdx );
	void					AddLightRefsToFrustumCoveredAreas( idRenderLightLocal *def );
	void					AddLightRefsByPortalFlow( idRenderLightLocal *def, const AreaList &startingAreas );

	void					AddEntityToAreas(idRenderEntityLocal* def);
	void					AddLightToAreas(idRenderLightLocal* def);

	//-------------------------------
	// tr_light.c
	void					CreateLightDefInteractions( idRenderLightLocal *ldef );
	void					CreateNewLightDefInteraction( idRenderLightLocal *ldef, idRenderEntityLocal *edef );
	bool					CullInteractionByLightFlow( idRenderLightLocal *ldef, idRenderEntityLocal *edef ) const;
};


//stgatilov: some informative labels suitable for tracing/logging/debugging
//ideally, it should match natvis definitions...

ID_FORCE_INLINE const char *GetTraceLabel(const renderEntity_t &rEnt) {
	if ( rEnt.entityNum != 0 ) {
		return gameLocal.entities[rEnt.entityNum]->name.c_str();
	} else if ( rEnt.hModel ) {
		return rEnt.hModel->Name();
	} else {
		return "[unknown]";
	}
}

ID_FORCE_INLINE const char *GetTraceLabel(const renderLight_t &rLight) {
	if ( rLight.entityNum != 0 ) {
		return gameLocal.entities[rLight.entityNum]->name.c_str();
	} else {
		return "[unknown]";
	}
}

#endif /* !__RENDERWORLDLOCAL_H__ */
