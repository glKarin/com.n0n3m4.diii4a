#include "q3common.h"
#if defined(VM_CG) && defined(HAVE_CLIENT)
#include "clq3defs.h"
#include "com_mesh.h"

//cl_ui.c
void CG_Command_f(void);

#define	CGAME_IMPORT_API_VERSION	4

static model_t		*box_model;
cvar_t *cl_shownet_ptr, *cl_c2sdupe_ptr, *cl_nodelta_ptr;

typedef enum {
	CG_PRINT,
	CG_ERROR,
	CG_MILLISECONDS,
	CG_CVAR_REGISTER,
	CG_CVAR_UPDATE,
	CG_CVAR_SET,
	CG_CVAR_VARIABLESTRINGBUFFER,
	CG_ARGC,
	CG_ARGV,
	CG_ARGS,
	CG_FS_FOPENFILE,				//10
	CG_FS_READ,
	CG_FS_WRITE,
	CG_FS_FCLOSEFILE,
	CG_SENDCONSOLECOMMAND,
	CG_ADDCOMMAND,
	CG_SENDCLIENTCOMMAND,
	CG_UPDATESCREEN,
	CG_CM_LOADMAP,
	CG_CM_NUMINLINEMODELS,
	CG_CM_INLINEMODEL,				//20
	CG_CM_LOADMODEL,
	CG_CM_TEMPBOXMODEL,
	CG_CM_POINTCONTENTS,
	CG_CM_TRANSFORMEDPOINTCONTENTS,
	CG_CM_BOXTRACE,
	CG_CM_TRANSFORMEDBOXTRACE,
	CG_CM_MARKFRAGMENTS,
	CG_S_STARTSOUND,
	CG_S_STARTLOCALSOUND,
	CG_S_CLEARLOOPINGSOUNDS,		//30
	CG_S_ADDLOOPINGSOUND,
	CG_S_UPDATEENTITYPOSITION,
	CG_S_RESPATIALIZE,
	CG_S_REGISTERSOUND,
	CG_S_STARTBACKGROUNDTRACK,
	CG_R_LOADWORLDMAP,
	CG_R_REGISTERMODEL,
	CG_R_REGISTERSKIN,
	CG_R_REGISTERSHADER,
	CG_R_CLEARSCENE,				//40
	CG_R_ADDREFENTITYTOSCENE,
	CG_R_ADDPOLYTOSCENE,
	CG_R_ADDLIGHTTOSCENE,
	CG_R_RENDERSCENE,
	CG_R_SETCOLOR,
	CG_R_DRAWSTRETCHPIC,
	CG_R_MODELBOUNDS,
	CG_R_LERPTAG,
	CG_GETGLCONFIG,
	CG_GETGAMESTATE,				//50
	CG_GETCURRENTSNAPSHOTNUMBER,
	CG_GETSNAPSHOT,
	CG_GETSERVERCOMMAND,
	CG_GETCURRENTCMDNUMBER,
	CG_GETUSERCMD,
	CG_SETUSERCMDVALUE,
	CG_R_REGISTERSHADERNOMIP,
	CG_MEMORY_REMAINING,
	CG_R_REGISTERFONT,
	CG_KEY_ISDOWN,					//60
	CG_KEY_GETCATCHER,
	CG_KEY_SETCATCHER,
	CG_KEY_GETKEY,
 	CG_PC_ADD_GLOBAL_DEFINE,
	CG_PC_LOAD_SOURCE,
	CG_PC_FREE_SOURCE,
	CG_PC_READ_TOKEN,
	CG_PC_SOURCE_FILE_AND_LINE,
	CG_S_STOPBACKGROUNDTRACK,
	CG_REAL_TIME,					//70
	CG_SNAPVECTOR,
	CG_REMOVECOMMAND,
	CG_R_LIGHTFORPOINT,				//73
	CG_CIN_PLAYCINEMATIC,			//74
	CG_CIN_STOPCINEMATIC,			//75
	CG_CIN_RUNCINEMATIC,			//76
	CG_CIN_DRAWCINEMATIC,			//77
	CG_CIN_SETEXTENTS,				//78
	CG_R_REMAP_SHADER,				//79
	CG_S_ADDREALLOOPINGSOUND,		//80
	CG_S_STOPLOOPINGSOUND,			//81

	CG_CM_TEMPCAPSULEMODEL,			//82
	CG_CM_CAPSULETRACE,				//83
	CG_CM_TRANSFORMEDCAPSULETRACE,	//84
	CG_R_ADDADDITIVELIGHTTOSCENE,	//85
	CG_GET_ENTITY_TOKEN,			//86
	CG_R_ADDPOLYSTOSCENE,			//87
	CG_R_INPVS,						//88
	// 1.32
	CG_FS_SEEK,						//89

	CG_MEMSET = 100,
	CG_MEMCPY,
	CG_STRNCPY,
	CG_SIN,
	CG_COS,
	CG_ATAN2,
	CG_SQRT,
	CG_FLOOR,
	CG_CEIL,
	CG_TESTPRINTINT,
	CG_TESTPRINTFLOAT,
	CG_ACOS,


	/*CG_FTE_FINDPARTICLEEFFECT = 200,
	CG_FTE_SPAWNPARTICLEEFFECT,
	CG_FTE_SPAWNPARTICLETRAIL,
	CG_FTE_FREEPARTICLESTATE*/
} cgameImport_t;

/*
int CG_FTE_FINDPARTICLEEFFECT(char *particleeffectname) = #200;
int CG_FTE_SPAWNPARTICLEEFFECT(vec3_t pos, vec3_t dir, float count, int effectnum, void *pstate) = #201;
int CG_FTE_SPAWNPARTICLETRAIL(vec3_t start, vec3_t end, int effectnum, void *pstate) = #202;
void CG_FTE_FREEPARTICLESTATE(void *pstate) = #203;
*/

/*
==================================================================

functions exported to the main executable

==================================================================
*/

typedef enum {
	CG_INIT,
//	void CG_Init( int serverMessageNum, int serverCommandSequence, int clientNum )
	// called when the level loads or when the renderer is restarted
	// all media should be registered at this time
	// cgame will display loading status by calling SCR_Update, which
	// will call CG_DrawInformation during the loading process
	// reliableCommandSequence will be 0 on fresh loads, but higher for
	// demos, tourney restarts, or vid_restarts

	CG_SHUTDOWN,
//	void (*CG_Shutdown)( void );
	// oportunity to flush and close any open files

	CG_CONSOLE_COMMAND,
//	qboolean (*CG_ConsoleCommand)( void );
	// a console command has been issued locally that is not recognized by the
	// main game system.
	// use Cmd_Argc() / Cmd_Argv() to read the command, return qfalse if the
	// command is not known to the game

	CG_DRAW_ACTIVE_FRAME,
//	void (*CG_DrawActiveFrame)( int serverTime, stereoFrame_t stereoView, qboolean demoPlayback );
	// Generates and draws a game scene and status information at the given time.
	// If demoPlayback is set, local movement prediction will not be enabled

	CG_CROSSHAIR_PLAYER,
//	int (*CG_CrosshairPlayer)( void );

	CG_LAST_ATTACKER,
//	int (*CG_LastAttacker)( void );

	CG_KEY_EVENT,
//	void	(*CG_KeyEvent)( int key, qboolean down );

	CG_MOUSE_EVENT,
//	void	(*CG_MouseEvent)( int dx, int dy );
	CG_EVENT_HANDLING
//	void (*CG_EventHandling)(int type);
} cgameExport_t;








