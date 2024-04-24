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
// world.c -- world query functions

#include "quakedef.h"
#include "clvm_cmds.h"
#include "cl_collision.h"
#include "com_list.h"

/*

entities never clip against themselves, or their owner

line of sight checks trace->inopen and trace->inwater, but bullets don't

*/

#ifdef USEODE
static void World_Physics_Init(void);
#endif
void World_Init(void)
{
	Collision_Init();
#ifdef USEODE
	World_Physics_Init();
#endif
}

#ifdef USEODE
static void World_Physics_Shutdown(void);
#endif
void World_Shutdown(void)
{
#ifdef USEODE
	World_Physics_Shutdown();
#endif
}

#ifdef USEODE
static void World_Physics_Start(world_t *world);
#endif
void World_Start(world_t *world)
{
#ifdef USEODE
	World_Physics_Start(world);
#endif
}

#ifdef USEODE
static void World_Physics_End(world_t *world);
#endif
void World_End(world_t *world)
{
#ifdef USEODE
	World_Physics_End(world);
#endif
}

//============================================================================

/// World_ClearLink is used for new headnodes
void World_ClearLink (link_t *l)
{
	l->entitynumber = 0;
	l->list.prev = l->list.next = &l->list;
}

void World_RemoveLink (link_t *l)
{
	List_Delete(&l->list);
}

void World_InsertLinkBefore (link_t *l, link_t *before, int entitynumber)
{
	l->entitynumber = entitynumber;
	List_Add_Tail(&l->list, &before->list);
}

/*
===============================================================================

ENTITY AREA CHECKING

===============================================================================
*/

void World_PrintAreaStats(world_t *world, const char *worldname)
{
	Con_Printf("%s areagrid check stats: %d calls %d nodes (%f per call) %d entities (%f per call)\n", worldname, world->areagrid_stats_calls, world->areagrid_stats_nodechecks, (double) world->areagrid_stats_nodechecks / (double) world->areagrid_stats_calls, world->areagrid_stats_entitychecks, (double) world->areagrid_stats_entitychecks / (double) world->areagrid_stats_calls);
	world->areagrid_stats_calls = 0;
	world->areagrid_stats_nodechecks = 0;
	world->areagrid_stats_entitychecks = 0;
}

/*
===============
World_SetSize

===============
*/
void World_SetSize(world_t *world, const char *filename, const vec3_t mins, const vec3_t maxs, prvm_prog_t *prog)
{
	int i;

	dp_strlcpy(world->filename, filename, sizeof(world->filename));
	VectorCopy(mins, world->mins);
	VectorCopy(maxs, world->maxs);
	world->prog = prog;

	// the areagrid_marknumber is not allowed to be 0
	if (world->areagrid_marknumber < 1)
		world->areagrid_marknumber = 1;
	// choose either the world box size, or a larger box to ensure the grid isn't too fine
	world->areagrid_size[0] = max(world->maxs[0] - world->mins[0], AREA_GRID * sv_areagrid_mingridsize.value);
	world->areagrid_size[1] = max(world->maxs[1] - world->mins[1], AREA_GRID * sv_areagrid_mingridsize.value);
	world->areagrid_size[2] = max(world->maxs[2] - world->mins[2], AREA_GRID * sv_areagrid_mingridsize.value);
	// figure out the corners of such a box, centered at the center of the world box
	world->areagrid_mins[0] = (world->mins[0] + world->maxs[0] - world->areagrid_size[0]) * 0.5f;
	world->areagrid_mins[1] = (world->mins[1] + world->maxs[1] - world->areagrid_size[1]) * 0.5f;
	world->areagrid_mins[2] = (world->mins[2] + world->maxs[2] - world->areagrid_size[2]) * 0.5f;
	world->areagrid_maxs[0] = (world->mins[0] + world->maxs[0] + world->areagrid_size[0]) * 0.5f;
	world->areagrid_maxs[1] = (world->mins[1] + world->maxs[1] + world->areagrid_size[1]) * 0.5f;
	world->areagrid_maxs[2] = (world->mins[2] + world->maxs[2] + world->areagrid_size[2]) * 0.5f;
	// now calculate the actual useful info from that
	VectorNegate(world->areagrid_mins, world->areagrid_bias);
	world->areagrid_scale[0] = AREA_GRID / world->areagrid_size[0];
	world->areagrid_scale[1] = AREA_GRID / world->areagrid_size[1];
	world->areagrid_scale[2] = AREA_GRID / world->areagrid_size[2];
	World_ClearLink(&world->areagrid_outside);
	for (i = 0;i < AREA_GRIDNODES;i++)
		World_ClearLink(&world->areagrid[i]);
	if (developer_extra.integer)
		Con_DPrintf("areagrid settings: divisions %ix%ix1 : box %f %f %f : %f %f %f size %f %f %f grid %f %f %f (mingrid %f)\n", AREA_GRID, AREA_GRID, world->areagrid_mins[0], world->areagrid_mins[1], world->areagrid_mins[2], world->areagrid_maxs[0], world->areagrid_maxs[1], world->areagrid_maxs[2], world->areagrid_size[0], world->areagrid_size[1], world->areagrid_size[2], 1.0f / world->areagrid_scale[0], 1.0f / world->areagrid_scale[1], 1.0f / world->areagrid_scale[2], sv_areagrid_mingridsize.value);
}

/*
===============
World_UnlinkAll

===============
*/
void World_UnlinkAll(world_t *world)
{
	prvm_prog_t *prog = world->prog;
	int i;
	link_t *grid;
	// unlink all entities one by one
	grid = &world->areagrid_outside;
	while (grid->list.next != &grid->list)
		World_UnlinkEdict(PRVM_EDICT_NUM(List_Entry(grid->list.next, link_t, list)->entitynumber));
	for (i = 0, grid = world->areagrid;i < AREA_GRIDNODES;i++, grid++)
		while (grid->list.next != &grid->list)
			World_UnlinkEdict(PRVM_EDICT_NUM(List_Entry(grid->list.next, link_t, list)->entitynumber));
}

/*
===============

===============
*/
void World_UnlinkEdict(prvm_edict_t *ent)
{
	int i;
	for (i = 0;i < ENTITYGRIDAREAS;i++)
	{
		if (ent->priv.server->areagrid[i].list.prev)
			World_RemoveLink (&ent->priv.server->areagrid[i]);
	}
}

int World_EntitiesInBox(world_t *world, const vec3_t requestmins, const vec3_t requestmaxs, int maxlist, prvm_edict_t **list)
{
	prvm_prog_t *prog = world->prog;
	int numlist;
	link_t *grid;
	link_t *l;
	prvm_edict_t *ent;
	vec3_t paddedmins, paddedmaxs;
	int igrid[3], igridmins[3], igridmaxs[3];

	// avoid crash in showtex code on level change
	if (prog == NULL || prog->num_edicts < 1)
		return 0;

	// LadyHavoc: discovered this actually causes its own bugs (dm6 teleporters being too close to info_teleport_destination)
	//VectorSet(paddedmins, requestmins[0] - 1.0f, requestmins[1] - 1.0f, requestmins[2] - 1.0f);
	//VectorSet(paddedmaxs, requestmaxs[0] + 1.0f, requestmaxs[1] + 1.0f, requestmaxs[2] + 1.0f);
	VectorCopy(requestmins, paddedmins);
	VectorCopy(requestmaxs, paddedmaxs);

	// FIXME: if areagrid_marknumber wraps, all entities need their
	// ent->priv.server->areagridmarknumber reset
	world->areagrid_stats_calls++;
	world->areagrid_marknumber++;
	igridmins[0] = (int) floor((paddedmins[0] + world->areagrid_bias[0]) * world->areagrid_scale[0]);
	igridmins[1] = (int) floor((paddedmins[1] + world->areagrid_bias[1]) * world->areagrid_scale[1]);
	//igridmins[2] = (int) ((paddedmins[2] + world->areagrid_bias[2]) * world->areagrid_scale[2]);
	igridmaxs[0] = (int) floor((paddedmaxs[0] + world->areagrid_bias[0]) * world->areagrid_scale[0]) + 1;
	igridmaxs[1] = (int) floor((paddedmaxs[1] + world->areagrid_bias[1]) * world->areagrid_scale[1]) + 1;
	//igridmaxs[2] = (int) ((paddedmaxs[2] + world->areagrid_bias[2]) * world->areagrid_scale[2]) + 1;
	igridmins[0] = max(0, igridmins[0]);
	igridmins[1] = max(0, igridmins[1]);
	//igridmins[2] = max(0, igridmins[2]);
	igridmaxs[0] = min(AREA_GRID, igridmaxs[0]);
	igridmaxs[1] = min(AREA_GRID, igridmaxs[1]);
	//igridmaxs[2] = min(AREA_GRID, igridmaxs[2]);

	// paranoid debugging
	//VectorSet(igridmins, 0, 0, 0);VectorSet(igridmaxs, AREA_GRID, AREA_GRID, AREA_GRID);

	numlist = 0;
	// add entities not linked into areagrid because they are too big or
	// outside the grid bounds
	if (world->areagrid_outside.list.next)
	{
		grid = &world->areagrid_outside;
		List_For_Each_Entry(l, &grid->list, link_t, list)
		{
			ent = PRVM_EDICT_NUM(l->entitynumber);
			if (ent->priv.server->areagridmarknumber != world->areagrid_marknumber)
			{
				ent->priv.server->areagridmarknumber = world->areagrid_marknumber;
				if (!ent->free && BoxesOverlap(paddedmins, paddedmaxs, ent->priv.server->areamins, ent->priv.server->areamaxs))
				{
					if (numlist < maxlist)
						list[numlist] = ent;
					numlist++;
				}
				world->areagrid_stats_entitychecks++;
			}
		}
	}
	// add grid linked entities
	for (igrid[1] = igridmins[1];igrid[1] < igridmaxs[1];igrid[1]++)
	{
		grid = world->areagrid + igrid[1] * AREA_GRID + igridmins[0];
		for (igrid[0] = igridmins[0];igrid[0] < igridmaxs[0];igrid[0]++, grid++)
		{
			if (grid->list.next)
			{
				List_For_Each_Entry(l, &grid->list, link_t, list)
				{
					ent = PRVM_EDICT_NUM(l->entitynumber);
					if (ent->priv.server->areagridmarknumber != world->areagrid_marknumber)
					{
						ent->priv.server->areagridmarknumber = world->areagrid_marknumber;
						if (!ent->free && BoxesOverlap(paddedmins, paddedmaxs, ent->priv.server->areamins, ent->priv.server->areamaxs))
						{
							if (numlist < maxlist)
								list[numlist] = ent;
							numlist++;
						}
						//Con_Printf("%d %f %f %f %f %f %f : %d : %f %f %f %f %f %f\n", BoxesOverlap(mins, maxs, ent->priv.server->areamins, ent->priv.server->areamaxs), ent->priv.server->areamins[0], ent->priv.server->areamins[1], ent->priv.server->areamins[2], ent->priv.server->areamaxs[0], ent->priv.server->areamaxs[1], ent->priv.server->areamaxs[2], PRVM_NUM_FOR_EDICT(ent), mins[0], mins[1], mins[2], maxs[0], maxs[1], maxs[2]);
					}
					world->areagrid_stats_entitychecks++;
				}
			}
		}
	}
	return numlist;
}

static void World_LinkEdict_AreaGrid(world_t *world, prvm_edict_t *ent)
{
	prvm_prog_t *prog = world->prog;
	link_t *grid;
	int igrid[3], igridmins[3], igridmaxs[3], gridnum, entitynumber = PRVM_NUM_FOR_EDICT(ent);

	if (entitynumber <= 0 || entitynumber >= prog->max_edicts || PRVM_EDICT_NUM(entitynumber) != ent)
	{
		Con_Printf ("World_LinkEdict_AreaGrid: invalid edict %p (edicts is %p, edict compared to prog->edicts is %i)\n", (void *)ent, (void *)prog->edicts, entitynumber);
		return;
	}

	igridmins[0] = (int) floor((ent->priv.server->areamins[0] + world->areagrid_bias[0]) * world->areagrid_scale[0]);
	igridmins[1] = (int) floor((ent->priv.server->areamins[1] + world->areagrid_bias[1]) * world->areagrid_scale[1]);
	//igridmins[2] = (int) floor((ent->priv.server->areamins[2] + world->areagrid_bias[2]) * world->areagrid_scale[2]);
	igridmaxs[0] = (int) floor((ent->priv.server->areamaxs[0] + world->areagrid_bias[0]) * world->areagrid_scale[0]) + 1;
	igridmaxs[1] = (int) floor((ent->priv.server->areamaxs[1] + world->areagrid_bias[1]) * world->areagrid_scale[1]) + 1;
	//igridmaxs[2] = (int) floor((ent->priv.server->areamaxs[2] + world->areagrid_bias[2]) * world->areagrid_scale[2]) + 1;
	if (igridmins[0] < 0 || igridmaxs[0] > AREA_GRID || igridmins[1] < 0 || igridmaxs[1] > AREA_GRID || ((igridmaxs[0] - igridmins[0]) * (igridmaxs[1] - igridmins[1])) > ENTITYGRIDAREAS)
	{
		// wow, something outside the grid, store it as such
		World_InsertLinkBefore (&ent->priv.server->areagrid[0], &world->areagrid_outside, entitynumber);
		return;
	}

	gridnum = 0;
	for (igrid[1] = igridmins[1];igrid[1] < igridmaxs[1];igrid[1]++)
	{
		grid = world->areagrid + igrid[1] * AREA_GRID + igridmins[0];
		for (igrid[0] = igridmins[0];igrid[0] < igridmaxs[0];igrid[0]++, grid++, gridnum++)
			World_InsertLinkBefore (&ent->priv.server->areagrid[gridnum], grid, entitynumber);
	}
}

/*
===============
World_LinkEdict

===============
*/
void World_LinkEdict(world_t *world, prvm_edict_t *ent, const vec3_t mins, const vec3_t maxs, qbool link_solid_not)
{
	prvm_prog_t *prog = world->prog;
	// unlink from old position first
	if (ent->priv.server->areagrid[0].list.prev)
		World_UnlinkEdict(ent);

	// some games don't want SOLID_NOT entities linked
	if (!link_solid_not && PRVM_serveredictfloat(ent, solid) == SOLID_NOT)
		return;

	// don't add the world
	if (ent == prog->edicts)
		return;

	// don't add free entities
	if (ent->free)
		return;

	VectorCopy(mins, ent->priv.server->areamins);
	VectorCopy(maxs, ent->priv.server->areamaxs);
	World_LinkEdict_AreaGrid(world, ent);
}




//============================================================================
// physics engine support
//============================================================================

#ifdef USEODE
cvar_t physics_ode_quadtree_depth = {CF_CLIENT | CF_SERVER, "physics_ode_quadtree_depth","5", "desired subdivision level of quadtree culling space"};
cvar_t physics_ode_allowconvex = {CF_CLIENT | CF_SERVER, "physics_ode_allowconvex", "0", "allow usage of Convex Hull primitive type on trimeshes that have custom 'collisionconvex' mesh. If disabled, trimesh primitive type are used."};
cvar_t physics_ode_contactsurfacelayer = {CF_CLIENT | CF_SERVER, "physics_ode_contactsurfacelayer","1", "allows objects to overlap this many units to reduce jitter"};
cvar_t physics_ode_worldstep_iterations = {CF_CLIENT | CF_SERVER, "physics_ode_worldstep_iterations", "20", "parameter to dWorldQuickStep"};
cvar_t physics_ode_contact_mu = {CF_CLIENT | CF_SERVER, "physics_ode_contact_mu", "1", "contact solver mu parameter - friction pyramid approximation 1 (see ODE User Guide)"};
cvar_t physics_ode_contact_erp = {CF_CLIENT | CF_SERVER, "physics_ode_contact_erp", "0.96", "contact solver erp parameter - Error Restitution Percent (see ODE User Guide)"};
cvar_t physics_ode_contact_cfm = {CF_CLIENT | CF_SERVER, "physics_ode_contact_cfm", "0", "contact solver cfm parameter - Constraint Force Mixing (see ODE User Guide)"};
cvar_t physics_ode_contact_maxpoints = {CF_CLIENT | CF_SERVER, "physics_ode_contact_maxpoints", "16", "maximal number of contact points between 2 objects, higher = stable (and slower), can be up to 32"};
cvar_t physics_ode_world_erp = {CF_CLIENT | CF_SERVER, "physics_ode_world_erp", "-1", "world solver erp parameter - Error Restitution Percent (see ODE User Guide); use defaults when set to -1"};
cvar_t physics_ode_world_cfm = {CF_CLIENT | CF_SERVER, "physics_ode_world_cfm", "-1", "world solver cfm parameter - Constraint Force Mixing (see ODE User Guide); not touched when -1"};
cvar_t physics_ode_world_damping = {CF_CLIENT | CF_SERVER, "physics_ode_world_damping", "1", "enabled damping scale (see ODE User Guide), this scales all damping values, be aware that behavior depends of step type"};
cvar_t physics_ode_world_damping_linear = {CF_CLIENT | CF_SERVER, "physics_ode_world_damping_linear", "0.01", "world linear damping scale (see ODE User Guide); use defaults when set to -1"};
cvar_t physics_ode_world_damping_linear_threshold = {CF_CLIENT | CF_SERVER, "physics_ode_world_damping_linear_threshold", "0.1", "world linear damping threshold (see ODE User Guide); use defaults when set to -1"};
cvar_t physics_ode_world_damping_angular = {CF_CLIENT | CF_SERVER, "physics_ode_world_damping_angular", "0.05", "world angular damping scale (see ODE User Guide); use defaults when set to -1"};
cvar_t physics_ode_world_damping_angular_threshold = {CF_CLIENT | CF_SERVER, "physics_ode_world_damping_angular_threshold", "0.1", "world angular damping threshold (see ODE User Guide); use defaults when set to -1"};
cvar_t physics_ode_world_gravitymod = {CF_CLIENT | CF_SERVER, "physics_ode_world_gravitymod", "1", "multiplies gravity got from sv_gravity, this may be needed to tweak if strong damping is used"};
cvar_t physics_ode_iterationsperframe = {CF_CLIENT | CF_SERVER, "physics_ode_iterationsperframe", "1", "divisor for time step, runs multiple physics steps per frame"};
cvar_t physics_ode_constantstep = {CF_CLIENT | CF_SERVER, "physics_ode_constantstep", "0", "use constant step instead of variable step which tends to increase stability, if set to 1 uses sys_ticrate, instead uses it's own value"};
cvar_t physics_ode_autodisable = {CF_CLIENT | CF_SERVER, "physics_ode_autodisable", "1", "automatic disabling of objects which dont move for long period of time, makes object stacking a lot faster"};
cvar_t physics_ode_autodisable_steps = {CF_CLIENT | CF_SERVER, "physics_ode_autodisable_steps", "10", "how many steps object should be dormant to be autodisabled"};
cvar_t physics_ode_autodisable_time = {CF_CLIENT | CF_SERVER, "physics_ode_autodisable_time", "0", "how many seconds object should be dormant to be autodisabled"};
cvar_t physics_ode_autodisable_threshold_linear = {CF_CLIENT | CF_SERVER, "physics_ode_autodisable_threshold_linear", "0.6", "body will be disabled if it's linear move below this value"};
cvar_t physics_ode_autodisable_threshold_angular = {CF_CLIENT | CF_SERVER, "physics_ode_autodisable_threshold_angular", "6", "body will be disabled if it's angular move below this value"};
cvar_t physics_ode_autodisable_threshold_samples = {CF_CLIENT | CF_SERVER, "physics_ode_autodisable_threshold_samples", "5", "average threshold with this number of samples"};
cvar_t physics_ode_movelimit = {CF_CLIENT | CF_SERVER, "physics_ode_movelimit", "0.5", "clamp velocity if a single move would exceed this percentage of object thickness, to prevent flying through walls, be aware that behavior depends of step type"};
cvar_t physics_ode_spinlimit = {CF_CLIENT | CF_SERVER, "physics_ode_spinlimit", "10000", "reset spin velocity if it gets too large"};
cvar_t physics_ode_trick_fixnan = {CF_CLIENT | CF_SERVER, "physics_ode_trick_fixnan", "1", "engine trick that checks and fixes NaN velocity/origin/angles on objects, a value of 2 makes console prints on each fix"};
cvar_t physics_ode_printstats = {CF_CLIENT | CF_SERVER, "physics_ode_printstats", "0", "print ODE stats each frame"};

cvar_t physics_ode = {CF_CLIENT | CF_SERVER, "physics_ode", "0", "run ODE physics (VERY experimental and potentially buggy)"};

// LadyHavoc: this large chunk of definitions comes from the ODE library
// include files.

#ifdef LINK_TO_LIBODE
#include "ode/ode.h"
#else
#ifdef WINAPI
// ODE does not use WINAPI
#define ODE_API
#else
#define ODE_API
#endif

// note: dynamic builds of ODE tend to be double precision, this is not used
// for static builds
typedef double dReal;

typedef dReal dVector3[4];
typedef dReal dVector4[4];
typedef dReal dMatrix3[4*3];
typedef dReal dMatrix4[4*4];
typedef dReal dMatrix6[8*6];
typedef dReal dQuaternion[4];

struct dxWorld;		/* dynamics world */
struct dxSpace;		/* collision space */
struct dxBody;		/* rigid body (dynamics object) */
struct dxGeom;		/* geometry (collision object) */
struct dxJoint;
struct dxJointNode;
struct dxJointGroup;
struct dxTriMeshData;

#define dInfinity 3.402823466e+38f

typedef struct dxWorld *dWorldID;
typedef struct dxSpace *dSpaceID;
typedef struct dxBody *dBodyID;
typedef struct dxGeom *dGeomID;
typedef struct dxJoint *dJointID;
typedef struct dxJointGroup *dJointGroupID;
typedef struct dxTriMeshData *dTriMeshDataID;

typedef struct dJointFeedback
{
	dVector3 f1;		/* force applied to body 1 */
	dVector3 t1;		/* torque applied to body 1 */
	dVector3 f2;		/* force applied to body 2 */
	dVector3 t2;		/* torque applied to body 2 */
}
dJointFeedback;

typedef enum dJointType
{
	dJointTypeNone = 0,
	dJointTypeBall,
	dJointTypeHinge,
	dJointTypeSlider,
	dJointTypeContact,
	dJointTypeUniversal,
	dJointTypeHinge2,
	dJointTypeFixed,
	dJointTypeNull,
	dJointTypeAMotor,
	dJointTypeLMotor,
	dJointTypePlane2D,
	dJointTypePR,
	dJointTypePU,
	dJointTypePiston
}
dJointType;

#define D_ALL_PARAM_NAMES(start) \
  /* parameters for limits and motors */ \
  dParamLoStop = start, \
  dParamHiStop, \
  dParamVel, \
  dParamFMax, \
  dParamFudgeFactor, \
  dParamBounce, \
  dParamCFM, \
  dParamStopERP, \
  dParamStopCFM, \
  /* parameters for suspension */ \
  dParamSuspensionERP, \
  dParamSuspensionCFM, \
  dParamERP, \

