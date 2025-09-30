/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2002-2021 Q3Rally Team (Per Thormann - q3rally@gmail.com)

This file is part of q3rally source code.

q3rally source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

q3rally source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with q3rally; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "g_local.h"

/*
static int EMPTY_BARREL_MASS = 50;
static int FULL_BARREL_MASS = 200;
static int BARREL_LEAK_TIME = 60000;

void Touch_Barrel (gentity_t *self, gentity_t *other, trace_t *trace );
*/
void DropToFloor( gentity_t *self ){
	trace_t		tr;
	vec3_t		dest;

	// drop to floor
	VectorSet( dest, self->s.origin[0], self->s.origin[1], self->s.origin[2] - 4096 );
	trap_Trace( &tr, self->s.origin, self->r.mins, self->r.maxs, dest, self->s.number, MASK_PLAYERSOLID );

	VectorCopy(tr.endpos, dest);

	// allow to ride movers
	self->s.groundEntityNum = tr.entityNum;
	G_SetOrigin( self, dest );
}


/*
void Think_Barrel_Poison (gentity_t *self ){
	self->nextthink = level.time + 200;

	if (self->leaktime && self->leaktime > level.time){
		CreatePoisonCloudHazard(self, self->r.currentOrigin);

		G_RadiusDamage_NoKnockBack(self->r.currentOrigin, self, self->damage, 208, self, MOD_POISON);

//		self->mass = EMPTY_BARREL_MASS + ((float)(FULL_BARREL_MASS - EMPTY_BARREL_MASS) * (1 - (level.time - self->leaktime) / (float)BARREL_LEAK_TIME));
	}
}
*/

/*
void Barrel_Damage (gentity_t *self, vec3_t kick ){
	vec3_t		vel;

	if (self->leaktime < level.time){
		switch(self->s.weapon){
		case HT_OIL:
			CreateOilHazard(self, self->r.currentOrigin);
			self->leaktime = level.time + 5000;
			break;

		case HT_BIO:
			CreateBioHazard(self, self->r.currentOrigin);
			self->leaktime = level.time + 5000;
			break;

		case HT_POISON:
			self->leaktime = level.time + 5000;
			break;

		case HT_EXPLOSIVE:
			CreateFireHazard(self, self->r.currentOrigin);
			self->leaktime = level.time + 2000;
			break;
		}
	}

	BG_EvaluateTrajectoryDelta(&self->s.pos, level.time, vel);

	BG_EvaluateTrajectory(&self->s.pos, level.time, self->s.pos.trBase);
	VectorCopy(self->s.pos.trBase, self->r.currentOrigin);

	VectorAdd (vel, kick, self->s.pos.trDelta);

	self->s.pos.trTime = level.time;
	self->s.pos.trType = TR_GRAVITY;
}
*/

/*
void Touch_Barrel (gentity_t *self, gentity_t *other, trace_t *trace ){
	vec3_t		force, torque, r, start, dest;
	int			damage;
	float		dp;
	trace_t		tr;

	if ( other->client ) {
		return;
	}

// STONELANCE
	if (self->leaktime < level.time){
		switch(self->s.weapon){
		case HT_OIL:
			CreateOilHazard(self, self->r.currentOrigin);
			self->leaktime = level.time + 5000;
			break;

		case HT_BIO:
			CreateBioHazard(self, self->r.currentOrigin);
			self->leaktime = level.time + 5000;
			break;

		case HT_POISON:
			self->leaktime = level.time + 5000;
			break;

		case HT_EXPLOSIVE:
			CreateFireHazard(self, self->r.currentOrigin);
			self->leaktime = level.time + 2000;
			break;
		}
	}
// END

//	Com_Printf("barrel touched by %s\n", other->classname);
//	Com_Printf("barrel located at %s\n", vtos(self->r.currentOrigin));
//	Com_Printf("trace ended at %s\n", vtos(trace->endpos));
//	Com_Printf("other ent located at %s\n", vtos(other->r.currentOrigin));

	damage = other->damage;
	if (damage <= 0)
		damage = 100;

//	VectorSubtract(self->r.currentOrigin, trace->endpos, force);

//	Com_Printf("collision location to object cm %s\n", vtos(force));

//	VectorNormalize(force);
//	VectorScale(force, damage, force);

	VectorNormalize2(other->s.pos.trDelta, force);

	// assume barrel not moving
	//VectorScale(self->s.pos.trDelta, self->mass, mv1);
	VectorScale(other->s.pos.trDelta, other->mass / (float)(self->mass + other->mass), force);

	self->s.pos.trType = TR_GRAVITY;
	VectorCopy(self->r.currentOrigin, self->s.pos.trBase);
	//VectorAdd(self->s.pos.trDelta, force, self->s.pos.trDelta);
	VectorScale(force, 10, force);
	VectorCopy(force, self->s.pos.trDelta);
	self->s.pos.trTime = level.time;

//	Com_Printf("force %s\n", vtos(force));

	VectorCopy(self->r.currentOrigin, start);
	start[2] += self->r.mins[2] / 1.1;
	BG_EvaluateTrajectory(&self->s.pos, level.time + 100, dest);
	dest[2] += self->r.mins[2] / 1.1;
	//trap_Trace( &tr, self->r.currentOrigin, self->r.mins, self->r.maxs, dest, self->s.number, MASK_SOLID );
	trap_Trace( &tr, start, NULL, NULL, dest, self->s.number, MASK_PLAYERSOLID );
	if (tr.fraction < 1){
		// If startsolid normal will be 0
//		Com_Printf("normal %f, %f, %f\n", tr.plane.normal[0], tr.plane.normal[1], tr.plane.normal[2]);

		dp = DotProduct(tr.plane.normal, force);
//		Com_Printf("dp %f\n", dp);
		if (dp < 0){
			VectorMA(force, -dp, tr.plane.normal, force);
			VectorCopy(force, self->s.pos.trDelta);
		}

		if (self->s.pos.trDelta[2] < 40)
			self->s.pos.trDelta[2] += 40;
	}

//	Com_Printf("barrel velocity after collision %s\n", vtos(self->s.pos.trDelta));
	// to overcome gravity
	SnapVector(self->s.pos.trDelta);

	// torque
	VectorSubtract(trace->endpos, self->r.currentOrigin, r);
	CrossProduct(r, other->s.pos.trDelta, torque);

	VectorScale(torque, 1.0F / (self->mass), self->s.apos.trDelta);

//Com_Printf("barrel rotates at %f %f %f\n", self->s.apos.trDelta[0], self->s.apos.trDelta[1], self->s.apos.trDelta[2]);

	self->s.apos.trDelta[YAW] = 0;
	self->s.apos.trDelta[ROLL] = 0;

	VectorCopy(self->r.currentAngles, self->s.apos.trBase);
	self->s.apos.trTime = level.time;
	self->s.apos.trType = TR_LINEAR;

//	Com_Printf("barrel feels a force of %s\n", vtos(force));
//	Com_Printf("barrel rotates at %f %f %f\n", self->s.apos.trBase[0], self->s.apos.trBase[1], self->s.apos.trBase[2]);
//	Com_Printf("barrel is on %f\n", self->s.groundEntityNum);

	trap_LinkEntity (self);
}
*/

