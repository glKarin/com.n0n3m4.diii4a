/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#ifndef __GAME_ENTITY_H__
#define __GAME_ENTITY_H__

#include "idlib/math/Curve.h"
#include "framework/DeclParticle.h"

#include "physics/Physics_Static.h"
#include "physics/Physics.h"
#include "script/Script_Program.h"
#include "gamesys/Class.h"
#include "gamesys/Event.h"
#include "Game_local.h"
#include "renderer/trackedInteraction.h"

/*
===============================================================================

	Game entity base class.

===============================================================================
*/

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

// SM
extern const idEventDef EV_PostPhysicsRest;

// Think flags
enum {
	TH_ALL					= -1,
	TH_THINK				= 1,		// run think function each frame
	TH_PHYSICS				= 2,		// run physics each frame
	TH_ANIMATE				= 4,		// update animation each frame
	TH_UPDATEVISUALS		= 8,		// update renderEntity
	TH_UPDATEPARTICLES		= 16,
	TH_PHYSICSFROMBIND		= 32,		// blendo eric: physics were activated from a bind event
	TH_DISABLED				= 64,		// blendo eric: disable on destruction
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

	//DOOM BFG
	idLinkList<idEntity>	aimAssistNode;			// linked into gameLocal.aimAssistEntities

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

	bool					dynamicSpectrum;		// Should this object update its spectrum dynamically at an entity level?
	int						spectrum;				// Track which light spectrum to apply to our materials (if dynamicSpectrum is checked)

	struct entityFlags_s {
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
		bool				grabbed				:1;	// if true object is currently being grabbed
		bool				drawGlobally		:1; // always draw this, regardless of whether it would usually be culled
	} fl;

#ifdef _D3XP
	int						timeGroup;

	bool					noGrab;

	renderEntity_t			xrayEntity;
	qhandle_t				xrayEntityHandle;
	const idDeclSkin *		xraySkin;

	void					DetermineTimeGroup( bool slowmo );

	void					SetGrabbedState( bool grabbed );
	bool					IsGrabbed();
#endif


	
	
	
	


public:
	ABSTRACT_PROTOTYPE( idEntity );

							idEntity();
							~idEntity();

	void					Spawn( void );

	void					Save( idSaveGame *savefile ) const; // blendo eric: savegame pass 1
	void					Restore( idRestoreGame *savefile );

	const char *			GetEntityDefName( void ) const;
	void					SetName( const char *name );
	const char *			GetName( void ) const;
	virtual void			UpdateChangeableSpawnArgs( const idDict *source );

							// clients generate views based on all the player specific options,
							// cameras have custom code, and everything else just uses the axis orientation
	virtual renderView_t *	GetRenderView();

	// thinking
	virtual void			Think( void );
	bool					CheckDormant( void );	// dormant == on the active list, but out of PVS
	virtual	void			DormantBegin( void );	// called when entity becomes dormant
	virtual	void			DormantEnd( void );		// called when entity wakes from being dormant
	bool					IsActive( void ) const;
	void					BecomeActive( int flags );
	void					BecomeInactive( int flags );
	void					UpdatePVSAreas( const idVec3 &pos );

	// visuals
	virtual void			Present( void );
	virtual renderEntity_t *GetRenderEntity( void );
	virtual int				GetModelDefHandle( void );
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
	void					UpdateVisuals( void );
	void					UpdateModel( void );
	void					UpdateModelTransform( void );
	virtual void			ProjectOverlay( const idVec3 &origin, const idVec3 &dir, float size, const char *material );
	int						GetNumPVSAreas( void );
	const int *				GetPVSAreas( void );
	void					ClearPVSAreas( void );
	bool					PhysicsTeamInPVS( pvsHandle_t pvsHandle );

	// animation
	virtual bool			UpdateAnimationControllers( void );
	bool					UpdateRenderEntity( renderEntity_s *renderEntity, const renderView_t *renderView );
	static bool				ModelCallback( renderEntity_s *renderEntity, const renderView_t *renderView );
	virtual idAnimator *	GetAnimator( void );	// returns animator object used by this entity

