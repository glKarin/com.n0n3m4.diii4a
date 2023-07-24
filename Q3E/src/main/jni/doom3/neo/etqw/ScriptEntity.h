// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __GAME_SCRIPTENTITY_H__
#define __GAME_SCRIPTENTITY_H__

#include "AnimatedEntity.h"
#include "interfaces/RadarInterface.h"
#include "interfaces/ResupplyInterface.h"
#include "interfaces/GuiInterface.h"
#include "interfaces/NetworkInterface.h"
#include "interfaces/CrosshairInterface.h"
#include "interfaces/CommandmapInterface.h"
#include "interfaces/DeployControl.h"
#include "interfaces/TaskInterface.h"
#include "interfaces/UsableInterface.h"
#include "Weapon.h"
#include "structures/TeamManager.h"
#include "botai/Bot_Common.h"
#include "effects/WaterEffects.h"

const static int						MAX_SCRIPTENTITY_PATHPOINTS = 128;

class sdScriptedEntityHelper;
class sdScriptEntity;
class sdPredictionErrorDecay_Origin;
class sdPredictionErrorDecay_Angles;

class sdMotorSound {
public:
	void				Init( gameSoundChannel_t _channel, idEntity* _entity );
	void				Start( const idSoundShader* shader );
	void				Update( float desiredValue );

private:
	idEntity*			entity;
	gameSoundChannel_t	channel;
	float				value;
	float				inrate;
	float				outrate;
	float				low;
	float				high;
	float				volume;
	bool				active;
	const idSoundShader*shader;
};

class sdMotorSoundGroup {
public:
	void				Init( idEntity* _entity ) { entity = _entity; }

						~sdMotorSoundGroup( void );

	sdMotorSound*		Alloc( void );

private:
	idEntity*				entity;
	idList< sdMotorSound* > sounds;

	static idBlockAlloc< sdMotorSound, 64 > s_allocator;
};

class sdScriptedRadarInterface {
public:
	typedef sdPair< sdTeamInfo*, sdTeamInfo::sdRadarLayer* > sdLayer;

										sdScriptedRadarInterface( void ) { ; }
										~sdScriptedRadarInterface( void ) { FreeLayers(); }

	int									AllocRadarLayer( sdTeamInfo* team );
	int									AllocJammerLayer( sdTeamInfo* team );

	void								FreeLayer( int index );
	void								FreeLayers( void );

	void								UpdatePosition( const idVec3& origin, const idMat3& axes );
	sdTeamInfo::sdRadarLayer*			GetLayer( int index ) { return index >= 0 && index < layers.Num() ? layers[ index ].second : NULL; }

private:
	idList< sdLayer >					layers;
};

class sdScriptedNetworkInterface : public sdNetworkInterface {
public:
										sdScriptedNetworkInterface( void ) { owner = NULL; }

	void								Init( sdScriptEntity* _owner );

	virtual void						HandleNetworkMessage( idPlayer* player, const char* message );
	virtual void						HandleNetworkEvent( const char* message );

protected:
	idEntity*							owner;
	const sdProgram::sdFunction*		messageFunction;
	const sdProgram::sdFunction*		eventFunction;
};

class sdScriptedGuiInterface : public sdGuiInterface {
public:
										sdScriptedGuiInterface( void ) { owner = NULL; }

	void								Init( sdScriptEntity* _owner );

	virtual void						UpdateGui( void );
	virtual void						HandleGuiScriptMessage( idPlayer* player, const char* message );

	bool								WantsToThink( void ) const;

protected:
	sdScriptEntity*						owner;
	const sdProgram::sdFunction*		function;
	const sdProgram::sdFunction*		messageFunction;
};

class sdScriptedCrosshairInterface : public sdCrosshairInterface {
public:
										sdScriptedCrosshairInterface( void ) { owner = NULL; }

	void								Init( sdScriptEntity* _owner );

	virtual bool						UpdateCrosshairInfo( idPlayer* player ) const;

protected:
	sdScriptEntity*						owner;
	const sdProgram::sdFunction*		function;
	idLinkList< idEntity >				targetNode;
};

class sdScriptedUsableInterface : public sdUsableInterface {
private:
	class sdPosition {
	public:
											sdPosition( void );

		void								Init( const idDict& info, sdScriptEntity* owner );

		idPlayer*							GetPlayer( void ) const;
		void								SetPlayer( idPlayer* player );

