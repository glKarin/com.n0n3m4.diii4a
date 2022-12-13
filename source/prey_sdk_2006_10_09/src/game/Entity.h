// Copyright (C) 2004 Id Software, Inc.
//

#ifndef __GAME_ENTITY_H__
#define __GAME_ENTITY_H__

/*
===============================================================================

	Game entity base class.

===============================================================================
*/

// HUMANHEAD

// forward declarations
class hhZone;
class hhFxInfo;
class hhEntityFx;
class hhReaction;

#define SAFE_REMOVE(pPtr) { if((pPtr) != NULL){ (pPtr)->PostEventMS(&EV_Remove, 0); (pPtr) = NULL; }}
#define SAFE_DELETE_PTR(pPtr) { if((pPtr) != NULL){ delete (pPtr); (pPtr) = NULL; }}
#define SAFE_DELETE_ARRAY_PTR(pPtr) { if((pPtr) != NULL){ delete [](pPtr); (pPtr) = NULL; }}
#define SAFE_FREELIGHT(handle) { if((handle) > -1){ gameRenderWorld->FreeLightDef(handle); (handle) = -1; }}

extern void AddRenderGui( const char *name, idUserInterface **gui, const idDict *args );
extern void SetDeformationOnRenderEntity(renderEntity_t *renderEntity, int deformType, float parm1, float parm2);
//HUMANHEAD END

//HUMANHEAD
extern const idEventDef EV_SetSkinByName;
extern const idEventDef EV_Deactivate;
extern const idEventDef EV_GlobalBeginState;
extern const idEventDef EV_GlobalTicker;
extern const idEventDef EV_GlobalEndState;
extern const idEventDef EV_GlobalDeactivate;
extern const idEventDef EV_GlobalActivate;
extern const idEventDef EV_DeadBeginState;
extern const idEventDef EV_DeadTicker;
extern const idEventDef EV_DeadEndState;
extern const idEventDef EV_StartSound;
extern const idEventDef EV_StopSound;
extern const idEventDef EV_StartFx;
extern const idEventDef EV_SetSoundTrigger;
extern const idEventDef EV_Gib;
extern const idEventDef EV_ResetGravity;
extern const idEventDef EV_MoveToJoint;
extern const idEventDef EV_MoveToJointWeighted;
extern const idEventDef EV_MoveJointToJoint;
extern const idEventDef EV_MoveJointToJointOffset;
extern const idEventDef EV_SpawnDebris;
extern const idEventDef EV_DamageEntity;
extern const idEventDef EV_DelayDamageEntity;
extern const idEventDef EV_Dispose;
extern const idEventDef EV_LoadReactions;
extern const idEventDef EV_UpdateCameraTarget;
extern const idEventDef EV_SetDeformation;
extern const idEventDef EV_SpawnBind;
extern const idEventDef EV_Broadcast_AssignFx;
extern const idEventDef EV_Broadcast_AppendFxToList;
extern const idEventDef EV_SetFloatKey;
extern const idEventDef EV_SetVectorKey;
extern const idEventDef EV_SetEntityKey;
extern const idEventDef EV_PlayerCanSee;
extern const idEventDef EV_KillBox;
//HUMANHEAD END

static const int DELAY_DORMANT_TIME = 3000;

extern const idEventDef EV_PostSpawn;
extern const idEventDef EV_FindTargets;
extern const idEventDef EV_Touch;
extern const idEventDef EV_Use;
extern const idEventDef EV_Activate;
extern const idEventDef EV_ActivateTargets;
extern const idEventDef EV_Hide;
extern const idEventDef EV_Show;
extern const idEventDef EV_GetShaderParm;
extern const idEventDef EV_SetShaderParm;
extern const idEventDef EV_SetOwner;
extern const idEventDef EV_GetAngles;
extern const idEventDef EV_SetAngles;
extern const idEventDef EV_SetLinearVelocity;
extern const idEventDef EV_SetAngularVelocity;
extern const idEventDef EV_SetSkin;
extern const idEventDef EV_StartSoundShader;
extern const idEventDef EV_StopSound;
extern const idEventDef EV_CacheSoundShader;

// Think flags
enum {
	TH_ALL					= -1,
	TH_THINK				= 1,		// run think function each frame
	TH_PHYSICS				= 2,		// run physics each frame
	TH_ANIMATE				= 4,		// update animation each frame
	TH_UPDATEVISUALS		= 8,		// update renderEntity
	TH_UPDATEPARTICLES		= 16,
	// HUMANHEAD pdm: new think flags
	TH_TICKER				= 32,		// Run the ticker function
	TH_COMBATMODEL			= 64,		// Update the combat model
	TH_MISC1				= 128,		// Misc
	TH_MISC2				= 256,
	TH_MISC3				= 512
	// HUMANHEAD END
};

