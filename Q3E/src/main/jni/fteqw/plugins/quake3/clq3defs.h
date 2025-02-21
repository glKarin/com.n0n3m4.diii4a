#ifndef _Q3DEFS_H_
#define _Q3DEFS_H_

void Q3_SetKeyCatcher(int newcatcher);
int Q3_GetKeyCatcher(void);

int StringKey( const char *string, int length );

typedef struct {
	qboolean	allsolid;	// if true, plane is not valid
	qboolean	startsolid;	// if true, the initial point was in a solid area
	float		fraction;	// time completed, 1.0 = didn't hit anything
	vec3_t		endpos;		// final position
	cplane_t	plane;		// surface normal at impact, transformed to world space
	int			surfaceFlags;	// surface hit
	int			contents;	// contents on other side of surface hit
	int			entityNum;	// entity the contacted surface is a part of
} q3trace_t;

#define	MAX_Q3_STATS				16
#define	MAX_Q3_PERSISTANT			16
#define	MAX_Q3_POWERUPS				16
#define	MAX_Q3_WEAPONS				16		
#define	MAX_PS_EVENTS				2
typedef struct q3playerState_s {
	int			commandTime;	// cmd->serverTime of last executed command
	int			pm_type;
	int			bobCycle;		// for view bobbing and footstep generation
	int			pm_flags;		// ducked, jump_held, etc
	int			pm_time;

	vec3_t		origin;
	vec3_t		velocity;
	int			weaponTime;
	int			gravity;
	int			speed;
	int			delta_angles[3];	// add to command angles to get view direction
									// changed by spawns, rotating objects, and teleporters

	int			groundEntityNum;// ENTITYNUM_NONE = in air

	int			legsTimer;		// don't change low priority animations until this runs out
	int			legsAnim;		// mask off ANIM_TOGGLEBIT

	int			torsoTimer;		// don't change low priority animations until this runs out
	int			torsoAnim;		// mask off ANIM_TOGGLEBIT

	int			movementDir;	// a number 0 to 7 that represents the reletive angle
								// of movement to the view angle (axial and diagonals)
								// when at rest, the value will remain unchanged
								// used to twist the legs during strafing

	vec3_t		grapplePoint;	// location of grapple to pull towards if PMF_GRAPPLE_PULL

	int			eFlags;			// copied to entityState_t->eFlags

	int			eventSequence;	// pmove generated events
	int			events[MAX_PS_EVENTS];
	int			eventParms[MAX_PS_EVENTS];

	int			externalEvent;	// events set on player from another source
	int			externalEventParm;
	int			externalEventTime;

	int			clientNum;		// ranges from 0 to MAX_CLIENTS-1
	int			weapon;			// copied to entityState_t->weapon
	int			weaponstate;

	vec3_t		viewangles;		// for fixed views
	int			viewheight;

	// damage feedback
	int			damageEvent;	// when it changes, latch the other parms
	int			damageYaw;
	int			damagePitch;
	int			damageCount;

	int			stats[MAX_Q3_STATS];
	int			persistant[MAX_Q3_PERSISTANT];	// stats that aren't cleared on death
	int			powerups[MAX_Q3_POWERUPS];	// level.time that the powerup runs out
	int			ammo[MAX_Q3_WEAPONS];
	int			generic1;
	int			loopSound;
	int			jumppad_ent;	// jumppad entity hit this frame
	// not communicated over the net at all
	int			ping;			// server to game info for scoreboard
	int			pmove_framecount;	// FIXME: don't transmit over the network
	int			jumppad_frame;
	int			entityEventSequence;

} q3playerState_t;