#define D_ALL_PARAM_NAMES_X(start,x) \
  /* parameters for limits and motors */ \
  dParamLoStop ## x = start, \
  dParamHiStop ## x, \
  dParamVel ## x, \
  dParamFMax ## x, \
  dParamFudgeFactor ## x, \
  dParamBounce ## x, \
  dParamCFM ## x, \
  dParamStopERP ## x, \
  dParamStopCFM ## x, \
  /* parameters for suspension */ \
  dParamSuspensionERP ## x, \
  dParamSuspensionCFM ## x, \
  dParamERP ## x,

enum {
  D_ALL_PARAM_NAMES(0)
  D_ALL_PARAM_NAMES_X(0x100,2)
  D_ALL_PARAM_NAMES_X(0x200,3)

  /* add a multiple of this constant to the basic parameter numbers to get
   * the parameters for the second, third etc axes.
   */
  dParamGroup=0x100
};

typedef struct dMass
{
	dReal mass;
	dVector3 c;
	dMatrix3 I;
}
dMass;

enum
{
	dContactMu2			= 0x001,
	dContactFDir1		= 0x002,
	dContactBounce		= 0x004,
	dContactSoftERP		= 0x008,
	dContactSoftCFM		= 0x010,
	dContactMotion1		= 0x020,
	dContactMotion2		= 0x040,
	dContactMotionN		= 0x080,
	dContactSlip1		= 0x100,
	dContactSlip2		= 0x200,
	
	dContactApprox0		= 0x0000,
	dContactApprox1_1	= 0x1000,
	dContactApprox1_2	= 0x2000,
	dContactApprox1		= 0x3000
};

typedef struct dSurfaceParameters
{
	/* must always be defined */
	int mode;
	dReal mu;

	/* only defined if the corresponding flag is set in mode */
	dReal mu2;
	dReal bounce;
	dReal bounce_vel;
	dReal soft_erp;
	dReal soft_cfm;
	dReal motion1,motion2,motionN;
	dReal slip1,slip2;
} dSurfaceParameters;

typedef struct dContactGeom
{
	dVector3 pos;          ///< contact position
	dVector3 normal;       ///< normal vector
	dReal depth;           ///< penetration depth
	dGeomID g1,g2;         ///< the colliding geoms
	int side1,side2;       ///< (to be documented)
}
dContactGeom;

typedef struct dContact
{
	dSurfaceParameters surface;
	dContactGeom geom;
	dVector3 fdir1;
}
dContact;

typedef void dNearCallback (void *data, dGeomID o1, dGeomID o2);

// SAP
// Order XZY or ZXY usually works best, if your Y is up.
#define dSAP_AXES_XYZ  ((0)|(1<<2)|(2<<4))
#define dSAP_AXES_XZY  ((0)|(2<<2)|(1<<4))
#define dSAP_AXES_YXZ  ((1)|(0<<2)|(2<<4))
#define dSAP_AXES_YZX  ((1)|(2<<2)|(0<<4))
#define dSAP_AXES_ZXY  ((2)|(0<<2)|(1<<4))
#define dSAP_AXES_ZYX  ((2)|(1<<2)|(0<<4))

const char*     (ODE_API *dGetConfiguration)(void);
int             (ODE_API *dCheckConfiguration)( const char* token );
int             (ODE_API *dInitODE)(void);
//int             (ODE_API *dInitODE2)(unsigned int uiInitFlags);
//int             (ODE_API *dAllocateODEDataForThread)(unsigned int uiAllocateFlags);
//void            (ODE_API *dCleanupODEAllDataForThread)(void);
void            (ODE_API *dCloseODE)(void);

//int             (ODE_API *dMassCheck)(const dMass *m);
//void            (ODE_API *dMassSetZero)(dMass *);
//void            (ODE_API *dMassSetParameters)(dMass *, dReal themass, dReal cgx, dReal cgy, dReal cgz, dReal I11, dReal I22, dReal I33, dReal I12, dReal I13, dReal I23);
//void            (ODE_API *dMassSetSphere)(dMass *, dReal density, dReal radius);
void            (ODE_API *dMassSetSphereTotal)(dMass *, dReal total_mass, dReal radius);
//void            (ODE_API *dMassSetCapsule)(dMass *, dReal density, int direction, dReal radius, dReal length);
void            (ODE_API *dMassSetCapsuleTotal)(dMass *, dReal total_mass, int direction, dReal radius, dReal length);
//void            (ODE_API *dMassSetCylinder)(dMass *, dReal density, int direction, dReal radius, dReal length);
void            (ODE_API *dMassSetCylinderTotal)(dMass *, dReal total_mass, int direction, dReal radius, dReal length);
//void            (ODE_API *dMassSetBox)(dMass *, dReal density, dReal lx, dReal ly, dReal lz);
void            (ODE_API *dMassSetBoxTotal)(dMass *, dReal total_mass, dReal lx, dReal ly, dReal lz);
//void            (ODE_API *dMassSetTrimesh)(dMass *, dReal density, dGeomID g);
//void            (ODE_API *dMassSetTrimeshTotal)(dMass *m, dReal total_mass, dGeomID g);
//void            (ODE_API *dMassAdjust)(dMass *, dReal newmass);
//void            (ODE_API *dMassTranslate)(dMass *, dReal x, dReal y, dReal z);
//void            (ODE_API *dMassRotate)(dMass *, const dMatrix3 R);
//void            (ODE_API *dMassAdd)(dMass *a, const dMass *b);
//
dWorldID        (ODE_API *dWorldCreate)(void);
void            (ODE_API *dWorldDestroy)(dWorldID world);
void            (ODE_API *dWorldSetGravity)(dWorldID, dReal x, dReal y, dReal z);
void            (ODE_API *dWorldGetGravity)(dWorldID, dVector3 gravity);
void            (ODE_API *dWorldSetERP)(dWorldID, dReal erp);
//dReal           (ODE_API *dWorldGetERP)(dWorldID);
void            (ODE_API *dWorldSetCFM)(dWorldID, dReal cfm);
//dReal           (ODE_API *dWorldGetCFM)(dWorldID);
//void            (ODE_API *dWorldStep)(dWorldID, dReal stepsize);
//void            (ODE_API *dWorldImpulseToForce)(dWorldID, dReal stepsize, dReal ix, dReal iy, dReal iz, dVector3 force);
void            (ODE_API *dWorldQuickStep)(dWorldID w, dReal stepsize);
void            (ODE_API *dWorldSetQuickStepNumIterations)(dWorldID, int num);
//int             (ODE_API *dWorldGetQuickStepNumIterations)(dWorldID);
//void            (ODE_API *dWorldSetQuickStepW)(dWorldID, dReal over_relaxation);
//dReal           (ODE_API *dWorldGetQuickStepW)(dWorldID);
//void            (ODE_API *dWorldSetContactMaxCorrectingVel)(dWorldID, dReal vel);
//dReal           (ODE_API *dWorldGetContactMaxCorrectingVel)(dWorldID);
void            (ODE_API *dWorldSetContactSurfaceLayer)(dWorldID, dReal depth);
//dReal           (ODE_API *dWorldGetContactSurfaceLayer)(dWorldID);
//void            (ODE_API *dWorldStepFast1)(dWorldID, dReal stepsize, int maxiterations);
//void            (ODE_API *dWorldSetAutoEnableDepthSF1)(dWorldID, int autoEnableDepth);
//int             (ODE_API *dWorldGetAutoEnableDepthSF1)(dWorldID);
//dReal           (ODE_API *dWorldGetAutoDisableLinearThreshold)(dWorldID);
void            (ODE_API *dWorldSetAutoDisableLinearThreshold)(dWorldID, dReal linear_threshold);
//dReal           (ODE_API *dWorldGetAutoDisableAngularThreshold)(dWorldID);
void            (ODE_API *dWorldSetAutoDisableAngularThreshold)(dWorldID, dReal angular_threshold);
//dReal           (ODE_API *dWorldGetAutoDisableLinearAverageThreshold)(dWorldID);
//void            (ODE_API *dWorldSetAutoDisableLinearAverageThreshold)(dWorldID, dReal linear_average_threshold);
//dReal           (ODE_API *dWorldGetAutoDisableAngularAverageThreshold)(dWorldID);
//void            (ODE_API *dWorldSetAutoDisableAngularAverageThreshold)(dWorldID, dReal angular_average_threshold);
//int             (ODE_API *dWorldGetAutoDisableAverageSamplesCount)(dWorldID);
void            (ODE_API *dWorldSetAutoDisableAverageSamplesCount)(dWorldID, unsigned int average_samples_count );
//int             (ODE_API *dWorldGetAutoDisableSteps)(dWorldID);
void            (ODE_API *dWorldSetAutoDisableSteps)(dWorldID, int steps);
//dReal           (ODE_API *dWorldGetAutoDisableTime)(dWorldID);
void            (ODE_API *dWorldSetAutoDisableTime)(dWorldID, dReal time);
//int             (ODE_API *dWorldGetAutoDisableFlag)(dWorldID);
void            (ODE_API *dWorldSetAutoDisableFlag)(dWorldID, int do_auto_disable);
//dReal           (ODE_API *dWorldGetLinearDampingThreshold)(dWorldID w);
void            (ODE_API *dWorldSetLinearDampingThreshold)(dWorldID w, dReal threshold);
//dReal           (ODE_API *dWorldGetAngularDampingThreshold)(dWorldID w);
void            (ODE_API *dWorldSetAngularDampingThreshold)(dWorldID w, dReal threshold);
//dReal           (ODE_API *dWorldGetLinearDamping)(dWorldID w);
void            (ODE_API *dWorldSetLinearDamping)(dWorldID w, dReal scale);
//dReal           (ODE_API *dWorldGetAngularDamping)(dWorldID w);
void            (ODE_API *dWorldSetAngularDamping)(dWorldID w, dReal scale);
//void            (ODE_API *dWorldSetDamping)(dWorldID w, dReal linear_scale, dReal angular_scale);
//dReal           (ODE_API *dWorldGetMaxAngularSpeed)(dWorldID w);
//void            (ODE_API *dWorldSetMaxAngularSpeed)(dWorldID w, dReal max_speed);
//dReal           (ODE_API *dBodyGetAutoDisableLinearThreshold)(dBodyID);
//void            (ODE_API *dBodySetAutoDisableLinearThreshold)(dBodyID, dReal linear_average_threshold);
//dReal           (ODE_API *dBodyGetAutoDisableAngularThreshold)(dBodyID);
//void            (ODE_API *dBodySetAutoDisableAngularThreshold)(dBodyID, dReal angular_average_threshold);
//int             (ODE_API *dBodyGetAutoDisableAverageSamplesCount)(dBodyID);
//void            (ODE_API *dBodySetAutoDisableAverageSamplesCount)(dBodyID, unsigned int average_samples_count);
//int             (ODE_API *dBodyGetAutoDisableSteps)(dBodyID);
//void            (ODE_API *dBodySetAutoDisableSteps)(dBodyID, int steps);
//dReal           (ODE_API *dBodyGetAutoDisableTime)(dBodyID);
//void            (ODE_API *dBodySetAutoDisableTime)(dBodyID, dReal time);
//int             (ODE_API *dBodyGetAutoDisableFlag)(dBodyID);
//void            (ODE_API *dBodySetAutoDisableFlag)(dBodyID, int do_auto_disable);
//void            (ODE_API *dBodySetAutoDisableDefaults)(dBodyID);
//dWorldID        (ODE_API *dBodyGetWorld)(dBodyID);
dBodyID         (ODE_API *dBodyCreate)(dWorldID);
void            (ODE_API *dBodyDestroy)(dBodyID);
void            (ODE_API *dBodySetData)(dBodyID, void *data);
void *          (ODE_API *dBodyGetData)(dBodyID);
void            (ODE_API *dBodySetPosition)(dBodyID, dReal x, dReal y, dReal z);
void            (ODE_API *dBodySetRotation)(dBodyID, const dMatrix3 R);
//void            (ODE_API *dBodySetQuaternion)(dBodyID, const dQuaternion q);
void            (ODE_API *dBodySetLinearVel)(dBodyID, dReal x, dReal y, dReal z);
void            (ODE_API *dBodySetAngularVel)(dBodyID, dReal x, dReal y, dReal z);
const dReal *   (ODE_API *dBodyGetPosition)(dBodyID);
//void            (ODE_API *dBodyCopyPosition)(dBodyID body, dVector3 pos);
const dReal *   (ODE_API *dBodyGetRotation)(dBodyID);
//void            (ODE_API *dBodyCopyRotation)(dBodyID, dMatrix3 R);
//const dReal *   (ODE_API *dBodyGetQuaternion)(dBodyID);
//void            (ODE_API *dBodyCopyQuaternion)(dBodyID body, dQuaternion quat);
const dReal *   (ODE_API *dBodyGetLinearVel)(dBodyID);
const dReal *   (ODE_API *dBodyGetAngularVel)(dBodyID);
void            (ODE_API *dBodySetMass)(dBodyID, const dMass *mass);
//void            (ODE_API *dBodyGetMass)(dBodyID, dMass *mass);
void            (ODE_API *dBodyAddForce)(dBodyID, dReal fx, dReal fy, dReal fz);
void            (ODE_API *dBodyAddTorque)(dBodyID, dReal fx, dReal fy, dReal fz);
//void            (ODE_API *dBodyAddRelForce)(dBodyID, dReal fx, dReal fy, dReal fz);
//void            (ODE_API *dBodyAddRelTorque)(dBodyID, dReal fx, dReal fy, dReal fz);
void            (ODE_API *dBodyAddForceAtPos)(dBodyID, dReal fx, dReal fy, dReal fz, dReal px, dReal py, dReal pz);
//void            (ODE_API *dBodyAddForceAtRelPos)(dBodyID, dReal fx, dReal fy, dReal fz, dReal px, dReal py, dReal pz);
//void            (ODE_API *dBodyAddRelForceAtPos)(dBodyID, dReal fx, dReal fy, dReal fz, dReal px, dReal py, dReal pz);
//void            (ODE_API *dBodyAddRelForceAtRelPos)(dBodyID, dReal fx, dReal fy, dReal fz, dReal px, dReal py, dReal pz);
//const dReal *   (ODE_API *dBodyGetForce)(dBodyID);
//const dReal *   (ODE_API *dBodyGetTorque)(dBodyID);
//void            (ODE_API *dBodySetForce)(dBodyID b, dReal x, dReal y, dReal z);
//void            (ODE_API *dBodySetTorque)(dBodyID b, dReal x, dReal y, dReal z);
//void            (ODE_API *dBodyGetRelPointPos)(dBodyID, dReal px, dReal py, dReal pz, dVector3 result);
//void            (ODE_API *dBodyGetRelPointVel)(dBodyID, dReal px, dReal py, dReal pz, dVector3 result);
//void            (ODE_API *dBodyGetPointVel)(dBodyID, dReal px, dReal py, dReal pz, dVector3 result);
//void            (ODE_API *dBodyGetPosRelPoint)(dBodyID, dReal px, dReal py, dReal pz, dVector3 result);
//void            (ODE_API *dBodyVectorToWorld)(dBodyID, dReal px, dReal py, dReal pz, dVector3 result);
//void            (ODE_API *dBodyVectorFromWorld)(dBodyID, dReal px, dReal py, dReal pz, dVector3 result);
//void            (ODE_API *dBodySetFiniteRotationMode)(dBodyID, int mode);
//void            (ODE_API *dBodySetFiniteRotationAxis)(dBodyID, dReal x, dReal y, dReal z);
//int             (ODE_API *dBodyGetFiniteRotationMode)(dBodyID);
//void            (ODE_API *dBodyGetFiniteRotationAxis)(dBodyID, dVector3 result);
int             (ODE_API *dBodyGetNumJoints)(dBodyID b);
dJointID        (ODE_API *dBodyGetJoint)(dBodyID, int index);
//void            (ODE_API *dBodySetDynamic)(dBodyID);
//void            (ODE_API *dBodySetKinematic)(dBodyID);
//int             (ODE_API *dBodyIsKinematic)(dBodyID);
void            (ODE_API *dBodyEnable)(dBodyID);
void            (ODE_API *dBodyDisable)(dBodyID);
int             (ODE_API *dBodyIsEnabled)(dBodyID);
void            (ODE_API *dBodySetGravityMode)(dBodyID b, int mode);
int             (ODE_API *dBodyGetGravityMode)(dBodyID b);
//void            (*dBodySetMovedCallback)(dBodyID b, void(ODE_API *callback)(dBodyID));
//dGeomID         (ODE_API *dBodyGetFirstGeom)(dBodyID b);
//dGeomID         (ODE_API *dBodyGetNextGeom)(dGeomID g);
//void            (ODE_API *dBodySetDampingDefaults)(dBodyID b);
//dReal           (ODE_API *dBodyGetLinearDamping)(dBodyID b);
//void            (ODE_API *dBodySetLinearDamping)(dBodyID b, dReal scale);
//dReal           (ODE_API *dBodyGetAngularDamping)(dBodyID b);
//void            (ODE_API *dBodySetAngularDamping)(dBodyID b, dReal scale);
//void            (ODE_API *dBodySetDamping)(dBodyID b, dReal linear_scale, dReal angular_scale);
//dReal           (ODE_API *dBodyGetLinearDampingThreshold)(dBodyID b);
//void            (ODE_API *dBodySetLinearDampingThreshold)(dBodyID b, dReal threshold);
//dReal           (ODE_API *dBodyGetAngularDampingThreshold)(dBodyID b);
//void            (ODE_API *dBodySetAngularDampingThreshold)(dBodyID b, dReal threshold);
//dReal           (ODE_API *dBodyGetMaxAngularSpeed)(dBodyID b);
//void            (ODE_API *dBodySetMaxAngularSpeed)(dBodyID b, dReal max_speed);
//int             (ODE_API *dBodyGetGyroscopicMode)(dBodyID b);
//void            (ODE_API *dBodySetGyroscopicMode)(dBodyID b, int enabled);
dJointID        (ODE_API *dJointCreateBall)(dWorldID, dJointGroupID);
dJointID        (ODE_API *dJointCreateHinge)(dWorldID, dJointGroupID);
dJointID        (ODE_API *dJointCreateSlider)(dWorldID, dJointGroupID);
dJointID        (ODE_API *dJointCreateContact)(dWorldID, dJointGroupID, const dContact *);
dJointID        (ODE_API *dJointCreateHinge2)(dWorldID, dJointGroupID);
dJointID        (ODE_API *dJointCreateUniversal)(dWorldID, dJointGroupID);
//dJointID        (ODE_API *dJointCreatePR)(dWorldID, dJointGroupID);
//dJointID        (ODE_API *dJointCreatePU)(dWorldID, dJointGroupID);
//dJointID        (ODE_API *dJointCreatePiston)(dWorldID, dJointGroupID);
dJointID        (ODE_API *dJointCreateFixed)(dWorldID, dJointGroupID);
//dJointID        (ODE_API *dJointCreateNull)(dWorldID, dJointGroupID);
//dJointID        (ODE_API *dJointCreateAMotor)(dWorldID, dJointGroupID);
//dJointID        (ODE_API *dJointCreateLMotor)(dWorldID, dJointGroupID);
//dJointID        (ODE_API *dJointCreatePlane2D)(dWorldID, dJointGroupID);
void            (ODE_API *dJointDestroy)(dJointID);
dJointGroupID   (ODE_API *dJointGroupCreate)(int max_size);
void            (ODE_API *dJointGroupDestroy)(dJointGroupID);
void            (ODE_API *dJointGroupEmpty)(dJointGroupID);
//int             (ODE_API *dJointGetNumBodies)(dJointID);
void            (ODE_API *dJointAttach)(dJointID, dBodyID body1, dBodyID body2);
//void            (ODE_API *dJointEnable)(dJointID);
//void            (ODE_API *dJointDisable)(dJointID);
//int             (ODE_API *dJointIsEnabled)(dJointID);
void            (ODE_API *dJointSetData)(dJointID, void *data);
void *          (ODE_API *dJointGetData)(dJointID);
//dJointType      (ODE_API *dJointGetType)(dJointID);
dBodyID         (ODE_API *dJointGetBody)(dJointID, int index);
//void            (ODE_API *dJointSetFeedback)(dJointID, dJointFeedback *);
//dJointFeedback *(ODE_API *dJointGetFeedback)(dJointID);
void            (ODE_API *dJointSetBallAnchor)(dJointID, dReal x, dReal y, dReal z);
//void            (ODE_API *dJointSetBallAnchor2)(dJointID, dReal x, dReal y, dReal z);
void            (ODE_API *dJointSetBallParam)(dJointID, int parameter, dReal value);
void            (ODE_API *dJointSetHingeAnchor)(dJointID, dReal x, dReal y, dReal z);
//void            (ODE_API *dJointSetHingeAnchorDelta)(dJointID, dReal x, dReal y, dReal z, dReal ax, dReal ay, dReal az);
void            (ODE_API *dJointSetHingeAxis)(dJointID, dReal x, dReal y, dReal z);
//void            (ODE_API *dJointSetHingeAxisOffset)(dJointID j, dReal x, dReal y, dReal z, dReal angle);
void            (ODE_API *dJointSetHingeParam)(dJointID, int parameter, dReal value);
//void            (ODE_API *dJointAddHingeTorque)(dJointID joint, dReal torque);
void            (ODE_API *dJointSetSliderAxis)(dJointID, dReal x, dReal y, dReal z);
//void            (ODE_API *dJointSetSliderAxisDelta)(dJointID, dReal x, dReal y, dReal z, dReal ax, dReal ay, dReal az);
void            (ODE_API *dJointSetSliderParam)(dJointID, int parameter, dReal value);
//void            (ODE_API *dJointAddSliderForce)(dJointID joint, dReal force);
void            (ODE_API *dJointSetHinge2Anchor)(dJointID, dReal x, dReal y, dReal z);
void            (ODE_API *dJointSetHinge2Axis1)(dJointID, dReal x, dReal y, dReal z);
void            (ODE_API *dJointSetHinge2Axis2)(dJointID, dReal x, dReal y, dReal z);
void            (ODE_API *dJointSetHinge2Param)(dJointID, int parameter, dReal value);
//void            (ODE_API *dJointAddHinge2Torques)(dJointID joint, dReal torque1, dReal torque2);
void            (ODE_API *dJointSetUniversalAnchor)(dJointID, dReal x, dReal y, dReal z);
void            (ODE_API *dJointSetUniversalAxis1)(dJointID, dReal x, dReal y, dReal z);
//void            (ODE_API *dJointSetUniversalAxis1Offset)(dJointID, dReal x, dReal y, dReal z, dReal offset1, dReal offset2);
void            (ODE_API *dJointSetUniversalAxis2)(dJointID, dReal x, dReal y, dReal z);
//void            (ODE_API *dJointSetUniversalAxis2Offset)(dJointID, dReal x, dReal y, dReal z, dReal offset1, dReal offset2);
void            (ODE_API *dJointSetUniversalParam)(dJointID, int parameter, dReal value);
//void            (ODE_API *dJointAddUniversalTorques)(dJointID joint, dReal torque1, dReal torque2);
//void            (ODE_API *dJointSetPRAnchor)(dJointID, dReal x, dReal y, dReal z);
//void            (ODE_API *dJointSetPRAxis1)(dJointID, dReal x, dReal y, dReal z);
//void            (ODE_API *dJointSetPRAxis2)(dJointID, dReal x, dReal y, dReal z);
//void            (ODE_API *dJointSetPRParam)(dJointID, int parameter, dReal value);
//void            (ODE_API *dJointAddPRTorque)(dJointID j, dReal torque);
//void            (ODE_API *dJointSetPUAnchor)(dJointID, dReal x, dReal y, dReal z);
//void            (ODE_API *dJointSetPUAnchorOffset)(dJointID, dReal x, dReal y, dReal z, dReal dx, dReal dy, dReal dz);
//void            (ODE_API *dJointSetPUAxis1)(dJointID, dReal x, dReal y, dReal z);
//void            (ODE_API *dJointSetPUAxis2)(dJointID, dReal x, dReal y, dReal z);
//void            (ODE_API *dJointSetPUAxis3)(dJointID, dReal x, dReal y, dReal z);
//void            (ODE_API *dJointSetPUAxisP)(dJointID id, dReal x, dReal y, dReal z);
//void            (ODE_API *dJointSetPUParam)(dJointID, int parameter, dReal value);
//void            (ODE_API *dJointAddPUTorque)(dJointID j, dReal torque);
//void            (ODE_API *dJointSetPistonAnchor)(dJointID, dReal x, dReal y, dReal z);
//void            (ODE_API *dJointSetPistonAnchorOffset)(dJointID j, dReal x, dReal y, dReal z, dReal dx, dReal dy, dReal dz);
//void            (ODE_API *dJointSetPistonParam)(dJointID, int parameter, dReal value);
//void            (ODE_API *dJointAddPistonForce)(dJointID joint, dReal force);
//void            (ODE_API *dJointSetFixed)(dJointID);
//void            (ODE_API *dJointSetFixedParam)(dJointID, int parameter, dReal value);
//void            (ODE_API *dJointSetAMotorNumAxes)(dJointID, int num);
//void            (ODE_API *dJointSetAMotorAxis)(dJointID, int anum, int rel, dReal x, dReal y, dReal z);
//void            (ODE_API *dJointSetAMotorAngle)(dJointID, int anum, dReal angle);
//void            (ODE_API *dJointSetAMotorParam)(dJointID, int parameter, dReal value);
//void            (ODE_API *dJointSetAMotorMode)(dJointID, int mode);
//void            (ODE_API *dJointAddAMotorTorques)(dJointID, dReal torque1, dReal torque2, dReal torque3);
//void            (ODE_API *dJointSetLMotorNumAxes)(dJointID, int num);
//void            (ODE_API *dJointSetLMotorAxis)(dJointID, int anum, int rel, dReal x, dReal y, dReal z);
//void            (ODE_API *dJointSetLMotorParam)(dJointID, int parameter, dReal value);
//void            (ODE_API *dJointSetPlane2DXParam)(dJointID, int parameter, dReal value);
//void            (ODE_API *dJointSetPlane2DYParam)(dJointID, int parameter, dReal value);
//void            (ODE_API *dJointSetPlane2DAngleParam)(dJointID, int parameter, dReal value);
//void            (ODE_API *dJointGetBallAnchor)(dJointID, dVector3 result);
//void            (ODE_API *dJointGetBallAnchor2)(dJointID, dVector3 result);
//dReal           (ODE_API *dJointGetBallParam)(dJointID, int parameter);
//void            (ODE_API *dJointGetHingeAnchor)(dJointID, dVector3 result);
//void            (ODE_API *dJointGetHingeAnchor2)(dJointID, dVector3 result);
//void            (ODE_API *dJointGetHingeAxis)(dJointID, dVector3 result);
//dReal           (ODE_API *dJointGetHingeParam)(dJointID, int parameter);
//dReal           (ODE_API *dJointGetHingeAngle)(dJointID);
//dReal           (ODE_API *dJointGetHingeAngleRate)(dJointID);
//dReal           (ODE_API *dJointGetSliderPosition)(dJointID);
//dReal           (ODE_API *dJointGetSliderPositionRate)(dJointID);
//void            (ODE_API *dJointGetSliderAxis)(dJointID, dVector3 result);
//dReal           (ODE_API *dJointGetSliderParam)(dJointID, int parameter);
//void            (ODE_API *dJointGetHinge2Anchor)(dJointID, dVector3 result);
//void            (ODE_API *dJointGetHinge2Anchor2)(dJointID, dVector3 result);
//void            (ODE_API *dJointGetHinge2Axis1)(dJointID, dVector3 result);
//void            (ODE_API *dJointGetHinge2Axis2)(dJointID, dVector3 result);
//dReal           (ODE_API *dJointGetHinge2Param)(dJointID, int parameter);
//dReal           (ODE_API *dJointGetHinge2Angle1)(dJointID);
//dReal           (ODE_API *dJointGetHinge2Angle1Rate)(dJointID);
//dReal           (ODE_API *dJointGetHinge2Angle2Rate)(dJointID);
//void            (ODE_API *dJointGetUniversalAnchor)(dJointID, dVector3 result);
//void            (ODE_API *dJointGetUniversalAnchor2)(dJointID, dVector3 result);
//void            (ODE_API *dJointGetUniversalAxis1)(dJointID, dVector3 result);
//void            (ODE_API *dJointGetUniversalAxis2)(dJointID, dVector3 result);
//dReal           (ODE_API *dJointGetUniversalParam)(dJointID, int parameter);
//void            (ODE_API *dJointGetUniversalAngles)(dJointID, dReal *angle1, dReal *angle2);
//dReal           (ODE_API *dJointGetUniversalAngle1)(dJointID);
//dReal           (ODE_API *dJointGetUniversalAngle2)(dJointID);
//dReal           (ODE_API *dJointGetUniversalAngle1Rate)(dJointID);
//dReal           (ODE_API *dJointGetUniversalAngle2Rate)(dJointID);
//void            (ODE_API *dJointGetPRAnchor)(dJointID, dVector3 result);
//dReal           (ODE_API *dJointGetPRPosition)(dJointID);
//dReal           (ODE_API *dJointGetPRPositionRate)(dJointID);
//dReal           (ODE_API *dJointGetPRAngle)(dJointID);
//dReal           (ODE_API *dJointGetPRAngleRate)(dJointID);
//void            (ODE_API *dJointGetPRAxis1)(dJointID, dVector3 result);
//void            (ODE_API *dJointGetPRAxis2)(dJointID, dVector3 result);
//dReal           (ODE_API *dJointGetPRParam)(dJointID, int parameter);
//void            (ODE_API *dJointGetPUAnchor)(dJointID, dVector3 result);
//dReal           (ODE_API *dJointGetPUPosition)(dJointID);
//dReal           (ODE_API *dJointGetPUPositionRate)(dJointID);
//void            (ODE_API *dJointGetPUAxis1)(dJointID, dVector3 result);
//void            (ODE_API *dJointGetPUAxis2)(dJointID, dVector3 result);
//void            (ODE_API *dJointGetPUAxis3)(dJointID, dVector3 result);
//void            (ODE_API *dJointGetPUAxisP)(dJointID id, dVector3 result);
//void            (ODE_API *dJointGetPUAngles)(dJointID, dReal *angle1, dReal *angle2);
//dReal           (ODE_API *dJointGetPUAngle1)(dJointID);
//dReal           (ODE_API *dJointGetPUAngle1Rate)(dJointID);
//dReal           (ODE_API *dJointGetPUAngle2)(dJointID);
//dReal           (ODE_API *dJointGetPUAngle2Rate)(dJointID);
//dReal           (ODE_API *dJointGetPUParam)(dJointID, int parameter);
//dReal           (ODE_API *dJointGetPistonPosition)(dJointID);
//dReal           (ODE_API *dJointGetPistonPositionRate)(dJointID);
//dReal           (ODE_API *dJointGetPistonAngle)(dJointID);
//dReal           (ODE_API *dJointGetPistonAngleRate)(dJointID);
//void            (ODE_API *dJointGetPistonAnchor)(dJointID, dVector3 result);
//void            (ODE_API *dJointGetPistonAnchor2)(dJointID, dVector3 result);
//void            (ODE_API *dJointGetPistonAxis)(dJointID, dVector3 result);
//dReal           (ODE_API *dJointGetPistonParam)(dJointID, int parameter);
//int             (ODE_API *dJointGetAMotorNumAxes)(dJointID);
//void            (ODE_API *dJointGetAMotorAxis)(dJointID, int anum, dVector3 result);
//int             (ODE_API *dJointGetAMotorAxisRel)(dJointID, int anum);
//dReal           (ODE_API *dJointGetAMotorAngle)(dJointID, int anum);
//dReal           (ODE_API *dJointGetAMotorAngleRate)(dJointID, int anum);
//dReal           (ODE_API *dJointGetAMotorParam)(dJointID, int parameter);
//int             (ODE_API *dJointGetAMotorMode)(dJointID);
//int             (ODE_API *dJointGetLMotorNumAxes)(dJointID);
//void            (ODE_API *dJointGetLMotorAxis)(dJointID, int anum, dVector3 result);
//dReal           (ODE_API *dJointGetLMotorParam)(dJointID, int parameter);
//dReal           (ODE_API *dJointGetFixedParam)(dJointID, int parameter);
//dJointID        (ODE_API *dConnectingJoint)(dBodyID, dBodyID);
//int             (ODE_API *dConnectingJointList)(dBodyID, dBodyID, dJointID*);
int             (ODE_API *dAreConnected)(dBodyID, dBodyID);
int             (ODE_API *dAreConnectedExcluding)(dBodyID body1, dBodyID body2, int joint_type);
//
dSpaceID        (ODE_API *dSimpleSpaceCreate)(dSpaceID space);
dSpaceID        (ODE_API *dHashSpaceCreate)(dSpaceID space);
dSpaceID        (ODE_API *dQuadTreeSpaceCreate)(dSpaceID space, const dVector3 Center, const dVector3 Extents, int Depth);
//dSpaceID        (ODE_API *dSweepAndPruneSpaceCreate)( dSpaceID space, int axisorder );
void            (ODE_API *dSpaceDestroy)(dSpaceID);
//void            (ODE_API *dHashSpaceSetLevels)(dSpaceID space, int minlevel, int maxlevel);
//void            (ODE_API *dHashSpaceGetLevels)(dSpaceID space, int *minlevel, int *maxlevel);
//void            (ODE_API *dSpaceSetCleanup)(dSpaceID space, int mode);
//int             (ODE_API *dSpaceGetCleanup)(dSpaceID space);
//void            (ODE_API *dSpaceSetSublevel)(dSpaceID space, int sublevel);
//int             (ODE_API *dSpaceGetSublevel)(dSpaceID space);
//void            (ODE_API *dSpaceSetManualCleanup)(dSpaceID space, int mode);
//int             (ODE_API *dSpaceGetManualCleanup)(dSpaceID space);
//void            (ODE_API *dSpaceAdd)(dSpaceID, dGeomID);
//void            (ODE_API *dSpaceRemove)(dSpaceID, dGeomID);
//int             (ODE_API *dSpaceQuery)(dSpaceID, dGeomID);
//void            (ODE_API *dSpaceClean)(dSpaceID);
//int             (ODE_API *dSpaceGetNumGeoms)(dSpaceID);
//dGeomID         (ODE_API *dSpaceGetGeom)(dSpaceID, int i);
//int             (ODE_API *dSpaceGetClass)(dSpaceID space);
//
void            (ODE_API *dGeomDestroy)(dGeomID geom);
void            (ODE_API *dGeomSetData)(dGeomID geom, void* data);
void *          (ODE_API *dGeomGetData)(dGeomID geom);
void            (ODE_API *dGeomSetBody)(dGeomID geom, dBodyID body);
dBodyID         (ODE_API *dGeomGetBody)(dGeomID geom);
void            (ODE_API *dGeomSetPosition)(dGeomID geom, dReal x, dReal y, dReal z);
void            (ODE_API *dGeomSetRotation)(dGeomID geom, const dMatrix3 R);
//void            (ODE_API *dGeomSetQuaternion)(dGeomID geom, const dQuaternion Q);
//const dReal *   (ODE_API *dGeomGetPosition)(dGeomID geom);
//void            (ODE_API *dGeomCopyPosition)(dGeomID geom, dVector3 pos);
//const dReal *   (ODE_API *dGeomGetRotation)(dGeomID geom);
//void            (ODE_API *dGeomCopyRotation)(dGeomID geom, dMatrix3 R);
//void            (ODE_API *dGeomGetQuaternion)(dGeomID geom, dQuaternion result);
//void            (ODE_API *dGeomGetAABB)(dGeomID geom, dReal aabb[6]);
int             (ODE_API *dGeomIsSpace)(dGeomID geom);
//dSpaceID        (ODE_API *dGeomGetSpace)(dGeomID);
//int             (ODE_API *dGeomGetClass)(dGeomID geom);
//void            (ODE_API *dGeomSetCategoryBits)(dGeomID geom, unsigned long bits);
//void            (ODE_API *dGeomSetCollideBits)(dGeomID geom, unsigned long bits);
//unsigned long   (ODE_API *dGeomGetCategoryBits)(dGeomID);
//unsigned long   (ODE_API *dGeomGetCollideBits)(dGeomID);
//void            (ODE_API *dGeomEnable)(dGeomID geom);
//void            (ODE_API *dGeomDisable)(dGeomID geom);
//int             (ODE_API *dGeomIsEnabled)(dGeomID geom);
//void            (ODE_API *dGeomSetOffsetPosition)(dGeomID geom, dReal x, dReal y, dReal z);
//void            (ODE_API *dGeomSetOffsetRotation)(dGeomID geom, const dMatrix3 R);
//void            (ODE_API *dGeomSetOffsetQuaternion)(dGeomID geom, const dQuaternion Q);
//void            (ODE_API *dGeomSetOffsetWorldPosition)(dGeomID geom, dReal x, dReal y, dReal z);
//void            (ODE_API *dGeomSetOffsetWorldRotation)(dGeomID geom, const dMatrix3 R);
//void            (ODE_API *dGeomSetOffsetWorldQuaternion)(dGeomID geom, const dQuaternion);
//void            (ODE_API *dGeomClearOffset)(dGeomID geom);
//int             (ODE_API *dGeomIsOffset)(dGeomID geom);
//const dReal *   (ODE_API *dGeomGetOffsetPosition)(dGeomID geom);
//void            (ODE_API *dGeomCopyOffsetPosition)(dGeomID geom, dVector3 pos);
//const dReal *   (ODE_API *dGeomGetOffsetRotation)(dGeomID geom);
//void            (ODE_API *dGeomCopyOffsetRotation)(dGeomID geom, dMatrix3 R);
//void            (ODE_API *dGeomGetOffsetQuaternion)(dGeomID geom, dQuaternion result);
int             (ODE_API *dCollide)(dGeomID o1, dGeomID o2, int flags, dContactGeom *contact, int skip);
//
void            (ODE_API *dSpaceCollide)(dSpaceID space, void *data, dNearCallback *callback);
void            (ODE_API *dSpaceCollide2)(dGeomID space1, dGeomID space2, void *data, dNearCallback *callback);
//
dGeomID         (ODE_API *dCreateSphere)(dSpaceID space, dReal radius);
//void            (ODE_API *dGeomSphereSetRadius)(dGeomID sphere, dReal radius);
//dReal           (ODE_API *dGeomSphereGetRadius)(dGeomID sphere);
//dReal           (ODE_API *dGeomSpherePointDepth)(dGeomID sphere, dReal x, dReal y, dReal z);
//
dGeomID         (ODE_API *dCreateConvex)(dSpaceID space, dReal *_planes, unsigned int _planecount, dReal *_points, unsigned int _pointcount,unsigned int *_polygons);
//void            (ODE_API *dGeomSetConvex)(dGeomID g, dReal *_planes, unsigned int _count, dReal *_points, unsigned int _pointcount,unsigned int *_polygons);
//
dGeomID         (ODE_API *dCreateBox)(dSpaceID space, dReal lx, dReal ly, dReal lz);
//void            (ODE_API *dGeomBoxSetLengths)(dGeomID box, dReal lx, dReal ly, dReal lz);
//void            (ODE_API *dGeomBoxGetLengths)(dGeomID box, dVector3 result);
//dReal           (ODE_API *dGeomBoxPointDepth)(dGeomID box, dReal x, dReal y, dReal z);
//dReal           (ODE_API *dGeomBoxPointDepth)(dGeomID box, dReal x, dReal y, dReal z);
//
//dGeomID         (ODE_API *dCreatePlane)(dSpaceID space, dReal a, dReal b, dReal c, dReal d);
//void            (ODE_API *dGeomPlaneSetParams)(dGeomID plane, dReal a, dReal b, dReal c, dReal d);
//void            (ODE_API *dGeomPlaneGetParams)(dGeomID plane, dVector4 result);
//dReal           (ODE_API *dGeomPlanePointDepth)(dGeomID plane, dReal x, dReal y, dReal z);
//
dGeomID         (ODE_API *dCreateCapsule)(dSpaceID space, dReal radius, dReal length);
//void            (ODE_API *dGeomCapsuleSetParams)(dGeomID ccylinder, dReal radius, dReal length);
//void            (ODE_API *dGeomCapsuleGetParams)(dGeomID ccylinder, dReal *radius, dReal *length);
//dReal           (ODE_API *dGeomCapsulePointDepth)(dGeomID ccylinder, dReal x, dReal y, dReal z);
//
dGeomID         (ODE_API *dCreateCylinder)(dSpaceID space, dReal radius, dReal length);
//void            (ODE_API *dGeomCylinderSetParams)(dGeomID cylinder, dReal radius, dReal length);
//void            (ODE_API *dGeomCylinderGetParams)(dGeomID cylinder, dReal *radius, dReal *length);
//
//dGeomID         (ODE_API *dCreateRay)(dSpaceID space, dReal length);
//void            (ODE_API *dGeomRaySetLength)(dGeomID ray, dReal length);
//dReal           (ODE_API *dGeomRayGetLength)(dGeomID ray);
//void            (ODE_API *dGeomRaySet)(dGeomID ray, dReal px, dReal py, dReal pz, dReal dx, dReal dy, dReal dz);
//void            (ODE_API *dGeomRayGet)(dGeomID ray, dVector3 start, dVector3 dir);
//
dGeomID         (ODE_API *dCreateGeomTransform)(dSpaceID space);
void            (ODE_API *dGeomTransformSetGeom)(dGeomID g, dGeomID obj);
//dGeomID         (ODE_API *dGeomTransformGetGeom)(dGeomID g);
void            (ODE_API *dGeomTransformSetCleanup)(dGeomID g, int mode);
//int             (ODE_API *dGeomTransformGetCleanup)(dGeomID g);
//void            (ODE_API *dGeomTransformSetInfo)(dGeomID g, int mode);
//int             (ODE_API *dGeomTransformGetInfo)(dGeomID g);

