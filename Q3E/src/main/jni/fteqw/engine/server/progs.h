/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

struct client_s;
struct edict_s;

#define MAX_PROGS 64
#define MAXADDONS 16

void SVQ1_CvarChanged(cvar_t *var);
#define NewGetEdictFieldValue GetEdictFieldValue
void Q_SetProgsParms(qboolean forcompiler);
void PR_Deinit(void);	//server shutting down
void PR_LoadGlabalStruct(qboolean muted);
enum initprogs_e
{
	INITPROGS_NORMAL	= 0,
	INITPROGS_EDITOR	= 1<<0,
	INITPROGS_REQUIRE	= 1<<1,
};
void Q_InitProgs(enum initprogs_e flags);
void PR_SpawnInitialEntities(const char *file);
void PR_RegisterFields(void);
void PR_Init(void);
void QDECL ED_Spawned (struct edict_s *ent, int loading);
void SSQC_MapEntityEdited(int modelidx, int idx, const char *newdata);
void SV_SetEntityButtons(struct edict_s *ent, unsigned int buttonbits);
qboolean SV_RunFullQCMovement(struct client_s *client, usercmd_t *ucmd);
qboolean PR_KrimzonParseCommand(const char *s);
qboolean PR_ParseClusterEvent(const char *dest, const char *source, const char *cmd, const char *info);
qboolean PR_UserCmd(const char *cmd);
qboolean PR_ConsoleCmd(const char *cmd);


#define PR_MAINPROGS 0	//this is a constant that should really be phased out. But seeing as QCLIB requires some sort of master progs due to extern funcs...
	//maybe go through looking for extern funcs, and remember which were not allocated. It would then be a first come gets priority. Not too bad I supppose.

extern int compileactive;

typedef enum {PROG_NONE, PROG_QW, PROG_NQ, PROG_H2, PROG_PREREL, PROG_TENEBRAE, PROG_UNKNOWN} progstype_t;	//unknown obtains NQ behaviour
extern progstype_t progstype;

#include "../qclib/progslib.h"

typedef struct edict_s
{
	//these 5 shared with qclib
	enum ereftype_e	ereftype;
	float			freetime; // sv.time when the object was freed
	int				entnum;
	unsigned int	fieldsize;
	pbool			readonly;	//world
#ifdef VM_Q1
	stdentvars_t	*v;
	extentvars_t	*xv;
#else
	union {
		stdentvars_t	*v;
		stdentvars_t	*xv;
	};
#endif
	/*qc lib doesn't care about the rest*/

	/*these are shared with csqc*/
#ifdef USEAREAGRID
	areagridlink_t	gridareas[AREAGRIDPERENT];	//on overflow, use the inefficient overflow list.
	size_t			gridareasequence;	//used to avoid iterrating the same ent twice.
#else
	link_t	area;
#endif
	pvscache_t pvsinfo;
	int lastruntime;
	int solidsize;
#ifdef USERBE
	entityrbe_t rbe;
#endif
	/*csqc doesn't reference the rest*/

#ifdef NQPROT
	float muzzletime;	//nq clients need special handling to retain EF_MUZZLEFLASH while not breaking qw clients running nq mods.
#endif
	entity_state_t	baseline;
// other fields from progs come immediately after
} edict_t;





#undef pr_global_struct
#define pr_global_struct *pr_global_ptrs

extern globalptrs_t *pr_global_ptrs;

extern pubprogfuncs_t *svprogfuncs;	//instance
extern progparms_t svprogparms;
extern progsnum_t svmainprogs;
extern progsnum_t clmainprogs;
#define	HLEDICT_FROM_AREA(l) STRUCT_FROM_LINK(l,hledict_t,area)
#define	Q2EDICT_FROM_AREA(l) STRUCT_FROM_LINK(l,q2edict_t,area)
#define	Q3EDICT_FROM_AREA(l) STRUCT_FROM_LINK(l,q3serverEntity_t,area)

extern func_t SpectatorConnect;
extern func_t SpectatorThink;
extern func_t SpectatorDisconnect;

extern func_t SV_PlayerPhysicsQC;
extern func_t EndFrameQC;

extern qboolean ssqc_deprecated_warned;
extern cvar_t pr_maxedicts; //used in too many places...
extern cvar_t noexit, temp1, saved1, saved2, saved3, saved4, savedgamecfg, scratch1, scratch2, scratch3, scratch4, gamecfg, nomonsters; //read by savegame.c
extern cvar_t pr_ssqc_memsize, pr_imitatemvdsv, sv_aim, sv_maxaim, pr_ssqc_coreonerror, dpcompat_nopreparse;
extern int pr_teamfield;

qboolean PR_QCChat(char *text, int say_type);

void PR_ClientUserInfoChanged(char *name, char *oldivalue, char *newvalue);
void PR_PreShutdown(void);
void PR_LocalInfoChanged(char *name, char *oldivalue, char *newvalue);
void PF_InitTempStrings(pubprogfuncs_t *inst);

#ifdef VM_LUA
qboolean PR_LoadLua(void);
#endif
#ifdef VM_Q1
#define VMFSID_Q1QVM 57235	//the q1qvm zone tag that is freed when the module is purged.
struct client_s;
void Q1QVM_Shutdown(qboolean notifygame);
qboolean PR_LoadQ1QVM(void);
void Q1QVM_ClientConnect(struct client_s *cl);
qboolean Q1QVM_GameConsoleCommand(void);
qboolean Q1QVM_ClientSay(edict_t *player, qboolean team);
qboolean Q1QVM_UserInfoChanged(edict_t *player, qboolean after);
void Q1QVM_PlayerPreThink(void);
void Q1QVM_RunPlayerThink(void);
void Q1QVM_PostThink(void);
void Q1QVM_StartFrame(qboolean botsarespecialsnowflakes);
void Q1QVM_Blocked(void);
qboolean Q1QVM_SendEntity(quint64_t sendflags);
void Q1QVM_SetNewParms(void);
void Q1QVM_SetChangeParms(void);
qboolean Q1QVM_ClientCommand(void);
void Q1QVM_GameCodePausedTic(float pausedduration);
void Q1QVM_DropClient(struct client_s *cl);
void Q1QVM_ChainMoved(void);
void Q1QVM_EndFrame(void);
#endif