unsigned int Contents_To_Q3(unsigned int fte)
{
	int ret = 0;

	if (fte & FTECONTENTS_SOLID)	//should use q3 constants.
		ret |= 1;
	if (fte & FTECONTENTS_WATER)	//should use q3 constants.
		ret |= 32;
	if (fte & FTECONTENTS_SLIME)	//should use q3 constants.
		ret |= 16;
	if (fte & FTECONTENTS_LAVA)	//should use q3 constants.
		ret |= 8;
	if (fte & FTECONTENTS_SKY)	//should use q3 constants.
		ret |= 0x80000000;

	return ret;
}
unsigned int Contents_From_Q3(unsigned int Q3)
{
	int ret = 0;

	if (Q3 & 1)	//should use q3 constants.
		ret |= FTECONTENTS_SOLID;
	if (Q3 & 32)	//should use q3 constants.
		ret |= FTECONTENTS_WATER;
	if (Q3 & 16)	//should use q3 constants.
		ret |= FTECONTENTS_SLIME;
	if (Q3 & 8)	//should use q3 constants.
		ret |= FTECONTENTS_LAVA;
	if (Q3 & 0x80000000)	//should use q3 constants.
		ret |= FTECONTENTS_SKY;

	return ret;
}

typedef struct {
	int			stringOffsets[MAX_Q3_CONFIGSTRINGS];
	char		stringData[MAX_Q3_GAMESTATE_CHARS];
	int			dataCount;
} gameState_t;
static gameState_t cggamestate;

void CG_ClearGameState(void)
{
	memset(&cggamestate, 0, sizeof(cggamestate));
}

void CG_InsertIntoGameState(int num, const char *str)
{
	if (num < 5)
	{
		Con_DPrintf("%i: %s", num, str);
	}

	if (num == CFGSTR_SYSINFO)
	{
		//check some things.
		ccs.servercount = atoi(worldfuncs->GetInfoKey(str, "sv_serverid"));
	}

	if (cggamestate.dataCount + strlen(str)+1 > countof(cggamestate.stringData))
	{
		char oldstringData[countof(cggamestate.stringData)];
		int i;
		char *oldstr;
		//copy the old strings to a temporary buffer
		memcpy(oldstringData, cggamestate.stringData, countof(cggamestate.stringData));
		cggamestate.dataCount = 0;
		for (i = 0; i < countof(cggamestate.stringOffsets); i++)
		{
			oldstr = oldstringData+cggamestate.stringOffsets[i];
			if (*oldstr)
			{
				if (cggamestate.dataCount + strlen(oldstr)+1 > countof(cggamestate.stringData))
					plugfuncs->EndGame("Too much configstring text\n");

				cggamestate.dataCount+=1;
				strcpy(cggamestate.stringData+cggamestate.dataCount, oldstr);
				cggamestate.stringOffsets[i] = cggamestate.dataCount;
				cggamestate.dataCount += strlen(oldstr);
			}
			else
				cggamestate.stringOffsets[i] = 0;
		}
	}

	if (!*str)
	{
		cggamestate.stringOffsets[num] = 0;
		return;
	}

	cggamestate.dataCount+=1;
	strcpy(cggamestate.stringData+cggamestate.dataCount, str);
	cggamestate.stringOffsets[num] = cggamestate.dataCount;
	cggamestate.dataCount += strlen(str);
}

const char *CG_GetConfigString(int num)
{
	if ((unsigned)num >= countof(cggamestate.stringOffsets))
		return "";
	return cggamestate.stringData + cggamestate.stringOffsets[num];
}

int CG_GetGameState(gameState_t *gs)
{
	memcpy(gs, &cggamestate, sizeof(gameState_t));
	return sizeof(gameState_t);
}

static int CGQ3_GetCurrentCmdNumber(void)
{	//Q3 sequences are 1-based, so 1<=idx<=latestsequence are valid
	//FTE's sequences are 0-based, so 0<=idx<latestsequence are valid
	return inputfuncs->GetMoveCount()-1;
}
static qboolean CGQ3_GetUserCmd(int cmdNumber, q3usercmd_t *ucmd)
{
	//q3 prediction uses for(int frame = CGQ3_GetCurrentCmdNumber() - CMD_BACKUP{64} + 1; frame <= CGQ3_GetCurrentCmdNumber(); frame++)
	//skipping any that are older than the snapshot's time.

	usercmd_t *cmd;

	//q3 does not do partials.
	if (cmdNumber < 0)
		return false;	//grr, stoopid q3.
	if (cmdNumber >= inputfuncs->GetMoveCount())
		plugfuncs->EndGame("CLQ3_GetUserCmd: %i >= %i", cmdNumber, inputfuncs->GetMoveCount());

	cmd = inputfuncs->GetMoveEntry(cmdNumber);
	if (!cmd)
		return false;
	ucmd->angles[0] = cmd->angles[0];
	ucmd->angles[1] = cmd->angles[1];
	ucmd->angles[2] = cmd->angles[2];
	ucmd->serverTime = cmd->servertime;
	ucmd->forwardmove = cmd->forwardmove;
	ucmd->rightmove = cmd->sidemove;
	ucmd->upmove = cmd->upmove;
	ucmd->buttons = cmd->buttons;
	ucmd->weapon = cmd->weapon;


	return true;
}

vm_t *cgvm;
static const char *mapentspointer;

extern int keycatcher;

qboolean CG_GetServerCommand(int cmdnum)
{
	static char bigconfigstring[65536];
	char *arg0;

	//quote from cgame code:
	// get the gamestate from the client system, which will have the
	// new configstring already integrated

	char *str = ccs.serverCommands[cmdnum & Q3TEXTCMD_MASK];

	Con_DPrintf("Dispaching %s\n", str);
	cmdfuncs->TokenizeString(str);
	arg0 = cmdfuncs->Argv(0, NULL, 0);

	if (!strcmp(arg0, "bcs0"))
	{	//start
		Q_snprintfz(bigconfigstring, sizeof(bigconfigstring), "cs %s \"%s", cmdfuncs->Argv(1, NULL, 0), cmdfuncs->Argv(2, NULL, 0));
		return false;
	}
	if (!strcmp(arg0, "bcs1"))
	{	//continuation
		Q_strncatz(bigconfigstring, cmdfuncs->Argv(2, NULL, 0), sizeof(bigconfigstring));
		return false;
	}
	if (!strcmp(arg0, "bcs2"))
	{	//end
		Q_strncatz(bigconfigstring, cmdfuncs->Argv(2, NULL, 0), sizeof(bigconfigstring));
		Q_strncatz(bigconfigstring, "\"", sizeof(bigconfigstring));
		cmdfuncs->TokenizeString(bigconfigstring);
	}

	if (!strcmp(arg0, "cs"))
		CG_InsertIntoGameState(atoi(cmdfuncs->Argv(1, NULL, 0)), cmdfuncs->Argv(2, NULL, 0));
	else if (!strcmp(arg0, "map_restart"))
		clientfuncs->ClearNotify();
	else if (!strcmp(arg0, "disconnect"))
		plugfuncs->EndGame("Server disconnected - %s", (cmdfuncs->Argc()>1)?cmdfuncs->Argv(1, NULL, 0):"No reason given");
	return true;
}