		bool								IsThirdperson( void ) const { return flags.thirdperson; }
		bool								GetShowPlayer( void ) const { return flags.showPlayer; }
		bool								GetHideHud( void ) const { return flags.hideHud; }
		bool								GetHideDecoyInfo( void ) const { return flags.hideDecoyInfo; }
		bool								GetShowTargetingInfo( void ) const { return flags.showTargetingInfo; }

		const idVec3&						GetFirstPersonViewOffset( void ) const { return firstPersonCameraOffset; }
		const idMat3&						GetFirstPersonViewAxis( void ) const { return firstPersonCameraAxis; }
		float								GetThirdpersonDistance( void ) const { return thirdpersonDistance; }
		float								GetThirdpersonHeight( void ) const { return thirdpersonHeight; }		
		const char*							GetPlayerAnim( void ) const { return playerAnim; }
		float								GetFov( void ) const { return fov; }
		const sdWeaponLockInfo*				GetWeaponLockInfo( void ) const { return &lockInfo; }
		jointHandle_t						GetAttachJoint( void ) const { return attachJoint; }
		bool								GetAllowPlayerDamage( void ) const { return flags.takesDamage; }
		bool								GetShowCockpit( void ) const { return cockpitInfo != NULL; }

		const sdDeclStringMap*				GetCockpit( void ) const { return cockpitInfo; }

		const angleClamp_t&					GetClampYaw( void ) const { return clampYaw; }
		const angleClamp_t&					GetClampPitch( void ) const { return clampPitch; }	

		virtual const sdDeclLocStr*			GetWeaponName( void ) const { return weaponName; }
		virtual const char*					GetWeaponLookupName( void ) const { return weaponLookupName.c_str(); }

	private:
		idEntityPtr< idPlayer >				boundPlayer;

		typedef struct usableFlags_s {
			bool							thirdperson			: 1;
			bool							showPlayer			: 1;
			bool							hideHud				: 1;
			bool							allowMovement		: 1;
			bool							takesDamage			: 1;
			bool							hideDecoyInfo		: 1;
			bool							showTargetingInfo	: 1;
		} usableFlags_t;

		float								thirdpersonDistance;
		float								thirdpersonHeight;

		idVec3								firstPersonCameraOffset;
		idMat3								firstPersonCameraAxis;

		float								fov;

		idStr								playerAnim;		

		angleClamp_t						clampYaw;
		angleClamp_t						clampPitch;

		usableFlags_t						flags;

		jointHandle_t						attachJoint;

		const sdDeclStringMap*				cockpitInfo;

		const sdDeclLocStr*					weaponName;
		idStr								weaponLookupName;

		sdWeaponLockInfo					lockInfo;
	};

public:
										sdScriptedUsableInterface( void ) { owner = NULL; }

	void								Init( sdScriptEntity* _owner );

	virtual void						ForcePlacement( idPlayer* other, int index, int oldIndex, bool keepCamera );
	virtual void						FindPositionForPlayer( idPlayer* other );
	virtual bool						OnExit( idPlayer* player, bool force );
	virtual int							GetNumPositions() const;
	virtual idPlayer*					GetPlayerAtPosition( int i ) const;

	virtual void						BecomeActiveViewProxy( idPlayer* viewPlayer );
	virtual void						StopActiveViewProxy( void );
	virtual void						UpdateProxyView( idPlayer* viewPlayer );

	virtual void						SwapPosition( idPlayer* player );
	virtual void						SwapViewMode( idPlayer* player );
	virtual void						SelectWeapon( idPlayer* player, int index );

	virtual bool						GetHideHud( idPlayer* player ) const;
	virtual bool						GetSensitivity( idPlayer* player, float& sensX, float& sensY ) const;
	virtual bool						GetShowPlayer( idPlayer* player ) const;
	virtual bool						GetShowPlayerShadow( idPlayer* player ) const { return false; }
	virtual float						GetFov( idPlayer* player ) const;
	virtual float						GetDamageScale( idPlayer* player ) const { return 1.f; }
	virtual bool						GetShowCrosshair( idPlayer* player ) const { return true; }
	virtual bool						GetHideDecoyInfo( idPlayer* player ) const;
	virtual bool						GetShowTargetingInfo( idPlayer* player ) const;
	virtual playerStance_t				GetPlayerStance( idPlayer* player ) const;
	virtual jointHandle_t				GetPlayerIconJoint( idPlayer* player ) const;

