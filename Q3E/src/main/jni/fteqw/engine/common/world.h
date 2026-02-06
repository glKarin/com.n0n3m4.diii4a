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
// world.h

#include "../client/quakedef.h"

typedef struct plane_s
{
	vec3_t	normal;
	float	dist;
} plane_t;

typedef struct csurface_s
{
	char		name[16];
	int			flags;
	int			value;
} q2csurface_t;

typedef struct cplane_s
{
	vec3_t	normal;
	float	dist;
	qbyte	type;			// for fast side tests
	qbyte	signbits;		// signx + (signy<<1) + (signz<<1)
	qbyte	pad[2];
} cplane_t;

/*
typedef struct trace_s
{
	qboolean	allsolid;	// if true, plane is not valid
	qboolean	startsolid;	// if true, the initial point was in a solid area
	qboolean	inopen, inwater;
	float	fraction;		// time completed, 1.0 = didn't hit anything
	vec3_t	endpos;			// final position
	plane_t	plane;			// surface normal at impact
	edict_t	*ent;			// entity the surface is on
} trace_t;
*/

//these two structures must match (except for extra qw members)
typedef struct trace_s
{
//DON'T ADD ANYTHING BETWEEN THIS LINE
//q2 game dll code will memcpy the lot from trace_t to q2trace_t.
	qboolean			allsolid;	// if true, plane is not valid
	qboolean			startsolid;	// if true, the initial point was in a solid area
	float				fraction;	// time completed, 1.0 = didn't hit anything (nudged closer to the start point to cover precision issues)
	vec3_t				endpos;		// final position
	cplane_t			plane;		// surface normal at impact
	const q2csurface_t	*surface;	// q2-compat surface hit
	unsigned int		contents;	// contents on other side of surface hit
	void				*ent;		// not set by CM_*() functions
//AND THIS LINE
	int					entnum;

	qboolean			inopen, inwater;
	float				truefraction;	//can be negative, also has floating point precision issues, etc.
	int					brush_id;
	int					brush_face;
	int					surface_id;
	int					triangle_id;
	int					bone_id;
} trace_t;

typedef struct q2trace_s
{
	qboolean	allsolid;	// if true, plane is not valid
	qboolean	startsolid;	// if true, the initial point was in a solid area
	float		fraction;	// time completed, 1.0 = didn't hit anything
	vec3_t		endpos;		// final position
	cplane_t	plane;		// surface normal at impact
	const q2csurface_t	*surface;	// surface hit
	int			contents;	// contents on other side of surface hit
	void	*ent;		// not set by CM_*() functions
} q2trace_t;


// edict->flags
#define	FL_FLY					(1<<0)
#define	FL_SWIM					(1<<1)
//#define	FL_GLIMPSE				(1<<2)
#define	FL_CLIENT				(1<<3)
#define	FL_INWATER				(1<<4)
#define	FL_MONSTER				(1<<5)
#define	FL_GODMODE				(1<<6)
#define	FL_NOTARGET				(1<<7)
#define	FL_ITEM					(1<<8)
#define	FL_ONGROUND				(1<<9)
#define	FL_PARTIALGROUND		(1<<10)	// not all corners are valid
#define	FL_WATERJUMP			(1<<11)	// player jumping out of water
#define	FL_JUMPRELEASED			(1<<12)
//#define FLRE_ISBOT			(1<<13)
#define FL_FINDABLE_NONSOLID	(1<<14)	//a cpqwsv feature
#define FL_MOVECHAIN_ANGLE		(1<<15) // hexen2 - when in a move chain, will update the angle
#define FLQW_LAGGEDMOVE			(1<<16)
#define FLH2_HUNTFACE			(1<<16)
#define FLH2_NOZ				(1<<17)
								//18
								//19
#define	FL_HUBSAVERESET			(1<<20) //hexen2, ent is reverted to original state on map changes.
#define FL_CLASS_DEPENDENT		(1<<21)	//hexen2
								//22
								//23



