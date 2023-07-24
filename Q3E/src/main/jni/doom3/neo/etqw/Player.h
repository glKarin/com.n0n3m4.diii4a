// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_PLAYER_H__
#define __GAME_PLAYER_H__

typedef enum playerStance_t {
	PS_DEAD,
	PS_PRONE,
	PS_CROUCH,
	PS_NORMAL,
};

#include "PredictionErrorDecay.h"
#include "Actor.h"
#include "PlayerView.h"
#include "PlayerIcon.h"
#include "GameEdit.h"
#include "physics/Physics_Player.h"
#include "roles/Inventory.h"
#include "roles/Tasks.h"
#include "proficiency/ProficiencyManager.h"
#include "CrosshairInfo.h"
#include "structures/DeployZone.h"
#include "Teleporter.h"
#include "effects/HardcodedParticleSystem.h"
#include "effects/WaterEffects.h"
#include "proficiency/StatsTracker.h"

class sdVehiclePosition;
class idWeapon;
class sdDeclToolTip;
class sdWorldToScreenConverter;
class sdPlayerStateData;
class sdDeployMask;
class sdPlayZone;
class sdPlayerDisplayIconList;
struct deployMaskExtents_t;

typedef sdPair< const sdDeclToolTip*, sdToolTipParms* > toolTipParms_t;

const int STEPUP_TIME				= 200;

#define PLAYER_DAMAGE_LOG

/*
===============================================================================

	Player entity.
	
===============================================================================
*/

enum viewState_t {
	VS_NONE,
	VS_REMOTE,
	VS_FULL,
};

extern const idEventDef EV_Player_GetButton;
extern const idEventDef EV_Player_GetMove;
extern const idEventDef EV_Player_GetAmmoFraction;
extern const idEventDef EV_Player_GetVehicle;
extern const idEventDef EV_Player_GetClassKey;
extern const idEventDef EV_Player_GetEnemy;

enum playerCombatState_t {
	COMBATSTATE_DAMAGERECEIVED	= BITT< 0 >::VALUE,
	COMBATSTATE_DAMAGEDEALT		= BITT< 1 >::VALUE,
	COMBATSTATE_KILLEDPLAYER	= BITT< 2 >::VALUE,
};

class sdPlayerStateData : public sdEntityStateNetworkData {
public:
										sdPlayerStateData( void ) { ; }

	virtual void						MakeDefault( void );

	virtual void						Write( idFile* file ) const;
	virtual void						Read( idFile* file );

	idList< int >						timers;

	int									stepUpTime;
	float								stepUpDelta;
};

class sdPlayerStateBroadcast : public sdEntityStateNetworkData {
public:
										sdPlayerStateBroadcast( void ) { ; }

	virtual void						MakeDefault( void );

	virtual void						Write( idFile* file ) const;
	virtual void						Read( idFile* file );

	sdInventoryPlayerStateData			inventoryData;

	int									lastDamageDecl;
	int									lastDamageDir;
	short								lastDamageLocation;

	int									lastHitCounter;
	int									lastHitEntity;
	bool								lastHitHeadshot;
};

class sdPlayerNetworkData : public sdEntityStateNetworkData {
public:
										sdPlayerNetworkData( void ) { ; }

	virtual void						MakeDefault( void );

	virtual void						Write( idFile* file ) const;
	virtual void						Read( idFile* file );

	int									proxyEntitySpawnId;

	sdPlayerPhysicsNetworkData			physicsData;
	sdScriptObjectNetworkData			scriptData;
	bool								hasPhysicsData;

	short								deltaViewAngles[ 2 ];
	short								viewAngles[ 2 ];

	sdPlayerStateData					playerStateData;
};

class sdPlayerBroadcastData : public sdEntityStateNetworkData {
public:
										sdPlayerBroadcastData( void ) { ; }

	virtual void						MakeDefault( void );

	virtual void						Write( idFile* file ) const;
	virtual void						Read( idFile* file );

	sdPlayerPhysicsBroadcastData		physicsData;
	sdInventoryBroadcastData			inventoryData;
	sdScriptObjectNetworkData			scriptData;

	short								health;
	short								maxHealth;

	int									targetLockSpawnId;
	int									targetLockEndTime;

	float								baseDeathYaw;

	bool								isLagged;
	bool								forceRespawn;
	bool								wantSpawn;

	sdPlayerStateBroadcast				playerStateData;
};

typedef enum locationDamageArea_e {
	LDA_INVALID				= -1,
	LDA_LEGS,
	LDA_TORSO,
	LDA_HEAD,
	LDA_NECK,
	LDA_HEADBOX,
	LDA_COUNT,
} locationDamageArea_t;

typedef struct locationalDamageInfo_s {
	jointHandle_t			joint;
	idVec3					pos;
	locationDamageArea_t	area;
} locationalDamageInfo_t;

class sdPlayerInteractiveInterface : public sdInteractiveInterface {
public:
										sdPlayerInteractiveInterface( void ) { _owner = NULL; _activateFunc = NULL; }

	void								Init( idPlayer* owner );

	virtual bool						OnActivate( idPlayer* player, float distance );
	virtual bool						OnActivateHeld( idPlayer* player, float distance );
	virtual bool						OnUsed( idPlayer* player, float distance );

private:
	idPlayer*							_owner;
	const sdProgram::sdFunction*		_activateFunc;
	const sdProgram::sdFunction*		_activateHeldFunc;
};

class idPlayer : public idActor {

    friend class idBot; //mal: need access to player stuff

public:
	enum {
		EVENT_RESPAWN = idActor::EVENT_MAXEVENTS,
		EVENT_SETCLASS,
		EVENT_SETCACHEDCLASS,
		EVENT_SPECTATE,
		EVENT_DISGUISE,
		EVENT_SELECTTASK,
		EVENT_SETPROXY,
		EVENT_SETPROXYVIEW,
		EVENT_BINADD,
		EVENT_BINREMOVE,
		EVENT_SETTELEPORT,
		EVENT_SETCAMERA,
		EVENT_SETREADY,
		EVENT_SETINVULNERABLE,
		EVENT_VOTE_DELAY,
#ifdef PLAYER_DAMAGE_LOG
		EVENT_TAKEDAMAGE,
#endif // PLAYER_DAMAGE_LOG
		EVENT_RESETPREDICTIONDECAY,
		EVENT_GODMODE,
		EVENT_NOCLIP,
		EVENT_SETWEAPON,
		EVENT_LIFESTAT,
		EVENT_MAXEVENTS
	};

	enum eyePos_t {
		EP_SPECTATOR,
		EP_DEAD,
		EP_CROUCH,
		EP_PRONE,
		EP_PROXY,
		EP_NORMAL,
		EP_INVALID,
	};

	const static int		MAX_PLAYER_BIN_SIZE			= 32;

	usercmd_t				usercmd;

	eyePos_t				oldEyePosition;
	eyePos_t				eyePosition;
	int						eyePosChangeTime;
	int						eyePosChangeDuration;
	idVec3					eyePosChangeStart;