	// sound
	virtual bool			CanPlayChatterSounds( void ) const;
	bool					StartSound( const char *soundName, const s_channelType channel, int soundShaderFlags = 0, bool broadcast = false, int *length = NULL );
	bool					StartSoundShader( const idSoundShader *shader, const s_channelType channel, int soundShaderFlags, bool broadcast, int *length );
	void					StopSound( const s_channelType channel, bool broadcast = false );	// pass SND_CHANNEL_ANY to stop all sounds
	void					SetSoundVolume( float volume );
	void					UpdateSound( void );
	int						GetListenerId( void ) const;
	idSoundEmitter *		GetSoundEmitter( void ) const;
	void					FreeSoundEmitter( bool immediate );
	bool					IsPlayingSound( const s_channelType channel );

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
	idPhysics *				GetPhysics( void ) const;
							// restore physics pointer for save games
	void					RestorePhysics( idPhysics *phys );
							// run the physics for this entity
	bool					RunPhysics( void );
							// set the origin of the physics object (relative to bindMaster if not NULL)
	void					SetOrigin( const idVec3 &org );
							// set the axis of the physics object (relative to bindMaster if not NULL)
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
	virtual	void			Damage( idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType = SURFTYPE_NONE); //BC added materialtype flag.
							// adds a damage effect like overlays, blood, sparks, debris etc.
	virtual void			AddDamageEffect( const trace_t &collision, const idVec3 &velocity, const char *damageDefName );
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
	void					ActivateTargets( idEntity *activator ) const;

	// misc
	virtual void			Teleport( const idVec3 &origin, const idAngles &angles, idEntity *destination );
	bool					TouchTriggers( void ) const;
	idCurve_Spline<idVec3> *GetSpline( void ) const;
	virtual void			ShowEditingDialog( void );

	enum {
		EVENT_STARTSOUNDSHADER,
		EVENT_STOPSOUNDSHADER,
		EVENT_MAXEVENTS
	};

	

	virtual void			ClientPredictionThink( void );
	virtual void			WriteToSnapshot( idBitMsgDelta &msg ) const;
	virtual void			ReadFromSnapshot( const idBitMsgDelta &msg );
	virtual bool			ServerReceiveEvent( int event, int time, const idBitMsg &msg );
	virtual bool			ClientReceiveEvent( int event, int time, const idBitMsg &msg );

	void					WriteBindToSnapshot( idBitMsgDelta &msg ) const;
	void					ReadBindFromSnapshot( const idBitMsgDelta &msg );
	void					WriteColorToSnapshot( idBitMsgDelta &msg ) const;
	void					ReadColorFromSnapshot( const idBitMsgDelta &msg );
	void					WriteGUIToSnapshot( idBitMsgDelta &msg ) const;
	void					ReadGUIFromSnapshot( const idBitMsgDelta &msg );

	void					ServerSendEvent( int eventId, const idBitMsg *msg, bool saveEvent, int excludeClient ) const;
	void					ClientSendEvent( int eventId, const idBitMsg *msg ) const;


	//BC PUBLIC ENTITY
	virtual bool			DoFrob(int index = 0, idEntity * frobber = NULL);
	virtual bool			DoFrobHold(int index = 0, idEntity * frobber = NULL);
	virtual bool			IsFrobHoldable() const { return isFrobHoldable; }
	virtual const idStr&	GetFrobName() const { return displayName; }
	virtual const idStr&	GetFrobNameHold() const { return displayNameHold; }
	bool					isFrobbable;
	idStr					displayName; //name that appears when item is frobbable.
	idStr					displayNameHold; //name that appears when item is frobbable (hold frob, i.e. for ventdoor peeking).
	idLinkList<idEntity>	bafflerNode;			// linked into baffler system
	void					SetDepthHack(bool value);
	void					Event_SetGuiParm(const char *key, const char *val);
	void					Event_SetGuiFloat(const char *key, float f);
	void					Event_SetGuiInt(const char *key, int f);
	void					Event_GuiNamedEvent(int guiNum, const char *event);
	int						maxHealth;
	virtual void			DoRepairTick(int amount);
	virtual void			DoGenericImpulse(int index);
	void					UpdateGravity(bool forceUpdate = false);
	idLinkList<idEntity>	aiSearchNodes;
	idLinkList<idEntity>	interestNode;
    int                     team; // This was in ACTOR. However, we want to allow non-actors to be enemies. So, moving the team var here.
	int						throwTime; //When was this thrown by the player.
	int						healthbarDisplaytime;
	int						cameraiconDisplaytime;
	void					ShowHealthbar();
	void					ShowCameraicon();

