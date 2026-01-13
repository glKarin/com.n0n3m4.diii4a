#ifndef __GAME_H__
#define __GAME_H__

/*
===============================================================================

	Public game interface with methods to run the game.

===============================================================================
*/

// RAVEN BEGIN
// bgeisler: moved into scripts directory
// default scripts
#define SCRIPT_DEFAULTDEFS			"scripts/defs.script"
#define SCRIPT_DEFAULT				"scripts/main.script"
// RAVEN END
#define SCRIPT_DEFAULTFUNC			"doom_main"

struct gameReturn_t {
	char		sessionCommand[MAX_STRING_CHARS];	// "map", "disconnect", "victory", etc
	int			consistencyHash;					// used to check for network game divergence
	int			health;
	int			heartRate;
	int			stamina;
	int			combat;
	bool		syncNextGameFrame;					// used when cinematics are skipped to prevent session from simulating several game frames to
													// keep the game time in sync with real time
};

enum allowReply_t {
	ALLOW_YES = 0,
	ALLOW_BADPASS,	// core will prompt for password and connect again
	ALLOW_NOTYET,	// core will wait with transmitted message
	ALLOW_NO		// core will abort with transmitted message
};

enum escReply_t {
	ESC_IGNORE = 0,	// do nothing
	ESC_MAIN,		// start main menu GUI
	ESC_GUI			// set an explicit GUI
};

enum demoState_t {
	DEMO_NONE,
	DEMO_RECORDING,
	DEMO_PLAYING
};

enum demoReliableGameMessage_t {
	DEMO_RECORD_CLIENTNUM,
	DEMO_RECORD_EXCLUDE,
	DEMO_RECORD_COUNT
};

//
// these defines work for all startsounds from all entity types
// make sure to change script/doom_defs.script if you add any channels, or change their order
//
typedef enum {
	SND_CHANNEL_ANY = SCHANNEL_ANY,
	SND_CHANNEL_VOICE = SCHANNEL_ONE,
	SND_CHANNEL_VOICE2,
	SND_CHANNEL_BODY,
	SND_CHANNEL_BODY2,
	SND_CHANNEL_BODY3,
	SND_CHANNEL_WEAPON,
	SND_CHANNEL_ITEM,
	SND_CHANNEL_HEART,
	SND_CHANNEL_DEMONIC,
	SND_CHANNEL_RADIO,

	// internal use only.  not exposed to script or framecommands.
	SND_CHANNEL_AMBIENT,
	SND_CHANNEL_DAMAGE

// RAVEN BEGIN
// bdube: added custom to tell us where the end of the predefined list is
	,
	SND_CHANNEL_POWERUP,
	SND_CHANNEL_POWERUP_IDLE,
	SND_CHANNEL_MP_ANNOUNCER,
	SND_CHANNEL_CUSTOM
// RAVEN END
} gameSoundChannel_t;

// RAVEN BEGIN
// bdube: forward reference
class rvClientEffect;
// RAVEN END

struct ClientStats_t {
	bool	isLastPredictFrame;
	bool	isLagged;
	bool	isNewFrame;
};

typedef struct userOrigin_s {
	idVec3	origin;
	int		followClient;
} userOrigin_t;

class idGame {
public:
	virtual						~idGame() {}

	// Initialize the game for the first time.
// RAVEN BEGIN
// jsinger: attempt to eliminate cross-DLL allocation issues
#ifdef RV_UNIFIED_ALLOCATOR
	virtual void				Init( void *(*allocator)( size_t size ), void (*deallocator)( void *ptr ), size_t (*msize)( void *ptr ) ) = 0;
#else
	virtual void				Init( void ) = 0;
#endif

	// Shut down the entire game.
	virtual void				Shutdown( void ) = 0;

	// Set the local client number. Distinguishes listen ( == 0 ) / dedicated ( == -1 )
	virtual void				SetLocalClient( int clientNum ) = 0;

	// Sets the user info for a client.
	// The game can modify the user info in the returned dictionary pointer, server will forward back.
	virtual const idDict *		SetUserInfo( int clientNum, const idDict &userInfo, bool isClient ) = 0;

	// Retrieve the game's userInfo dict for a client.
	virtual const idDict *		GetUserInfo( int clientNum ) = 0;

