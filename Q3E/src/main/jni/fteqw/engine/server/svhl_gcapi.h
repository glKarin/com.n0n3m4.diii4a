/*supported halflife API versions:
138
140
*/
#if HLSERVER >= 1
	#define HALFLIFE_API_VERSION HLSERVER
#else
	#define HALFLIFE_API_VERSION 140
#endif

#ifndef QDECL
	#ifdef _MSC_VER
		#define QDECL _cdecl
	#else
		#define QDECL
	#endif
#endif

typedef long hllong; //long is 64bit on amd64+linux, not sure that's what valve meant, but lets keep it for compatibility.
typedef struct hledict_s hledict_t;
typedef unsigned long hlintptr_t;	//this may be problematic. CRestore::ReadField needs a fix. Or 20.
typedef unsigned long hlcrc_t;	//supposed to be 32bit... *sigh*

typedef struct
{
	int allsolid;
	int startsolid;
	int inopen;
	int inwater;
	float fraction;
	vec3_t endpos;
	float planedist;
	vec3_t planenormal;
	hledict_t *touched;
	int hitgroup;
} hltraceresult_t;

#if HALFLIFE_API_VERSION <= 138
typedef struct
{
	int etype;
	int number;
	int flags;
	vec3_t origin;
	vec3_t angles;
	int modelindex;
	int sequence1;
	float frame;
	int colourmap;
	short skin;
	short solid;
	int effects;
	float scale;
	int rendermode;
	int renderamt;
	int rendercolour;
	int renderfx;
	int movetype;
	float frametime;
	float framerate;
	int body;
	qbyte controller[4];
	qbyte blending[2];
	short padding;
	vec3_t velocity;
	vec3_t mins;
	vec3_t maxs;
	int aiment;
} hlbaseline_t;
#endif

typedef struct
{
	string_t	classname;
	string_t	globalname;

	vec3_t	origin;
	vec3_t	oldorigin;
	vec3_t	velocity;
vec3_t	basevelocity;
vec3_t	clbasevelocity;

	vec3_t	movedir;

	vec3_t	angles;
	vec3_t	avelocity;
	vec3_t	punchangles;
	vec3_t	v_angle;

#if HALFLIFE_API_VERSION > 138
vec3_t	endpos;
vec3_t	startpos;
float	impacttime;
float	starttime;
#endif

	int		fixangle;

	float	ideal_pitch;
	float	pitch_speed;
	float	ideal_yaw;
	float	yaw_speed;


	int		modelindex;
	string_t	model;
int		vmodelindex;
int		vwmodelindex;

	vec3_t	absmin;
	vec3_t	absmax;
	vec3_t	mins;
	vec3_t	maxs;
	vec3_t	size;

	float	ltime;
	float	nextthink;
	int		movetype;
	int		solid;

	int	skin;
int		body;
	int		effects;

float	gravity;
float	friction;

int		light_level;

int		sequence1;
int		sequence2;
	float	frame;
float	framestarttime;
float	framerate;
qbyte	controller[4];
qbyte	blending[2];
float	scale;

int		rendermode;
float	renderamt;
vec3_t	rendercolour;
int		renderfx;

	float	health;
	float	frags;
int weapons;
	float	takedamage;
	int		deadflag;
	vec3_t	view_ofs;

int buttons;
	int impulse;

	hledict_t *chain;
	hledict_t *dmg_inflictor;
	hledict_t *enemy;
	hledict_t *aiment;
	hledict_t *owner;
	hledict_t *groundentity;


	int spawnflags;
	int flags;

	int	colormap;
	int	team;

	float	max_health;
	float	teleport_time;
	float	armortype;
	float	armorvalue;
	int	waterlevel;
	int	watertype;

	string_t	target;
	string_t	targetname;
	string_t	netname;
	string_t	message;	//WARNING: hexen2 uses a float and not a string

	float	dmg_take;
	float	dmg_save;
	float	dmg;
	float	dmg_time;

	string_t	noise;
	string_t	noise1;
	string_t	noise2;
	string_t	noise3;

float speed;
float air_finished;
float pain_finished;
float radsuit_finished;

hledict_t *edict;
#if HALFLIFE_API_VERSION > 138
int		playerclass;
float	maxspeed;
float	fov;
int		weaponanim;
int		msec;
int		ducking;
int		timestepsound;
int		swimtime;
int		ducktime;
int		stepleft;
float	fallvelocity;
int		gamestate;
int		oldbuttons;
int		groupinfo;

int		customi0;
int		customi1;
int		customi2;
int		customi3;
float		customf0;
float		customf1;
float		customf2;
float		customf3;
vec3_t		customv0;
vec3_t		customv1;
vec3_t		customv2;
vec3_t		customv3;
hledict_t *custome0;
hledict_t *custome1;
hledict_t *custome2;
hledict_t *custome3;
#endif
} hlentvars_t;

