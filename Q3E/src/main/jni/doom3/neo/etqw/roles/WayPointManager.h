// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __GAME_ROLES_WAYPOINTMANAGER_H__
#define __GAME_ROLES_WAYPOINTMANAGER_H__

#define WAYPOINT_MAX_WAYPOINTS		64

class sdWorldToScreenConverter;

class sdWayPoint {
public:
	typedef idLinkList< sdWayPoint > link_t;
	typedef	idStaticList< sdWayPoint*, WAYPOINT_MAX_WAYPOINTS >	wayPointArray_t;

	enum waypointFlags_t {
		WF_ALWAYSACTIVE				= BITT< 0 >::VALUE,
		WF_FIXEDSIZE				= BITT< 1 >::VALUE,
		WF_RENDERMODEL				= BITT< 2 >::VALUE,
		WF_FIXEDPOSITION			= BITT< 3 >::VALUE,
		WF_ACTIVE					= BITT< 4 >::VALUE,
		WF_HIGHLIGHTACTIVE			= BITT< 5 >::VALUE,
		WF_BRACKETED				= BITT< 6 >::VALUE,
		WF_CHECKLOS					= BITT< 7 >::VALUE,
		WF_TEAMCOLOR				= BITT< 8 >::VALUE,
	};

	static const int				ACTIVATE_TIME = 250;

									sdWayPoint( void );

	link_t&							GetActiveNode( void ) { return _activeNode; }
	const link_t&					GetActiveNode( void ) const { return _activeNode; }

	const wchar_t*					GetText( void ) const { return _text.c_str(); }

	void							Clear( void );
	void							Init( void );

	void							Update( void );

	const idBounds&					GetBounds( void ) const;
	const idVec3					GetPosition( void ) const;
	const idVec3					GetBracketPosition( void ) const;
	const idMat3&					GetOrientation( void ) const;
	float							GetRange( void ) const { return _range; }
	float							GetMinRange( void ) const { return _minRange; }
	float							GetActiveFraction( void ) const;
	float							GetHighLightActiveFraction( void ) const;
	const idMaterial*				GetMaterial( void ) const { return _material; }
	const idMaterial*				GetOffScreenMaterial( void ) const { return _offScreenMaterial; }
	int								GetFlags() const { return _flags; }
	int								GetFlashEndTime() const { return _flashEndTime; }

	int								Selected() const { return _selected; }
	void							SetSelected( int time ) { _selected = time; }
	
	void							MakeActive( void );
	void							MakeInActive( void );

	void							MakeHighlightActive( void );
	void							MakeHighlightInActive( void );

	void							SetText( const wchar_t* text ) { _text = text; }
	void							SetBounds( const idBounds& bounds );
	void							SetOrigin( const idVec3& org );
	void							SetIconOffset( const idVec3& offset );
	void							SetOwner( idEntity* owner );
	void							SetAlwaysActive( void ) { _flags |= WF_ALWAYSACTIVE; }
	void							SetMinRange( float value ) { _minRange = value; }
	void							SetTeamColored( void ) { _flags |= WF_TEAMCOLOR; }
	void							SetFlashEndTime( int time ) { _flashEndTime = time; }

	idEntity*						GetOwner( void ) const { return _owner; }

	void							Event_Free( void );
	void							Event_SetBounds( const idVec3& mins, const idVec3& maxs );
	void							Event_SetOrigin( const idVec3& org );
	void							UseRenderModel( void );
	void							Event_SetOwner( idEntity* owner );
	void							SetRange( float range );
	void							SetMaterial( const idMaterial* material );
	void							SetOffScreenMaterial( const idMaterial* material );
	void							SetBracketed( bool bracketed );

	int								IsActive( void ) const { return _flags & WF_ACTIVE; }
	int								IsHighlightActive( void ) const { return _flags & WF_HIGHLIGHTACTIVE; }
	bool							IsValid( void ) const { return _owner.IsValid() || ( ( _flags & WF_FIXEDPOSITION ) != 0 ); }
	int								IsAlwaysActive( void ) const { return _flags & WF_ALWAYSACTIVE; }
	int								Bracketed( void ) const { return _flags & WF_BRACKETED; }

	bool							ShouldCheckLineOfSight( void ) const { return _shouldCheckLineOfSight; }
	void							SetCheckLineOfSight( bool value ) { _shouldCheckLineOfSight = _flags & WF_CHECKLOS ? value : false; }
	bool							IsVisible( void ) const { return _isVisible; }
	void							SetVisible( bool vis ) { _isVisible = vis; }
	void							SetFlag( waypointFlags_t flag ) { _flags |= flag; }

private:
	link_t							_activeNode;

	idWStr							_text;
	idBounds						_fixedBounds;
	idVec3							_fixedLocation;
	idVec3							_iconOffset;

	idEntityPtr< idEntity >			_owner;

	const idMaterial*				_material;
	const idMaterial*				_offScreenMaterial;

	int								_activeTime;
	int								_highlightActiveTime;
	int								_flags;
	float							_range;
	float							_minRange;

	bool							_shouldCheckLineOfSight;
	bool							_isVisible;
	int								_selected;
	int								_flashEndTime;
};

class sdWayPointManagerLocal {
public:
									sdWayPointManagerLocal( void );
									~sdWayPointManagerLocal( void );

	sdWayPoint*						AllocWayPoint( void );
	void							FreeWayPoint( sdWayPoint* wayPoint );

	void							Think( void );
	void							UpdateActive( void );
	void							UpdateWayPoints( void );
	void							Init( void );

	void							ShowWayPoints( bool value ) { _forceShowWayPoints = value; }
	bool							GetShowWayPoints( void ) { return _forceShowWayPoints; }

	void							AddWayPoint( sdWayPoint* way );
	void							RemoveWayPoint( sdWayPoint* way );

	sdWayPoint*						GetFirstActiveWayPoint() { return _activeWayPoints.Next(); }
	const sdWayPoint*				GetFirstActiveWayPoint() const { return _activeWayPoints.Next(); }

private:
	int								_lastActiveUpdate;
	int								_lastUpdatedWayPointIndex;
	bool							_forceShowWayPoints;
	sdWayPoint::link_t				_activeWayPoints;
	sdWayPoint::wayPointArray_t		_wayPoints;
	idBlockAlloc< sdWayPoint, WAYPOINT_MAX_WAYPOINTS >	_allocator;
};

typedef sdSingleton< sdWayPointManagerLocal > sdWayPointManager;

#endif // __GAME_ROLES_WAYPOINTMANAGER_H__
