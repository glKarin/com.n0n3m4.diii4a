// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_ENTITY_H__
#define __GAME_ENTITY_H__

/*
===============================================================================

	Game entity base class.

===============================================================================
*/

#include "Game.h"
#include "Pvs.h"
#include "physics/Physics_Static.h"
#include "script/Script_Interface.h"
#include "../sound/SoundShader.h"
#include "../sound/SoundEmitter.h"
#include "guis/UserInterfaceTypes.h"

typedef enum radarMasks_e {
	RM_RADAR		= BITT< 0 >::VALUE,
} radarMasks_t;

extern const idEventDefInternal EV_PostSpawn;
extern const idEventDefInternal EV_FindTargets;
extern const idEventDef EV_Use;
extern const idEventDef EV_Activate;
extern const idEventDef EV_Bind;
extern const idEventDef EV_BindToJoint;
extern const idEventDef EV_Unbind;
extern const idEventDef EV_Hide;
extern const idEventDef EV_Show;
extern const idEventDef EV_GetShaderParm;
extern const idEventDef EV_SetShaderParm;
extern const idEventDef EV_GetAngles;
extern const idEventDef EV_SetAngles;
extern const idEventDef EV_SetWorldAxis;
extern const idEventDef EV_SetLinearVelocity;
extern const idEventDef EV_SetAngularVelocity;
extern const idEventDef EV_SetSkin;
extern const idEventDef EV_StartSoundShader;
extern const idEventDef EV_StopSound;
extern const idEventDef EV_StopAllEffects;
extern const idEventDef EV_PlayEffect;
extern const idEventDef EV_StopEffect;
extern const idEventDef EV_StopEffectHandle;
extern const idEventDef EV_KillEffect;
extern const idEventDef EV_PlayOriginEffect;
extern const idEventDef EV_GetMins;
extern const idEventDef EV_GetMaxs;
extern const idEventDef EV_SetOrigin;
extern const idEventDef EV_SetAngles;
extern const idEventDef EV_SetGravity;
extern const idEventDef EV_SetTeam;

extern const idEventDef EV_SetModel;
extern const idEventDef EV_GetLinearVelocity;

extern const idEventDef EV_GetKeyWithDefault;
extern const idEventDef EV_GetIntKeyWithDefault;
extern const idEventDef EV_GetFloatKeyWithDefault;
extern const idEventDef EV_GetVectorKeyWithDefault;

extern const idEventDef EV_GetWorldOrigin;
extern const idEventDef EV_GetWorldAxis;
extern const idEventDef EV_GetOwner;
extern const idEventDef EV_IsOwner;
extern const idEventDef EV_GetDamagePower;

extern const idEventDef EV_PlayMaterialEffect;

extern const idEventDef EV_GetKey;
extern const idEventDef EV_GetIntKey;
extern const idEventDef EV_GetFloatKey;
extern const idEventDef EV_GetVectorKey;
extern const idEventDef EV_GetEntityKey;

extern const idEventDef EV_GetGUI;

extern const idEventDef AI_SetHealth;

extern const idEventDef EV_AddCheapDecal;

typedef enum teamAllegiance_e {
	TA_FRIEND,
	TA_NEUTRAL,
	TA_ENEMY,
} teamAllegiance_t;

const int TA_FLAG_FRIEND	= BITT< 0 >::VALUE;
const int TA_FLAG_NEUTRAL	= BITT< 1 >::VALUE;
const int TA_FLAG_ENEMY		= BITT< 2 >::VALUE;

class idProjectile;
class sdBindContext;
class sdDeclTargetInfo;
class sdDeclAOR;

// Think flags
typedef enum thinkFlags_s {
	TH_ALL					= -1,
	TH_THINK				= 1,		// run think function each frame
	TH_PHYSICS				= 2,		// run physics each frame
	TH_ANIMATE				= 4,		// update animation each frame
	TH_UPDATEVISUALS		= 8,		// update renderEntity
	TH_UPDATEPARTICLES		= 16,
	TH_RUNSCRIPT			= 32,
} thinkFlags_t;

enum cheapDecalUsage_t {
	CDU_INHIBIT,// genau genau die CDU ist gans toll! oder?
	CDU_LOCAL,	// Decails are attached and move around with the render entity
	CDU_WORLD	// Decails go in a big world list (But don't get deleted or moved then the spawner moves)
};

struct renderView_s;
struct renderEntity_t;
struct trace_t;
struct impactInfo_s;
class sdTeamInfo;
class sdScriptHelper;
class sdRequirementContainer;
class rvClientEntity;
class idPlayer;
class idAnimator;
class idPhysics;
class sdDeclDamage;
class sdCrosshairInfo;
class sdCommandMapInfo;
class sdRequirementCheckInterface;
class sdRadarInterface;
class sdGuiInterface;
class sdNetworkInterface;
class sdTaskInterface;
class sdDeclDeployableObject;
class sdUsableInterface;
class sdInteractiveInterface;
class sdEntityDisplayIconInterface;
class sdPlayerTask;
class sdTeleporter;

typedef sdTeamInfo* ( idEntity::*teamFunc_t )( void ) const;

const int PVS_VISIBLE			= BITT< 0 >::VALUE;
const int PVS_BROADCAST			= BITT< 1 >::VALUE;
const int PVS_DEFERRED_VISIBLE	= BITT< 2 >::VALUE;
const int PVS_NUMBITS			= 3;

class sdBindNetworkData {
public:
							sdBindNetworkData( void );

	void					MakeDefault( void );

	void					Write( const idEntity* ent, const sdBindNetworkData& base, idBitMsg& msg );
	void					Read( const idEntity* ent, const sdBindNetworkData& base, const idBitMsg& msg );
	bool					Check( const idEntity* ent ) const;
	void					Apply( idEntity* ent ) const;

	static const int		bindPosBits;

	void					Write( idFile* file ) const;
	void					Read( idFile* file );

	jointHandle_t			joint;
	int						bodyId;
	int						bindMasterId;
	bool					oriented;
};

class sdColorNetworkData {
public:
							sdColorNetworkData( void );

	void					MakeDefault( void );

	void					Write( const idEntity* ent, const sdColorNetworkData& base, idBitMsg& msg );
	void					Read( const idEntity* ent, const sdColorNetworkData& base, const idBitMsg& msg );
	void					Apply( idEntity* ent ) const;
	bool					Check( const idEntity* ent ) const;