enum { TRIMESH_FACE_NORMALS };
typedef int dTriCallback(dGeomID TriMesh, dGeomID RefObject, int TriangleIndex);
typedef void dTriArrayCallback(dGeomID TriMesh, dGeomID RefObject, const int* TriIndices, int TriCount);
typedef int dTriRayCallback(dGeomID TriMesh, dGeomID Ray, int TriangleIndex, dReal u, dReal v);
typedef int dTriTriMergeCallback(dGeomID TriMesh, int FirstTriangleIndex, int SecondTriangleIndex);

dTriMeshDataID  (ODE_API *dGeomTriMeshDataCreate)(void);
void            (ODE_API *dGeomTriMeshDataDestroy)(dTriMeshDataID g);
//void            (ODE_API *dGeomTriMeshDataSet)(dTriMeshDataID g, int data_id, void* in_data);
//void*           (ODE_API *dGeomTriMeshDataGet)(dTriMeshDataID g, int data_id);
//void            (*dGeomTriMeshSetLastTransform)( (ODE_API *dGeomID g, dMatrix4 last_trans );
//dReal*          (*dGeomTriMeshGetLastTransform)( (ODE_API *dGeomID g );
void            (ODE_API *dGeomTriMeshDataBuildSingle)(dTriMeshDataID g, const void* Vertices, int VertexStride, int VertexCount,  const void* Indices, int IndexCount, int TriStride);
//void            (ODE_API *dGeomTriMeshDataBuildSingle1)(dTriMeshDataID g, const void* Vertices, int VertexStride, int VertexCount,  const void* Indices, int IndexCount, int TriStride, const void* Normals);
//void            (ODE_API *dGeomTriMeshDataBuildDouble)(dTriMeshDataID g,  const void* Vertices,  int VertexStride, int VertexCount,  const void* Indices, int IndexCount, int TriStride);
//void            (ODE_API *dGeomTriMeshDataBuildDouble1)(dTriMeshDataID g,  const void* Vertices,  int VertexStride, int VertexCount,  const void* Indices, int IndexCount, int TriStride, const void* Normals);
//void            (ODE_API *dGeomTriMeshDataBuildSimple)(dTriMeshDataID g, const dReal* Vertices, int VertexCount, const dTriIndex* Indices, int IndexCount);
//void            (ODE_API *dGeomTriMeshDataBuildSimple1)(dTriMeshDataID g, const dReal* Vertices, int VertexCount, const dTriIndex* Indices, int IndexCount, const int* Normals);
//void            (ODE_API *dGeomTriMeshDataPreprocess)(dTriMeshDataID g);
//void            (ODE_API *dGeomTriMeshDataGetBuffer)(dTriMeshDataID g, unsigned char** buf, int* bufLen);
//void            (ODE_API *dGeomTriMeshDataSetBuffer)(dTriMeshDataID g, unsigned char* buf);
//void            (ODE_API *dGeomTriMeshSetCallback)(dGeomID g, dTriCallback* Callback);
//dTriCallback*   (ODE_API *dGeomTriMeshGetCallback)(dGeomID g);
//void            (ODE_API *dGeomTriMeshSetArrayCallback)(dGeomID g, dTriArrayCallback* ArrayCallback);
//dTriArrayCallback* (ODE_API *dGeomTriMeshGetArrayCallback)(dGeomID g);
//void            (ODE_API *dGeomTriMeshSetRayCallback)(dGeomID g, dTriRayCallback* Callback);
//dTriRayCallback* (ODE_API *dGeomTriMeshGetRayCallback)(dGeomID g);
//void            (ODE_API *dGeomTriMeshSetTriMergeCallback)(dGeomID g, dTriTriMergeCallback* Callback);
//dTriTriMergeCallback* (ODE_API *dGeomTriMeshGetTriMergeCallback)(dGeomID g);
dGeomID         (ODE_API *dCreateTriMesh)(dSpaceID space, dTriMeshDataID Data, dTriCallback* Callback, dTriArrayCallback* ArrayCallback, dTriRayCallback* RayCallback);
//void            (ODE_API *dGeomTriMeshSetData)(dGeomID g, dTriMeshDataID Data);
//dTriMeshDataID  (ODE_API *dGeomTriMeshGetData)(dGeomID g);
//void            (ODE_API *dGeomTriMeshEnableTC)(dGeomID g, int geomClass, int enable);
//int             (ODE_API *dGeomTriMeshIsTCEnabled)(dGeomID g, int geomClass);
//void            (ODE_API *dGeomTriMeshClearTCCache)(dGeomID g);
//dTriMeshDataID  (ODE_API *dGeomTriMeshGetTriMeshDataID)(dGeomID g);
//void            (ODE_API *dGeomTriMeshGetTriangle)(dGeomID g, int Index, dVector3* v0, dVector3* v1, dVector3* v2);
//void            (ODE_API *dGeomTriMeshGetPoint)(dGeomID g, int Index, dReal u, dReal v, dVector3 Out);
//int             (ODE_API *dGeomTriMeshGetTriangleCount )(dGeomID g);
//void            (ODE_API *dGeomTriMeshDataUpdate)(dTriMeshDataID g);