	// SW: Callback method for handling tracked interactions, for entities that want to know about the lights they're currently interacting with.
	// Generally this is for light/dark detection and associated features
	static void				UpdateTrackedInteractions(renderEntity_s* renderEnt, idList<trackedInteraction_t>* interactions);


	//Repair node
	bool					needsRepair;
	idVec3					repairWorldposition; //World position. Set via repair_offset spawnflag (X = forward/back Y = left/right   Z = up)
	int						repairrequestTimestamp;
	bool					repairRequestVerified; //In order to be repaired, must have an AI request (notice) the broken thing first.


	idLinkList<idEntity>	confinedNode;
	idLinkList<idEntity>	ventdoorNode;
	idLinkList<idEntity>	repairNode; //for repairable things.
	idLinkList<idEntity>	hatchNode; //for the repair hatches.
	idLinkList<idEntity>	turretNode;
	idLinkList<idEntity>	petNode;
	idLinkList<idEntity>	securitycameraNode; //for security cameras.
	idLinkList<idEntity>	airlockNode;
	idLinkList<idEntity>	catfriendNode;
	idLinkList<idEntity>	landmineNode;
	idLinkList<idEntity>	skullsaverNode;
	idLinkList<idEntity>	catcageNode;
	idLinkList<idEntity>	spacenudgeNode;
	idLinkList<idEntity>	memorypalaceNode;
	idLinkList<idEntity>	dynatipNode;
	idLinkList<idEntity>	idletaskNode;
	idLinkList<idEntity>	windowshutterNode;
	idLinkList<idEntity>	electricalboxNode;
	idLinkList<idEntity>	spectatenodeNode;


	bool					isInOuterSpace(); //is this entity in outer space??

	void					Event_SetGui(int guiNum, const char *guiName);// bc moved to  public.
	void					Event_SetGuiUnique(int guiNum, const char* guiName); // SW: as above, but always pulls a unique gui
	void					Event_PrecacheGui(const char *guiName);// bc moved to  public.
	void					Event_UpdateCameraTarget(void); //bc moved to public.

	void					Event_SetCameraTarget(idEntity *ent);

	int						lastFireattachmentDamagetime;

	virtual void			ListByClassNameDebugDraw() { }

	float					itemDamageVelocityMin;

	bool					DoesPlayerHaveLOStoMe();
	bool					DoesPlayerHaveLOStoMe_Simple();

	virtual void			DoHack(); //for the hackgrenade.

	int						lastHealth; //for the UI. Displays how much damage was last inflicted.
	int						lastHealthTimer; //for the lastHealth UI.

	bool					CanClamber() const { return canClamber; }
	bool					IsFrozen() const { return isFrozen; }

	int						lastDamageTime;


	idStr					locID;

	bool					systemicVoEnabled;
	virtual void			SetPostFlag(customPostFlag_t flag, bool enable);
	void					DynamicUpdateDrawGlobally(bool isGlobal);

	idVec2					infomapPositionOffset;

	virtual void			PostSaveRestore( idRestoreGame * savefile ) {} // blendo eric: allow ents to setup after game save load/restore

	virtual bool			RequiresPresentOnRestore() const { return modelDefHandle != -1; }

	//BC PUBLIC END ENTITY
	


protected:
	renderEntity_t			renderEntity;						// used to present a model to the renderer
	qhandle_t				modelDefHandle;						// handle to static renderer model
	refSound_t				refSound;							// used to present sound to the audio engine

	// unused?
	//idList<trackedInteraction_t>	trackedInteractions;		// List of light interactions that occurred on the last frame (if we're tracking them -- see spawnarg 'trackInteractions')