	void					Write( idFile* file ) const;
	void					Read( idFile* file );

	static int				GetColor( const idEntity* ent );

	int						color;
};

class idEntity : public idClass {
public:
	static const int		MAX_PROXIMITY_ENTITIES = 128;
	static const int		NUM_ENTITY_CAHCES = 8;

	typedef idStaticList< idEntityPtr< idEntity >, MAX_PROXIMITY_ENTITIES > entityCache_t;

	int						entityNumber;			// index into the entity list
	int						entityDefNumber;		// index into the entity def list
	int						mapSpawnId;				//

	idLinkList<idEntity>	spawnNode;				// for being linked into spawnedEntities list
	idLinkList<idEntity>	activeNode;				// for being linked into activeEntities list
	idLinkList<idEntity>	activeNetworkNode;		// for being linked into activeEntities list
	idLinkList<idEntity>	networkNode;			// for being linked into networkedEntities list

	sdEntityState*			freeStates[ NSM_NUM_MODES ];				//

	idLinkList< idClass >	instanceNode;			//

	idStr					name;					// name of entity
	idDict					spawnArgs;				// key/value pairs used to spawn and initialize entity
	idScriptObject*			scriptObject;			// contains all script defined data for this entity

	int						thinkFlags;				// TH_? flags

	// temp storage during write snapshot
	int						snapshotPVSFlags;		//

	const sdDeclAOR*		aorLayout;
	int						aorFlags;
	float					aorDistanceSqr;			// last distance squared used for AOR calculations
	int						aorPacketSpread;

//	renderView_s*			renderView;				// for camera views from this entity

	idList< idEntityPtr< idEntity > >	targets;	// when this entity is activated these entities entity are activated

	// RAVEN BEGIN
	// bdube: client entities
	idLinkList< rvClientEntity >	clientEntities;

	// RAVEN END

	struct entityFlags_s {
		bool				notarget			:1;	// if true never attack or target this entity
		bool				noknockback			:1;	// if true no knockback from hits
		bool				takedamage			:1;	// if true this entity can be damaged
		bool				hidden				:1;	// if true this entity is not visible
		bool				bindOrientated		:1;	// if true both the master orientation is used for binding

		bool				forceDisableClip	:1; // EnableClip will not re-enable clip if this flag is set

		bool				preventDeployment	:1; // don't allow objects to be deployed on top of this one

		bool				selected			:1; // FIXME: deprecated...

		bool				canCollideWithTeam	:1;

		bool				noCrosshairTrace	:1; // disable crosshair traces against this entity

		bool				spotted				:1;

		bool				unlockInterpolate	:1;	// origin interpolated when running unlock fps mode

		bool				noGuiInteraction	:1;

		bool				dontLink			:1; // never link the clip

		bool				noDamageFeedback	:1; // allow damage feedback if this entity is the inflictor

		bool				allowPredictionErrorDecay	:1;	// allow PED to run this frame

		bool				forceDoorCollision	:1;

		bool				forceDecalUsageLocal :1;
	} fl;

	idLinkList<idEntity>			interpolateNode;			// entities marked for interpolation in fps unlock mode
	idVec3							interpolateHistory[2];		// holds origin for interpolation work
	int								interpolateLastFramenum;	// holds last frame number this entity was picked up for interpolation

	idRandom						missileRandom;	// random number generator for things like bullet spread etc

public:

	ABSTRACT_PROTOTYPE( idEntity );

							idEntity();
	virtual					~idEntity();

	void					Spawn( void );

	virtual bool			StartSynced( void ) const { return spawnArgs.GetBool( "networkSync" ); }

	virtual const char*		GetDefaultSurfaceType( void ) const { return ""; }

	void					WriteCreateEvent( idBitMsg* msg, const sdReliableMessageClientInfoBase& target );
	void					WriteDestroyEvent( void );

	static void				ClearEntityCaches( void );

	static idEntity*		FromCreationData( idFile* file );
	void					WriteCreationData( idFile* file ) const;

	void					LoadDemoBaseData( idFile* file );
	virtual void			ReadDemoBaseData( idFile* file ) { ; }
	virtual void			WriteDemoBaseData( idFile* file ) const { ; }

	virtual bool			NoThink( void ) const { return false; }

	void					FixupRemoteCamera( void );

	void					CallNonBlockingScriptEvent( const sdProgram::sdFunction* function, sdScriptHelper& helper ) const;
	bool					CallBooleanNonBlockingScriptEvent( const sdProgram::sdFunction* function, sdScriptHelper& helper ) const;
	float					CallFloatNonBlockingScriptEvent( const sdProgram::sdFunction* function, sdScriptHelper& helper ) const;

	const char *			GetEntityDefName( void ) const;
	void					SetName( const char *name );
	virtual const char *	GetName( void ) const;
	virtual void			UpdateChangeableSpawnArgs( const idDict *source );

	virtual bool			DoRadiusPush( void ) const { return !fl.noknockback; }
	virtual float			GetRadiusPushScale( void ) const { return 1.0f; }
	virtual void			ApplyRadiusPush( const idVec3& pushOrigin, const idVec3& entityOrigin, const sdDeclDamage* damageDecl, float pushScale, float radius );

	virtual idScriptObject*	GetScriptObject( void ) const { return this ? scriptObject : NULL; }

	// command map
	void					SetSpotted( idEntity* other );
	bool					SendCommandMapInfo( idPlayer* player );
	bool					IsInRadar( idPlayer* player );

	void					SetNetworkSynced( bool value );
	bool					IsNetSynced( void ) const;