	// Sets the user info for a viewer.
	// The game can modify the user info in the returned dictionary pointer.
	virtual const idDict *		RepeaterSetUserInfo( int clientNum, const idDict &userInfo ) = 0;

	// Checks to see if a client is active
	virtual bool				IsClientActive( int clientNum ) = 0;

	// The game gets a chance to alter userinfo before they are emitted to server.
	virtual void				ThrottleUserInfo( void ) = 0;

	// Sets the serverinfo at map loads and when it changes.
	virtual void				SetServerInfo( const idDict &serverInfo ) = 0;

	// The session calls this before moving the single player game to a new level.
	virtual const idDict &		GetPersistentPlayerInfo( int clientNum ) = 0;

	// The session calls this right before a new level is loaded.
	virtual void				SetPersistentPlayerInfo( int clientNum, const idDict &playerInfo ) = 0;

	// Loads a map and spawns all the entities.
	virtual void				InitFromNewMap( const char *mapName, idRenderWorld *renderWorld, bool isServer, bool isClient, int randseed ) = 0;

	// Loads a map from a savegame file.
	virtual bool				InitFromSaveGame( const char *mapName, idRenderWorld *renderWorld, idFile *saveGameFile ) = 0;

	// Saves the current game state, the session may have written some data to the file already.
// RAVEN BEGIN
// mekberg: added saveTypes
	virtual void				SaveGame( idFile *saveGameFile, saveType_t saveType = ST_REGULAR ) = 0;
// RAVEN END

	// Shut down the current map.
	virtual void				MapShutdown( void ) = 0;

	// Caches media referenced from in key/value pairs in the given dictionary.
	virtual void				CacheDictionaryMedia( const idDict *dict ) = 0;

	// Spawns the player entity to be used by the client.
	virtual void				SpawnPlayer( int clientNum ) = 0;

// RAVEN BEGIN
	// Runs a game frame, may return a session command for level changing, etc
	// lastCatchupFrame is always true except if we are running several game frames in a row and this one is not the last one
	// subsystems which can tolerate skipping frames will not run during those catchup frames
	// several game frames in a row happen when game + renderer time goes above the tick time ( 16ms )
	virtual gameReturn_t		RunFrame( const usercmd_t *clientCmds, int activeEditors, bool lastCatchupFrame, int serverGameFrame ) = 0;

	virtual void				MenuFrame( void ) = 0;
// RAVEN END

	// Runs a repeater frame
	virtual void				RepeaterFrame( const userOrigin_t *clientOrigins, bool lastCatchupFrame, int serverGameFrame ) = 0;

	// Makes rendering and sound system calls to display for a given clientNum.
	virtual bool				Draw( int clientNum ) = 0;

	// Let the game do it's own UI when ESCAPE is used
	virtual escReply_t			HandleESC( idUserInterface **gui ) = 0;

	// get the games menu if appropriate ( multiplayer )
	virtual idUserInterface *	StartMenu() = 0;

	// When the game is running it's own UI fullscreen, GUI commands are passed through here
	// return NULL once the fullscreen UI mode should stop, or "main" to go to main menu
	virtual const char *		HandleGuiCommands( const char *menuCommand ) = 0;

	// main menu commands not caught in the engine are passed here
	virtual void				HandleMainMenuCommands( const char *menuCommand, idUserInterface *gui ) = 0;

	// Early check to deny connect.
	virtual allowReply_t		ServerAllowClient( int clientId, int numClients, const char *IP, const char *guid, const char *password, const char *privatePassword, char reason[MAX_STRING_CHARS] ) = 0;

	// Connects a client.
	virtual void				ServerClientConnect( int clientNum, const char *guid ) = 0;

	// Spawns the player entity to be used by the client.
	virtual void				ServerClientBegin( int clientNum ) = 0;

	// Disconnects a client and removes the player entity from the game.
	virtual void				ServerClientDisconnect( int clientNum ) = 0;

	// Writes initial reliable messages a client needs to recieve when first joining the game.
	virtual void				ServerWriteInitialReliableMessages( int clientNum ) = 0;

	// Early check to deny connect.
	virtual allowReply_t		RepeaterAllowClient( int clientId, int numClients, const char *IP, const char *guid, bool repeater, const char *password, const char *privatePassword, char reason[MAX_STRING_CHARS] ) = 0;

	// Connects a client.
	virtual void				RepeaterClientConnect( int clientNum ) = 0;