static dllfunction_t odefuncs[] =
{
	{"dGetConfiguration",							(void **) &dGetConfiguration},
	{"dCheckConfiguration",							(void **) &dCheckConfiguration},
	{"dInitODE",									(void **) &dInitODE},
//	{"dInitODE2",									(void **) &dInitODE2},
//	{"dAllocateODEDataForThread",					(void **) &dAllocateODEDataForThread},
//	{"dCleanupODEAllDataForThread",					(void **) &dCleanupODEAllDataForThread},
	{"dCloseODE",									(void **) &dCloseODE},
//	{"dMassCheck",									(void **) &dMassCheck},
//	{"dMassSetZero",								(void **) &dMassSetZero},
//	{"dMassSetParameters",							(void **) &dMassSetParameters},
//	{"dMassSetSphere",								(void **) &dMassSetSphere},
	{"dMassSetSphereTotal",							(void **) &dMassSetSphereTotal},
//	{"dMassSetCapsule",								(void **) &dMassSetCapsule},
	{"dMassSetCapsuleTotal",						(void **) &dMassSetCapsuleTotal},
//	{"dMassSetCylinder",							(void **) &dMassSetCylinder},
	{"dMassSetCylinderTotal",						(void **) &dMassSetCylinderTotal},
//	{"dMassSetBox",									(void **) &dMassSetBox},
	{"dMassSetBoxTotal",							(void **) &dMassSetBoxTotal},
//	{"dMassSetTrimesh",								(void **) &dMassSetTrimesh},
//	{"dMassSetTrimeshTotal",						(void **) &dMassSetTrimeshTotal},
//	{"dMassAdjust",									(void **) &dMassAdjust},
//	{"dMassTranslate",								(void **) &dMassTranslate},
//	{"dMassRotate",									(void **) &dMassRotate},
//	{"dMassAdd",									(void **) &dMassAdd},

	{"dWorldCreate",								(void **) &dWorldCreate},
	{"dWorldDestroy",								(void **) &dWorldDestroy},
	{"dWorldSetGravity",							(void **) &dWorldSetGravity},
	{"dWorldGetGravity",							(void **) &dWorldGetGravity},
	{"dWorldSetERP",								(void **) &dWorldSetERP},
//	{"dWorldGetERP",								(void **) &dWorldGetERP},
	{"dWorldSetCFM",								(void **) &dWorldSetCFM},
//	{"dWorldGetCFM",								(void **) &dWorldGetCFM},
//	{"dWorldStep",									(void **) &dWorldStep},
//	{"dWorldImpulseToForce",						(void **) &dWorldImpulseToForce},
	{"dWorldQuickStep",								(void **) &dWorldQuickStep},
	{"dWorldSetQuickStepNumIterations",				(void **) &dWorldSetQuickStepNumIterations},
//	{"dWorldGetQuickStepNumIterations",				(void **) &dWorldGetQuickStepNumIterations},
//	{"dWorldSetQuickStepW",							(void **) &dWorldSetQuickStepW},
//	{"dWorldGetQuickStepW",							(void **) &dWorldGetQuickStepW},
//	{"dWorldSetContactMaxCorrectingVel",			(void **) &dWorldSetContactMaxCorrectingVel},
//	{"dWorldGetContactMaxCorrectingVel",			(void **) &dWorldGetContactMaxCorrectingVel},
	{"dWorldSetContactSurfaceLayer",				(void **) &dWorldSetContactSurfaceLayer},
//	{"dWorldGetContactSurfaceLayer",				(void **) &dWorldGetContactSurfaceLayer},
//	{"dWorldStepFast1",								(void **) &dWorldStepFast1},
//	{"dWorldSetAutoEnableDepthSF1",					(void **) &dWorldSetAutoEnableDepthSF1},
//	{"dWorldGetAutoEnableDepthSF1",					(void **) &dWorldGetAutoEnableDepthSF1},
//	{"dWorldGetAutoDisableLinearThreshold",			(void **) &dWorldGetAutoDisableLinearThreshold},
	{"dWorldSetAutoDisableLinearThreshold",			(void **) &dWorldSetAutoDisableLinearThreshold},
//	{"dWorldGetAutoDisableAngularThreshold",		(void **) &dWorldGetAutoDisableAngularThreshold},
	{"dWorldSetAutoDisableAngularThreshold",		(void **) &dWorldSetAutoDisableAngularThreshold},
//	{"dWorldGetAutoDisableLinearAverageThreshold",	(void **) &dWorldGetAutoDisableLinearAverageThreshold},
//	{"dWorldSetAutoDisableLinearAverageThreshold",	(void **) &dWorldSetAutoDisableLinearAverageThreshold},
//	{"dWorldGetAutoDisableAngularAverageThreshold",	(void **) &dWorldGetAutoDisableAngularAverageThreshold},
//	{"dWorldSetAutoDisableAngularAverageThreshold",	(void **) &dWorldSetAutoDisableAngularAverageThreshold},
//	{"dWorldGetAutoDisableAverageSamplesCount",		(void **) &dWorldGetAutoDisableAverageSamplesCount},
	{"dWorldSetAutoDisableAverageSamplesCount",		(void **) &dWorldSetAutoDisableAverageSamplesCount},
//	{"dWorldGetAutoDisableSteps",					(void **) &dWorldGetAutoDisableSteps},
	{"dWorldSetAutoDisableSteps",					(void **) &dWorldSetAutoDisableSteps},
//	{"dWorldGetAutoDisableTime",					(void **) &dWorldGetAutoDisableTime},
	{"dWorldSetAutoDisableTime",					(void **) &dWorldSetAutoDisableTime},
//	{"dWorldGetAutoDisableFlag",					(void **) &dWorldGetAutoDisableFlag},
	{"dWorldSetAutoDisableFlag",					(void **) &dWorldSetAutoDisableFlag},
//	{"dWorldGetLinearDampingThreshold",				(void **) &dWorldGetLinearDampingThreshold},
	{"dWorldSetLinearDampingThreshold",				(void **) &dWorldSetLinearDampingThreshold},
//	{"dWorldGetAngularDampingThreshold",			(void **) &dWorldGetAngularDampingThreshold},
	{"dWorldSetAngularDampingThreshold",			(void **) &dWorldSetAngularDampingThreshold},
//	{"dWorldGetLinearDamping",						(void **) &dWorldGetLinearDamping},
	{"dWorldSetLinearDamping",						(void **) &dWorldSetLinearDamping},
//	{"dWorldGetAngularDamping",						(void **) &dWorldGetAngularDamping},
	{"dWorldSetAngularDamping",						(void **) &dWorldSetAngularDamping},
//	{"dWorldSetDamping",							(void **) &dWorldSetDamping},
//	{"dWorldGetMaxAngularSpeed",					(void **) &dWorldGetMaxAngularSpeed},
//	{"dWorldSetMaxAngularSpeed",					(void **) &dWorldSetMaxAngularSpeed},
//	{"dBodyGetAutoDisableLinearThreshold",			(void **) &dBodyGetAutoDisableLinearThreshold},
//	{"dBodySetAutoDisableLinearThreshold",			(void **) &dBodySetAutoDisableLinearThreshold},
//	{"dBodyGetAutoDisableAngularThreshold",			(void **) &dBodyGetAutoDisableAngularThreshold},
//	{"dBodySetAutoDisableAngularThreshold",			(void **) &dBodySetAutoDisableAngularThreshold},
//	{"dBodyGetAutoDisableAverageSamplesCount",		(void **) &dBodyGetAutoDisableAverageSamplesCount},
//	{"dBodySetAutoDisableAverageSamplesCount",		(void **) &dBodySetAutoDisableAverageSamplesCount},
//	{"dBodyGetAutoDisableSteps",					(void **) &dBodyGetAutoDisableSteps},
//	{"dBodySetAutoDisableSteps",					(void **) &dBodySetAutoDisableSteps},
//	{"dBodyGetAutoDisableTime",						(void **) &dBodyGetAutoDisableTime},
//	{"dBodySetAutoDisableTime",						(void **) &dBodySetAutoDisableTime},
//	{"dBodyGetAutoDisableFlag",						(void **) &dBodyGetAutoDisableFlag},
//	{"dBodySetAutoDisableFlag",						(void **) &dBodySetAutoDisableFlag},
//	{"dBodySetAutoDisableDefaults",					(void **) &dBodySetAutoDisableDefaults},
//	{"dBodyGetWorld",								(void **) &dBodyGetWorld},
	{"dBodyCreate",									(void **) &dBodyCreate},
	{"dBodyDestroy",								(void **) &dBodyDestroy},
	{"dBodySetData",								(void **) &dBodySetData},
	{"dBodyGetData",								(void **) &dBodyGetData},
	{"dBodySetPosition",							(void **) &dBodySetPosition},
	{"dBodySetRotation",							(void **) &dBodySetRotation},
//	{"dBodySetQuaternion",							(void **) &dBodySetQuaternion},
	{"dBodySetLinearVel",							(void **) &dBodySetLinearVel},
	{"dBodySetAngularVel",							(void **) &dBodySetAngularVel},
	{"dBodyGetPosition",							(void **) &dBodyGetPosition},
//	{"dBodyCopyPosition",							(void **) &dBodyCopyPosition},
	{"dBodyGetRotation",							(void **) &dBodyGetRotation},
//	{"dBodyCopyRotation",							(void **) &dBodyCopyRotation},
//	{"dBodyGetQuaternion",							(void **) &dBodyGetQuaternion},
//	{"dBodyCopyQuaternion",							(void **) &dBodyCopyQuaternion},
	{"dBodyGetLinearVel",							(void **) &dBodyGetLinearVel},
	{"dBodyGetAngularVel",							(void **) &dBodyGetAngularVel},
	{"dBodySetMass",								(void **) &dBodySetMass},
//	{"dBodyGetMass",								(void **) &dBodyGetMass},
	{"dBodyAddForce",								(void **) &dBodyAddForce},
	{"dBodyAddTorque",								(void **) &dBodyAddTorque},
//	{"dBodyAddRelForce",							(void **) &dBodyAddRelForce},
//	{"dBodyAddRelTorque",							(void **) &dBodyAddRelTorque},
	{"dBodyAddForceAtPos",							(void **) &dBodyAddForceAtPos},
//	{"dBodyAddForceAtRelPos",						(void **) &dBodyAddForceAtRelPos},
//	{"dBodyAddRelForceAtPos",						(void **) &dBodyAddRelForceAtPos},
//	{"dBodyAddRelForceAtRelPos",					(void **) &dBodyAddRelForceAtRelPos},
//	{"dBodyGetForce",								(void **) &dBodyGetForce},
//	{"dBodyGetTorque",								(void **) &dBodyGetTorque},
//	{"dBodySetForce",								(void **) &dBodySetForce},
//	{"dBodySetTorque",								(void **) &dBodySetTorque},
//	{"dBodyGetRelPointPos",							(void **) &dBodyGetRelPointPos},
//	{"dBodyGetRelPointVel",							(void **) &dBodyGetRelPointVel},
//	{"dBodyGetPointVel",							(void **) &dBodyGetPointVel},
//	{"dBodyGetPosRelPoint",							(void **) &dBodyGetPosRelPoint},
//	{"dBodyVectorToWorld",							(void **) &dBodyVectorToWorld},
//	{"dBodyVectorFromWorld",						(void **) &dBodyVectorFromWorld},
//	{"dBodySetFiniteRotationMode",					(void **) &dBodySetFiniteRotationMode},
//	{"dBodySetFiniteRotationAxis",					(void **) &dBodySetFiniteRotationAxis},
//	{"dBodyGetFiniteRotationMode",					(void **) &dBodyGetFiniteRotationMode},
//	{"dBodyGetFiniteRotationAxis",					(void **) &dBodyGetFiniteRotationAxis},
	{"dBodyGetNumJoints",							(void **) &dBodyGetNumJoints},
	{"dBodyGetJoint",								(void **) &dBodyGetJoint},
//	{"dBodySetDynamic",								(void **) &dBodySetDynamic},
//	{"dBodySetKinematic",							(void **) &dBodySetKinematic},
//	{"dBodyIsKinematic",							(void **) &dBodyIsKinematic},
	{"dBodyEnable",									(void **) &dBodyEnable},
	{"dBodyDisable",								(void **) &dBodyDisable},
	{"dBodyIsEnabled",								(void **) &dBodyIsEnabled},
	{"dBodySetGravityMode",							(void **) &dBodySetGravityMode},
	{"dBodyGetGravityMode",							(void **) &dBodyGetGravityMode},
//	{"dBodySetMovedCallback",						(void **) &dBodySetMovedCallback},
//	{"dBodyGetFirstGeom",							(void **) &dBodyGetFirstGeom},
//	{"dBodyGetNextGeom",							(void **) &dBodyGetNextGeom},
//	{"dBodySetDampingDefaults",						(void **) &dBodySetDampingDefaults},
//	{"dBodyGetLinearDamping",						(void **) &dBodyGetLinearDamping},
//	{"dBodySetLinearDamping",						(void **) &dBodySetLinearDamping},
//	{"dBodyGetAngularDamping",						(void **) &dBodyGetAngularDamping},
//	{"dBodySetAngularDamping",						(void **) &dBodySetAngularDamping},
//	{"dBodySetDamping",								(void **) &dBodySetDamping},
//	{"dBodyGetLinearDampingThreshold",				(void **) &dBodyGetLinearDampingThreshold},
//	{"dBodySetLinearDampingThreshold",				(void **) &dBodySetLinearDampingThreshold},
//	{"dBodyGetAngularDampingThreshold",				(void **) &dBodyGetAngularDampingThreshold},
//	{"dBodySetAngularDampingThreshold",				(void **) &dBodySetAngularDampingThreshold},
//	{"dBodyGetMaxAngularSpeed",						(void **) &dBodyGetMaxAngularSpeed},
//	{"dBodySetMaxAngularSpeed",						(void **) &dBodySetMaxAngularSpeed},
//	{"dBodyGetGyroscopicMode",						(void **) &dBodyGetGyroscopicMode},
//	{"dBodySetGyroscopicMode",						(void **) &dBodySetGyroscopicMode},
	{"dJointCreateBall",							(void **) &dJointCreateBall},
	{"dJointCreateHinge",							(void **) &dJointCreateHinge},
	{"dJointCreateSlider",							(void **) &dJointCreateSlider},
	{"dJointCreateContact",							(void **) &dJointCreateContact},
	{"dJointCreateHinge2",							(void **) &dJointCreateHinge2},
	{"dJointCreateUniversal",						(void **) &dJointCreateUniversal},
//	{"dJointCreatePR",								(void **) &dJointCreatePR},
//	{"dJointCreatePU",								(void **) &dJointCreatePU},
//	{"dJointCreatePiston",							(void **) &dJointCreatePiston},
	{"dJointCreateFixed",							(void **) &dJointCreateFixed},
//	{"dJointCreateNull",							(void **) &dJointCreateNull},
//	{"dJointCreateAMotor",							(void **) &dJointCreateAMotor},
//	{"dJointCreateLMotor",							(void **) &dJointCreateLMotor},
//	{"dJointCreatePlane2D",							(void **) &dJointCreatePlane2D},
	{"dJointDestroy",								(void **) &dJointDestroy},
	{"dJointGroupCreate",							(void **) &dJointGroupCreate},
	{"dJointGroupDestroy",							(void **) &dJointGroupDestroy},
	{"dJointGroupEmpty",							(void **) &dJointGroupEmpty},
//	{"dJointGetNumBodies",							(void **) &dJointGetNumBodies},
	{"dJointAttach",								(void **) &dJointAttach},
//	{"dJointEnable",								(void **) &dJointEnable},
//	{"dJointDisable",								(void **) &dJointDisable},
//	{"dJointIsEnabled",								(void **) &dJointIsEnabled},
	{"dJointSetData",								(void **) &dJointSetData},
	{"dJointGetData",								(void **) &dJointGetData},
//	{"dJointGetType",								(void **) &dJointGetType},
	{"dJointGetBody",								(void **) &dJointGetBody},
//	{"dJointSetFeedback",							(void **) &dJointSetFeedback},
//	{"dJointGetFeedback",							(void **) &dJointGetFeedback},
	{"dJointSetBallAnchor",							(void **) &dJointSetBallAnchor},
//	{"dJointSetBallAnchor2",						(void **) &dJointSetBallAnchor2},
	{"dJointSetBallParam",							(void **) &dJointSetBallParam},
	{"dJointSetHingeAnchor",						(void **) &dJointSetHingeAnchor},
//	{"dJointSetHingeAnchorDelta",					(void **) &dJointSetHingeAnchorDelta},
	{"dJointSetHingeAxis",							(void **) &dJointSetHingeAxis},
//	{"dJointSetHingeAxisOffset",					(void **) &dJointSetHingeAxisOffset},
	{"dJointSetHingeParam",							(void **) &dJointSetHingeParam},
//	{"dJointAddHingeTorque",						(void **) &dJointAddHingeTorque},
	{"dJointSetSliderAxis",							(void **) &dJointSetSliderAxis},
//	{"dJointSetSliderAxisDelta",					(void **) &dJointSetSliderAxisDelta},
	{"dJointSetSliderParam",						(void **) &dJointSetSliderParam},
//	{"dJointAddSliderForce",						(void **) &dJointAddSliderForce},
	{"dJointSetHinge2Anchor",						(void **) &dJointSetHinge2Anchor},
	{"dJointSetHinge2Axis1",						(void **) &dJointSetHinge2Axis1},
	{"dJointSetHinge2Axis2",						(void **) &dJointSetHinge2Axis2},
	{"dJointSetHinge2Param",						(void **) &dJointSetHinge2Param},
//	{"dJointAddHinge2Torques",						(void **) &dJointAddHinge2Torques},
	{"dJointSetUniversalAnchor",					(void **) &dJointSetUniversalAnchor},
	{"dJointSetUniversalAxis1",						(void **) &dJointSetUniversalAxis1},
//	{"dJointSetUniversalAxis1Offset",				(void **) &dJointSetUniversalAxis1Offset},
	{"dJointSetUniversalAxis2",						(void **) &dJointSetUniversalAxis2},
//	{"dJointSetUniversalAxis2Offset",				(void **) &dJointSetUniversalAxis2Offset},
	{"dJointSetUniversalParam",						(void **) &dJointSetUniversalParam},
//	{"dJointAddUniversalTorques",					(void **) &dJointAddUniversalTorques},
//	{"dJointSetPRAnchor",							(void **) &dJointSetPRAnchor},
//	{"dJointSetPRAxis1",							(void **) &dJointSetPRAxis1},
//	{"dJointSetPRAxis2",							(void **) &dJointSetPRAxis2},
//	{"dJointSetPRParam",							(void **) &dJointSetPRParam},
//	{"dJointAddPRTorque",							(void **) &dJointAddPRTorque},
//	{"dJointSetPUAnchor",							(void **) &dJointSetPUAnchor},
//	{"dJointSetPUAnchorOffset",						(void **) &dJointSetPUAnchorOffset},
//	{"dJointSetPUAxis1",							(void **) &dJointSetPUAxis1},
//	{"dJointSetPUAxis2",							(void **) &dJointSetPUAxis2},
//	{"dJointSetPUAxis3",							(void **) &dJointSetPUAxis3},
//	{"dJointSetPUAxisP",							(void **) &dJointSetPUAxisP},
//	{"dJointSetPUParam",							(void **) &dJointSetPUParam},
//	{"dJointAddPUTorque",							(void **) &dJointAddPUTorque},
//	{"dJointSetPistonAnchor",						(void **) &dJointSetPistonAnchor},
//	{"dJointSetPistonAnchorOffset",					(void **) &dJointSetPistonAnchorOffset},
//	{"dJointSetPistonParam",						(void **) &dJointSetPistonParam},
//	{"dJointAddPistonForce",						(void **) &dJointAddPistonForce},
//	{"dJointSetFixed",								(void **) &dJointSetFixed},
//	{"dJointSetFixedParam",							(void **) &dJointSetFixedParam},
//	{"dJointSetAMotorNumAxes",						(void **) &dJointSetAMotorNumAxes},
//	{"dJointSetAMotorAxis",							(void **) &dJointSetAMotorAxis},
//	{"dJointSetAMotorAngle",						(void **) &dJointSetAMotorAngle},
//	{"dJointSetAMotorParam",						(void **) &dJointSetAMotorParam},
//	{"dJointSetAMotorMode",							(void **) &dJointSetAMotorMode},
//	{"dJointAddAMotorTorques",						(void **) &dJointAddAMotorTorques},
//	{"dJointSetLMotorNumAxes",						(void **) &dJointSetLMotorNumAxes},
//	{"dJointSetLMotorAxis",							(void **) &dJointSetLMotorAxis},
//	{"dJointSetLMotorParam",						(void **) &dJointSetLMotorParam},
//	{"dJointSetPlane2DXParam",						(void **) &dJointSetPlane2DXParam},
//	{"dJointSetPlane2DYParam",						(void **) &dJointSetPlane2DYParam},
//	{"dJointSetPlane2DAngleParam",					(void **) &dJointSetPlane2DAngleParam},
//	{"dJointGetBallAnchor",							(void **) &dJointGetBallAnchor},
//	{"dJointGetBallAnchor2",						(void **) &dJointGetBallAnchor2},
//	{"dJointGetBallParam",							(void **) &dJointGetBallParam},
//	{"dJointGetHingeAnchor",						(void **) &dJointGetHingeAnchor},
//	{"dJointGetHingeAnchor2",						(void **) &dJointGetHingeAnchor2},
//	{"dJointGetHingeAxis",							(void **) &dJointGetHingeAxis},
//	{"dJointGetHingeParam",							(void **) &dJointGetHingeParam},
//	{"dJointGetHingeAngle",							(void **) &dJointGetHingeAngle},
//	{"dJointGetHingeAngleRate",						(void **) &dJointGetHingeAngleRate},
//	{"dJointGetSliderPosition",						(void **) &dJointGetSliderPosition},
//	{"dJointGetSliderPositionRate",					(void **) &dJointGetSliderPositionRate},
//	{"dJointGetSliderAxis",							(void **) &dJointGetSliderAxis},
//	{"dJointGetSliderParam",						(void **) &dJointGetSliderParam},
//	{"dJointGetHinge2Anchor",						(void **) &dJointGetHinge2Anchor},
//	{"dJointGetHinge2Anchor2",						(void **) &dJointGetHinge2Anchor2},
//	{"dJointGetHinge2Axis1",						(void **) &dJointGetHinge2Axis1},
//	{"dJointGetHinge2Axis2",						(void **) &dJointGetHinge2Axis2},
//	{"dJointGetHinge2Param",						(void **) &dJointGetHinge2Param},
//	{"dJointGetHinge2Angle1",						(void **) &dJointGetHinge2Angle1},
//	{"dJointGetHinge2Angle1Rate",					(void **) &dJointGetHinge2Angle1Rate},
//	{"dJointGetHinge2Angle2Rate",					(void **) &dJointGetHinge2Angle2Rate},
//	{"dJointGetUniversalAnchor",					(void **) &dJointGetUniversalAnchor},
//	{"dJointGetUniversalAnchor2",					(void **) &dJointGetUniversalAnchor2},
//	{"dJointGetUniversalAxis1",						(void **) &dJointGetUniversalAxis1},
//	{"dJointGetUniversalAxis2",						(void **) &dJointGetUniversalAxis2},
//	{"dJointGetUniversalParam",						(void **) &dJointGetUniversalParam},
//	{"dJointGetUniversalAngles",					(void **) &dJointGetUniversalAngles},
//	{"dJointGetUniversalAngle1",					(void **) &dJointGetUniversalAngle1},
//	{"dJointGetUniversalAngle2",					(void **) &dJointGetUniversalAngle2},
//	{"dJointGetUniversalAngle1Rate",				(void **) &dJointGetUniversalAngle1Rate},
//	{"dJointGetUniversalAngle2Rate",				(void **) &dJointGetUniversalAngle2Rate},
//	{"dJointGetPRAnchor",							(void **) &dJointGetPRAnchor},
//	{"dJointGetPRPosition",							(void **) &dJointGetPRPosition},
//	{"dJointGetPRPositionRate",						(void **) &dJointGetPRPositionRate},
//	{"dJointGetPRAngle",							(void **) &dJointGetPRAngle},
//	{"dJointGetPRAngleRate",						(void **) &dJointGetPRAngleRate},
//	{"dJointGetPRAxis1",							(void **) &dJointGetPRAxis1},
//	{"dJointGetPRAxis2",							(void **) &dJointGetPRAxis2},
//	{"dJointGetPRParam",							(void **) &dJointGetPRParam},
//	{"dJointGetPUAnchor",							(void **) &dJointGetPUAnchor},
//	{"dJointGetPUPosition",							(void **) &dJointGetPUPosition},
//	{"dJointGetPUPositionRate",						(void **) &dJointGetPUPositionRate},
//	{"dJointGetPUAxis1",							(void **) &dJointGetPUAxis1},
//	{"dJointGetPUAxis2",							(void **) &dJointGetPUAxis2},
//	{"dJointGetPUAxis3",							(void **) &dJointGetPUAxis3},
//	{"dJointGetPUAxisP",							(void **) &dJointGetPUAxisP},
//	{"dJointGetPUAngles",							(void **) &dJointGetPUAngles},
//	{"dJointGetPUAngle1",							(void **) &dJointGetPUAngle1},
//	{"dJointGetPUAngle1Rate",						(void **) &dJointGetPUAngle1Rate},
//	{"dJointGetPUAngle2",							(void **) &dJointGetPUAngle2},
//	{"dJointGetPUAngle2Rate",						(void **) &dJointGetPUAngle2Rate},
//	{"dJointGetPUParam",							(void **) &dJointGetPUParam},
//	{"dJointGetPistonPosition",						(void **) &dJointGetPistonPosition},
//	{"dJointGetPistonPositionRate",					(void **) &dJointGetPistonPositionRate},
//	{"dJointGetPistonAngle",						(void **) &dJointGetPistonAngle},
//	{"dJointGetPistonAngleRate",					(void **) &dJointGetPistonAngleRate},
//	{"dJointGetPistonAnchor",						(void **) &dJointGetPistonAnchor},
//	{"dJointGetPistonAnchor2",						(void **) &dJointGetPistonAnchor2},
//	{"dJointGetPistonAxis",							(void **) &dJointGetPistonAxis},
//	{"dJointGetPistonParam",						(void **) &dJointGetPistonParam},
//	{"dJointGetAMotorNumAxes",						(void **) &dJointGetAMotorNumAxes},
//	{"dJointGetAMotorAxis",							(void **) &dJointGetAMotorAxis},
//	{"dJointGetAMotorAxisRel",						(void **) &dJointGetAMotorAxisRel},
//	{"dJointGetAMotorAngle",						(void **) &dJointGetAMotorAngle},
//	{"dJointGetAMotorAngleRate",					(void **) &dJointGetAMotorAngleRate},
//	{"dJointGetAMotorParam",						(void **) &dJointGetAMotorParam},
//	{"dJointGetAMotorMode",							(void **) &dJointGetAMotorMode},
//	{"dJointGetLMotorNumAxes",						(void **) &dJointGetLMotorNumAxes},
//	{"dJointGetLMotorAxis",							(void **) &dJointGetLMotorAxis},
//	{"dJointGetLMotorParam",						(void **) &dJointGetLMotorParam},
//	{"dJointGetFixedParam",							(void **) &dJointGetFixedParam},
//	{"dConnectingJoint",							(void **) &dConnectingJoint},
//	{"dConnectingJointList",						(void **) &dConnectingJointList},
	{"dAreConnected",								(void **) &dAreConnected},
	{"dAreConnectedExcluding",						(void **) &dAreConnectedExcluding},
	{"dSimpleSpaceCreate",							(void **) &dSimpleSpaceCreate},
	{"dHashSpaceCreate",							(void **) &dHashSpaceCreate},
	{"dQuadTreeSpaceCreate",						(void **) &dQuadTreeSpaceCreate},
//	{"dSweepAndPruneSpaceCreate",					(void **) &dSweepAndPruneSpaceCreate},
	{"dSpaceDestroy",								(void **) &dSpaceDestroy},
//	{"dHashSpaceSetLevels",							(void **) &dHashSpaceSetLevels},
//	{"dHashSpaceGetLevels",							(void **) &dHashSpaceGetLevels},
//	{"dSpaceSetCleanup",							(void **) &dSpaceSetCleanup},
//	{"dSpaceGetCleanup",							(void **) &dSpaceGetCleanup},
//	{"dSpaceSetSublevel",							(void **) &dSpaceSetSublevel},
//	{"dSpaceGetSublevel",							(void **) &dSpaceGetSublevel},
//	{"dSpaceSetManualCleanup",						(void **) &dSpaceSetManualCleanup},
//	{"dSpaceGetManualCleanup",						(void **) &dSpaceGetManualCleanup},
//	{"dSpaceAdd",									(void **) &dSpaceAdd},
//	{"dSpaceRemove",								(void **) &dSpaceRemove},
//	{"dSpaceQuery",									(void **) &dSpaceQuery},
//	{"dSpaceClean",									(void **) &dSpaceClean},
//	{"dSpaceGetNumGeoms",							(void **) &dSpaceGetNumGeoms},
//	{"dSpaceGetGeom",								(void **) &dSpaceGetGeom},
//	{"dSpaceGetClass",								(void **) &dSpaceGetClass},
	{"dGeomDestroy",								(void **) &dGeomDestroy},
	{"dGeomSetData",								(void **) &dGeomSetData},
	{"dGeomGetData",								(void **) &dGeomGetData},
	{"dGeomSetBody",								(void **) &dGeomSetBody},
	{"dGeomGetBody",								(void **) &dGeomGetBody},
	{"dGeomSetPosition",							(void **) &dGeomSetPosition},
	{"dGeomSetRotation",							(void **) &dGeomSetRotation},
//	{"dGeomSetQuaternion",							(void **) &dGeomSetQuaternion},
//	{"dGeomGetPosition",							(void **) &dGeomGetPosition},
//	{"dGeomCopyPosition",							(void **) &dGeomCopyPosition},
//	{"dGeomGetRotation",							(void **) &dGeomGetRotation},
//	{"dGeomCopyRotation",							(void **) &dGeomCopyRotation},
//	{"dGeomGetQuaternion",							(void **) &dGeomGetQuaternion},
//	{"dGeomGetAABB",								(void **) &dGeomGetAABB},
	{"dGeomIsSpace",								(void **) &dGeomIsSpace},
//	{"dGeomGetSpace",								(void **) &dGeomGetSpace},
//	{"dGeomGetClass",								(void **) &dGeomGetClass},
//	{"dGeomSetCategoryBits",						(void **) &dGeomSetCategoryBits},
//	{"dGeomSetCollideBits",							(void **) &dGeomSetCollideBits},
//	{"dGeomGetCategoryBits",						(void **) &dGeomGetCategoryBits},
//	{"dGeomGetCollideBits",							(void **) &dGeomGetCollideBits},
//	{"dGeomEnable",									(void **) &dGeomEnable},
//	{"dGeomDisable",								(void **) &dGeomDisable},
//	{"dGeomIsEnabled",								(void **) &dGeomIsEnabled},
//	{"dGeomSetOffsetPosition",						(void **) &dGeomSetOffsetPosition},
//	{"dGeomSetOffsetRotation",						(void **) &dGeomSetOffsetRotation},
//	{"dGeomSetOffsetQuaternion",					(void **) &dGeomSetOffsetQuaternion},
//	{"dGeomSetOffsetWorldPosition",					(void **) &dGeomSetOffsetWorldPosition},
//	{"dGeomSetOffsetWorldRotation",					(void **) &dGeomSetOffsetWorldRotation},
//	{"dGeomSetOffsetWorldQuaternion",				(void **) &dGeomSetOffsetWorldQuaternion},
//	{"dGeomClearOffset",							(void **) &dGeomClearOffset},
//	{"dGeomIsOffset",								(void **) &dGeomIsOffset},
//	{"dGeomGetOffsetPosition",						(void **) &dGeomGetOffsetPosition},
//	{"dGeomCopyOffsetPosition",						(void **) &dGeomCopyOffsetPosition},
//	{"dGeomGetOffsetRotation",						(void **) &dGeomGetOffsetRotation},
//	{"dGeomCopyOffsetRotation",						(void **) &dGeomCopyOffsetRotation},
//	{"dGeomGetOffsetQuaternion",					(void **) &dGeomGetOffsetQuaternion},
	{"dCollide",									(void **) &dCollide},
	{"dSpaceCollide",								(void **) &dSpaceCollide},
	{"dSpaceCollide2",								(void **) &dSpaceCollide2},
	{"dCreateSphere",								(void **) &dCreateSphere},
//	{"dGeomSphereSetRadius",						(void **) &dGeomSphereSetRadius},
//	{"dGeomSphereGetRadius",						(void **) &dGeomSphereGetRadius},
//	{"dGeomSpherePointDepth",						(void **) &dGeomSpherePointDepth},
	{"dCreateConvex",								(void **) &dCreateConvex},
//	{"dGeomSetConvex",								(void **) &dGeomSetConvex},
	{"dCreateBox",									(void **) &dCreateBox},
//	{"dGeomBoxSetLengths",							(void **) &dGeomBoxSetLengths},
//	{"dGeomBoxGetLengths",							(void **) &dGeomBoxGetLengths},
//	{"dGeomBoxPointDepth",							(void **) &dGeomBoxPointDepth},
//	{"dGeomBoxPointDepth",							(void **) &dGeomBoxPointDepth},
//	{"dCreatePlane",								(void **) &dCreatePlane},
//	{"dGeomPlaneSetParams",							(void **) &dGeomPlaneSetParams},
//	{"dGeomPlaneGetParams",							(void **) &dGeomPlaneGetParams},
//	{"dGeomPlanePointDepth",						(void **) &dGeomPlanePointDepth},
	{"dCreateCapsule",								(void **) &dCreateCapsule},
//	{"dGeomCapsuleSetParams",						(void **) &dGeomCapsuleSetParams},
//	{"dGeomCapsuleGetParams",						(void **) &dGeomCapsuleGetParams},
//	{"dGeomCapsulePointDepth",						(void **) &dGeomCapsulePointDepth},
	{"dCreateCylinder",								(void **) &dCreateCylinder},
//	{"dGeomCylinderSetParams",						(void **) &dGeomCylinderSetParams},
//	{"dGeomCylinderGetParams",						(void **) &dGeomCylinderGetParams},
//	{"dCreateRay",									(void **) &dCreateRay},
//	{"dGeomRaySetLength",							(void **) &dGeomRaySetLength},
//	{"dGeomRayGetLength",							(void **) &dGeomRayGetLength},
//	{"dGeomRaySet",									(void **) &dGeomRaySet},
//	{"dGeomRayGet",									(void **) &dGeomRayGet},
	{"dCreateGeomTransform",						(void **) &dCreateGeomTransform},
	{"dGeomTransformSetGeom",						(void **) &dGeomTransformSetGeom},
//	{"dGeomTransformGetGeom",						(void **) &dGeomTransformGetGeom},
	{"dGeomTransformSetCleanup",					(void **) &dGeomTransformSetCleanup},
//	{"dGeomTransformGetCleanup",					(void **) &dGeomTransformGetCleanup},
//	{"dGeomTransformSetInfo",						(void **) &dGeomTransformSetInfo},
//	{"dGeomTransformGetInfo",						(void **) &dGeomTransformGetInfo},
	{"dGeomTriMeshDataCreate",                      (void **) &dGeomTriMeshDataCreate},
	{"dGeomTriMeshDataDestroy",                     (void **) &dGeomTriMeshDataDestroy},
//	{"dGeomTriMeshDataSet",                         (void **) &dGeomTriMeshDataSet},
//	{"dGeomTriMeshDataGet",                         (void **) &dGeomTriMeshDataGet},
//	{"dGeomTriMeshSetLastTransform",                (void **) &dGeomTriMeshSetLastTransform},
//	{"dGeomTriMeshGetLastTransform",                (void **) &dGeomTriMeshGetLastTransform},
	{"dGeomTriMeshDataBuildSingle",                 (void **) &dGeomTriMeshDataBuildSingle},
//	{"dGeomTriMeshDataBuildSingle1",                (void **) &dGeomTriMeshDataBuildSingle1},
//	{"dGeomTriMeshDataBuildDouble",                 (void **) &dGeomTriMeshDataBuildDouble},
//	{"dGeomTriMeshDataBuildDouble1",                (void **) &dGeomTriMeshDataBuildDouble1},
//	{"dGeomTriMeshDataBuildSimple",                 (void **) &dGeomTriMeshDataBuildSimple},
//	{"dGeomTriMeshDataBuildSimple1",                (void **) &dGeomTriMeshDataBuildSimple1},
//	{"dGeomTriMeshDataPreprocess",                  (void **) &dGeomTriMeshDataPreprocess},
//	{"dGeomTriMeshDataGetBuffer",                   (void **) &dGeomTriMeshDataGetBuffer},
//	{"dGeomTriMeshDataSetBuffer",                   (void **) &dGeomTriMeshDataSetBuffer},
//	{"dGeomTriMeshSetCallback",                     (void **) &dGeomTriMeshSetCallback},
//	{"dGeomTriMeshGetCallback",                     (void **) &dGeomTriMeshGetCallback},
//	{"dGeomTriMeshSetArrayCallback",                (void **) &dGeomTriMeshSetArrayCallback},
//	{"dGeomTriMeshGetArrayCallback",                (void **) &dGeomTriMeshGetArrayCallback},
//	{"dGeomTriMeshSetRayCallback",                  (void **) &dGeomTriMeshSetRayCallback},
//	{"dGeomTriMeshGetRayCallback",                  (void **) &dGeomTriMeshGetRayCallback},
//	{"dGeomTriMeshSetTriMergeCallback",             (void **) &dGeomTriMeshSetTriMergeCallback},
//	{"dGeomTriMeshGetTriMergeCallback",             (void **) &dGeomTriMeshGetTriMergeCallback},
	{"dCreateTriMesh",                              (void **) &dCreateTriMesh},
//	{"dGeomTriMeshSetData",                         (void **) &dGeomTriMeshSetData},
//	{"dGeomTriMeshGetData",                         (void **) &dGeomTriMeshGetData},
//	{"dGeomTriMeshEnableTC",                        (void **) &dGeomTriMeshEnableTC},
//	{"dGeomTriMeshIsTCEnabled",                     (void **) &dGeomTriMeshIsTCEnabled},
//	{"dGeomTriMeshClearTCCache",                    (void **) &dGeomTriMeshClearTCCache},
//	{"dGeomTriMeshGetTriMeshDataID",                (void **) &dGeomTriMeshGetTriMeshDataID},
//	{"dGeomTriMeshGetTriangle",                     (void **) &dGeomTriMeshGetTriangle},
//	{"dGeomTriMeshGetPoint",                        (void **) &dGeomTriMeshGetPoint},
//	{"dGeomTriMeshGetTriangleCount",                (void **) &dGeomTriMeshGetTriangleCount},
//	{"dGeomTriMeshDataUpdate",                      (void **) &dGeomTriMeshDataUpdate},
	{NULL, NULL}
};
// Handle for ODE DLL
dllhandle_t ode_dll = NULL;
#endif