//
// Signals
// make sure to change script/doom_defs.script if you add any, or change their order
//
typedef enum {
	SIG_TOUCH,				// object was touched
	SIG_USE,				// object was used
	SIG_TRIGGER,			// object was activated
	SIG_REMOVED,			// object was removed from the game
	SIG_DAMAGE,				// object was damaged
	SIG_BLOCKED,			// object was blocked

	SIG_MOVER_POS1,			// mover at position 1 (door closed)
	SIG_MOVER_POS2,			// mover at position 2 (door open)
	SIG_MOVER_1TO2,			// mover changing from position 1 to 2
	SIG_MOVER_2TO1,			// mover changing from position 2 to 1

	NUM_SIGNALS
} signalNum_t;

// FIXME: At some point we may want to just limit it to one thread per signal, but
// for now, I'm allowing multiple threads.  We should reevaluate this later in the project
#define MAX_SIGNAL_THREADS 16		// probably overkill, but idList uses a granularity of 16

struct signal_t {
	int					threadnum;
	const function_t	*function;
};

class signalList_t {
public:
	idList<signal_t> signal[ NUM_SIGNALS ];
};


class idEntity : public idClass {
public:
	static const int		MAX_PVS_AREAS = 4;

	int						entityNumber;			// index into the entity list
	int						entityDefNumber;		// index into the entity def list

	idLinkList<idEntity>	spawnNode;				// for being linked into spawnedEntities list
	idLinkList<idEntity>	activeNode;				// for being linked into activeEntities list

	idLinkList<idEntity>	snapshotNode;			// for being linked into snapshotEntities list
	int						snapshotSequence;		// last snapshot this entity was in
	int						snapshotBits;			// number of bits this entity occupied in the last snapshot

	idStr					name;					// name of entity
	idDict					spawnArgs;				// key/value pairs used to spawn and initialize entity
	idScriptObject			scriptObject;			// contains all script defined data for this entity

	int						thinkFlags;				// TH_? flags
	int						dormantStart;			// time that the entity was first closed off from player
	bool					cinematic;				// during cinematics, entity will only think if cinematic is set

	renderView_t *			renderView;				// for camera views from this entity
	idEntity *				cameraTarget;			// any remoteRenderMap shaders will use this

	idList< idEntityPtr<idEntity> >	targets;		// when this entity is activated these entities entity are activated

	int						health;					// FIXME: do all objects really need health?

	struct entityFlags_s {
		// HUMANHEAD
		bool				noRemoveWhenUnbound	:1;	// Will not be removed when RemoveBinds is called
		bool				refreshReactions	:1;	// This entity wants an update for notifying AI
		bool				touchTriggers		:1;	// This entity touches triggers each frame
		bool				robustDormant		:1;	// More complex dormant test, that tests all possible pvs areas of the entity against all the player
		bool				canBeTractored		:1;	// Tractor beam can attach to it
		bool				tooHeavyForTractor	:1;	// Used on actors that are too heavy to pick up
		bool				isTractored			:1; // rww - entity is currently held by a tractor beam
		bool				onlySpiritWalkTouch	:1;	// Can only be touched in spiritwalk
		bool				allowSpiritWalkTouch:1;	// Can be touched in spiritwalk or not
		bool				applyDamageEffects	:1; // Will have damage effects applied to it
		bool				clientEntity		:1; //this entity should think on the client, and exists only on the client. -rww
		bool				clientEvents		:1; //allows events to be queued on the client -rww
		bool				clientZombie		:1; //an entity which has exited the pvs but still exists on the client -rww
		bool				disposed			:1;	// has been disposed
		bool				ignoreGravityZones	:1;	// do not obey gravity zones no matter what
		bool				noPortal			:1; // cjr:  do not allow this entity to portal
		bool				noJawFlap			:1;	// Never allow jaw flap, optimization
		bool				acceptsWounds		:1;	// entity accepts decals, overlays, and wounds
		bool				accurateGuiTrace	:1; // forces more expensive/accurate gui trace (set on console)
		//HUMANHEAD PCF rww 05/27/06 - force pusher sort
		bool				forcePusherSort		:1; //forces entity to be sorted as a pusher, ignoring status as a bindmaster
						//no more bits left!
		//HUMANHEAD END
		//HUMANHEAD END
		bool				notarget			:1;	// if true never attack or target this entity
		bool				noknockback			:1;	// if true no knockback from hits
		bool				takedamage			:1;	// if true this entity can be damaged
		bool				hidden				:1;	// if true this entity is not visible
		bool				bindOrientated		:1;	// if true both the master orientation is used for binding
		bool				solidForTeam		:1;	// if true this entity is considered solid when a physics team mate pushes entities
		bool				forcePhysicsUpdate	:1;	// if true always update from the physics whether the object moved or not
		bool				selected			:1;	// if true the entity is selected for editing
		bool				neverDormant		:1;	// if true the entity never goes dormant
		bool				isDormant			:1;	// if true the entity is dormant
		bool				hasAwakened			:1;	// before a monster has been awakened the first time, use full PVS for dormant instead of area-connected
		bool				networkSync			:1; // if true the entity is synchronized over the network
	} fl;

	// HUMANHEAD Variables 
public:
	