	virtual void						ApplyNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState ) { ; }
	virtual void						ReadNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, const idBitMsg& msg ) const { ; }
	virtual void						WriteNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& baseState, sdEntityStateNetworkData& newState, idBitMsg& msg ) const { ; }
	virtual bool						CheckNetworkStateChanges( networkStateMode_t mode, const sdEntityStateNetworkData& baseState ) const { return false; }
	virtual sdEntityStateNetworkData*	CreateNetworkStructure( networkStateMode_t mode ) const { return NULL; }
	// this is called when resetting the state back to the base state before applying a new state and repredicting
	virtual void						ResetNetworkState( networkStateMode_t mode, const sdEntityStateNetworkData& newState ) { ; }

	virtual void			WriteInitialReliableMessages( const sdReliableMessageClientInfoBase& target ) const {};
	virtual void			OnSnapshotHitch() { ; }

	bool					IsInterpolated( void ) const;

							// clients generate views based on all the player specific options,
							// cameras have custom code, and everything else just uses the axis orientation
	virtual renderView_t *	GetRenderView();

	virtual idLinkList< idClass >* GetInstanceNode( void ) { return &instanceNode; }

	virtual bool			HasAbility( qhandle_t handle ) const { return false; }
	virtual bool			SupportsAbilities( void ) const { return false; }

	// thinking
	virtual void			Think( void );
	virtual void					PostThink( void ) { assert( false ); }
	virtual idLinkList<idEntity>*	GetPostThinkNode( void ) { return NULL; }
	bool					IsActive( void ) const;
	void					BecomeActive( int flags, bool force = false );
	void					BecomeInactive( int flags, bool force = false );

	virtual void			DisableClip( bool activateContacting = true );
	virtual void			EnableClip( void );
	virtual void			GetLocalAngles( idAngles& localAng );

	virtual void			SetMaxHealth( int count ) { }
	virtual void			SetHealth( int count ) { }
	virtual void			SetHealthDamaged( int count, teamAllegiance_t allegiance ) { SetHealth( count ); } // Gordon: Specifically from ::Damage, some things may wish to filter this in some way
	virtual int				GetHealth( void ) const { return 0; }
	virtual int				GetMaxHealth( void ) const { return 0; }
	virtual int				GetMinDisplayHealth( void ) const { return 0; }

	virtual void			ReachedPosition( void ) { }
	virtual void			PostMapSpawn( void );

	virtual const idVec3&	GetWayPointOrigin( void ) const { return GetPhysics()->GetOrigin(); }
	virtual const idBounds&	GetWayPointBounds( void ) const { return GetPhysics()->GetBounds(); }
	virtual const idMat3&	GetWayPointAxis( void ) const { return GetPhysics()->GetAxis(); }

	// visuals
	virtual void			OnModelDefCreated( void ) { }
	virtual void			Present( void );
	virtual renderEntity_t* GetRenderEntity( void ) { return &renderEntity; }
	virtual const renderEntity_t* GetRenderEntity( void ) const { return &renderEntity; }
	virtual int				GetModelDefHandle( int id = 0 );
	virtual void			SetModel( const char *modelname );
	void					SetSkin( const idDeclSkin *skin );
	const idDeclSkin *		GetSkin( void ) const;
	void					SetShaderParm( int parmnum, float value );
	virtual void			SetColor( float red, float green, float blue );
	virtual void			SetColor( const idVec3 &color );
	virtual void			GetColor( idVec3 &out ) const;
	virtual void			SetColor( const idVec4 &color );
	virtual void			GetColor( idVec4 &out ) const;
	virtual void			FreeModelDef( void );
	virtual void			FreeLightDef( void );
	virtual void			Hide( void );
	virtual void			Show( void );
	bool					IsHidden( void ) const;
	void					ForceDisableClip( void );
	void					ForceEnableClip( void );
	void					UpdateVisuals( void );
	void					UpdateModel( void );
	virtual void			UpdateModelTransform( void );
	virtual void			OnUpdateVisuals( void );
	virtual int				EvaluateContacts( contactInfo_t* list, contactInfoExt_t* extList, int max ) { return 0; }
	virtual int				AddCustomConstraints( constraintInfo_t* list, int max ) { return 0; }

	virtual bool			IsOwner( idEntity* other ) const { return false; }

	// animation
	virtual bool			AllowAnimationInhibit( void ) { return true; }
	virtual bool			UpdateAnimationControllers( void );
	virtual bool			UpdateRenderEntity( renderEntity_t* renderEntity, const renderView_s* renderView, int& lastModifiedGameTime );
	static bool				ModelCallback( renderEntity_t* renderEntity, const renderView_s* renderView, int& lastModifiedGameTime );

	virtual void			GetWaterCurrent( idVec3& waterCurrent ) const { waterCurrent = vec3_zero; }
	virtual void			CheckWater( const idVec3& waterBodyOrg, const idMat3& waterBodyAxis, idCollisionModel* waterBodyModel ) { ; }
	virtual void			CheckWaterEffectsOnly( void ) { ; }

	int						FindSurfaceId( const char* surfaceName );

	virtual void			OnGuiActivated( void ) { ; }
	virtual void			OnGuiDeactivated( void ) { ; }

	virtual sdBindContext*	GetBindContext( void ) { return NULL; }

	// sound
	bool					StartSound( const char *soundName, const soundChannel_t channelStart, const soundChannel_t channelEnd, int soundShaderFlags, int *length );
	bool					StartSound( const char *soundName, const soundChannel_t channel, int soundShaderFlags, int *length );
	bool					StartSoundShader( const idSoundShader *shader, const soundChannel_t channel, int soundShaderFlags, int *length );
	bool					StartSoundShader( const idSoundShader *shader, const soundChannel_t channelStart, const soundChannel_t channelEnd, int soundShaderFlags, int *length );
	void					StopSound( const soundChannel_t channel );	// pass SND_ANY to stop all sounds
	void					SetChannelVolume( const soundChannel_t channel, float volume );
	void					SetChannelPitchShift( const soundChannel_t channel, float pitchshift );
	void					SetChannelFlags( const soundChannel_t channel, int flags );
	void					ClearChannelFlags( const soundChannel_t channel, int flags );
	void					SetChannelOffset( const soundChannel_t channel, int ms );
	void					SetSoundVolume( float volume );
	void					UpdateSound( void );
	int						GetListenerId( void ) const;
	idSoundEmitter *		GetSoundEmitter( void ) const;
	void					FreeSoundEmitter( bool immediate );
	void					FadeSound( const soundChannel_t channel, float to, float over );

	virtual void			OnEventRemove( void );

