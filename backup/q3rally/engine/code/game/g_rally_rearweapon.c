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

//static	float	s_quadFactor;
//static	vec3_t	forward, right, up;
//static	vec3_t	muzzle;

extern vec3_t	forward, right, up;

/*
======================================================================

REAR WEAPON - Smoke

======================================================================
*/
void SmokerThink(gentity_t *ent){
	gentity_t	*owner;
	vec3_t		angles;

	ent->nextthink = level.time + 100;

	if (ent->freeAfterTime < level.time){
		ent->think = G_FreeEntity;
		return;
	}

	owner = &g_entities[ent->r.ownerNum];

	VectorCopy(owner->client->ps.viewangles, angles);
	angles[PITCH] = 0;
	AngleVectors (angles, forward, NULL, NULL);
	VectorMA( owner->r.currentOrigin, -40, forward, ent->r.currentOrigin );

	CreateSmokeHazard(owner, ent->r.currentOrigin);
}

void RFWeapon_SmokeFire( gentity_t *ent ) {
	trace_t		tr;
	vec3_t		end;
	gentity_t	*tent;

	VectorMA( ent->r.currentOrigin, -40, forward, end );

	trap_Trace( &tr, ent->r.currentOrigin, NULL, NULL, end, ent->s.number, MASK_SHOT );

	tent = G_TempRallyEntity( tr.endpos, EV_HAZARD );
	tent->s.eventParm = 0;
	tent->s.weapon = HT_SMOKE;
	tent->r.ownerNum = ent->s.number;
	tent->parent = ent;
	tent->think = SmokerThink;
	tent->nextthink = level.time;
	tent->freeAfterTime = level.time + 1000;
}


/*
======================================================================

REAR WEAPON - Oil

======================================================================
*/

void RFWeapon_OilFire( gentity_t *ent ) {
	trace_t		tr;
	vec3_t		end;

	VectorMA( ent->r.currentOrigin, -80, forward, end );

	trap_Trace( &tr, ent->r.currentOrigin, NULL, NULL, end, ent->s.number, MASK_SHOT );

	CreateOilHazard(ent, tr.endpos);
}


/*
======================================================================

REAR WEAPON - Mine

======================================================================
*/

void RFWeapon_MineFire( gentity_t *ent ) {
	trace_t		tr;
	vec3_t		end, back;
	gentity_t	*tent;

	VectorMA( ent->r.currentOrigin, -64, forward, end );

	trap_Trace( &tr, ent->r.currentOrigin, NULL, NULL, end, ent->s.number, MASK_SHOT );

	VectorCopy(forward, back);
	VectorInverse(back);

	tent = fire_mine(ent, tr.endpos, back);
	tent->s.otherEntityNum = ent->s.number;

	VectorSet(tent->r.mins, -4, -4, 0);
	VectorSet(tent->r.maxs, 4, 4, 4);

	trap_LinkEntity (tent);
}

/*
======================================================================

REAR WEAPON - Flame

======================================================================
*/
void FlameThink(gentity_t *ent){
	gentity_t	*owner;
	trace_t		tr;
	vec3_t		dest;
	vec3_t		angles;

	ent->nextthink = level.time + 200;

	if (ent->freeAfterTime < level.time){
		ent->think = G_FreeEntity;
		return;
	}

	owner = &g_entities[ent->r.ownerNum];

	VectorCopy(owner->client->ps.viewangles, angles);
	angles[PITCH] = 0;
	AngleVectors (angles, forward, NULL, NULL);
	VectorMA( owner->r.currentOrigin, -80, forward, dest );

	trap_Trace( &tr, owner->r.currentOrigin, NULL, NULL, dest, ent->s.number, MASK_SOLID );
	VectorMA(tr.endpos, 1, tr.plane.normal, dest);
	// allow to ride movers
	ent->s.groundEntityNum = tr.entityNum;
	G_SetOrigin( ent, dest );

	CreateFireHazard(owner, ent->r.currentOrigin);
}

void RFWeapon_FlameFire( gentity_t *ent ) {
	trace_t		tr;
	vec3_t		end;
	gentity_t	*tent;

	VectorMA( ent->r.currentOrigin, -80, forward, end );

	trap_Trace( &tr, ent->r.currentOrigin, NULL, NULL, end, ent->s.number, MASK_SHOT );

	tent = G_Spawn();
	G_SetOrigin(tent, tr.endpos);
	tent->r.ownerNum = ent->s.number;
	tent->parent = ent;
	tent->think = FlameThink;
	tent->nextthink = level.time;
	tent->freeAfterTime = level.time + 1000;

//	CreateFireHazard(tent);
}


/*
======================================================================

REAR WEAPON - Bio

======================================================================
*/

void RFWeapon_BioFire( gentity_t *ent ) {
	trace_t		tr;
	vec3_t		end;

	VectorMA( ent->r.currentOrigin, -80, forward, end );

	trap_Trace( &tr, ent->r.currentOrigin, NULL, NULL, end, ent->s.number, MASK_SHOT );

	CreateBioHazard(ent, tr.endpos);
}


/*
===============
FireRearWeapon
===============
*/
void FireRearWeapon( gentity_t *ent ) {
	vec3_t		angles;
	int			weapon, i;
/*
==============================================
Quadfactor-Test 16/03/2020
==============================================
	if (ent->client->ps.powerups[PW_QUAD] ) {
		G_AddEvent( ent, EV_POWERUP_QUAD, 0 );
		s_quadFactor = g_quadfactor.value;
	} else {
		s_quadFactor = 1;
	}
==============================================
Quadfactor-Test 16/03/2020   End
==============================================
*/

	// set aiming directions
	AngleVectors (ent->client->ps.viewangles, forward, right, up);

	for (i = RWP_SMOKE; i < WP_NUM_WEAPONS; i++){
		if (ent->client->ps.stats[STAT_WEAPONS] & ( 1 << i ) && !ent->client->ps.ammo[ i ]){
			ent->client->ps.stats[STAT_WEAPONS] &= ~( 1 << i );
		}

		if (ent->client->ps.stats[STAT_WEAPONS] & ( 1 << i ) && ent->client->ps.ammo[ i ])
			break;
	}

	if (i < WP_NUM_WEAPONS){
		weapon = i;
	}
	else {
		return; // no weapon
	}

	// track shots taken for accuracy tracking.  Grapple is not a weapon and gauntet is just not tracked
	/*
	if( ent->s.weapon != WP_GRAPPLING_HOOK && ent->s.weapon != WP_GAUNTLET ) {
		ent->client->ps.persistant[PERS_ACCURACY_SHOTS]++;
	}
	*/

	// set aiming directions
	VectorCopy(ent->client->ps.viewangles, angles);
	angles[PITCH] = 0;

	AngleVectors (angles, forward, right, up);

	// fire the specific weapon
	switch( weapon ) {
	case RWP_SMOKE:
		RFWeapon_SmokeFire( ent );
		break;
	case RWP_OIL:
		RFWeapon_OilFire( ent );
		break;
	case RWP_FLAME:
		RFWeapon_FlameFire( ent );
		break;
	case RWP_BIO:
		RFWeapon_BioFire( ent );
		break;
	case RWP_MINE:
		RFWeapon_MineFire( ent );
		break;
	default:
		// FIXME: print no ammo message or something
		break;
	}
}