struct hledict_s
{
	qboolean	isfree;
	int			spawnnumber;
	link_t		area;				// linked to a division node or leaf

#if HALFLIFE_API_VERSION > 138
	int			headnode;
	int			num_leafs;
#define HLMAX_ENT_LEAFS 48
	short		leafnums[HLMAX_ENT_LEAFS];
#else
	int			num_leafs;
#define HLMAX_ENT_LEAFS 24
	short		leafnums[HLMAX_ENT_LEAFS];

	hlbaseline_t	baseline;
#endif

	float		freetime; // sv.time when the object was freed

	void		*moddata;
	hlentvars_t	v;
};

#define unk void
/*
#define	FCVAR_ARCHIVE		1	// set to cause it to be saved to vars.rc
#define	FCVAR_USERINFO		2	// changes the client's info string
#define	FCVAR_SERVERINFO	4	// notifies players when changed
#define FCVAR_SERVERDLL		8	// defined by external DLL
#define FCVAR_CLIENTDLL     16  // defined by the client dll
#define HLCVAR_PROTECTED     32  // It's a server cvar, but we don't send the data since it's a password, etc.  Sends 1 if it's not bland/zero, 0 otherwise as value
#define HLCVAR_SPONLY        64  // This cvar cannot be changed by clients connected to a multiplayer server.
*/
typedef struct hlcvar_s
{
	char	*name;
	char	*string;
	int		flags;
	float	value;
	struct hlcvar_s *next;
} hlcvar_t;

typedef struct
{
	char *classname;
	char *key;
	char *value;
	hllong handled;
} hlfielddef_t;



typedef struct
{
//	int	self;
//	int	other;
//	int	world;
	float	time;
	float	frametime;	
	float	force_retouch;
	string_t	mapname;
string_t startspot;
	float	deathmatch;
	float	coop;
	float	teamplay;
	float	serverflags;
//	float	total_secrets;
//	float	total_monsters;
	float	found_secrets;
//	float	killed_monsters;
//	float parms1, parm2, parm3, parm4, parm;
	vec3_t	v_forward;
	vec3_t	v_up;
	vec3_t	v_right;
	float	trace_allsolid;
	float	trace_startsolid;
	float	trace_fraction;
	vec3_t	trace_endpos;
	vec3_t	trace_plane_normal;
	float	trace_plane_dist;
	int	trace_ent;
	float	trace_inopen;
	float	trace_inwater;
int trace_hitgroup;
int trace_flags;
	int	msg_entity;
int audiotrack;
int maxclients;
int maxentities;

char *stringbase;
void *savedata;
vec3_t landmark;
//43...
} SVHL_Globals_t;




//http://metamod.org/dllapi_notes.html
typedef struct
{
	void (QDECL *GameDLLInit)(void);
    int (QDECL *DispatchSpawn)(hledict_t *ed);
    void (QDECL *DispatchThink)(hledict_t *ed);
    unk (QDECL *DispatchUse)(unk);
    void (QDECL *DispatchTouch)(hledict_t *e1, hledict_t *e2);
    void (QDECL *DispatchBlocked)(hledict_t *self, hledict_t *other);
    void (QDECL *DispatchKeyValue)(hledict_t *ed, hlfielddef_t *fdef);
    unk (QDECL *DispatchSave)(unk);
    unk (QDECL *DispatchRestore)(unk);
    unk (QDECL *DispatchObjectCollsionBox)(unk);
    unk (QDECL *SaveWriteFields)(unk);
    unk (QDECL *SaveReadFields)(unk);
    unk (QDECL *SaveGlobalState)(unk);
    unk (QDECL *RestoreGlobalState)(unk);
    unk (QDECL *ResetGlobalState)(unk);
    qboolean (QDECL *ClientConnect)(hledict_t *ed, char *name, char *ip, char reject[128]);
    void (QDECL *ClientDisconnect)(hledict_t *ed);
    void (QDECL *ClientKill)(hledict_t *ed);
    void (QDECL *ClientPutInServer)(hledict_t *ed);
    void (QDECL *ClientCommand)(hledict_t *ed);
    unk (QDECL *ClientUserInfoChanged)(unk);
    void (QDECL *ServerActivate)(hledict_t *edictlist, int numedicts, int numplayers);
#if HALFLIFE_API_VERSION > 138
    unk (QDECL *ServerDeactivate)(unk);
#endif
    void (QDECL *PlayerPreThink)(hledict_t *ed);
    void (QDECL *PlayerPostThink)(hledict_t *ed);
    unk (QDECL *StartFrame)(unk);
    unk (QDECL *ParmsNewLevel)(unk);
    unk (QDECL *ParmsChangeLevel)(unk);
    unk (QDECL *GetGameDescription)(unk);
    unk (QDECL *PlayerCustomization)(unk);
    unk (QDECL *SpectatorConnect)(unk);
    unk (QDECL *SpectatorDisconnect)(unk);
    unk (QDECL *SpectatorThink)(unk);
	//138
#if HALFLIFE_API_VERSION > 138
    unk (QDECL *Sys_Error)(unk);
    unk (QDECL *PM_Move)(unk);
    unk (QDECL *PM_Init)(unk);
    unk (QDECL *PM_FindTextureType)(unk);
    unk (QDECL *SetupVisibility)(unk);
    unk (QDECL *UpdateClientData)(unk);
    unk (QDECL *AddToFullPack)(unk);
    unk (QDECL *CreateBaseline)(unk);
    unk (QDECL *RegisterEncoders)(unk);
    unk (QDECL *GetWeaponData)(unk);
    unk (QDECL *CmdStart)(unk);
    unk (QDECL *CmdEnd)(unk);
    unk (QDECL *ConnectionlessPacket)(unk);
    unk (QDECL *GetHullBounds)(unk);
    unk (QDECL *CreateInstancedBaselines)(unk);
    unk (QDECL *InconsistentFile)(unk);
    unk (QDECL *AllowLagCompensation)(unk);
#endif
} SVHL_GameFuncs_t;