	virtual idPlayer*					GetXPSharer( float& shareFactor );

	virtual void						UpdateHud( idPlayer* player, guiHandle_t handle );
	virtual void						UpdateViewAngles( idPlayer* player );
	virtual void						UpdateViewPos( idPlayer* player, idVec3& origin, idMat3& axis, bool fullUpdate );
	virtual const idAngles				GetRequiredViewAngles( idPlayer* player, idVec3& target );
	virtual void						CalculateRenderView( idPlayer* player, renderView_t& renderView );

	virtual const sdWeaponLockInfo*		GetWeaponLockInfo( idPlayer* player ) const;

	virtual void						ClampViewAngles( idPlayer* player, const idAngles& oldViewAngles ) const;

	virtual void						NextWeapon( idPlayer* player ) const;
	virtual void						PrevWeapon( idPlayer* player ) const;

	virtual int							GetDestructionEndTime( void ) const { return 0; }
	virtual bool						GetDirectionWarning( void ) const { return false; }
	virtual int							GetRouteKickDistance( void ) const { return -1; }

	virtual const sdDeclGUI*			GetOverlayGUI( void ) const;
	virtual bool						HasAbility( qhandle_t handle, const idPlayer* player ) const;

	void								UpdatePlayerViews( void ) const;
	void								PresentPlayers( void ) const;
	idPlayer*							GetBoundPlayer( int index ) const;

	void								SetPositionPlayer( idPlayer* player, int index );

	int									NumFreePositions( void ) const;

	bool								IsEmpty( void ) const;

	void								SetXPShareInfo( idPlayer* player, float factor );

	void								SetupCockpit( idPlayer* viewPlayer );

	sdPosition&							PositionForPlayer( const idPlayer* player );
	const sdPosition&					PositionForPlayer( const idPlayer* player ) const;


	virtual bool						GetAllowPlayerMove( idPlayer* player ) const { return false; }
	virtual bool						GetAllowPlayerDamage( idPlayer* player ) const;
	virtual bool						GetAllowPlayerWeapon( idPlayer* player ) const { return false; }
	virtual bool						GetAllowAdjustBodyAngles( idPlayer* player ) const { return false; }

	void								ForceExitForAllPlayers( void );

	virtual const sdDeclLocStr*			GetWeaponName( const idPlayer* player ) const;
	virtual const char*					GetWeaponLookupName( const idPlayer* player ) const;
	virtual const sdDeclLocStr*			GetPositionTitle( int i ) const;

	virtual void						OnPostThink( void ) {}

private:
	idEntityPtr< idPlayer >				xpSharer;
	float								xpShareFactor;
	sdScriptEntity*						owner;
	idList< sdPosition >				positions;
	const sdDeclGUI*					overlay;
	const sdProgram::sdFunction*		onEnter;
	const sdProgram::sdFunction*		onExit;	
	idVec3								originalPos;				// TODO: remove me -- use joints like vehicles
};

class sdScriptedInteractiveInterface : public sdInteractiveInterface {
public:
										sdScriptedInteractiveInterface( void ) { owner = NULL; }

	void								Init( sdScriptEntity* _owner );
	virtual bool						OnActivate( idPlayer* player, float distance );
	virtual bool						OnActivateHeld( idPlayer* player, float distance );
	virtual bool						OnUsed( idPlayer* player, float distance );

private:
	const sdProgram::sdFunction*		onActivate;
	const sdProgram::sdFunction*		onActivateHeld;
	const sdProgram::sdFunction*		onUsed;

	sdScriptEntity*						owner;
};

// TODO: This isn't the best place to keep this around.
//       Finding a better place for it would be good.
class sdEntityDisplayIconInfo {
public:
	static int SortByDistance( const sdEntityDisplayIconInfo* a, const sdEntityDisplayIconInfo* b ) {
		return ( b->distance - a->distance ) > 0.0f ? 1 : -1;
	}

	idVec2				size;
	const idMaterial*	material;
	idVec4				color;

	idVec2				origin;
	float				distance;
};

class sdScriptEntityDisplayIconInterface : public sdEntityDisplayIconInterface {
public:
	void								Init( sdScriptEntity* _owner );

	virtual bool						HasIcon( idPlayer* viewer, sdWorldToScreenConverter& converter );
	virtual bool						GetEntityDisplayIconInfo( idPlayer* viewer, sdWorldToScreenConverter& converter, sdEntityDisplayIconInfo& iconInfo );

