// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __GAME_VEHICLES_TRANSPORT_H__
#define __GAME_VEHICLES_TRANSPORT_H__

#define UPStoKPH( value ) ( InchesToMetres( value ) * 3.6f )
#define KPHtoUPS( value ) ( MetresToInches( value ) / 3.6f )

#include "../AF.h"
#include "../Player.h"
#include "TransportSystems.h"
#include "TransportExtras.h"
#include "../ScriptEntity.h"
#include "../physics/Physics_RigidBodyMultiple.h"
#include "../physics/Physics_AF.h"
#include "../proficiency/ProficiencyManager.h"
#include "../effects/Effects.h"
#include "../effects/WaterEffects.h"
#include "RouteConstraint.h"

class sdVehicleWeapon;
class sdVehicleIKSystem;
class idAFBody;
class idIK;
class sdDeclToolTip;
class sdVehiclePosition;
class sdVehicleView;
class sdTransport;
class sdVehicleControlBase;
class sdVehicleSoundControlBase;
class rvClientMoveable;
class sdBindContext;

typedef sdVehicleWeapon* sdVehicleWeaponPtr;

const int PFC_SELF_COLLISION	= BITT< 0 >::VALUE;
const int PFC_CAN_DAMAGE		= BITT< 1 >::VALUE;

class sdVehicleDriveObject;

#define MAX_VEHICLE_PARTS				32
#define MAX_VEHICLE_WEAPONS				8
#define MAX_VEHICLE_POSITIONS			8
#define MAX_VEHICLE_EFFECTS				32

typedef idStaticList< sdVehicleDriveObject*, MAX_VEHICLE_PARTS >	vehicleDriveObjectList_t;
typedef idStaticList< sdEffect, MAX_VEHICLE_EFFECTS >				vehicleEffectList_t;


class sdTransportUsableInterface : public sdUsableInterface, public sdScriptedInteractiveInterface {
public:
										sdTransportUsableInterface( void ) { owner = NULL; }

	void								Init( sdTransport* transport );

	virtual void						ForcePlacement( idPlayer* other, int index, int oldIndex, bool keepCamera );
	virtual void						FindPositionForPlayer( idPlayer* player );
	virtual bool						OnExit( idPlayer* player, bool force );
	virtual bool						OnActivate( idPlayer* player, float distance );
	virtual bool						OnActivateHeld( idPlayer* player, float distance );
	virtual bool						OnUsed( idPlayer* player, float distance );

	virtual void						BecomeActiveViewProxy( idPlayer* viewPlayer );
	virtual void						StopActiveViewProxy( void );
	virtual void						UpdateProxyView( idPlayer* viewPlayer );

	virtual void						SwapPosition( idPlayer* player );
	virtual void						SwapViewMode( idPlayer* player );
	virtual void						SelectWeapon( idPlayer* player, int index );

	virtual idPlayer*					GetXPSharer( float& shareFactor );

	virtual bool						GetHideHud( idPlayer* player ) const;
	virtual bool						GetSensitivity( idPlayer* player, float& sensX, float& sensY ) const;
	virtual bool						GetShowPlayer( idPlayer* player ) const;
	virtual bool						GetShowPlayerShadow( idPlayer* player ) const;
	virtual float						GetFov( idPlayer* player ) const;
	virtual float						GetDamageScale( idPlayer* player ) const;
	virtual bool						GetShowCrosshair( idPlayer* player ) const;
	virtual bool						GetHideDecoyInfo( idPlayer* player ) const;
	virtual bool						GetShowTargetingInfo( idPlayer* player ) const;
	virtual playerStance_t				GetPlayerStance( idPlayer* player ) const;
	virtual jointHandle_t				GetPlayerIconJoint( idPlayer* player ) const;
	
	virtual void						UpdateHud( idPlayer* player, guiHandle_t handle );
	virtual void						UpdateViewAngles( idPlayer* player );
	virtual void						UpdateViewPos( idPlayer* player, idVec3& origin, idMat3& axis, bool fullUpdate );
	virtual void						CalculateRenderView( idPlayer* player, renderView_t& renderView );
	virtual const idAngles				GetRequiredViewAngles( idPlayer* player, idVec3& target );

