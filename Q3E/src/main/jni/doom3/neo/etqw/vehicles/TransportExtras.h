// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __GAME_VEHICLES_TRANSPORTEXTRAS_H__
#define __GAME_VEHICLES_TRANSPORTEXTRAS_H__

class sdVehicleWeapon;
class sdVehicleIKSystem;
class sdVehicleView;
class sdTransport;

class sdVehiclePosition {
public:
									sdVehiclePosition( void );
									~sdVehiclePosition( void );

	void							Think( void );

	const sdDeclLocStr*				GetHudName( void ) const { return hudname; }
	idPlayer*						GetPlayer( void ) const;
	int								GetPositionId( void ) const { return index; }
	const char*						GetName( void ) const { return name; }
	sdTransport*					GetTransport( void ) const { return transport; }
	int								GetWeaponIndex( void ) const { return weaponIndex; }
	int								GetNumViewModes( void ) const { return views.Num(); }
	const sdDeclToolTip*			GetBlockedToolTip( void ) const { return blockedTip; }
	float							GetDamageScale( void ) const { return damageScale; }
	playerStance_t					GetPlayerStance( void ) const { return playerStance; }
	jointHandle_t					GetIconJoint( void ) const { return iconJoint; }
	float							GetPlayerHeight( void ) const { return playerHeight; }

	int								FindDefaultWeapon( void ) const;
	int								FindLastWeapon( void ) const;
	bool							WeaponValid( sdVehicleWeapon* weapon ) const;

	void							SetPositionId( int _index ) { index = _index; }
	void							SetHudName( const sdDeclLocStr* _name ) { hudname = _name; }
	void							SetPlayer( idPlayer* _player );
	void							SetName( const char* _name ) { name = _name; }
	void							SetTransport( sdTransport* _transport ) { transport = _transport; }
	void							SetWeaponIndex( int _weaponIndex );

	void							LoadData( const idDict& dict );

	bool							CheckRequirements( idPlayer* player );

	void							AddIKSystem( sdVehicleIKSystem* _ikSystem );

	void							AddView( const positionViewMode_t& parms );
	const sdVehicleView&			GetViewParms( void ) const;
	sdVehicleView&					GetViewParms( void );
	void							CycleCamera( void );

	bool							GetTakesDamage( void ) const { return takesDamage; }
	bool							GetAllowWeapon( void ) const { return allowWeapon; }
	bool							GetAllowAdjustBodyAngles( void ) const { return allowAdjustBodyAngles; }

	void							UpdateIK( void );
	void							UpdatePlayerView( void );
	void							PresentPlayer( void );

	bool							HasAbility( qhandle_t handle ) const;

	static bool						ClampAngle( idAngles& newAngles, const idAngles& oldAngles, angleClamp_t clamp, int index, float epsilon = -1 );

	void							UpdateViews( sdVehicleWeapon* weapon );

	float							GetCurrentViewOffset( void ) const { return currentViewOffsetAngles; }

	jointHandle_t					GetAttachJoint( void ) const { return attachJoint; }
	const char*						GetAttachAnim( void ) const { return attachAnim; }
	bool							GetShowPlayer( void ) const { return showPlayer; }
	bool							GetEjectOnKilled( void ) const { return ejectOnKilled; }
	bool							GetResetViewOnEnter( void ) const { return resetViewOnEnter; }

	const char*						GetCockpit( void ) const { return cockpitName; }

	void							OnVehicleScriptReloaded();

protected:
	void							UpdateWeapon( int index );

	int								index;
	int								weaponIndex;
	bool							showPlayer;
	float							minZfrac;
	bool							ejectOnKilled;
	bool							takesDamage;
	float							playerHeight;
	bool							allowWeapon;
	bool							allowAdjustBodyAngles;
	bool							resetViewOnEnter;

	playerStance_t					playerStance;
	jointHandle_t					iconJoint;

	int								playerEnteredTime;

	const sdDeclLocStr*				hudname;
	idStr							name;

	idEntityPtr< idPlayer >			player;

	static const int				MAX_POSITION_VIEWS = 4;
	idStaticList< sdVehicleView*, MAX_POSITION_VIEWS >	views;

	sdTransport*					transport;

	const sdDeclToolTip*			blockedTip;

	sdRequirementContainer			requirements;

	static const int				MAX_POSITION_IK = 2;
	idStaticList< sdVehicleIKSystem*, MAX_POSITION_VIEWS >	ikSystems;

	jointHandle_t					attachJoint;
	idStr							attachAnim;

	float							maxViewOffset;
	float							viewOffsetRate;
	float							currentViewOffset;
	float							currentViewOffsetAngles;

	float							damageScale;

	sdAbilityProvider				abilities;

	sdPlayerStatEntry*				statTimeSpent;

	idStr							cockpitName;
};


typedef enum ejectionFlags_e {
	EF_KILL_PLAYERS			= BITT< 0 >::VALUE,
} ejectionFlags_t;

class sdTransportPositionManager {
public:
									sdTransportPositionManager( void );
									~sdTransportPositionManager( void );