	typedef enum {
		EI_CENTER = 0,
		EI_ABOVE
	} iconPosition_t;

	typedef enum {
		EI_NONE = 0,
		EI_WHITE = 0,
		EI_TEAM
	} iconColorMode_t;

	void								SetIconMaterial( const idMaterial* material ) { storedInfo.material = material; }
	void								SetIconSize( const idVec2& size ) { storedInfo.size = size; }
	void								SetIconColorMode( iconColorMode_t mode ) { colorMode = mode; }
	void								SetIconPosition( iconPosition_t pos ) { position = pos; }
	void								SetIconEnabled( bool _enabled ) { enabled = _enabled; }
	void								SetIconCutoff( float dist ) { cutoffDistance = dist; }
	void								SetIconAlphaScale( float scale ) { alphaScale = scale; }


private:
	sdScriptEntity*						owner;

	bool								enabled;
	float								cutoffDistance;
	iconPosition_t						position;
	iconColorMode_t						colorMode;
	float								alphaScale;

	bool								hasIconResult;
	sdEntityDisplayIconInfo				storedInfo;
};

class sdScriptEntityNetworkData : public sdEntityStateNetworkData {
public:
										sdScriptEntityNetworkData( void ) : physicsData( NULL ) { ; }
	virtual								~sdScriptEntityNetworkData( void );

	virtual void						MakeDefault( void );

	virtual void						Write( idFile* file ) const;
	virtual void						Read( idFile* file );

	sdScriptObjectNetworkData			scriptData;
	sdEntityStateNetworkData*			physicsData;
	idAngles							deltaViewAngles;
};

class sdScriptEntityBroadcastData : public sdEntityStateNetworkData {
public:
										sdScriptEntityBroadcastData( void ) : physicsData( NULL ), team( NULL ) { ; }
	virtual								~sdScriptEntityBroadcastData( void );

	virtual void						MakeDefault( void );

	virtual void						Write( idFile* file ) const;
	virtual void						Read( idFile* file );

	sdScriptObjectNetworkData			scriptData;
	sdEntityStateNetworkData*			physicsData;

	sdTeamInfo*							team;
	int									health;
	sdBindNetworkData					bindData;
	bool								frozen;
};

class idPhysics_RigidBody;
class sdPhysics_SimpleRigidBody;
class sdScriptEntity : public idAnimatedEntity {
public:
	CLASS_PROTOTYPE( sdScriptEntity );

	enum {
		EVENT_SETBOXCLIPMODEL = idAnimatedEntity::EVENT_MAXEVENTS,
		EVENT_RESETPREDICTIONDECAY,
		EVENT_MAXEVENTS
	};
											sdScriptEntity(	void );
	virtual									~sdScriptEntity( void );

	void									Spawn( void );
	virtual void							PostMapSpawn( void );

	void									UpdateScript( void );
	bool									SetState( const sdProgram::sdFunction* newState );

	virtual sdTeamInfo*						GetGameTeam( void ) const { return team; }
	virtual void							SetGameTeam( sdTeamInfo* _team );

	virtual bool							WantsTouch( void ) const { return touchFunc != NULL; }

	virtual bool							ShouldConstructScriptObjectAtSpawn( void ) const { return false; }

	virtual bool							StartSynced( void ) const { return spawnArgs.GetBool( "option_no_sync" ) == 0; }

	virtual void							OnTouch( idEntity *other, const trace_t& trace );

	int										GetTeamIndex( void );

	bool									GetIsDisabled( void );
	idEntity*								GetOwner( void );
	bool									GetIsDeployed( void );
	int										GetDeployableType( void ) { return deployableType; }
	float									GetDeployableRange( void ) { return deployableRange; }

	virtual void							Think( void );
	virtual void							PostThink( void );
	virtual idLinkList<idEntity>*			GetPostThinkNode( void );
	virtual void							Killed( idEntity* inflictor, idEntity* attacker, int damage, const idVec3& dir, int location, const sdDeclDamage* damageDecl );
	virtual void							Damage( idEntity* inflictor, idEntity* attacker, const idVec3& dir, const sdDeclDamage* damage, const float damageScale, const trace_t* collision, bool forceKill = false );

	virtual	void							UpdateKillStats( idPlayer* player, const sdDeclDamage* damageDecl, bool headshot );

	virtual bool							WantsToThink( void ) const;