typedef struct {
	int		firstPoint;
	int		numPoints;
} q3markFragment_t;
typedef struct {
	float *points;
	size_t maxpoints;
	size_t numpoints;
	q3markFragment_t *frags;
	size_t maxfrags;
	size_t numfrags;
} q3markFragment_ctx_t;
static void CG_MarkFragments_Callback(void *vctx, vec3_t *fte_restrict points, size_t numtris, shader_t *shader)
{
	q3markFragment_ctx_t *ctx = vctx;
	size_t i;
	if (numtris > ctx->maxfrags-ctx->numfrags)
		numtris = ctx->maxfrags-ctx->numfrags;
	if (numtris > (ctx->maxpoints-ctx->numpoints)/3)
		numtris = (ctx->maxpoints-ctx->numpoints)/3;
	for (i = 0; i < numtris; i++)
	{
		ctx->frags[ctx->numfrags].numPoints = 3;
		ctx->frags[ctx->numfrags].firstPoint = ctx->numpoints;
		VectorCopy(points[0], ctx->points+3*(ctx->numpoints+0));
		VectorCopy(points[1], ctx->points+3*(ctx->numpoints+1));
		VectorCopy(points[2], ctx->points+3*(ctx->numpoints+2));
		points += 3;
		ctx->numfrags += 1;
		ctx->numpoints += 3;
	}
}

int CG_MarkFragments( int numPoints, const vec3_t *points, const vec3_t projection,
				   int maxPoints, float *pointBuffer, int maxFragments, q3markFragment_t *fragmentBuffer )
{
	vec3_t center;
	vec3_t axis[3];
	vec3_t p[4];
	int i;
	float radius;
	q3markFragment_ctx_t ctx;

	if (numPoints != 4)
		return 0;

	/*
	q3 gamecode includes something like this

	originalPoints[0][i] = origin[i] - radius * axis[1][i] - radius * axis[2][i];
	originalPoints[1][i] = origin[i] + radius * axis[1][i] - radius * axis[2][i];
	originalPoints[2][i] = origin[i] + radius * axis[1][i] + radius * axis[2][i];
	originalPoints[3][i] = origin[i] - radius * axis[1][i] + radius * axis[2][i];

	We want that origional axis and the origin
	axis[0] is given in the 'projection' parameter.

	Yes, reversing this stuff means that we'll have no support for triangles.
	*/

	VectorClear(center);
	VectorMA(center, 0.25, points[0], center);
	VectorMA(center, 0.25, points[1], center);
	VectorMA(center, 0.25, points[2], center);
	VectorMA(center, 0.25, points[3], center);

	VectorSubtract(points[0], center, p[0]);
	VectorSubtract(points[1], center, p[1]);
	VectorSubtract(points[2], center, p[2]);
	VectorSubtract(points[3], center, p[3]);

	for (i = 0; i < 3; i++)
	{
		axis[1][i] = (p[2][i]-p[1][i]);
		axis[2][i] = (p[3][i]-p[2][i]);
	}

	radius = VectorNormalize(axis[1]);
	VectorNormalize(axis[2]);
	VectorNormalize2(projection, axis[0]);

	ctx.points = pointBuffer;
	ctx.maxpoints = maxPoints;
	ctx.numpoints = 0;
	ctx.frags = fragmentBuffer;
	ctx.numfrags = 0;
	ctx.maxfrags = maxFragments;
	scenefuncs->ClipDecal(ccs.worldmodel, center, axis[0], axis[1], axis[2], radius, 0,0, CG_MarkFragments_Callback, &ctx);
	return ctx.numfrags;
}



//called by the sound code.
static struct
{
	unsigned int entnum;
	vec3_t origin;
//	vec3_t velocity;
	sfx_t *sfx;
	qboolean ispersistent;
} *loopers;
static size_t numloopers;
static size_t maxloopers;
unsigned int CG_GatherLoopingSounds(vec3_t *positions, unsigned int *entnums, sfx_t **sounds, unsigned int max)
{
	size_t i;
	if (max > numloopers)
		max = numloopers;
	for (i = 0; i < max; i++)
	{
		entnums[i] = loopers[i].entnum;
		VectorCopy(loopers[i].origin, positions[i]);
		sounds[i] = loopers[i].sfx;
	}
	return i;
}
static void CG_StopLoopingSounds(unsigned int entnum)
{
	size_t i;
	for (i = 0; i < numloopers; i++)
	{
		if (loopers[i].entnum == entnum)
			break;
	}
	if (i == numloopers)
		return;
	loopers[i] = loopers[numloopers-1];
	numloopers--;
}
static void CG_StartLoopingSounds(unsigned int entnum, float *origin, float *velocity, int range, const char *soundname, float volume, qboolean persistent)
{
	size_t i;
	for (i = 0; i < numloopers; i++)
	{
		if (loopers[i].entnum == entnum)
			break;
	}

	if (i == numloopers)
	{
		if (numloopers == maxloopers)
			Z_ReallocElements((void**)&loopers, &maxloopers, maxloopers+1, sizeof(*loopers));
		numloopers++;
	}
	loopers[i].entnum = entnum;
	VectorCopy(origin, loopers[i].origin);
	//VectorCopy(velocity, loopers[i].velocity);
	loopers[i].sfx = audiofuncs->PrecacheSound(soundname);
	loopers[i].ispersistent = persistent;
//	loopers[i].range = range;
//	loopers[i].volume = volume;
}

static void CG_ClearLoopingSounds(qboolean clearall)
{
	if (clearall)
		numloopers = 0;
	else
	{
		size_t i;
		for (i = 0; i < numloopers; )
		{
			if (!loopers[i].ispersistent)
			{
				loopers[i] = loopers[numloopers-1];
				numloopers--;
			}
			else
				i++;
		}
	}
}

/* same principle as the looper code above */
static struct
{
	unsigned int entnum;
	vec3_t origin;
} *entsounds;
static size_t numentsounds;
static size_t maxentsounds;

/* called by CG_S_UPDATEENTITYPOSITION to inform us of an ent its actual position */
static void CG_UpdateEntityPosition(unsigned int entnum, float *origin)
{
	size_t i;

	if (entnum == ENTITYNUM_NONE)
		return;

	for (i = 0; i < numentsounds; i++)
	{
		if (entsounds[i].entnum == entnum)
			break;
	}

	/* not present in list. */
	if (i == numentsounds)
	{
		if (numentsounds == maxentsounds)
			Z_ReallocElements((void**)&entsounds, & maxentsounds, maxentsounds+1, sizeof(*entsounds));
		numentsounds++;

		entsounds[i].entnum = entnum;
	}

	VectorCopy(origin, entsounds[i].origin);
}

/* returns the last updated position of an entity. */
static float* CG_GetEntityPosition(unsigned int entnum)
{
	int i;

	for (i = 0; i < numentsounds; i++)
	{
		if (entsounds[i].entnum == entnum) {
			return entsounds[i].origin;
		}
	}

	Con_Printf("CG_GetEntityPosition: entity num %i not known.\n", entnum);
	return NULL;
}


int VM_LerpTag(float *out, model_t *model, int f1, int f2, float l2, char *tagname);

#define VALIDATEPOINTER(o,l) if ((int)o + l >= mask || VM_POINTER(o) < offset) plugfuncs->EndGame("Call to cgame trap %u passes invalid pointer\n", (unsigned int)fn);	//out of bounds.