	virtual const sdWeaponLockInfo*		GetWeaponLockInfo( idPlayer* player ) const;
	virtual void						ClampViewAngles( idPlayer* player, const idAngles& oldViewAngles ) const;

	virtual void						NextWeapon( idPlayer* player ) const;
	virtual void						PrevWeapon( idPlayer* player ) const;

	virtual int							GetDestructionEndTime( void ) const;
	virtual bool						GetDirectionWarning( void ) const;
	virtual int							GetRouteKickDistance( void ) const;

	virtual const sdDeclGUI*			GetOverlayGUI( void ) const;
	virtual bool						HasAbility( qhandle_t handle, const idPlayer* player ) const;

	virtual bool						GetAllowPlayerMove( idPlayer* player ) const;
	virtual bool						GetAllowPlayerDamage( idPlayer* player ) const;
	virtual bool						GetAllowPlayerWeapon( idPlayer* player ) const;
	virtual bool						GetAllowAdjustBodyAngles( idPlayer* player ) const;

	virtual const char*					GetWeaponLookupName( const idPlayer* player ) const;
	virtual const sdDeclLocStr*			GetWeaponName( const idPlayer* player ) const;
	virtual void						OnPostThink( void ) {}

	virtual int							GetNumPositions() const;
	virtual idPlayer*					GetPlayerAtPosition( int i ) const;
	virtual const sdDeclLocStr*			GetPositionTitle( int i ) const;

public:
	sdTransport*						owner;
	const sdProgram::sdFunction*		onActivateFunc;
	const sdProgram::sdFunction*		onActivateHeldFunc;

	float								maxEnterDistance;
};

class sdTransportBroadcastData : public sdScriptEntityBroadcastData {
public:
										sdTransportBroadcastData( void ) : controlState( NULL ) { ; }
	virtual								~sdTransportBroadcastData( void );

	virtual void						MakeDefault( void );

	virtual void						Write( idFile* file ) const;
	virtual void						Read( idFile* file );

	idStaticList< int, MAX_VEHICLE_POSITIONS >	positionWeapons;

	bool								teleporting;
	bool								modelDisabled;
	bool								deathThroes;
	int									careenStartTime;
	int									lastRepairedPart;
	int									lastDamageDir;
	int									empTime;
	int									weaponEmpTime;
	bool								weaponDisabled;

	idStaticList< sdEntityStateNetworkData*, MAX_VEHICLE_PARTS >	objectStates;
	sdEntityStateNetworkData*			controlState;
};

class sdTransportNetworkData : public sdScriptEntityNetworkData {
public:
										sdTransportNetworkData( void ) : controlState( NULL ) { ; }
	virtual								~sdTransportNetworkData( void );

	virtual void						MakeDefault( void );

	virtual void						Write( idFile* file ) const;
	virtual void						Read( idFile* file );

	idStaticList< sdEntityStateNetworkData*, MAX_VEHICLE_PARTS >	objectStates;
	sdEntityStateNetworkData*			controlState;

	int									routeKickDistance;
};

class sdTransport : public sdScriptEntity {
public:
	ABSTRACT_PROTOTYPE( sdTransport );

	enum {
		EVENT_SETTELEPORTER = sdScriptEntity::EVENT_MAXEVENTS,
		EVENT_CONTROLMESSAGE,
		EVENT_PARTSTATE,
		EVENT_DECAY,
		EVENT_ROUTEWARNING,
		EVENT_ROUTEMASKWARNING,
		EVENT_ROUTETRACKERENTITY,
		EVENT_MAXEVENTS
	};

										sdTransport( void );
	virtual								~sdTransport( void );

	sdTransportPositionManager&			GetPositionManager( void )			{ return positionManager; }
	const sdTransportPositionManager&	GetPositionManager( void ) const	{ return positionManager; }