#define	MOVE_NORMAL		0
#define	MOVE_NOMONSTERS	(1<<0)
#define	MOVE_MISSILE	(1<<1)
#ifdef HAVE_LEGACY
#define MOVE_WORLDONLY	(MOVE_NOMONSTERS|MOVE_MISSILE) //use MOVE_OTHERONLY instead
#endif
#define	MOVE_HITMODEL	(1<<2)
#define MOVE_RESERVED	(1<<3)			//so we are less likly to get into tricky situations when we want to steal another future DP extension.
#define MOVE_TRIGGERS	(1<<4)			//triggers must be marked with FINDABLE_NONSOLID	(an alternative to solid-corpse)
#define MOVE_EVERYTHING	(1<<5)			//can return triggers and non-solid items if they're marked with FINDABLE_NONSOLID (works even if the items are not properly linked)
#define MOVE_LAGGED		(1<<6)			//trace touches current last-known-state, instead of actual ents (just affects players for now)
#define MOVE_ENTCHAIN	(1<<7)			//chain of impacted ents, otherwise result shows only world
#define MOVE_OTHERONLY	(1<<8)			//test the trace against a single entity, ignoring non-solid/owner/etc flags (but respecting contents).
#define MOVE_IGNOREHULL	(1u<<31)	//used on tracelines etc to simplify the code a little

#ifdef USEAREAGRID
//this macro does it in two steps to avoid float precision issues.
//it also ensures that it will return at least one grid section.
#define CALCAREAGRIDBOUNDS(w,min,max) \
		ming[0] = floor(((min)[0]+(w)->gridbias[0]) / (w)->gridscale[0]);	\
		ming[1] = floor(((min)[1]+(w)->gridbias[1]) / (w)->gridscale[1]);	\
		maxg[0] = floor(((max)[0]+(w)->gridbias[0]) / (w)->gridscale[0]);	\
		maxg[1] = floor(((max)[1]+(w)->gridbias[1]) / (w)->gridscale[1]);	\
		ming[0] = bound(0, ming[0], (w)->gridsize[0]-1);	\
		ming[1] = bound(0, ming[1], (w)->gridsize[1]-1);	\
		maxg[0] = bound(ming[0], maxg[0], (w)->gridsize[0]-1)+1;	\
		maxg[1] = bound(ming[1], maxg[1], (w)->gridsize[1]-1)+1;
#else
#define	EDICT_FROM_AREA(l) STRUCT_FROM_LINK(l,wedict_t,area)
#endif

#if defined(Q2SERVER) || !defined(USEAREAGRID)
//q2 game code embeds a link_t struct inside the public edicts.
//this is why we can't have nice things.

//a binary tree. ents straddling a node are inserted into the parent.
//this is a problem when your root note passes through '0 0 0' and you have shitty mods with 2000 such ents.
typedef struct areanode_s
{
	int		axis;		// -1 = leaf node
	float	dist;
	struct areanode_s	*children[2];
	link_t	edicts;
} areanode_t;
#endif

typedef struct wedict_s wedict_t;
#define PROG_TO_WEDICT (wedict_t*)PROG_TO_EDICT
#define WEDICT_NUM_UB (wedict_t *)EDICT_NUM_UB	//ent number isn't bounded
#define WEDICT_NUM_PB (wedict_t *)EDICT_NUM_PB	//pre-bound
#define G_WEDICT (wedict_t *)G_EDICT

typedef struct
{
	qboolean present;
	vec3_t origin;
	vec3_t angles;
} laggedentinfo_t;

#ifdef USERBE
typedef struct
{
	void (QDECL *End)(struct world_s *world);
	void (QDECL *RemoveJointFromEntity)(struct world_s *world, wedict_t *ed);
	void (QDECL *RemoveFromEntity)(struct world_s *world, wedict_t *ed);
	qboolean (QDECL *RagMatrixToBody)(rbebody_t *bodyptr, float *mat);
	qboolean (QDECL *RagCreateBody)(struct world_s *world, rbebody_t *bodyptr, rbebodyinfo_t *bodyinfo, float *mat, wedict_t *ent);
	void (QDECL *RagMatrixFromJoint)(rbejoint_t *joint, rbejointinfo_t *info, float *mat);
	void (QDECL *RagMatrixFromBody)(struct world_s *world, rbebody_t *bodyptr, float *mat);
	void (QDECL *RagEnableJoint)(rbejoint_t *joint, qboolean enabled);
	void (QDECL *RagCreateJoint)(struct world_s *world, rbejoint_t *joint, rbejointinfo_t *info, rbebody_t *body1, rbebody_t *body2, vec3_t aaa2[3]);
	void (QDECL *RagDestroyBody)(struct world_s *world, rbebody_t *bodyptr);
	void (QDECL *RagDestroyJoint)(struct world_s *world, rbejoint_t *joint);
	void (QDECL *RunFrame)(struct world_s *world, double frametime, double gravity);
	void (QDECL *PushCommand)(struct world_s *world, rbecommandqueue_t *cmd);
//	void (QDECL *ExpandBodyAABB)(struct world_s *world, rbebody_t *bodyptr, float *mins, float *maxs);	//expands an aabb to include the size of the body.
	void (QDECL *Trace) (struct world_s *world, wedict_t *ed, vec3_t start, vec3_t end, trace_t *trace);
} rigidbodyengine_t;
#endif