typedef enum {
	TR_STATIONARY,
	TR_INTERPOLATE,				// non-parametric, but interpolate between snapshots
	TR_LINEAR,
	TR_LINEAR_STOP,
	TR_SINE,					// value = base + sin( time / duration ) * delta
	TR_GRAVITY
} trType_t;
typedef struct {
	trType_t	trType;
	int		trTime;
	int		trDuration;			// if non 0, trTime + trDuration = stop time
	vec3_t	trBase;
	vec3_t	trDelta;			// velocity, etc
} trajectory_t;
typedef struct q3entityState_s {
	int		number;			// entity index
	int		eType;			// entityType_t
	int		eFlags;

	trajectory_t	pos;	// for calculating position
	trajectory_t	apos;	// for calculating angles

	int		time;
	int		time2;

	vec3_t	origin;
	vec3_t	origin2;

	vec3_t	angles;
	vec3_t	angles2;

	int		otherEntityNum;	// shotgun sources, etc
	int		otherEntityNum2;

	int		groundEntityNum;	// -1 = in air

	int		constantLight;	// r + (g<<8) + (b<<16) + (intensity<<24)
	int		loopSound;		// constantly loop this sound

	int		modelindex;
	int		modelindex2;
	int		clientNum;		// 0 to (MAX_CLIENTS - 1), for players and corpses
	int		frame;

	int		solid;			// for client side prediction, trap_linkentity sets this properly

	int		event;			// impulse events -- muzzle flashes, footsteps, etc
	int		eventParm;


	// for players
	int		powerups;		// bit flags
	int		weapon;			// determines weapon and flash model, etc
	int		legsAnim;		// mask off ANIM_TOGGLEBIT
	int		torsoAnim;		// mask off ANIM_TOGGLEBIT

	int		generic1;

} q3entityState_t;

#define MAX_MAP_AREA_BYTES 32

#define MAX_ENTITIES_IN_SNAPSHOT 256
//This struct is exposed to the cgame
typedef struct snapshot_s {
	int				snapFlags;			// SNAPFLAG_RATE_DELAYED, etc
	int				ping;

	int				serverTime;		// server time the message is valid for (in msec)

	qbyte			areamask[MAX_MAP_AREA_BYTES];		// portalarea visibility bits

	q3playerState_t	ps;						// complete information about the current player at this time

	int				numEntities;			// all of the entities that need to be presented
	q3entityState_t	entities[MAX_ENTITIES_IN_SNAPSHOT];	// at the time of this snapshot

	int				numServerCommands;		// text based server commands to execute when this
	int				serverCommandSequence;	// snapshot becomes current
} snapshot_t;
#define	SNAPFLAG_NOT_ACTIVE		2



//this struct is used internally, using a ringbuffer for entities instead of worst-case memory usage.
typedef struct clientSnap_s {
	qboolean		valid;			// cleared if delta parsing was invalid
	int				snapFlags;
	int				serverMessageNum;
	int				serverCommandNum;
	int				serverTime;		// server time the message is valid for (in msec)
	int				deltaFrame;
	qbyte			areabits[MAX_MAP_AREA_BYTES];		// portalarea visibility bits
	q3playerState_t	playerstate;
	int				numEntities;
	int				firstEntity;	// non-masked index into cl.parseEntities[] array
	int				ping;
} clientSnap_t;

// for ping calculation, rate estimation, usercmd delta'ing
typedef struct frame_s {
	int		userCmdNumber;
	int		serverMessageNum;
	int		clientTime;
	int		serverTime;
} q3frame_t;

typedef struct {
	int				serverTime;
	int				angles[3];
	int 			buttons;
	qbyte			weapon;           // weapon
	signed char	forwardmove, rightmove, upmove;
} q3usercmd_t;
#define Q3CMD_BACKUP 64		//number of q3usercmd_ts that the client can queue before acks
#define Q3CMD_MASK	 (Q3CMD_BACKUP-1)

#define Q3MAX_PARSE_ENTITIES 2048
#define Q3PARSE_ENTITIES_MASK (Q3MAX_PARSE_ENTITIES-1)

#define	MAX_Q3_GAMESTATE_CHARS	16000
#define	MAX_STRING_CHARS	1024
	#define MAX_Q3_CONFIGSTRINGS 1024
	#define Q3TEXTCMD_BACKUP	64			//number of reliable text commands that can be queued, must be power of two
#define Q3TEXTCMD_MASK		(Q3TEXTCMD_BACKUP-1)

#define CFGSTR_SYSINFO 1

	#define MODELINDEX_BITS 8