void Barrel_Die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod ){
	self->health = 1000;
}

// *********************** Harmful Barrels ************************

//
// Biohazard Barrels
//
void SP_rally_misc_barrelbio( gentity_t *ent ){
	ent->die = Barrel_Die;

	ent->s.modelindex = G_ModelIndex( "models/mapobjects/barrels/b_bio.md3" );
	ent->classname = "barrel";
	VectorSet(ent->r.mins, -8, -8, -13);
	VectorSet(ent->r.maxs, 8, 8, 13);
	ent->s.eType = ET_GENERAL;
	ent->clipmask = MASK_PLAYERSOLID;
//	ent->r.contents = CONTENTS_BODY;

//	ent->mass = FULL_BARREL_MASS;
//	ent->coRest = 0.4;

//	ent->objectType = OT_BARREL;
//	ent->s.weapon = HT_BIO;

//	ent->health = 1000;
//	ent->takedamage = qtrue;

	ent->s.origin[2] += 1;
	DropToFloor(ent);

	trap_LinkEntity (ent);
}

//
// Explosive Barrel
//
void SP_rally_misc_barrelexp( gentity_t *ent ){
	ent->die = Barrel_Die;

	ent->s.modelindex = G_ModelIndex( "models/mapobjects/barrels/b_exp.md3" );
	ent->classname = "barrel";
	VectorSet(ent->r.mins, -8, -8, -13);
	VectorSet(ent->r.maxs, 8, 8, 13);
	ent->s.eType = ET_GENERAL;
	ent->clipmask = MASK_PLAYERSOLID;
//	ent->r.contents = CONTENTS_BODY;

//	ent->mass = FULL_BARREL_MASS;
//	ent->coRest = 0.4;

//	ent->objectType = OT_BARREL;
//	ent->s.weapon = HT_EXPLOSIVE;

//	ent->health = 1000;
//	ent->damage = 60;
//	ent->takedamage = qtrue;

	ent->s.origin[2] += 1;
	DropToFloor(ent);

	trap_LinkEntity (ent);
}

//
// Poison Barrel
//
void SP_rally_misc_barrelpoison( gentity_t *ent ){
	ent->die = Barrel_Die;
//	ent->think = Think_Barrel_Poison;
//	ent->nextthink = level.time + FRAMETIME;

	ent->s.modelindex = G_ModelIndex( "models/mapobjects/barrels/b_psn.md3" );
	ent->classname = "barrel";
	VectorSet(ent->r.mins, -8, -8, -13);
	VectorSet(ent->r.maxs, 8, 8, 13);
	ent->s.eType = ET_GENERAL;
	ent->clipmask = MASK_PLAYERSOLID;
//	ent->r.contents = CONTENTS_BODY;

//	ent->mass = FULL_BARREL_MASS;
//	ent->coRest = 0.4;

//	ent->objectType = OT_BARREL;
//	ent->s.weapon = HT_POISON;

//	ent->health = 1000;
//	ent->damage = 5;
//	ent->takedamage = qtrue;

	ent->s.origin[2] += 1;
	DropToFloor(ent);

	trap_LinkEntity (ent);
}

//
// Oil Barrel
//
void SP_rally_misc_barreloil( gentity_t *ent ){
	ent->die = Barrel_Die;

	ent->s.modelindex = G_ModelIndex( "models/mapobjects/barrels/b_oil.md3" );
	ent->classname = "barrel";
	VectorSet(ent->r.mins, -8, -8, -13);
	VectorSet(ent->r.maxs, 8, 8, 13);
	ent->s.eType = ET_GENERAL;
	ent->clipmask = MASK_PLAYERSOLID;
//	ent->r.contents = CONTENTS_BODY;

//	ent->mass = FULL_BARREL_MASS;
//	ent->coRest = 0.4;

//	ent->objectType = OT_BARREL;
//	ent->s.weapon = HT_OIL;

//	ent->health = 1000;
//	ent->takedamage = qtrue;

	ent->s.origin[2] += 1;
	DropToFloor(ent);

	trap_LinkEntity (ent);
}