	idDict *			 	lastTimeSoundPlayed;	// nla - To prevent the same sound from being played too often
	int						spawnHealth;			// jrm - the initial health we were spawned with
	int						lastDamageTime;			// jrm - time stamp of the last time this entity received damage
	int						lastHealTime;			// jrm - time stamp of the last time this entity received health	
	mutable int				nextCallbackTime;		// pdm - for deforms, only needed on rendered entities
	deferredEntityCallback_t	animCallback;		// pdm - original md5 callback, used when deform is present

	float					thinkMS;				// MS spent thinking (for dormancy comparison and profiling)
	float					dormantMS;				// MS spent checking for dormancy (temp for profiling dormancy)
	
	bool					pushes;
	float					pushCosine;

	int						GetNumReactions(void)				const	{return reactions.Num();}
	hhReaction*				GetReaction(int i)							{return reactions[i];}	
protected:
	void					LoadReactions();	// HUMANHEAD JRM: Load this entity's reactions - called from spawn
	idList<hhReaction*>		reactions;			// HUMANHEAD JRM: All of the reactions this entity can send

	hhWoundManagerRenderEntity* woundManager;
	virtual hhWoundManagerRenderEntity* CreateWoundManager() const { return new hhWoundManagerRenderEntity(this); }
	virtual hhWoundManagerRenderEntity* GetWoundManager() { if(!woundManager) { woundManager = CreateWoundManager(); } return woundManager; }
	// HUMANHEAD END

public:
	ABSTRACT_PROTOTYPE( idEntity );

							idEntity();
	virtual					~idEntity();

	void					Spawn( void );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	const char *			GetEntityDefName( void ) const;
	void					SetName( const char *name );
	const char *			GetName( void ) const;
	virtual void			UpdateChangeableSpawnArgs( const idDict *source );

							// clients generate views based on all the player specific options,
							// cameras have custom code, and everything else just uses the axis orientation
	virtual renderView_t *	GetRenderView();

	// HUMANHEAD Methods
	virtual void			LockedGuiReleased(hhPlayer *player) {}
	virtual void			SquishedByDoor(idEntity *door);
	void					ActivatePrefixed(const char *prefix, idEntity *activator);
	virtual void			GetMasterDefaultPosition( idVec3 &masterOrigin, idMat3 &masterAxis ) const;
	void					MoveToJoint( idEntity *master, const char *bonename );  // nla
	void					MoveToJointWeighted( idEntity *master, const char *bonename, idVec3 &weight );  // nla
	void					MoveJointToJoint( const char *ourBone, idEntity *master, const char *masterBone );  // nla
	void					MoveJointToJointOffset( const char *ourBone, idEntity *master, const char *masterBone, idVec3 &offset ); // nla
	void					AlignToJoint( idEntity *master, const char *bonename );  // nla
	void					AlignJointToJoint( const char *ourBone, idEntity *master, const char *masterBone );  // nla
	void					SetSkinByName( const char *skinname );
	virtual void			Ticker(void);							//  HUMANHEAD pdm
	virtual void			DrawDebug(int page);					// HUMANHEAD pdm: to do custom debug drawing when selected
	virtual void			FillDebugVars(idDict *args, int page);	// HUMANHEAD pdm: to show debug variables when selected (GAMEINFO)
	virtual idVec3			GetPortalPoint( void ) { return GetPhysics()->GetOrigin(); } // HUMANHEAD cjr:  The entity will portal when this point crosses the portal plane.  Origin for most, eye location for players
	virtual void			Portalled(idEntity *portal);			// HUMANHEAD:  This entity was just portalled 
	virtual bool			CheckPortal( const idEntity *other, int contentMask ); // HUMANHEAD CJR
	virtual bool			CheckPortal( const idClipModel *mdl, int contentMask ); // HUMANHEAD CJR
	virtual void			CollideWithPortal( const idEntity *other ); // HUMANHEAD CJR PCF 04/26/06
	virtual void			CollideWithPortal( const idClipModel *mdl ); // HUMANHEAD CJR PCF 04/26/06

	int						ModelTraceAttach(idVec3 start, idVec3 direction, idVec3 &hitPoint, idVec3 &hitNormal);

	bool					IsDamaged()		{	return health < GetMaxHealth();	}
	virtual int				GetMaxHealth()	{	return spawnHealth;	}
	int						GetHealth()		{	return health;		}
	virtual void			SetHealth( const int newHealth );
	virtual float			GetHealthLevel();
	virtual float			GetWoundLevel();
	virtual const idMat3&	GetAxis( int id = 0 ) const { return GetPhysics()->GetAxis( id ); }
	virtual const idVec3&	GetOrigin( int id = 0 ) const { return GetPhysics()->GetOrigin( id ); }
	virtual void			SetGravity( const idVec3 &newGravity ) { GetPhysics()->SetGravity( newGravity ); }
	virtual const idVec3&	GetGravity() const { return GetPhysics()->GetGravity(); }
	bool					SwapBindInfo(idEntity *ent2);	// pdm
	virtual bool			AllowCollision(const trace_t &collision);