	virtual void							OnGuiActivated( void );

	virtual bool							HasAbility( qhandle_t handle ) const;
	virtual bool							SupportsAbilities( void ) const { return scriptEntityFlags.allowAbilities; }

	void									UpdateRadar( void );

	bool									IsFrozen( void ) const { return scriptEntityFlags.frozen; }

	sdProgramThread*						ConstructScriptObject( void );
	sdProgramThread*						CreateScriptThread( const sdProgram::sdFunction* state );

	void									Event_SetState( const char* stateName );
	void									Event_AddHelper( int stringMapIndex );
	void									Event_GetBoundPlayer( int index );
	void									Event_RemoveBoundPlayer( idEntity* other );
	void 									Event_RadarSetNumLayers( int count );

	void									Event_RadarFreeLayers( void );
	void									Event_AllocRadarLayer( void );
	void									Event_AllocJammerLayer( void );

	void 									Event_RadarSetLayerRange( int index, float range );
	void 									Event_RadarSetMaxAngle( int index, float maxAngle );
	void 									Event_RadarSetJammer( int index, float value );
	void 									Event_RadarSetMask( int index, int mask );
	void									Event_Freeze( bool freeze );
	void									Event_SetRemoteViewAngles( const idVec3& angles, idEntity* refPlayer );
	void									Event_GetRemoteViewAngles( idEntity* refPlayer );

	// path manipulation
	int										PathGetNumPoints( void ) { return pathPoints.Num(); }
	const idVec3&							PathGetPoint( int index );
	float									PathGetLength( void );
	void									PathGetPosition( float position, idVec3& result );
	void									PathGetDirection( float position, idVec3& result );
	void									PathGetAxes( float position, idMat3& result );

	void									Event_PathFind( const char* pathName, const idVec3& target, float startTime, float direction, float cornerX, float cornerY, float heightOffset, bool append );
	void									Event_PathFindVampire( const idVec3& runStart, const idVec3& runEnd, float runHeightOffset );
	void									Event_PathLevel( float maxSlopeAngle, int startIndex, int endIndex );
	void									Event_PathStraighten();
	void									Event_PathGetNumPoints( void );
	void									Event_PathGetPoint( int index );
	void									Event_PathGetLength( void );
	void									Event_PathGetPosition( float position );
	void									Event_PathGetDirection( float position );
	void									Event_PathGetAngles( float position );
	void									Event_GetVampireBombPosition( const idVec3& targetPos, float gravity, float pathSpeed, float stepDist, float pathLength );
	void									Event_GetVampireBombAcceleration( void );
	void									Event_GetVampireBombFallTime( void );


	void									Event_SetGroundPosition( const idVec3& position );
	void									Event_SetXPShareInfo( idEntity* entity, float scale );
	void									Event_SetBoxClipModel( const idVec3& mins, const idVec3& maxs, float mass );

	void									Event_GetTeamDamageDone( void );
	void									Event_SetTeamDamageDone( int value );

	void									Event_ForceAnimUpdate( bool value );
	void									Event_SetIKTarget( idEntity* other, int index );

	void									Event_HideInLocalView( void );
	void									Event_ShowInLocalView( void );

	void									Event_SetClipOriented( bool oriented );

	virtual bool							UpdateCrosshairInfo( idPlayer* player, sdCrosshairInfo& info );

	virtual void							DisableClip( bool activateContacting = true );
	virtual void							EnableClip( void );

	virtual bool							ClientReceiveEvent( int event, int time, const idBitMsg& msg );
	virtual void							WriteDemoBaseData( idFile* file ) const;
	virtual void							ReadDemoBaseData( idFile* file );