static qintptr_t CG_SystemCalls(void *offset, quintptr_t mask, qintptr_t fn, const qintptr_t *arg)
{
	int ret=0;

	//Remember to range check pointers.
	//The QVM must not be allowed to write to anything outside it's memory.
	//This includes getting the exe to copy it for it.

	//don't bother with reading, as this isn't a virus risk.
	//could be a cheat risk, but hey.

	//make sure that any called functions are also range checked.
	//like reading from files copies names into alternate buffers, allowing stack screwups.
//OutputDebugString(va("cl_cg: %i\n", fn));
	switch((cgameImport_t)fn)
	{
	case CG_PRINT:
		{
			const char *text = VM_POINTER(arg[0]);
				Con_Printf("%s", text);
		}
		break;
	case CG_ERROR:
		plugfuncs->EndGame("cgame: %s", (char*)VM_POINTER(arg[0]));
		break;

	case CG_ARGC:
		VM_LONG(ret) = cmdfuncs->Argc();
		break;
	case CG_ARGV:
		VALIDATEPOINTER(arg[1], arg[2]);
		cmdfuncs->Argv(VM_LONG(arg[0]), VM_POINTER(arg[1]), VM_LONG(arg[2]));
		break;
	case CG_ARGS:
		VALIDATEPOINTER(arg[0], arg[1]);
		cmdfuncs->Args(VM_POINTER(arg[0]), VM_LONG(arg[1]));
		break;
	case CG_CVAR_REGISTER:
		if (arg[0])
			VALIDATEPOINTER(arg[0], sizeof(q3vmcvar_t));
		return VMQ3_Cvar_Register(VM_POINTER(arg[0]), VM_POINTER(arg[1]), VM_POINTER(arg[2]), VM_LONG(arg[3]));
	case CG_CVAR_UPDATE:
		VALIDATEPOINTER(arg[0], sizeof(q3vmcvar_t));
		return VMQ3_Cvar_Update(VM_POINTER(arg[0]));

	case CG_CVAR_SET:
		cvarfuncs->SetString(VM_POINTER(arg[0]), VM_POINTER(arg[1])?VM_POINTER(arg[1]):"");
		break;
	case CG_CVAR_VARIABLESTRINGBUFFER:
		{
			cvar_t *var = cvarfuncs->GetNVFDG(VM_POINTER(arg[0]), NULL, 0, NULL, "Q3CG created");
			if (!VM_LONG(arg[2]))
				VM_LONG(ret) = 0;
			else if (!var)
			{
				VALIDATEPOINTER(arg[1], 1);
				*(char *)VM_POINTER(arg[1]) = '\0';
				VM_LONG(ret) = -1;
			}
			else
			{
				VALIDATEPOINTER(arg[1], arg[2]);
				Q_strncpyz(VM_POINTER(arg[1]), var->string, VM_LONG(arg[2]));
			}
		}
		break;

	case CG_SENDCONSOLECOMMAND:
		Con_DPrintf("CG_SENDCONSOLECOMMAND: %s", (char*)VM_POINTER(arg[0]));
		cmdfuncs->AddText(VM_POINTER(arg[0]), false);
		break;
	case CG_ADDCOMMAND:
		cmdfuncs->AddCommand(VM_POINTER(arg[0]), NULL, NULL);
		break;
	case CG_SENDCLIENTCOMMAND:
		Con_DPrintf("CG_SENDCLIENTCOMMAND: %s", (char*)VM_POINTER(arg[0]));
		CLQ3_SendClientCommand("%s", (char*)VM_POINTER(arg[0]));
		break;

	case CG_UPDATESCREEN:	//force a buffer swap cos loading won't refresh it soon.
		drawfuncs->RedrawScreen();
		break;

	case CG_FS_FOPENFILE: //fopen
		if (arg[1])
			VALIDATEPOINTER(arg[1], 4);
		VM_LONG(ret) = VM_fopen(VM_POINTER(arg[0]), VM_POINTER(arg[1]), VM_LONG(arg[2]), 1);
		break;

	case CG_FS_READ:	//fread
		VALIDATEPOINTER(arg[1], 4);
		VM_LONG(ret) = VM_FRead(VM_POINTER(arg[0]), VM_LONG(arg[1]), VM_LONG(arg[2]), 1);
		break;
	case CG_FS_WRITE:	//fwrite
		Con_DPrintf("CG_FS_WRITE: not implemented\n");
		break;
	case CG_FS_SEEK:
		return VM_FSeek(arg[0], arg[1], arg[2], 1);
	case CG_FS_FCLOSEFILE:	//fclose
		VM_fclose(VM_LONG(arg[0]), 1);
		break;

	case CG_CM_POINTCONTENTS: //int			trap_CM_PointContents( const vec3_t p, clipHandle_t model );
		{
			unsigned int pc;
			unsigned int modhandle = VM_LONG(arg[1]);
			model_t *mod;
			if (modhandle >= countof(ccs.model_precache))
			{
//				if (modhandle == countof(ccs.model_precache)+1)
//					mod = &capsule_model;
//				else
					mod = box_model;
			}
			else
				mod = ccs.model_precache[modhandle];
			if (mod && mod->loadstate == MLS_LOADED)
				pc = mod->funcs.NativeContents(mod, 0, 0, NULL, VM_POINTER(arg[0]), vec3_origin, vec3_origin);
			else
				pc = 1;//FTECONTENTS_SOLID;
			VM_LONG(ret) = pc;//Contents_To_Q3(pc);
		}
		break;

	case CG_CM_TRANSFORMEDPOINTCONTENTS: //int		trap_CM_TransformedPointContents( const vec3_t p, clipHandle_t model, const vec3_t origin, const vec3_t angles ) {
		{
			unsigned int pc;
			float *p = VM_POINTER(arg[0]);
			unsigned int modhandle = VM_LONG(arg[1]);
			float *origin = VM_POINTER(arg[2]);
			float *angles = VM_POINTER(arg[3]);
			model_t *mod;
			if (modhandle >= countof(ccs.model_precache))
			{
//				if (modhandle == countof(ccs.model_precache)+1)
//					mod = &capsule_model;
//				else
					mod = box_model;
			}
			else
				mod = ccs.model_precache[modhandle];

			if (mod && mod->loadstate == MLS_LOADED)
			{
				vec3_t		p_l;
				vec3_t		axis[3];

				// subtract origin offset
				VectorSubtract (p, origin, p_l);

				// rotate start and end into the models frame of reference
				if (angles[0] || angles[1] || angles[2])
				{
					AngleVectors (angles, axis[0], axis[1], axis[2]);
					VectorNegate(axis[1], axis[1]);
					pc = mod->funcs.NativeContents(mod, 0, 0, axis, p_l, vec3_origin, vec3_origin);
				}
				else
					pc = mod->funcs.NativeContents(mod, 0, 0, NULL, p_l, vec3_origin, vec3_origin);
			}
			else
				pc = Q3CONTENTS_SOLID;
			VM_LONG(ret) = pc;
		}
		break;

	case CG_CM_TRANSFORMEDCAPSULETRACE:
	case CG_CM_TRANSFORMEDBOXTRACE:
//		void		trap_CM_BoxTrace( trace_t *results, const vec3_t start, const vec3_t end,
//					  const vec3_t mins, const vec3_t maxs,
//					  clipHandle_t model, int brushmask );
		{
//FIXME: no protection of result trace.
			trace_t tr;
			q3trace_t *results	= VM_POINTER(arg[0]);
			float *start		= VM_POINTER(arg[1]);
			float *end			= VM_POINTER(arg[2]);
			float *mins			= VM_POINTER(arg[3]);
			float *maxs			= VM_POINTER(arg[4]);
			unsigned int modhandle		= VM_LONG(arg[5]);
			int brushmask		= VM_LONG(arg[6]);
			float *origin		= VM_POINTER(arg[7]);
			float *angles		= VM_POINTER(arg[8]);
			model_t *mod;
			if (modhandle >= countof(ccs.model_precache))
			{
//				if (modhandle == countof(ccs.model_precache)+1)
//					mod = &capsule_model;
//				else
					mod = box_model;
			}
			else
				mod = ccs.model_precache[modhandle];

			if (!mins)
				mins = vec3_origin;
			if (!maxs)
				maxs = vec3_origin;
			if (!origin)
				origin = vec3_origin;
			if (!angles)
				angles = vec3_origin;
			if (mod && mod->loadstate == MLS_LOADED)
				worldfuncs->TransformedTrace(mod, 0, 0, start, end, mins, maxs, fn==CG_CM_TRANSFORMEDCAPSULETRACE, &tr, origin, angles, brushmask);
			else
			{
				memset(&tr, 0, sizeof(tr));
				tr.allsolid = tr.startsolid = true;
				tr.contents = 1;
			}
			results->allsolid = tr.allsolid;
			results->contents = tr.contents;
			results->fraction = tr.fraction;
			results->entityNum = 0;
			results->startsolid = tr.startsolid;
			results->surfaceFlags = tr.surface?tr.surface->flags:0;
			memcpy(results->endpos, tr.endpos, sizeof(vec3_t));
			memcpy(&results->plane, &tr.plane, sizeof(cplane_t));
		}
		break;
	case CG_CM_CAPSULETRACE:
	case CG_CM_BOXTRACE:
//		void		trap_CM_BoxTrace( trace_t *results, const vec3_t start, const vec3_t end,
//					  const vec3_t mins, const vec3_t maxs,
//					  clipHandle_t model, int brushmask );
		{
//FIXME: no protection of result trace.
			trace_t tr;
			q3trace_t *results	= VM_POINTER(arg[0]);
			float *start		= VM_POINTER(arg[1]);
			float *end			= VM_POINTER(arg[2]);
			float *mins			= VM_POINTER(arg[3]);
			float *maxs			= VM_POINTER(arg[4]);
			unsigned int modhandle	= VM_LONG(arg[5]);
			unsigned int brushmask	= VM_LONG(arg[6]);
			model_t *mod;
			if (modhandle >= countof(ccs.model_precache))
			{
//				if (modhandle == countof(ccs.model_precache)+1)
//					mod = &capsule_model;
//				else
					mod = box_model;
			}
			else
				mod = ccs.model_precache[modhandle];

			if (mod->loadstate != MLS_LOADED)
			{
				if (mod->loadstate == MLS_NOTLOADED)
					scenefuncs->LoadModel(mod->publicname, MLV_SILENTSYNC);
				if (mod->loadstate == MLS_LOADING && threadfuncs)
					threadfuncs->WaitForCompletion(mod, &mod->loadstate, MLS_LOADING);
				if (mod->loadstate != MLS_LOADED)
				{
					memset(results, 0, sizeof(*results));
					results->fraction = 1;
					break;
				}
			}

			if (!mins)
				mins = vec3_origin;
			if (!maxs)
				maxs = vec3_origin;
			mod->funcs.NativeTrace(mod, 0, NULLFRAMESTATE, NULL, start, end, mins, maxs, fn==CG_CM_CAPSULETRACE, brushmask, &tr);
			results->allsolid = tr.allsolid;
			results->contents = tr.contents;
			results->fraction = tr.fraction;
			results->entityNum = 0;
			results->startsolid = tr.startsolid;
			results->surfaceFlags = tr.surface?tr.surface->flags:0;
			memcpy(results->endpos, tr.endpos, sizeof(vec3_t));
			memcpy(&results->plane, &tr.plane, sizeof(cplane_t));
		}
		break;

	case CG_R_LOADWORLDMAP:
		{	//rendering
			scenefuncs->NewMap(worldfuncs->LoadModel(VM_POINTER(arg[0]), MLV_SILENTSYNC));
		}
		break;

	case CG_CM_LOADMAP:
		{	//collisions
			int i;
			char *mapname = VM_POINTER(arg[0]);
			ccs.worldmodel = ccs.model_precache[0] = worldfuncs->LoadModel(mapname, MLV_SILENTSYNC);
			if (ccs.worldmodel->loadstate != MLS_LOADED)
				plugfuncs->EndGame("Couldn't load map %s", mapname);


			for (i=1 ; i<=ccs.worldmodel->numsubmodels && i < countof(ccs.model_precache); i++)
				ccs.model_precache[i] = worldfuncs->LoadModel(worldfuncs->FixName(va("*%i", i), mapname), MLV_SILENTSYNC);
		}

		break;

	case CG_CM_INLINEMODEL:
		if ((unsigned int)VM_LONG(arg[0]) > (ccs.worldmodel?ccs.worldmodel->numsubmodels:0))
			plugfuncs->EndGame("cgame asked for invalid model number\n");
		VM_LONG(ret) = VM_LONG(arg[0]);
		break;
	case CG_CM_NUMINLINEMODELS:
		VM_LONG(ret) = ccs.worldmodel?ccs.worldmodel->numsubmodels:0;
		break;

	case CG_CM_TEMPBOXMODEL:
		box_model = worldfuncs->TempBoxModel(VM_POINTER(arg[0]), VM_POINTER(arg[1]));
		VM_LONG(ret) = countof(ccs.model_precache);
		break;
	case CG_CM_TEMPCAPSULEMODEL:
		box_model = worldfuncs->TempBoxModel(VM_POINTER(arg[0]), VM_POINTER(arg[1]));
		VM_LONG(ret) = countof(ccs.model_precache)+1;
		break;

	case CG_R_MODELBOUNDS:
		VALIDATEPOINTER(arg[1], sizeof(vec3_t));
		VALIDATEPOINTER(arg[2], sizeof(vec3_t));
		{
			model_t *mod = scenefuncs->ModelFromId(arg[0]);
			if (mod)
			{
				if (mod->loadstate == MLS_LOADING && threadfuncs)
					threadfuncs->WaitForCompletion(mod, &mod->loadstate, MLS_LOADING);

				VectorCopy(mod->mins, ((float*)VM_POINTER(arg[1])));
				VectorCopy(mod->maxs, ((float*)VM_POINTER(arg[2])));
			}
		}
		break;

	case CG_R_REGISTERMODEL:	//precache model
		{
			char *name = VM_POINTER(arg[0]);
			model_t *mod;
			if (!name)
				return 0;
			mod = scenefuncs->LoadModel(worldfuncs->FixName(name, ccs.worldmodel->name), MLV_SILENTSYNC);
			if (mod->loadstate == MLS_FAILED || mod->type == mod_dummy)
				VM_LONG(ret) = 0;
			else
				VM_LONG(ret) = scenefuncs->ModelToId(mod);
		}
		break;

	case CG_R_REGISTERSKIN:
		VM_LONG(ret) = scenefuncs->RegisterSkinFile(VM_POINTER(arg[0]));
		break;

	case CG_R_REGISTERSHADER:
		if (!*(char*)VM_POINTER(arg[0]))
			VM_LONG(ret) = 0;
		else
			VM_LONG(ret) = drawfuncs->LoadImage(VM_POINTER(arg[0]));
		break;
	case CG_R_REGISTERSHADERNOMIP:
		if (!*(char*)VM_POINTER(arg[0]))
			VM_LONG(ret) = 0;
		else
			VM_LONG(ret) = drawfuncs->LoadImage(VM_POINTER(arg[0]));
		break;

	case CG_R_CLEARSCENE:	//clear scene (not rtlights, only dynamic ones)
		scenefuncs->ClearScene();
		break;
	case CG_R_ADDPOLYTOSCENE:
		VQ3_AddPolys(drawfuncs->ShaderFromId(arg[0]), VM_LONG(arg[1]), VM_POINTER(arg[2]), 1);
		break;
	case CG_R_ADDPOLYSTOSCENE:
		VQ3_AddPolys(drawfuncs->ShaderFromId(arg[0]), VM_LONG(arg[1]), VM_POINTER(arg[2]), VM_LONG(arg[3]));
		break;
	case CG_R_ADDREFENTITYTOSCENE:	//add ent to scene
		VQ3_AddEntity(VM_POINTER(arg[0]));
		break;
	case CG_R_ADDADDITIVELIGHTTOSCENE:
	case CG_R_ADDLIGHTTOSCENE:	//add light to scene.
		{
			dlight_t *dl = scenefuncs->AllocDlightOrg(-1, VM_POINTER(arg[0]));
			dl->flags = LFLAG_NORMALMODE|LFLAG_REALTIMEMODE;
			dl->radius = VM_FLOAT(arg[1]);
			dl->die = ccs.time+0.1;
			VectorSet(dl->color, VM_FLOAT(arg[2]), VM_FLOAT(arg[3]), VM_FLOAT(arg[4]));
		}
		break;
	case CG_R_RENDERSCENE:	//render scene
		VQ3_RenderView(VM_POINTER(arg[0]));
		break;

	case CG_R_SETCOLOR:	//setcolour float*
		{
			float *f = VM_POINTER(arg[0]);
			if (f)
				drawfuncs->Colour4f(f[0], f[1], f[2], f[3]);
			else
				drawfuncs->Colour4f(1, 1, 1, 1);
		}
		break;

	case CG_R_DRAWSTRETCHPIC:
		drawfuncs->Image(VM_FLOAT(arg[0]), VM_FLOAT(arg[1]), VM_FLOAT(arg[2]), VM_FLOAT(arg[3]), VM_FLOAT(arg[4]), VM_FLOAT(arg[5]), VM_FLOAT(arg[6]), VM_FLOAT(arg[7]), VM_LONG(arg[8]));
		break;

	case CG_R_LERPTAG:	//Lerp tag...
		VALIDATEPOINTER(arg[0], sizeof(float)*12);
		VM_LONG(ret) = VM_LerpTag(VM_POINTER(arg[0]), scenefuncs->ModelFromId(arg[1]), VM_LONG(arg[2]), VM_LONG(arg[3]), VM_FLOAT(arg[4]), VM_POINTER(arg[5]));
		break;

	case CG_S_REGISTERSOUND:
		{
			sfx_t *sfx;
			sfx = audiofuncs->PrecacheSound(VM_POINTER(arg[0]));
			if (sfx)
				VM_LONG(ret) = VM_TOSTRCACHE(arg[0]);
			else
				VM_LONG(ret) = -1;
		}
		break;

	case CG_S_STARTLOCALSOUND:
		if (VM_LONG(arg[0]) != -1 && arg[0])
			audiofuncs->LocalSound(VM_FROMSTRCACHE(arg[0]), VM_LONG(arg[1]), 1.0);
		break;

	case CG_S_STARTSOUND:// ( vec3_t origin, int entityNum, int entchannel, sfxHandle_t sfx )
		/* if NULL gets passed for an origin, it wants us to look at those
		   provided by CG_S_UPDATEENTITYPOSITION updates instead. */
		if (VM_POINTER(arg[0]) == NULL)
			audiofuncs->StartSound(VM_LONG(arg[1])+1, VM_LONG(arg[2]), audiofuncs->PrecacheSound(VM_FROMSTRCACHE(arg[3])), CG_GetEntityPosition(VM_LONG(arg[1])+1), NULL, 1, 1, 0, 0, CF_CLI_NODUPES | CF_FOLLOW);
		else
			audiofuncs->StartSound(VM_LONG(arg[1])+1, VM_LONG(arg[2]), audiofuncs->PrecacheSound(VM_FROMSTRCACHE(arg[3])), VM_POINTER(arg[0]), NULL, 1, 1, 0, 0, CF_CLI_NODUPES);
		break;

	case CG_S_ADDLOOPINGSOUND:
		//entnum, origin, velocity, sfx
		CG_StartLoopingSounds(VM_LONG(arg[0])+1, VM_POINTER(arg[1]), VM_POINTER(arg[2]), -1, VM_FROMSTRCACHE(arg[3]), 1, false);
		break;
	case CG_S_ADDREALLOOPINGSOUND:
		//entnum, origin, velocity, sfx
		CG_StartLoopingSounds(VM_LONG(arg[0])+1, VM_POINTER(arg[1]), VM_POINTER(arg[2]), -1, VM_FROMSTRCACHE(arg[3]), 1, true);
		break;
	case CG_S_STOPLOOPINGSOUND:
		//entnum
		CG_StopLoopingSounds(VM_LONG(arg[0])+1);
		break;
	case CG_S_CLEARLOOPINGSOUNDS:
		//clearall
		CG_ClearLoopingSounds(VM_LONG(arg[0]));
		break;
	case CG_S_UPDATEENTITYPOSITION://void		trap_S_UpdateEntityPosition( int entityNum, const vec3_t origin );
		//entnum, org
		CG_UpdateEntityPosition(VM_LONG(arg[0])+1, VM_POINTER(arg[1]));
		break;

	case CG_S_STARTBACKGROUNDTRACK:
		audiofuncs->ChangeMusicTrack(VM_POINTER(arg[0]), VM_POINTER(arg[1]));
		return 0;
	case CG_S_STOPBACKGROUNDTRACK:
		audiofuncs->ChangeMusicTrack(NULL, NULL);
		return 0;

	case CG_S_RESPATIALIZE://void		trap_S_Respatialize( int entityNum, const vec3_t origin, vec3_t axis[3], int inwater );
		audiofuncs->Spacialize(0, VM_LONG(arg[0])+1, VM_POINTER(arg[1]), VM_POINTER(arg[2]), VM_LONG(arg[3])?1:0, vec3_origin);
		break;

	case CG_KEY_ISDOWN:
		{
			if (inputfuncs->IsKeyDown(VM_LONG(arg[0])))
				VM_LONG(ret) = 1;
			else
				VM_LONG(ret) = 0;
		}
		break;

	case CG_KEY_GETKEY:
		{
			int ret[1];
			inputfuncs->FindKeysForCommand(0, VM_POINTER(arg[0]), ret, NULL, countof(ret));
			return ret[0];
		}
		break;

	case CG_KEY_GETCATCHER:
		VM_LONG(ret) = Q3_GetKeyCatcher();
		break;
	case CG_KEY_SETCATCHER:
		Q3_SetKeyCatcher(VM_LONG(arg[0]));
		break;

	case CG_GETGLCONFIG:
		{
			float vsize[2];
			q3glconfig_t *cfg;
			VALIDATEPOINTER(arg[0], sizeof(q3glconfig_t));
			drawfuncs->GetVideoSize(vsize, NULL);
			cfg = VM_POINTER(arg[0]);

			//do any needed work
			memset(cfg, 0, sizeof(*cfg));

			Q_strncpyz(cfg->renderer_string, "", sizeof(cfg->renderer_string));
			Q_strncpyz(cfg->vendor_string, "", sizeof(cfg->vendor_string));
			Q_strncpyz(cfg->version_string, "", sizeof(cfg->version_string));
			Q_strncpyz(cfg->extensions_string, "", sizeof(cfg->extensions_string));

			cfg->colorBits = 32;
			cfg->depthBits = 24;
			cfg->stencilBits = 8;//sh_config.stencilbits;
			cfg->textureCompression = true;//!!sh_config.hw_bc;
			cfg->textureEnvAddAvailable = true;//sh_config.env_add;

			//these are the only three that really matter.
			cfg->vidWidth = vsize[0];
			cfg->vidHeight = vsize[1];
			cfg->windowAspect = (float)vsize[0]/vsize[1];
		}
		break;

	case CG_GETGAMESTATE:
		VALIDATEPOINTER(arg[0], sizeof(gameState_t));
		VM_LONG(ret) = CG_GetGameState(VM_POINTER(arg[0]));
		break;

	case CG_CM_MARKFRAGMENTS:
		VM_LONG(ret) = CG_MarkFragments( VM_LONG(arg[0]), VM_POINTER(arg[1]), VM_POINTER(arg[2]), VM_LONG(arg[3]), VM_POINTER(arg[4]), VM_LONG(arg[5]), VM_POINTER(arg[6]) );
		break;

	case CG_GETCURRENTSNAPSHOTNUMBER:
		VALIDATEPOINTER(arg[0], sizeof(int));
		VALIDATEPOINTER(arg[1], sizeof(int));
		*(int *)VM_POINTER(arg[0]) = ccs.snap.serverMessageNum;
		*(int *)VM_POINTER(arg[1]) = ccs.snap.serverTime;
		break;

	case CG_GETSNAPSHOT:
		VALIDATEPOINTER(arg[1], sizeof(snapshot_t));
		VM_LONG(ret) = CG_FillQ3Snapshot(VM_LONG(arg[0]), VM_POINTER(arg[1]));
		break;

	case CG_GETCURRENTCMDNUMBER:
		VM_LONG(ret) = CGQ3_GetCurrentCmdNumber();
		break;
	case CG_GETUSERCMD:
		VALIDATEPOINTER(arg[1], sizeof(q3usercmd_t));
		VM_LONG(ret) = CGQ3_GetUserCmd(VM_LONG(arg[0]), VM_POINTER(arg[1]));
		break;
	case CG_SETUSERCMDVALUE:	//weaponselect, zoomsensitivity.
		ccs.selected_weapon = VM_LONG(arg[0]);
		inputfuncs->SetSensitivityScale(VM_FLOAT(arg[1]));
		break;

	case CG_GETSERVERCOMMAND:
		VM_LONG(ret) = CG_GetServerCommand(VM_LONG(arg[0]));
		break;

	case CG_MEMORY_REMAINING:
		VM_LONG(ret) = 1024*1024*8;//Hunk_LowMemAvailable();
		break;

	case CG_MILLISECONDS:
		VM_LONG(ret) = plugfuncs->GetMilliseconds();
		break;
	case CG_REAL_TIME:
		VALIDATEPOINTER(arg[0], sizeof(q3time_t));
		return Q3VM_GetRealtime(VM_POINTER(arg[0]));

	case CG_SNAPVECTOR: // ( float *v )
		VALIDATEPOINTER(arg[0], sizeof(vec3_t));
		{
			float *fp = (float *)VM_POINTER(arg[0]);
#define rint(x) (int)((x > 0)?(x + 0.5f):(x-0.5f))
			fp[0] = rint(fp[0]);
			fp[1] = rint(fp[1]);
			fp[2] = rint(fp[2]);
		}
		break;

	case CG_PC_ADD_GLOBAL_DEFINE:
		Con_Printf("CG_PC_ADD_GLOBAL_DEFINE not supported\n");
		break;
	case CG_PC_SOURCE_FILE_AND_LINE:
		Script_Get_File_And_Line(arg[0], VM_POINTER(arg[1]), VM_POINTER(arg[2]));
		break;

	case CG_PC_LOAD_SOURCE:
		return Script_LoadFile(VM_POINTER(arg[0]));
	case CG_PC_FREE_SOURCE:
		Script_Free(arg[0]);
		break;
	case CG_PC_READ_TOKEN:
		//fixme: memory protect.
		VALIDATEPOINTER(arg[1], sizeof(struct pc_token_s));
		return Script_Read(arg[0], VM_POINTER(arg[1]));

// standard Q3
	case CG_MEMSET:
		VALIDATEPOINTER(arg[0], arg[2]);
		{
			void *dst = VM_POINTER(arg[0]);
			memset(dst, arg[1], arg[2]);
		}
		break;
	case CG_MEMCPY:
		VALIDATEPOINTER(arg[0], arg[2]);
		{
			void *dst = VM_POINTER(arg[0]);
			void *src = VM_POINTER(arg[1]);
			memcpy(dst, src, arg[2]);
		}
		break;
	case CG_STRNCPY:
		VALIDATEPOINTER(arg[0], arg[2]);
		{
			void *dst = VM_POINTER(arg[0]);
			void *src = VM_POINTER(arg[1]);
			strncpy(dst, src, arg[2]);
		}
		break;
	case CG_SIN:
		VM_FLOAT(ret)=(float)sin(VM_FLOAT(arg[0]));
		break;
	case CG_COS:
		VM_FLOAT(ret)=(float)cos(VM_FLOAT(arg[0]));
		break;
	case CG_ACOS:
		VM_FLOAT(ret)=(float)acos(VM_FLOAT(arg[0]));
		break;
	case CG_ATAN2:
		VM_FLOAT(ret)=(float)atan2(VM_FLOAT(arg[0]), VM_FLOAT(arg[1]));
		break;
	case CG_SQRT:
		VM_FLOAT(ret)=(float)sqrt(VM_FLOAT(arg[0]));
		break;
	case CG_FLOOR:
		VM_FLOAT(ret)=(float)floor(VM_FLOAT(arg[0]));
		break;
	case CG_CEIL:
		VM_FLOAT(ret)=(float)ceil(VM_FLOAT(arg[0]));
		break;

	case CG_R_REMAP_SHADER:
		scenefuncs->RemapShader(VM_POINTER(arg[0]), VM_POINTER(arg[1]), VM_FLOAT(arg[2]));
		break;

	case CG_R_REGISTERFONT:
		VALIDATEPOINTER(arg[2], sizeof(fontInfo_t));
		UI_RegisterFont(VM_POINTER(arg[0]), VM_LONG(arg[1]), VM_POINTER(arg[2]));
		break;

	case CG_GET_ENTITY_TOKEN:
		mapentspointer = cmdfuncs->ParseToken(mapentspointer, VM_POINTER(arg[0]), arg[1], NULL);
		return !!mapentspointer;


	case CG_CIN_PLAYCINEMATIC:
		return UI_Cin_Play(VM_POINTER(arg[0]), VM_LONG(arg[1]), VM_LONG(arg[2]), VM_LONG(arg[3]), VM_LONG(arg[4]), VM_LONG(arg[5]));
	case CG_CIN_STOPCINEMATIC:
		return UI_Cin_Stop(VM_LONG(arg[0]));
	case CG_CIN_RUNCINEMATIC:
		return UI_Cin_Run(VM_LONG(arg[0]));
	case CG_CIN_DRAWCINEMATIC:
		return UI_Cin_Draw(VM_LONG(arg[0]));
	case CG_CIN_SETEXTENTS:
		return UI_Cin_SetExtents(VM_LONG(arg[0]), VM_LONG(arg[1]), VM_LONG(arg[2]), VM_LONG(arg[3]), VM_LONG(arg[4]));

/*	case CG_FTE_FINDPARTICLEEFFECT:
		return pe->FindParticleType(VM_POINTER(arg[0]));
	case CG_FTE_SPAWNPARTICLEEFFECT:
		return pe->RunParticleEffectState(VM_POINTER(arg[0]), VM_POINTER(arg[1]), VM_FLOAT(arg[2]), VM_LONG(arg[3]), VM_POINTER(arg[4]));
	case CG_FTE_SPAWNPARTICLETRAIL:
		return pe->ParticleTrail(VM_POINTER(arg[0]), VM_POINTER(arg[1]), VM_LONG(arg[2]), 0, 1, NULL, VM_POINTER(arg[3]));
	case CG_FTE_FREEPARTICLESTATE:
		pe->DelinkTrailstate(VM_POINTER(arg[0]));
		break;
*/
	case CG_CM_LOADMODEL:
	case CG_REMOVECOMMAND:
	case CG_TESTPRINTINT:
	case CG_TESTPRINTFLOAT:
	case CG_R_INPVS:
	case CG_R_LIGHTFORPOINT:
//	default:
		Con_Printf("Q3CG: Bad system trap: %i\n", (int)fn);
	}

	return ret;
}

