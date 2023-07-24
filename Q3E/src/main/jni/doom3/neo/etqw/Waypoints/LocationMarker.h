// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_WAYPOINTS_LOCATION_MARKER_H__
#define __GAME_WAYPOINTS_LOCATION_MARKER_H__

class sdWayPoint;

struct locationInfo_t {
	idVec3						origin;
	const sdDeclLocStr*			locationName;
	const sdDeclLocStr*			commandMapName;
	float						minRange;
	float						maxRange;
	locationInfo_t*				nextInArea;
	qhandle_t					commandMapHandle;
	idStr						font;
	float						textScale;
	const idMaterial*			waypointMaterial;
	sdWayPoint*					wayPoint;
};

struct compassDirection_t {
	idVec2						dir;
	const sdDeclLocStr*			name;
};

class sdLocationMarker : public idEntity {
public:
	CLASS_PROTOTYPE( sdLocationMarker );

	class sdLocationCVarCallback : public idCVarCallback {
	public:
		virtual void OnChanged( void ) { sdLocationMarker::OnShowMarkersChanged(); }
	};

	class sdWayPointCVarCallback : public idCVarCallback {
	public:
		virtual void OnChanged( void ) { sdLocationMarker::OnShowWayPointsChanged(); }
	};
								sdLocationMarker( void );

	void						Spawn( void );

	static const int			MAX_LOCATIONS = 256;

	static locationInfo_t*		LocationForPosition( const idVec3& position );

	static void					GetLocationText( const idVec3& position, idWStr& text );

	static void					DebugDraw( const idVec3& position );

	static void					OnNewMapLoad( void );
	static void					OnMapStart( void );
	static void					OnMapClear( bool all );

	static void					ShowLocations( bool value );

private:
	static void					FreeCommandMapIcon( locationInfo_t& info );
	static void					CreateCommandMapIcon( locationInfo_t& info );
	static void					FreeWayPoint( locationInfo_t& info );
	static void					CreateWayPoint( locationInfo_t& info );
	static void					Clear( void );

	static void					OnShowMarkersChanged( void );
	static void					OnShowWayPointsChanged( void );

	static void					FreeCommandMapIcons( void );
	static void					CreateCommandMapIcons( void );

	static void					FreeWayPoints( void );
	static void					CreateWayPoints( void );

	static idStaticList< locationInfo_t, MAX_LOCATIONS >	s_locations;
	static idStaticList< locationInfo_t*, MAX_LOCATIONS >	s_exteriorLocations;
	static idStaticList< locationInfo_t*, MAX_LOCATIONS >	s_interiorLocations;
	static idList< int >									s_areaCollapse;
	static idList< locationInfo_t* >						s_areaLocations;
	static const sdDeclLocStr*								s_locationTextMissing;
	static const sdDeclLocStr*								s_locationTextRange;
	static sdLocationCVarCallback							s_callback;
	static sdWayPointCVarCallback							s_callback2;
	static compassDirection_t								s_compassDirections[ 8 ];
};

#endif // __GAME_WAYPOINTS_LOCATION_MARKER_H__