	// Spawns the player entity to be used by the client.
	virtual void				RepeaterClientBegin( int clientNum ) = 0;

	// Disconnects a client and removes the player entity from the game.
	virtual void				RepeaterClientDisconnect( int clientNum ) = 0;

	// Writes initial reliable messages a client needs to recieve when first joining the game.
	virtual void				RepeaterWriteInitialReliableMessages( int clientNum ) = 0;

	// Writes a snapshot of the server game state for the given client.
	virtual void				ServerWriteSnapshot( int clientNum, int sequence, idBitMsg &msg, dword *clientInPVS, int numPVSClients, int lastSnapshotFrame ) = 0;

	// Patches the network entity states at the server with a snapshot for the given client.
	virtual bool				ServerApplySnapshot( int clientNum, int sequence ) = 0;

	// Processes a reliable message from a client.
	virtual void				ServerProcessReliableMessage( int clientNum, const idBitMsg &msg ) = 0;

	// Patches the network entity states at the server with a snapshot for the given client.
	virtual bool				RepeaterApplySnapshot( int clientNum, int sequence ) = 0;

	// Processes a reliable message from a client.
	virtual void				RepeaterProcessReliableMessage( int clientNum, const idBitMsg &msg ) = 0;

	// Reads a snapshot and updates the client game state.
	virtual void				ClientReadSnapshot( int clientNum, int snapshotSequence, const int gameFrame, const int gameTime, const int dupeUsercmds, const int aheadOfServer, const idBitMsg &msg ) = 0;

	// Patches the network entity states at the client with a snapshot.
	virtual bool				ClientApplySnapshot( int clientNum, int sequence ) = 0;

	// Processes a reliable message from the server.
	virtual void				ClientProcessReliableMessage( int clientNum, const idBitMsg &msg ) = 0;

	// Runs prediction on entities at the client.
	virtual gameReturn_t		ClientPrediction( int clientNum, const usercmd_t *clientCmds, bool lastPredictFrame = true, ClientStats_t *cs = NULL ) = 0;

// RAVEN BEGIN
// ddynerman: client game frame
	virtual void				ClientRun( void ) = 0;
	virtual void				ClientEndFrame( void ) = 0;

// jshepard: rcon password check
	virtual void				ProcessRconReturn( bool success ) = 0;

// RAVEN END

	virtual bool				ValidateServerSettings( const char *map, const char *gameType ) = 0;

	// Returns a summary of stats for a given client
	virtual void				GetClientStats( int clientNum, char *data, const int len ) = 0;

	// Switch a player to a particular team
	virtual void				SwitchTeam( int clientNum, int team ) = 0;

	virtual bool				DownloadRequest( const char *IP, const char *guid, const char *paks, char urls[ MAX_STRING_CHARS ] ) = 0;

