#ifndef PHYS_H
#define PHYS_H

#include "quakedef.h"


/*! move an entity that is stuck out of the surface it is stuck in (can move large amounts)
 * returns 1 if it found a better place, 0 if it remains stuck, -1 if it wasn't stuck.
 */
int PHYS_NudgeOutOfSolid(prvm_prog_t *prog, prvm_edict_t *ent);
extern cvar_t cl_gameplayfix_nudgeoutofsolid_separation;


#endif // PHYS_H guard
