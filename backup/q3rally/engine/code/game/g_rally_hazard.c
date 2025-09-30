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

#define SPLASH_RADIUS_SCALE		16.0f
#define MAX_SPLASH_RADIUS		14.0f



qboolean G_FrictionCalc( const carPoint_t *point, float *sCOF, float *kCOF )
{
	gentity_t	*ent;
	int			entityList[MAX_GENTITIES];
	int			numListedEntities;
	vec3_t		mins, maxs;
	float		radius;
	int			i;

	radius = point->radius + SPLASH_RADIUS_SCALE * MAX_SPLASH_RADIUS;

	for ( i = 0 ; i < 3 ; i++ ) {
		mins[i] = point->r[i] - radius;
		maxs[i] = point->r[i] + radius;
	}

	numListedEntities = trap_EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

	for ( i = 0 ; i < numListedEntities ; i++ ) {
		ent = &g_entities[entityList[ i ]];

		if( ent->s.eType != ET_EVENTS + EV_HAZARD ) continue;
		if( ent->s.weapon != HT_OIL ) continue;

		radius = ( ent->splashRadius * SPLASH_RADIUS_SCALE ) + point->radius;
		radius *= radius;
		if( DistanceSquared( ent->r.currentOrigin, point->r ) > radius ) continue;

		*sCOF = CP_OIL_SCOF;
		*kCOF = CP_OIL_KCOF;

		return qtrue;
	}

	return qfalse;
}


/*
============
FireHazard_Think

============
*/

void FireHazard_Think (gentity_t *self ){
	self->nextthink = level.time + 200;

	if (self->freeAfterTime < level.time){
		self->think = G_FreeEntity;
		return;
	}

	G_RadiusDamage_NoKnockBack(self->r.currentOrigin, self, self->damage, SPLASH_RADIUS_SCALE * self->splashRadius, self, MOD_FIRE);
}

/*
============
CreateFireHazard

  owner - the entity that caused this hazard.  If the hazard is from a weapon
    the owner is the player who fired the weapon.

============
*/

void CreateFireHazard (gentity_t *owner, vec3_t origin){
	gentity_t		*ent;
	gentity_t		*other;
	vec3_t			dist;

	other = NULL;
	while ((other = G_Find(other, FOFS(classname), "fire")) != NULL){
		VectorSubtract(other->r.currentOrigin, origin, dist);
		if (VectorLength(dist) < 16){
//			G_FreeEntity(other);
			other->freeAfterTime = level.time + 10000;
			return;
		}
	}

	ent = G_TempRallyEntity( origin, EV_HAZARD );

	ent->splashRadius = 1;

	ent->s.weapon = HT_FIRE;
	ent->s.otherEntityNum = owner->s.number;
	ent->parent = owner;
	ent->classname = "fire";
	ent->think = FireHazard_Think;
	ent->damage = 15;
	ent->nextthink = level.time;
	ent->freeAfterTime = level.time + 10000;
}

/*
============
BioHazard_Think

============
*/

void BioHazard_Think (gentity_t *self ){
	self->nextthink = level.time + 200;

	if (self->freeAfterTime < level.time){
		self->think = G_FreeEntity;
		return;
	}

	G_RadiusDamage_NoKnockBack(self->r.currentOrigin, self, self->damage, 0.75f * SPLASH_RADIUS_SCALE * self->splashRadius, self, MOD_BIOHAZARD);
}

/*
============
CreateBioHazard

  owner - the entity that caused this hazard.  If the hazard is from a weapon
    the owner is the player who fired the weapon.

============
*/

void CreateBioHazard (gentity_t *owner, vec3_t origin){
	gentity_t		*ent;
	gentity_t		*other;
	vec3_t			dist;
	int				highest;

	other = NULL;
	highest = 0;
	while ((other = G_Find(other, FOFS(classname), "radiation")) != NULL){
		VectorSubtract(other->r.currentOrigin, origin, dist);
		if (VectorLength(dist) < other->splashRadius * 8){
			if (other->splashRadius > highest)
				highest = other->splashRadius;

			G_FreeEntity(other);
		}
	}

	ent = G_TempRallyEntity( origin, EV_HAZARD );

	if (highest){
		ent->splashRadius = highest + 1;

		if (ent->splashRadius > MAX_SPLASH_RADIUS)
			ent->splashRadius = MAX_SPLASH_RADIUS;
	}
	else {
		ent->splashRadius = 4;
	}

	ent->s.eventParm = ent->splashRadius;

	ent->s.weapon = HT_BIO;
	ent->s.otherEntityNum = owner->s.number;
	ent->parent = owner;
	ent->classname = "radiation";
	ent->think = BioHazard_Think;
	ent->damage = 10;
	ent->nextthink = level.time;
	ent->freeAfterTime = level.time + 10000;
}