static void World_Physics_Init(void)
{
#ifndef LINK_TO_LIBODE
	const char* dllnames [] =
	{
# if defined(WIN32)
		"libode3.dll",
		"libode2.dll",
		"libode1.dll",
# elif defined(MACOSX)
		"libode.3.dylib",
		"libode.2.dylib",
		"libode.1.dylib",
# else
		"libode.so.3",
		"libode.so.2",
		"libode.so.1",
# endif
		NULL
	};
#endif

	Cvar_RegisterVariable(&physics_ode_quadtree_depth);
	Cvar_RegisterVariable(&physics_ode_contactsurfacelayer);
	Cvar_RegisterVariable(&physics_ode_worldstep_iterations);
	Cvar_RegisterVariable(&physics_ode_contact_mu);
	Cvar_RegisterVariable(&physics_ode_contact_erp);
	Cvar_RegisterVariable(&physics_ode_contact_cfm);
	Cvar_RegisterVariable(&physics_ode_contact_maxpoints);
	Cvar_RegisterVariable(&physics_ode_world_erp);
	Cvar_RegisterVariable(&physics_ode_world_cfm);
	Cvar_RegisterVariable(&physics_ode_world_damping);
	Cvar_RegisterVariable(&physics_ode_world_damping_linear);
	Cvar_RegisterVariable(&physics_ode_world_damping_linear_threshold);
	Cvar_RegisterVariable(&physics_ode_world_damping_angular);
	Cvar_RegisterVariable(&physics_ode_world_damping_angular_threshold);
	Cvar_RegisterVariable(&physics_ode_world_gravitymod);
	Cvar_RegisterVariable(&physics_ode_iterationsperframe);
	Cvar_RegisterVariable(&physics_ode_constantstep);
	Cvar_RegisterVariable(&physics_ode_movelimit);
	Cvar_RegisterVariable(&physics_ode_spinlimit);
	Cvar_RegisterVariable(&physics_ode_trick_fixnan);
	Cvar_RegisterVariable(&physics_ode_autodisable);
	Cvar_RegisterVariable(&physics_ode_autodisable_steps);
	Cvar_RegisterVariable(&physics_ode_autodisable_time);
	Cvar_RegisterVariable(&physics_ode_autodisable_threshold_linear);
	Cvar_RegisterVariable(&physics_ode_autodisable_threshold_angular);
	Cvar_RegisterVariable(&physics_ode_autodisable_threshold_samples);
	Cvar_RegisterVariable(&physics_ode_printstats);
	Cvar_RegisterVariable(&physics_ode_allowconvex);
	Cvar_RegisterVariable(&physics_ode);

#ifndef LINK_TO_LIBODE
	// Load the DLL
	if (Sys_LoadDependency (dllnames, &ode_dll, odefuncs))
#endif
	{
		dInitODE();
//		dInitODE2(0);
#ifndef LINK_TO_LIBODE
# ifdef dSINGLE
		if (!dCheckConfiguration("ODE_single_precision"))
# else
		if (!dCheckConfiguration("ODE_double_precision"))
# endif
		{
# ifdef dSINGLE
			Con_Printf("ODE library not compiled for single precision - incompatible!  Not using ODE physics.\n");
# else
			Con_Printf("ODE library not compiled for double precision - incompatible!  Not using ODE physics.\n");
# endif
			Sys_FreeLibrary(&ode_dll);
			ode_dll = NULL;
		}
		else
		{
# ifdef dSINGLE
			Con_Printf("ODE library loaded with single precision.\n");
# else
			Con_Printf("ODE library loaded with double precision.\n");
# endif
			Con_Printf("ODE configuration list: %s\n", dGetConfiguration());
		}
#endif
	}
}
static void World_Physics_Shutdown(void)
{
#ifndef LINK_TO_LIBODE
	if (ode_dll)
#endif
	{
		dCloseODE();
#ifndef LINK_TO_LIBODE
		Sys_FreeLibrary(&ode_dll);
		ode_dll = NULL;
#endif
	}
}

static void World_Physics_UpdateODE(world_t *world)
{
	dWorldID odeworld;

	odeworld = (dWorldID)world->physics.ode_world;

	// ERP and CFM
	if (physics_ode_world_erp.value >= 0)
		dWorldSetERP(odeworld, physics_ode_world_erp.value);
	if (physics_ode_world_cfm.value >= 0)
		dWorldSetCFM(odeworld, physics_ode_world_cfm.value);
	// Damping
	if (physics_ode_world_damping.integer)
	{
		dWorldSetLinearDamping(odeworld, (physics_ode_world_damping_linear.value >= 0) ? (physics_ode_world_damping_linear.value * physics_ode_world_damping.value) : 0);
		dWorldSetLinearDampingThreshold(odeworld, (physics_ode_world_damping_linear_threshold.value >= 0) ? (physics_ode_world_damping_linear_threshold.value * physics_ode_world_damping.value) : 0);
		dWorldSetAngularDamping(odeworld, (physics_ode_world_damping_angular.value >= 0) ? (physics_ode_world_damping_angular.value * physics_ode_world_damping.value) : 0);
		dWorldSetAngularDampingThreshold(odeworld, (physics_ode_world_damping_angular_threshold.value >= 0) ? (physics_ode_world_damping_angular_threshold.value * physics_ode_world_damping.value) : 0);
	}
	else
	{
		dWorldSetLinearDamping(odeworld, 0);
		dWorldSetLinearDampingThreshold(odeworld, 0);
		dWorldSetAngularDamping(odeworld, 0);
		dWorldSetAngularDampingThreshold(odeworld, 0);
	}
	// Autodisable
	dWorldSetAutoDisableFlag(odeworld, (physics_ode_autodisable.integer) ? 1 : 0);
	if (physics_ode_autodisable.integer)
	{
		dWorldSetAutoDisableSteps(odeworld, bound(1, physics_ode_autodisable_steps.integer, 100)); 
		dWorldSetAutoDisableTime(odeworld, physics_ode_autodisable_time.value);
		dWorldSetAutoDisableAverageSamplesCount(odeworld, bound(1, physics_ode_autodisable_threshold_samples.integer, 100));
		dWorldSetAutoDisableLinearThreshold(odeworld, physics_ode_autodisable_threshold_linear.value); 
		dWorldSetAutoDisableAngularThreshold(odeworld, physics_ode_autodisable_threshold_angular.value); 
	}
}

static void World_Physics_EnableODE(world_t *world)
{
	dVector3 center, extents;
	if (world->physics.ode)
		return;
#ifndef LINK_TO_LIBODE
	if (!ode_dll)
		return;
#endif
	world->physics.ode = true;
	VectorMAM(0.5f, world->mins, 0.5f, world->maxs, center);
	VectorSubtract(world->maxs, center, extents);
	world->physics.ode_world = dWorldCreate();
	world->physics.ode_space = dQuadTreeSpaceCreate(NULL, center, extents, bound(1, physics_ode_quadtree_depth.integer, 10));
	world->physics.ode_contactgroup = dJointGroupCreate(0);

	World_Physics_UpdateODE(world);
}

static void World_Physics_Start(world_t *world)
{
	if (world->physics.ode)
		return;
	World_Physics_EnableODE(world);
}