	idEntity*				GetTeamChain() const { return teamChain; }
	virtual bool			ShouldTouchTrigger( idEntity* entity ) const { return true; }
	bool					IsActive( int flags ) const { return (thinkFlags & flags) != 0; }
	virtual idVec3 			GetAimPosition() const;
	virtual int 			GetChannelForAnim( const char *anim ) const;		// nla
	virtual int				ChannelName2Num( const char *channelName ) const;
	virtual const char *	GetChannelNameForAnim( const char *animName ) const;
	virtual //HUMANHEAD jsh made virtual
	bool					Pushes( ) { return( pushes ); };
	float					PushCosine() { return( pushCosine ); };			
	virtual void			RestoreGUI( const char* guiKey, idUserInterface** gui );
	virtual void			RestoreGUIs();
	virtual void			StartDisposeCountdown();
	idEntity*				PickRandomTarget();
	void					SetSoundMinDistance( float minDistance, const s_channelType channel=SND_CHANNEL_ANY );
	void					SetSoundMaxDistance( float maxDistance, const s_channelType channel=SND_CHANNEL_ANY );
	void					HH_SetSoundVolume( float volume, const s_channelType channel=SND_CHANNEL_ANY );
	float					GetVolume( const s_channelType channel=SND_CHANNEL_ANY );
	void					SetSoundShakes( float shakes, const s_channelType channel=SND_CHANNEL_ANY );
	void					SetSoundShaderFlags( int flags, const s_channelType channel=SND_CHANNEL_ANY );
	void					FadeSoundShader( int to, int over, const s_channelType channel=SND_CHANNEL_ANY );
	virtual void			ConsoleActivated()	{}
	virtual void			ClearTalonTargetType() {}; // CJR - called when a gui is used
	idDict *				GetLastTimeSoundPlayed() { if ( !lastTimeSoundPlayed ) { lastTimeSoundPlayed = new idDict(); } return( lastTimeSoundPlayed ); }		// nla
	void					SetDeformation(int deformType, float parm1, float parm2=0.0f);
	void					SetDeformCallback();
	static bool				MSCallback( renderEntity_s *renderEntity, const renderView_t *renderView, int ms);
	static bool				TenHertzCallback( renderEntity_s *renderEntity, const renderView_t *renderView );
	static bool				TwentyHertzCallback( renderEntity_s *renderEntity, const renderView_t *renderView );
	static bool				ThirtyHertzCallback( renderEntity_s *renderEntity, const renderView_t *renderView );
	static bool				SixtyHertzCallback( renderEntity_s *renderEntity, const renderView_t *renderView );
	bool					IsScaled();
	float					GetScale();
	void					CallNamedEvent(const char *eventName);
	float					GetDoorShaderParm(bool locked, bool startup);

	//Generic Spawn interfaces
	//HUMANHEAD rww - added forceClient
	virtual //HUMANHEAD jsh made virtual
	hhEntityFx*				SpawnFxLocal( const char *fxName, const idVec3 &origin, const idMat3& axis = mat3_identity, const hhFxInfo* const fxInfo = NULL, bool forceClient = false );
	void					SpawnFXPrefixLocal( const idDict* dict, const char* fxKeyPrefix, const idVec3& origin, const idMat3& axis, const hhFxInfo* const fxInfo = NULL, const idEventDef* eventDef = NULL );
	virtual void			BroadcastEventDef( const idEventDef& eventDef );
	virtual void			BroadcastDecl( declType_t type, const char* defName, const idEventDef& eventDef );
	virtual void			BroadcastFx( const char* defName, const idEventDef& eventDef );
	virtual void			BroadcastEntityDef( const char* defName, const idEventDef& eventDef );
	virtual void			BroadcastBeam( const char* defName, const idEventDef& eventDef );
	virtual void			BroadcastFxInfo( const char* fx, const idVec3& origin, const idMat3& axis, const hhFxInfo* const fxInfo = NULL, const idEventDef* eventDef = NULL, bool broadcast = true ); //HUMANHEAD rww - added broadcast
	virtual void			BroadcastFxInfoPrefixed( const char *fxPrefix, const idVec3 &origin, const idMat3& axis = mat3_identity, const hhFxInfo* const fxInfo = NULL, const idEventDef* eventDef = NULL, bool broadcast = true ); // aob
	virtual void			BroadcastFxInfoPrefixedRandom( const char *fxPrefix, const idVec3 &origin, const idMat3& axis = mat3_identity, const hhFxInfo* const fxInfo = NULL, const idEventDef* eventDef = NULL ); // aob
	virtual void			BroadcastFxPrefixedRandom( const char *fxPrefix, const idEventDef &eventDef ); // cjr

	// HH pdm - matter based impact routines
	virtual void			AddLocalMatterWound( jointHandle_t jointNum, const idVec3 &localOrigin, const idVec3 &localNormal, const idVec3 &localDir, int damageDefIndex, const idMaterial *collisionMaterial );
	const char *			DetermineImpactSound(int flags);
	int						PlayImpactSound( const idDict* dict, const idVec3 &origin, surfTypes_t type );