	// return true to allow download from the built-in http server
	virtual bool				HTTPRequest( const char *IP, const char *file, bool isGamePak ) = 0;

// RAVEN BEGIN
// jscott: for the effects system
	virtual void				StartViewEffect( int type, float time, float scale ) = 0;
	virtual rvClientEffect*		PlayEffect( const idDecl *effect, const idVec3& origin, const idMat3& axis, bool loop = false, const idVec3& endOrigin = vec3_origin, bool broadcast = false, bool predictBit = false, effectCategory_t category = EC_IGNORE, const idVec4& effectTint = vec4_one ) = 0;
	virtual void				GetPlayerView( idVec3 &origin, idMat3 &axis ) = 0;
	virtual const idVec3		GetCurrentGravity( const idVec3& origin, const idMat3& axis ) const = 0;
	virtual void				Translation( trace_t &trace, idVec3 &source, idVec3 &dest, idTraceModel *trm, int clipMask ) = 0;
	virtual void				SpawnClientMoveable ( const char* name, int lifetime, const idVec3& origin, const idMat3& axis, const idVec3& velocity, const idVec3& angular_velocity ) = 0;
// bdube: debugging stuff	
	virtual void				DebugSetString ( const char* name, const char* value ) = 0;
	virtual void				DebugSetFloat ( const char* name, float value ) = 0;
	virtual void				DebugSetInt ( const char* name, int value ) = 0;
	virtual const char*			DebugGetStatString ( const char* name ) = 0;
	virtual int					DebugGetStatInt ( const char* name ) = 0;
	virtual float				DebugGetStatFloat ( const char* name ) = 0;
	virtual bool				IsDebugHudActive ( void ) const = 0;
// rjohnson: for new note taking mechanism
	virtual bool				GetPlayerInfo( idVec3 &origin, idMat3 &axis, int PlayerNum = -1, idAngles *deltaViewAngles = NULL, int reqClientNum = -1 ) = 0;
	virtual void				SetPlayerInfo( idVec3 &origin, idMat3 &axis, int PlayerNum = -1 ) = 0;
	virtual	bool				PlayerChatDisabled( int clientNum ) = 0;
	virtual void				SetViewComments( const char *text = 0 ) = 0;
// ddynerman: utility functions
	virtual void				GetPlayerName( int clientNum, char* name ) = 0;
	virtual void				GetPlayerClan( int clientNum, char* clan ) = 0;
	virtual void				SetFriend( int clientNum, bool isFriend ) = 0;
	virtual const char*			GetLongGametypeName( const char* gametype ) = 0;
	virtual void				ReceiveRemoteConsoleOutput( const char* output ) = 0;
// rjohnson: entity usage stats
	virtual void				ListEntityStats( const idCmdArgs &args ) = 0;
// shouchard:  for ban lists
	virtual void				RegisterClientGuid( int clientNum, const char *guid ) = 0;
	virtual bool				IsMultiplayer( void ) = 0;
// mekberg: added
	virtual bool				InCinematic( void ) = 0;
// mekberg: so banlist can be populated outside of multiplayer game
	virtual void				PopulateBanList( idUserInterface* hud ) = 0;
	virtual void				RemoveGuidFromBanList( const char *guid ) = 0;
// mekberg: interface
	virtual void				AddGuidToBanList( const char *guid ) = 0;
	virtual const char*			GetGuidByClientNum( int clientNum ) = 0;
// jshepard: updating player post-menu
	virtual void				UpdatePlayerPostMainMenu( void ) = 0;
	virtual void				ResetRconGuiStatus( void ) = 0;
// RAVEN END

// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
#if defined(_RV_MEM_SYS_SUPPORT)
	virtual void				FlushBeforelevelLoad( void ) = 0;
#endif
// RAVEN END

	// Set the demo state.
	virtual void				SetDemoState( demoState_t state, bool serverDemo, bool timeDemo ) = 0;

	// Set the repeater state; engine will call this with true for isRepeater if this is a repeater, and true for serverIsRepeater if we are connected to a repeater
	virtual void				SetRepeaterState( bool isRepeater, bool serverIsRepeater ) = 0;

	// Writes current network info to a file (used as initial state for demo recording).
	virtual void				WriteNetworkInfo( idFile* file, int clientNum ) = 0;

	// Reads current network info from a file (used as initial state for demo playback).
	virtual void				ReadNetworkInfo( int gameTime, idFile* file, int clientNum ) = 0;

	// Let gamecode decide if it wants to accept demos from older releases of the engine.
	virtual bool				ValidateDemoProtocol( int minor_ref, int minor ) = 0;

	// Write a snapshot for server demo recording.
	virtual void				ServerWriteServerDemoSnapshot( int sequence, idBitMsg &msg, int lastSnapshotFrame ) = 0;

	// Read a snapshot from a server demo stream.
	virtual void				ClientReadServerDemoSnapshot( int sequence, const int gameFrame, const int gameTime, const idBitMsg &msg ) = 0;

	// Write a snapshot for repeater clients.
	virtual void				RepeaterWriteSnapshot( int clientNum, int sequence, idBitMsg &msg, dword *clientInPVS, int numPVSClients, const userOrigin_t &pvs_origin, int lastSnapshotFrame ) = 0;

	// Done writing snapshots for repeater clients.
	virtual void				RepeaterEndSnapshots( void ) = 0;

	// Read a snapshot from a repeater stream.
	virtual void				ClientReadRepeaterSnapshot( int sequence, const int gameFrame, const int gameTime, const int aheadOfServer, const idBitMsg &msg ) = 0;

	// Get the currently followed client in demo playback
	virtual int					GetDemoFollowClient( void ) = 0;

	// Build a bot's userCmd
	virtual void				GetBotInput( int clientNum, usercmd_t &userCmd ) = 0;