static void World_Physics_End(world_t *world)
{
	if (world->physics.ode)
	{
		dWorldDestroy((dWorldID)world->physics.ode_world);
		dSpaceDestroy((dSpaceID)world->physics.ode_space);
		dJointGroupDestroy((dJointGroupID)world->physics.ode_contactgroup);
		world->physics.ode = false;
	}
}

void World_Physics_RemoveJointFromEntity(world_t *world, prvm_edict_t *ed)
{
	ed->priv.server->ode_joint_type = 0;
	if(ed->priv.server->ode_joint)
		dJointDestroy((dJointID)ed->priv.server->ode_joint);
	ed->priv.server->ode_joint = NULL;
}

void World_Physics_RemoveFromEntity(world_t *world, prvm_edict_t *ed)
{
	edict_odefunc_t *f, *nf;

	// entity is not physics controlled, free any physics data
	ed->priv.server->ode_physics = false;
	if (ed->priv.server->ode_geom)
		dGeomDestroy((dGeomID)ed->priv.server->ode_geom);
	ed->priv.server->ode_geom = NULL;
	if (ed->priv.server->ode_body)
	{
		dJointID j;
		dBodyID b1, b2;
		prvm_edict_t *ed2;
		while(dBodyGetNumJoints((dBodyID)ed->priv.server->ode_body))
		{
			j = dBodyGetJoint((dBodyID)ed->priv.server->ode_body, 0);
			ed2 = (prvm_edict_t *) dJointGetData(j);
			b1 = dJointGetBody(j, 0);
			b2 = dJointGetBody(j, 1);
			if(b1 == (dBodyID)ed->priv.server->ode_body)
			{
				b1 = 0;
				ed2->priv.server->ode_joint_enemy = 0;
			}
			if(b2 == (dBodyID)ed->priv.server->ode_body)
			{
				b2 = 0;
				ed2->priv.server->ode_joint_aiment = 0;
			}
			dJointAttach(j, b1, b2);
		}
		dBodyDestroy((dBodyID)ed->priv.server->ode_body);
	}
	ed->priv.server->ode_body = NULL;
	if (ed->priv.server->ode_vertex3f)
		Mem_Free(ed->priv.server->ode_vertex3f);
	ed->priv.server->ode_vertex3f = NULL;
	ed->priv.server->ode_numvertices = 0;
	if (ed->priv.server->ode_element3i)
		Mem_Free(ed->priv.server->ode_element3i);
	ed->priv.server->ode_element3i = NULL;
	ed->priv.server->ode_numtriangles = 0;
	if(ed->priv.server->ode_massbuf)
		Mem_Free(ed->priv.server->ode_massbuf);
	ed->priv.server->ode_massbuf = NULL;
	// clear functions stack
	for(f = ed->priv.server->ode_func; f; f = nf)
	{
		nf = f->next;
		Mem_Free(f);
	}
	ed->priv.server->ode_func = NULL;
}

void World_Physics_ApplyCmd(prvm_edict_t *ed, edict_odefunc_t *f)
{
	dBodyID body = (dBodyID)ed->priv.server->ode_body;

	switch(f->type)
	{
	case ODEFUNC_ENABLE:
		dBodyEnable(body);
		break;
	case ODEFUNC_DISABLE:
		dBodyDisable(body);
		break;
	case ODEFUNC_FORCE:
		dBodyEnable(body);
		dBodyAddForceAtPos(body, f->v1[0], f->v1[1], f->v1[2], f->v2[0], f->v2[1], f->v2[2]);
		break;
	case ODEFUNC_TORQUE:
		dBodyEnable(body);
		dBodyAddTorque(body, f->v1[0], f->v1[1], f->v1[2]);
		break;
	default:
		break;
	}
}

static void World_Physics_Frame_BodyToEntity(world_t *world, prvm_edict_t *ed)
{
	prvm_prog_t *prog = world->prog;
	const dReal *avel;
	const dReal *o;
	const dReal *r; // for some reason dBodyGetRotation returns a [3][4] matrix
	const dReal *vel;
	dBodyID body = (dBodyID)ed->priv.server->ode_body;
	int movetype;
	matrix4x4_t bodymatrix;
	matrix4x4_t entitymatrix;
	vec3_t angles;
	vec3_t avelocity;
	vec3_t forward, left, up;
	vec3_t origin;
	vec3_t spinvelocity;
	vec3_t velocity;
	int jointtype;
	if (!body)
		return;
	movetype = (int)PRVM_gameedictfloat(ed, movetype);
	if (movetype != MOVETYPE_PHYSICS)
	{
		jointtype = (int)PRVM_gameedictfloat(ed, jointtype);
		switch(jointtype)
		{
			// TODO feed back data from physics
			case JOINTTYPE_POINT:
				break;
			case JOINTTYPE_HINGE:
				break;
			case JOINTTYPE_SLIDER:
				break;
			case JOINTTYPE_UNIVERSAL:
				break;
			case JOINTTYPE_HINGE2:
				break;
			case JOINTTYPE_FIXED:
				break;
		}
		return;
	}
	// store the physics engine data into the entity
	o = dBodyGetPosition(body);
	r = dBodyGetRotation(body);
	vel = dBodyGetLinearVel(body);
	avel = dBodyGetAngularVel(body);
	VectorCopy(o, origin);
	forward[0] = r[0];
	forward[1] = r[4];
	forward[2] = r[8];
	left[0] = r[1];
	left[1] = r[5];
	left[2] = r[9];
	up[0] = r[2];
	up[1] = r[6];
	up[2] = r[10];
	VectorCopy(vel, velocity);
	VectorCopy(avel, spinvelocity);
	Matrix4x4_FromVectors(&bodymatrix, forward, left, up, origin);
	Matrix4x4_Concat(&entitymatrix, &bodymatrix, &ed->priv.server->ode_offsetimatrix);
	Matrix4x4_ToVectors(&entitymatrix, forward, left, up, origin);

	AnglesFromVectors(angles, forward, up, false);
	VectorSet(avelocity, RAD2DEG(spinvelocity[PITCH]), RAD2DEG(spinvelocity[ROLL]), RAD2DEG(spinvelocity[YAW]));

	{
		float pitchsign = 1;
		if(prog == SVVM_prog) // FIXME some better way?
		{
			pitchsign = SV_GetPitchSign(prog, ed);
		}
		else if(prog == CLVM_prog)
		{
			pitchsign = CL_GetPitchSign(prog, ed);
		}
		angles[PITCH] *= pitchsign;
		avelocity[PITCH] *= pitchsign;
	}

	VectorCopy(origin, PRVM_gameedictvector(ed, origin));
	VectorCopy(velocity, PRVM_gameedictvector(ed, velocity));
	//VectorCopy(forward, PRVM_gameedictvector(ed, axis_forward));
	//VectorCopy(left, PRVM_gameedictvector(ed, axis_left));
	//VectorCopy(up, PRVM_gameedictvector(ed, axis_up));
	//VectorCopy(spinvelocity, PRVM_gameedictvector(ed, spinvelocity));
	VectorCopy(angles, PRVM_gameedictvector(ed, angles));
	VectorCopy(avelocity, PRVM_gameedictvector(ed, avelocity));

	// values for BodyFromEntity to check if the qc modified anything later
	VectorCopy(origin, ed->priv.server->ode_origin);
	VectorCopy(velocity, ed->priv.server->ode_velocity);
	VectorCopy(angles, ed->priv.server->ode_angles);
	VectorCopy(avelocity, ed->priv.server->ode_avelocity);
	ed->priv.server->ode_gravity = dBodyGetGravityMode(body) != 0;

	if(prog == SVVM_prog) // FIXME some better way?
	{
		SV_LinkEdict(ed);
		SV_LinkEdict_TouchAreaGrid(ed);
	}
}

static void World_Physics_Frame_ForceFromEntity(world_t *world, prvm_edict_t *ed)
{
	prvm_prog_t *prog = world->prog;
	int forcetype = 0, movetype = 0, enemy = 0;
	vec3_t movedir, origin;

	movetype = (int)PRVM_gameedictfloat(ed, movetype);
	forcetype = (int)PRVM_gameedictfloat(ed, forcetype);
	if (movetype == MOVETYPE_PHYSICS)
		forcetype = FORCETYPE_NONE; // can't have both
	if (!forcetype)
		return;
	enemy = PRVM_gameedictedict(ed, enemy);
	if (enemy <= 0 || enemy >= prog->num_edicts || prog->edicts[enemy].free || prog->edicts[enemy].priv.server->ode_body == 0)
		return;
	VectorCopy(PRVM_gameedictvector(ed, movedir), movedir);
	VectorCopy(PRVM_gameedictvector(ed, origin), origin);
	dBodyEnable((dBodyID)prog->edicts[enemy].priv.server->ode_body);
	switch(forcetype)
	{
		case FORCETYPE_FORCE:
			if (movedir[0] || movedir[1] || movedir[2])
				dBodyAddForce((dBodyID)prog->edicts[enemy].priv.server->ode_body, movedir[0], movedir[1], movedir[2]);
			break;
		case FORCETYPE_FORCEATPOS:
			if (movedir[0] || movedir[1] || movedir[2])
				dBodyAddForceAtPos((dBodyID)prog->edicts[enemy].priv.server->ode_body, movedir[0], movedir[1], movedir[2], origin[0], origin[1], origin[2]);
			break;
		case FORCETYPE_TORQUE:
			if (movedir[0] || movedir[1] || movedir[2])
				dBodyAddTorque((dBodyID)prog->edicts[enemy].priv.server->ode_body, movedir[0], movedir[1], movedir[2]);
			break;
		case FORCETYPE_NONE:
		default:
			// bad force
			break;
	}
}

static void World_Physics_Frame_JointFromEntity(world_t *world, prvm_edict_t *ed)
{
	prvm_prog_t *prog = world->prog;
	dJointID j = 0;
	dBodyID b1 = 0;
	dBodyID b2 = 0;
	int movetype = 0;
	int jointtype = 0;
	int enemy = 0, aiment = 0;
	vec3_t origin, velocity, angles, forward, left, up, movedir;
	vec_t CFM, ERP, FMax, Stop, Vel;

	movetype = (int)PRVM_gameedictfloat(ed, movetype);
	jointtype = (int)PRVM_gameedictfloat(ed, jointtype);
	VectorClear(origin);
	VectorClear(velocity);
	VectorClear(angles);
	VectorClear(movedir);
	enemy = PRVM_gameedictedict(ed, enemy);
	aiment = PRVM_gameedictedict(ed, aiment);
	VectorCopy(PRVM_gameedictvector(ed, origin), origin);
	VectorCopy(PRVM_gameedictvector(ed, velocity), velocity);
	VectorCopy(PRVM_gameedictvector(ed, angles), angles);
	VectorCopy(PRVM_gameedictvector(ed, movedir), movedir);
	if(movetype == MOVETYPE_PHYSICS)
		jointtype = JOINTTYPE_NONE; // can't have both
	if(enemy <= 0 || enemy >= prog->num_edicts || prog->edicts[enemy].free || prog->edicts[enemy].priv.server->ode_body == 0)
		enemy = 0;
	if(aiment <= 0 || aiment >= prog->num_edicts || prog->edicts[aiment].free || prog->edicts[aiment].priv.server->ode_body == 0)
		aiment = 0;
	// see http://www.ode.org/old_list_archives/2006-January/017614.html
	// we want to set ERP? make it fps independent and work like a spring constant
	// note: if movedir[2] is 0, it becomes ERP = 1, CFM = 1.0 / (H * K)
	if(movedir[0] > 0 && movedir[1] > 0)
	{
		float K = movedir[0];
		float D = movedir[1];
		float R = 2.0 * D * sqrt(K); // we assume D is premultiplied by sqrt(sprungMass)
		CFM = 1.0 / (world->physics.ode_step * K + R); // always > 0
		ERP = world->physics.ode_step * K * CFM;
		Vel = 0;
		FMax = 0;
		Stop = movedir[2];
	}
	else if(movedir[1] < 0)
	{
		CFM = 0;
		ERP = 0;
		Vel = movedir[0];
		FMax = -movedir[1]; // TODO do we need to multiply with world.physics.ode_step?
		Stop = movedir[2] > 0 ? movedir[2] : dInfinity;
	}
	else // movedir[0] > 0, movedir[1] == 0 or movedir[0] < 0, movedir[1] >= 0
	{
		CFM = 0;
		ERP = 0;
		Vel = 0;
		FMax = 0;
		Stop = dInfinity;
	}
	if(jointtype == ed->priv.server->ode_joint_type && VectorCompare(origin, ed->priv.server->ode_joint_origin) && VectorCompare(velocity, ed->priv.server->ode_joint_velocity) && VectorCompare(angles, ed->priv.server->ode_joint_angles) && enemy == ed->priv.server->ode_joint_enemy && aiment == ed->priv.server->ode_joint_aiment && VectorCompare(movedir, ed->priv.server->ode_joint_movedir))
		return; // nothing to do
	AngleVectorsFLU(angles, forward, left, up);
	switch(jointtype)
	{
		case JOINTTYPE_POINT:
			j = dJointCreateBall((dWorldID)world->physics.ode_world, 0);
			break;
		case JOINTTYPE_HINGE:
			j = dJointCreateHinge((dWorldID)world->physics.ode_world, 0);
			break;
		case JOINTTYPE_SLIDER:
			j = dJointCreateSlider((dWorldID)world->physics.ode_world, 0);
			break;
		case JOINTTYPE_UNIVERSAL:
			j = dJointCreateUniversal((dWorldID)world->physics.ode_world, 0);
			break;
		case JOINTTYPE_HINGE2:
			j = dJointCreateHinge2((dWorldID)world->physics.ode_world, 0);
			break;
		case JOINTTYPE_FIXED:
			j = dJointCreateFixed((dWorldID)world->physics.ode_world, 0);
			break;
		case JOINTTYPE_NONE:
		default:
			// no joint
			j = 0;
			break;
	}
	if(ed->priv.server->ode_joint)
	{
		//Con_Printf("deleted old joint %i\n", (int) (ed - prog->edicts));
		dJointAttach((dJointID)ed->priv.server->ode_joint, 0, 0);
		dJointDestroy((dJointID)ed->priv.server->ode_joint);
	}
	ed->priv.server->ode_joint = (void *) j;
	ed->priv.server->ode_joint_type = jointtype;
	ed->priv.server->ode_joint_enemy = enemy;
	ed->priv.server->ode_joint_aiment = aiment;
	VectorCopy(origin, ed->priv.server->ode_joint_origin);
	VectorCopy(velocity, ed->priv.server->ode_joint_velocity);
	VectorCopy(angles, ed->priv.server->ode_joint_angles);
	VectorCopy(movedir, ed->priv.server->ode_joint_movedir);
	if(j)
	{
		//Con_Printf("made new joint %i\n", (int) (ed - prog->edicts));
		dJointSetData(j, (void *) ed);
		if(enemy)
			b1 = (dBodyID)prog->edicts[enemy].priv.server->ode_body;
		if(aiment)
			b2 = (dBodyID)prog->edicts[aiment].priv.server->ode_body;
		dJointAttach(j, b1, b2);

		switch(jointtype)
		{
			case JOINTTYPE_POINT:
				dJointSetBallAnchor(j, origin[0], origin[1], origin[2]);
				break;
			case JOINTTYPE_HINGE:
				dJointSetHingeAnchor(j, origin[0], origin[1], origin[2]);
				dJointSetHingeAxis(j, forward[0], forward[1], forward[2]);
				dJointSetHingeParam(j, dParamFMax, FMax);
				dJointSetHingeParam(j, dParamHiStop, Stop);
				dJointSetHingeParam(j, dParamLoStop, -Stop);
				dJointSetHingeParam(j, dParamStopCFM, CFM);
				dJointSetHingeParam(j, dParamStopERP, ERP);
				dJointSetHingeParam(j, dParamVel, Vel);
				break;
			case JOINTTYPE_SLIDER:
				dJointSetSliderAxis(j, forward[0], forward[1], forward[2]);
				dJointSetSliderParam(j, dParamFMax, FMax);
				dJointSetSliderParam(j, dParamHiStop, Stop);
				dJointSetSliderParam(j, dParamLoStop, -Stop);
				dJointSetSliderParam(j, dParamStopCFM, CFM);
				dJointSetSliderParam(j, dParamStopERP, ERP);
				dJointSetSliderParam(j, dParamVel, Vel);
				break;
			case JOINTTYPE_UNIVERSAL:
				dJointSetUniversalAnchor(j, origin[0], origin[1], origin[2]);
				dJointSetUniversalAxis1(j, forward[0], forward[1], forward[2]);
				dJointSetUniversalAxis2(j, up[0], up[1], up[2]);
				dJointSetUniversalParam(j, dParamFMax, FMax);
				dJointSetUniversalParam(j, dParamHiStop, Stop);
				dJointSetUniversalParam(j, dParamLoStop, -Stop);
				dJointSetUniversalParam(j, dParamStopCFM, CFM);
				dJointSetUniversalParam(j, dParamStopERP, ERP);
				dJointSetUniversalParam(j, dParamVel, Vel);
				dJointSetUniversalParam(j, dParamFMax2, FMax);
				dJointSetUniversalParam(j, dParamHiStop2, Stop);
				dJointSetUniversalParam(j, dParamLoStop2, -Stop);
				dJointSetUniversalParam(j, dParamStopCFM2, CFM);
				dJointSetUniversalParam(j, dParamStopERP2, ERP);
				dJointSetUniversalParam(j, dParamVel2, Vel);
				break;
			case JOINTTYPE_HINGE2:
				dJointSetHinge2Anchor(j, origin[0], origin[1], origin[2]);
				dJointSetHinge2Axis1(j, forward[0], forward[1], forward[2]);
				dJointSetHinge2Axis2(j, velocity[0], velocity[1], velocity[2]);
				dJointSetHinge2Param(j, dParamFMax, FMax);
				dJointSetHinge2Param(j, dParamHiStop, Stop);
				dJointSetHinge2Param(j, dParamLoStop, -Stop);
				dJointSetHinge2Param(j, dParamStopCFM, CFM);
				dJointSetHinge2Param(j, dParamStopERP, ERP);
				dJointSetHinge2Param(j, dParamVel, Vel);
				dJointSetHinge2Param(j, dParamFMax2, FMax);
				dJointSetHinge2Param(j, dParamHiStop2, Stop);
				dJointSetHinge2Param(j, dParamLoStop2, -Stop);
				dJointSetHinge2Param(j, dParamStopCFM2, CFM);
				dJointSetHinge2Param(j, dParamStopERP2, ERP);
				dJointSetHinge2Param(j, dParamVel2, Vel);
				break;
			case JOINTTYPE_FIXED:
				break;
			case 0:
			default:
				Sys_Error("what? but above the joint was valid...\n");
				break;
		}
#undef SETPARAMS

	}
}

// test convex geometry data
// planes for a cube, these should coincide with the 
dReal test_convex_planes[] = 
{
    1.0f ,0.0f ,0.0f ,2.25f,
    0.0f ,1.0f ,0.0f ,2.25f,
    0.0f ,0.0f ,1.0f ,2.25f,
    -1.0f,0.0f ,0.0f ,2.25f,
    0.0f ,-1.0f,0.0f ,2.25f,
    0.0f ,0.0f ,-1.0f,2.25f
};
const unsigned int test_convex_planecount = 6;
// points for a cube
dReal test_convex_points[] = 
{
	2.25f,2.25f,2.25f,    // point 0
	-2.25f,2.25f,2.25f,   // point 1
    2.25f,-2.25f,2.25f,   // point 2
    -2.25f,-2.25f,2.25f,  // point 3
    2.25f,2.25f,-2.25f,   // point 4
    -2.25f,2.25f,-2.25f,  // point 5
    2.25f,-2.25f,-2.25f,  // point 6
    -2.25f,-2.25f,-2.25f, // point 7
};
const unsigned int test_convex_pointcount = 8;
// polygons for a cube (6 squares), index 
unsigned int test_convex_polygons[] = 
{
	4,0,2,6,4, // positive X
    4,1,0,4,5, // positive Y
    4,0,1,3,2, // positive Z
    4,3,1,5,7, // negative X
    4,2,3,7,6, // negative Y
    4,5,4,6,7, // negative Z
};