	// HH EVENTS
	//	void					Event_StartSound( const char *action, bool notifyAI = true );
	void					Event_MoveToJoint( idEntity *master, const char *bonename );  // nla
	void					Event_MoveToJointWeighted( idEntity *master, const char *bonename, idVec3 &weight );  // nla
	void					Event_MoveJointToJoint( const char *ourBone, idEntity *master, const char *masterBone );  // nla
	void					Event_MoveJointToJointOffset( const char *ourBone, idEntity *master, const char *masterBone, idVec3 &offset ); // nla
	void					Event_SetSkinByName( const char *skinname );	
	void					Event_SpawnDebris(const char *debrisKey);	
	virtual void			Event_DamageEntity( idEntity *target, const char *damageDefName );// HUMANHEAD JRM
	virtual void			Event_DelayDamageEntity( idEntity *target, const char *damageDefName, float secs );//HUMANHEAD JRM	
	virtual void			Event_Dispose();	
	virtual void			Event_ResetGravity();
	void					Event_LoadReactions();
	void					Event_SetDeformation(int deformType, float parm1, float parm2);
	void					Event_SetFloatKey( const char* key, float value );
	void					Event_SetVectorKey( const char* key, const idVec3& value );
	void					Event_SetEntityKey( const char* key, const idEntity* value );
	void					Event_PlayerCanSee();
	void					Event_SetContents(int contents);
	void					Event_SetClipmask(int clipmask);
	void					Event_SetPortalCollision( int collide ); // cjr
	void					Event_KillBox( void );
	void					Event_Remove( void ); //HUMANHEAD rww

protected:
	void					InitCoreStateInfo();
public:
	// HUMANHEAD END Methods

	// thinking
	virtual void			Think( void );
	bool					CheckDormant( void );	// dormant == on the active list, but out of PVS
	virtual	void			DormantBegin( void );	// called when entity becomes dormant
	virtual	void			DormantEnd( void );		// called when entity wakes from being dormant
	bool					IsActive( void ) const;
	virtual // HUMANHEAD
	void					BecomeActive( int flags );
	void					BecomeInactive( int flags );
	void					UpdatePVSAreas( const idVec3 &pos );

	// visuals
	virtual void			Present( void );
	virtual renderEntity_t *GetRenderEntity( void );
	virtual int				GetModelDefHandle( void );
	virtual void			SetModel( const char *modelname );
	virtual //HUMANHEAD
	void					SetSkin( const idDeclSkin *skin );
	const idDeclSkin *		GetSkin( void ) const;
	virtual //HUMANHEAD
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
	void					UpdateVisuals( void );
	virtual //HUMANHEAD: aob
	void					UpdateModel( void );
	virtual //HUMANHEAD: aob
	void					UpdateModelTransform( void );
	virtual void			ProjectOverlay( const idVec3 &origin, const idVec3 &dir, float size, const char *material );
	int						GetNumPVSAreas( void );
	const int *				GetPVSAreas( void );
	void					ClearPVSAreas( void );
	bool					PhysicsTeamInPVS( pvsHandle_t pvsHandle );

	// animation
	virtual bool			UpdateAnimationControllers( void );
	virtual		// HUMANHEAD nla - Used to test which anims are played in the hand
	bool					UpdateRenderEntity( renderEntity_s *renderEntity, const renderView_t *renderView ) const;
	static bool				ModelCallback( renderEntity_s *renderEntity, const renderView_t *renderView );
	// HUMANHEAD nla - Made return type hhAnimator
	virtual hhAnimator *	GetAnimator( void );	// returns animator object used by this entity
	// aob - made const version
	virtual const hhAnimator *	GetAnimator( void ) const;
	// HUMANHEAD END

	// sound
	virtual bool			CanPlayChatterSounds( void ) const;
	virtual//HUMANHEAD: aob
	bool					StartSound( const char *soundName, const s_channelType channel, int soundShaderFlags = 0, bool broadcast = false, int *length = NULL );
	virtual//HUMANHEAD: aob
	bool					StartSoundShader( const idSoundShader *shader, const s_channelType channel, int soundShaderFlags = 0, bool broadcast = false, int *length = NULL );
	virtual//HUMANHEAD: aob
	void					StopSound( const s_channelType channel, bool broadcast = false);	// pass SND_CHANNEL_ANY to stop all sounds
	void					SetSoundVolume( float volume );
	void					UpdateSound( void );
	int						GetListenerId( void ) const;
	idSoundEmitter *		GetSoundEmitter( void ) const;
	void					FreeSoundEmitter( bool immediate );

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
	virtual			//HUMANHEAD jsh
	idEntity *				GetBindMaster( void ) const;
	jointHandle_t			GetBindJoint( void ) const;
	int						GetBindBody( void ) const;
	idEntity *				GetTeamMaster( void ) const;
	idEntity *				GetNextTeamEntity( void ) const;
	void					ConvertLocalToWorldTransform( idVec3 &offset, idMat3 &axis );
	idVec3					GetLocalVector( const idVec3 &vec ) const;
	idVec3					GetLocalCoordinates( const idVec3 &vec ) const;
	idVec3					GetWorldVector( const idVec3 &vec ) const;
	idVec3					GetWorldCoordinates( const idVec3 &vec ) const;
	bool					GetMasterPosition( idVec3 &masterOrigin, idMat3 &masterAxis ) const;
	void					GetWorldVelocities( idVec3 &linearVelocity, idVec3 &angularVelocity ) const;