	idAngles				spawnAngles;
	idAngles				viewAngles;			// player view angles
	idAngles				cmdAngles;			// player cmd angles
	idAngles				cameraViewAngles;
	idAngles				clientViewAngles;	// client interpolated view angles
	float					baseDeathYaw;
	
	bool					lastWeaponViewAnglesValid;
	idAngles				lastWeaponViewAngles;
	idVec3					weaponAngVel;

	userButtonsUnion_t		oldButtons;
	int						oldFlags;
	userButtonsUnion_t		clientOldButtons;
	userButtonsUnion_t		clientButtons;
	bool					clientButtonsUsed;
	
	int						nextSndHitTime;			// MP hit sound - != lastHurtTime because we throttle

	int						lastReviveTime;

	int						voiceChatLimit;

	idScriptBool			AI_FORWARD;
	idScriptBool			AI_BACKWARD;
	idScriptBool			AI_STRAFE_LEFT;
	idScriptBool			AI_STRAFE_RIGHT;
	idScriptBool			AI_ATTACK_HELD;
	idScriptBool			AI_WEAPON_FIRED;
	idScriptBool			AI_JUMP;
	idScriptBool			AI_CROUCH;
	idScriptBool			AI_PRONE;
	idScriptBool			AI_SPRINT;
	idScriptBool			AI_ONGROUND;
	idScriptBool			AI_ONLADDER;
	idScriptBool			AI_DEAD;
	idScriptFloat			AI_LEAN;
	idScriptBool			AI_RUN;
	idScriptBool			AI_HARDLANDING;
	idScriptBool			AI_SOFTLANDING;
	idScriptBool			AI_RELOAD;
	idScriptBool			AI_TELEPORT;
	idScriptBool			AI_TURN_LEFT;
	idScriptBool			AI_TURN_RIGHT;
	idScriptBool			AI_PUTAWAY_ACTIVE;
	idScriptBool			AI_TAKEOUT_ACTIVE;
	idScriptInt				AI_INWATER;

	int						lastGroundContactTime;
	idEntityPtr< idWeapon >	weapon;
	
	int						vehicleViewCurrentZoom;

	int						lastTimeInPlayZone;

	float					targetIdRangeNormal;

	int						nextBannerPlayTime;
	int						nextBannerIndex;

	int						nextCallVoteTime;

	renderView_s			renderView;

	idLinkList< idEntity >	targetNode;	

	idEntityPtr< idEntity >	targetEntity;
	idEntityPtr< idEntity >	targetEntityPrevious;
	int						targetLockEndTime;
	int						targetLockDuration;
	int						targetLockLastTime;
	int						targetLockSafeTime;
	float					targetLockTimeScale;

	int						lastTeamSetTime;
	int						nextTeamSwitchTime;
	int						nextReadyToggleTime;
	int						nextSuicideTime;
	int						nextTeamBalanceSwitchTime;
	int						lastAliveTimeRegistered;

	int						gameStartTime;

	userInfo_t				userInfo;

	angleClamp_t			deadClampPitch;
	angleClamp_t			deadClampYaw;

	angleClamp_t			proneClampPitch;
	angleClamp_t			proneClampYaw;

	// damage taken from other players/vehicles/whatever
	struct damageEvent_t {
		int					hitTime;
		float				hitAngle;
		float				hitDamage;
		bool				updateDirection;
	};
	int						lastHurtTime;			// the last time we took damage (used for screen effects, etc.)

	static const int		MAX_DAMAGE_EVENTS = 16;
	static const int		NUM_REPAIR_INDICATORS = 4;
	idStaticList< damageEvent_t, MAX_DAMAGE_EVENTS > damageEvents;

	bool					ownsVehicle;
	bool					aasPullPlayer;
	int						lastOwnedVehicleSpawnID;
	int						lastOwnedVehicleTime;

private:
	idEntityPtr< idEntity >	selectedSpawnPoint;

	const sdDeclRating*		rating;

	int						lastDamageDealtTime;		// last time projectile fired by player hit target
	teamAllegiance_t		lastDamageDealtType;		// the allegiance of the last item hit
	bool					newDamageDealt;				// 

	int						lastDamageFriendlyVO;

public:
	virtual bool			Pain( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location, const sdDeclDamage* damageDecl );
	bool					InhibitMovement( void ) const;
	bool					InhibitTurning( void );
	bool					InhibitHud( void );
	bool					InhibitWeaponSwitch( bool allowPause = false ) const;
	bool					InhibitWeapon( void ) const;
	void					SetClip( int modIndex, int count );
	int						GetClip( int modIndex );

	int						GetNextTeamSwitchTime( void ) const { return nextTeamSwitchTime; }
	int						GetNextTeamBalanceSwitchTime( void ) const { return nextTeamBalanceSwitchTime; }
	void					SetNextTeamBalanceSwitchTime( int time ) { nextTeamBalanceSwitchTime = time; }

	int						GetNextCallVoteTime( void ) const { return nextCallVoteTime; }
	void					SetNextCallVoteTime( int time );

	void					RegisterTimeAlive( void );

	void					RequestQuickChat( const sdDeclQuickChat* quickChatDecl, int targetSpawnId );

							// the last time we took damage
	int						GetLastHurtTime( void ) { return lastHurtTime; }
	void					AddDamageEvent( int time, float angle, float damage, bool updateDirection );
	void					ClearDamageEvents();
	const damageEvent_t&	GetLastDamageEvent() const;

							// feedback for the player dealing damage to another entity
	void					SetLastDamageDealtTime( int time );
	int						GetLastDamageDealtTime() const { return lastDamageDealtTime; }
	teamAllegiance_t		GetLastDamageDealtType() const { return lastDamageDealtType; }
	bool					NewDamageDealt() const { return newDamageDealt; }
	void					ClearDamageDealt() { newDamageDealt = false; }


	void					SelectWeaponByName( const char* weaponName );

	idVec3					GetEyeOffset( eyePos_t pos );
	float					GetEyeChangeRate( eyePos_t pos );

	bool					HandleGuiEvent( const sdSysEvent* event );	
	bool					TranslateGuiBind( const idKey& key, sdKeyCommand** cmd );

	void					SetActionMessage( const char* message );
	void					UpdateToolTips( void );
	void					UpdateToolTipTimeline( void );

	void					RunToolTipTimelineEvent( const sdDeclToolTip::timelineEvent_t& event );
	
	bool					IsToolTipPlaying( void ) const { return toolTips.Num() > 0 || nextTooltipTime > gameLocal.time; }

	bool					IsSinglePlayerToolTipPlaying( void ) const { return currentToolTip != NULL && currentToolTip->GetSinglePlayerToolTip(); }

	void					UpdatePlayerInformation( void ); //mal: for the bots
	void					UpdatePlayerTeamInfo( void ); //mal: ditto
	bool					IsSniperScopeUp( void );
	bool					IsPanting( void );
	bool					InVehicle( void );

	virtual float			GetDamageXPScale( void ) const { return damageXPScale; }

	bool					IsReady( void ) const { return playerFlags.ready; }
	void					SetReady( bool value, bool force );
	
	void					UsercommandCallback( usercmd_t& cmd );
	bool					GetSensitivity( float& scaleX, float& scaleY );