static void World_Physics_Frame_BodyFromEntity(world_t *world, prvm_edict_t *ed)
{
	prvm_prog_t *prog = world->prog;
	const float *iv;
	const int *ie;
	dBodyID body;
	dMass mass;
	const dReal *ovelocity, *ospinvelocity;
	void *dataID;
	model_t *model;
	float *ov;
	int *oe;
	int axisindex;
	int modelindex = 0;
	int movetype = MOVETYPE_NONE;
	int numtriangles;
	int numvertices;
	int solid = SOLID_NOT, geomtype = 0;
	int triangleindex;
	int vertexindex;
	mempool_t *mempool;
	qbool modified = false;
	vec3_t angles;
	vec3_t avelocity;
	vec3_t entmaxs;
	vec3_t entmins;
	vec3_t forward;
	vec3_t geomcenter;
	vec3_t geomsize;
	vec3_t left;
	vec3_t origin;
	vec3_t spinvelocity;
	vec3_t up;
	vec3_t velocity;
	vec_t f;
	vec_t length;
	vec_t massval = 1.0f;
	vec_t movelimit;
	vec_t radius;
	vec3_t scale;
	vec_t spinlimit;
	vec_t test;
	qbool gravity;
	qbool geom_modified = false;
	edict_odefunc_t *func, *nextf;

	dReal *planes, *planesData, *pointsData;
	unsigned int *polygons, *polygonsData, polyvert;
	qbool *mapped, *used, convex_compatible;
	int numplanes = 0, numpoints = 0, i;

#ifndef LINK_TO_LIBODE
	if (!ode_dll)
		return;
#endif
	VectorClear(entmins);
	VectorClear(entmaxs);

	solid = (int)PRVM_gameedictfloat(ed, solid);
	geomtype = (int)PRVM_gameedictfloat(ed, geomtype);
	movetype = (int)PRVM_gameedictfloat(ed, movetype);
	// support scale and q3map/radiant's modelscale_vec
	if (PRVM_gameedictvector(ed, modelscale_vec)[0] != 0.0 || PRVM_gameedictvector(ed, modelscale_vec)[1] != 0.0 || PRVM_gameedictvector(ed, modelscale_vec)[2] != 0.0)
		VectorCopy(PRVM_gameedictvector(ed, modelscale_vec), scale);
	else if (PRVM_gameedictfloat(ed, scale))
		VectorSet(scale, PRVM_gameedictfloat(ed, scale), PRVM_gameedictfloat(ed, scale), PRVM_gameedictfloat(ed, scale));
	else
		VectorSet(scale, 1.0f, 1.0f, 1.0f);
	modelindex = 0;
	if (PRVM_gameedictfloat(ed, mass))
		massval = PRVM_gameedictfloat(ed, mass);
	if (movetype != MOVETYPE_PHYSICS)
		massval = 1.0f;
	mempool = prog->progs_mempool;
	model = NULL;
	if (!geomtype)
	{
		// VorteX: keep support for deprecated solid fields to not break mods
		if (solid == SOLID_PHYSICS_TRIMESH || solid == SOLID_BSP)
			geomtype = GEOMTYPE_TRIMESH;
		else if (solid == SOLID_NOT || solid == SOLID_TRIGGER)
			geomtype = GEOMTYPE_NONE;
		else if (solid == SOLID_PHYSICS_SPHERE)
			geomtype = GEOMTYPE_SPHERE;
		else if (solid == SOLID_PHYSICS_CAPSULE)
			geomtype = GEOMTYPE_CAPSULE;
		else if (solid == SOLID_PHYSICS_CYLINDER)
			geomtype = GEOMTYPE_CYLINDER;
		else if (solid == SOLID_PHYSICS_BOX)
			geomtype = GEOMTYPE_BOX;
		else
			geomtype = GEOMTYPE_BOX;
	}
	if (geomtype == GEOMTYPE_TRIMESH)
	{
		modelindex = (int)PRVM_gameedictfloat(ed, modelindex);
		if (world == &sv.world)
			model = SV_GetModelByIndex(modelindex);
		else if (world == &cl.world)
			model = CL_GetModelByIndex(modelindex);
		else
			model = NULL;
		if (model)
		{
			entmins[0] = model->normalmins[0] * scale[0];
			entmins[1] = model->normalmins[1] * scale[1];
			entmins[2] = model->normalmins[2] * scale[2];
			entmaxs[0] = model->normalmaxs[0] * scale[0];
			entmaxs[1] = model->normalmaxs[1] * scale[1];
			entmaxs[2] = model->normalmaxs[2] * scale[2];
			geom_modified = !VectorCompare(ed->priv.server->ode_scale, scale) || ed->priv.server->ode_modelindex != modelindex;
		}
		else
		{
			Con_Printf("entity %i (classname %s) has no model\n", PRVM_NUM_FOR_EDICT(ed), PRVM_GetString(prog, PRVM_gameedictstring(ed, classname)));
			geomtype = GEOMTYPE_BOX;
			VectorCopy(PRVM_gameedictvector(ed, mins), entmins);
			VectorCopy(PRVM_gameedictvector(ed, maxs), entmaxs);
			modelindex = 0;
			geom_modified = !VectorCompare(ed->priv.server->ode_mins, entmins) || !VectorCompare(ed->priv.server->ode_maxs, entmaxs);
		}
	}
	else if (geomtype && geomtype != GEOMTYPE_NONE)
	{
		VectorCopy(PRVM_gameedictvector(ed, mins), entmins);
		VectorCopy(PRVM_gameedictvector(ed, maxs), entmaxs);
		geom_modified = !VectorCompare(ed->priv.server->ode_mins, entmins) || !VectorCompare(ed->priv.server->ode_maxs, entmaxs);
	}
	else
	{
		// geometry type not set, falling back
		if (ed->priv.server->ode_physics)
			World_Physics_RemoveFromEntity(world, ed);
		return;
	}

	VectorSubtract(entmaxs, entmins, geomsize);
	if (VectorLength2(geomsize) == 0)
	{
		// we don't allow point-size physics objects...
		if (ed->priv.server->ode_physics)
			World_Physics_RemoveFromEntity(world, ed);
		return;
	}

	// get friction
	ed->priv.server->ode_friction = PRVM_gameedictfloat(ed, friction) ? PRVM_gameedictfloat(ed, friction) : 1.0f;

	// check if we need to create or replace the geom
	if (!ed->priv.server->ode_physics || ed->priv.server->ode_mass != massval || geom_modified)
	{
		modified = true;
		World_Physics_RemoveFromEntity(world, ed);
		ed->priv.server->ode_physics = true;
		VectorMAM(0.5f, entmins, 0.5f, entmaxs, geomcenter);
		if (PRVM_gameedictvector(ed, massofs))
			VectorCopy(geomcenter, PRVM_gameedictvector(ed, massofs));

		// check geomsize
		if (geomsize[0] * geomsize[1] * geomsize[2] == 0)
		{
			if (movetype == MOVETYPE_PHYSICS)
				Con_Printf("entity %i (classname %s) .mass * .size_x * .size_y * .size_z == 0\n", PRVM_NUM_FOR_EDICT(ed), PRVM_GetString(prog, PRVM_gameedictstring(ed, classname)));
			VectorSet(geomsize, 1.0f, 1.0f, 1.0f);
		}

		// greate geom
		switch(geomtype)
		{
		case GEOMTYPE_TRIMESH:
			// add an optimized mesh to the model containing only the SUPERCONTENTS_SOLID surfaces
			if (!model->brush.collisionmesh)
				Mod_CreateCollisionMesh(model);
			if (!model->brush.collisionmesh)
			{
				Con_Printf("entity %i (classname %s) has no geometry\n", PRVM_NUM_FOR_EDICT(ed), PRVM_GetString(prog, PRVM_gameedictstring(ed, classname)));
				goto treatasbox;
			}

			// check if trimesh can be defined with convex
			convex_compatible = false;
			for (i = model->submodelsurfaces_start;i < model->submodelsurfaces_end;i++)
			{
				if (!strcmp(model->data_surfaces[i].texture->name, "collisionconvex"))
				{
					convex_compatible = true;
					break;
				}
			}

			// ODE requires persistent mesh storage, so we need to copy out
			// the data from the model because renderer restarts could free it
			// during the game, additionally we need to flip the triangles...
			// note: ODE does preprocessing of the mesh for culling, removing
			// concave edges, etc., so this is not a lightweight operation
			ed->priv.server->ode_numvertices = numvertices = model->brush.collisionmesh->numverts;
			ed->priv.server->ode_vertex3f = (float *)Mem_Alloc(mempool, numvertices * sizeof(float[3]));

			// VorteX: rebuild geomsize based on entity's collision mesh, honor scale
			VectorSet(entmins, 0, 0, 0);
			VectorSet(entmaxs, 0, 0, 0);
			for (vertexindex = 0, ov = ed->priv.server->ode_vertex3f, iv = model->brush.collisionmesh->vertex3f;vertexindex < numvertices;vertexindex++, ov += 3, iv += 3)
			{
				ov[0] = iv[0] * scale[0];
				ov[1] = iv[1] * scale[1];
				ov[2] = iv[2] * scale[2];
				entmins[0] = min(entmins[0], ov[0]);
				entmins[1] = min(entmins[1], ov[1]);
				entmins[2] = min(entmins[2], ov[2]);
				entmaxs[0] = max(entmaxs[0], ov[0]);
				entmaxs[1] = max(entmaxs[1], ov[1]);
				entmaxs[2] = max(entmaxs[2], ov[2]);
			}
			if (!PRVM_gameedictvector(ed, massofs))
				VectorMAM(0.5f, entmins, 0.5f, entmaxs, geomcenter);
			for (vertexindex = 0, ov = ed->priv.server->ode_vertex3f, iv = model->brush.collisionmesh->vertex3f;vertexindex < numvertices;vertexindex++, ov += 3, iv += 3)
			{
				ov[0] = ov[0] - geomcenter[0];
				ov[1] = ov[1] - geomcenter[1];
				ov[2] = ov[2] - geomcenter[2];
			}
			VectorSubtract(entmaxs, entmins, geomsize);
			if (VectorLength2(geomsize) == 0)
			{
				if (movetype == MOVETYPE_PHYSICS)
					Con_Printf("entity %i collision mesh has null geomsize\n", PRVM_NUM_FOR_EDICT(ed));
				VectorSet(geomsize, 1.0f, 1.0f, 1.0f);
			}
			ed->priv.server->ode_numtriangles = numtriangles = model->brush.collisionmesh->numtriangles;
			ed->priv.server->ode_element3i = (int *)Mem_Alloc(mempool, numtriangles * sizeof(int[3]));
			//memcpy(ed->priv.server->ode_element3i, model->brush.collisionmesh->element3i, ed->priv.server->ode_numtriangles * sizeof(int[3]));
			for (triangleindex = 0, oe = ed->priv.server->ode_element3i, ie = model->brush.collisionmesh->element3i;triangleindex < numtriangles;triangleindex++, oe += 3, ie += 3)
			{
				oe[0] = ie[2];
				oe[1] = ie[1];
				oe[2] = ie[0];
			}
			// create geom
			Matrix4x4_CreateTranslate(&ed->priv.server->ode_offsetmatrix, geomcenter[0], geomcenter[1], geomcenter[2]);
			if (!convex_compatible || !physics_ode_allowconvex.integer)
			{
				// trimesh
				dataID = dGeomTriMeshDataCreate();
				dGeomTriMeshDataBuildSingle((dTriMeshDataID)dataID, (void*)ed->priv.server->ode_vertex3f, sizeof(float[3]), ed->priv.server->ode_numvertices, ed->priv.server->ode_element3i, ed->priv.server->ode_numtriangles*3, sizeof(int[3]));
				ed->priv.server->ode_geom = (void *)dCreateTriMesh((dSpaceID)world->physics.ode_space, (dTriMeshDataID)dataID, NULL, NULL, NULL);
				dMassSetBoxTotal(&mass, massval, geomsize[0], geomsize[1], geomsize[2]);
			}
			else
			{
				// VorteX: this code is unfinished in two ways
				// - no duplicate vertex merging are done
				// - triangles that shares same edge and havee sam plane are not merget into poly
				// so, currently it only works for geosphere meshes with no UV

				Con_Printf("Build convex hull for model %s...\n", model->name);
				// build convex geometry from trimesh data
				// this ensures that trimesh's triangles can form correct convex geometry
				// not many of error checking is performed
				// ODE's conve hull data consist of:
				//    planes  : an array of planes in the form: normal X, normal Y, normal Z, distance
				//    points  : an array of points X,Y,Z
				//    polygons: an array of indices to the points of each  polygon,it should be the number of vertices
				//              followed by that amount of indices to "points" in counter clockwise order
				polygonsData = polygons = (unsigned int *)Mem_Alloc(mempool, numtriangles*sizeof(int)*4);
				planesData = planes = (dReal *)Mem_Alloc(mempool, numtriangles*sizeof(dReal)*4);
				mapped = (qbool *)Mem_Alloc(mempool, numvertices*sizeof(qbool));
				used = (qbool *)Mem_Alloc(mempool, numtriangles*sizeof(qbool));
				memset(mapped, 0, numvertices*sizeof(qbool));
				memset(used, 0, numtriangles*sizeof(qbool));
				numplanes = numpoints = polyvert = 0;
				// build convex hull
				// todo: merge duplicated verts here
				Con_Printf("Building...\n");
				iv = ed->priv.server->ode_vertex3f;
				for (triangleindex = 0; triangleindex < numtriangles; triangleindex++)
				{
					// already formed a polygon?
					if (used[triangleindex])
						continue; 
					// init polygon
					// switch clockwise->counterclockwise
					ie = &model->brush.collisionmesh->element3i[triangleindex*3];
					used[triangleindex] = true;
					TriangleNormal(&iv[ie[0]*3], &iv[ie[1]*3], &iv[ie[2]*3], planes);
					VectorNormalize(planes);
					polygons[0] = 3;
					polygons[3] = (unsigned int)ie[0]; mapped[polygons[3]] = true;
					polygons[2] = (unsigned int)ie[1]; mapped[polygons[2]] = true;
					polygons[1] = (unsigned int)ie[2]; mapped[polygons[1]] = true;

					// now find and include concave triangles
					for (i = triangleindex; i < numtriangles; i++)
					{
						if (used[i])
							continue;
						// should share at least 2 vertexes
						for (polyvert = 1; polyvert <= polygons[0]; polyvert++)
						{
							// todo: merge in triangles that shares an edge and have same plane here
						}
					}

					// add polygon to overall stats
					planes[3] = DotProduct(&iv[polygons[1]*3], planes);
					polygons += (polygons[0]+1);
					planes += 4;
					numplanes++;
				}
				Mem_Free(used);
				// save points
				for (vertexindex = 0, numpoints = 0; vertexindex < numvertices; vertexindex++)
					if (mapped[vertexindex])
						numpoints++;
				pointsData = (dReal *)Mem_Alloc(mempool, numpoints*sizeof(dReal)*3 + numplanes*sizeof(dReal)*4); // planes is appended
				for (vertexindex = 0, numpoints = 0; vertexindex < numvertices; vertexindex++)
				{
					if (mapped[vertexindex])
					{
						VectorCopy(&iv[vertexindex*3], &pointsData[numpoints*3]);
						numpoints++;
					}
				}
				Mem_Free(mapped);
				Con_Printf("Points: \n");
				for (i = 0; i < (int)numpoints; i++)
					Con_Printf("%3i: %3.1f %3.1f %3.1f\n", i, pointsData[i*3], pointsData[i*3+1], pointsData[i*3+2]);
				// save planes
				planes = planesData;
				planesData = pointsData + numpoints*3;
				memcpy(planesData, planes, numplanes*sizeof(dReal)*4);
				Mem_Free(planes);
				Con_Printf("planes...\n");
				for (i = 0; i < numplanes; i++)
					Con_Printf("%3i: %1.1f %1.1f %1.1f %1.1f\n", i, planesData[i*4], planesData[i*4 + 1], planesData[i*4 + 2], planesData[i*4 + 3]);
				// save polygons
				polyvert = polygons - polygonsData;
				polygons = polygonsData;
				polygonsData = (unsigned int *)Mem_Alloc(mempool, polyvert*sizeof(int));
				memcpy(polygonsData, polygons, polyvert*sizeof(int));
				Mem_Free(polygons);
				Con_Printf("Polygons: \n");
				polygons = polygonsData;
				for (i = 0; i < numplanes; i++)
				{
					Con_Printf("%3i : %i ", i, polygons[0]);
					for (triangleindex = 1; triangleindex <= (int)polygons[0]; triangleindex++)
						Con_Printf("%3i ", polygons[triangleindex]);
					polygons += (polygons[0]+1);
					Con_Printf("\n");
				}
				Mem_Free(ed->priv.server->ode_element3i);
				ed->priv.server->ode_element3i = (int *)polygonsData;
				Mem_Free(ed->priv.server->ode_vertex3f);
				ed->priv.server->ode_vertex3f = (float *)pointsData;
				// check for properly build polygons by calculating the determinant of the 3x3 matrix composed of the first 3 points in the polygon
				// this code is picked from ODE Source
				Con_Printf("Check...\n");
				polygons = polygonsData;
				for (i = 0; i < numplanes; i++)
				{
					if((pointsData[(polygons[1]*3)+0]*pointsData[(polygons[2]*3)+1]*pointsData[(polygons[3]*3)+2] +
						pointsData[(polygons[1]*3)+1]*pointsData[(polygons[2]*3)+2]*pointsData[(polygons[3]*3)+0] +
						pointsData[(polygons[1]*3)+2]*pointsData[(polygons[2]*3)+0]*pointsData[(polygons[3]*3)+1] -
						pointsData[(polygons[1]*3)+2]*pointsData[(polygons[2]*3)+1]*pointsData[(polygons[3]*3)+0] -
						pointsData[(polygons[1]*3)+1]*pointsData[(polygons[2]*3)+0]*pointsData[(polygons[3]*3)+2] -
						pointsData[(polygons[1]*3)+0]*pointsData[(polygons[2]*3)+2]*pointsData[(polygons[3]*3)+1]) < 0)
						Con_Printf(CON_WARN "WARNING: Polygon %d is not defined counterclockwise\n", i);
					if (planesData[(i*4)+3] < 0)
						Con_Printf(CON_WARN "WARNING: Plane %d does not contain the origin\n", i);
					polygons += (*polygons + 1);
				}
				// create geom
				Con_Printf("Create geom...\n");
				ed->priv.server->ode_geom = (void *)dCreateConvex((dSpaceID)world->physics.ode_space, planesData, numplanes, pointsData, numpoints, polygonsData);
				dMassSetBoxTotal(&mass, massval, geomsize[0], geomsize[1], geomsize[2]);
				Con_Printf("Done!\n");
			}
			break;
		case GEOMTYPE_BOX:
treatasbox:
			Matrix4x4_CreateTranslate(&ed->priv.server->ode_offsetmatrix, geomcenter[0], geomcenter[1], geomcenter[2]);
			ed->priv.server->ode_geom = (void *)dCreateBox((dSpaceID)world->physics.ode_space, geomsize[0], geomsize[1], geomsize[2]);
			dMassSetBoxTotal(&mass, massval, geomsize[0], geomsize[1], geomsize[2]);
			break;
		case GEOMTYPE_SPHERE:
			Matrix4x4_CreateTranslate(&ed->priv.server->ode_offsetmatrix, geomcenter[0], geomcenter[1], geomcenter[2]);
			ed->priv.server->ode_geom = (void *)dCreateSphere((dSpaceID)world->physics.ode_space, geomsize[0] * 0.5f);
			dMassSetSphereTotal(&mass, massval, geomsize[0] * 0.5f);
			break;
		case GEOMTYPE_CAPSULE:
			axisindex = 0;
			if (geomsize[axisindex] < geomsize[1])
				axisindex = 1;
			if (geomsize[axisindex] < geomsize[2])
				axisindex = 2;
			// the qc gives us 3 axis radius, the longest axis is the capsule
			// axis, since ODE doesn't like this idea we have to create a
			// capsule which uses the standard orientation, and apply a
			// transform to it
			if (axisindex == 0)
			{
				Matrix4x4_CreateFromQuakeEntity(&ed->priv.server->ode_offsetmatrix, geomcenter[0], geomcenter[1], geomcenter[2], 0, 0, 90, 1);
				radius = min(geomsize[1], geomsize[2]) * 0.5f;
			}
			else if (axisindex == 1)
			{
				Matrix4x4_CreateFromQuakeEntity(&ed->priv.server->ode_offsetmatrix, geomcenter[0], geomcenter[1], geomcenter[2], 90, 0, 0, 1);
				radius = min(geomsize[0], geomsize[2]) * 0.5f;
			}
			else
			{
				Matrix4x4_CreateFromQuakeEntity(&ed->priv.server->ode_offsetmatrix, geomcenter[0], geomcenter[1], geomcenter[2], 0, 0, 0, 1);
				radius = min(geomsize[0], geomsize[1]) * 0.5f;
			}
			length = geomsize[axisindex] - radius*2;
			// because we want to support more than one axisindex, we have to
			// create a transform, and turn on its cleanup setting (which will
			// cause the child to be destroyed when it is destroyed)
			ed->priv.server->ode_geom = (void *)dCreateCapsule((dSpaceID)world->physics.ode_space, radius, length);
			dMassSetCapsuleTotal(&mass, massval, axisindex+1, radius, length);
			break;
		case GEOMTYPE_CAPSULE_X:
			Matrix4x4_CreateFromQuakeEntity(&ed->priv.server->ode_offsetmatrix, geomcenter[0], geomcenter[1], geomcenter[2], 0, 0, 90, 1);
			radius = min(geomsize[1], geomsize[2]) * 0.5f;
			length = geomsize[0] - radius*2;
			// check if length is not enough, reduce radius then
			if (length <= 0)
			{
				radius -= (1 - length)*0.5;
				length = 1;
			}
			ed->priv.server->ode_geom = (void *)dCreateCapsule((dSpaceID)world->physics.ode_space, radius, length);
			dMassSetCapsuleTotal(&mass, massval, 1, radius, length);
			break;
		case GEOMTYPE_CAPSULE_Y:
			Matrix4x4_CreateFromQuakeEntity(&ed->priv.server->ode_offsetmatrix, geomcenter[0], geomcenter[1], geomcenter[2], 90, 0, 0, 1);
			radius = min(geomsize[0], geomsize[2]) * 0.5f;
			length = geomsize[1] - radius*2;
			// check if length is not enough, reduce radius then
			if (length <= 0)
			{
				radius -= (1 - length)*0.5;
				length = 1;
			}
			ed->priv.server->ode_geom = (void *)dCreateCapsule((dSpaceID)world->physics.ode_space, radius, length);
			dMassSetCapsuleTotal(&mass, massval, 2, radius, length);
			break;
		case GEOMTYPE_CAPSULE_Z:
			Matrix4x4_CreateFromQuakeEntity(&ed->priv.server->ode_offsetmatrix, geomcenter[0], geomcenter[1], geomcenter[2], 0, 0, 0, 1);
			radius = min(geomsize[1], geomsize[0]) * 0.5f;
			length = geomsize[2] - radius*2;
			// check if length is not enough, reduce radius then
			if (length <= 0)
			{
				radius -= (1 - length)*0.5;
				length = 1;
			}
			ed->priv.server->ode_geom = (void *)dCreateCapsule((dSpaceID)world->physics.ode_space, radius, length);
			dMassSetCapsuleTotal(&mass, massval, 3, radius, length);
			break;
		case GEOMTYPE_CYLINDER:
			axisindex = 0;
			if (geomsize[axisindex] < geomsize[1])
				axisindex = 1;
			if (geomsize[axisindex] < geomsize[2])
				axisindex = 2;
			// the qc gives us 3 axis radius, the longest axis is the capsule
			// axis, since ODE doesn't like this idea we have to create a
			// capsule which uses the standard orientation, and apply a
			// transform to it
			if (axisindex == 0)
			{
				Matrix4x4_CreateFromQuakeEntity(&ed->priv.server->ode_offsetmatrix, geomcenter[0], geomcenter[1], geomcenter[2], 0, 0, 90, 1);
				radius = min(geomsize[1], geomsize[2]) * 0.5f;
			}
			else if (axisindex == 1)
			{
				Matrix4x4_CreateFromQuakeEntity(&ed->priv.server->ode_offsetmatrix, geomcenter[0], geomcenter[1], geomcenter[2], 90, 0, 0, 1);
				radius = min(geomsize[0], geomsize[2]) * 0.5f;
			}
			else
			{
				Matrix4x4_CreateFromQuakeEntity(&ed->priv.server->ode_offsetmatrix, geomcenter[0], geomcenter[1], geomcenter[2], 0, 0, 0, 1);
				radius = min(geomsize[0], geomsize[1]) * 0.5f;
			}
			length = geomsize[axisindex];
			// check if length is not enough, reduce radius then
			if (length <= 0)
			{
				radius -= (1 - length)*0.5;
				length = 1;
			}
			ed->priv.server->ode_geom = (void *)dCreateCylinder((dSpaceID)world->physics.ode_space, radius, length);
			dMassSetCylinderTotal(&mass, massval, axisindex+1, radius, length);
			break;
		case GEOMTYPE_CYLINDER_X:
			Matrix4x4_CreateFromQuakeEntity(&ed->priv.server->ode_offsetmatrix, geomcenter[0], geomcenter[1], geomcenter[2], 0, 0, 90, 1);
			radius = min(geomsize[1], geomsize[2]) * 0.5f;
			length = geomsize[0];
			ed->priv.server->ode_geom = (void *)dCreateCylinder((dSpaceID)world->physics.ode_space, radius, length);
			dMassSetCylinderTotal(&mass, massval, 1, radius, length);
			break;
		case GEOMTYPE_CYLINDER_Y:
			Matrix4x4_CreateFromQuakeEntity(&ed->priv.server->ode_offsetmatrix, geomcenter[0], geomcenter[1], geomcenter[2], 90, 0, 0, 1);
			radius = min(geomsize[0], geomsize[2]) * 0.5f;
			length = geomsize[1];
			ed->priv.server->ode_geom = (void *)dCreateCylinder((dSpaceID)world->physics.ode_space, radius, length);
			dMassSetCylinderTotal(&mass, massval, 2, radius, length);
			break;
		case GEOMTYPE_CYLINDER_Z:
			Matrix4x4_CreateFromQuakeEntity(&ed->priv.server->ode_offsetmatrix, geomcenter[0], geomcenter[1], geomcenter[2], 0, 0, 0, 1);
			radius = min(geomsize[0], geomsize[1]) * 0.5f;
			length = geomsize[2];
			ed->priv.server->ode_geom = (void *)dCreateCylinder((dSpaceID)world->physics.ode_space, radius, length);
			dMassSetCylinderTotal(&mass, massval, 3, radius, length);
			break;
		default:
			Sys_Error("World_Physics_BodyFromEntity: unrecognized geomtype value %i was accepted by filter\n", solid);
			// this goto only exists to prevent warnings from the compiler
			// about uninitialized variables (mass), while allowing it to
			// catch legitimate uninitialized variable warnings
			goto treatasbox;
		}
		ed->priv.server->ode_mass = massval;
		ed->priv.server->ode_modelindex = modelindex;
		VectorCopy(entmins, ed->priv.server->ode_mins);
		VectorCopy(entmaxs, ed->priv.server->ode_maxs);
		VectorCopy(scale, ed->priv.server->ode_scale);
		ed->priv.server->ode_movelimit = min(geomsize[0], min(geomsize[1], geomsize[2]));
		Matrix4x4_Invert_Simple(&ed->priv.server->ode_offsetimatrix, &ed->priv.server->ode_offsetmatrix);
		ed->priv.server->ode_massbuf = Mem_Alloc(mempool, sizeof(mass));
		memcpy(ed->priv.server->ode_massbuf, &mass, sizeof(dMass));
	}

	if (ed->priv.server->ode_geom)
		dGeomSetData((dGeomID)ed->priv.server->ode_geom, (void*)ed);
	if (movetype == MOVETYPE_PHYSICS && ed->priv.server->ode_geom)
	{
		// entity is dynamic
		if (ed->priv.server->ode_body == NULL)
		{
			ed->priv.server->ode_body = (void *)(body = dBodyCreate((dWorldID)world->physics.ode_world));
			dGeomSetBody((dGeomID)ed->priv.server->ode_geom, body);
			dBodySetData(body, (void*)ed);
			dBodySetMass(body, (dMass *) ed->priv.server->ode_massbuf);
			modified = true;
		}
	}
	else
	{
		// entity is deactivated
		if (ed->priv.server->ode_body != NULL)
		{
			if(ed->priv.server->ode_geom)
				dGeomSetBody((dGeomID)ed->priv.server->ode_geom, 0);
			dBodyDestroy((dBodyID) ed->priv.server->ode_body);
			ed->priv.server->ode_body = NULL;
			modified = true;
		}
	}

	// get current data from entity
	VectorClear(origin);
	VectorClear(velocity);
	//VectorClear(forward);
	//VectorClear(left);
	//VectorClear(up);
	//VectorClear(spinvelocity);
	VectorClear(angles);
	VectorClear(avelocity);
	gravity = true;
	VectorCopy(PRVM_gameedictvector(ed, origin), origin);
	VectorCopy(PRVM_gameedictvector(ed, velocity), velocity);
	//VectorCopy(PRVM_gameedictvector(ed, axis_forward), forward);
	//VectorCopy(PRVM_gameedictvector(ed, axis_left), left);
	//VectorCopy(PRVM_gameedictvector(ed, axis_up), up);
	//VectorCopy(PRVM_gameedictvector(ed, spinvelocity), spinvelocity);
	VectorCopy(PRVM_gameedictvector(ed, angles), angles);
	VectorCopy(PRVM_gameedictvector(ed, avelocity), avelocity);
	if (PRVM_gameedictfloat(ed, gravity) != 0.0f && PRVM_gameedictfloat(ed, gravity) < 0.5f) gravity = false;
	if (ed == prog->edicts)
		gravity = false;

	// compatibility for legacy entities
	//if (!VectorLength2(forward) || solid == SOLID_BSP)
	{
		float pitchsign = 1;
		vec3_t qangles, qavelocity;
		VectorCopy(angles, qangles);
		VectorCopy(avelocity, qavelocity);

		if(prog == SVVM_prog) // FIXME some better way?
		{
			pitchsign = SV_GetPitchSign(prog, ed);
		}
		else if(prog == CLVM_prog)
		{
			pitchsign = CL_GetPitchSign(prog, ed);
		}
		qangles[PITCH] *= pitchsign;
		qavelocity[PITCH] *= pitchsign;

		AngleVectorsFLU(qangles, forward, left, up);
		// convert single-axis rotations in avelocity to spinvelocity
		// FIXME: untested math - check signs
		VectorSet(spinvelocity, DEG2RAD(qavelocity[PITCH]), DEG2RAD(qavelocity[ROLL]), DEG2RAD(qavelocity[YAW]));
	}

	// compatibility for legacy entities
	switch (solid)
	{
	case SOLID_BBOX:
	case SOLID_SLIDEBOX:
	case SOLID_CORPSE:
		VectorSet(forward, 1, 0, 0);
		VectorSet(left, 0, 1, 0);
		VectorSet(up, 0, 0, 1);
		VectorSet(spinvelocity, 0, 0, 0);
		break;
	}


	// we must prevent NANs...
	if (physics_ode_trick_fixnan.integer)
	{
		test = VectorLength2(origin) + VectorLength2(forward) + VectorLength2(left) + VectorLength2(up) + VectorLength2(velocity) + VectorLength2(spinvelocity);
		if (isnan(test))
		{
			modified = true;
			//Con_Printf("Fixing NAN values on entity %i : .classname = \"%s\" .origin = '%f %f %f' .velocity = '%f %f %f' .axis_forward = '%f %f %f' .axis_left = '%f %f %f' .axis_up = %f %f %f' .spinvelocity = '%f %f %f'\n", PRVM_NUM_FOR_EDICT(ed), PRVM_GetString(PRVM_gameedictstring(ed, classname)), origin[0], origin[1], origin[2], velocity[0], velocity[1], velocity[2], forward[0], forward[1], forward[2], left[0], left[1], left[2], up[0], up[1], up[2], spinvelocity[0], spinvelocity[1], spinvelocity[2]);
			if (physics_ode_trick_fixnan.integer >= 2)
				Con_Printf("Fixing NAN values on entity %i : .classname = \"%s\" .origin = '%f %f %f' .velocity = '%f %f %f' .angles = '%f %f %f' .avelocity = '%f %f %f'\n", PRVM_NUM_FOR_EDICT(ed), PRVM_GetString(prog, PRVM_gameedictstring(ed, classname)), origin[0], origin[1], origin[2], velocity[0], velocity[1], velocity[2], angles[0], angles[1], angles[2], avelocity[0], avelocity[1], avelocity[2]);
			test = VectorLength2(origin);
			if (isnan(test))
				VectorClear(origin);
			test = VectorLength2(forward) * VectorLength2(left) * VectorLength2(up);
			if (isnan(test))
			{
				VectorSet(angles, 0, 0, 0);
				VectorSet(forward, 1, 0, 0);
				VectorSet(left, 0, 1, 0);
				VectorSet(up, 0, 0, 1);
			}
			test = VectorLength2(velocity);
			if (isnan(test))
				VectorClear(velocity);
			test = VectorLength2(spinvelocity);
			if (isnan(test))
			{
				VectorClear(avelocity);
				VectorClear(spinvelocity);
			}
		}
	}

	// check if the qc edited any position data
	if (!VectorCompare(origin, ed->priv.server->ode_origin)
	 || !VectorCompare(velocity, ed->priv.server->ode_velocity)
	 || !VectorCompare(angles, ed->priv.server->ode_angles)
	 || !VectorCompare(avelocity, ed->priv.server->ode_avelocity)
	 || gravity != ed->priv.server->ode_gravity)
		modified = true;

	// store the qc values into the physics engine
	body = (dBodyID)ed->priv.server->ode_body;
	if (modified && ed->priv.server->ode_geom)
	{
		dVector3 r[3];
		matrix4x4_t entitymatrix;
		matrix4x4_t bodymatrix;

#if 0
		Con_Printf("entity %i got changed by QC\n", (int) (ed - prog->edicts));
		if(!VectorCompare(origin, ed->priv.server->ode_origin))
			Con_Printf("  origin: %f %f %f -> %f %f %f\n", ed->priv.server->ode_origin[0], ed->priv.server->ode_origin[1], ed->priv.server->ode_origin[2], origin[0], origin[1], origin[2]);
		if(!VectorCompare(velocity, ed->priv.server->ode_velocity))
			Con_Printf("  velocity: %f %f %f -> %f %f %f\n", ed->priv.server->ode_velocity[0], ed->priv.server->ode_velocity[1], ed->priv.server->ode_velocity[2], velocity[0], velocity[1], velocity[2]);
		if(!VectorCompare(angles, ed->priv.server->ode_angles))
			Con_Printf("  angles: %f %f %f -> %f %f %f\n", ed->priv.server->ode_angles[0], ed->priv.server->ode_angles[1], ed->priv.server->ode_angles[2], angles[0], angles[1], angles[2]);
		if(!VectorCompare(avelocity, ed->priv.server->ode_avelocity))
			Con_Printf("  avelocity: %f %f %f -> %f %f %f\n", ed->priv.server->ode_avelocity[0], ed->priv.server->ode_avelocity[1], ed->priv.server->ode_avelocity[2], avelocity[0], avelocity[1], avelocity[2]);
		if(gravity != ed->priv.server->ode_gravity)
			Con_Printf("  gravity: %i -> %i\n", ed->priv.server->ode_gravity, gravity);
#endif
		// values for BodyFromEntity to check if the qc modified anything later
		VectorCopy(origin, ed->priv.server->ode_origin);
		VectorCopy(velocity, ed->priv.server->ode_velocity);
		VectorCopy(angles, ed->priv.server->ode_angles);
		VectorCopy(avelocity, ed->priv.server->ode_avelocity);
		ed->priv.server->ode_gravity = gravity;

		Matrix4x4_FromVectors(&entitymatrix, forward, left, up, origin);
		Matrix4x4_Concat(&bodymatrix, &entitymatrix, &ed->priv.server->ode_offsetmatrix);
		Matrix4x4_ToVectors(&bodymatrix, forward, left, up, origin);
		r[0][0] = forward[0];
		r[1][0] = forward[1];
		r[2][0] = forward[2];
		r[0][1] = left[0];
		r[1][1] = left[1];
		r[2][1] = left[2];
		r[0][2] = up[0];
		r[1][2] = up[1];
		r[2][2] = up[2];
		if (body)
		{
			if (movetype == MOVETYPE_PHYSICS)
			{
				dGeomSetBody((dGeomID)ed->priv.server->ode_geom, body);
				dBodySetPosition(body, origin[0], origin[1], origin[2]);
				dBodySetRotation(body, r[0]);
				dBodySetLinearVel(body, velocity[0], velocity[1], velocity[2]);
				dBodySetAngularVel(body, spinvelocity[0], spinvelocity[1], spinvelocity[2]);
				dBodySetGravityMode(body, gravity);
			}
			else
			{
				dGeomSetBody((dGeomID)ed->priv.server->ode_geom, body);
				dBodySetPosition(body, origin[0], origin[1], origin[2]);
				dBodySetRotation(body, r[0]);
				dBodySetLinearVel(body, velocity[0], velocity[1], velocity[2]);
				dBodySetAngularVel(body, spinvelocity[0], spinvelocity[1], spinvelocity[2]);
				dBodySetGravityMode(body, gravity);
				dGeomSetBody((dGeomID)ed->priv.server->ode_geom, 0);
			}
		}
		else
		{
			// no body... then let's adjust the parameters of the geom directly
			dGeomSetBody((dGeomID)ed->priv.server->ode_geom, 0); // just in case we previously HAD a body (which should never happen)
			dGeomSetPosition((dGeomID)ed->priv.server->ode_geom, origin[0], origin[1], origin[2]);
			dGeomSetRotation((dGeomID)ed->priv.server->ode_geom, r[0]);
		}
	}

	if (body)
	{

		// limit movement speed to prevent missed collisions at high speed
		ovelocity = dBodyGetLinearVel(body);
		ospinvelocity = dBodyGetAngularVel(body);
		movelimit = ed->priv.server->ode_movelimit * world->physics.ode_movelimit;
		test = VectorLength2(ovelocity);
		if (test > movelimit*movelimit)
		{
			// scale down linear velocity to the movelimit
			// scale down angular velocity the same amount for consistency
			f = movelimit / sqrt(test);
			VectorScale(ovelocity, f, velocity);
			VectorScale(ospinvelocity, f, spinvelocity);
			dBodySetLinearVel(body, velocity[0], velocity[1], velocity[2]);
			dBodySetAngularVel(body, spinvelocity[0], spinvelocity[1], spinvelocity[2]);
		}

		// make sure the angular velocity is not exploding
		spinlimit = physics_ode_spinlimit.value;
		test = VectorLength2(ospinvelocity);
		if (test > spinlimit)
		{
			dBodySetAngularVel(body, 0, 0, 0);
		}

		// apply functions and clear stack
		for(func = ed->priv.server->ode_func; func; func = nextf)
		{
			nextf = func->next;
			World_Physics_ApplyCmd(ed, func);
			Mem_Free(func);
		}
		ed->priv.server->ode_func = NULL;
	}
}