	// physics
							// set a new physics object to be used by this entity
	void					SetPhysics( idPhysics *phys );
							// get the physics object used by this entity
	ID_INLINE idPhysics *	GetPhysics( void ) const { return physics; } // HUMANHEAD mdl:  Made inline
							// restore physics pointer for save games
	void					RestorePhysics( idPhysics *phys );
							// run the physics for this entity
	virtual		// HUMANHEAD nla
	bool					RunPhysics( void );
							// set the origin of the physics object (relative to bindMaster if not NULL)
	void					SetOrigin( const idVec3 &org );
							// set the axis of the physics object (relative to bindMaster if not NULL)
	virtual //HUMANHEAD: aob
	void					SetAxis( const idMat3 &axis );
							// use angles to set the axis of the physics object (relative to bindMaster if not NULL)
	void					SetAngles( const idAngles &ang );
							// get the floor position underneath the physics object
	bool					GetFloorPos( float max_dist, idVec3 &floorpos ) const;
							// retrieves the transformation going from the physics origin/axis to the visual origin/axis
	virtual bool			GetPhysicsToVisualTransform( idVec3 &origin, idMat3 &axis );
							// retrieves the transformation going from the physics origin/axis to the sound origin/axis
	virtual bool			GetPhysicsToSoundTransform( idVec3 &origin, idMat3 &axis );
							// called from the physics object when colliding, should return true if the physics simulation should stop
	virtual bool			Collide( const trace_t &collision, const idVec3 &velocity );
							// retrieves impact information, 'ent' is the entity retrieving the info
	virtual void			GetImpactInfo( idEntity *ent, int id, const idVec3 &point, impactInfo_t *info );
							// apply an impulse to the physics object, 'ent' is the entity applying the impulse
	virtual void			ApplyImpulse( idEntity *ent, int id, const idVec3 &point, const idVec3 &impulse );
							// add a force to the physics object, 'ent' is the entity adding the force
	virtual void			AddForce( idEntity *ent, int id, const idVec3 &point, const idVec3 &force );
							// activate the physics object, 'ent' is the entity activating this entity
	virtual void			ActivatePhysics( idEntity *ent );
							// returns true if the physics object is at rest
	virtual bool			IsAtRest( void ) const;
							// returns the time the physics object came to rest
	virtual int				GetRestStartTime( void ) const;
							// add a contact entity
	virtual void			AddContactEntity( idEntity *ent );
							// remove a touching entity
	virtual void			RemoveContactEntity( idEntity *ent );

	// damage
							// returns true if this entity can be damaged from the given origin
	virtual bool			CanDamage( const idVec3 &origin, idVec3 &damagePoint ) const;
							// applies damage to this entity
	virtual	void			Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location );
							// adds a damage effect like overlays, blood, sparks, debris etc.
	virtual void			AddDamageEffect( const trace_t &collision, const idVec3 &velocity, const char *damageDefName, bool broadcast = false ); //HUMANHEAD rww - added broadcast
							// callback function for when another entity received damage from this entity.  damage can be adjusted and returned to the caller.
	virtual void			DamageFeedback( idEntity *victim, idEntity *inflictor, int &damage );
							// notifies this entity that it is in pain
	virtual bool			Pain( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );
							// notifies this entity that is has been killed
	virtual void			Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );

	// scripting
	virtual bool			ShouldConstructScriptObjectAtSpawn( void ) const;
	virtual idThread *		ConstructScriptObject( void );
	virtual void			DeconstructScriptObject( void );
	void					SetSignal( signalNum_t signalnum, idThread *thread, const function_t *function );
	void					ClearSignal( idThread *thread, signalNum_t signalnum );
	void					ClearSignalThread( signalNum_t signalnum, idThread *thread );
	bool					HasSignal( signalNum_t signalnum ) const;
	void					Signal( signalNum_t signalnum );
	void					SignalEvent( idThread *thread, signalNum_t signalnum );

	// gui
	void					TriggerGuis( void );
	bool					HandleGuiCommands( idEntity *entityGui, const char *cmds );
	virtual bool			HandleSingleGuiCommand( idEntity *entityGui, idLexer *src );

	// targets
	void					FindTargets( void );
	void					RemoveNullTargets( void );
	virtual	// HUMANHEAD: made virtual
	void					ActivateTargets( idEntity *activator ) const;
	virtual void			DeactivateTargetsType( const idTypeInfo &classdef ) const;	//HUMANHEAD jsh

	// misc
	virtual void			Teleport( const idVec3 &origin, const idAngles &angles, idEntity *destination );
	bool					TouchTriggers( void ) const;
	idCurve_Spline<idVec3> *GetSpline( void ) const;
	virtual void			ShowEditingDialog( void );

	enum {
		EVENT_STARTSOUNDSHADER,
		EVENT_STOPSOUNDSHADER,
		//HUMANHEAD: aob
		EVENT_ADD_WOUND,
		EVENT_PROJECT_DECAL, //rww added - needs to always be done on the client.
		EVENT_EVENTDEF,
		EVENT_DECL,
		//HUMANEHAD END
		EVENT_MAXEVENTS
	};

	virtual void			ClientPredictionThink( void );
	virtual void			WriteToSnapshot( idBitMsgDelta &msg ) const;
	virtual void			ReadFromSnapshot( const idBitMsgDelta &msg );
	virtual bool			ServerReceiveEvent( int event, int time, const idBitMsg &msg );
	virtual bool			ClientReceiveEvent( int event, int time, const idBitMsg &msg );
	//HUMANHEAD rww
	virtual void			NetZombify(void);
	virtual void			NetResurrect(void);
	//HUMANHEAD END

	void					WriteBindToSnapshot( idBitMsgDelta &msg ) const;
	void					ReadBindFromSnapshot( const idBitMsgDelta &msg );
	void					WriteColorToSnapshot( idBitMsgDelta &msg ) const;
	void					ReadColorFromSnapshot( const idBitMsgDelta &msg );
	void					WriteGUIToSnapshot( idBitMsgDelta &msg ) const;
	void					ReadGUIFromSnapshot( const idBitMsgDelta &msg );

	//HUMANHEAD rww
	void					ServerSendPVSEvent( int eventId, const idBitMsg *msg, const idVec3 &pvsPoint ) const;

	//HUMANHEAD rww - added singleClient and unreliable
	void					ServerSendEvent( int eventId, const idBitMsg *msg, bool saveEvent, int excludeClient, int singleClient = -1, bool unreliable = false ) const;
	void					ClientSendEvent( int eventId, const idBitMsg *msg ) const;