#define GENTITYNUM_BITS 10
#define	MAX_GENTITIES		(1<<GENTITYNUM_BITS)
#define ENTITYNUM_NONE (MAX_GENTITIES-1)

typedef struct {
	cactive_t	state;
	double time;
	netchan_t netchan;
	unsigned int challenge;
	int lastClientCommandNum;
	int lastServerCommandNum;
	int numClientCommands;

	int	servercount;	//bumped on map changes
	int	serverMessageNum;

	int downloadchunknum;

	int firstParseEntity;

	int playernum;
	int fs_key;

	//this stuff gets inserted into usercmd_t messages.
	int selected_weapon;

	qdownload_t *download;
	model_t *worldmodel;
	model_t *model_precache[1u<<MODELINDEX_BITS];

	clientSnap_t snapshots[Q3UPDATE_BACKUP];
	clientSnap_t snap;

	q3entityState_t parseEntities[Q3MAX_PARSE_ENTITIES];

	q3entityState_t baselines[MAX_GENTITIES];

	char		clientCommands[Q3TEXTCMD_BACKUP][MAX_STRING_CHARS];
	char		serverCommands[Q3TEXTCMD_BACKUP][MAX_STRING_CHARS];
} ClientConnectionState_t;
extern ClientConnectionState_t ccs;


typedef enum {
	svcq3_bad,
	svcq3_nop,
	svcq3_gamestate,
	svcq3_configstring,
	svcq3_baseline,
	svcq3_serverCommand,
	svcq3_download,
	svcq3_snapshot,
	svcq3_eom
} svc_ops_t;

typedef enum {
	clcq3_bad,
	clcq3_nop,
	clcq3_move,
	clcq3_nodeltaMove,
	clcq3_clientCommand,
	clcq3_eom
} clc_ops_t;











//fonts... *sigh*...
#define GLYPH_START 0
#define GLYPH_END 255
#define GLYPH_CHARSTART 32
#define GLYPH_CHAREND 127
#define GLYPHS_PER_FONT GLYPH_END - GLYPH_START + 1
typedef struct {
  int height;       // number of scan lines
  int top;          // top of glyph in buffer
  int bottom;       // bottom of glyph in buffer
  int pitch;        // width for copying
  int xSkip;        // x adjustment
  int imageWidth;   // width of actual image
  int imageHeight;  // height of actual image
  float s;          // x offset in image where glyph starts
  float t;          // y offset in image where glyph starts
  float s2;
  float t2;
  int glyph;  // handle to the shader with the glyph
  char shaderName[32];
} glyphInfo_t;
typedef struct {
  glyphInfo_t glyphs [GLYPHS_PER_FONT];
  float glyphScale;
  char name[OLD_MAX_QPATH];
} fontInfo_t;
void UI_RegisterFont(char *fontName, int pointSize, fontInfo_t *font);
qboolean UI_OpenMenu(void);

int UI_Cin_Play(const char *name, int x, int y, int w, int h, unsigned int flags);
int UI_Cin_Stop(int idx);
int UI_Cin_Run(int idx);
int UI_Cin_Draw(int idx);
int UI_Cin_SetExtents(int idx, int x, int y, int w, int h);

typedef struct {
	char		renderer_string[MAX_STRING_CHARS];
	char		vendor_string[MAX_STRING_CHARS];
	char		version_string[MAX_STRING_CHARS];
	char		extensions_string[8192];

	int			maxTextureSize;         // queried from GL
	int			numTextureUnits;        // multitexture ability

	int			colorBits, depthBits, stencilBits;

	int			driverType;	//glDriverType_t
	int			hardwareType;	//glHardwareType_t

	qboolean	deviceSupportsGamma;
	int			textureCompression;	//textureCompression_t
	qboolean	textureEnvAddAvailable;

	int			vidWidth, vidHeight;
	float		windowAspect;

	int			displayFrequency;

	qboolean	isFullscreen;
	qboolean	stereoEnabled;
	qboolean	smpActive;
} q3glconfig_t;