// RAVEN BEGIN
// bdube: added effect functions
	// effects
	rvClientEffect*			PlayEffect		( const char* effectName, const idVec3& color, const char* materialType, jointHandle_t joint, bool loop = false, const idVec3& endOrigin = vec3_origin );
	rvClientEffect*			PlayEffect		( const char* effectName, const idVec3& color, const char* materialType, const idVec3& origin, const idMat3& axis, bool loop = false, const idVec3& endOrigin = vec3_origin, bool viewsuppress = true );
	rvClientEffect*			PlayEffectMaxVisDist		( const char* effectName, const idVec3& color, const char* materialType, jointHandle_t joint, bool loop = false, bool isStatic = false, float maxVisDist = 0.f, const idVec3& endOrigin = vec3_origin );
	rvClientEffect*			PlayEffectMaxVisDist		( const char* effectName, const idVec3& color, const char* materialType, const idVec3& origin, const idMat3& axis, bool loop = false, bool isStatic = false, float maxVisDist = 0.f, const idVec3& endOrigin = vec3_origin );
	rvClientEffect*			PlayEffect		( const int effectHandle, const idVec3& color, jointHandle_t joint, bool loop = false, const idVec3& endOrigin = vec3_origin );
	rvClientEffect*			PlayEffect		( const int effectHandle, const idVec3& color, const idVec3& origin, const idMat3& axis, bool loop = false, const idVec3& endOrigin = vec3_origin, bool viewsuppress = true );
	rvClientEffect*			PlayEffectMaxVisDist		( const int effectHandle, const idVec3& color, jointHandle_t joint, bool loop = false, bool isStatic = false, float maxVisDist= 0.f, const idVec3& endOrigin = vec3_origin );
	rvClientEffect*			PlayEffectMaxVisDist		( const int effectHandle, const idVec3& color, const idVec3& origin, const idMat3& axis, bool loop = false, bool isStatic = false, float maxVisDist = 0.f, const idVec3& endOrigin = vec3_origin );
	void					StopEffect		( const char* effectName, bool destroyParticles = false );
	void					StopEffect		( int effectHandle, bool destroyParticles = false );
	void					StopEffectHandle( unsigned int handle, bool destroyParticles = false );
	void					UnBindEffectHandle( unsigned int handle );
	void					SetEffectAttenuation( unsigned int handle, float attenuation );
	void					SetEffectColor	( unsigned int handle, const idVec4& color );
	void					SetEffectOrigin	( unsigned int handle, const idVec3& origin );
	void					SetEffectAxis	( unsigned int handle, const idMat3& axis);

	void					StopAllEffects	( bool destroyParticles = false );
	void					UpdateEffects	( void );

	float					DistanceTo		( idEntity* ent );
	float					DistanceTo		( const idVec3& pos );
	float					DistanceTo2d	( idEntity* ent );
	float					DistanceTo2d	( const idVec3& pos );