protected:
	renderEntity_t			renderEntity;						// used to present a model to the renderer
	int						modelDefHandle;						// handle to static renderer model
	refSound_t				refSound;							// used to present sound to the audio engine

	//HUMANHEAD: aob
	void					BroadcastEventReroute( int eventId, const idBitMsg *msg, bool saveEvent = false, int excludeClient = -1 );
	//HUMANHEAD END

protected://HUMANHEAD
	idPhysics_Static		defaultPhysicsObj;					// default physics object
	idPhysics *				physics;							// physics used for this entity
	idEntity *				bindMaster;							// entity bound to if unequal NULL
	jointHandle_t			bindJoint;							// joint bound to if unequal INVALID_JOINT
	int						bindBody;							// body bound to if unequal -1
	idEntity *				teamMaster;							// master of the physics team
	idEntity *				teamChain;							// next entity in physics team

	int						numPVSAreas;						// number of renderer areas the entity covers
	int						PVSAreas[MAX_PVS_AREAS];			// numbers of the renderer areas the entity covers

	signalList_t *			signals;

	int						mpGUIState;							// local cache to avoid systematic SetStateInt

protected://HUMANHEAD
	void					FixupLocalizedStrings();

	bool					DoDormantTests( void );				// dormant == on the active list, but out of PVS

	// physics
							// initialize the default physics
	void					InitDefaultPhysics( const idVec3 &origin, const idMat3 &axis );
							// update visual position from the physics
	virtual	// HUMANHEAD made virtual
	void					UpdateFromPhysics( bool moveBack );

	// entity binding
	bool					InitBind( idEntity *master );		// initialize an entity binding
	void					FinishBind( void );					// finish an entity binding
	void					RemoveBinds( void );				// deletes any entities bound to this object
	void					QuitTeam( void );					// leave the current team

	void					UpdatePVSAreas( void );

	// events
	void					Event_GetName( void );
	void					Event_SetName( const char *name );
	void					Event_FindTargets( void );
	void					Event_ActivateTargets( idEntity *activator );
	void					Event_NumTargets( void );
	void					Event_GetTarget( float index );
	void					Event_RandomTarget( const char *ignore );
	void					Event_Bind( idEntity *master );
	void					Event_BindPosition( idEntity *master );
	void					Event_BindToJoint( idEntity *master, const char *jointname, float orientated );
	void					Event_Unbind( void );
	void					Event_RemoveBinds( void );
	void					Event_SpawnBind( void );
	void					Event_SetOwner( idEntity *owner );
	void					Event_SetModel( const char *modelname );
	void					Event_SetSkin( const char *skinname );
	void					Event_GetShaderParm( int parmnum );
	void					Event_SetShaderParm( int parmnum, float value );
	void					Event_SetShaderParms( float parm0, float parm1, float parm2, float parm3 );
	void					Event_SetColor( float red, float green, float blue );
	void					Event_GetColor( void );
	void					Event_IsHidden( void );
	void					Event_Hide( void );
	void					Event_Show( void );
	void					Event_CacheSoundShader( const char *soundName );
	void					Event_StartSoundShader( const char *soundName, int channel );
	void					Event_StopSound( int channel, int netSync );
	void					Event_StartSound( const char *soundName, int channel, int netSync );
	void					Event_FadeSound( int channel, float to, float over );
	void					Event_GetWorldOrigin( void );
	void					Event_SetWorldOrigin( idVec3 const &org );
	void					Event_GetOrigin( void );
	void					Event_SetOrigin( const idVec3 &org );
	void					Event_GetAngles( void );
	void					Event_SetAngles( const idAngles &ang );
	void					Event_SetLinearVelocity( const idVec3 &velocity );
	void					Event_GetLinearVelocity( void );
	void					Event_SetAngularVelocity( const idVec3 &velocity );
	void					Event_GetAngularVelocity( void );
	void					Event_SetSize( const idVec3 &mins, const idVec3 &maxs );
	void					Event_GetSize( void );
	void					Event_GetMins( void );
	void					Event_GetMaxs( void );
	void					Event_Touches( idEntity *ent );
	void					Event_SetGuiParm( const char *key, const char *val );
	void					Event_SetGuiFloat( const char *key, float f );
	void					Event_GetNextKey( const char *prefix, const char *lastMatch );
	void					Event_SetKey( const char *key, const char *value );
	void					Event_GetKey( const char *key );
	void					Event_GetIntKey( const char *key );
	void					Event_GetFloatKey( const char *key );
	void					Event_GetVectorKey( const char *key );
	void					Event_GetEntityKey( const char *key );
	void					Event_RestorePosition( void );
	void					Event_UpdateCameraTarget( void );
	void					Event_DistanceTo( idEntity *ent );
	void					Event_DistanceToPoint( const idVec3 &point );
	void					Event_StartFx( const char *fx );
	void					Event_WaitFrame( void );
	void					Event_Wait( float time );
	void					Event_HasFunction( const char *name );
	void					Event_CallFunction( const char *name );
	virtual // HUMANHEAD jsh
	void					Event_SetNeverDormant( int enable );