	void							RemovePlayer( sdVehiclePosition& position );
	bool							EjectPlayer( sdVehiclePosition& position, bool force );
	void							EjectAllPlayers( int flags = 0 );

	sdVehiclePosition&				PositionForPlayer( const idPlayer* player );	// uses fast lookup using index on player
	const sdVehiclePosition&		PositionForPlayer( const idPlayer* player ) const;	// uses fast lookup using index on player
	sdVehiclePosition*				FreePosition( idPlayer* player, const sdDeclToolTip** tip, int startIndex = 0 );
	sdVehiclePosition*				PositionForId( const int positionId );
	const sdVehiclePosition*		PositionForId( const int positionId ) const;

	bool							HasFreePosition( void ); //mal: does this vehicle have a free seat open a bot could use?

	int								NumPositions( void ) const { return positions.Num(); }

	void							SwapPosition( idPlayer* player, bool allowCycle );

	const wchar_t*					PositionNameForPlayer( idPlayer* player );

	void							ClearPositions( void );
	idPlayer*						FindDriver( void );
	sdVehiclePosition*				FindDriverPosition( void );

	void							Think( void );
	void							UpdatePlayerViews( void );
	void							PresentPlayers( void );

	void							Init( const sdDeclVehicleScript* vehicleScript, sdTransport* other );

	void							BanPlayer( int clientNum, int length );
	void							ResetBan( int clientNum );
	bool							IsPlayedBanned( int clientNum ) { return bannedPlayers[ clientNum ] > gameLocal.time; }

	bool							IsEmpty( void );

	void							OnVehicleScriptReloaded();

	int								GetPlayerExitTime( idPlayer* player ) const { return playerExitTime[ player->entityNumber ]; }

protected:
	static const int				MAX_EXIT_JOINTS	= 8;
	static const int				MAX_POSITIONS = 8;

	int								bannedPlayers[ MAX_CLIENTS ];
	sdTransport*					transport;
	idStaticList< sdVehiclePosition, MAX_POSITIONS >	positions;
	idStaticList< jointHandle_t, MAX_EXIT_JOINTS >		exitJoints;


	idStaticList< int, MAX_CLIENTS >			playerExitTime;
};

class sdVehicleInput {
public:
	sdVehicleInput( void ) { Clear(); }

	float		GetForward( void ) const;
	float		GetRight( void ) const;
	float		GetUp( void ) const;
	float		GetCmdYaw( void ) const;
	float		GetCmdPitch( void ) const;
	float		GetCmdRoll( void ) const;
	idAngles	GetCmdAngles( void ) const { return idAngles( GetCmdPitch(), GetCmdYaw(), GetCmdRoll() ); }

	void		Clear( void ) { memset( &data, 0, sizeof( data ) ); }

	// driving
	void		SetForce( float force )				{ data.force = force; }
	void		SetRightSpeed( float speed )		{ data.leftSpeed = speed; }
	void		SetLeftSpeed( float speed )			{ data.rightSpeed = speed; }
	void		SetSteerAngle( float steerAngle )	{ data.steerAngle = steerAngle; }
	void		SetSteerSpeed( float value )		{ data.steerSpeed = value; }
	void		SetBraking( bool braking )			{ data.braking = braking; }
	void		SetHandBraking( bool handBraking )	{ data.handBraking = handBraking; }
	
	float		GetForce( void ) const				{ return data.force; }
	float		GetRightSpeed( void ) const			{ return data.rightSpeed; }
	float		GetLeftSpeed( void ) const			{ return data.leftSpeed; }
	float		GetSteerAngle( void ) const			{ return data.steerAngle; }
	float		GetSteerSpeed( void ) const			{ return data.steerSpeed; }
	bool		GetBraking( void ) const			{ return data.braking; }
	bool		GetHandBraking( void ) const		{ return data.handBraking; }

	// flying
	void		SetCollective( float value )		{ data.collective = value; }
	void		SetYaw( float value )				{ data.yaw = value; }
	void		SetPitch( float value )				{ data.pitch= value; }
	void		SetRoll( float value )				{ data.roll= value; }

	float		GetCollective( void ) const			{ return data.collective; }
	float		GetYaw( void ) const				{ return data.yaw; }
	float		GetPitch( void ) const				{ return data.pitch; }
	float		GetRoll( void ) const				{ return data.roll; }


	void		SetPlayer( idPlayer* player );
	idPlayer*	GetPlayer( void ) const { return data.player; }

	const usercmd_t& GetUserCmd( void ) const { return data.usercmd; }

private:
	typedef struct data_s {
		float		force;
		float		leftSpeed;
		float		rightSpeed;
		float		steerAngle;
		float		steerSpeed;
		bool		braking;
		bool		handBraking;

		float		collective;
		float		yaw;
		float		pitch;
		float		roll;

		idPlayer*	player;
		usercmd_t	usercmd;
		idAngles	cmdAngles;
	} data_t;

	data_t data;
};

#endif // __GAME_VEHICLES_TRANSPORTEXTRAS_H__