static int CG_SystemCallsVM(void *offset, quintptr_t mask, int fn, const int *arg)
{
	if (sizeof(qintptr_t) == sizeof(int))
		return CG_SystemCalls(offset, mask, fn, (qintptr_t*)arg);
	else
	{
		qintptr_t args[10];

		args[0]=arg[0];
		args[1]=arg[1];
		args[2]=arg[2];
		args[3]=arg[3];
		args[4]=arg[4];
		args[5]=arg[5];
		args[6]=arg[6];
		args[7]=arg[7];
		args[8]=arg[8];
		args[9]=arg[9];

		return CG_SystemCalls(offset, mask, fn, args);
	}
}

//I'm not keen on this.
//but dlls call it without saying what sort of vm it comes from, so I've got to have them as specifics
static qintptr_t EXPORT_FN CG_SystemCallsNative(qintptr_t arg, ...)
{
	qintptr_t args[10];
	va_list argptr;

	va_start(argptr, arg);
	args[0]=va_arg(argptr, qintptr_t);
	args[1]=va_arg(argptr, qintptr_t);
	args[2]=va_arg(argptr, qintptr_t);
	args[3]=va_arg(argptr, qintptr_t);
	args[4]=va_arg(argptr, qintptr_t);
	args[5]=va_arg(argptr, qintptr_t);
	args[6]=va_arg(argptr, qintptr_t);
	args[7]=va_arg(argptr, qintptr_t);
	args[8]=va_arg(argptr, qintptr_t);
	args[9]=va_arg(argptr, qintptr_t);
	va_end(argptr);

	return CG_SystemCalls(NULL, ~(quintptr_t)0, arg, args);
}