	// Return the name of a gui to override the loading screen
	virtual const char *		GetLoadingGui( const char *mapDeclName ) = 0;

	// Set any additional gui variables needed by the loading screen
	virtual void				SetupLoadingGui( idUserInterface *gui ) = 0;
};

extern idGame *					game;


/*
===============================================================================

	Public game interface with methods for in-game editing.

===============================================================================
*/

struct refSound_t {
// RAVEN BEGIN
	int							referenceSoundHandle;	// this is the interface to the sound system, created
														// with idSoundWorld::AllocSoundEmitter() when needed
// RAVEN END
	idVec3						origin;
// RAVEN BEGIN
// jscott: for Miles doppler
	idVec3						velocity;
// RAVEN END
	int							listenerId;		// SSF_PRIVATE_SOUND only plays if == listenerId from PlaceListener
												// no spatialization will be performed if == listenerID
	const idSoundShader *		shader;			// this really shouldn't be here, it is a holdover from single channel behavior
	float						diversity;		// 0.0 to 1.0 value used to select which
												// samples in a multi-sample list from the shader are used
	bool						waitfortrigger;	// don't start it at spawn time
	soundShaderParms_t			parms;			// override volume, flags, etc
};

enum {
	TEST_PARTICLE_MODEL = 0,
	TEST_PARTICLE_IMPACT,
	TEST_PARTICLE_MUZZLE,
	TEST_PARTICLE_FLIGHT,
	TEST_PARTICLE_SELECTED
};

class idEntity;
class idMD5Anim;
// RAVEN BEGIN
// bdube: more forward declarations
class idProgram;
class idInterpreter;
class idThread;

typedef void (*debugInfoProc_t) ( const char* classname, const char* name, const char* value, void *userdata );
// RAVEN END

// FIXME: this interface needs to be reworked but it properly separates code for the time being
class idGameEdit {
public:
	virtual						~idGameEdit( void ) {}

	// These are the canonical idDict to parameter parsing routines used by both the game and tools.
	virtual bool				ParseSpawnArgsToRenderLight( const idDict *args, renderLight_t *renderLight );
	virtual void				ParseSpawnArgsToRenderEntity( const idDict *args, renderEntity_t *renderEntity );
	virtual void				ParseSpawnArgsToRefSound( const idDict *args, refSound_t *refSound );

	// Animation system calls for non-game based skeletal rendering.
	virtual idRenderModel *		ANIM_GetModelFromEntityDef( const char *classname );
	virtual const idVec3 		&ANIM_GetModelOffsetFromEntityDef( const char *classname );
	virtual idRenderModel *		ANIM_GetModelFromEntityDef( const idDict *args );
	virtual idRenderModel *		ANIM_GetModelFromName( const char *modelName );
	virtual const idMD5Anim *	ANIM_GetAnimFromEntityDef( const char *classname, const char *animname );
// RAVEN BEGIN
// bdube: added
// scork: added 'const' qualifiers so other stuff would compile
	virtual const idMD5Anim *	ANIM_GetAnimFromEntity( const idEntity* ent, int animNum );
	virtual float				ANIM_GetAnimPlaybackRateFromEntity ( idEntity* ent, int animNum );
	virtual const char*			ANIM_GetAnimNameFromEntity ( const idEntity* ent, int animNum );
// RAVEN END
	virtual int					ANIM_GetNumAnimsFromEntityDef( const idDict *args );
	virtual const char *		ANIM_GetAnimNameFromEntityDef( const idDict *args, int animNum );
	virtual const idMD5Anim *	ANIM_GetAnim( const char *fileName );
	virtual int					ANIM_GetLength( const idMD5Anim *anim );
	virtual int					ANIM_GetNumFrames( const idMD5Anim *anim );
// RAVEN BEGIN
// bdube: added
	virtual const char *		ANIM_GetFilename( const idMD5Anim* anim );
	virtual int					ANIM_ConvertFrameToTime ( const idMD5Anim* anim, int frame );
	virtual int					ANIM_ConvertTimeToFrame ( const idMD5Anim* anim, int time );
// RAVEN END
	virtual void				ANIM_CreateAnimFrame( const idRenderModel *model, const idMD5Anim *anim, int numJoints, idJointMat *frame, int time, const idVec3 &offset, bool remove_origin_offset );
	virtual idRenderModel *		ANIM_CreateMeshForAnim( idRenderModel *model, const char *classname, const char *animname, int frame, bool remove_origin_offset );

// RAVEN BEGIN
// mekberg: access to animationlib functions for radiant
	virtual void				FlushUnusedAnims( void );
// RAVEN END