	virtual void						Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const sdDeclDamage* damage, const float damageScale, const trace_t* collision, bool forceKill = false );
	virtual bool						Pain( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location, const sdDeclDamage* damageDecl );
	virtual void						Decayed( void );

	virtual void						SetOrigin( const idVec3 &org );
	virtual void						SetAxis( const idMat3 &org );

	virtual bool						StartSynced( void ) const { return true; }

	const wchar_t*						GetPassengerNames( void ) const;

	virtual bool						ClientReceiveEvent( int event, int time, const idBitMsg& msg );

	virtual void						OnTeleportStarted( sdTeleporter* teleporter );
	virtual void						OnTeleportFinished( void );
	virtual bool						AllowTeleport( void ) const { return gameLocal.time > nextTeleportTime; }

	virtual void						OnPhysicsRested( void );

	void								SetTrackerEntity( idEntity* trackerEntity );

	void								Event_SetArmor( float _armor );
	void								Event_GetArmor( void );

	virtual sdBindContext*				GetBindContext( void ) { return bindContext; }
	virtual void						OnInputInit( void );
	virtual void						OnInputShutdown( void );

	int									GetDestructionEndTime( void ) const { return routeMaskWarningEndTime; }
	bool								GetDirectionWarning( void ) const { return vehicleFlags.routeWarning; }
	int									GetRouteKickDistance( void ) const { return routeKickDistance; }

	int									BitsForPartIndex( void ) const { return idMath::BitsForInteger( driveObjects.Num() + 1 ); }

	virtual void						WriteDemoBaseData( idFile* file ) const;
	virtual void						ReadDemoBaseData( idFile* file );

	void								Event_Repair( int repairCount );
	void								Event_GetLastRepairOrigin( void );
	void								Event_Input_SetPlayer( idEntity* entity );
	void								Event_Input_GetCollective();
	void								Event_GetLandingGearDown();
	void								Event_GetNumOccupiedPositions( void );
	void								Event_SwapPosition( idEntity* player );
	void								Event_IsEmpty( void );
	void								Event_UpdateEngine( bool disabled );
	void								Event_SetLightsEnabled( int group, bool enabled );
	void								Event_DisableWheelSuspension( bool disable );
	void								Event_EnableKnockback();
	void								Event_DisableKnockback();
	void								Event_DisableTimeout( bool disable );
	void								Event_GetSteerAngle( void );

	void								Event_GetPassengerNames( void );
	void								Event_Lock( bool _locked );
	void								Event_KickPlayer( int index, int flags );
	void								Event_Freeze( bool freeze );

	void								Event_DestroyParts( int mask );
	void								Event_DecayParts( int mask );

	void								Event_DecayLeftWheels( void );
	void								Event_DecayRightWheels( void );
	void								Event_DecayNonWheels( void );

	void								Event_ResetDecayTime( void );

	void								Event_HasHiddenParts( void );
	void								Event_SelectWeapon( idEntity* other, const char* name );

	void								Event_GetObject( const char* objectName );

	void								Event_DisableModel( bool disabled );
	
	void								Event_SetDeathThroeHealth( int health );
	void								Event_GetMinDisplayHealth( void );

	void								Event_GetSurfaceType( void );

	void								Event_IsEMPed( void );
	void								Event_IsWeaponEMPed( void );
	void								Event_GetRemainingEMP( void );
	void								Event_ApplyEMPDamage( float time, float weaponTime );

	void								Event_IsWeaponDisabled( void );
	void								Event_SetWeaponDisabled( bool disabled );
	bool								IsWeaponDisabled( void ) const;

	void								Event_SetLockAlarmActive( bool active );
	void								Event_SetImmobilized( bool immobile );
	void								Event_SetActive( bool active );

	void								Event_InSiegeMode( void );

	void								Event_GetNumWeapons( void );
	void								Event_GetWeapon( int index );

	void								Event_SetTrackerEntity( idEntity* trackerEntity );
	void								Event_IsPlayerBanned( idEntity* entity );
	void								Event_BanPlayer( idEntity* entity, float time );

	void								Event_ClearLastAttacker();
	
	void								Event_EMPChanged( void );
	void								Event_WeaponEMPChanged( void );

	void								Event_DisablePart( const char* name );
	void								Event_EnablePart( const char* name );

	void								Event_DestructionTime();
	void								Event_DirectionWarning();

	void								Event_IsTeleporting( void );

	virtual int							GetMinDisplayHealth( void ) const;

	const playerVehicleTypes_t&			GetVehicleType( void ) const { return playerVehicleType; }
	const playerTeamTypes_t&			GetVehicleTeam( void ) const { return playerVehicleTeam; }
	int									GetVehicleFlags( void ) const { return playerVehicleFlags; }
	bool								HasLockonDanger( void );
	bool								IsDeployed( void );
	int									GetDestroyedCriticalDriveParts() const { return damagedCriticalDriveParts; }
	void								RemoveCriticalDrivePart() { damagedCriticalDriveParts++; }
	void								AddCriticalDrivePart() { damagedCriticalDriveParts--; }
	int									GetRouteActionNumber() const { return routeActionNumber; }

	bool								IsEMPed( void );
	bool								IsWeaponEMPed( void );
	void								SetEMPTime( int newTime, int newWeaponTime );
	void								OnEMPStateChanged( void );
	void								OnWeaponEMPStateChanged( void );
	float								GetRemainingEMP( void );
	float								GetRemainingWeaponEMP( void );

	void								DestroyParts( int mask );
	void								DecayParts( int mask );
	void								DecayLeftWheels( void );
	void								DecayRightWheels( void );
	void								DecayNonWheels( void );
	bool								CanUse( idPlayer *other, const sdDeclToolTip** tip );
	void								OnUsed( idPlayer *other );
	void								ForcePlacement( idPlayer* other, int index, int oldIndex, bool keepCamera );
	virtual void						FindPositionForPlayer( idPlayer* player );
	void								PlacePlayerInPosition( idPlayer *other, sdVehiclePosition &position, sdVehiclePosition* oldPosition, bool keepCamera );
	bool								Exit( idPlayer* player, bool force );
	void								CancelDriverBackupVote( idPlayer* player );
	void								CancelAllDriverBackupVotes();

	virtual void						OnPlayerEntered( idPlayer* player, int position, int oldPosition );
	virtual void						OnPlayerExited( idPlayer* player, int position );

	const sdDeclGUI*					GetOverlayGUI( void ) const { return overlayGUI; }

	void								ClearDriveObjects( void );
	void								AddDriveObject( sdVehicleDriveObject* part );

	bool								IsTeleporting( void ) const { return teleportEntity.IsValid(); }
	bool								IsAmphibious( void ) const { return vehicleFlags.amphibious; }
	bool								IsLocked( void ) const { return vehicleFlags.locked; }
	bool								IsFlipped( void );
	void								AutoRight( idPlayer* other );
	void								SelectWeapon( idPlayer* player, int index );

	void								UpdateEngine( bool disabled );
	void								RunObjectPrePhysics( void );
	void								RunObjectPostPhysics( void );

	virtual void						UpdateVisibility( void );

	virtual void						OnUpdateVisuals( void );

	virtual void						SetDefaultMode( sdVehiclePosition& position, idPlayer* other );
	void								LoadVehicleScript( void );
	virtual void						DoLoadVehicleScript( void ) = 0;
	void								ParseVehicleScript( void );
	virtual void						FreezePhysics( bool freeze ) = 0;

	virtual	void						SetLastAttacker( idEntity *attacker, const sdDeclDamage* damage ) { lastAttacker = attacker; lastDamage = damage; }
	idEntity*							GetLastAttacker( void ) const { return lastAttacker; }
	const sdDeclDamage*					GetLastDamage( void ) const { return lastDamage; }

	idPlayer*							GetLastOccupant( void ) const { return lastOccupant; }

	const sdDeclDamage*					GetKillPlayerDamage( void ) const { return killPlayerDamage; }

	void								PrevWeapon( idPlayer* player );
	void								NextWeapon( idPlayer* player );

	sdVehicleWeapon*					GetActiveWeapon( idPlayer* player );

	virtual void						ApplyNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState );
	virtual void						ReadNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const;
	virtual void						WriteNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const;
	virtual bool						CheckNetworkStateChanges( networkStateMode_t mode, const sdEntityStateNetworkData& baseState ) const;
	virtual sdEntityStateNetworkData*	CreateNetworkStructure( networkStateMode_t mode ) const;
	virtual sdTransportNetworkData*		CreateTransportNetworkStructure( void ) const;
	virtual void						ResetNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState );
	virtual void						WriteInitialReliableMessages( const sdReliableMessageClientInfoBase& target ) const;

	void								SendFullPartStates( const sdReliableMessageClientInfoBase& target ) const;

	virtual void						UpdateHud( idPlayer* player, guiHandle_t handle );

	virtual void						UpdateViewAngles( idPlayer* player );
	virtual void						ClampViewAngles( idPlayer* player, const idAngles& oldAngles );

	virtual idIK*						GetIK( void ) { return NULL; }

	virtual void						LoadParts( int partTypes ) = 0;


	virtual bool						DoRadiusPush( void ) const { return !vehicleFlags.disableKnockback; }

	virtual bool						WantsToThink( void );

	virtual void						Think( void );
	virtual void						PostThink( void );
	virtual idLinkList<idEntity>*		GetPostThinkNode( void );
	void								Spawn( void );
	virtual void						PostMapSpawn( void );

										// jrad - fill a list with sdEffects specific to each type of surfacetype
										// effectname should be of the form "fx_prefix_<surfacetypename>"
	void								InitEffectList( vehicleEffectList_t& list, const char* effectName, int& numSurfaceTypes );

	virtual void						CheckWater( const idVec3& waterBodyOrg, const idMat3& waterBodyAxis, idCollisionModel* waterBodyModel );

	void								SetupCockpit( idPlayer* viewPlayer );

	static bool							CheckVisibility( renderEntity_t* renderEntity, const renderView_t* renderView );
	static void							ReloadVehicleScripts( idDecl* decl );

	vehicleDriveObjectList_t&			GetDriveObjects( void ) { return driveObjects; }
	const vehicleDriveObjectList_t&		GetDriveObjects( void ) const { return driveObjects; }
	sdVehicleDriveObject*				GetDriveObject( const char* objectName ) const;

	void								DisableWheelSuspension( bool disable );

	sdVehicleDriveObject*				PartForCollisionByPosition( const trace_t& collision, int flags ) const;
	sdVehicleDriveObject*				PartForCollisionById( const trace_t& collision, int flags ) const;
	sdVehicleDriveObject*				PartForCollision( const trace_t& collision, int flags ) const;

	int									Repair( int repair );
	virtual bool						NeedsRepair();

	void								SelfDestruct( void );

	const sdVehicleControlBase*			GetVehicleControl( void ) const { return vehicleControl; }
	sdVehicleControlBase*				GetVehicleControl( void ) { return vehicleControl; }

	bool								IsModelDisabled( void ) { return vehicleFlags.modelDisabled; }
	bool								CycleAllPositions( void ) { return vehicleFlags.cycleAllPositions; }
	void								DisableModel( bool hide );

	virtual int							EvaluateContacts( contactInfo_t* list, contactInfoExt_t* extList, int max );
	virtual int							AddCustomConstraints( constraintInfo_t* list, int max );

	int									PrevWeaponIndex( idPlayer* player, int index );
	int									NextWeaponIndex( idPlayer* player, int index );
	sdVehicleWeapon*					GetWeapon( const char* name );
	sdVehicleWeapon*					GetWeapon( int index ) { return weapons[ index ]; }
	int									GetWeaponIndex( const char* name );
	void								AddWeapon( sdVehicleWeapon* weapon ) { weapons.Append( weapon ); }
	int									NumWeapons( void ) const { return weapons.Num(); }
	void								SortWeapons( void );

	sdVehicleView&						GetViewForPlayer( const idPlayer* player );
	void								UpdateViews( sdVehicleWeapon* weapon );

	const idMaterial*					GetAttitudeMaterial( void ) const { return attitudeMaterial; }

	const sdDeclVehicleScript*			GetVehicleScript() const { return vehicleScript; }
	const sdTransportEngine&			GetEngine() const { return engine; }

	const sdVehicleInput&				GetInput( void ) const { return input; }

	float								GetSteerVisualAngle( void ) const { return steerVisualAngle; }
	void								SetSteerVisualAngle( float angle ) { steerVisualAngle = angle; }

	virtual sdUsableInterface*			GetUsableInterface( void ) { return &usableInterface; }
	virtual sdInteractiveInterface*		GetInteractiveInterface( void ) { return &usableInterface; }

	virtual void						UpdateModelTransform( void );

	void								UpdatePlayZoneInfo( void );
	void								UpdateDecay( void );

	void								SetInteriorSound( idPlayer* player );

	virtual bool						ShowFirstPersonPlayer() { return false; }
	virtual bool						UnbindOnEject() { return true; }
	virtual bool						GetAllowPlayerMove( idPlayer* player ) const { return true; }

	void								CheckFlipped( void );

	bool								InDeathThroes() const { return vehicleFlags.deathThroes; }
	void								SetDeathThroes( bool enabled );
	bool								IsCareening() const { return careenStartTime > 0; }
	int									GetCareeningTime() const { return gameLocal.time - careenStartTime; }
	void								SetCareening( int startTime );

	void								RemoveActiveDriveObject( sdVehicleDriveObject* object );
	void								AddActiveDriveObject( sdVehicleDriveObject* object );

	void								SetLightsEnabled( int group, bool enabled );
	virtual void						SetDamageDealtScale( float scale ) {}

	virtual cheapDecalUsage_t			GetDecalUsage( void );

	virtual void						SetTeleportEntity( sdTeleporter* teleporter );
	sdTeleporter*						GetTeleportEntity( void ) { return teleportEntity; }

	virtual int							PlayHitBeep( idPlayer* player, bool headshot ) const;

	virtual bool						OnKeyMove( char forward, char right, char up, usercmd_t& cmd );
	virtual void						OnControllerMove( bool doGameCallback, const int numControllers, const int* controllerNumbers,
												const float** controllerAxis, idVec3& viewAngles, usercmd_t& cmd );

	void								SetMasterDestroyedPart( rvClientMoveable* part );
	rvClientMoveable*					GetMasterDestroyedPart( void );

	bool								IsInPlayzone( void ) const { return vehicleFlags.inPlayZone; }

	void								SetRouteWarning( bool value );
	void								SetRouteMaskWarning( bool value );

	virtual float						GetDamageXPScale( void ) const { return damageXPScale; }

	bool								AreTracksOnGround( void ) const;
	float								AreWheelsOnGround( void ) const;