	// SM BEGIN
	bool					isFrobHoldable;
	// SM END

	//BC PROTECTED
	int						jockeyFrobDebounceMax;
	int						jockeyFrobDebounceTimer;
	bool					canClamber;

	idHashTable<idStr>		GuiParamStr; // blendo eric
	idHashTable<int>		GuiParamInt; // blendo eric
	idHashTable<float>		GuiParamFloat; // blendo eric

private:
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

private:
	void					FixupLocalizedStrings();

	bool					DoDormantTests( void );				// dormant == on the active list, but out of PVS

	// physics
							// initialize the default physics
	void					InitDefaultPhysics( const idVec3 &origin, const idMat3 &axis );
							// update visual position from the physics
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
	void					Event_StopSound( int channel, int netSync);
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
	
	
	void					Event_GetNextKey( const char *prefix, const char *lastMatch );
	void					Event_SetKey( const char *key, const char *value );
	void					Event_GetKey( const char *key );
	void					Event_GetIntKey( const char *key );
	void					Event_GetFloatKey( const char *key );
	void					Event_GetVectorKey( const char *key );
	void					Event_GetEntityKey( const char *key );
	void					Event_RestorePosition( void );
	
	void					Event_DistanceTo( idEntity *ent );
	void					Event_DistanceToPoint( const idVec3 &point );
	void					Event_StartFx( const char *fx );
	void					Event_WaitFrame( void );
	void					Event_Wait( float time );
	void					Event_HasFunction( const char *name );
	void					Event_CallFunction( const char *name );
	void					Event_SetNeverDormant( int enable );
#ifdef _D3XP
	
	
	void					Event_GetGuiParm(int guiNum, const char *key);
	void					Event_GetGuiParmFloat(int guiNum, const char *key);
	
#endif

	//BC PRIVATE
	void					Event_DistanceToPoint2D(const idVec3 &point);
	void					Event_GetIndex(void);
	void					Event_GetRepairPosition(void);
	void					Event_GetCurrentHealth(void);
	void					Event_GetMaxHealth(void);
	void					Event_SetCurrentHealth(int newHealth);
	void					Event_SetMaxHealth(int newHealth);
	void					Event_UpdateGravity(void);

	bool					lastGravityState;
	bool					forceNoGravity;
	bool					zeroGravityNudge; // SW: Whether or not to nudge the object when it enters/exits a vacuum. Disabled for some special circumstances.
	void					Event_SetCinematic(int value);
	virtual void			Event_PostPhysicsRest() { }

	virtual void			Event_PostSpawn();

	void					ApplyDamage(const char *damageDef);
	
	void					UpdateSpectrum(void); // SW

	void					Event_SetFrobbable(int value);

    void                    Event_SetSolid(int enable);

	bool					isFrozen;
	void					Event_SetFrozen( int enable );

	idStr					speakerName;
	
	bool					DamageDefHasDmgType(const idDict *damageDef, idStr filtername);

	void					Event_UpdateDisplayName( char* newName );

	void					Event_RepairEntity();

	void					Event_EntityTeleport(const idVec3 &origin, const idVec3 &angles);

	void					Event_SetSystemicVoEnabled(int value);

	void					SetGui(int guiNum, const char* guiName, bool unique); // SW: intermediary for Event_SetGui and Event_SetGuiUnique

	void					HandleSubtitle(const char* soundName, int length, idStr& sampleName, s_channelType channel);

	void					Event_IsPlayingSound(int channel);

	// SW 3rd March 2025
	// Introducing a tuning factor for idEntity::CanDamage. It's not really 'radius' but I don't know what else to call it.
	float					directDamageRadius;

	//BC PRIVATE END
};

/* blendo eric: made inline
================
idEntity::GetPhysics
================
*/
ID_INLINE idPhysics *idEntity::GetPhysics(void) const {
	return physics;
}

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
	const idDeclParticle*	type = nullptr;
	struct damageEffect_s *	next = nullptr;
} damageEffect_t;

class idAnimatedEntity : public idEntity {
public:
	CLASS_PROTOTYPE( idAnimatedEntity );

							idAnimatedEntity();
							~idAnimatedEntity();

