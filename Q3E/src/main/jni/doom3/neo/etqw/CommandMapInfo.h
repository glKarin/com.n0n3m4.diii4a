// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_COMMANDMAPINFO_H__
#define __GAME_COMMANDMAPINFO_H__

class sdCommandMapInfo {
public:
	typedef enum drawMode_e {
		DM_MATERIAL,
		DM_CIRCLE,
		DM_ARC,
		DM_ROTATED_MATERIAL,
		DM_TEXT,
		DM_CROSSHAIR,
	} drawMode_t;

	typedef enum colorMode_e {
		CM_NORMAL,
		CM_FRIENDLY,
		CM_ALLEGIANCE,
	} colorMode_t;

	typedef enum positionMode_e {
		PM_ENTITY,
		PM_FIXED,
	} positionMode_t;

	typedef enum scaleMode_e {
		SM_FIXED,
		SM_WORLD,
	} scaleMode_t;

	typedef enum cmFlags_e {
		CMF_TEAMONLY					= BITT< 0 >::VALUE,
		CMF_NOADJUST					= BITT< 1 >::VALUE,
		CMF_ALWAYSKNOWN					= BITT< 2 >::VALUE,
		CMF_ONLYSHOWKNOWN				= BITT< 3 >::VALUE,
		CMF_DROPSHADOW					= BITT< 4 >::VALUE,
		CMF_ENEMYONLY					= BITT< 5 >::VALUE,
		CMF_FOLLOWROTATION				= BITT< 6 >::VALUE,
		CMF_ENEMYALWAYSKNOWN			= BITT< 7 >::VALUE,
		CMF_ONLYSHOWONFULLVIEW			= BITT< 8 >::VALUE,
		CMF_FIRETEAMONLY				= BITT< 9 >::VALUE,
		CMF_FIRETEAMKNOWN				= BITT< 10 >::VALUE,
		CMF_FOLLOWREMOTECAMERAORIGIN	= BITT< 11 >::VALUE,
		CMF_PLAYERROTATIONONLY			= BITT< 12 >::VALUE,
		CMF_FIRETEAMCOLORING			= BITT< 13 >::VALUE,
	} cmFlags_t;

public:
									sdCommandMapInfo( idEntity* owner, int sort );
									~sdCommandMapInfo( void );

	int								GetSort( void ) const { return _sort; }
	idLinkList< sdCommandMapInfo >&	GetActiveNode( void ) { return _activeNode; }

	void							SetSort( int sort );
	void							SetColor( const idVec4& color ) { _color = color; }
	void							SetColor( const idVec3& color ) { _color.x = color.x; _color.y = color.y; _color.z = color.z; }
	void							SetAlpha( float alpha ) { _color.w = alpha; }
	void							SetDrawMode( drawMode_t drawMode ) { _drawMode = drawMode; }
	void							SetColorMode( colorMode_t colorMode ) { _colorMode = colorMode; }
	void							SetScaleMode( scaleMode_t scaleMode ) { _scaleMode = scaleMode; }
	void							SetPositionMode( positionMode_t positionMode ) { _positionMode = positionMode; }
	void							SetOrigin( const idVec2& origin ) { _origin = origin; }
	void							SetSize( float size ) { _size = idVec2( size, size ); }
	void							SetUnknownSize( float size ) { _unknownSize = idVec2( size, size ); }
	void							SetSize( const idVec2& size ) { _size = size; }
	void							SetUnknownSize( const idVec2& size ) { _unknownSize = size; }
	void							SetAngle( float angle ) { _angle = idMath::AngleNormalize360( angle ); }
	void							SetSides( int sides ) { _sides = sides; }
	void							Show( void );
	void							Hide( void );
	void							SetUnknownMaterial( const idMaterial* material ) { _unknownMaterial = material; }
	void							SetFireteamMaterial( const idMaterial* material ) { _fireteamMaterial = material; }
	void							SetMaterial( const idMaterial* material ) { _material = material; }
	void							SetGuiMessage( const char* message ) { _guiMessage = message; }
	void							SetFlag( int flag ) { _flagsBackup = -1; _flags |= flag; }
	void							ClearFlag( int flag ) { _flagsBackup = -1; _flags &= ~flag; }
	int								GetFlags( void ) { return _flags; }
	void							SetShaderParm( int index, float value );
	void							SetText( const wchar_t* text ) { _text = text; }
	void							SetFont( const char* fontName );
	void							SetTextScale( float textScale ) { _textScale = textScale; }
	void							Flash( const idMaterial* material, int msec, int setFlags );