int CG_Refresh(double time)
{
	if (!cgvm)
		return false;

	ccs.time = time;
	vmfuncs->Call(cgvm, CG_DRAW_ACTIVE_FRAME, (int)(ccs.time*1000), 0, false);
	drawfuncs->Colour4f(1, 1, 1, 1);
	return true;
}



void CG_Stop (void)
{
	Q3_SetKeyCatcher(Q3_GetKeyCatcher()&~2);
	if (cgvm)
	{
		vmfuncs->Call(cgvm, CG_SHUTDOWN);
		vmfuncs->Destroy(cgvm);
		VM_fcloseall(1);
		cgvm = NULL;

	}
}

qboolean CG_VideoRestarted(void)
{
	if (cgvm)
	{
		vmfuncs->Call(cgvm, CG_INIT, ccs.serverMessageNum, ccs.lastServerCommandNum, ccs.playernum);
		return true;
	}
	return false;
}

void CG_Start (void)
{
	CG_Stop();

	box_model = worldfuncs->TempBoxModel(vec3_origin, vec3_origin);	//just in case.

	clientfuncs->SetLoadingState(true);
	cgvm = vmfuncs->Create("cgame", cvarfuncs->GetFloat("com_gamedirnativecode")?CG_SystemCallsNative:NULL, "vm/cgame", CG_SystemCallsVM);
	clientfuncs->SetLoadingState(false);
	if (cgvm)
	{	//hu... cgame doesn't appear to have a query version call!
		vmfuncs->Call(cgvm, CG_INIT, ccs.serverMessageNum, ccs.lastServerCommandNum, ccs.playernum);
	}
	else
		plugfuncs->EndGame("Failed to initialise cgame module\n");
}

