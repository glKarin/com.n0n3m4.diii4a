#ifndef PHYS_H
#define PHYS_H

#include "quakedef.h"


qbool PHYS_TestEntityPosition (prvm_prog_t *prog, prvm_edict_t *ent, vec3_t offset);

typedef enum unstickresult_e
{
	// matching the DP_QC_NUDGEOUTOFSOLID return values
	UNSTICK_STUCK = 0,
	UNSTICK_GOOD = -1, ///< didn't need to be unstuck
	UNSTICK_UNSTUCK = 1
}
unstickresult_t;
/*! move an entity that is stuck by small amounts in various directions to try to nudge it back into the collision hull
 * returns 1 if it found a better place, 0 if it remains stuck, -1 if it wasn't stuck.
 * Replaces SV_TryUnstick() and SV_CheckStuck() which in Quake applied to players only.
 */
unstickresult_t PHYS_UnstickEntityReturnOffset (prvm_prog_t *prog, prvm_edict_t *ent, vec3_t offset);
/*! move an entity that is stuck out of the surface it is stuck in (can move large amounts)
 * with consideration to the properties of the surface and support for multiple surfaces.
 * returns 1 if it found a better place, 0 if it remains stuck, -1 if it wasn't stuck.
 * Replaces PHYS_UnstickEntityReturnOffset() but falls back to it when using cliphulls.
 */
unstickresult_t PHYS_NudgeOutOfSolid(prvm_prog_t *prog, prvm_edict_t *ent);
extern cvar_t cl_gameplayfix_nudgeoutofsolid_separation;


#endif // PHYS_H guard