	void							FreeFont( void );

	bool							CanAdjustPosition( void ) const { return _scaleMode == SM_FIXED && ( ( _flags & CMF_NOADJUST ) == 0 ); }
	bool							IsAlwaysKnown( void ) const { return ( _flags & CMF_ALWAYSKNOWN ) != 0; }
	bool							OnlyInFullView( void ) const { return ( _flags & CMF_ONLYSHOWONFULLVIEW ) != 0; }
	bool							EnemyOnly( void ) const { return ( _flags & CMF_ENEMYONLY ) != 0; }
	bool							EnemyAlwaysKnown( void ) const { return ( _flags & CMF_ENEMYALWAYSKNOWN ) != 0; }
	bool							IsFireTeamOnly( void ) const { return ( _flags & CMF_FIRETEAMONLY ) != 0; }
	bool							IsFireTeamKnown( void ) const { return ( _flags & CMF_FIRETEAMKNOWN ) != 0; }

	void							SetArcAngle( float arcAngle ) { _arcAngle = arcAngle; }

	const idVec2&					GetSize( void ) const { return _size; }
	idEntity*						GetOwner( void ) const { return _owner; }
	drawMode_t						GetDrawMode( void ) const { return _drawMode; }
	const char*						GetGuiMessage( void ) const { return _guiMessage; }
	void							GetOrigin( idVec2& out ) const;
	const sdRequirementContainer&	GetRequirements( void ) const { return _requirements; }
	sdRequirementContainer&			GetRequirements( void ) { return _requirements; }

	void							Draw( idPlayer* player, const idVec2& position, const idVec2& size, const idVec2& screenPos, const idVec2& mapScale, bool known, float sizeScale, const idMat2& rotation, float angle, bool fullSize );

private:
	drawMode_t						_drawMode;
	colorMode_t						_colorMode;
	scaleMode_t						_scaleMode;
	positionMode_t					_positionMode;

	const idMaterial*				_material;
	const idMaterial*				_unknownMaterial;
	const idMaterial*				_fireteamMaterial;
	const idMaterial*				_flashMaterial;

	int								_sort;
	idVec2							_size;
	idVec2							_unknownSize;
	idVec4							_color;
	float							_angle;
	int								_sides;
	idStr							_guiMessage;
	int								_flags;
	int								_flagsBackup;
	float							_arcAngle;
	idVec2							_origin;
	idWStr							_text;
	qhandle_t						_font;
	float							_textScale;
	int								_flashEndTime;

	idEntityPtr< idEntity >			_owner;

	sdRequirementContainer			_requirements;

	idLinkList< sdCommandMapInfo >	_activeNode;

	idStaticList< float, MAX_ENTITY_SHADER_PARMS - 4 >		_shaderParms;

public:
	static idCVar					g_rotateCommandMap;
};

class sdCommandMapInfoManagerLocal {
public:
	static const int				MAX_ICONS = 256;

public:
	qhandle_t						Alloc( idEntity* owner, int sort );
	void							Free( qhandle_t handle );
	void							Init( void );
	void							Shutdown( void );
	void							SortIntoList( sdCommandMapInfo* info );
	sdCommandMapInfo*				GetInfo( qhandle_t handle ) { return handle == -1 ? NULL : commandMapIcons[ handle ]; }
	sdCommandMapInfo*				GetIcons( void ) { return activeIcons.Next(); }
	void							Clear( void );

	void							OnEntityDeleted( idEntity* ent );

private:
	idLinkList< sdCommandMapInfo >	activeIcons;
	idList< sdCommandMapInfo* >		commandMapIcons;
};

typedef sdSingleton< sdCommandMapInfoManagerLocal > sdCommandMapInfoManager;

#endif // __GAME_COMMANDMAPINFO_H__