	virtual void							ApplyNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState );
	virtual void							ReadNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const;
	virtual void							WriteNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const;
	virtual bool							CheckNetworkStateChanges( networkStateMode_t mode, const sdEntityStateNetworkData& baseState ) const;
	virtual sdEntityStateNetworkData*		CreateNetworkStructure( networkStateMode_t mode ) const;
	virtual void							ResetNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState );
	virtual void							OnSnapshotHitch();

	virtual sdGuiInterface*					GetGuiInterface( void ) { return guiInterface; }
	virtual sdNetworkInterface*				GetNetworkInterface( void ) { return networkInterface; }
	virtual sdTaskInterface*				GetTaskInterface( void ) { return taskInterface; }
	virtual sdUsableInterface*				GetUsableInterface( void ) { return usableInterface; }	
	virtual sdInteractiveInterface*			GetInteractiveInterface( void ) { return interactiveInterface; }

	idLinkList< sdScriptedEntityHelper >&	GetHelpers( void ) { return helpers; }

	virtual const idVec3&					GetRadarOrigin( void ) const;
	virtual const idMat3&					GetRadarAxes( void ) const;

	virtual bool							Collide( const trace_t &collision, const idVec3& velocity, int bodyId );
	virtual void							Hit( const trace_t &collision, const idVec3 &velocity, idEntity *other );

	virtual bool							NeedsRepair();

	virtual void							SetHealth( int count );
	virtual void							SetHealthDamaged( int count, teamAllegiance_t allegiance );
	virtual void							SetMaxHealth( int count ) { maxHealth = count; }
	virtual int								GetHealth( void ) const { return health; }
	virtual int								GetMaxHealth( void ) const { return maxHealth; }

	void									SetRemoteViewAngles( const idAngles& angles, const idPlayer* refPlayer );
	idAngles								GetRemoteViewAngles( const idPlayer* refPlayer );
	idAngles								GetDeltaViewAngles( void ) const { return deltaViewAngles; }

	virtual float							GetRadiusPushScale( void ) const { return radiusPushScale; }

	virtual void							OnPlayerEntered( idPlayer* player, int index ) { ; }
	virtual void							OnPlayerExited( idPlayer* player, int index ) { ; }

	virtual bool							OverridePreventDeployment( idPlayer* p );

	virtual cheapDecalUsage_t				GetDecalUsage( void );

	virtual void							GetImpactInfo( idEntity *ent, int id, const idVec3 &point, impactInfo_s* info );
	virtual	bool							IsCollisionPushable( void );

	void									Event_EnableCollisionPush( void );
	void									Event_DisableCollisionPush( void );

	sdMotorSoundGroup&						GetMotorSounds( void ) { return motorSounds; }

	virtual void							UpdateModelTransform( void );

	void									IncForcedAnimUpdate( void ) { forcedAnimCounter++; }
	void									DecForcedAnimUpdate( void ) { forcedAnimCounter--; }

	virtual void							UpdatePredictionErrorDecay( void );
	virtual void							DoPredictionErrorDecay( void );
	virtual void							OnNewOriginRead( const idVec3& newOrigin );
	virtual void							OnNewAxesRead( const idMat3& newAxes );
	virtual void							ResetPredictionErrorDecay( const idVec3* origin = NULL, const idMat3* axes = NULL );

	virtual bool							IsPhysicsInhibited( void ) const { return idEntity::IsPhysicsInhibited() && !scriptEntityFlags.noInhibitPhysics; }

	virtual void							CheckWater( const idVec3& waterBodyOrg, const idMat3& waterBodyAxis, idCollisionModel* waterBodyModel );
	virtual void							CheckWaterEffectsOnly( void ) { if ( waterEffects != NULL ) { waterEffects->CheckWaterEffectsOnly(); } }

	virtual bool							AllowAnimationInhibit( void ) { return forcedAnimCounter == 0; }

	virtual void							OnMouseMove( idPlayer* player, const idAngles& baseAngles, idAngles& angleDelta );

	virtual bool							DisableClipOnRemove( void ) const { return true; }

	virtual sdEntityDisplayIconInterface*	GetDisplayIconInterface( void ) { return &displayIconInterface; }
	
	void									Event_SetIconMaterial( const char* materialName );
	void									Event_SetIconSize( float width, float height );
	void									Event_SetIconColorMode( int mode );
	void									Event_SetIconPosition( int pos );
	void									Event_SetIconEnabled( bool _enabled );
	void									Event_SetIconCutoff( float distance );
	void									Event_SetIconAlphaScale( float scale );