	bool					KeyMove( char forward, char right, char up, usercmd_t& cmd );
	void					ControllerMove( bool doGameCallback, const int numControllers, const int* controllerNumbers,
											const float** controllerAxis, idVec3& viewAngles, usercmd_t& cmd );
	void					MouseMove( const idAngles& baseAngles, idAngles& angleDelta );

	void					UpdateShadows( void );
	viewState_t				HasShadow( void );

	virtual const char*		GetDefaultSurfaceType( void ) const { return "flesh"; }

	virtual void			ShutdownThreads( void );
	virtual bool			HasAbility( qhandle_t handle ) const;
	virtual bool			SupportsAbilities( void ) const { return true; }

	virtual bool			StartSynced( void ) const { return true; }

	void					UpdateRating( void );

	void					Revive( idPlayer* other, float healthScale );

	bool					IsUIFocused( const guiHandle_t& ui ) { return ui == focusUI; }

	bool					InhibitUserCommands();

	void					SetHipJoint( jointHandle_t handle ) { hipJoint = handle; }
	void					SetChestJoint( jointHandle_t handle ) { chestJoint = handle; }
	void					SetTorsoJoint( jointHandle_t handle ) { torsoJoint = handle; }
	void					SetHeadJoint( jointHandle_t handle ) { headJoint = handle; }
	void					SetShoulderJoint( int index, jointHandle_t handle ) { arms[ index ].shoulderJoint = handle; }
	void					SetElbowJoint( int index, jointHandle_t handle ) { arms[ index ].elbowJoint = handle; }
	void					SetHandJoint( int index, jointHandle_t handle ) { arms[ index ].handJoint = handle; }
	void					SetFootJoint( int index, jointHandle_t handle ) { arms[ index ].footJoint = handle; }

	void					SetHeadModelJoint( jointHandle_t handle ) { headModelJoint = handle; }
	void					SetHeadModelOffset( const idVec3& offset ) { headModelOffset = offset; }

	jointHandle_t			GetHipJoint( void ) const { return hipJoint; }
	jointHandle_t			GetChestJoint( void ) const { return chestJoint; }
	jointHandle_t			GetTorsoJoint( void ) const { return torsoJoint; }
	jointHandle_t			GetHeadJoint( void ) const { return headJoint; }
	jointHandle_t			GetShoulderJoint( int index ) const { return arms[ index ].shoulderJoint; }
	jointHandle_t			GetElbowJoint( int index ) const { return arms[ index ].elbowJoint; }
	jointHandle_t			GetHandJoint( int index ) const { return arms[ index ].handJoint; }
	jointHandle_t			GetFootJoint( int index ) const { return arms[ index ].footJoint; }

	jointHandle_t			GetHeadModelJoint( void ) const { return headModelJoint; }
	const idVec3&			GetHeadModelOffset() const { return headModelOffset; }

	void					ClearIKJoints( void );

	float					GetSwayScale( void ) const { return swayScale; }
	void					SetSwayScale( float newSwayScale ) { swayScale = newSwayScale; }

	void					UpdatePlayZoneInfo( void );

	virtual void			ApplyRadiusPush( const idVec3& pushOrigin, const idVec3& entityOrigin, const sdDeclDamage* damageDecl, float pushScale, float radius );

	void					SendUnLocalisedMessage( const wchar_t* message );
	void					SendLocalisedMessage( const sdDeclLocStr* locStr, const idWStrList& parms );

	bool					NeedsRevive( void ) const;
	bool					WantRespawn( void ) const { return ( playerFlags.forceRespawn || playerFlags.wantSpawn ) && CanPlay(); }
	bool					CanPlay( void ) const;
	void					ServerForceRespawn( bool noBody );
	void					ClientForceRespawn( void );
	void					OnForceRespawn( bool noBody );
	void					OnFullyKilled( bool noBody );
	void					SetSpawnPoint( idEntity* other );
	idEntity*				GetSpawnPoint( void ) const { return selectedSpawnPoint; }
	void					CreateBody( void );