// RAVEN END

	// entity binding
	virtual void			PreBind( void );
	virtual void			PostBind( void );
	virtual void			PreUnbind( void );
	virtual void			PostUnbind( void );
	void					JoinTeam( idEntity *teammember );
	void					Bind( idEntity *master, bool orientated );
	void					BindToJoint( idEntity *master, const char *jointname, bool orientated );
	void					BindToJoint( idEntity *master, jointHandle_t jointnum, bool orientated );
	void					BindToBody( idEntity *master, int bodyId, bool orientated );
	void					Unbind( void );
	bool					IsBound( void ) const;
	bool					IsBoundTo( idEntity *master ) const;

	// called when the bind master's visibility changes
	virtual void			OnBindMasterVisChanged();
	void					NotifyVisChanged();

	idEntity*				GetBindMaster( void ) const { return bindMaster; }
	jointHandle_t			GetBindJoint( void ) const { return bindJoint; }
	int						GetBindBody( void ) const { return bindBody; }
	idEntity*				GetTeamMaster( void ) const { return teamMaster; }
	idEntity*				GetNextTeamEntity( void ) const { return teamChain; }

	void					ConvertLocalToWorldTransform( idVec3 &offset, idMat3 &axis );
	void					ConvertLocalToWorldTransform( idVec3& offset );
	void					ConvertLocalToWorldTransform( idMat3& axis );
	idVec3					GetLocalVector( const idVec3 &vec ) const;
	idVec3					GetLocalCoordinates( const idVec3 &vec ) const;
	idVec3					GetWorldVector( const idVec3 &vec ) const;
	idVec3					GetWorldCoordinates( const idVec3 &vec ) const;
	bool					GetMasterPosition( idVec3 &masterOrigin, idMat3 &masterAxis ) const;
	idEntity*				GetMaster( void ) const { return bindMaster; }
	void					GetWorldVelocities( idVec3 &linearVelocity, idVec3 &angularVelocity ) const;
	
	// physics
							// set a new physics object to be used by this entity
	void					SetPhysics( idPhysics *phys );
							// get the physics object used by this entity
	idPhysics *				GetPhysics( void ) const { return physics; }
							// restore physics pointer for save games
	void					RestorePhysics( idPhysics *phys );
							// run the physics for this entity
	bool					RunPhysics( void );
							// set the origin of the physics object (relative to bindMaster if not NULL)
	virtual void			SetOrigin( const idVec3 &org );
							// set the axis of the physics object (relative to bindMaster if not NULL)
	virtual void			SetAxis( const idMat3 &axis );
	virtual const idMat3	&GetAxis( void );
							// sets the origin and axis of the physics object (relative to bindMaster if not NULL)
	virtual void			SetPosition( const idVec3 &org, const idMat3 &axis );
							// use angles to set the axis of the physics object (relative to bindMaster if not NULL)
	void					SetAngles( const idAngles &ang );
							// get the floor position underneath the physics object
	bool					GetFloorPos( float max_dist, idVec3 &floorpos ) const;
							// retrieves the transformation going from the physics origin/axis to the visual origin/axis
	virtual bool			GetPhysicsToVisualTransform( idVec3 &origin, idMat3 &axis );
							// retrieves the transformation going from the physics origin/axis to the sound origin/axis
	virtual bool			GetPhysicsToSoundTransform( idVec3 &origin, idMat3 &axis );
							// called from the physics object when colliding, should return true if the physics simulation should stop
	virtual bool			Collide( const trace_t& collision, const idVec3 &velocity, int bodyId );
							// called from the physics object when colliding, this outright kills whatever is being hit
	virtual void			CollideFatal( idEntity* other );
							// called from the physics object when being collided with
	virtual void			Hit( const trace_t &collision, const idVec3 &velocity, idEntity *other );
							// retrieves impact information, 'ent' is the entity retrieving the info
	virtual void			GetImpactInfo( idEntity *ent, int id, const idVec3 &point, impactInfo_s* info );
							// apply an impulse to the physics object, 'ent' is the entity applying the impulse
	virtual void			ApplyImpulse( idEntity *ent, int id, const idVec3 &point, const idVec3 &impulse );
							// add a force to the physics object, 'ent' is the entity adding the force
	virtual void			AddForce( idEntity *ent, int id, const idVec3 &point, const idVec3 &force );
							// activate the physics object, 'ent' is the entity activating this entity
	virtual void			ActivatePhysics( void );
							// returns true if the physics object is at rest
	virtual bool			IsAtRest( void ) const;
							// returns the time the physics object came to rest
	virtual int				GetRestStartTime( void ) const;
							// add a contact entity
	virtual void			AddContactEntity( idEntity *ent );
							// remove a touching entity
	virtual void			RemoveContactEntity( idEntity *ent );

	virtual	bool			IsCollisionPushable( void ) { return true; }

	// damage
							// returns true if this entity can be damaged from the given origin
	virtual bool			CanDamage( const idVec3 &origin, idVec3 &damagePoint, int mask, idEntity* passEntity, trace_t* tr = NULL ) const;

							// applies damage to this entity
	virtual	void			Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const sdDeclDamage* damage, const float damageScale, const trace_t* collision, bool forceKill = false );

	bool					CheckTeamDamage( idEntity *inflictor, const sdDeclDamage* damageDecl );

	virtual	void			UpdateKillStats( idPlayer* player, const sdDeclDamage* damageDecl, bool headshot ) { ; }

	virtual	void			OnBulletImpact( idEntity* attacker, const trace_t& trace ) { ; }

	virtual	void			SetLastAttacker( idEntity* attacker, const sdDeclDamage* damage ) { ; }

							// callback function for when another entity received damage from this entity.  damage can be adjusted and returned to the caller.
	virtual void			DamageFeedback( idEntity *victim, idEntity *inflictor, int oldHealth, int newHealth, const sdDeclDamage* damageDecl, bool headshot ) { ; }
							// notifies this entity that it is in pain
	virtual bool			Pain( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location, const sdDeclDamage* damageDecl );
							// notifies this entity that is has been killed
	virtual void			Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location, const sdDeclDamage* damageDecl );

	virtual bool			CanCollide( const idEntity* other, int traceId ) const;
							// this is the proxy entity for the other - should it collide?
	virtual bool			ShouldProxyCollide( const idEntity* other ) const;

	static void				CheckForDuplicateRenderEntityHandles( void );

	// scripting
	virtual bool			ShouldConstructScriptObjectAtSpawn( void ) const;
	virtual sdProgramThread* ConstructScriptObject( void );
	virtual void			DeconstructScriptObject( void );

	virtual void			OnTeamBlocked( idEntity* blockedPart, idEntity* blockingEntity ) { ; }
	virtual void			OnPartBlocked( idEntity* blockingEntity ) { ; }

	virtual void			OnMoveStarted( void ) { ; }
	virtual void			OnMoveFinished( void ) { ; }

	virtual bool			WantsTouch( void ) const { return false; }
	virtual void			OnTouch( idEntity *other, const trace_t& trace ) { ; }

	virtual void			OnTeleportStarted( sdTeleporter* teleporter ) { ; }
	virtual void			OnTeleportFinished( void ) { ; }
	virtual bool			AllowTeleport( void ) const { return false; }

	virtual const idBounds*	GetSelectionBounds( void ) const { return NULL; }

	virtual bool			RunPausedPhysics( void ) const { return false; }

	// targets
	void					FindTargets( void );
	void					RemoveNullTargets( void );

	// misc
	virtual void			Teleport( const idVec3 &origin, const idAngles &angles, idEntity *destination );
	bool					TouchTriggers( void );
	idCurve_Spline<idVec3> *GetSpline( void ) const;
	virtual void			ShowEditingDialog( void );

	bool					GetWorldAxis( jointHandle_t joint, idMat3& axis );
	bool					GetWorldOrigin( jointHandle_t joint, idVec3& org );
	bool					GetWorldOriginAxisNoUpdate( jointHandle_t joint, idVec3& org, idMat3& axis );
	bool					GetWorldOriginAxis( jointHandle_t joint, idVec3& org, idMat3& axis );

	enum {
		EVENT_MAXEVENTS
	};

	virtual void			UpdatePredictionErrorDecay( void ) { ; }
	virtual void			OnNewOriginRead( const idVec3& newOrigin ) { ; }
	virtual void			OnNewAxesRead( const idMat3& newAxes ) { ; }
	virtual void			ResetPredictionErrorDecay( const idVec3* origin = NULL, const idMat3* axes = NULL ) { ; }

	virtual void			OnPhysicsRested( void ) { BecomeInactive( TH_PHYSICS ); }

	void					ForceNetworkUpdate( void );

	virtual bool			ClientReceiveEvent( int event, int time, const idBitMsg &msg );
	virtual bool			ClientReceiveUnreliableEvent( int event, int time, const idBitMsg &msg );

	virtual bool			UpdateCrosshairInfo( idPlayer* player, sdCrosshairInfo& info );

	void					BroadcastEvent( int eventId, const idBitMsg *msg, bool saveEvent, const sdReliableMessageClientInfoBase& target ) const;
	void					BroadcastUnreliableEvent( int eventId, const idBitMsg *msg ) const;

	virtual void			SetGameTeam( sdTeamInfo* _team ) { ; }
	virtual sdTeamInfo*		GetGameTeam( void ) const { return NULL; }
	teamAllegiance_t		GetEntityAllegiance( const idEntity* other ) const;
	const idVec4&			GetColorForAllegiance( idEntity* other ) const;
	static const idVec4&	GetColorForAllegiance( teamAllegiance_t allegiance );
	static const idVec4&	GetFireteamColor( void );
	static const idVec4&	GetFireteamLeaderColor( void );
	static const idVec4&	GetBuddyColor( void );
	virtual void			GetTaskName( idWStr& out );

	virtual sdRequirementCheckInterface*	GetRequirementInterface( void ) { return NULL; }
	virtual sdGuiInterface*					GetGuiInterface( void ) { return NULL; }
	virtual sdNetworkInterface*				GetNetworkInterface( void ) { return NULL; }
	virtual sdTaskInterface*				GetTaskInterface( void ) { return NULL; }
	virtual sdRequirementContainer*			GetSpawnRequirements( void ) { return NULL; }
	virtual sdUsableInterface*				GetUsableInterface( void ) { return NULL; }
	virtual sdInteractiveInterface*			GetInteractiveInterface( void ) { return NULL; }
	virtual sdEntityDisplayIconInterface*	GetDisplayIconInterface( void ) { return NULL; }

	idPhysics_Static&		GetDefaultPhysics( void ) { return defaultPhysicsObj; }

	virtual bool			NeedsRepair() { return false; }

	virtual idEntity*		GetDisguiseEntity( void ) { return this; }

	virtual cheapDecalUsage_t GetDecalUsage( void );

	virtual	bool			OverridePreventDeployment( idPlayer* ) { return false; }

	virtual	int				PlayHitBeep( idPlayer* player, bool headshot ) const;
	virtual float			GetDamageXPScale( void ) const { return 1.f; }

	// input -> usercmd translation
	virtual bool			OnKeyMove( char forward, char right, char up, usercmd_t& cmd ) { return false; }
	virtual void			OnControllerMove( bool doGameCallback, const int numControllers, const int* controllerNumbers,
											const float** controllerAxis, idVec3& viewAngles, usercmd_t& cmd );
	virtual void			OnMouseMove( idPlayer* player, const idAngles& baseAngles, idAngles& angleDelta ) {}
	

	virtual int							GetOcclusionQueryHandle( void ) const { return occlusionQueryHandle; }
	virtual const occlusionTest_t&		GetOcclusionQueryInfo( void ) const { return occlusionQueryInfo; }
	virtual const occlusionTest_t&		UpdateOcclusionInfo( int viewId );
	virtual bool						IsVisibleOcclusionTest();

	virtual bool			IsPhysicsInhibited( void ) const { return ( aorFlags & AOR_INHIBIT_PHYSICS ) != 0; }
	int						GetAORPhysicsLOD( void ) const;

	const idVec3&			GetLastPushedOrigin( void ) const { return lastPushedOrigin; }
	const idMat3&			GetLastPushedAxis( void ) const { return lastPushedAxis; }
			
	virtual bool			DisableClipOnRemove( void ) const { return false; }