protected:
	virtual bool							OnApplyPain( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location, const sdDeclDamage* damageDecl );

	virtual void							UpdatePlayZoneInfo( void );

	sdMotorSoundGroup						motorSounds;

	sdTeamInfo*								team;
	sdScriptedRadarInterface				radarInterface;
	sdScriptedGuiInterface*					guiInterface;
	sdScriptedCrosshairInterface*			crosshairInterface;
	sdScriptedNetworkInterface*				networkInterface;
	sdTaskInterface*						taskInterface;
	sdScriptedUsableInterface*				usableInterface;
	sdScriptedInteractiveInterface*			interactiveInterface;
	sdScriptEntityDisplayIconInterface		displayIconInterface;

	int										health;
	int										maxHealth;
	float									armor;

	int										forcedAnimCounter;

	sdPredictionErrorDecay_Origin*			predictionErrorDecay_Origin;
	sdPredictionErrorDecay_Angles*			predictionErrorDecay_Angles;

	struct scriptEntityFlags_t {
		bool								writeBind : 1;
		bool								frozen : 1;
		bool								inhibitDecals : 1;
		bool								noCollisionPush : 1;
		bool								takesOobDamage : 1;
		bool								hasNewBoxClip : 1;
		bool								writeViewAngles : 1;
		bool								noInhibitPhysics : 1;
		bool								allowAbilities : 1;
	};

	scriptEntityFlags_t						scriptEntityFlags;

	idPhysics*								physicsObj;

	// state variables
	sdProgramThread*						baseScriptThread;
	const sdProgram::sdFunction*			scriptState;
	const sdProgram::sdFunction*			scriptIdealState;

	const sdProgram::sdFunction*			preDamageFunc;
	const sdProgram::sdFunction*			postDamageFunc;
	const sdProgram::sdFunction*			onCollideFunc; // I hit target
	const sdProgram::sdFunction*			onHitFunc; // Target hit me
	const sdProgram::sdFunction*			overrideFunc;
	const sdProgram::sdFunction*			onPostThink;	

	const sdProgram::sdFunction*			isDeployedFunc;
	const sdProgram::sdFunction*			isDisabledFunc;
	const sdProgram::sdFunction*			getOwnerFunc;
	int										deployableType;
	float									deployableRange;

	int										teamDamageDone;

	const sdProgram::sdFunction*			needsRepairFunc; // need repair callback
															// Gordon: not sure what this is for?

	const sdProgram::sdFunction*			touchFunc;

	const sdDeclDeployableObject*			deployObject;

	idLinkList< idEntity >					postThinkEntNode;

	idLinkList< idEntity >					iconNode;

	idLinkList< sdScriptedEntityHelper >	helpers;

	sdAbilityProvider						abilities;

	idAngles								deltaViewAngles;

	idStr									killStatSuffix;

	// pathfinding request data
	void									LevelPathSegment( float maxSlope, int startPoint, int endPoint, bool reverse );
	void									FindPathCircle( const idVec3& a, const idVec3& b, const idVec3& c, idVec3& center, float& radius );

	struct pathPoint_t {
		idVec3			origin;
		idMat3			axis;
	};

	idList< pathPoint_t >					pathPoints;
	float									lastPathPosition;
	float									lastPathLength;
	int										lastPathPoint;
	idVec3									cachedBombAccel;
	float									cachedBombFallTime;

	float									radiusPushScale;

	int										lastTimeInPlayZone;

	int										oobDamageInterval;
	sdWaterEffects							*waterEffects;
};



class sdScriptEntity_ProjectileBroadcastData : public sdScriptEntityBroadcastData {
public:
										sdScriptEntity_ProjectileBroadcastData( void ) : launchTime( 0 ), owner( 0 ) { ; }

	virtual void						MakeDefault( void );

	virtual void						Write( idFile* file ) const;
	virtual void						Read( idFile* file );

	int						launchTime;
	int						owner;
};

class sdScriptEntity_Projectile : public sdScriptEntity {
public:
	CLASS_PROTOTYPE( sdScriptEntity_Projectile );

	virtual									~sdScriptEntity_Projectile( void ) {}

	virtual void							Create( idEntity* owner, const idVec3& start, const idVec3& dir );

	idEntity*								GetOwner( void ) const { return owner; }

	virtual bool							IsOwner( idEntity* other ) { return owner == other; }

	virtual bool							CanCollide( const idEntity* other, int traceId ) const;

	virtual void							ApplyNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState );
	virtual void							ReadNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const;
	virtual void							WriteNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const;
	virtual bool							CheckNetworkStateChanges( networkStateMode_t mode, const sdEntityStateNetworkData& baseState ) const;
	virtual sdEntityStateNetworkData*		CreateNetworkStructure( networkStateMode_t mode ) const;

private:
	void									Event_GetLaunchTime( void );
	void									Event_GetOwner( void );

private:
	int								launchTime;
	idEntityPtr< idEntity >			owner;
};

#endif // __GAME_SCRIPTENTITY_H__