qboolean CG_ConsoleCommand(void)
{
	if (!cgvm)
		return false;
//	Con_DPrintf("CG_Command: %s %s\n", cmdfuncs->Argv(0), cmdfuncs->Args());
	return vmfuncs->Call(cgvm, CG_CONSOLE_COMMAND);
}

/*void CG_Command_f(void)
{
	if (cgvm)
	{
		Con_DPrintf("CG_Command_f: %s %s\n", Cmd_Argv(0), Cmd_Args());
		if (!VM_Call(cgvm, CG_CONSOLE_COMMAND))
		{
			Cmd_ForwardToServer();
		}
	}
}*/

qboolean CG_KeyPressed(int key, int unicode, int down)
{

	if (!cgvm || !(Q3_GetKeyCatcher()&8))
		return false;

	/* if you change this here, it'll have to be changed in cl_cg.c too */
	switch (key)
	{
		/* all these get interpreted as enter in Q3's UI... */
		case K_JOY1:
		case K_JOY2:
		case K_JOY3:
		case K_JOY4:
		case K_AUX1:
		case K_AUX2:
		case K_AUX3:
		case K_AUX4:
		case K_AUX5:
		case K_AUX6:
		case K_AUX7:
		case K_AUX8:
		case K_AUX9:
		case K_AUX10:
		case K_AUX11:
		case K_AUX14:
		case K_AUX15:
		case K_AUX16:
			return true;
			break;
		/* Q3 doesn't know about these keys, remap them */
		case K_GP_START:
			key = K_ESCAPE;
			break;
		case K_GP_DPAD_UP:
			key = K_UPARROW;
			break;
		case K_GP_DPAD_DOWN:
			key = K_DOWNARROW;
			break;
		case K_GP_DPAD_LEFT:
			key = K_LEFTARROW;
			break;
		case K_GP_DPAD_RIGHT:
			key = K_RIGHTARROW;
			break;
		case K_GP_A:
			key = K_ENTER;
			break;
		case K_GP_B:
			key = K_ESCAPE;
			break;
		case K_GP_X:
			key = K_BACKSPACE;
			break;
	}

	return vmfuncs->Call(cgvm, CG_KEY_EVENT, key, down);
}

void CG_Restart(void)
{
	CG_Stop();
	CG_Start();
}

#endif