#if GAMEPAD_SUPPORT	// VENOM BEGIN
	void					Event_PlayRumbleEffect( int effect );
	void					Event_StopRumbleEffect( void );
#endif // VENOM END
};

/*
===============================================================================

	Animated entity base class.

===============================================================================
*/

typedef struct damageEffect_s {
	jointHandle_t			jointNum;
	idVec3					localOrigin;
	idVec3					localNormal;
	int						time;
	const idDeclParticle*	type;
	struct damageEffect_s *	next;
} damageEffect_t;

//HUMANHEAD: aob - changed inheritance to hhRenderEntity
#include "../prey/game_renderentity.h"
class idAnimatedEntity : public hhRenderEntity {
public:
	CLASS_PROTOTYPE( idAnimatedEntity );

							idAnimatedEntity();
							~idAnimatedEntity();

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	virtual void			ClientPredictionThink( void );
	virtual void			Think( void );

	virtual	//HUMANHEAD jsh
	void					UpdateAnimation( void );

	// HUMANHEAD nla - Make it return our animator
	virtual hhAnimator *	GetAnimator( void );
	// HUMANHEAD END
	virtual void			SetModel( const char *modelname );

	virtual // HUMANHEAD: aob
	bool					GetJointWorldTransform( jointHandle_t jointHandle, int currentTime, idVec3 &offset, idMat3 &axis );
	bool					GetJointTransformForAnim( jointHandle_t jointHandle, int animNum, int currentTime, idVec3 &offset, idMat3 &axis ) const;

	virtual int				GetDefaultSurfaceType( void ) const;
	virtual void			AddDamageEffect( const trace_t &collision, const idVec3 &velocity, const char *damageDefName, bool broadcast = false ); //HUMANHEAD rww - added broadcast
	void					AddLocalDamageEffect( jointHandle_t jointNum, const idVec3 &localPoint, const idVec3 &localNormal, const idVec3 &localDir, const idDeclEntityDef *def, const idMaterial *collisionMaterial );
	void					UpdateDamageEffects( void );

	virtual bool			ClientReceiveEvent( int event, int time, const idBitMsg &msg );

	enum {
		EVENT_ADD_DAMAGE_EFFECT = idEntity::EVENT_MAXEVENTS,
		EVENT_MAXEVENTS
	};

protected:
// HUMANHEAD nla - Make it our animator
#ifdef HUMANHEAD
	hhAnimator				animator;
#else
	idAnimator				animator;
#endif
// HUMANHEAD END
	damageEffect_t *		damageEffects;

private:
	void					Event_GetJointHandle( const char *jointname );
	void 					Event_ClearAllJoints( void );
	void 					Event_ClearJoint( jointHandle_t jointnum );
	void 					Event_SetJointPos( jointHandle_t jointnum, jointModTransform_t transform_type, const idVec3 &pos );
	void 					Event_SetJointAngle( jointHandle_t jointnum, jointModTransform_t transform_type, const idAngles &angles );
	void 					Event_GetJointPos( jointHandle_t jointnum );
	void 					Event_GetJointAngle( jointHandle_t jointnum );
};

#endif /* !__GAME_ENTITY_H__ */