	// Articulated Figure calls for AF editor and Radiant.
	virtual bool				AF_SpawnEntity( const char *fileName );
	virtual void				AF_UpdateEntities( const char *fileName );
	virtual void				AF_UndoChanges( void );
	virtual idRenderModel *		AF_CreateMesh( const idDict &args, idVec3 &meshOrigin, idMat3 &meshAxis, bool &poseIsSet );


	// Entity selection.
	virtual void				ClearEntitySelection( void );
	virtual int					GetSelectedEntities( idEntity *list[], int max );
	virtual void				AddSelectedEntity( idEntity *ent );

	// Selection methods
	virtual void				TriggerSelected();

	// Entity defs and spawning.
	virtual const idDict *		FindEntityDefDict( const char *name, bool makeDefault = true ) const;
	virtual void				SpawnEntityDef( const idDict &args, idEntity **ent );
	virtual idEntity *			FindEntity( const char *name ) const;
	virtual const char *		GetUniqueEntityName( const char *classname ) const;

	// Entity methods.
	virtual void				EntityGetOrigin( idEntity *ent, idVec3 &org ) const;
	virtual void				EntityGetAxis( idEntity *ent, idMat3 &axis ) const;
	virtual void				EntitySetOrigin( idEntity *ent, const idVec3 &org );
	virtual void				EntitySetAxis( idEntity *ent, const idMat3 &axis );
	virtual void				EntityTranslate( idEntity *ent, const idVec3 &org );
// RAVEN BEGIN
// scork: const-qualified 'ent' so other things would compile
	virtual const idDict *		EntityGetSpawnArgs( const idEntity *ent ) const;
// RAVEN END
	virtual void				EntityUpdateChangeableSpawnArgs( idEntity *ent, const idDict *dict );
	virtual void				EntityChangeSpawnArgs( idEntity *ent, const idDict *newArgs );
	virtual void				EntityUpdateVisuals( idEntity *ent );
	virtual void				EntitySetModel( idEntity *ent, const char *val );
	virtual void				EntityStopSound( idEntity *ent );
	virtual void				EntityDelete( idEntity *ent );
	virtual void				EntitySetColor( idEntity *ent, const idVec3 color );
// RAVEN BEGIN
// bdube: added
	virtual const char*			EntityGetName ( idEntity* ent ) const;
	virtual int					EntityToSafeId( idEntity* ent ) const;
	virtual idEntity *			EntityFromSafeId( int safeID) const;
	virtual void				EntitySetSkin ( idEntity *ent, const char* temp ) const;
	virtual void				EntityClearSkin ( idEntity *ent ) const;
	virtual void				EntityShow ( idEntity* ent ) const;
	virtual void				EntityHide ( idEntity* ent ) const;
	virtual void				EntityGetBounds ( idEntity* ent, idBounds &bounds ) const;
	virtual int					EntityPlayAnim ( idEntity* ent, int animNum, int time, int blendtime );
	virtual void				EntitySetFrame ( idEntity* ent, int animNum, int frame, int time, int blendtime );
	virtual void				EntityStopAllEffects ( idEntity* ent );
	virtual void				EntityGetDelta ( idEntity* ent, int fromTime, int toTime, idVec3& delta ); 
	virtual void				EntityRemoveOriginOffset ( idEntity* ent, bool remove );
	virtual const char*			EntityGetClassname ( idEntity* ent ) const;
	virtual bool				EntityIsDerivedFrom ( idEntity* ent, const char* classname ) const;
	virtual renderEntity_t*		EntityGetRenderEntity ( idEntity* ent );
// scork: accessor functions for various utils
	virtual	idEntity *			EntityGetNextTeamEntity( idEntity *pEnt ) const;
	virtual void				GetPlayerInfo( idVec3 &v3Origin, idMat3 &mat3Axis, int PlayerNum = -1, idAngles *deltaViewAngles = NULL ) const;
	virtual void				SetPlayerInfo( idVec3 &v3Origin, idMat3 &mat3Axis, int PlayerNum = -1 ) const;
	virtual void				EntitySetName( idEntity* pEnt, const char *psName );
// RAVEN END