struct world_s
{
	void (QDECL *Event_Touch)(struct world_s *w, wedict_t *s, wedict_t *o, trace_t *trace);
	void (QDECL *Event_Think)(struct world_s *w, wedict_t *s);
	void (QDECL *Event_Sound) (float *origin, wedict_t *entity, int channel, const char *sample, int volume, float attenuation, float pitchadj, float timeoffset, unsigned int flags);
	qboolean (QDECL *Event_ContentsTransition) (struct world_s *w, wedict_t *ent, int oldwatertype, int newwatertype);
	model_t *(QDECL *Get_CModel)(struct world_s *w, int modelindex);
	void (QDECL *Get_FrameState)(struct world_s *w, wedict_t *s, framestate_t *fstate);
	void (QDECL *Event_Backdate)(struct world_s *w, wedict_t *s, float timestamp);	//called for MOVE_LAGGED+MOVE_HITMODEL traces

	unsigned int	keydestmask;	//menu:kdm_menu, csqc:kdm_game, server:0
	unsigned int	max_edicts;	//limiting factor... 1024 fields*4*MAX_EDICTS == a heck of a lot.
	unsigned int	num_edicts;			// increases towards MAX_EDICTS
/*FTE_DEPRECATED*/	unsigned int	edict_size; //still used in copyentity
	wedict_t		*edicts;			// can NOT be array indexed.
	struct pubprogfuncs_s *progs;
	qboolean		usesolidcorpse;	//to disable SOLID_CORPSE when running hexen2 due to conflict.
	model_t			*worldmodel;
	unsigned int	spawncount;	//number of times it got restarted, so we can stop events from happening after vm restarts
	qboolean		remasterlogic;	//workarounds needed

#ifdef USEAREAGRID
	vec2_t			gridbias;
	vec2_t			gridscale;
	size_t			gridsize[2];
	areagridlink_t	*gridareas;		//[gridsize[0]*gridsize[1]]
	areagridlink_t	jumboarea;		//node containing ents too large to fit.
	areagridlink_t	portallist;
#endif

	double		physicstime;		// the last time global physics were run
	unsigned int    framenum;
	int			lastcheck;			// used by PF_checkclient
	double		lastchecktime;		// for monster ai
	qbyte		*lastcheckpvs;		// for monster ai

	/*antilag*/
	float	lagentsfrac;
	float	lagentstime;
	laggedentinfo_t *lagents;
	unsigned int maxlagents;

	/*qc globals*/
	struct {
		pint_t     *self;
		pint_t     *other;
		pint_t     *newmis;
		pvec_t	*time;
		pvec_t	*frametime;
		pvec_t	*force_retouch;
		pvec_t	*physics_mode;
		pvec_t	*v_forward;
		pvec_t	*v_right;
		pvec_t	*v_up;
		pvec_t	*defaultgravitydir;

		//used by menu+csqc.
		pvec_t *drawfont;
		pvec_t *drawfontscale;
	} g;
	struct qcstate_s *qcthreads;

#ifdef USERBE
	qboolean rbe_hasphysicsents;
	rigidbodyengine_t *rbe;
#endif

#if defined(Q2SERVER) || !defined(USEAREAGRID)
	areanode_t		*areanodes;
	int				areanodedepth;
	int				numareanodes;
#ifndef USEAREAGRID
	areanode_t		portallist;
#endif
#endif

#ifdef ENGINE_ROUTING
	void *waypoints;
#endif
};
typedef struct world_s world_t;

void PF_Common_RegisterCvars(void);




qboolean QDECL World_RegisterPhysicsEngine(const char *enginename, void(QDECL*World_Bullet_Start)(world_t*world));
void QDECL World_UnregisterPhysicsEngine(const char *enginename);
qboolean QDECL World_GenerateCollisionMesh(world_t *world, model_t *mod, wedict_t *ed, vec3_t geomcenter);
void QDECL World_ReleaseCollisionMesh(wedict_t *ed);
