protected:
	renderEntity_t			renderEntity;						// used to present a model to the renderer
	int						modelDefHandle;						// handle to static renderer model
	refSound_t				refSound;							// used to present sound to the audio engine

	float					occlusionQueryBBScale;				// Gordon: FIXME: This probably shouldn't be here
	int						occlusionQueryHandle;
	occlusionTest_t			occlusionQueryInfo;

	// last origin & axis pushed to the renderer
	idVec3					lastPushedOrigin;
	idMat3					lastPushedAxis;

private:
	idPhysics_Static		defaultPhysicsObj;					// default physics object
	idPhysics *				physics;							// physics used for this entity
	const sdDeclTargetInfo*	contentBoundsFilter;

	// Gordon: FIXME: Alloc only as needed
	idEntity*				bindMaster;							// entity bound to if unequal NULL
	jointHandle_t			bindJoint;							// joint bound to if unequal INVALID_JOINT
	int						bindBody;							// body bound to if unequal -1
	idEntity*				teamMaster;							// master of the physics team
	idEntity*				teamChain;							// next entity in physics team

	typedef struct beamInfo_s {
		renderEntity_t			beamEnt;
		qhandle_t				beamHandle;
	} beamInfo_t;

	idList< beamInfo_t* >	beams;
	const sdDeclLocStr*		taskName;

protected:
	static entityCache_t					scriptEntityCache;
	static sdPair< bool, entityCache_t >	savedEntityCache[ NUM_ENTITY_CAHCES ];
	static sdCrosshairInfo*					crosshairInfo;

public:
	// physics
							// initialize the default physics
	void					InitDefaultPhysics( const idVec3 &origin, const idMat3 &axis );
	
	idClipModel*			InitDefaultClipModel();
	const char*				GetClipModelName( void ) const;

	bool					CheckEntityContentBoundsFilter( idEntity* other ) const;

	// deletes any entities bound to this object
	void					RemoveBinds( idBounds* bounds = NULL, bool onlyUnbindScripted = false );

protected:

	// physics

	// entity binding
	virtual bool			InitBind( idEntity *master );		// initialize an entity binding
	virtual void			FinishBind( void );					// finish an entity binding
	void					QuitTeam( void );					// leave the current team
	void					FreeBeam( int index );				// kill a specific handle's beam render entity
	void					FreeAllBeams( void );				// kill all beam render entities

// RAVEN BEGIN
// bdube: client entities
	void					RemoveClientEntities ( void );		// deletes any client entities bound to this object
