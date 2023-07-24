// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_VEHICLES_ROUTECONSTRAINT_H__
#define __GAME_VEHICLES_ROUTECONSTRAINT_H__

#include "../misc/RenderEntityBundle.h"

class sdRouteConstraintMarker;
class sdRouteConstraintTracker;

class sdRoutePoint {
public:
	static const int										MAX_NODES = 8;
	typedef idStaticList< const sdRoutePoint*, MAX_NODES >	nodeList_t;
	typedef idStaticList< sdRenderEntityBundle, sdRoutePoint::MAX_NODES > renderList_t;

	void							Init( sdRouteConstraintMarker* marker );
	void							Clear( void );
	void							AddParent( sdRoutePoint* point );
	void							AddChild( sdRoutePoint* point );
	void							SetAngles( const idAngles& value ) { angles = value; }

	static void						Link( sdRoutePoint* parent, sdRoutePoint* child );
	static sdRoutePoint*			Allocate( void ) { sdRoutePoint* point = s_allocator.Alloc(); point->Clear(); return point; }
	static void						Free( sdRoutePoint* point ) { s_allocator.Free( point ); }

	const idStrList&				GetTargetNames( void ) const { return targetNames; }
	const char*						GetName( void ) const { return name.c_str(); }
	const idVec3&					GetOrigin( void ) const { return origin; }
	const idAngles&					GetAngles( void ) const { return angles; }
	const nodeList_t&				GetChildren( void ) const { return children; }
	const nodeList_t&				GetParents( void ) const { return parents; }
	bool							GetAllowAirDrop( void ) const { return allowAirDrop; }

	int								OnChain( const sdRoutePoint* chainEnd ) const;
	const sdRoutePoint*				FindCommonParent( const sdRoutePoint* other ) const;
	const sdRoutePoint*				FindCommonChild( const sdRoutePoint* other ) const;

private:
	const sdRoutePoint*				FindCommonParent( const sdRoutePoint* other, const sdRoutePoint* node ) const;
	const sdRoutePoint*				FindCommonChild( const sdRoutePoint* other, const sdRoutePoint* node ) const;

private:
	idVec3							origin;
	idAngles						angles;
	nodeList_t						parents;
	nodeList_t						children;
	idStr							name;
	idStrList						targetNames;
	bool							allowAirDrop;
	qhandle_t						maskHandle;

	static idBlockAlloc< sdRoutePoint, 32 >			s_allocator;
};

class sdRouteConstraintController : public idEntity {
	CLASS_PROTOTYPE( sdRouteConstraintController );
public:
									sdRouteConstraintController( void );
									~sdRouteConstraintController( void );

	void							Spawn( void );
	void							Display( sdRouteConstraintTracker* tracker );
	void							SetRenderPoint( sdRouteConstraintTracker* tracker, sdRoutePoint* newPoint );
	void							Update( sdRouteConstraintTracker* tracker );
	void							AddPoint( sdRouteConstraintMarker* marker );

	const sdRoutePoint*				GetStartPoint( void ) const { return startPoint; }
	const sdRoutePoint*				GetEndPoint( void ) const { return endPoint; }

private:
	void							Event_Link( void ); // Gordon: it is wii day after all ;)

	void							CheckForLoops( idList< const sdRoutePoint* >& checkPoints, const sdRoutePoint* point );
	sdRoutePoint*					FindPoint( const char* name );
	sdRoutePoint*					FindPoint( const idVec3& origin );


private:
	idList< sdRoutePoint* >			points;
	sdRoutePoint*					startPoint;
	sdRoutePoint*					endPoint;
	bool							linked;
	qhandle_t						maskHandle;
	int								warningPointDeviance;
	int								maxPointDeviance;
	idRenderModel*					directionalModel;
	const idDeclSkin*				directionalModelSkin;
	idVec3							directionalModelColor;
};

class sdRouteConstraintMarker : public idEntity {
	CLASS_PROTOTYPE( sdRouteConstraintMarker );
public:
	virtual void						PostMapSpawn( void );

private:
};

class sdRouteConstraintTracker {
public:
												sdRouteConstraintTracker( void );
	void										Init( idEntity* _entity );
	void										SetTrackerEntity( idEntity* entity );
	
	bool										IsValid( void ) const { return controller.IsValid(); }
	void										Display( void );
	void										Hide( void );
	void										Update( void );
	void										Reset( void ) { bestPoint = currentPoint; positionWarning = false; maskWarning = false; kickPlayer = false; }

	idEntity*									GetEntity( void ) const { return entity; }
	const sdRoutePoint*							GetCurrentPoint( void ) const { return currentPoint; }
	const sdRoutePoint*							GetBestPoint( void ) const { return bestPoint; }
	const sdRoutePoint*							GetBestReserve( void ) const { return bestPointReserve; }
	const sdRoutePoint*							GetRenderPoint( void ) const { return renderPoint; }
	bool										GetPositionWarning( void ) const { return positionWarning; }
	bool										GetMaskWarning( void ) const { return maskWarning; }
	bool										GetKickPlayer( void ) const { return kickPlayer; }
	float										GetKickDistance( void ) const { return kickDistance; }
	sdRoutePoint::renderList_t&					GetRenderList( void ) { return markers; }
	idEntity*									GetTrackerEntity( void ) const { return controller; }

	void										SetCurrentPoint( const sdRoutePoint* point ) { currentPoint = point; }
	void										SetBestPoint( const sdRoutePoint* point ) { SetReserve( bestPoint ); bestPoint = point; }
	void										SetRenderPoint( const sdRoutePoint* point ) { renderPoint = point; }

	void										SetPositionWarning( bool value ) { positionWarning = value; }
	void										SetMaskWarning( bool value ) { maskWarning = value; }
	void										SetKickPlayer( bool value ) { kickPlayer = value; }
	void										SetKickDistance( int value ) { kickDistance = value; }

	void										GetDropLocation( idVec3& position, idAngles& angles ) const;

private:
	void										SetReserve( const sdRoutePoint* point );

	const sdRoutePoint*							currentPoint;
	const sdRoutePoint*							bestPoint;
	const sdRoutePoint*							bestPointReserve;
	const sdRoutePoint*							renderPoint;
	idEntity*									entity;
	idEntityPtr< sdRouteConstraintController >	controller;
	bool										positionWarning;
	bool										maskWarning;
	bool										kickPlayer;
	int											kickDistance;
	sdRoutePoint::renderList_t					markers;
};

#endif // __GAME_VEHICLES_ROUTECONSTRAINT_H__