void World_Destroy (world_t *w);
void World_RBE_Start(world_t *world);
void World_RBE_Shutdown(world_t *world);


void World_ClearWorld (world_t *w, qboolean relink);
// called after the world model has been loaded, before linking any entities
void World_ClearWorld_Nodes (world_t *w, qboolean relink); //legacy code for q2 compat.

void World_UnlinkEdict (wedict_t *ent);
// call before removing an entity, and before trying to move one,
// so it doesn't clip against itself
// flags ent->v.modified

void QDECL World_LinkEdict (world_t *w, wedict_t *ent, qboolean touch_triggers);
// Needs to be called any time an entity changes origin, mins, maxs, or solid
// flags ent->v.modified
// sets ent->v.absmin and ent->v.absmax
// if touchtriggers, calls prog functions for the intersected triggers

#define AREA_ALL 0
#define AREA_SOLID 1
#define AREA_TRIGGER 2
extern int World_AreaEdicts (world_t *w, vec3_t mins, vec3_t maxs, wedict_t **list, int maxcount, int areatype);

#ifdef USEAREAGRID
void World_TouchAllLinks (world_t *w, wedict_t *ent);
extern size_t areagridsequence;
#else
void World_TouchLinks (world_t *w, wedict_t *ent, areanode_t *node);
#define World_TouchAllLinks(w,e) World_TouchLinks(w,e,(w)->areanodes)
#endif

int World_PointContentsWorldOnly (world_t *w, vec3_t p);
int World_PointContentsAllBSPs (world_t *w, vec3_t p);
// returns the CONTENTS_* value from the world at the given point.
// does not check any entities at all

wedict_t	*World_TestEntityPosition (world_t *w, wedict_t *ent);

qboolean World_TransformedTrace (struct model_s *model, int hulloverride, framestate_t *framestate, vec3_t start, vec3_t end, vec3_t mins, vec3_t maxs, qboolean capsule, struct trace_s *trace, vec3_t origin, vec3_t angles, unsigned int hitcontentsmask);

/*
 World_Move:
 mins and maxs are reletive

 if the entire move stays in a solid volume, trace.allsolid will be set

 if the starting point is in a solid, it will be allowed to move out
 to an open area

 nomonsters is used for line of sight or edge testing, where mosnters
 shouldn't be considered solid objects

 passedict is explicitly excluded from clipping checks (normally NULL)
*/
trace_t World_Move (world_t *w, vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int type, wedict_t *passedict);


#ifdef Q2SERVER
typedef struct q2edict_s q2edict_t;

void VARGS WorldQ2_LinkEdict(world_t *w, q2edict_t *ent);
void VARGS WorldQ2_UnlinkEdict(world_t *w, q2edict_t *ent);
int VARGS WorldQ2_AreaEdicts (world_t *w, const vec3_t mins, const vec3_t maxs, q2edict_t **list,
	int maxcount, int areatype);
trace_t WorldQ2_Move (world_t *w, vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, int hitcontentsmask, q2edict_t *passedict);
#endif


/*sv_move.c*/
#if defined(CSQC_DAT) || !defined(CLIENTONLY)
qboolean World_CheckBottom (world_t *world, wedict_t *ent, vec3_t up);
qboolean World_movestep (world_t *world, wedict_t *ent, vec3_t move, vec3_t axis[3], qboolean relink, qboolean noenemy, void (*set_move_trace)(pubprogfuncs_t *inst, trace_t *trace));
qboolean World_MoveToGoal (world_t *world, wedict_t *ent, float dist);
qboolean World_GetEntGravityAxis(wedict_t *ent, vec3_t axis[3]);
#endif

//
// sv_phys.c
//
void WPhys_Init(void);
void World_Physics_Frame(world_t *w);
void SV_SetMoveVars(void);
void WPhys_RunNewmis (world_t *w);
qboolean SV_Physics (void);
void WPhys_CheckVelocity (world_t *w, wedict_t *ent);
trace_t WPhys_Trace_Toss (world_t *w, wedict_t *ent, wedict_t *ignore);
void SV_ProgStartFrame (void);
void WPhys_RunEntity (world_t *w, wedict_t *ent);
qboolean WPhys_RunThink (world_t *w, wedict_t *ent);