#define MAX_CONTACTS 32
static void nearCallback (void *data, dGeomID o1, dGeomID o2)
{
	world_t *world = (world_t *)data;
	prvm_prog_t *prog = world->prog;
	dContact contact[MAX_CONTACTS]; // max contacts per collision pair
	int b1enabled = 0, b2enabled = 0;
	dBodyID b1, b2;
	dJointID c;
	int i;
	int numcontacts;
	float bouncefactor1 = 0.0f;
	float bouncestop1 = 60.0f / 800.0f;
	float bouncefactor2 = 0.0f;
	float bouncestop2 = 60.0f / 800.0f;
	float erp;
	dVector3 grav;
	prvm_edict_t *ed1, *ed2;

	if (dGeomIsSpace(o1) || dGeomIsSpace(o2))
	{
		// colliding a space with something
		dSpaceCollide2(o1, o2, data, &nearCallback);
		// Note we do not want to test intersections within a space,
		// only between spaces.
		//if (dGeomIsSpace(o1)) dSpaceCollide(o1, data, &nearCallback);
		//if (dGeomIsSpace(o2)) dSpaceCollide(o2, data, &nearCallback);
		return;
	}

	b1 = dGeomGetBody(o1);
	if (b1)
		b1enabled = dBodyIsEnabled(b1);
	b2 = dGeomGetBody(o2);
	if (b2)
		b2enabled = dBodyIsEnabled(b2);

	// at least one object has to be using MOVETYPE_PHYSICS and should be enabled or we just don't care
	if (!b1enabled && !b2enabled)
		return;
	
	// exit without doing anything if the two bodies are connected by a joint
	if (b1 && b2 && dAreConnectedExcluding(b1, b2, dJointTypeContact))
		return;

	ed1 = (prvm_edict_t *) dGeomGetData(o1);
	if(ed1 && ed1->free)
		ed1 = NULL;
	if(ed1)
	{
		bouncefactor1 = PRVM_gameedictfloat(ed1, bouncefactor);
		bouncestop1 = PRVM_gameedictfloat(ed1, bouncestop);
		if (!bouncestop1)
			bouncestop1 = 60.0f / 800.0f;
	}

	ed2 = (prvm_edict_t *) dGeomGetData(o2);
	if(ed2 && ed2->free)
		ed2 = NULL;
	if(ed2)
	{
		bouncefactor2 = PRVM_gameedictfloat(ed2, bouncefactor);
		bouncestop2 = PRVM_gameedictfloat(ed2, bouncestop);
		if (!bouncestop2)
			bouncestop2 = 60.0f / 800.0f;
	}

	if(prog == SVVM_prog)
	{
		if(ed1 && PRVM_serveredictfunction(ed1, touch))
		{
			SV_LinkEdict_TouchAreaGrid_Call(ed1, ed2 ? ed2 : prog->edicts);
		}
		if(ed2 && PRVM_serveredictfunction(ed2, touch))
		{
			SV_LinkEdict_TouchAreaGrid_Call(ed2, ed1 ? ed1 : prog->edicts);
		}
	}

	// merge bounce factors and bounce stop
	if(bouncefactor2 > 0)
	{
		if(bouncefactor1 > 0)
		{
			// TODO possibly better logic to merge bounce factor data?
			if(bouncestop2 < bouncestop1)
				bouncestop1 = bouncestop2;
			if(bouncefactor2 > bouncefactor1)
				bouncefactor1 = bouncefactor2;
		}
		else
		{
			bouncestop1 = bouncestop2;
			bouncefactor1 = bouncefactor2;
		}
	}
	dWorldGetGravity((dWorldID)world->physics.ode_world, grav);
	bouncestop1 *= fabs(grav[2]);

	// get erp
	// select object that moves faster ang get it's erp
	erp = (VectorLength2(PRVM_gameedictvector(ed1, velocity)) > VectorLength2(PRVM_gameedictvector(ed2, velocity))) ? PRVM_gameedictfloat(ed1, erp) : PRVM_gameedictfloat(ed2, erp);

	// get max contact points for this collision
	numcontacts = (int)PRVM_gameedictfloat(ed1, maxcontacts);
	if (!numcontacts)
		numcontacts = physics_ode_contact_maxpoints.integer;
	if (PRVM_gameedictfloat(ed2, maxcontacts))
		numcontacts = max(numcontacts, (int)PRVM_gameedictfloat(ed2, maxcontacts));
	else
		numcontacts = max(numcontacts, physics_ode_contact_maxpoints.integer);

	// generate contact points between the two non-space geoms
	numcontacts = dCollide(o1, o2, min(MAX_CONTACTS, numcontacts), &(contact[0].geom), sizeof(contact[0]));
	// add these contact points to the simulation
	for (i = 0;i < numcontacts;i++)
	{
		contact[i].surface.mode = (physics_ode_contact_mu.value != -1 ? dContactApprox1 : 0) | (physics_ode_contact_erp.value != -1 ? dContactSoftERP : 0) | (physics_ode_contact_cfm.value != -1 ? dContactSoftCFM : 0) | (bouncefactor1 > 0 ? dContactBounce : 0);
		contact[i].surface.mu = physics_ode_contact_mu.value * ed1->priv.server->ode_friction * ed2->priv.server->ode_friction;
		contact[i].surface.soft_erp = physics_ode_contact_erp.value + erp;
		contact[i].surface.soft_cfm = physics_ode_contact_cfm.value;
		contact[i].surface.bounce = bouncefactor1;
		contact[i].surface.bounce_vel = bouncestop1;
		c = dJointCreateContact((dWorldID)world->physics.ode_world, (dJointGroupID)world->physics.ode_contactgroup, contact + i);
		dJointAttach(c, b1, b2);
	}
}

void World_Physics_Frame(world_t *world, double frametime, double gravity)
{
	prvm_prog_t *prog = world->prog;
	double tdelta, tdelta2, tdelta3, simulationtime, collisiontime;

	tdelta = Sys_DirtyTime();
	if (world->physics.ode && physics_ode.integer)
	{
		int i;
		prvm_edict_t *ed;

		if (!physics_ode_constantstep.value)
		{
			world->physics.ode_iterations = bound(1, physics_ode_iterationsperframe.integer, 1000);
			world->physics.ode_step = frametime / world->physics.ode_iterations;
		}
		else
		{
			world->physics.ode_time += frametime;
			// step size
			if (physics_ode_constantstep.value > 0 && physics_ode_constantstep.value < 1)
				world->physics.ode_step = physics_ode_constantstep.value;
			else
				world->physics.ode_step = sys_ticrate.value;
			if (world->physics.ode_time > 0.2f)
				world->physics.ode_time = world->physics.ode_step;
			// set number of iterations to process
			world->physics.ode_iterations = 0;
			while(world->physics.ode_time >= world->physics.ode_step)
			{
				world->physics.ode_iterations++;
				world->physics.ode_time -= world->physics.ode_step;
			}
		}	
		world->physics.ode_movelimit = physics_ode_movelimit.value / world->physics.ode_step;
		World_Physics_UpdateODE(world);

		// copy physics properties from entities to physics engine
		if (prog)
		{
			for (i = 0, ed = prog->edicts + i;i < prog->num_edicts;i++, ed++)
				if (!prog->edicts[i].free)
					World_Physics_Frame_BodyFromEntity(world, ed);
			// oh, and it must be called after all bodies were created
			for (i = 0, ed = prog->edicts + i;i < prog->num_edicts;i++, ed++)
				if (!prog->edicts[i].free)
					World_Physics_Frame_JointFromEntity(world, ed);
		}

		tdelta2 = Sys_DirtyTime();
		collisiontime = 0;
		for (i = 0;i < world->physics.ode_iterations;i++)
		{
			// set the gravity
			dWorldSetGravity((dWorldID)world->physics.ode_world, 0, 0, -gravity * physics_ode_world_gravitymod.value);
			// set the tolerance for closeness of objects
			dWorldSetContactSurfaceLayer((dWorldID)world->physics.ode_world, max(0, physics_ode_contactsurfacelayer.value));
			// run collisions for the current world state, creating JointGroup
			tdelta3 = Sys_DirtyTime();
			dSpaceCollide((dSpaceID)world->physics.ode_space, (void *)world, nearCallback);
			collisiontime += (Sys_DirtyTime() - tdelta3)*10000;
			// apply forces
			if (prog)
			{
				int j;
				for (j = 0, ed = prog->edicts + j;j < prog->num_edicts;j++, ed++)
					if (!prog->edicts[j].free)
						World_Physics_Frame_ForceFromEntity(world, ed);
			}
			// run physics (move objects, calculate new velocities)
			// be sure not to pass 0 as step time because that causes an ODE error
			dWorldSetQuickStepNumIterations((dWorldID)world->physics.ode_world, bound(1, physics_ode_worldstep_iterations.integer, 200));
			if (world->physics.ode_step > 0)
				dWorldQuickStep((dWorldID)world->physics.ode_world, world->physics.ode_step);
			// clear the JointGroup now that we're done with it
			dJointGroupEmpty((dJointGroupID)world->physics.ode_contactgroup);
		}
		simulationtime = (Sys_DirtyTime() - tdelta2)*10000;

		// copy physics properties from physics engine to entities and do some stats
		if (prog)
		{
			for (i = 1, ed = prog->edicts + i;i < prog->num_edicts;i++, ed++)
				if (!prog->edicts[i].free)
					World_Physics_Frame_BodyToEntity(world, ed);

			// print stats
			if (physics_ode_printstats.integer)
			{
				dBodyID body;

				world->physics.ode_numobjects = 0;
				world->physics.ode_activeovjects = 0;
				for (i = 1, ed = prog->edicts + i;i < prog->num_edicts;i++, ed++)
				{
					if (prog->edicts[i].free)
						continue;
					body = (dBodyID)prog->edicts[i].priv.server->ode_body;
					if (!body)
						continue;
					world->physics.ode_numobjects++;
					if (dBodyIsEnabled(body))
						world->physics.ode_activeovjects++;
				}
				Con_Printf("ODE Stats(%s): %i iterations, %3.01f (%3.01f collision) %3.01f total : %i objects %i active %i disabled\n", prog->name, world->physics.ode_iterations, simulationtime, collisiontime, (Sys_DirtyTime() - tdelta)*10000, world->physics.ode_numobjects, world->physics.ode_activeovjects, (world->physics.ode_numobjects - world->physics.ode_activeovjects));
			}
		}
	}
}
#endif