	// Player methods.
	virtual bool				PlayerIsValid() const;
	virtual void				PlayerGetOrigin( idVec3 &org ) const;
	virtual void				PlayerGetAxis( idMat3 &axis ) const;
	virtual void				PlayerGetViewAngles( idAngles &angles ) const;
	virtual void				PlayerGetEyePosition( idVec3 &org ) const;
// RAVEN BEGIN
// bdube: new game edit stuff
	virtual bool				PlayerTraceFromEye ( trace_t &results, float length, int contentMask );

	// Effect methods
	virtual void				EffectRefreshTemplate ( const idDecl *effect ) const;

	// Light entity methods
	virtual void				LightSetParms ( idEntity* ent, int maxLevel, int currentLevel, float radius );

	// Common editing functions
	virtual int					GetGameTime ( int *previous = NULL ) const;
	virtual void				SetGameTime	( int time ) const;
	virtual bool				TracePoint ( trace_t &results, const idVec3 &start, const idVec3 &end, int contentMask ) const;
	virtual void				CacheDictionaryMedia ( const idDict* dict ) const;
	virtual void				SetCamera ( idEntity* camera ) const;
// RAVEN BEGIN
// bdube: added
	virtual int					GetGameEntityRegisterTime ( void ) const;
	virtual idEntity*			GetFirstSpawnedEntity ( void ) const;
	virtual idEntity*			GetNextSpawnedEntity ( idEntity* from ) const; 
// jscott: added
	virtual	void				DrawPlaybackDebugInfo( void );
	virtual	void				RecordPlayback( const usercmd_t &cmd, idEntity *source );
	virtual	bool				PlayPlayback( void );
	virtual	void				ShutdownPlaybacks( void );
// RAVEN END	
	
	// Script methods
	virtual int					ScriptGetStatementLineNumber ( idProgram* program, int instructionPointer ) const;
	virtual const char*			ScriptGetStatementFileName ( idProgram* program, int instructionPointer ) const;
	virtual int					ScriptGetStatementOperator ( idProgram* program, int instructionPointer ) const;
	virtual void*				ScriptGetCurrentFunction ( idInterpreter* interpreter ) const;
	virtual const char*			ScriptGetCurrentFunctionName ( idInterpreter* interpreter ) const;
	virtual int					ScriptGetCallstackDepth ( idInterpreter* interpreter ) const;
	virtual void*				ScriptGetCallstackFunction ( idInterpreter* interpreter, int depth ) const;
	virtual const char*			ScriptGetCallstackFunctionName ( idInterpreter* interpreter, int depth ) const;
	virtual int					ScriptGetCallstackStatement ( idInterpreter* interpreter, int depth ) const;
	virtual bool				ScriptIsReturnOperator ( int op ) const;
	virtual const char*			ScriptGetRegisterValue ( idInterpreter* interpreter, const char* varname, int callstackDepth ) const;
	virtual idThread*			ScriptGetThread ( idInterpreter* interpreter ) const;
	
	// Thread methods
	virtual int					ThreadGetCount ( void );
	virtual idThread*			ThreadGetThread ( int index );
	virtual const char*			ThreadGetName ( idThread* thread );
	virtual int					ThreadGetNumber ( idThread* thread );
	virtual const char*			ThreadGetState ( idThread* thread );
	
	// Class externals for entity viewer
	virtual void				GetClassDebugInfo ( const idEntity* entity, debugInfoProc_t proc, void* userdata );

	// In game map editing support.
	virtual const idDict *		MapGetEntityDict( const char *name ) const;
	virtual void				MapSave( const char *path = NULL ) const;
// RAVEN BEGIN
// rjohnson: added entity export
	virtual bool				MapHasExportEntities( void ) const;
// scork: simple func for the sound editor
	virtual const char*			MapLoaded( void ) const;
// cdr: AASTactical
	virtual idAASFile*			GetAASFile( int i );
// jscott: added entries for memory tracking
	virtual void				PrintMemInfo( MemInfo *mi );
	virtual size_t				ScriptSummary( const idCmdArgs &args ) const;
	virtual size_t				ClassSummary( const idCmdArgs &args ) const;
	virtual size_t				EntitySummary( const idCmdArgs &args ) const;
// RAVEN END
	virtual void				MapSetEntityKeyVal( const char *name, const char *key, const char *val ) const ;
	virtual void				MapCopyDictToEntity( const char *name, const idDict *dict ) const;
	virtual int					MapGetUniqueMatchingKeyVals( const char *key, const char *list[], const int max ) const;
	virtual void				MapAddEntity( const idDict *dict ) const;
	virtual int					MapGetEntitiesMatchingClassWithString( const char *classname, const char *match, const char *list[], const int max ) const;
	virtual void				MapRemoveEntity( const char *name ) const;
	virtual void				MapEntityTranslate( const char *name, const idVec3 &v ) const;
};