//http://metamod.org/newapi_notes.html
struct 
{
	unk (QDECL *OnFreeEntPrivateData)(unk);
    unk (QDECL *GameShutdown)(unk);
    unk (QDECL *ShouldCollide)(unk);
    unk (QDECL *CvarValue)(unk);
    unk (QDECL *CvarValue2 )(unk);
} *SVHL_GameFuncsEx;

// http://metamod.org/engine_notes.html
typedef struct
{
	int (QDECL *PrecacheModel)(char *name);
	int (QDECL *PrecacheSound)(char *name);
	void (QDECL *SetModel)(hledict_t *ed, char *modelname);
	unk (QDECL *ModelIndex)(unk);
	int (QDECL *ModelFrames)(int midx);
	void (QDECL *SetSize)(hledict_t *ed, float *mins, float *maxs);
	void (QDECL *ChangeLevel)(char *nextmap, char *startspot);
	unk (QDECL *GetSpawnParms)(unk);
	unk (QDECL *SaveSpawnParms)(unk);
	float (QDECL *VecToYaw)(float *inv);
	void (QDECL *VecToAngles)(float *inv, float *outa);
	void (QDECL *MoveToOrigin)(hledict_t *ent, vec3_t dest, float dist, int moveflags);
	unk (QDECL *ChangeYaw)(unk);
	unk (QDECL *ChangePitch)(unk);
	hledict_t *(QDECL *FindEntityByString)(hledict_t *last, char *field, char *value);
	int (QDECL *GetEntityIllum)(hledict_t *ent);
	hledict_t *(QDECL *FindEntityInSphere)(hledict_t *last, float *org, float radius);
	hledict_t *(QDECL *FindClientInPVS)(hledict_t *ed);
	unk (QDECL *EntitiesInPVS)(unk);
	void (QDECL *MakeVectors)(float *angles);
	void (QDECL *AngleVectors)(float *angles, float *forward, float *right, float *up);
	hledict_t *(QDECL *CreateEntity)(void);
	void (QDECL *RemoveEntity)(hledict_t *ed);
	hledict_t *(QDECL *CreateNamedEntity)(string_t name);
	unk (QDECL *MakeStatic)(unk);
	unk (QDECL *EntIsOnFloor)(unk);
	int (QDECL *DropToFloor)(hledict_t *ed);
	int (QDECL *WalkMove)(hledict_t *ed, float yaw, float dist, int mode);
	void (QDECL *SetOrigin)(hledict_t *ed, float *neworg);
	void (QDECL *EmitSound)(hledict_t *ed, int chan, char *soundname, float vol, float atten, int flags, int pitch);
	void (QDECL *EmitAmbientSound)(hledict_t *ed, float *org, char *samp, float vol, float attenuation, unsigned int flags, int pitch);
	void (QDECL *TraceLine)(float *start, float *end, int flags, hledict_t *ignore, hltraceresult_t *result);
	unk (QDECL *TraceToss)(unk);
	unk (QDECL *TraceMonsterHull)(unk);
	void (QDECL *TraceHull)(float *start, float *end, int flags, int hullnum, hledict_t *ignore, hltraceresult_t *result);
	unk (QDECL *TraceModel)(unk);
	char *(QDECL *TraceTexture)(hledict_t *againstent, vec3_t start, vec3_t end);
	unk (QDECL *TraceSphere)(unk);
	unk (QDECL *GetAimVector)(unk);
	void (QDECL *ServerCommand)(char *cmd);
	void (QDECL *ServerExecute)(void);
	unk (QDECL *ClientCommand)(unk);
	unk (QDECL *ParticleEffect)(unk);
	void (QDECL *LightStyle)(int stylenum, char *stylestr);
	int (QDECL *DecalIndex)(char *decalname);
	int (QDECL *PointContents)(float *org);
	void (QDECL *MessageBegin)(int dest, int svc, float *org, hledict_t *ent);
	void (QDECL *MessageEnd)(void);
	void (QDECL *WriteByte)(int value);
	void (QDECL *WriteChar)(int value);
	void (QDECL *WriteShort)(int value);
	void (QDECL *WriteLong)(int value);
	void (QDECL *WriteAngle)(float value);
	void (QDECL *WriteCoord)(float value);
	void (QDECL *WriteString)(char *string);
	void (QDECL *WriteEntity)(int entnum);
	void (QDECL *CVarRegister)(hlcvar_t *hlvar);
	float (QDECL *CVarGetFloat)(char *vname);
	char *(QDECL *CVarGetString)(char *vname);
	void (QDECL *CVarSetFloat)(char *vname, float v);
	void (QDECL *CVarSetString)(char *vname, char *v);
	void (QDECL *AlertMessage)(int level, char *fmt, ...);
	void (QDECL *EngineFprintf)(FILE *f, char *fmt, ...);
	void *(QDECL *PvAllocEntPrivateData)(hledict_t *ed, long quant);
	unk (QDECL *PvEntPrivateData)(unk);
	unk (QDECL *FreeEntPrivateData)(unk);
	unk (QDECL *SzFromIndex)(unk);
	string_t (QDECL *AllocString)(char *string);
	unk (QDECL *GetVarsOfEnt)(unk);
	hledict_t * (QDECL *PEntityOfEntOffset)(int ednum);
	int (QDECL *EntOffsetOfPEntity)(hledict_t *ed);
	int (QDECL *IndexOfEdict)(hledict_t *ed);
	hledict_t *(QDECL *PEntityOfEntIndex)(int idx);
	unk (QDECL *FindEntityByVars)(unk);
	void *(QDECL *GetModelPtr)(hledict_t *ed);
	int (QDECL *RegUserMsg)(char *msgname, int msgsize);
	unk (QDECL *AnimationAutomove)(unk);
	void (QDECL *GetBonePosition)(hledict_t *ed, int bone, vec3_t org, vec3_t ang);
	hlintptr_t (QDECL *FunctionFromName)(char *name);
	char *(QDECL *NameForFunction)(hlintptr_t);
	unk (QDECL *ClientPrintf)(unk);
	void (QDECL *ServerPrint)(char *msg);
	char *(QDECL *Cmd_Args)(void);
	char *(QDECL *Cmd_Argv)(int argno);
	int (QDECL *Cmd_Argc)(void);
	unk (QDECL *GetAttachment)(unk);
	void (QDECL *CRC32_Init)(hlcrc_t *crc);
	void (QDECL *CRC32_ProcessBuffer)(hlcrc_t *crc, qbyte *p, int len);
	void (QDECL *CRC32_ProcessByte)(hlcrc_t *crc, qbyte b);
	hlcrc_t (QDECL *CRC32_Final)(hlcrc_t crc);
	long (QDECL *RandomLong)(long minv, long maxv);
	float (QDECL *RandomFloat)(float minv, float maxv);
	unk (QDECL *SetView)(unk);
	unk (QDECL *Time)(unk);
	unk (QDECL *CrosshairAngle)(unk);
	void *(QDECL *LoadFileForMe)(char *name, int *size_out);
	void (QDECL *FreeFile)(void *fptr);
	unk (QDECL *EndSection)(unk);
	int (QDECL *CompareFileTime)(char *fname1, char *fname2, int *result);
	void (QDECL *GetGameDir)(char *gamedir);
	unk (QDECL *Cvar_RegisterVariable)(unk);
	unk (QDECL *FadeClientVolume)(unk);
	unk (QDECL *SetClientMaxspeed)(unk);
	unk (QDECL *CreateFakeClient)(unk);
	unk (QDECL *RunPlayerMove)(unk);
	unk (QDECL *NumberOfEntities)(unk);
	char *(QDECL *GHL_GetInfoKeyBuffer)(hledict_t *ed);
	char *(QDECL *GHL_InfoKeyValue)(char *infostr, char *key);
	unk (QDECL *SetKeyValue)(unk);
	unk (QDECL *SetClientKeyValue)(unk);
	unk (QDECL *IsMapValid)(unk);
	unk (QDECL *StaticDecal)(unk);
	unk (QDECL *PrecacheGeneric)(unk);
	int (QDECL *GetPlayerUserId)(hledict_t *ed);
	unk (QDECL *BuildSoundMsg)(unk);
	unk (QDECL *IsDedicatedServer)(unk);
	//138
#if HALFLIFE_API_VERSION > 138
	hlcvar_t *(QDECL *CVarGetPointer)(char *varname);
	unk (QDECL *GetPlayerWONId)(unk);
	unk (QDECL *Info_RemoveKey)(unk);
	unk (QDECL *GetPhysicsKeyValue)(unk);
	void (QDECL *SetPhysicsKeyValue)(hledict_t *ent, char *key, char *value);
	unk (QDECL *GetPhysicsInfoString)(unk);
	unsigned short (QDECL *PrecacheEvent)(int eventtype, char *eventname);
	void (QDECL *PlaybackEvent)(int flags, hledict_t *ent, unsigned short eventidx, float delay, float *origin, float *angles, float f1, float f2, int i1, int i2, int b1, int b2);
	unk (QDECL *SetFatPVS)(unk);
	unk (QDECL *SetFatPAS)(unk);
	unk (QDECL *CheckVisibility)(unk);
	unk (QDECL *DeltaSetField)(unk);
	unk (QDECL *DeltaUnsetField)(unk);
	unk (QDECL *DeltaAddEncoder)(unk);
	unk (QDECL *GetCurrentPlayer)(unk);
	int (QDECL *CanSkipPlayer)(hledict_t *playerent);
	unk (QDECL *DeltaFindField)(unk);
	unk (QDECL *DeltaSetFieldByIndex)(unk);
	unk (QDECL *DeltaUnsetFieldByIndex)(unk);
	unk (QDECL *SetGroupMask)(unk);
	unk (QDECL *CreateInstancedBaseline)(unk);
	unk (QDECL *Cvar_DirectSet)(unk);
	unk (QDECL *ForceUnmodified)(unk);
	unk (QDECL *GetPlayerStats)(unk);
	unk (QDECL *AddServerCommand)(unk);
	//
	unk (QDECL *Voice_GetClientListening)(unk);
	qboolean (QDECL *Voice_SetClientListening)(int listener, int sender, int shouldlisten);
	//140
	char *(QDECL *GetPlayerAuthId)(hledict_t *playerent);
	unk (QDECL *SequenceGet)(unk);
	unk (QDECL *SequencePickSentence)(unk);
	unk (QDECL *GetFileSize)(unk);
	unk (QDECL *GetApproxWavePlayLen)(unk);
	unk (QDECL *IsCareerMatch)(unk);
	unk (QDECL *GetLocalizedStringLength)(unk);
	unk (QDECL *RegisterTutorMessageShown)(unk);
	unk (QDECL *GetTimesTutorMessageShown)(unk);
	unk (QDECL *ProcessTutorMessageDecayBuffer)(unk);
	unk (QDECL *ConstructTutorMessageDecayBuffer)(unk);
	unk (QDECL *ResetTutorMessageDecayData)(unk);
	unk (QDECL *QueryClientCvarValue)(unk);
	unk (QDECL *QueryClientCvarValue2)(unk);
#endif

	int check;	//added so I can be sure parameters match this list, etc
} SVHL_Builtins_t;

extern SVHL_Globals_t SVHL_Globals;
extern SVHL_GameFuncs_t SVHL_GameFuncs;

extern hledict_t *SVHL_Edict;
extern int SVHL_NumActiveEnts;


void QDECL GHL_RemoveEntity(hledict_t *ed);

void SVHL_LinkEdict (hledict_t *ent, qboolean touch_triggers);
void SVHL_UnlinkEdict (hledict_t *ent);
hledict_t	*SVHL_TestEntityPosition (hledict_t *ent);
void SVHL_TouchLinks ( hledict_t *ent, areanode_t *node );
trace_t SVHL_Move (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int type, int forcehull, hledict_t *passedict);
int SVHL_PointContents (vec3_t p);
int SVHL_AreaEdicts (vec3_t mins, vec3_t maxs, hledict_t **list, int maxcount);