protected:
	virtual bool						OnApplyPain( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location, const sdDeclDamage* damageDecl );
	virtual void						BindPlayerToPosition( idPlayer *other, sdVehiclePosition &position );

	void								SendPartState( sdVehicleDriveObject* object ) const;

	void								UpdateWaterDamage();
	void								UpdateLockAlarm();
	virtual void						UpdateScrapeEffects( void ) {}
	void								UpdateShaderParms( void );
	
	sdTransportPositionManager			positionManager;
	vehicleDriveObjectList_t			driveObjects;
	vehicleDriveObjectList_t			activeObjects;
	vehicleDriveObjectList_t			physicsObjects;
	vehicleDriveObjectList_t			networkObjects;
	int									lastEnteredTime;
	int									lastOccupiedTime;
	idEntityPtr< idPlayer >				lastOccupant;
	int									decayTime;
	int									decayDistance;
	float								steerVisualAngle;
	float								lastSpeed;

	sdBindContext*						bindContext;

	idEntityPtr< sdTeleporter >			teleportEntity;
	int									nextTeleportTime;

	const sdDeclSurfaceType*			surfaceType;

	int									nextFlippedDamageTime;
	int									flippedTime;
	int									lastMovedTime;

	const sdDeclVehicleScript*			vehicleScript;

	const sdDeclDamage*					killPlayerDamage;
	const sdDeclDamage*					flippedDamage;

	float								damageXPScale;
	
	sdVehicleInput						input;
	const sdProgram::sdFunction*		weaponSelectedFunction;
	const sdProgram::sdFunction*		surfaceTypeChangedFunction;

	const sdDeclGUI*					overlayGUI;

	struct vehicleFlags_t {
		bool							modelDisabled		: 1;
		bool							locked				: 1;
		bool							timeoutEnabled		: 1;
		bool							tryKeepDriver		: 1;
		bool							decaying			: 1;
		bool							disableKnockback	: 1;
		bool							showedDecayTooltip	: 1;
		bool							deathThroes			: 1;
		bool							inPlayZone			: 1;
		bool							lastDamageDirValid	: 1;
		bool							lockAlarmActive		: 1;
		bool							cycleAllPositions	: 1;
		bool							routeWarning		: 1;
		bool							routeWarningTimeout	: 1;
		bool							disablePartStateUpdate : 1;
		bool							weaponDisabled		: 1;

		// these are more like type-specific data, but this is a handy location to store them
		bool							amphibious			: 1;
		bool							decals				: 1;
	};

	vehicleFlags_t						vehicleFlags;

	const sdDeclToolTip*				toolTipEnter;
	const sdDeclToolTip*				toolTipAbandon;
	const idMaterial*					attitudeMaterial;

	playerVehicleTypes_t				playerVehicleType;
	playerTeamTypes_t					playerVehicleTeam;
	int									playerVehicleFlags;

	sdTransportEngine					engine;
	sdVehicleLightSystem				lights;

	sdTransportUsableInterface			usableInterface;
	sdVehicleControlBase*				vehicleControl;
	sdVehicleSoundControlBase*			vehicleSoundControl;

	idStaticList< sdVehicleWeapon*, MAX_VEHICLE_WEAPONS >	weapons;

	idLinkList< idEntity >				postThinkEntNode;

	idEntityPtr< idEntity >				lastAttacker;
	const sdDeclDamage*					lastDamage;
	idVec3								lastDamageDir;

	const sdProgram::sdFunction*		updateHudFunc;
	const sdProgram::sdFunction*		enemyLockedOnUsFunc;
	const sdProgram::sdFunction*		isDeployedFunc;

	int									routeActionNumber;

	sdWaterEffects*						waterEffects;

	int									lastRepairedPart;

	int									empTime;
	int									weaponEmpTime;
	int									submergeTime;
	int									lastWaterDamageTime;
	const sdDeclDamage*					waterDamageDecl;
	int									lockAlarmTime;
	
	int									careenStartTime;

	int									routeMaskWarningEndTime;
	sdRouteConstraintTracker			routeTracker;

	rvClientEntityPtr< rvClientMoveable >	masterDestroyedPart;

	int									damagedCriticalDriveParts;

	int									routeKickDistance;
};