extern idGameEdit *				gameEdit;

// RAVEN BEGIN
// bdube: game logging
/*
===============================================================================

	Game Log.

===============================================================================
*/
class rvGameLog {
public:
	virtual				~rvGameLog( void ) {}

	virtual void		Init		( void ) = 0;
	virtual void		Shutdown	( void ) = 0;

	virtual void		BeginFrame	( int time ) = 0;
	virtual void		EndFrame	( void ) = 0;

	virtual	void		Set			( const char* keyword, int value ) = 0;
	virtual void		Set			( const char* keyword, float value ) = 0;
	virtual void		Set			( const char* keyword, const char* value ) = 0;
	virtual void		Set			( const char* keyword, bool value ) = 0;
	
	virtual void		Add			( const char* keyword, int value ) = 0;
	virtual void		Add			( const char* keyword, float value ) = 0;
};

extern rvGameLog *				gameLog;

#define GAMELOG_SET(x,y)		{if(g_gamelog.GetBool())gameLog->Set ( x, y );}
#define GAMELOG_ADD(x,y)		{if(g_gamelog.GetBool())gameLog->Add ( x, y );}

#define GAMELOG_SET_IF(x,y,z)	{if(g_gamelog.GetBool()&&(z))gameLog->Set ( x, y );}
#define GAMELOG_ADD_IF(x,y,z)	{if(g_gamelog.GetBool()&&(z))gameLog->Add ( x, y );}

// RAVEN END

/*
===============================================================================

	Game API.

===============================================================================
*/

// 4: network demos
// 5: fix idNetworkSystem ( memory / DLL boundary related )
// 6: more network demo APIs
// 7: cleanups
// 8: added some demo functions to the FS class
// 9: bump up for 1.1 patch
// 9: Q4 Gold
// 10: Patch 2 changes
// 14: 1.3
// 26: 1.4 beta
// 30: 1.4
// 37: 1.4.2
const int GAME_API_VERSION		= 37;

struct gameImport_t {

	int							version;				// API version
	idSys *						sys;					// non-portable system services
	idCommon *					common;					// common
	idCmdSystem *				cmdSystem;				// console command system
	idCVarSystem *				cvarSystem;				// console variable system
	idFileSystem *				fileSystem;				// file system
	idNetworkSystem *			networkSystem;			// network system
	idRenderSystem *			renderSystem;			// render system
	idSoundSystem *				soundSystem;			// sound system
	idRenderModelManager *		renderModelManager;		// render model manager
	idUserInterfaceManager *	uiManager;				// user interface manager
	idDeclManager *				declManager;			// declaration manager
	idAASFileManager *			AASFileManager;			// AAS file manager
	idCollisionModelManager *	collisionModelManager;	// collision model manager

// RAVEN BEGIN
// jscott:
	rvBSEManager *				bse;					// Raven effects system
// RAVEN END

// RAVEN BEGIN
// dluetscher: added the following members to exchange memory system data
#ifdef _RV_MEM_SYS_SUPPORT
	rvHeapArena *				heapArena;								// main heap arena that all other heaps use
	rvHeap *					systemHeapArray[MAX_SYSTEM_HEAPS];		// array of pointers to rvHeaps that are common to idLib, Game, and executable 
#endif
// RAVEN END
};

struct gameExport_t {

	int							version;				// API version
	idGame *					game;					// interface to run the game
	idGameEdit *				gameEdit;				// interface for in-game editing
// RAVEN BEGIN
// bdube: added
	rvGameLog *					gameLog;				// interface for game logging
// RAVEN END	
};

extern "C" {
typedef gameExport_t * (*GetGameAPI_t)( gameImport_t *import );
}

#endif /* !__GAME_H__ */