// RAVEN END

	static void				AddClassToBoundsCache( idClass* cls );

	bool					LaunchBullet( idEntity* owner, idEntity* ignoreEntity, const idDict& projectileDict, const idVec3& origin, const idMat3& axis, const idVec3& tracerMuzzleOrigin, const idMat3& tracerMuzzleAxis, float damagePower, int forceTracer, rvClientEffect** tracerOut = NULL, bool useAntiLag = true );
	bool					DoLaunchBullet( idEntity* owner, idEntity* ignoreEntity, const idDict& projectileDict, const idVec3& startPos, const idVec3& endPos, const idVec3& tracerMuzzleOrigin, const idMat3& tracerMuzzleAxis, float damagePower, int forceTracer, rvClientEffect** tracerOut, int mask, const sdDeclDamage* bulletDamage, bool doEffects, bool doImpact, bool useAntiLag );

	idVec3					bulletTracerStart;
	idVec3					bulletTracerEnd;

	guiHandle_t				GetEntityGuiHandle( const int guiNum );

	// events
	void					Event_GetName( void );
	void					Event_FindTargets( void );
	void					Event_Bind( idEntity *master );
	void					Event_BindPosition( idEntity *master );
	void					Event_BindToJoint( idEntity *master, const char *jointname, bool orientated );
	void					Event_Unbind( void );
	void					Event_RemoveBinds( void );
	void					Event_SetModel( const char *modelname );
	void					Event_SetSkin( const char *skinname );
	void					Event_SetCoverage( float v );
	void					Event_GetShaderParm( int parmnum );
	void					Event_SetShaderParm( int parmnum, float value );
	void					Event_SetShaderParms( float parm0, float parm1, float parm2, float parm3 );
	void					Event_SetColor( float red, float green, float blue );
	void					Event_GetColor( void );
	void					Event_IsHidden( void );
	void					Event_Hide( void );
	void					Event_Show( void );
	void					Event_StartSound( const char *soundName, int channel );
	void					Event_StopSound( int channel );
	void					Event_FadeSound( int channel, float to, float over );
	void					Event_SetChannelPitchShift( const soundChannel_t channel, float shift );
	void					Event_SetChannelFlags( const soundChannel_t channel, int flags );
	void					Event_ClearChannelFlags( const soundChannel_t channel, int flags );
	void					Event_GetWorldAxis( int index );
	void					Event_SetWorldAxis( const idVec3& fwd, const idVec3& right, const idVec3& up );
	void					Event_GetWorldOrigin( void );
	void					Event_GetGravityNormal( void );
	void					Event_SetWorldOrigin( const idVec3 &org );
	void					Event_GetOrigin( void );
	void					Event_SetOrigin( const idVec3 &org );
	void					Event_GetAngles( void );
	void					Event_SetAngles( const idAngles &ang );
	void					Event_SetGravity( const idVec3& gravity );
	void					Event_AlignToAxis( const idVec3& vec, int axis );
	void					Event_SetLinearVelocity( const idVec3 &velocity );
	void					Event_GetLinearVelocity( void );
	void					Event_SetAngularVelocity( const idVec3 &velocity );
	void					Event_GetAngularVelocity( void );
	void					Event_GetMass( void );
	void					Event_GetCenterOfMass( void );
	void					Event_SetFriction( float linear, float angular, float contact );
	void					Event_SetSize( const idVec3 &mins, const idVec3 &maxs );
	void					Event_GetSize( void );
	void					Event_GetMins( void );
	void					Event_GetMaxs( void );
	void					Event_GetAbsMins( void );
	void					Event_GetAbsMaxs( void );
	void					Event_GetRenderMins( void );
	void					Event_GetRenderMaxs( void );
	void					Event_SetRenderBounds( const idVec3& mins, const idVec3& maxs );
	void					Event_Touches( idEntity *ent, bool ignoreNonTrace );
	void					Event_TouchesBounds( idEntity *ent );
	void					Event_GetNextKey( const char *prefix, const char *lastMatch );
	void					Event_SetKey( const char *key, const char *value );
	void					Event_GetKey( const char *key );
	void					Event_GetIntKey( const char *key );
	void					Event_GetFloatKey( const char *key );
	void					Event_GetVectorKey( const char *key );
	void					Event_GetEntityKey( const char *key );
	void					Event_DistanceTo( idEntity *ent );
	void					Event_DistanceToPoint( const idVec3 &point );
	void					Event_DetachRotationBind( int handle );

	void					Event_DisablePhysics( void );
	void					Event_EnablePhysics( void );
	void					Event_EntitiesInBounds( const idVec3& mins, const idVec3& maxs, int contentMask, bool absoluteCoords );
	void					Event_EntitiesInLocalBounds( const idVec3& mins, const idVec3& maxs, int contentMask );
	void					Event_EntitiesInTranslation( const idVec3& start, const idVec3& end, int contentMask, idEntity* passEntity );
	void					Event_EntitiesInRadius( const idVec3& centre, float radius, int contentMask, bool absoluteCoords );
	void					Event_EntitiesOfClass( int typeHandle, bool additive );
	void					Event_EntitiesOfType( int typeHandle );
	void					Event_EntitiesOfCollection( const char* name );
	void					Event_FilterEntitiesByRadius( const idVec3& origin, float radius, bool inclusive );
	void					Event_FilterEntitiesByClass( const char* typeName, bool inclusive );
	void					Event_FilterEntitiesByAllegiance( int mask, bool inclusive );
	void					Event_FilterEntitiesByDisguiseAllegiance( int mask, bool inclusive );
	void					Event_FilterEntitiesByFilter( int filterIndex, bool inclusive );
	void					Event_FilterEntitiesByTouching( bool inclusive );
	void					Event_GetBoundsCacheCount( void );
	void					Event_GetBoundsCacheEntity( int index );
	void					Event_GetEntityAllegiance( idEntity* entity );
	void					Event_HasAbility( const char* abilityName );
	void					Event_SyncScriptField( const char* fieldName );
	void					Event_SyncScriptFieldBroadcast( const char* fieldName );
	void					Event_SyncScriptFieldCallback( const char* fieldName, const char* functionName );
	void					Event_GetMaster( void );
	void					Event_TakesDamage( void );
	void					Event_SetTakesDamage( bool value );
	void					Event_SetNetworkSynced( bool value );
	void					Event_ApplyDamage( idEntity* inflictor, idEntity* attacker, const idVec3& dir, int damageIndex, float damageScale, idScriptObject* collisionHandle );
	void					Event_Physics_ClearContacts( void );
	void					Event_Physics_SetContents( int contents );
	void					Event_Physics_SetClipmask( int clipmask );
	void					Event_Physics_PutToRest( void );
	void					Event_Physics_HasGroundContacts( void );
	void					Event_DisableGravity( bool disable );
	void					Event_Physics_SetComeToRest( bool value );
	void					Event_Physics_ApplyImpulse( const idVec3& origin, const idVec3& impulse );
	void					Event_Physics_AddForce( const idVec3& force );
	void					Event_Physics_AddForceAt( const idVec3& force, const idVec3& position );
	void					Event_Physics_AddTorque( const idVec3& torque );
	void					Event_HasForceDisableClip( void );
	void					Event_ForceDisableClip( void );
	void					Event_ForceEnableClip( void );
	void					Event_PreventDeployment( bool prevent );
	void					Event_TurnTowards( const idVec3& dir, float maxAngularSpeed );
	void					Event_GetTeam( void );
	void					Event_SetTeam( idScriptObject* object );
	void					Event_LaunchMissile( idEntity* owner, idEntity* owner2, idEntity* enemy, int projectileDefIndex, int clientProjectileDefIndex, float spread, const idVec3& origin, const idVec3& velocity );
	void					Event_LaunchBullet( idEntity* owner, idEntity* ignoreEntity, int projectileDefIndex, float spread, const idVec3& origin, const idVec3& velocity, int forceTracer, bool useAntiLag );
	void					Event_GetBulletTracerStart( void );
	void					Event_GetBulletTracerEnd( void );
	void					Event_GetGUI( const char* name );
	void					Event_IsAtRest( void );
	void					Event_IsBound( void );
	void					Event_DisableImpact( void );
	void					Event_EnableImpact( void );
	void					Event_EnableKnockback( void );
	void					Event_DisableKnockback( void );
	void					Event_Physics_Activate( void );

	void					Event_SetNumCrosshairLines( int count );
	void					Event_AddCrosshairLine();
	void					Event_GetCrosshairDistance();
	void					Event_SetCrosshairLineText( int index, const wchar_t* text );
	void					Event_SetCrosshairLineTextIndex( int index, const int textIndex );
	void					Event_SetCrosshairLineMaterial( int index, const char* material );
	void					Event_SetCrosshairLineColor( int index, const idVec3& color, float alpha );
	void					Event_SetCrosshairLineSize( int index, float x, float y );
	void					Event_SetCrosshairLineFraction( int index, float frac );
	void					Event_SetCrosshairLineType( int index, int type );

	void					Event_SendNetworkEvent( int clientIndex, bool isRepeaterClient, const char* message );
	void					Event_SendNetworkCommand( const char* message );
	void					Event_PointInRadar( const idVec3& point, radarMasks_t type, bool ignoreJammers );

	void					Event_GetMaxHealth( void );
	void					Event_SetMaxHealth( int value );
	void					Event_GetHealth( void );
	void					Event_SetHealth( int value );

	void					Event_AllocBeam( const char* shader );
	void					Event_UpdateBeam( int handle, const idVec3& start, const idVec3& end, const idVec3& color, float alpha, float width );
	void					Event_FreeBeam( int index );
	void					Event_FreeAllBeams();
	
	void					Event_GetNextTeamEntity( void );

	void					Event_GetKeyWithDefault( const char *key, const char* defaultvalue );
	void					Event_GetIntKeyWithDefault( const char *key, int defaultvalue );
	void					Event_GetFloatKeyWithDefault( const char *key, float defaultvalue );
	void					Event_GetVectorKeyWithDefault( const char *key, idVec3 &defaultvalue );

	void					Event_SetCanCollideWithTeam( bool canCollide );

	void					Event_PlayMaterialEffect( const char *effectName, const idVec3& color, const char* jointName, const char* materialType, bool loop );
	void					Event_PlayMaterialEffectMaxVisDist( const char *effectName, const idVec3& color, const char* jointName, const char* materialType, bool loop, float maxVisDist, bool isStatic );
	void					Event_PlayEffect( const char* effectName, const char* boneName, bool loop );
	void					Event_PlayEffectMaxVisDist( const char* effectName, const char* boneName, bool loop, float maxVisDist, bool isStatic );
	void					Event_PlayJointEffect( const char* effectName, jointHandle_t joint, bool loop );
	void					Event_PlayJointEffectViewSuppress( const char* effectName, jointHandle_t joint, bool loop, bool suppress );
	void					Event_PlayOriginEffect( const char* effectName, const char* materialType, const idVec3& origin, const idVec3& forward, bool loop );
	void					Event_PlayOriginEffectMaxVisDist( const char* effectName, const char* materialType, const idVec3& origin, const idVec3& forward, bool loop, float maxVisDist, bool isStatic );
	void					Event_PlayBeamEffect( const char* effectName, const char* materialType, const idVec3& origin, const idVec3& endOrigin, bool loop );
	void					Event_LookupEffect( const char* effectName, const char* materialType );
	void					Event_StopEffect( const char* effectName );
	void					Event_KillEffect( const char *effectName );
	void					Event_StopEffectHandle( int handle );
	void					Event_UnBindEffectHandle( int handle );
	void					Event_SetEffectRenderBounds( int handle, bool renderBounds );
	void					Event_SetEffectAttenuation( int handle, float attenuation );
	void					Event_SetEffectColor( int handle, const idVec3& color, float alpha );
	void					Event_SetEffectOrigin( int handle, const idVec3& origin );
	void					Event_SetEffectAngles( int handle, const idAngles& angles );
	void					Event_GetEffectOrigin( int handle );
	void					Event_GetEffectEndOrigin( int handle );
	void					Event_StopAllEffects( void );
	void					Event_SpawnClientEffect( const char* effectName );
	void					Event_SpawnClientCrawlEffect( const char* effectName, idEntity* ent, float crawlTime );

	void					Event_IsInWater( void );
	void					Event_GetSpawnID( void );
	void					Event_IsSpotted( void );
	void					Event_SetSpotted( idEntity* other );
	void					Event_ForceNetworkUpdate( void );

	void					Event_AddCheapDecal( idEntity *attachTo, idVec3 &origin, idVec3 &normal, const char* decalName, const char* materialName );

	void					Event_GetEntityNumber( void );

	void					Event_DisableCrosshairTrace( bool value );
	void					Event_InCollection( const char* name );

	void					Event_GetEntityContents();
	void					Event_GetMaskedEntityContents( int mask );

	void					Event_SaveCachedEntities( void );
	void					Event_FreeSavedCache( int index );
	void					Event_GetSavedCacheCount( int index );
	void					Event_GetSavedCacheEntity( int index, int entityIndex );

	void					Event_GetDriver( void );
	void					Event_GetNumPositions( void );
	void					Event_GetPositionPlayer( int index );
	void					Event_GetDamageScale( void );

	void					Event_GetJointHandle( const char *jointname );
	void					Event_GetDefaultSurfaceType( void );
	void					Event_ForceRunPhysics( void );