void Netchan_SetupQ3(unsigned int flags, netchan_t *chan, netadr_t *adr, int qport);
void Netchan_TransmitNextFragment(struct ftenet_connections_s *socket, netchan_t *chan);
void Netchan_TransmitQ3(struct ftenet_connections_s *socket, netchan_t *chan, int length, const qbyte *data);
qboolean Netchan_ProcessQ3 (netchan_t *chan, sizebuf_t *msg);

qboolean MSG_Q3_ReadDeltaEntity(const q3entityState_t *from, q3entityState_t *to, int number);
void MSGQ3_WriteDeltaEntity(sizebuf_t *msg, const q3entityState_t *from, const q3entityState_t *to, qboolean force);
void MSG_Q3_ReadDeltaPlayerstate(const q3playerState_t *from, q3playerState_t *to);
void MSGQ3_WriteDeltaPlayerstate(sizebuf_t *msg, const q3playerState_t *from, const q3playerState_t *to);

void MSG_Q3_ReadDeltaUsercmd(int key, const usercmd_t *from, usercmd_t *to);


void MSG_WriteBits(sizebuf_t *msg, int value, int bits);



typedef struct q3refEntity_s q3refEntity_t;
void VQ3_AddEntity(const q3refEntity_t *q3);
typedef struct q3polyvert_s q3polyvert_t;
void VQ3_AddPolys(shader_t *s, int num, q3polyvert_t *verts, size_t count);
typedef struct q3refdef_s q3refdef_t;
void VQ3_RenderView(const q3refdef_t *ref);




qboolean CG_FillQ3Snapshot(int snapnum, snapshot_t *snapshot);
void CG_ClearGameState(void);
void CG_InsertIntoGameState(int num, const char *str);
const char *CG_GetConfigString(int num);
void CG_Restart(void);
void UI_Restart_f(void);
vm_t *UI_GetUIVM(void);
int Script_LoadFile(char *filename);
void Script_Get_File_And_Line(int handle, char *filename, int *line);
int Script_Read(int handle, struct pc_token_s *token);
void Script_Free(int handle);


void VARGS CLQ3_SendClientCommand(const char *fmt, ...) LIKEPRINTF(1);
void CLQ3_SendAuthPacket(struct ftenet_connections_s *socket, netadr_t *gameserver);
void CLQ3_SendConnectPacket(struct ftenet_connections_s *socket, netadr_t *to, int challenge, int qport, infobuf_t *userinfo);
void CLQ3_Established(void);
void CLQ3_Disconnect(struct ftenet_connections_s *socket);
void CLQ3_SendCmd(struct ftenet_connections_s *socket, usercmd_t *cmd, unsigned int movesequence, double gametime);
int CLQ3_ParseServerMessage (sizebuf_t *msg);

void CG_Stop (void);
void CG_Start (void);
int CG_Refresh(double time);
qboolean CG_ConsoleCommand(void);
qboolean CG_KeyPressed(int key, int unicode, int down);
unsigned int CG_GatherLoopingSounds(vec3_t *positions, unsigned int *entnums, sfx_t **sounds, unsigned int max);

qboolean UI_IsRunning(void);
qboolean UI_ConsoleCommand(void);
void UI_Init (void);
void UI_Start (void);
void UI_Stop (void);
qboolean UI_OpenMenu(void);
void UI_Reset(void);

#ifdef HAVE_SERVER
void SVQ3_ShutdownGame(qboolean restarting);
qboolean SVQ3_InitGame(server_static_t *server_state_static, server_t *server_state, qboolean restart);
qboolean SVQ3_ConsoleCommand(void);
qboolean SVQ3_PrefixedConsoleCommand(void);
qboolean SVQ3_HandleClient(netadr_t *from, sizebuf_t *msg);
void SVQ3_DirectConnect(netadr_t *from, sizebuf_t *msg);
void SVQ3_NewMapConnects(void);
void SVQ3_DropClient(struct client_s *cl);
void SVQ3_RunFrame(void);
void SVQ3_SendMessage(struct client_s *client);
qboolean SVQ3_RestartGamecode(void);
void SVQ3_ServerinfoChanged(const char *key);
#endif
#endif