	virtual void						ApplyNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState );
	virtual void						ReadNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const;
	virtual void						WriteNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const;
	virtual bool						CheckNetworkStateChanges( networkStateMode_t mode, const sdEntityStateNetworkData& baseState ) const;
	virtual void						ResetNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState );
	virtual sdEntityStateNetworkData*	CreateNetworkStructure( networkStateMode_t mode ) const;
	virtual void						OnSnapshotHitch();

	virtual bool						RunPausedPhysics( void ) const;

	void								ApplyPlayerStateData( const sdEntityStateNetworkData& newState );
	void								ReadPlayerStateData( const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const;
	void								WritePlayerStateData( const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const;
	bool								CheckPlayerStateData( const sdEntityStateNetworkData& baseState ) const;

	void								ApplyPlayerStateBroadcast( const sdEntityStateNetworkData& newState );
	void								ReadPlayerStateBroadcast( const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const;
	void								WritePlayerStateBroadcast( const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const;
	bool								CheckPlayerStateBroadcast( const sdEntityStateNetworkData& baseState ) const;

	bool								ShouldReadPlayerState( void ) const {
		if ( gameLocal.isRepeater && gameLocal.serverIsRepeater ) {
			return true;
		}
		return gameLocal.GetSnapshotPlayer() == this;
	}

	bool								ShouldWritePlayerState( void ) const {
		if ( gameLocal.isRepeater && gameLocal.snapShotClientIsRepeater ) {
			return true;
		}
		return gameLocal.GetSnapshotPlayer() == this;
	}

	virtual void						WriteInitialReliableMessages( const sdReliableMessageClientInfoBase& target ) const;


	idEntity*				GetProxyEntity( void ) const { return proxyEntity; }
	int						GetProxyPositionId( void ) const { return proxyPositionId; }
	int						GetProxyViewMode( void ) const { return proxyViewMode; }

	void					SetProxyEntity( idEntity* proxy, int positionId );
	void					SetProxyViewMode( int mode, bool force );

	void					OnProxyUpdate( int newProxySpawnId, int newProxyPositionId );

	void					ShowWeaponMenu( sdWeaponSelectionMenu::eActivationType show, bool noTimeout = false );
	virtual bool			AcceptWeaponSwitch( bool hideWeaponMenu = true );
	void					SwitchWeapon();

	virtual bool			CanCollide( const idEntity* other, int traceId ) const;

	virtual void			WriteDemoBaseData( idFile* file ) const;
	virtual void			ReadDemoBaseData( idFile* file );

	virtual cheapDecalUsage_t GetDecalUsage( void );

	virtual void			OnBindMasterVisChanged();

public:	
	void					PlayFootStep( const char* prefix, bool rightFoot );
	virtual void			PlayPain( const char* strength );

	void					PlayQuickChat( const idVec3& location, int sendingClientNum, const sdDeclQuickChat* quickChat, idEntity* target );

	void					PlayProneFailedToolTip( void );

	void					CancelToolTips( void );
	void					CancelToolTipTimeline( void );
	void					SendToolTip( const sdDeclToolTip* toolTip, sdToolTipParms* toolTipParms = NULL );
	void					SpawnToolTip( const sdDeclToolTip* toolTip, sdToolTipParms* toolTipParms = NULL );

	const sdDeclToolTip*	currentToolTip;
	idList< toolTipParms_t >toolTips;
	int						nextTooltipTime;
	int						invulnerableEndTime;
	int						lastDmgTime;

	int						actionMessageTime;
	idStr					actionMessage;

	const idVec3 &			GetViewPos( void ) const { return firstPersonViewOrigin; }
	const idMat3 &			GetViewAxis( void ) const { return firstPersonViewAxis; }
	const idAngles &		GetViewAngles( void ) const { return viewAngles; }
	const idAngles &		GetCommandAngles( void ) const { return cmdAngles; }

	idPlayer*				GetViewClient( void );
	idPlayer*				GetSpectateClient( void ) const;
	int						GetSpectateClientId( void );
	void					SetSpectateClient( idPlayer* player );
	void					SetSpectateId( int id );
	void					OnSetClientSpectatee( idPlayer* spectatee );

	qhandle_t				GetUserGroup( void );
	void					SetUserGroup( qhandle_t group );

	void						DropDisguise( void );
	int							GetDisguiseClient( void ) const { return disguiseClient; }
	virtual idEntity*			GetDisguiseEntity( void );
	const sdDeclPlayerClass*	GetDisguiseClass( void ) const { return disguiseClass; }
	sdTeamInfo*					GetDisguiseTeam( void ) const { return disguiseTeam; }
	bool						IsDisguised( void ) const { return disguiseClient != -1; }
	void						SetDisguised( int disguiseSpawnId, const sdDeclPlayerClass* playerClass, sdTeamInfo* playerTeam, const sdDeclRank* rank, const sdDeclRating* rating, const char* name );
	bool						CheckDetected( float detectionRange );

	void						BuildViewFrustum( idFrustum& frustum, float range ) const;

	virtual void				CheckWater( const idVec3& waterBodyOrg, const idMat3& waterBodyAxis, idCollisionModel* waterBodyModel );
	virtual void				CheckWaterEffectsOnly( void ) { if ( waterEffects != NULL ) { waterEffects->CheckWaterEffectsOnly(); } }

	static const int			NO_DEATH_SOUND = -0xD1E;	// HACK: magic value to not play death sound
	void						ClearDeathSound( void ) { health = NO_DEATH_SOUND; }

	bool						IsFPSUnlock( void );

	bool						IsCarryingObjective( void ) const { return playerFlags.carryingObjective; }

	bool						IsSendingVoice( void ) const { return ( sys->Milliseconds() - networkSystem->GetLastVoiceReceivedTime( entityNumber ) < SEC2MS( 0.5f ) ); }

private:

	void						PlayDeathSound( void );

	int							spectateClient;

	// Gordon: FIXME: Make a struct/class
	int							disguiseClient;
	idStr						disguiseUserName;
	const sdDeclRank*			disguiseRank;
	const sdDeclRating*			disguiseRating;
	sdTeamInfo*					disguiseTeam;
	const sdDeclPlayerClass*	disguiseClass;

	struct playerFlags_t {
		bool				forceRespawn			: 1;
		bool				sprintDisabled			: 1;
		bool				runDisabled				: 1;
		bool				isLagged				: 1;		// replicated from server, true if packets haven't been received from client.
		bool				wantSpectate			: 1;
		bool				wantSpawn				: 1;
		bool				targetLocked			: 1;
		bool				commandMapInfoVisible	: 1;
		bool				noclip					: 1;
		bool				godmode					: 1;
		bool				ingame					: 1;
		bool				weaponChanged			: 1;
		bool				turningFrozen			: 1;
		bool				sniperAOR				: 1;
		bool				spawnAnglesSet			: 1;		// on first usercmd, we must set deltaAngles
		bool				weaponLock				: 1;
		bool				clientWeaponLock		: 1;
		bool				legsForward				: 1;
		bool				scriptHide				: 1;		// Scripts want the player hidden
		bool				noShadow				: 1;
		bool				inPlayZone				: 1;
		bool				inhibitGUIs				: 1;
		bool				ready					: 1;
		bool				noFootsteps				: 1;
		bool				noFallingDamage			: 1;
		bool				manualTaskSelection		: 1;
		bool				carryingObjective		: 1;
	};

	playerFlags_t			playerFlags;

	int						killedTime;

	int						weaponSwitchTimeout;

	// index inside the UI for the weapon bank
	int						weaponSelectionItemIndex;

	sdWaterEffects*			waterEffects;

public:
	int						lastSpectateTeleport;
	int						lastHitCounter;
	idEntityPtr< idEntity >	lastHitEntity;
	bool					lastHitHeadshot;

	qhandle_t				lagIcon;

	qhandle_t				godIcon;

	// for crash landings
	int						landChange;
	int						landTime;

	int						nextHealthCheckTime;
	int						healthCheckTickTime;
	int						healthCheckTickCount;

	int						nextPlayerPushTime;

	// the first person view values are always calculated, even
	// if a third person view is used
	idVec3					firstPersonViewOrigin;
	idMat3					firstPersonViewAxis;

	idMat3					vehicleEyeAxis;
	idVec3					vehicleEyeOrigin;
	idAngles				lastVehicleViewAngles;
	int						lastVehicleViewAnglesChange;

	int						oobDamageInterval;

	void					ToggleCommandMap( void );

	int						GetWeaponSwitchTimeout( void ) const { return weaponSwitchTimeout; }
	void					SetWeaponSelectionItemIndex( int index ) { weaponSelectionItemIndex = index; }

	void					SetVehicleCameraMode( int viewMode, bool resetAngles, bool force );
	void					CycleVehicleCameraMode( void );
	void					SwapVehiclePosition( void );

	void					DoActivate( void );
	void					DoWeapNext( void );
	void					DoWeapPrev( void );

	void					SetActiveTask( taskHandle_t task );
	void					UpdateActiveTask( void );
	void					SetManualTaskSelection( bool value ) { playerFlags.manualTaskSelection = value; }
	bool					GetManualTaskSelection( void ) { return playerFlags.manualTaskSelection; }
	taskHandle_t			GetActiveTaskHandle( void ) const { return activeTask; }
	sdPlayerTask*			GetActiveTask( void ) const;
	void					SelectNextTask();

	virtual void			GetTaskName( idWStr& out ) { out = userInfo.wideName; }

	void					UpdatePopupGuiInfo( void );
	bool					UpdateCrosshairInfo( idPlayer* player, sdCrosshairInfo& info );

	virtual sdTeamInfo*		GetGameTeam( void ) const;

public:
	CLASS_PROTOTYPE( idPlayer );

							idPlayer();
	virtual					~idPlayer();

	void					Spawn( void );
	void					Think( void );

	virtual void			Hide( void );
	virtual void			Show( void );

	void					Init( bool setWeapon );
	void					LinkScriptVariables( void );
	void					SetupWeaponEntity( void );
	void					SetupWeaponEntity( int spawnId );
	void					SpawnFromSpawnSpot( void );
	void					OnRespawn( bool revive, bool parachute, int respawnTime, bool forced );
	void					SpawnToPoint( const idVec3& spawnOrigin, const idAngles& spawnAngles, bool revive, bool parachute );

	void					UserInfoChanged( void );
	const userInfo_t&		GetUserInfo( void ) const { return userInfo; }

	// for very specific usage. != GetPhysics()
	idPhysics_Player&		GetPlayerPhysics( void ) { return physicsObj; }
	const idPhysics_Player&	GetPlayerPhysics( void ) const { return physicsObj; }

	idWeapon*				GetWeapon( void ) { return weapon.GetEntity(); }
	const idWeapon*			GetWeapon( void ) const { return weapon.GetEntity(); }
	
	void					UpdateConditions( void );
	void					SetViewAngles( const idAngles &angles );
	virtual void			SetAxis( const idMat3 &axis );
	virtual void			SetOrigin( const idVec3 &org );
	virtual void			SetPosition( const idVec3 &org, const idMat3 &axis );

							// delta view angles to allow movers to rotate the view of the player
	void					UpdateDeltaViewAngles( const idAngles &angles );

	virtual bool			Collide( const trace_t &collision, const idVec3 &velocity, int bodyId );

	virtual void			SetHealth( int count );
	virtual void			SetMaxHealth( int count );
	virtual int				GetHealth( void ) const { return health; }
	virtual int				GetMaxHealth( void ) const { return maxHealth; }

	virtual void			DisableClip( bool activateContacting = true );
	virtual void			EnableClip( void );

	void					GetHeadModelCenter( idVec3& output );
	float					GetDamageScaleForTrace( const trace_t& t, const idVec3& traceDirection, locationDamageArea_t& area );
	void					CalcDamagePoints(  idEntity *inflictor, idEntity *attacker, const sdDeclDamage* damageDecl, 
											const float damageScale, const trace_t* collision, float& _health, const idVec3& dir, bool& headshot );

	virtual void			DamageFeedback( idEntity *victim, idEntity *inflictor, int oldHealth, int newHealth, const sdDeclDamage* damageDecl, bool headshot );

	virtual	void			Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const sdDeclDamage* damage, const float damageScale, const trace_t* collision, bool forceKill = false );

#ifdef PLAYER_DAMAGE_LOG
	void					LogDamage( int damage, const sdDeclDamage* damageDecl );
	void					UpdateDamageLog( void );
	void					ClearDamageLog( void );
	void					InitDamageLog( void );
	static bool				DamageLogModelCallback( renderEntity_t *renderEntity, const renderView_t *renderView, int& lastGameModifiedTime );
#endif // PLAYER_DAMAGE_LOG

	void					UpdateCombatModel( void );

	void					WriteStats( void );

	virtual	void			UpdateKillStats( idPlayer* player, const sdDeclDamage* damageDecl, bool headshot );

	void					CalcLifeStats( void );
	void					SendLifeStatsMessage( int statIndex, const sdPlayerStatEntry::statValue_t& oldValue, const sdPlayerStatEntry::statValue_t& newValue );
	void					HandleLifeStatsMessage( int statIndex, const sdPlayerStatEntry::statValue_t& oldValue, const sdPlayerStatEntry::statValue_t& newValue );

	void					Kill( idEntity* killer, bool noBody = false, const sdDeclDamage* damage = NULL, const sdDeclDamage* applyDamage = NULL );
	void					OnKilled( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );
	virtual void			Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location, const sdDeclDamage* damageDecl );

	virtual bool			UpdateAnimationControllers( void );

	void					Obituary( idEntity* killer, const sdDeclDamage* damageDecl );

	renderView_t *			GetRenderView( void );
	void					SetupDeploymentView( renderView_t& rView );
	void					CalculateRenderView( void );	// called every tic by player code
	void					CalculateFirstPersonView( bool fullUpdate );

	void					SetTeleportEntity( sdTeleporter* teleport );
	sdTeleporter*			GetTeleportEntity( void ) { return teleportEntity; }

	void					WeaponFireFeedback( const weaponFeedback_t& feedback );

	static float			DefaultFov( void );

	float					CalcFov( void );
	void					CalculateViewWeaponPos( idVec3 &origin, idMat3 &axis, bool ignorePitch );
	idVec3					GetEyePosition( void ) const;
	void					GetViewPos( idVec3 &origin, idMat3 &axis ) const;
	void					GetRenderViewAxis( idMat3 &axis ) const;
	void					OffsetThirdPersonView( float angle, float range, float height, bool clip, renderView_t& rView );
	bool					CalculateLookAtView( idPlayer* other, renderView_t& rView, bool doTraceCheck = false, float maxDist = -1.0f );

	void					UpdatePausedObjectiveView();

	void					UpdateBriefingView();

	bool					CanGetClass( const sdDeclPlayerClass* pc );

	bool					Give( const char *statname, const char *value );
	bool					GiveClass( const char* classname );
	void					ChangeClass( const sdDeclPlayerClass* pc, int classOption );
	bool					GivePackage( const sdDeclItemPackage* package );
	void					GiveClassProficiency( float count, const char* reason );

	int						Heal( int count );
	void					OnHealed( int oldHealth, int health );

	bool					IsWeaponValid( void );
	void					Reload( void );
	void					NextWeapon( bool safe = false );
	void					PrevWeapon( void );
	void					SelectWeapon( int num, bool force );
	void					UseWeaponAmmo( int modIndex );
	int						AmmoForWeapon( int modIndex );
	void					UseAmmo( ammoType_t type, int amount );


	void					LowerWeapon( void );
	void					RaiseWeapon( void );
	void					WeaponLoweringCallback( void );
	void					WeaponRisingCallback( void );

	// deployment stuff
	void					EnterDeploymentMode( void );
	void					ExitDeploymentMode( void );
	deployResult_t			CheckBoundsForDeployment( const deployMaskExtents_t& extents, const sdDeployMaskInstance& mask, const sdDeclDeployableObject* deploymentObject, const sdPlayZone* pz );
	deployResult_t			GetDeployResult( idVec3& point, const sdDeclDeployableObject* deploymentObject );
	deployResult_t			CheckDeployPosition( idVec3& point, const sdDeclDeployableObject* deploymentObject );
	bool					GetDeployPosition( idVec3& point );

	static void				DrawLocalPlayerAttitude( sdUserInterfaceLocal* ui, float x, float y, float w, float h );
	static void				ZoomInCommandMap_f( const idCmdArgs& args );
	static void				ZoomOutCommandMap_f( const idCmdArgs& args );
	static void				SetupCommandMapZoom( void );

	idEntity*				GetEnemy( void );
	void					ClearTargetLock( void );
	void					UpdateTargetLock( void );
	bool					CheckTargetLockValid( idEntity* entity, const sdWeaponLockInfo* lockInfo );
	bool					CheckTargetCanLock( idEntity* target, float boundsScale, float& distance, const idFrustum& frustum );
	const sdCrosshairInfo&	GetCrosshairInfo( bool doTrace );
	const sdCrosshairInfo&	GetCrosshairInfoDirect( void ) { return crosshairInfo; }

	void					UpdateVisibility( void );

	void					PerformImpulse( int impulse );
	void					Spectate( void );
	void					UpdateHud( void );

	void					UpdatePlayerKills( int victimNum, idEntity *client );
	bool					IsCamping( void );

	virtual void			SetGameTeam( sdTeamInfo* team );
	bool					IsTeam( const sdTeamInfo* _team ) const { return ( team != NULL ) && ( team == _team ); }
	bool					IsSpectating( void ) const;
	bool					IsSpectator( void ) const;

	// New Inventory System
	sdInventory&			GetInventory( void ) { return inventoryExt; }
	const sdInventory&		GetInventory( void ) const { return inventoryExt; }

	// Proficiency Stuff
	sdProficiencyTable&			GetProficiencyTable( void ) { return gameLocal.GetProficiencyTable( entityNumber ); }
	const sdProficiencyTable&	GetProficiencyTable( void ) const { return gameLocal.GetProficiencyTable( entityNumber ); }

	const sdDeclRating*		GetRating( void ) const { return rating; }

	void					OnProficiencyGain( int index, float count, const char* reason );
	void					OnProficiencyLevelGain( const sdDeclProficiencyType* type, int oldLevel, int newLevel );

	void					CalculateView( void );

	void					SetRemoteCamera( idEntity* other, bool force );
	idEntity*				GetRemoteCamera( void ) { return remoteCamera.GetEntity(); }

	virtual bool			GetPhysicsToSoundTransform( idVec3 &origin, idMat3 &axis );
	virtual void			UpdateModelTransform( void );

	virtual void			UpdatePredictionErrorDecay( void );
	virtual void			OnNewOriginRead( const idVec3& newOrigin );
	virtual void			OnNewAxesRead( const idMat3& newAxes );
	virtual void			ResetPredictionErrorDecay( const idVec3* origin = NULL, const idMat3* axes = NULL );
	void					ResetAntiLag( void );

	virtual bool			ClientReceiveEvent( int event, int time, const idBitMsg &msg );

	// server side work for in/out of spectate. takes care of spawning it into the world as well
	void					ServerSpectate( void );

	bool					OnLadder( void ) const;

	void					UpdatePlayerIcons( void );
	bool					GetPlayerIcon( idPlayer* viewer, sdPlayerDisplayIcon& icon, const sdWorldToScreenConverter& converter );
	void					FlashPlayerIcon( void );
	bool					IsObscuredBySmoke( idPlayer* viewer );

	void					SetLagged( bool lagged );
	void					SetGodMode( bool god );
	void					UpdateGodIcon();
	void					SetClientWeaponLock( bool lock ) { playerFlags.clientWeaponLock = lock; }
	void					SetNoClip( bool noclip );
	void					SetInGame( bool inGame );
	void					SetWantSpectate( bool want ) { playerFlags.wantSpectate = want; }

	void					SetInhibitGuis( bool inhibit ) { playerFlags.inhibitGUIs = inhibit; }
	bool					GetInhibitGuis() const { return playerFlags.inhibitGUIs; }

	bool					GetGodMode( void ) const { return playerFlags.godmode; }
	bool					GetNoClip( void ) const { return playerFlags.noclip; }
	bool					GetWantSpectate( void ) const { return !gameLocal.isClient && playerFlags.wantSpectate; }
	bool					GetIsLagged( void ) const { return playerFlags.isLagged; }
	bool					GetSniperAOR( void ) const { return playerFlags.sniperAOR; }
	bool					GetTargetLocked( void ) const { return playerFlags.targetLocked; }

	bool					IsInLimbo( void ) const { return playerFlags.forceRespawn; }
	bool					IsDead( void ) const { return health <= 0 && !IsInLimbo() && !GetWantSpectate() && !IsSpectator(); }
	bool					IsInPlayZone( void ) const { return playerFlags.inPlayZone; }
	bool					IsInvulernable( void ) const { return playerFlags.noclip || playerFlags.godmode || invulnerableEndTime > gameLocal.time; }
	bool					IsBeingBriefed() const;

	void					SetInvulnerableEndTime( int time );

	void					SetGameStartTime( int time );
	void					LogTimePlayed( void );

	void					Suicide( void );

	virtual	void			OnUpdateVisuals( void );

	bool					ServerDeploy( const sdDeclDeployableObject* object, float rotation, int delayMS );

	const sdWeaponLockInfo* GetCurrentLockInfo( void );
	void					SetTargetEntity( idEntity* entity );

	void					AddLocationalDamageInfo( const locationalDamageInfo_t& ldi ) { locationalDamageInfo.Alloc() = ldi; }
	void					RemoveLocationalDamageInfo( void ) { locationalDamageInfo.Clear(); }
	locationDamageArea_t	LocationalDamageAreaForString( const char* const str ) const;

	void					SetWeaponSpreadInfo( void ) const;

	playerTaskList_t&		GetTaskList( void ) { return availableTasks; }

	void					SetSuppressPredictionReset( bool reset ) { suppressPredictionReset = reset; }
	void					GetAORView( idVec3& origin, idMat3& axis );
	
	const idVec3&			GetLastPredictionErrorDecayOrigin( void );

private:
	// Gordon: vehicle stuff
	idEntityPtr< idEntity >	proxyEntity;
	int						proxyPositionId;
	int						proxyViewMode;

	sdInventory				inventoryExt;

	sdCrosshairInfo			crosshairInfo;
	int						lastCrosshairTraceTime;
	bool					crosshairEntActivate;

	float					speedModifier;
	float					sprintScale;
	float					swayScale;

	float					damageXPScale;

	jointHandle_t			hipJoint;
	jointHandle_t			chestJoint;
	jointHandle_t			torsoJoint;
	jointHandle_t			headJoint;

	struct arm_t {
		jointHandle_t		shoulderJoint;
		jointHandle_t		elbowJoint;
		jointHandle_t		handJoint;
		jointHandle_t		footJoint; // Gordon: we have feet on our arms, honest
	};

	arm_t					arms[ 2 ];

	jointHandle_t			headModelJoint;
	idVec3					headModelOffset;

	taskHandle_t			activeTask;
	playerTaskList_t		availableTasks;

	idList< float >						damageAreasScale;		// FIXME: Move to the player class
	idList< locationalDamageInfo_t >	locationalDamageInfo;	// FIXME: Move to the player class

	idPhysics_Player		physicsObj;			// player physics

	int						bobFoot;
	float					bobFrac;
	float					bobfracsin;
	int						bobCycle;			// for view bobbing and footstep generation
	float					xyspeed;
	int						stepUpTime;
	float					stepUpDelta;
	float					idealLegsYaw;
	float					legsYaw;
	float					oldViewYaw;
	idAngles				viewBobAngles;
	idVec3					viewBob;

	int						health;
	int						maxHealth;
	int						teamDamageDone;

	const idDeclSkin*		skin;

	jointHandle_t			leftFootJH, rightFootJH;

	idEntityPtr< idEntity >	remoteCamera;
	idVec3					remoteCameraViewOffset;

	idEntityPtr< idPlayer >	killer;
	int						killerTime;

	idEntityPtr< sdTeleporter >	teleportEntity;
	int							nextTeleportTime;
	int							lastTeleportTime;

	// if there is a focusGUIent, the attack button will be changed into mouse clicks
	idEntityPtr< idEntity >	focusGUIent;
	guiHandle_t				focusUI;				// focusGUIent->renderEntity.gui, gui2, or gui3
	int						focusTime;	

	int						lastDamageDecl;
	idVec3					lastDamageDir;
	int						lastDamageLocation;

	sdPredictionErrorDecay_Origin	predictionErrorDecay_Origin;
	sdPredictionErrorDecay_Angles	predictionErrorDecay_Angles;
	bool					suppressPredictionReset;

	// mp
	int						nextSpectateChange;

	int						combatState;
	int						nextBattleSenseBonusTime;

	idPlayerIcon			playerIcon;
	int						playerIconFlashTime;

	bool					lastWeaponMenuState;
	int						lastWeaponSwitchPos;
	bool					enableWeaponSwitchTimeout;
	int						lastTimelineTime;

	// script callback functions
	const sdProgram::sdFunction*	raiseWeaponFunction;
	const sdProgram::sdFunction*	lowerWeaponFunction;
	const sdProgram::sdFunction*	reloadWeaponFunction;
	const sdProgram::sdFunction*	weaponFiredFunction;

	const sdProgram::sdFunction*	onProficiencyChangedFunction;
	const sdProgram::sdFunction*	onSpawnFunction;
	const sdProgram::sdFunction*	onKilledFunction;
	const sdProgram::sdFunction*	onHitActivateFunction;
	const sdProgram::sdFunction*	onSetTeamFunction;
	const sdProgram::sdFunction*	onFullyKilledFunction;
	const sdProgram::sdFunction*	onPainFunction;
	const sdProgram::sdFunction*	onDisguisedFunction;
	const sdProgram::sdFunction*	onConsumeHealthFunction;
	const sdProgram::sdFunction*	onConsumeStroyentFunction;
	const sdProgram::sdFunction*	onFireTeamJoinedFunction;
	const sdProgram::sdFunction*	onFireTeamBecomeLeader;
	const sdProgram::sdFunction*	onHealedFunction;
	const sdProgram::sdFunction*	onJumpFunction;
	const sdProgram::sdFunction*	onSendingVoice;
	const sdProgram::sdFunction*	sniperScopeUpFunc;
	const sdProgram::sdFunction*	needsParachuteFunc;
	const sdProgram::sdFunction*	isPantingFunc;

	const idDeclSkin*		viewSkin;

	sdPlayerInteractiveInterface	interactiveInterface;

	float					armor;
	
	idStaticList< idEntityPtr< idEntity >, MAX_PLAYER_BIN_SIZE >	entityBin;
	idStaticList< idEntityPtr< idEntity >, MAX_PLAYER_BIN_SIZE >	abilityEntityBin;

	// model based sights
	float sightFOV;
	rvClientEntityPtr< sdClientAnimated > sight;

	taskHandle_t					lookAtTask;

#ifdef PLAYER_DAMAGE_LOG
	struct damageLogInfo_t {
		idVec3				location;
		int					time;
		int					damage;
		const sdDeclDamage*	damageDecl;
	};
	class sdDamageLogNumbers : public sdHardcodedParticleSystem {
	public:
		static const int	MAX_CHARS = 256;

		void				Init( idPlayer* player );
		void				Update( idPlayer* player );
		srfTriangles_t *	GetTriSurf( void ) { return renderEntity.hModel->Surface( 0 )->geometry; }
	};
	struct damageLog_t {
		idList< damageLogInfo_t >	log;
		sdDamageLogNumbers			numbers;
	};
	damageLog_t				damageLog;

	const damageLog_t&		GetDamageLog( void ) { return damageLog; }

	bool					sendingVoice;
#endif // PLAYER_DAMAGE_LOG

public:
	virtual sdInteractiveInterface* GetInteractiveInterface( void ) { return &interactiveInterface; }

	virtual void			OnTeleportStarted( sdTeleporter* teleporter );
	virtual void			OnTeleportFinished( void );
	virtual bool			AllowTeleport( void ) const { return gameLocal.time > nextTeleportTime; }

	void					OnFireTeamJoined( sdFireTeam* newFireTeam );
	void					OnFireTeamDisbanded( void );
	void					OnBecomeFireTeamLeader( void );

	void					StopFiring( void );
	void					FireWeapon( void );
	bool					CanAttack( void );
	void					Weapon_Proxy( void );
	void					Weapon_Combat( void );
	void					Weapon_GUI( void );
	void					UpdateWeapon( void );
	void					UpdateSpectating( const usercmd_t& oldCmd );
	void					SpectateFreeFly( bool force );	// ignore the timeout to force when followed spec is no longer valid
	void					SpectateCycle( bool force = false, bool reverse = false );
	void					SpectateCrosshair( bool force = false );
	idPlayer*				GetNextSpectateClient( bool reverse = false ) const;
	idPlayer*				GetPrevSpectateClient( void ) const;

	typedef enum {
		SPECTATE_INVALID = -1,
		SPECTATE_NEXT = 0,
		SPECTATE_PREV,
		SPECTATE_OBJECTIVE,
		SPECTATE_POSITION,
		SPECTATE_MAX
	} spectateCommand_t;

	void					SpectateObjective( void );
	void					SpectateCommand( spectateCommand_t command, const idVec3& origin, const idAngles& angles );

	void					BobCycle( const idVec3 &pushVelocity );
	void					UpdateViewAngles( void );
	void					EvaluateControls( const usercmd_t& oldCmd );
	void					AdjustSpeed( void );
	void					AdjustBodyAngles( void );
	void					Move( const usercmd_t &usercmd, const idAngles &viewAngles );
	void					SetSpectateOrigin( void );

	void					SetStroyBombState( idEntity* bomb );

	void					ClearFocus( void );
	void					UpdateFocus( void );
	bool					InhibitGuiFocus( void );
	guiHandle_t				ActiveGui( void );

	void					ServerUseObject( void );

	bool					IsProne( void ) const;
	bool					IsCrouching( void ) const;
	bool					InhibitProne( void ) const;
	bool					InhibitSprint( void ) const;

	const idDeclSkin*		GetViewSkin( void ) const { return viewSkin; }

	void					ConsumeHealth( void );
	void					ConsumeStroyent( void );

	float					GetSightFOV( void ) const { return sightFOV; }
	sdClientAnimated		*GetSight( void ) { return sight.GetEntity(); }

	void					Event_GetButton( int key );
	void					Event_GetMove( void );
	void					Event_GetUserCmdAngles( void );
	void					Event_GetViewAngles( void );
	void					Event_SetViewAngles( const idVec3& angles );
	void					Event_GetCameraViewAngles( void );
	void					Event_GetRenderViewAngles( void );
	void					Event_GetViewOrigin( void );
	void					Event_GetWeaponEntity( void );
	
	void					Event_IsGunHidden( void );

	void					Event_CreateIcon( const char* materialName, int priority, float timeout );
	void					Event_FreeIcon( qhandle_t handle );

	void					Event_Freeze( bool freeze );
	void					Event_FreezeTurning( bool freeze );
	void					Event_SetRemoteCamera( idEntity* other );
	void					Event_SetRemoteCameraViewOffset( const idVec3& offset );
	void					Event_GetViewingEntity( void );
	void					Event_SetSkin( const char *skinname );
	void					Event_GiveProficiency( int index, float scale, idScriptObject* object, const char* reason );
	void					Event_GiveClassProficiency( float count, const char* reason );
	void					Event_Heal( int count );
	void					Event_Unheal( int count );
	void					Event_MakeInvulnerable( float len );
	void					Event_GiveClass( const char* classname );
	void					Event_GivePackage( const char* packageName );
	void					Event_SetClassOption( int optionIndex, int itemIndex );
	void					Event_SendToolTip( int toolTipIndex );
	void					Event_CancelToolTips( void );
	void					Event_SetWeapon( const char* weaponName, bool instant );
	void					Event_SelectBestWeapon( bool allowCurrent );
	void					Event_GetCurrentWeapon( void );
	void					Event_HasWeapon( const char* weaponName );
	void					Event_GetVehicle( void );
	void					Event_DeployOptionTitle( int index );
	void					Event_NumDeployOptions( void );
	void					Event_GetAmmoFraction( void );
	void					Event_GetUserName( void );
	void					Event_GetCleanUserName( void );
	void					Event_GetClassName( void );
	void					Event_GetCachedClassName( void );
	void					Event_GetPlayerClass( void );
	void					Event_GetShortRank( void );
	void					Event_GetRank( void );
	void					Event_GetProficiencyLevel( int index );
	void					Event_GetXP( int index, bool base );
	void					Event_GetMaxHealth( void );
	void					Event_GetCrosshairEntity( void );
	void					Event_GetCrosshairDistance( bool needValidInfo );
	void					Event_GetCrosshairEndPos( void );
	void					Event_GetCrosshairStartTime( void );
	void					Event_DeploymentMode( void );
	void					Event_ExitDeploymentMode( void );
	void					Event_GetDeployResult( int deployObjectIndex );
	void					Event_GetDeploymentActive( void );
	void					Event_SetSpawnPoint( idEntity* other );
	void					Event_GetSpawnPoint( void );
	void					Event_SetSpeedModifier( float value );
	void					Event_SetSprintScale( float value );
	void					Event_GetEnemy( void );
	void					Event_NeedsRevive( void );
	void					Event_IsInvulernable( void );
	void					Event_Revive( idEntity* other, float healthScale );
	void					Event_SetProxyEntity( idEntity* proxy, int positionId );
	void					Event_GetProxyEntity( void );
	void					Event_GetProxyAllowWeapon( void );
	void					Event_SetSniperAOR( bool enabled );
	void					Event_Enter( idEntity* ent );
	void					Event_GetKilledTime( void );
	void					Event_ForceRespawn( void );
	void					Event_GetClassKey( const char* key );
	void					Event_SetAmmo( ammoType_t ammoType, int amount );
	void					Event_GetAmmo( ammoType_t ammoType );
	void					Event_SetMaxAmmo( ammoType_t ammoType, int amount );
	void					Event_GetMaxAmmo( ammoType_t ammoType );
	void					Event_SetTargetTimeScale( float scale );
	void					Event_DisableShadows( bool value );
	void					Event_GetRemoteCamera( void );

	void					Event_DisableSprint( bool disable );
	void					Event_DisableRun( bool disable );
	void					Event_DisableFootsteps( bool disable );
	void					Event_DisableFallingDamage( bool disable );

	void					Event_Disguise( idEntity* other );
	void					Event_GetDisguiseClient( void );
	void					Event_IsDisguised( void );
	void					Event_SetSpectateClient( idEntity* other );
	void					Event_SetViewSkin( const char *name );
	void					Event_GetViewSkin( void );
	
	void					Event_SetGUIClipIndex( int index );

	void					Event_GetDeploymentRequest( void );
	void					Event_GetDeploymentPosition( void );

	void					Event_Hide( void );
	void					Event_Show( void );

	void					Event_GetActiveTask( void );

	void					BinAddEntity( idEntity* other );
	void					BinRemoveEntity( idEntity* other );

	void					Event_BinAdd( idEntity* other );
	void					Event_BinGet( int index );
	void					Event_BinGetSize( void );
	void					Event_BinRemove( idEntity* other );
	void					BinCleanup();
	idEntity*				BinFindEntityOfType( const idDeclEntityDef* type );

	void					Event_SetArmor( float _armor );
	void					Event_GetArmor( void );

	void					Event_InhibitGuis( bool inhibit );

	void					Event_GetPostArmFindBestWeapon( void );

	void					Event_SameFireTeam( idEntity* other );
	void					Event_IsFireTeamLeader();

	void					Event_IsLocalPlayer( void );
	void					Event_IsToolTipPlaying( void );
	void					Event_IsSinglePlayerToolTipPlaying( void );

	void					Event_SetPlayerChargeOrigin( idEntity *self );
	void					Event_SetPlayerChargeArmed( bool chargeArmed, idEntity *self );
	void					Event_SetPlayerItemState( idEntity *self, bool destroy );
	void					Event_SetPlayerGrenadeState( idEntity *self, bool destroy );
	void					Event_SetPlayerMineState( idEntity *self, bool destroy, bool spotted );
	void					Event_SetPlayerMineArmed( bool chargeArmed, idEntity *self, bool isVisible );
	void					Event_SetPlayerSupplyCrateState( idEntity *self, bool destroy );
	void					Event_SetPlayerAirStrikeState( idEntity *self, bool destroy, bool strikeOnWay );
	void					Event_SetPlayerCovertToolState( idEntity *self, bool destroy );
	void					Event_SetPlayerSpawnHostState( idEntity *self, bool destroy );
	void					Event_SetSmokeNadeState( idEntity *self );
	void					Event_SetArtyAttackLocation( const idVec3& vector, int artyType );
	void					Event_SetPlayerKillTarget( idEntity* killTarget );
	void					Event_SetPlayerRepairTarget( idEntity* repairTarget );
	void					Event_SetPlayerPickupRequestTime( idEntity* pickUpTarget );
	void					Event_SetPlayerCommandRequestTime( void );
	void					Event_SetTeleporterState( bool destroy );
	void					Event_SetForceShieldState( bool destroy, idEntity* self );
	void					Event_SetRepairDroneState( bool destroy );
	void					Event_IsBot();
	void					Event_SetBotEscort( idEntity* botEscort );
	void					Event_SetPlayerSpawnHostTarget( idEntity* spawnHostTarget );
	
	void					Event_ResetTargetLock( void );
	void					Event_IsLocking( void );

	void					Event_EnableClientModelSights( const char* name );
	void					Event_DisableClientModelSights( void );

	void					Event_GetCrosshairTitle( bool allowDisguise );

	void					Event_GetTeamDamageDone( void );
	void					Event_SetTeamDamageDone( int value );

	void					Event_AdjustDeathYaw( float value );

	void					Event_SetCarryingObjective( bool isCarrying );
	void					Event_AddDamageEvent( int time, float angle, float damage, bool updateDirection );
	void					Event_IsInLimbo( void );
};

#endif /* !__GAME_PLAYER_H__ */