private:
	void					FilterEntitiesByAllegiance( int mask, bool inclusive, bool ignoreDisguise );

};

class sdEntityBroadcastEvent : public idBitMsg {
public:
	sdEntityBroadcastEvent( const idEntity* _owner, int event ) {
		assert( _owner );

		InitWrite( buffer, sizeof( buffer ) );
		eventId = event;
		owner	= _owner;
	}

	void Send( bool saveEvent, const sdReliableMessageClientInfoBase& target ) {
		owner->BroadcastEvent( eventId, this, saveEvent, target );
	}

protected:
	byte				buffer[ MAX_GAME_MESSAGE_SIZE ];
	int					eventId;
	const idEntity*		owner;
};

class sdEntityBroadcastUnreliableEvent : public idBitMsg {
public:
	sdEntityBroadcastUnreliableEvent( const idEntity* _owner, int event ) {
		assert( _owner );

		InitWrite( buffer, sizeof( buffer ) );
		eventId = event;
		owner	= _owner;
	}

	void Send( void ) {
		owner->BroadcastUnreliableEvent( eventId, this );
	}

protected:
	byte				buffer[ MAX_GAME_MESSAGE_SIZE ];
	int					eventId;
	const idEntity*		owner;
};

#endif /* !__GAME_ENTITY_H__ */