class sdTransport_AF : public sdTransport {
public:
	ABSTRACT_PROTOTYPE( sdTransport_AF );

										sdTransport_AF( void );
	virtual								~sdTransport_AF( void );

	idPhysics_AF*						GetAFPhysics( void ) { return af.GetPhysics(); }
	const idPhysics_AF*					GetAFPhysics( void ) const { return af.GetPhysics(); }

	virtual bool						UpdateAnimationControllers( void );
	virtual int							BodyId( idAFBody* body ) const { return af.GetPhysics()->GetBodyId( body ); }
	virtual idAFBody*					BodyForName( const char* name ) const { return af.GetPhysics()->GetBody( name ); }
	virtual idAFBody*					BodyForId( int id ) const { return af.GetPhysics()->GetBody( id ); }
	virtual int							BodyForClipModelId( int id ) const { return af.BodyForClipModelId( id ); }
	virtual const idVec3&				GetCenterOfMass( void ) const { return af.GetPhysics()->GetCenterOfMass(); }
	virtual	void						DisableClip( bool activateContacting = true );
	virtual void						EnableClip( void );
	virtual void						LoadParts( int partTypes );

protected:

	idAF								af;
};

class sdTransport_RB : public sdTransport {
public:
	ABSTRACT_PROTOTYPE( sdTransport_RB );

										sdTransport_RB( void );
	virtual								~sdTransport_RB( void );

	void								Spawn( void );
	
	sdPhysics_RigidBodyMultiple*		GetRBPhysics( void ) { return &physicsObj; }
	const sdPhysics_RigidBodyMultiple*	GetRBPhysics( void ) const { return &physicsObj; }

	virtual const idVec3&				GetCenterOfMass( void ) const { return physicsObj.GetMainCenterOfMass(); }

	virtual	void						DisableClip( bool activateContacting = true );
	virtual void						EnableClip( void );
	virtual void						LoadParts( int partTypes );

protected:
	virtual void						UpdateScrapeEffects( void );

	sdPhysics_RigidBodyMultiple			physicsObj;

	float								lightScrapeSpeed;
	float								mediumScrapeSpeed;
	float								heavyScrapeSpeed;

	vehicleEffectList_t					lightScrapeEffects;
	vehicleEffectList_t					mediumScrapeEffects;
	vehicleEffectList_t					heavyScrapeEffects;
	idLinkList< sdEffect >				activeEffects;
};

#endif // __GAME