/*
============
CheckForOil

============
*/

void CheckForOil(vec3_t origin, float radius){
	gentity_t		*other;
	vec3_t			dist;
	vec3_t			newOrigin;
	float			length;

	other = NULL;
	while ((other = G_Find(other, FOFS(classname), "oil")) != NULL){
		if (other->count > 5) continue;

		VectorSubtract(origin, other->r.currentOrigin, dist);
		length = VectorLength(dist);
		if (length < radius + MAX_SPLASH_RADIUS * SPLASH_RADIUS_SCALE){
			if (length < other->splashRadius * SPLASH_RADIUS_SCALE){
				CreateFireHazard(other, origin);
				other->count++;
			}
			else if (length < radius + other->splashRadius * SPLASH_RADIUS_SCALE){
				VectorMA(other->r.currentOrigin, length / (radius + other->splashRadius * SPLASH_RADIUS_SCALE), dist, newOrigin);
				CreateFireHazard(other, newOrigin);
				other->count++;
			}
		}
	}
}

/*
============
OilHazard_Think

============
*/

void OilHazard_Think (gentity_t *self ){
	self->nextthink = level.time + 200;

	if (self->freeAfterTime < level.time){
		self->think = G_FreeEntity;
		return;
	}
}

/*
============
CreateOilHazard

  owner - the entity that caused this hazard.  If the hazard is from a weapon
    the owner is the player who fired the weapon.

============
*/

void CreateOilHazard (gentity_t *owner, vec3_t origin){
	gentity_t		*ent;
	gentity_t		*other;
	vec3_t			dist;
	int				highest;

	other = NULL;
	highest = 0;

	ent = G_TempRallyEntity( origin, EV_HAZARD );

	while ((other = G_Find(other, FOFS(classname), "oil")) != NULL){
		VectorSubtract(other->r.currentOrigin, origin, dist);
		if (VectorLength(dist) < other->splashRadius * 8){
			if (other->splashRadius > highest){
				highest = other->splashRadius;
//				ent->count = other->count;
			}

			G_FreeEntity(other);
		}
	}

	if (highest){
		ent->splashRadius = highest + 1;

		if (ent->splashRadius > MAX_SPLASH_RADIUS)
			ent->splashRadius = MAX_SPLASH_RADIUS;
	}
	else {
		ent->splashRadius = 4;
	}

	ent->s.eventParm = ent->splashRadius;

	ent->s.weapon = HT_OIL;
	ent->s.otherEntityNum = owner->s.number;
	ent->parent = owner;
	ent->classname = "oil";
	ent->think = OilHazard_Think;
	ent->nextthink = level.time;
	ent->freeAfterTime = level.time + 10000;
}

/*
============
PoisonHazard_Think

============
*/

void PoisonHazard_Think (gentity_t *self ){
	self->nextthink = level.time + 200;

	if (self->freeAfterTime < level.time){
		self->think = G_FreeEntity;
		return;
	}

	G_RadiusDamage_NoKnockBack(self->r.currentOrigin, self, self->damage, SPLASH_RADIUS_SCALE * self->splashRadius, self, MOD_POISON);
}

/*
============
CreatePoisonHazard

  owner - the entity that caused this hazard.  If the hazard is from a weapon
    the owner is the player who fired the weapon.

  used with rear weapons

============
*/

void CreatePoisonHazard (gentity_t *owner, vec3_t origin){
	gentity_t		*ent;

	ent = G_TempRallyEntity( origin, EV_HAZARD );

	ent->splashRadius = 3;
	ent->s.weapon = HT_POISON;
	ent->s.otherEntityNum = owner->s.number;
	ent->parent = owner;
	ent->classname = "poison";
	ent->think = PoisonHazard_Think;
	ent->damage = 2;
	ent->nextthink = level.time;
	ent->freeAfterTime = level.time + 10000;
}

/*
============
CreatePoisonCloudHazard

  owner - the entity that caused this hazard.  If the hazard is from a weapon
    the owner is the player who fired the weapon.

  used with barrels

============
*/

void CreatePoisonCloudHazard (gentity_t *owner, vec3_t origin) {
	gentity_t		*ent;

	ent = G_TempEntity( origin, EV_HAZARD );
	ent->s.weapon = HT_POISON;
}

/*
============
CreateSmokeHazard

  owner - the entity that caused this hazard.  If the hazard is from a weapon
    the owner is the player who fired the weapon.

  used with barrels

============
*/

void CreateSmokeHazard (gentity_t *owner, vec3_t origin) {
	gentity_t		*ent;

	ent = G_TempEntity( origin, EV_HAZARD );
	ent->s.weapon = HT_SMOKE;
}