	void					Save( idSaveGame *savefile ) const; // blendo eric: savegame pass 1
	void					Restore( idRestoreGame *savefile );

	virtual void			ClientPredictionThink( void );
	virtual void			Think( void );

	void					UpdateAnimation( void );

	virtual idAnimator *	GetAnimator( void );
	virtual void			SetModel( const char *modelname );

	bool					GetJointWorldTransform( jointHandle_t jointHandle, int currentTime, idVec3 &offset, idMat3 &axis );
	bool					GetJointTransformForAnim( jointHandle_t jointHandle, int animNum, int currentTime, idVec3 &offset, idMat3 &axis ) const;

	virtual int				GetDefaultSurfaceType( void ) const;
	virtual void			AddDamageEffect( const trace_t &collision, const idVec3 &velocity, const char *damageDefName );
	void					AddLocalDamageEffect( jointHandle_t jointNum, const idVec3 &localPoint, const idVec3 &localNormal, const idVec3 &localDir, const idDeclEntityDef *def, const idMaterial *collisionMaterial );
	void					UpdateDamageEffects( void );

	virtual bool			ClientReceiveEvent( int event, int time, const idBitMsg &msg );

	enum {
		EVENT_ADD_DAMAGE_EFFECT = idEntity::EVENT_MAXEVENTS,
		EVENT_MAXEVENTS
	};

	//BC ANIM EVENT PUBLIC
	int						Event_PlayAnim(const char* animname, int animBlendFrames, bool looping = false);

	


protected:
	idAnimator				animator;
	damageEffect_t *		damageEffects;

private:
	void					Event_GetJointHandle( const char *jointname );
	void					Event_ClearAllJoints( void );
	void					Event_ClearJoint( jointHandle_t jointnum );
	void					Event_SetJointPos( jointHandle_t jointnum, jointModTransform_t transform_type, const idVec3 &pos );
	void					Event_SetJointAngle( jointHandle_t jointnum, jointModTransform_t transform_type, const idAngles &angles );
	void					Event_GetJointPos( jointHandle_t jointnum );
	void					Event_GetJointAngle( jointHandle_t jointnum );

	
};

#ifdef _D3XP
class SetTimeState {
	bool					activated;
	bool					previousFast;
	bool					fast;

public:
							SetTimeState();
							SetTimeState( int timeGroup );
							~SetTimeState();

	void					PushState( int timeGroup );
};

ID_INLINE SetTimeState::SetTimeState() {
	activated = false;
	fast = false;
	previousFast = false;
}

ID_INLINE SetTimeState::SetTimeState( int timeGroup ) {
	SetTimeState();
	PushState( timeGroup );
}

ID_INLINE void SetTimeState::PushState( int timeGroup ) {

	// Don't mess with time in Multiplayer
	if ( !gameLocal.isMultiplayer ) {

		activated = true;

		// determine previous fast setting
		if ( gameLocal.time == gameLocal.slow.time ) {
			previousFast = false;
		}
		else {
			previousFast = true;
		}

		// determine new fast setting
		if ( timeGroup ) {
			fast = true;
		}
		else {
			fast = false;
		}

		// set correct time
		if ( fast ) {
			gameLocal.fast.Get( gameLocal.time, gameLocal.previousTime, gameLocal.msec, gameLocal.framenum, gameLocal.realClientTime );
		}
		else {
			gameLocal.slow.Get( gameLocal.time, gameLocal.previousTime, gameLocal.msec, gameLocal.framenum, gameLocal.realClientTime );
		}
	}
}

ID_INLINE SetTimeState::~SetTimeState() {
	if ( activated && !gameLocal.isMultiplayer ) {
		// set previous correct time
		if ( previousFast ) {
			gameLocal.fast.Get( gameLocal.time, gameLocal.previousTime, gameLocal.msec, gameLocal.framenum, gameLocal.realClientTime );
		}
		else {
			gameLocal.slow.Get( gameLocal.time, gameLocal.previousTime, gameLocal.msec, gameLocal.framenum, gameLocal.realClientTime );
		}
	}
}
#endif

#endif /* !__GAME_ENTITY_H__ */
