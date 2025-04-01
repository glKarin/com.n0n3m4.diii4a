/*
===========================================================================

Return to Castle Wolfenstein single player GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Return to Castle Wolfenstein single player GPL Source Code (RTCW SP Source Code).  

RTCW SP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW SP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW SP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW SP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW SP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

/*
 * name:		g_weapon.c
 *
 * desc:		perform the server side effects of a weapon firing
 *
*/


#include "g_local.h"

static float s_quadFactor;
static vec3_t forward, right, up;
static vec3_t muzzleEffect;
vec3_t muzzleTrace;


// forward dec
void weapon_zombiespit( gentity_t *ent );

void Bullet_Fire( gentity_t *ent, float spread, int damage );
qboolean Bullet_Fire_Extended( gentity_t *source, gentity_t *attacker, vec3_t start, vec3_t end, float spread, int damage, int recursion );

int G_GetWeaponDamage( int weapon, qboolean player ); // JPW

/*
======================================================================

KNIFE/GAUNTLET (NOTE: gauntlet is now the Zombie melee)

======================================================================
*/

#define KNIFE_DIST 48

/*
==============
Weapon_Knife
==============
*/
void Weapon_Knife( gentity_t *ent ) {
	trace_t tr;
	gentity_t   *traceEnt, *tent;
	int damage, mod;

	vec3_t end;
	qboolean	isPlayer = (ent->client && !ent->aiCharacter);	// Knightmare added

	mod = MOD_KNIFE;

	AngleVectors( ent->client->ps.viewangles, forward, right, up );
	CalcMuzzlePoint( ent, ent->s.weapon, forward, right, up, muzzleTrace );
	VectorMA( muzzleTrace, KNIFE_DIST, forward, end );
	trap_Trace( &tr, muzzleTrace, NULL, NULL, end, ent->s.number, MASK_SHOT );

	if ( tr.surfaceFlags & SURF_NOIMPACT ) {
		return;
	}

	// no contact
	if ( tr.fraction == 1.0f ) {
		return;
	}

	if ( tr.entityNum >= MAX_CLIENTS ) {   // world brush or non-player entity (no blood)
		tent = G_TempEntity( tr.endpos, EV_MISSILE_MISS );
	} else {                            // other player
		tent = G_TempEntity( tr.endpos, EV_MISSILE_HIT );
	}

	tent->s.otherEntityNum = tr.entityNum;
	tent->s.eventParm = DirToByte( tr.plane.normal );
	tent->s.weapon = ent->s.weapon;

	if ( tr.entityNum == ENTITYNUM_WORLD ) { // don't worry about doing any damage
		return;
	}

	traceEnt = &g_entities[ tr.entityNum ];

	if ( !( traceEnt->takedamage ) ) {
		return;
	}

	// RF, no knife damage for big guys
	switch ( traceEnt->aiCharacter ) {
	case AICHAR_SUPERSOLDIER:
	case AICHAR_SUPERSOLDIER_LAB:
	case AICHAR_HEINRICH:
		return;
	}

	damage = G_GetWeaponDamage( ent->s.weapon, isPlayer ); // JPW		// default knife damage for frontal attacks

    if ( g_gametype.integer == GT_GOTHIC ) { 
	switch ( traceEnt->aiCharacter ) {
	case AICHAR_ZOMBIE:
	case AICHAR_WARZOMBIE:
	case AICHAR_LOPER:
		damage *= 0.3;
	default:
	    damage *= 1.0;
	}
	}


	if ( traceEnt->client ) {
		if (G_GetEnemyPosition(ent, traceEnt) == POSITION_BEHIND) 
		{
			damage = 100;       // enough to drop a 'normal' (100 health) human with one jab
			mod = MOD_KNIFE_STEALTH;
		}
	}

	G_Damage( traceEnt, ent, ent, vec3_origin, tr.endpos, ( damage + rand() % 5 ) * s_quadFactor, 0, mod );
}


/*
==============
Weapon_Dagger
==============
*/
void Weapon_Dagger( gentity_t *ent ) {
	trace_t tr;
	gentity_t   *traceEnt, *tent;
	int damage, mod;

	vec3_t end;
	qboolean	isPlayer = (ent->client && !ent->aiCharacter);	// Knightmare added

	mod =  MOD_DAGGER;

	AngleVectors( ent->client->ps.viewangles, forward, right, up );
	CalcMuzzlePoint( ent, ent->s.weapon, forward, right, up, muzzleTrace );
	VectorMA( muzzleTrace, KNIFE_DIST, forward, end );
	trap_Trace( &tr, muzzleTrace, NULL, NULL, end, ent->s.number, MASK_SHOT );

	if ( tr.surfaceFlags & SURF_NOIMPACT ) {
		return;
	}

	// no contact
	if ( tr.fraction == 1.0f ) {
		return;
	}

	if ( tr.entityNum >= MAX_CLIENTS ) {   // world brush or non-player entity (no blood)
		tent = G_TempEntity( tr.endpos, EV_MISSILE_MISS );
	} else {                            // other player
		tent = G_TempEntity( tr.endpos, EV_MISSILE_HIT );
	}

	tent->s.otherEntityNum = tr.entityNum;
	tent->s.eventParm = DirToByte( tr.plane.normal );
	tent->s.weapon = ent->s.weapon;

	if ( tr.entityNum == ENTITYNUM_WORLD ) { // don't worry about doing any damage
		return;
	}

	traceEnt = &g_entities[ tr.entityNum ];

	if ( !( traceEnt->takedamage ) ) {
		return;
	}

	// RF, no knife damage for big guys
	switch ( traceEnt->aiCharacter ) {
	case AICHAR_HEINRICH:
		return;
	}

	damage = G_GetWeaponDamage( ent->s.weapon, isPlayer ); // JPW		// default knife damage for frontal attacks

	if ( traceEnt->client ) {
		if (G_GetEnemyPosition(ent, traceEnt) == POSITION_BEHIND)  
		{
			damage = 100;       // enough to drop a 'normal' (100 health) human with one jab
			mod = MOD_DAGGER_STEALTH;
		}
	}

	G_Damage( traceEnt, ent, ent, vec3_origin, tr.endpos, ( damage + rand() % 5 ) * s_quadFactor, 0, mod );
}

// JPW NERVE -- launch airstrike as line of bombs mostly-perpendicular to line of grenade travel
// (close air support should *always* drop parallel to friendly lines, tho accidents do happen)
void G_ExplodeMissile( gentity_t *ent );

void G_AirStrikeExplode( gentity_t *self ) {

	self->r.svFlags &= ~SVF_NOCLIENT;
	self->r.svFlags |= SVF_BROADCAST;

	self->think = G_ExplodeMissile;
	self->nextthink = level.time + 50;
}


#define NUMBOMBS 10
#define BOMBSPREAD 150
void weapon_callAirStrike( gentity_t *ent ) {
	int i;
	vec3_t bombaxis, lookaxis, pos, bomboffset, fallaxis, temp;
	gentity_t *bomb,*te;
	trace_t tr;
	float traceheight, bottomtraceheight;

	VectorCopy( ent->s.pos.trBase,bomboffset );
	bomboffset[2] += 4096;

	// cancel the airstrike if FF off and player joined spec
	if ( !g_friendlyFire.integer && ent->parent->client && ent->parent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
		ent->splashDamage = 0; // no damage
		ent->think = G_ExplodeMissile;
		ent->nextthink = level.time + crandom() * 50;
		return; // do nothing, don't hurt anyone
	}

	// turn off smoke grenade
	ent->think = G_ExplodeMissile;
	ent->nextthink = level.time + 950 + NUMBOMBS * 100 + crandom() * 50; // 3000 offset is for aircraft flyby

	trap_Trace( &tr, ent->s.pos.trBase, NULL, NULL, bomboffset, ent->s.number, MASK_SHOT );
	if ( ( tr.fraction < 1.0 ) && ( !( tr.surfaceFlags & SURF_NOIMPACT ) ) ) { //SURF_SKY)) ) { // JPW NERVE changed for trenchtoast foggie prollem
		//G_SayTo( ent->parent, ent->parent, 2, COLOR_YELLOW, "Pilot: ", "Aborting, can't see target.", qtrue );
			te = G_TempEntity( ent->parent->s.pos.trBase, EV_GLOBAL_SOUND );
			te->s.eventParm = G_SoundIndex( "sound/weapons/airstrike/a-aborting.wav" );
			te->s.teamNum = ent->parent->s.clientNum;
		return;
	}

		te = G_TempEntity( ent->parent->s.pos.trBase, EV_GLOBAL_SOUND );
		te->s.eventParm = G_SoundIndex( "sound/weapons/airstrike/a-affirmative_omw.wav" );
		te->s.teamNum = ent->parent->s.clientNum;

	VectorCopy( tr.endpos, bomboffset );
	traceheight = bomboffset[2];
	bottomtraceheight = traceheight - 8192;
	VectorSubtract( ent->s.pos.trBase,ent->parent->client->ps.origin,lookaxis );
	lookaxis[2] = 0;
	VectorNormalize( lookaxis );
	pos[0] = 0;
	pos[1] = 0;
	pos[2] = crandom(); // generate either up or down vector,
	VectorNormalize( pos ); // which adds randomness to pass direction below
	RotatePointAroundVector( bombaxis,pos,lookaxis,90 + crandom() * 30 ); // munge the axis line a bit so it's not totally perpendicular
	VectorNormalize( bombaxis );
	VectorCopy( bombaxis,pos );
	VectorScale( pos,(float)( -0.5f * BOMBSPREAD * NUMBOMBS ),pos );
	VectorAdd( ent->s.pos.trBase, pos, pos ); // first bomb position
	VectorScale( bombaxis,BOMBSPREAD,bombaxis ); // bomb drop direction offset

	for ( i = 0; i < NUMBOMBS; i++ ) {
		bomb = G_Spawn();
		bomb->nextthink = level.time + i * 100 + crandom() * 50 + 1000; // 1000 for aircraft flyby, other term for tumble stagger
		bomb->think = G_AirStrikeExplode;
		bomb->s.eType       = ET_MISSILE;
		bomb->r.svFlags     = SVF_USE_CURRENT_ORIGIN | SVF_NOCLIENT;
		bomb->s.weapon      = WP_GRENADE_PINEAPPLE; // might wanna change this
		bomb->r.ownerNum    = ent->s.number;
		bomb->parent        = ent->parent;
		bomb->damage        = 400; // maybe should un-hard-code these?
		bomb->splashDamage  = 400;
		bomb->classname             = "air strike";
		bomb->splashRadius          = 400;
		bomb->methodOfDeath         = MOD_AIRSTRIKE;
		bomb->splashMethodOfDeath   = MOD_AIRSTRIKE;
		bomb->clipmask = MASK_MISSILESHOT;
		bomb->s.pos.trType = TR_STATIONARY; // was TR_GRAVITY,  might wanna go back to this and drop from height
		//bomb->s.pos.trTime = level.time;		// move a bit on the very first frame
		bomboffset[0] = crandom() * 0.5 * BOMBSPREAD;
		bomboffset[1] = crandom() * 0.5 * BOMBSPREAD;
		bomboffset[2] = 0;
		VectorAdd( pos,bomboffset,bomb->s.pos.trBase );
		VectorCopy( bomb->s.pos.trBase,bomboffset ); // make sure bombs fall "on top of" nonuniform scenery
		bomboffset[2] = traceheight;
		VectorCopy( bomboffset, fallaxis );
		fallaxis[2] = bottomtraceheight;
		trap_Trace( &tr, bomboffset, NULL, NULL, fallaxis, ent->s.number, MASK_SHOT );
		if ( tr.fraction != 1.0 ) {
			VectorCopy( tr.endpos,bomb->s.pos.trBase );
		}

		VectorClear( bomb->s.pos.trDelta );

		// Snap origin!
		VectorCopy( bomb->s.pos.trBase, temp );
		temp[2] += 2.f;
		SnapVectorTowards( bomb->s.pos.trBase, temp );          // save net bandwidth

		VectorCopy( bomb->s.pos.trBase, bomb->r.currentOrigin );

		// move pos for next bomb
		VectorAdd( pos,bombaxis,pos );
	}
}

gentity_t *LaunchItem( gitem_t *item, vec3_t origin, vec3_t velocity );

/*
===============
CheckMeleeAttack
	using 'isTest' to return hits to world surfaces
===============
*/
trace_t *CheckMeleeAttack( gentity_t *ent, float dist, qboolean isTest ) {
	static trace_t tr;
	vec3_t end;
	gentity_t   *tent;
	gentity_t   *traceEnt;

	// set aiming directions
	AngleVectors( ent->client->ps.viewangles, forward, right, up );

	CalcMuzzlePoint( ent, WP_KNIFE, forward, right, up, muzzleTrace );

	VectorMA( muzzleTrace, dist, forward, end );

	trap_Trace( &tr, muzzleTrace, NULL, NULL, end, ent->s.number, MASK_SHOT );
	if ( tr.surfaceFlags & SURF_NOIMPACT ) {
		return NULL;
	}

	// no contact
	if ( tr.fraction == 1.0f ) {
		return NULL;
	}

	if ( ent->client->noclip ) {
		return NULL;
	}

	traceEnt = &g_entities[ tr.entityNum ];

	// send blood impact
	if ( traceEnt->takedamage && traceEnt->client ) {
		tent = G_TempEntity( tr.endpos, EV_MISSILE_HIT );
		tent->s.otherEntityNum = traceEnt->s.number;
		tent->s.eventParm = DirToByte( tr.plane.normal );
		tent->s.weapon = ent->s.weapon;
	}

//----(SA)	added
	if ( isTest ) {
		return &tr;
	}
//----(SA)

	if ( !traceEnt->takedamage ) {
		return NULL;
	}

	if ( ent->client->ps.powerups[PW_QUAD] ) {
		G_AddEvent( ent, EV_POWERUP_QUAD, 0 );
		s_quadFactor = 4;
	} else {
		s_quadFactor = 1;
	}

	return &tr;
}

#define SMOKEBOMB_GROWTIME 1000
#define SMOKEBOMB_SMOKETIME 15000
#define SMOKEBOMB_POSTSMOKETIME 2000
// xkan, 11/25/2002 - increases postsmoke time from 2000->32000, this way, the entity
// is still around while the smoke is around, so we can check if it blocks bot's vision
// Arnout: eeeeeh this is wrong. 32 seconds is way too long. Also - we shouldn't be
// rendering the grenade anymore after the smoke stops and definately not send it to the client
// xkan, 12/06/2002 - back to the old value 2000, now that it looks like smoke disappears more
// quickly

void weapon_smokeBombExplode( gentity_t *ent ) {
	int lived = 0;

	if ( !ent->grenadeExplodeTime ) {
		ent->grenadeExplodeTime = level.time;
	}

	lived = level.time - ent->grenadeExplodeTime;
	ent->nextthink = level.time + FRAMETIME;

	if ( lived < SMOKEBOMB_GROWTIME ) {
		// Just been thrown, increase radius
		ent->s.effect1Time = 16 + lived * ( ( 640.f - 16.f ) / (float)SMOKEBOMB_GROWTIME );
	} else if ( lived < SMOKEBOMB_SMOKETIME + SMOKEBOMB_GROWTIME ) {
		// Smoking
		ent->s.effect1Time = 640;
	} else if ( lived < SMOKEBOMB_SMOKETIME + SMOKEBOMB_GROWTIME + SMOKEBOMB_POSTSMOKETIME ) {
		// Dying out
		ent->s.effect1Time = -1;
	} else {
		// Poof and it's gone
		G_FreeEntity( ent );
	}
}

void G_PoisonGasExplode(gentity_t* ent) {
    int lived = 0;

    if (!ent->grenadeExplodeTime)
        ent->grenadeExplodeTime = level.time;

    lived = level.time - ent->grenadeExplodeTime;
    ent->nextthink = level.time + FRAMETIME;

    if (lived < SMOKEBOMB_GROWTIME) {
        // Just been thrown, increase radius
		ent->s.effect1Time = 16 + lived * ( ( 640.f - 16.f ) / (float)SMOKEBOMB_GROWTIME );
    }
    else if (lived < SMOKEBOMB_SMOKETIME + SMOKEBOMB_GROWTIME) {
        // Smoking
        ent->s.effect1Time = 640;

        if (level.time >= ent->poisonGasAlarm) {
            ent->poisonGasAlarm = level.time + 1500;
                G_RadiusDamage2(
                ent->r.currentOrigin,
                ent,
                ent->parent,
                ent->poisonGasDamage,
                ent->poisonGasRadius,
                ent,
                MOD_POISONGAS,
                RADIUS_SCOPE_CLIENTS );
        }
    }
    else if (lived < SMOKEBOMB_SMOKETIME + SMOKEBOMB_GROWTIME + SMOKEBOMB_POSTSMOKETIME) {
        // Dying out
        ent->s.effect1Time = -1;
    }
    else {
        // Poof and it's gone
        G_FreeEntity( ent );
    }
}

/*
======================================================================

MACHINEGUN

======================================================================
*/

/*
======================
SnapVectorTowards

Round a vector to integers for more efficient network
transmission, but make sure that it rounds towards a given point
rather than blindly truncating.  This prevents it from truncating
into a wall.
======================
*/

// (SA) modified so it doesn't have trouble with negative locations (quadrant problems)
//			(this was causing some problems with bullet marks appearing since snapping
//			too far off the target surface causes the the distance between the transmitted impact
//			point and the actual hit surface larger than the mark radius.  (so nothing shows) )

void SnapVectorTowards( vec3_t v, vec3_t to ) {
	int i;

	for ( i = 0 ; i < 3 ; i++ ) {
		if ( to[i] <= v[i] ) {
			v[i] = floor( v[i] );
		} else {
			v[i] = ceil( v[i] );
		}
	}
}

int G_GetWeaponDamage( int weapon, qboolean player ) {
		if (player) {
        return GetWeaponTableData(weapon)->playerDamage;
		}
		else {	// AI weapon damage
        return GetWeaponTableData(weapon)->aiDamage;
		}
	}

float G_GetWeaponSpread( int weapon ) {
		if ( g_userAim.integer ) 
		{
        return GetWeaponTableData(weapon)->spread;
	    }
	G_Printf( "shouldn't ever get here (weapon %d)\n",weapon );
	// jpw
	return 0;   // shouldn't get here
}

#define LUGER_SPREAD    G_GetWeaponSpread( WP_LUGER )
#define LUGER_DAMAGE(e)    G_GetWeaponDamage( WP_LUGER, e ) 
#define SILENCER_SPREAD G_GetWeaponSpread( WP_SILENCER )

#define COLT_SPREAD     G_GetWeaponSpread( WP_COLT )
#define COLT_DAMAGE(e)     G_GetWeaponDamage( WP_COLT, e ) 

#define VENOM_SPREAD    G_GetWeaponSpread( WP_VENOM )
#define VENOM_DAMAGE(e)    G_GetWeaponDamage( WP_VENOM, e ) 

#define MP40_SPREAD     G_GetWeaponSpread( WP_MP40 )
#define MP40_DAMAGE(e)     G_GetWeaponDamage( WP_MP40, e ) 

#define MP34_SPREAD     G_GetWeaponSpread( WP_MP34 )
#define MP34_DAMAGE(e)     G_GetWeaponDamage( WP_MP34, e ) 

#define TT33_SPREAD		G_GetWeaponSpread( WP_TT33 )
#define TT33_DAMAGE(e)		G_GetWeaponDamage( WP_TT33, e )

#define P38_SPREAD		G_GetWeaponSpread( WP_P38 )
#define P38_DAMAGE(e)		G_GetWeaponDamage( WP_P38, e )

#define HDM_SPREAD		G_GetWeaponSpread( WP_HDM )
#define HDM_DAMAGE(e)	G_GetWeaponDamage( WP_HDM, e )

#define REVOLVER_SPREAD		G_GetWeaponSpread( WP_REVOLVER )
#define REVOLVER_DAMAGE(e)		G_GetWeaponDamage( WP_REVOLVER, e )

#define PPSH_SPREAD     G_GetWeaponSpread( WP_PPSH )
#define PPSH_DAMAGE(e)     G_GetWeaponDamage( WP_PPSH, e ) 

#define MOSIN_SPREAD     G_GetWeaponSpread( WP_MOSIN )
#define MOSIN_DAMAGE(e)     G_GetWeaponDamage( WP_MOSIN, e ) 

#define G43_SPREAD     G_GetWeaponSpread( WP_G43 )
#define G43_DAMAGE(e)     G_GetWeaponDamage( WP_G43, e ) 

#define M1941_SPREAD     G_GetWeaponSpread( WP_M1941 )
#define M1941_DAMAGE(e)     G_GetWeaponDamage( WP_M1941, e ) 

#define M1941SCOPE_SPREAD   G_GetWeaponSpread( WP_M1941SCOPE )
#define M1941SCOPE_DAMAGE(e)   G_GetWeaponDamage( WP_M1941SCOPE, e ) 

#define M1GARAND_SPREAD     G_GetWeaponSpread( WP_M1GARAND )
#define M1GARAND_DAMAGE(e)     G_GetWeaponDamage( WP_M1GARAND, e ) 

#define BAR_SPREAD     G_GetWeaponSpread( WP_BAR )
#define BAR_DAMAGE(e)     G_GetWeaponDamage( WP_BAR, e ) 

#define MP44_SPREAD     G_GetWeaponSpread( WP_MP44 )
#define MP44_DAMAGE(e)     G_GetWeaponDamage( WP_MP44, e ) 

#define MG42M_SPREAD     G_GetWeaponSpread( WP_MG42M )
#define MG42M_DAMAGE(e)     G_GetWeaponDamage( WP_MG42M, e ) 

#define BROWNING_SPREAD     G_GetWeaponSpread( WP_BROWNING )
#define BROWNING_DAMAGE(e)     G_GetWeaponDamage( WP_BROWNING, e ) 

#define M97_SPREAD     G_GetWeaponSpread( WP_M97 )
#define M97_DAMAGE(e)     G_GetWeaponDamage( WP_M97, e ) 

#define AUTO5_SPREAD     G_GetWeaponSpread( WP_AUTO5 )
#define AUTO5_DAMAGE(e)     G_GetWeaponDamage( WP_AUTO5, e ) 

#define M30_SPREAD     G_GetWeaponSpread( WP_M30 )
#define M30_DAMAGE(e)     G_GetWeaponDamage( WP_M30, e ) 

#define THOMPSON_SPREAD G_GetWeaponSpread( WP_THOMPSON )
#define THOMPSON_DAMAGE(e) G_GetWeaponDamage( WP_THOMPSON, e ) 

#define STEN_SPREAD     G_GetWeaponSpread( WP_STEN )
#define STEN_DAMAGE(e)     G_GetWeaponDamage( WP_STEN, e ) 

#define FG42_SPREAD     G_GetWeaponSpread( WP_FG42 )
#define FG42_DAMAGE(e)     G_GetWeaponDamage( WP_FG42, e ) 

#define MAUSER_SPREAD   G_GetWeaponSpread( WP_MAUSER )
#define MAUSER_DAMAGE(e)   G_GetWeaponDamage( WP_MAUSER, e ) 

#define DELISLE_SPREAD   G_GetWeaponSpread( WP_DELISLE )
#define DELISLE_DAMAGE(e)   G_GetWeaponDamage( WP_DELISLE, e ) 

#define DELISLESCOPE_SPREAD   G_GetWeaponSpread( WP_DELISLESCOPE )
#define DELISLESCOPE_DAMAGE(e)   G_GetWeaponDamage( WP_DELISLESCOPE, e ) 

#define GARAND_SPREAD   G_GetWeaponSpread( WP_GARAND )
#define GARAND_DAMAGE(e)   G_GetWeaponDamage( WP_GARAND, e ) 

#define SNIPER_SPREAD   G_GetWeaponSpread( WP_SNIPERRIFLE )
#define SNIPER_DAMAGE(e)   G_GetWeaponDamage( WP_SNIPERRIFLE, e ) 

#define SNOOPER_SPREAD  G_GetWeaponSpread( WP_SNOOPERSCOPE )
#define SNOOPER_DAMAGE(e)  G_GetWeaponDamage( WP_SNOOPERSCOPE, e )

#define FG42SCOPE_SPREAD	G_GetWeaponSpread( WP_FG42SCOPE ) 
#define	FG42SCOPE_DAMAGE(e)	G_GetWeaponDamage( WP_FG42SCOPE, e ) 

/*
==============
EmitterCheck
	see if a new particle emitter should be created at the bullet impact point
==============
*/
void EmitterCheck( gentity_t *ent, gentity_t *attacker, trace_t *tr ) {
	gentity_t *tent;
	vec3_t origin;

	if ( !ent->emitNum ) { // no emitters left for this entity.
		return;
	}

	VectorCopy( tr->endpos, origin );
	SnapVectorTowards( tr->endpos, attacker->s.origin ); // make sure it's out of the wall


	// why were these stricmp's?...
	if ( ent->s.eType == ET_EXPLOSIVE ) {
	} else if ( ent->s.eType == ET_LEAKY ) {

		tent = G_TempEntity( origin, EV_EMITTER );
		VectorCopy( origin, tent->s.origin );
		tent->s.time = ent->emitTime;
		tent->s.density = ent->emitPressure;    // 'pressure'
		tent->s.teamNum = ent->emitID;          // 'type'
		VectorCopy( tr->plane.normal, tent->s.origin2 );
	}

	ent->emitNum--;
}


void SniperSoundEFX( vec3_t pos ) {
	G_TempEntity( pos, EV_SNIPER_SOUND );
}


/*
==============
Bullet_Endpos
	find target end position for bullet trace based on entities weapon and accuracy
==============
*/
void Bullet_Endpos( gentity_t *ent, float spread, vec3_t *end ) {
	float r, u;
	qboolean randSpread = qtrue;
	int dist = 8192;

	r = crandom() * spread;
	u = crandom() * spread;

	// Ridah, if this is an AI shooting, apply their accuracy
	if ( ent->r.svFlags & SVF_CASTAI ) {
		float accuracy;
		accuracy = ( 1.0 - AICast_GetAccuracy( ent->s.number ) ) * AICAST_AIM_SPREAD;
		r += crandom() * accuracy;
		u += crandom() * ( accuracy * 1.25 );
	} else {
		if ( ent->s.weapon == WP_SNOOPERSCOPE || ent->s.weapon == WP_SNIPERRIFLE || ent->s.weapon == WP_FG42SCOPE || ent->s.weapon == WP_DELISLESCOPE  ) {
			dist *= 2;
			randSpread = qfalse;
		}
	}

	VectorMA( muzzleTrace, dist, forward, *end );

	if ( randSpread ) {
		VectorMA( *end, r, right, *end );
		VectorMA( *end, u, up, *end );
	}
}

/*
==============
Bullet_Fire
==============
*/
void Bullet_Fire( gentity_t *ent, float spread, int damage ) {
	vec3_t end;

	Bullet_Endpos( ent, spread, &end );
	Bullet_Fire_Extended( ent, ent, muzzleTrace, end, spread, damage, 0 );
}

/*
==============
Bullet_Fire_Extended
	A modified Bullet_Fire with more parameters.
	The original Bullet_Fire still passes through here and functions as it always has.

	uses for this include shooting through entities (windows, doors, other players, etc.) and reflecting bullets
==============
*/
qboolean Bullet_Fire_Extended( gentity_t *source, gentity_t *attacker, vec3_t start, vec3_t end, float spread, int damage, int recursion ) {
	trace_t tr;
	gentity_t   *tent;
	gentity_t   *traceEnt;
	qboolean reflectBullet = qfalse;
	qboolean hitClient = qfalse;

	// RF, abort if too many recursions.. there must be a real solution for this, but for now this is the safest
	// fix I can find
	if ( recursion > 12 ) {
		return qfalse;
	}

	damage *= s_quadFactor;

	// (SA) changed so player could shoot his own dynamite.
	// (SA) whoops, but that broke bullets going through explosives...
	trap_Trace( &tr, start, NULL, NULL, end, source->s.number, MASK_SHOT );
//	trap_Trace (&tr, start, NULL, NULL, end, ENTITYNUM_NONE, MASK_SHOT);

	// DHM - Nerve :: only in single player
	AICast_ProcessBullet( attacker, start, tr.endpos );
	

	// bullet debugging using Q3A's railtrail
	if ( g_debugBullets.integer & 1 ) {
		tent = G_TempEntity( start, EV_RAILTRAIL );
		VectorCopy( tr.endpos, tent->s.origin2 );
		tent->s.otherEntityNum2 = attacker->s.number;
	}

	traceEnt = &g_entities[ tr.entityNum ];

	EmitterCheck( traceEnt, attacker, &tr );

	// snap the endpos to integers, but nudged towards the line
	SnapVectorTowards( tr.endpos, start );
    if ( g_gametype.integer == GT_GOTHIC || g_weaponfalloff.integer == 1 ) {
		vec_t dist;
		vec3_t shotvec;
		float scale;

		VectorSubtract( tr.endpos, muzzleTrace, shotvec );
		dist = VectorLengthSquared( shotvec );

		// zinx - start at 100% at 1000 units (and before),
		// and go to 50% at 2000 units (and after)

		// Square(1500) to Square(2500) -> 0.0 to 1.0
        scale = ( dist - Square ( ammoTable [attacker->s.weapon].falloffDistance[0] ) ) / ( Square( ammoTable [attacker->s.weapon].falloffDistance[1] ) - Square( ammoTable [attacker->s.weapon].falloffDistance[0] ) );
		// 0.0 to 1.0 -> 0.0 to 0.5
		scale *= 0.5f;
		// 0.0 to 0.5 -> 1.0 to 0.5
		scale = 1.0f - scale;

		// And, finally, cap it.
		if ( scale >= 1.0f ) {
			scale = 1.0f;
		} else if ( scale < 0.5f ) {
			scale = 0.5f;
		}

		damage *= scale;
	
	}

	// should we reflect this bullet?
	if ( traceEnt->flags & FL_DEFENSE_GUARD ) {
		reflectBullet = qtrue;
	} else if ( traceEnt->flags & FL_DEFENSE_CROUCH ) {
		if ( rand() % 3 < 2 ) {
			reflectBullet = qtrue;
		}
	}

	// send bullet impact
	if ( traceEnt->takedamage && traceEnt->client && !reflectBullet ) {
		tent = G_TempEntity( tr.endpos, EV_BULLET_HIT_FLESH );
		tent->s.eventParm = traceEnt->s.number;
		if ( LogAccuracyHit( traceEnt, attacker ) ) {
			hitClient = qtrue;
			attacker->client->ps.persistant[PERS_ACCURACY_HITS]++;
		}

//----(SA)	added
		if ( g_debugBullets.integer >= 2 ) {   // show hit player bb
			gentity_t *bboxEnt;
			vec3_t b1, b2;
			VectorCopy( traceEnt->r.currentOrigin, b1 );
			VectorCopy( traceEnt->r.currentOrigin, b2 );
			VectorAdd( b1, traceEnt->r.mins, b1 );
			VectorAdd( b2, traceEnt->r.maxs, b2 );
			bboxEnt = G_TempEntity( b1, EV_RAILTRAIL );
			VectorCopy( b2, bboxEnt->s.origin2 );
			bboxEnt->s.dmgFlags = 1;    // ("type")
		}
//----(SA)	end

	} else if ( traceEnt->takedamage && traceEnt->s.eType == ET_BAT ) {
		tent = G_TempEntity( tr.endpos, EV_BULLET_HIT_FLESH );
		tent->s.eventParm = traceEnt->s.number;
	} else {
		// Ridah, bullet impact should reflect off surface
		vec3_t reflect;
		float dot;

		if ( g_debugBullets.integer <= -2 ) {  // show hit thing bb
			gentity_t *bboxEnt;
			vec3_t b1, b2;
			VectorCopy( traceEnt->r.currentOrigin, b1 );
			VectorCopy( traceEnt->r.currentOrigin, b2 );
			VectorAdd( b1, traceEnt->r.mins, b1 );
			VectorAdd( b2, traceEnt->r.maxs, b2 );
			bboxEnt = G_TempEntity( b1, EV_RAILTRAIL );
			VectorCopy( b2, bboxEnt->s.origin2 );
			bboxEnt->s.dmgFlags = 1;    // ("type")
		}

		if ( reflectBullet ) {
			// reflect off sheild
			VectorSubtract( tr.endpos, traceEnt->r.currentOrigin, reflect );
			VectorNormalize( reflect );
			VectorMA( traceEnt->r.currentOrigin, 15, reflect, reflect );
			tent = G_TempEntity( reflect, EV_BULLET_HIT_WALL );
		} else {
			tent = G_TempEntity( tr.endpos, EV_BULLET_HIT_WALL );
		}

		dot = DotProduct( forward, tr.plane.normal );
		VectorMA( forward, -2 * dot, tr.plane.normal, reflect );
		VectorNormalize( reflect );

		tent->s.eventParm = DirToByte( reflect );

		if ( reflectBullet ) {
			tent->s.otherEntityNum2 = traceEnt->s.number;   // force sparks
		} else {
			tent->s.otherEntityNum2 = ENTITYNUM_NONE;
		}
		// done.
	}
	tent->s.otherEntityNum = attacker->s.number;

	if ( traceEnt->takedamage ) {
		qboolean reflectBool = qfalse;
		vec3_t trDir;

		if ( reflectBullet ) {
			// if we are facing the direction the bullet came from, then reflect it
			AngleVectors( traceEnt->s.apos.trBase, trDir, NULL, NULL );
			if ( DotProduct( forward, trDir ) < 0.6 ) {
				reflectBool = qtrue;
			}
		}

		if ( reflectBool ) {
			vec3_t reflect_end;
			// reflect this bullet
			G_AddEvent( traceEnt, EV_GENERAL_SOUND, level.bulletRicochetSound );
			CalcMuzzlePoints( traceEnt, traceEnt->s.weapon );

//----(SA)	modified to use extended version so attacker would pass through
//			Bullet_Fire( traceEnt, 1000, damage );
			Bullet_Endpos( traceEnt, 2800, &reflect_end );    // make it inaccurate
			Bullet_Fire_Extended( traceEnt, attacker, muzzleTrace, reflect_end, spread, damage, recursion + 1 );
//----(SA)	end

		} else {

			// Ridah, don't hurt team-mates
			// DHM - Nerve :: Only in single player
			if ( attacker->client && traceEnt->client && ( traceEnt->r.svFlags & SVF_CASTAI ) && ( attacker->r.svFlags & SVF_CASTAI ) && AICast_SameTeam( AICast_GetCastState( attacker->s.number ), traceEnt->s.number ) ) {
				// AI's don't hurt members of their own team
				return qfalse;
			}
			// done.

			G_Damage( traceEnt, attacker, attacker, forward, tr.endpos, damage, ( g_weaponfalloff.integer ? DAMAGE_DISTANCEFALLOFF : 0 ), ammoTable[attacker->s.weapon].mod );

			// allow bullets to "pass through" func_explosives if they break by taking another simultanious shot
			// start new bullet at position this hit and continue to the end position (ignoring shot-through ent in next trace)
			// spread = 0 as this is an extension of an already spread shot (so just go straight through)
			if ( Q_stricmp( traceEnt->classname, "func_explosive" ) == 0 ) {
				if ( traceEnt->health <= 0 ) {
					Bullet_Fire_Extended( traceEnt, attacker, tr.endpos, end, 0, damage, recursion + 1 );
				}
			} else if ( traceEnt->client ) {
				if ( traceEnt->health <= 0 ) {
					Bullet_Fire_Extended( traceEnt, attacker, tr.endpos, end, 0, damage / 2, recursion + 1 ); // halve the damage each player it goes through
				}
			}
		}
	}
	return hitClient;
}



/*
======================================================================

GRENADE LAUNCHER

  700 has been the standard direction multiplier in fire_grenade()

======================================================================
*/
extern void G_ExplodeMissilePoisonGas( gentity_t *ent );

gentity_t *weapon_gpg40_fire( gentity_t *ent, int grenType ) {
	gentity_t   *m /*, *te*/; // JPW NERVE
	trace_t tr;
	vec3_t viewpos;
//	float		upangle = 0, pitch;			//	start with level throwing and adjust based on angle
	vec3_t tosspos;
	//bani - to prevent nade-through-teamdoor sploit
	vec3_t orig_viewpos;

	AngleVectors( ent->client->ps.viewangles, forward, NULL, NULL );

	VectorCopy( muzzleEffect, tosspos );

	// check for valid start spot (so you don't throw through or get stuck in a wall)
	VectorCopy( ent->s.pos.trBase, viewpos );
	viewpos[2] += ent->client->ps.viewheight;
	VectorCopy( viewpos, orig_viewpos );    //bani - to prevent nade-through-teamdoor sploit
	VectorMA( viewpos, 32, forward, viewpos );

	//bani - to prevent nade-through-teamdoor sploit
	trap_Trace( &tr, orig_viewpos, tv( -4.f, -4.f, 0.f ), tv( 4.f, 4.f, 6.f ), viewpos, ent->s.number, MASK_MISSILESHOT );
	if ( tr.fraction < 1 ) { // oops, bad launch spot ) {
		VectorCopy( tr.endpos, tosspos );
		SnapVectorTowards( tosspos, orig_viewpos );
	} else {
		trap_Trace( &tr, viewpos, tv( -4.f, -4.f, 0.f ), tv( 4.f, 4.f, 6.f ), tosspos, ent->s.number, MASK_MISSILESHOT );
		if ( tr.fraction < 1 ) { // oops, bad launch spot
			VectorCopy( tr.endpos, tosspos );
			SnapVectorTowards( tosspos, viewpos );
		}
	}

	VectorScale( forward, 2000, forward );

	m = fire_grenade( ent, tosspos, forward, grenType );

	m->damage = 0;

	// Ridah, return the grenade so we can do some prediction before deciding if we really want to throw it or not
	return m;
}

gentity_t *weapon_grenadelauncher_fire( gentity_t *ent, int grenType ) {
	gentity_t   *m, *te;
	float upangle = 0;                  //	start with level throwing and adjust based on angle
	vec3_t tosspos;
	qboolean underhand = 0;



	if ( underhand ) {
		forward[2] = 0;                 //	start the toss level for underhand
	} else {
		forward[2] += 0.2;              //	extra vertical velocity for overhand
	}
	VectorNormalize( forward );         //	make sure forward is normalized
	upangle = -( ent->s.apos.trBase[0] ); //	this will give between	-90 / 90
	upangle = min( upangle, 50 );
	upangle = max( upangle, -50 );        //	now clamped to			-50 / 50	(don't allow firing straight up/down)
	upangle = upangle / 100.0f;           //						   -0.5 / 0.5
	upangle += 0.5f;                    //						    0.0 / 1.0
	if ( upangle < .1 ) {
		upangle = .1;
	}

		switch ( grenType ) {
		case WP_GRENADE_LAUNCHER:
		case WP_GRENADE_PINEAPPLE:
		case WP_POISONGAS:
		case WP_DYNAMITE:
		case WP_AIRSTRIKE:
			upangle *= ammoTable[grenType].upAngle;
			break;
		default:
		break;
		}



	{
		VectorCopy( muzzleEffect, tosspos );
		if ( underhand ) {
			VectorMA( muzzleEffect, 15, forward, tosspos );   // move a little bit more away from the player (so underhand tosses don't get caught on nearby lips)
			tosspos[2] -= 24;   // lower origin for the underhand throw
			upangle *= 1.3;     // a little more force to counter the lower position / lack of additional lift
		}
		VectorScale( forward, upangle, forward );
		{
			// check for valid start spot (so you don't throw through or get stuck in a wall)
			trace_t tr;
			vec3_t viewpos;
			VectorCopy( ent->s.pos.trBase, viewpos );
			viewpos[2] += ent->client->ps.viewheight;
			trap_Trace( &tr, viewpos, NULL, NULL, tosspos, ent->s.number, MASK_SHOT );
			if ( tr.fraction < 1 ) {   // oops, bad launch spot
				VectorCopy( tr.endpos, tosspos );
			}
		}
		m = fire_grenade( ent, tosspos, forward, grenType );
	}
	//m->damage *= s_quadFactor;
	m->damage = 0;  // Ridah, grenade's don't explode on contact
	m->splashDamage *= s_quadFactor;

	if ( grenType == WP_POISONGAS ) 
	{
            m->s.effect1Time = 16;
            m->think = G_PoisonGasExplode;
            m->poisonGasAlarm  = level.time + SMOKEBOMB_GROWTIME;
			m->poisonGasRadius          = ammoTable[WP_POISONGAS].playerSplashRadius;
			m->poisonGasDamage        =  ammoTable[WP_POISONGAS].playerDamage;	
		    
	}

	if ( grenType == WP_AIRSTRIKE ) {

		//m->s.otherEntityNum2 = 1; 
		m->s.otherEntityNum2 = 0;
		m->nextthink = level.time + 4000;
		m->think = weapon_callAirStrike;

		te = G_TempEntity( m->s.pos.trBase, EV_GLOBAL_SOUND );
		te->s.eventParm = G_SoundIndex( "sound/weapons/airstrike/airstrike_01.wav" );
		te->r.svFlags |= SVF_BROADCAST | SVF_USE_CURRENT_ORIGIN;
	}

	if ( ent->aiCharacter == AICHAR_VENOM ) { // poison gas grenade
		m->think = G_ExplodeMissilePoisonGas;
		m->s.density = 1;
	}

	//----(SA)	adjust for movement of character.  TODO: Probably comment in later, but only for forward/back not strafing
//	VectorAdd( m->s.pos.trDelta, ent->client->ps.velocity, m->s.pos.trDelta );	// "real" physics
	// let the AI know which grenade it has fired
	ent->grenadeFired = m->s.number;
	// Ridah, return the grenade so we can do some prediction before deciding if we really want to throw it or not
	return m;
}

gentity_t *quickgren_fire( gentity_t *ent, int grenType ) {
	gentity_t   *m ;
	float upangle = 0;                  //	start with level throwing and adjust based on angle
	vec3_t tosspos;
	qboolean underhand = 0;
	gentity_t	*tent;


	s_quadFactor = 1;


	if ( underhand ) {
		forward[2] = 0;                 //	start the toss level for underhand
	} else {
		forward[2] += 0.2;              //	extra vertical velocity for overhand
	}
	VectorNormalize( forward );         //	make sure forward is normalized
	upangle = -( ent->s.apos.trBase[0] ); //	this will give between	-90 / 90
	upangle = min( upangle, 50 );
	upangle = max( upangle, -50 );        //	now clamped to			-50 / 50	(don't allow firing straight up/down)
	upangle = upangle / 100.0f;           //						   -0.5 / 0.5
	upangle += 0.5f;                    //						    0.0 / 1.0
	if ( upangle < .1 ) {
		upangle = .1;
	}

		switch ( grenType ) {
		case WP_GRENADE_LAUNCHER:
		case WP_GRENADE_PINEAPPLE:
			upangle *= ammoTable[grenType].upAngle;
			break;
		default:
		break;
		}


		tent = G_TempEntity( ent->r.currentOrigin, EV_QUICKGRENS );
		tent->s.eventParm = ent->s.number;
	
	{
		VectorCopy( muzzleEffect, tosspos );
		if ( underhand ) {
			VectorMA( muzzleEffect, 15, forward, tosspos );   // move a little bit more away from the player (so underhand tosses don't get caught on nearby lips)
			tosspos[2] -= 24;   // lower origin for the underhand throw
			upangle *= 1.3;     // a little more force to counter the lower position / lack of additional lift
		}
		VectorScale( forward, upangle, forward );
		{
			// check for valid start spot (so you don't throw through or get stuck in a wall)
			trace_t tr;
			vec3_t viewpos;
			VectorCopy( ent->s.pos.trBase, viewpos );
			viewpos[2] += ent->client->ps.viewheight;
			trap_Trace( &tr, viewpos, NULL, NULL, tosspos, ent->s.number, MASK_SHOT );
			if ( tr.fraction < 1 ) {   // oops, bad launch spot
				VectorCopy( tr.endpos, tosspos );
			}
		}
		m = fire_grenade( ent, tosspos, forward, grenType );
	}
	//m->damage *= s_quadFactor;
	m->damage = 0;  // Ridah, grenade's don't explode on contact
	m->splashDamage *= s_quadFactor;

	//----(SA)	adjust for movement of character.  TODO: Probably comment in later, but only for forward/back not strafing
//	VectorAdd( m->s.pos.trDelta, ent->client->ps.velocity, m->s.pos.trDelta );	// "real" physics
	// let the AI know which grenade it has fired
	ent->grenadeFired = m->s.number;
	// Ridah, return the grenade so we can do some prediction before deciding if we really want to throw it or not
	return m;
}

/*
=====================
Zombie spit
=====================
*/
void weapon_zombiespit( gentity_t *ent ) {
	return;
#if 0 //RF, HARD disable
	gentity_t *m;

	m = fire_zombiespit( ent, muzzleTrace, forward );
	m->damage *= s_quadFactor;
	m->splashDamage *= s_quadFactor;

	if ( m ) {
		G_AddEvent( ent, EV_GENERAL_SOUND, G_SoundIndex( "sound/Loogie/spit.wav" ) );
	}
#endif
}

/*
=====================
Zombie spirit
=====================
*/
void weapon_zombiespirit( gentity_t *ent, gentity_t *missile ) {
	gentity_t *m;

	m = fire_zombiespirit( ent, missile, muzzleTrace, forward );
	m->damage *= s_quadFactor;
	m->splashDamage *= s_quadFactor;

	if ( m ) {
		G_AddEvent( ent, EV_GENERAL_SOUND, G_SoundIndex( "zombieAttackPlayer" ) );
	}

}

//----(SA)	modified this entire "venom" section
/*
============================================================================

VENOM GUN TRACING

============================================================================
*/
#define DEFAULT_VENOM_COUNT 10
#define DEFAULT_VENOM_SPREAD 20
#define DEFAULT_VENOM_DAMAGE 15

qboolean VenomPellet( vec3_t start, vec3_t end, gentity_t *ent ) {
	trace_t tr;
	int damage;
	gentity_t       *traceEnt;

	trap_Trace( &tr, start, NULL, NULL, end, ent->s.number, MASK_SHOT );
	traceEnt = &g_entities[ tr.entityNum ];

	// send bullet impact
	if (  tr.surfaceFlags & SURF_NOIMPACT ) {
		return qfalse;
	}

	if ( traceEnt->takedamage ) {
		damage = DEFAULT_VENOM_DAMAGE * s_quadFactor;

		G_Damage( traceEnt, ent, ent, forward, tr.endpos, damage, 0, MOD_VENOM );
		if ( LogAccuracyHit( traceEnt, ent ) ) {
			return qtrue;
		}
	}
	return qfalse;
}

// this should match CG_VenomPattern
void VenomPattern( vec3_t origin, vec3_t origin2, int seed, gentity_t *ent ) {
	int i;
	float r, u;
	vec3_t end;
	vec3_t forward, right, up;
	qboolean hitClient = qfalse;

	// derive the right and up vectors from the forward vector, because
	// the client won't have any other information
	VectorNormalize2( origin2, forward );
	PerpendicularVector( right, forward );
	CrossProduct( forward, right, up );

	// generate the "random" spread pattern
	for ( i = 0 ; i < DEFAULT_VENOM_COUNT ; i++ ) {
		r = Q_crandom( &seed ) * DEFAULT_VENOM_SPREAD;
		u = Q_crandom( &seed ) * DEFAULT_VENOM_SPREAD;
		VectorMA( origin, 8192, forward, end );
		VectorMA( end, r, right, end );
		VectorMA( end, u, up, end );
		if ( VenomPellet( origin, end, ent ) && !hitClient ) {
			hitClient = qtrue;
			ent->client->ps.persistant[PERS_ACCURACY_HITS]++;
		}
	}
}



/*
==============
weapon_venom_fire
==============
*/
void weapon_venom_fire( gentity_t *ent, qboolean fullmode, float aimSpreadScale ) {
	gentity_t       *tent;
	qboolean	isPlayer = (ent->client && !ent->aiCharacter);	// Knightmare added

	if ( fullmode ) {
		tent = G_TempEntity( muzzleTrace, EV_VENOMFULL );
	} else {
		tent = G_TempEntity( muzzleTrace, EV_VENOM );
	}

	VectorScale( forward, 4096, tent->s.origin2 );
	SnapVector( tent->s.origin2 );
	tent->s.eventParm = rand() & 255;       // seed for spread pattern
	tent->s.otherEntityNum = ent->s.number;

	if ( fullmode ) {
		VenomPattern( tent->s.pos.trBase, tent->s.origin2, tent->s.eventParm, ent );
	} else
	{
		int dam;
		dam = VENOM_DAMAGE(isPlayer);

		Bullet_Fire( ent, VENOM_SPREAD * aimSpreadScale, dam );
	}
}





/*
======================================================================

ROCKET

======================================================================
*/

void Weapon_RocketLauncher_Fire( gentity_t *ent, float aimSpreadScale ) {
//	trace_t		tr;
	float r, u;
	vec3_t dir, launchpos;     //, viewpos, wallDir;
	gentity_t   *m;

	// get a little bit of randomness and apply it back to the direction
	if ( !ent->aiCharacter ) {
		r = crandom() * aimSpreadScale;
		u = crandom() * aimSpreadScale;

		VectorScale( forward, 16, dir );
		VectorMA( dir, r, right, dir );
		VectorMA( dir, u, up, dir );
		VectorNormalize( dir );

		VectorCopy( muzzleEffect, launchpos );

		m = fire_rocket( ent, launchpos, dir );

		// add kick-back
		VectorMA( ent->client->ps.velocity, -80, forward, ent->client->ps.velocity ); // RealRTCW was -64

	} else {
		m = fire_rocket( ent, muzzleEffect, forward );
	}

	m->damage *= s_quadFactor;
	m->splashDamage *= s_quadFactor;

//	VectorAdd( m->s.pos.trDelta, ent->client->ps.velocity, m->s.pos.trDelta );	// "real" physics
}

void Use_Item( gentity_t *ent, gentity_t *other, gentity_t *activator );

void ThrowKnife( gentity_t *ent )
{
	gentity_t	*knife;
	float		speed;
	vec3_t		dir;
	float		r, u;

	CalcMuzzlePoints(ent, ent->s.weapon);

	// 'spread'
	r = crandom()*(ammoTable[ent->s.weapon].spread/1000);
	u = crandom()*(ammoTable[ent->s.weapon].spread/1000);
	VectorScale(forward, 16, dir);
	VectorMA (dir, r, right, dir);
	VectorMA (dir, u, up, dir);
	VectorNormalize(dir);

	// entity handling
	knife						= G_Spawn();
	knife->classname 			= "knife";
	knife->nextthink 			= level.time + 100000;
	knife->think				= G_FreeEntity;

	// misc
	knife->s.clientNum			= ent->client->ps.clientNum;
	knife->s.eType				= ET_ITEM;
	knife->s.weapon				= ent->s.weapon;						// Use the correct weapon in multiplayer
	knife->parent 				= ent;
	knife->r.svFlags            = SVF_USE_CURRENT_ORIGIN | SVF_BROADCAST;

	// usage
	knife->touch				= Touch_Item;	// no auto-pickup, only activate
	knife->use					= Use_Item;

	// damage
	knife->damage 				= 50; 	// JPW NERVE
	knife->splashDamage			= 0;
	knife->splashRadius			= 0;
	knife->methodOfDeath 		= MOD_THROWKNIFE;

	// clipping
	knife->clipmask 			= CONTENTS_SOLID|MASK_MISSILESHOT;
	knife->r.contents			= CONTENTS_TRIGGER|CONTENTS_ITEM;

	// angles/origin
	G_SetAngle( knife, ent->client->ps.viewangles );
	G_SetOrigin( knife, muzzleEffect);

	// trajectory
	knife->s.pos.trType 		= TR_GRAVITY_LOW;
	knife->s.pos.trTime 		= level.time - 50;	// move a bit on the very first frame

	// bouncing
	knife->physicsBounce		= 0.20;
	knife->physicsObject		= qtrue;

	// NQ physics
	knife->physicsSlide			= qfalse;
	knife->physicsFlush			= qtrue;

	// bounding box
	VectorSet( knife->r.mins, -ITEM_RADIUS, -ITEM_RADIUS, 0 );
	VectorSet( knife->r.maxs, ITEM_RADIUS, ITEM_RADIUS, 2*ITEM_RADIUS );

	// speed / dir
	speed = KNIFESPEED; //*ent->client->ps.grenadeTimeLeft/500;

	// minimal toss speed
	/*if ( speed < MIN_KNIFESPEED )
		speed = MIN_KNIFESPEED;*/

	VectorScale( dir, speed, knife->s.pos.trDelta );
	SnapVector( knife->s.pos.trDelta );

	// rotation
	knife->s.apos.trTime = level.time - 50;
	knife->s.apos.trType = TR_LINEAR;
	VectorCopy( ent->client->ps.viewangles, knife->s.apos.trBase );
	knife->s.apos.trDelta[0] = speed*3;

	// item
	knife->item =  BG_FindItemForWeapon( ent->s.weapon );
	knife->s.modelindex = knife->item - bg_itemlist;	// store item number in modelindex
	knife->s.otherEntityNum2 = 1;	// DHM - Nerve :: this is taking modelindex2's place for a dropped item
	knife->flags |= FL_DROPPED_ITEM;	// so it gets removd after being picked up

	// add knife to game
	trap_LinkEntity (knife);

	// player himself
	ent->client->ps.grenadeTimeLeft = 0;

}


/*
======================================================================

LIGHTNING GUN

======================================================================
*/

// RF, not used anymore for Flamethrower (still need it for tesla?)
void Weapon_LightningFire( gentity_t *ent ) {
	trace_t tr;
	vec3_t end;
	gentity_t   *traceEnt;
	int damage;

	damage = 5 * s_quadFactor;

	VectorMA( muzzleTrace, LIGHTNING_RANGE, forward, end );

	trap_Trace( &tr, muzzleTrace, NULL, NULL, end, ent->s.number, MASK_SHOT );

	if ( tr.entityNum == ENTITYNUM_NONE ) {
		return;
	}

	traceEnt = &g_entities[ tr.entityNum ];
/*
	if ( traceEnt->takedamage && traceEnt->client ) {
		tent = G_TempEntity( tr.endpos, EV_MISSILE_HIT );
		tent->s.otherEntityNum = traceEnt->s.number;
		tent->s.eventParm = DirToByte( tr.plane.normal );
		tent->s.weapon = ent->s.weapon;
		if( LogAccuracyHit( traceEnt, ent ) ) {
			ent->client->ps.persistant[PERS_ACCURACY_HITS]++;
		}
	} else if ( !( tr.surfaceFlags & SURF_NOIMPACT ) ) {
		tent = G_TempEntity( tr.endpos, EV_MISSILE_MISS );
		tent->s.eventParm = DirToByte( tr.plane.normal );
	}
*/
	if ( traceEnt->takedamage && !AICast_NoFlameDamage( traceEnt->s.number ) ) {
		#define FLAME_THRESHOLD 50

		// RF, only do damage once they start burning
		//if (traceEnt->health > 0)	// don't explode from flamethrower
		//	G_Damage( traceEnt, ent, ent, forward, tr.endpos, 1, 0, MOD_LIGHTNING);

		// now check the damageQuota to see if we should play a pain animation
		// first reduce the current damageQuota with time
		if ( traceEnt->flameQuotaTime && traceEnt->flameQuota > 0 ) {
			traceEnt->flameQuota -= (int)( ( (float)( level.time - traceEnt->flameQuotaTime ) / 1000 ) * (float)damage / 2.0 );
			if ( traceEnt->flameQuota < 0 ) {
				traceEnt->flameQuota = 0;
			}
		}

		// add the new damage
		traceEnt->flameQuota += damage;
		traceEnt->flameQuotaTime = level.time;

		// Ridah, make em burn
		if ( traceEnt->client && ( traceEnt->health <= 0 || traceEnt->flameQuota > FLAME_THRESHOLD ) ) {
			if ( traceEnt->s.onFireEnd < level.time ) {
				traceEnt->s.onFireStart = level.time;
			}
			if ( traceEnt->health <= 0 || !( traceEnt->r.svFlags & SVF_CASTAI ) ) {
				if ( traceEnt->r.svFlags & SVF_CASTAI ) {
					traceEnt->s.onFireEnd = level.time + 6000;
				} else {
					traceEnt->s.onFireEnd = level.time + FIRE_FLASH_TIME;
				}
			} else {
				traceEnt->s.onFireEnd = level.time + 99999; // make sure it goes for longer than they need to die
			}
			traceEnt->flameBurnEnt = ent->s.number;
			// add to playerState for client-side effect
			traceEnt->client->ps.onFireStart = level.time;
		}
	}
}

//======================================================================


/*
==============
AddLean
	add leaning offset
==============
*/
void AddLean( gentity_t *ent, vec3_t point ) {
	if ( ent->client ) {
		if ( ent->client->ps.leanf ) {
			vec3_t right;
			AngleVectors( ent->client->ps.viewangles, NULL, right, NULL );
			VectorMA( point, ent->client->ps.leanf, right, point );
		}
	}
}

/*
===============
LogAccuracyHit
===============
*/
qboolean LogAccuracyHit( gentity_t *target, gentity_t *attacker ) {
	if ( !target->takedamage ) {
		return qfalse;
	}

	if ( target == attacker ) {
		return qfalse;
	}

	if ( !target->client ) {
		return qfalse;
	}

	if ( !attacker->client ) {
		return qfalse;
	}

	if ( target->client->ps.stats[STAT_HEALTH] <= 0 ) {
		return qfalse;
	}

	if ( OnSameTeam( target, attacker ) ) {
		return qfalse;
	}

	return qtrue;
}


/*
===============
CalcMuzzlePoint

set muzzle location relative to pivoting eye
===============
*/
void CalcMuzzlePoint( gentity_t *ent, int weapon, vec3_t forward, vec3_t right, vec3_t up, vec3_t muzzlePoint ) {
	VectorCopy( ent->r.currentOrigin, muzzlePoint );
	muzzlePoint[2] += ent->client->ps.viewheight;
	// Ridah, this puts the start point outside the bounding box, isn't necessary
//	VectorMA( muzzlePoint, 14, forward, muzzlePoint );
	// done.

	// Ridah, offset for more realistic firing from actual gun position
	//----(SA) modified
	switch ( weapon )  // Ridah, changed this so I can predict weapons
	{
	case WP_PANZERFAUST:
//			VectorMA( muzzlePoint, 14, right, muzzlePoint );	//----(SA)	new first person rl position
		VectorMA( muzzlePoint, 10, right, muzzlePoint );        //----(SA)	new first person rl position
		VectorMA( muzzlePoint, -10, up, muzzlePoint );
		break;
//		case WP_ROCKET_LAUNCHER:
//			VectorMA( muzzlePoint, 14, right, muzzlePoint );	//----(SA)	new first person rl position
//			break;
	case WP_DYNAMITE:
	case WP_GRENADE_PINEAPPLE:
	case WP_GRENADE_LAUNCHER:
	case WP_POISONGAS:
		VectorMA( muzzlePoint, 20, right, muzzlePoint );
		break;
	case WP_AKIMBO:     // left side rather than right
	case WP_DUAL_TT33:
		VectorMA( muzzlePoint, -6, right, muzzlePoint );
		VectorMA( muzzlePoint, -4, up, muzzlePoint );
		break;
	case WP_KNIFE:
	    break;
	default:
		VectorMA( muzzlePoint, 6, right, muzzlePoint );
		VectorMA( muzzlePoint, -4, up, muzzlePoint );
		break;
	}

	// done.

	// (SA) actually, this is sort of moot right now since
	// you're not allowed to fire when leaning.  Leave in
	// in case we decide to enable some lean-firing.
	AddLean( ent, muzzlePoint );

	// snap to integer coordinates for more efficient network bandwidth usage
	SnapVector( muzzlePoint );
}

// Rafael - for activate
void CalcMuzzlePointForActivate( gentity_t *ent, vec3_t forward, vec3_t right, vec3_t up, vec3_t muzzlePoint ) {

	VectorCopy( ent->s.pos.trBase, muzzlePoint );
	muzzlePoint[2] += ent->client->ps.viewheight;

	AddLean( ent, muzzlePoint );

	// snap to integer coordinates for more efficient network bandwidth usage
//	SnapVector( muzzlePoint );
	// (SA) /\ only used server-side, so leaving the accuracy in is fine (and means things that show a cursorhint will be hit when activated)
	//			(there were differing views of activatable stuff between cursorhint and activatable)
}
// done.

// Ridah
void CalcMuzzlePoints( gentity_t *ent, int weapon ) {
	vec3_t viewang;

	VectorCopy( ent->client->ps.viewangles, viewang );

	if ( !( ent->r.svFlags & SVF_CASTAI ) ) {   // non ai's take into account scoped weapon 'sway' (just another way aimspread is visualized/utilized)
		float spreadfrac, phase;

		if ( weapon == WP_SNIPERRIFLE || weapon == WP_SNOOPERSCOPE || weapon == WP_FG42SCOPE || weapon == WP_DELISLESCOPE || weapon == WP_M1941SCOPE ) {
			spreadfrac = ent->client->currentAimSpreadScale;

			// rotate 'forward' vector by the sway
			phase = level.time / 1000.0 * ZOOM_PITCH_FREQUENCY * M_PI * 2;
			viewang[PITCH] += ZOOM_PITCH_AMPLITUDE * sin( phase ) * ( spreadfrac + ZOOM_PITCH_MIN_AMPLITUDE );

			phase = level.time / 1000.0 * ZOOM_YAW_FREQUENCY * M_PI * 2;
			viewang[YAW] += ZOOM_YAW_AMPLITUDE * sin( phase ) * ( spreadfrac + ZOOM_YAW_MIN_AMPLITUDE );
		}
	}


	// set aiming directions
	AngleVectors( viewang, forward, right, up );

//----(SA)	modified the muzzle stuff so that weapons that need to fire down a perfect trace
//			straight out of the camera (SP5, Mauser right now) can have that accuracy, but
//			weapons that need an offset effect (bazooka/grenade/etc.) can still look like
//			they came out of the weap.
	CalcMuzzlePointForActivate( ent, forward, right, up, muzzleTrace );
	CalcMuzzlePoint( ent, weapon, forward, right, up, muzzleEffect );
}

/*
===============
FireWeapon
===============
*/
void FireWeapon( gentity_t *ent ) {
	float aimSpreadScale;
	vec3_t viewang;  // JPW NERVE
	qboolean	isPlayer = (ent->client && !ent->aiCharacter);	// Knightmare added

	// Rafael mg42
	if ( ent->client->ps.persistant[PERS_HWEAPON_USE] && ent->active ) {
		return;
	}

	if ( ent->client->ps.powerups[PW_QUAD] ) {
		s_quadFactor = 4;
	} else {
		s_quadFactor = 1;
	}

	// Ridah, need to call this for AI prediction also
	CalcMuzzlePoints( ent, ent->s.weapon );

	if ( g_userAim.integer ) {
		aimSpreadScale = ent->client->currentAimSpreadScale;
		// Ridah, add accuracy factor for AI
		if ( ent->aiCharacter ) {
			float aim_accuracy;
			aim_accuracy = AICast_GetAccuracy( ent->s.number );
			if ( aim_accuracy <= 0 ) {
				aim_accuracy = 0.0001;
			}
			aimSpreadScale = ( 1.0 - aim_accuracy ) * 2.0;
		} else {
			//	/maximum/ accuracy for player for a given weapon
			switch ( ent->s.weapon ) {
			case WP_LUGER:
			case WP_SILENCER:
			case WP_COLT:
			case WP_AKIMBO:
			case WP_DUAL_TT33:
				aimSpreadScale += 0.4f;
				break;

			case WP_PANZERFAUST:
				aimSpreadScale += 0.3f;     // it's calculated a different way, so this keeps the accuracy never perfect, but never rediculously wild either
				break;

			default:
				aimSpreadScale += 0.15f;
				break;
			}

			if ( aimSpreadScale > 1 ) {
				aimSpreadScale = 1.0f;  // still cap at 1.0
			}
		}
	} else {
		aimSpreadScale = 1.0;
	}

	// fire the specific weapon
	switch ( ent->s.weapon ) {
	case WP_KNIFE:
		Weapon_Knife( ent );
		break;
	case WP_DAGGER:
		Weapon_Dagger( ent );
		break;
	case WP_LUGER:
		Bullet_Fire( ent, LUGER_SPREAD * aimSpreadScale, LUGER_DAMAGE(isPlayer) );
		break;
	case WP_SILENCER:
		Bullet_Fire( ent, SILENCER_SPREAD * aimSpreadScale, LUGER_DAMAGE(isPlayer) );
		break;
	case WP_AKIMBO: //----(SA)	added
	case WP_COLT:
		Bullet_Fire( ent, COLT_SPREAD * aimSpreadScale, COLT_DAMAGE(isPlayer) );
		break;
	case WP_VENOM:
		weapon_venom_fire( ent, qfalse, aimSpreadScale );
		break;
	case WP_M7:
		weapon_gpg40_fire( ent, ent->s.weapon );
		break;
	case WP_AIRSTRIKE:
		if ( level.time - ent->client->ps.classWeaponTime >= g_LTChargeTime.integer ) {
			if ( level.time - ent->client->ps.classWeaponTime > g_LTChargeTime.integer ) {
				ent->client->ps.classWeaponTime = level.time - g_LTChargeTime.integer;
			}
			ent->client->ps.classWeaponTime = level.time; //+= g_LTChargeTime.integer*0.5f; FIXME later
			weapon_grenadelauncher_fire( ent,WP_AIRSTRIKE );
		}
		break;
	case WP_SNIPERRIFLE:
		Bullet_Fire( ent, SNIPER_SPREAD * aimSpreadScale, SNIPER_DAMAGE(isPlayer) );
		if ( !ent->aiCharacter ) {
			VectorCopy( ent->client->ps.viewangles,viewang );
			ent->client->sniperRifleMuzzleYaw = crandom() * ammoTable[WP_SNIPERRIFLE].weapRecoilYaw[0]; // used in clientthink
			ent->client->sniperRifleMuzzlePitch = ammoTable[WP_SNIPERRIFLE].weapRecoilPitch[0];
			ent->client->sniperRifleFiredTime = level.time;
			SetClientViewAngle( ent,viewang );
		}
		break;
		
	case WP_SNOOPERSCOPE:
		Bullet_Fire( ent, SNOOPER_SPREAD * aimSpreadScale, SNOOPER_DAMAGE(isPlayer)  );
		if ( !ent->aiCharacter ) {
			VectorCopy( ent->client->ps.viewangles,viewang );
			ent->client->sniperRifleMuzzleYaw = crandom() * ammoTable[WP_SNOOPERSCOPE].weapRecoilYaw[0]; // used in clientthink
			ent->client->sniperRifleMuzzlePitch = ammoTable[WP_SNOOPERSCOPE].weapRecoilPitch[0];
			ent->client->sniperRifleFiredTime = level.time;
			SetClientViewAngle( ent,viewang );
		}
		break;
	case WP_MAUSER:
		Bullet_Fire( ent, MAUSER_SPREAD * aimSpreadScale, MAUSER_DAMAGE(isPlayer) );
		break;
	case WP_DELISLE:
		Bullet_Fire( ent, DELISLE_SPREAD * aimSpreadScale, DELISLE_DAMAGE(isPlayer) );
		break;
	case WP_DELISLESCOPE:
		Bullet_Fire( ent, DELISLESCOPE_SPREAD * aimSpreadScale, DELISLESCOPE_DAMAGE(isPlayer) );
		if ( !ent->aiCharacter ) {
			VectorCopy( ent->client->ps.viewangles,viewang );
			ent->client->sniperRifleMuzzleYaw = crandom() * ammoTable[WP_DELISLESCOPE].weapRecoilYaw[0]; // used in clientthink
			ent->client->sniperRifleMuzzlePitch = ammoTable[WP_DELISLESCOPE].weapRecoilPitch[0];
			ent->client->sniperRifleFiredTime = level.time;
			SetClientViewAngle( ent,viewang );
		}
		break;
	case WP_GARAND:
		Bullet_Fire( ent, GARAND_SPREAD * aimSpreadScale, GARAND_DAMAGE(isPlayer) );
		break;
	case WP_FG42SCOPE:
		Bullet_Fire( ent, FG42SCOPE_SPREAD*aimSpreadScale, FG42SCOPE_DAMAGE(isPlayer)  ); 
		if ( !ent->aiCharacter ) {
			VectorCopy( ent->client->ps.viewangles,viewang );
			ent->client->sniperRifleMuzzleYaw = crandom() * ammoTable[WP_FG42SCOPE].weapRecoilYaw[0]; // used in clientthink
			ent->client->sniperRifleMuzzlePitch = ammoTable[WP_FG42SCOPE].weapRecoilPitch[0];
			ent->client->sniperRifleFiredTime = level.time;
			SetClientViewAngle( ent,viewang );
		}
	    break; 
	case WP_FG42:
		Bullet_Fire( ent, FG42_SPREAD * aimSpreadScale, FG42_DAMAGE(isPlayer) );
		break;
	case WP_STEN:
		Bullet_Fire( ent, STEN_SPREAD * aimSpreadScale, STEN_DAMAGE(isPlayer)  );
		break;
	case WP_MP40:
		Bullet_Fire( ent, MP40_SPREAD * aimSpreadScale, MP40_DAMAGE(isPlayer)  );
		break;
	case WP_MP34: 
		Bullet_Fire( ent, MP34_SPREAD * aimSpreadScale, MP34_DAMAGE(isPlayer)  );
		break;
	case WP_TT33:
	case WP_DUAL_TT33:
		Bullet_Fire( ent, TT33_SPREAD * aimSpreadScale, TT33_DAMAGE(isPlayer)  );
		break;
	case WP_P38:
		Bullet_Fire( ent, P38_SPREAD * aimSpreadScale, P38_DAMAGE(isPlayer)  );
		break;
	case WP_HDM:
		Bullet_Fire( ent, HDM_SPREAD * aimSpreadScale, HDM_DAMAGE(isPlayer) );
		break;
	case WP_REVOLVER:
		Bullet_Fire( ent, REVOLVER_SPREAD * aimSpreadScale, REVOLVER_DAMAGE(isPlayer) );
		break;
	case WP_PPSH: 
		Bullet_Fire( ent, PPSH_SPREAD * aimSpreadScale, PPSH_DAMAGE(isPlayer) );
		break;
	case WP_MOSIN: 
		Bullet_Fire( ent, MOSIN_SPREAD * aimSpreadScale, MOSIN_DAMAGE(isPlayer)  );
		break;
	case WP_G43: 
		Bullet_Fire( ent, G43_SPREAD * aimSpreadScale, G43_DAMAGE(isPlayer)  );
		break;
	case WP_M1941: 
		Bullet_Fire( ent, M1941_SPREAD * aimSpreadScale, M1941_DAMAGE(isPlayer)  );
		break;
	case WP_M1941SCOPE:
		Bullet_Fire( ent, M1941SCOPE_SPREAD * aimSpreadScale, M1941SCOPE_DAMAGE (isPlayer) );
		if ( !ent->aiCharacter ) {
			VectorCopy( ent->client->ps.viewangles,viewang );
			ent->client->sniperRifleMuzzleYaw = crandom() * ammoTable[WP_M1941SCOPE].weapRecoilYaw[0]; // used in clientthink
			ent->client->sniperRifleMuzzlePitch = ammoTable[WP_M1941SCOPE].weapRecoilPitch[0];
			ent->client->sniperRifleFiredTime = level.time;
			SetClientViewAngle( ent,viewang );
		}
		break;
	case WP_M1GARAND: 
		Bullet_Fire( ent, M1GARAND_SPREAD * aimSpreadScale, M1GARAND_DAMAGE(isPlayer)  );
		break;
	case WP_BAR: 
		Bullet_Fire( ent, BAR_SPREAD * aimSpreadScale, BAR_DAMAGE(isPlayer) );
		break;
	case WP_MP44: 
		Bullet_Fire( ent, MP44_SPREAD * aimSpreadScale, MP44_DAMAGE(isPlayer) );
		break;
	case WP_MG42M: 
	case WP_BROWNING:
		Bullet_Fire( ent, MG42M_SPREAD * 0.6f * aimSpreadScale, MG42M_DAMAGE(isPlayer) );
		if (!ent->aiCharacter) {
		vec3_t vec_forward, vec_vangle;
		VectorCopy(ent->client->ps.viewangles, vec_vangle);
		vec_vangle[PITCH] = 0;	
		AngleVectors(vec_vangle, vec_forward, NULL, NULL);
		if (ent->s.groundEntityNum == ENTITYNUM_NONE)
			VectorMA(ent->client->ps.velocity, -8, vec_forward, ent->client->ps.velocity);
		else
			VectorMA(ent->client->ps.velocity, -24, vec_forward, ent->client->ps.velocity);
		}
		break;
	
		case WP_M97:
		Bullet_Fire(ent, M97_SPREAD* aimSpreadScale, M97_DAMAGE(isPlayer) );
		Bullet_Fire(ent, M97_SPREAD* aimSpreadScale, M97_DAMAGE(isPlayer) );
		Bullet_Fire(ent, M97_SPREAD* aimSpreadScale, M97_DAMAGE(isPlayer) );
		Bullet_Fire(ent, M97_SPREAD* aimSpreadScale, M97_DAMAGE(isPlayer) );
		Bullet_Fire(ent, M97_SPREAD* aimSpreadScale, M97_DAMAGE(isPlayer) );
		Bullet_Fire(ent, M97_SPREAD* aimSpreadScale, M97_DAMAGE(isPlayer) );
		Bullet_Fire(ent, M97_SPREAD* aimSpreadScale, M97_DAMAGE(isPlayer) );
		Bullet_Fire(ent, M97_SPREAD* aimSpreadScale, M97_DAMAGE(isPlayer) );
		Bullet_Fire(ent, M97_SPREAD* aimSpreadScale, M97_DAMAGE(isPlayer) );
		Bullet_Fire(ent, M97_SPREAD* aimSpreadScale, M97_DAMAGE(isPlayer) );
		Bullet_Fire(ent, M97_SPREAD* aimSpreadScale, M97_DAMAGE(isPlayer) );
		Bullet_Fire(ent, M97_SPREAD* aimSpreadScale, M97_DAMAGE(isPlayer) );
		if (!ent->aiCharacter) {
			vec3_t vec_forward, vec_vangle;
			VectorCopy(ent->client->ps.viewangles, vec_vangle);
			vec_vangle[PITCH] = 0;	// nullify pitch so you can't lightning jump
			AngleVectors(vec_vangle, vec_forward, NULL, NULL);
			 // make it less if in the air
			if (ent->s.groundEntityNum == ENTITYNUM_NONE)
				VectorMA(ent->client->ps.velocity, -8, vec_forward, ent->client->ps.velocity);
			else
				VectorMA(ent->client->ps.velocity, -24, vec_forward, ent->client->ps.velocity);
		}
		break;

		case WP_AUTO5:
		Bullet_Fire(ent, AUTO5_SPREAD* aimSpreadScale, AUTO5_DAMAGE(isPlayer) );
		Bullet_Fire(ent, AUTO5_SPREAD* aimSpreadScale, AUTO5_DAMAGE(isPlayer) );
		Bullet_Fire(ent, AUTO5_SPREAD* aimSpreadScale, AUTO5_DAMAGE(isPlayer) );
		Bullet_Fire(ent, AUTO5_SPREAD* aimSpreadScale, AUTO5_DAMAGE(isPlayer) );
		Bullet_Fire(ent, AUTO5_SPREAD* aimSpreadScale, AUTO5_DAMAGE(isPlayer) );
		Bullet_Fire(ent, AUTO5_SPREAD* aimSpreadScale, AUTO5_DAMAGE(isPlayer) );
		Bullet_Fire(ent, AUTO5_SPREAD* aimSpreadScale, AUTO5_DAMAGE(isPlayer) );
		Bullet_Fire(ent, AUTO5_SPREAD* aimSpreadScale, AUTO5_DAMAGE(isPlayer) );
		Bullet_Fire(ent, AUTO5_SPREAD* aimSpreadScale, AUTO5_DAMAGE(isPlayer) );
		Bullet_Fire(ent, AUTO5_SPREAD* aimSpreadScale, AUTO5_DAMAGE(isPlayer) );
		Bullet_Fire(ent, AUTO5_SPREAD* aimSpreadScale, AUTO5_DAMAGE(isPlayer) );
		Bullet_Fire(ent, AUTO5_SPREAD* aimSpreadScale, AUTO5_DAMAGE(isPlayer) );
		if (!ent->aiCharacter) {
			vec3_t vec_forward, vec_vangle;
			VectorCopy(ent->client->ps.viewangles, vec_vangle);
			vec_vangle[PITCH] = 0;	// nullify pitch so you can't lightning jump
			AngleVectors(vec_vangle, vec_forward, NULL, NULL);
			 // make it less if in the air
			if (ent->s.groundEntityNum == ENTITYNUM_NONE)
				VectorMA(ent->client->ps.velocity, -8, vec_forward, ent->client->ps.velocity);
			else
				VectorMA(ent->client->ps.velocity, -24, vec_forward, ent->client->ps.velocity);
		}
		break;

		case WP_M30:
		Bullet_Fire(ent, M30_SPREAD* aimSpreadScale, M30_DAMAGE(isPlayer) );
		Bullet_Fire(ent, M30_SPREAD* aimSpreadScale, M30_DAMAGE(isPlayer) );
		Bullet_Fire(ent, M30_SPREAD* aimSpreadScale, M30_DAMAGE(isPlayer) );
		Bullet_Fire(ent, M30_SPREAD* aimSpreadScale, M30_DAMAGE(isPlayer) );
		Bullet_Fire(ent, M30_SPREAD* aimSpreadScale, M30_DAMAGE(isPlayer) );
		Bullet_Fire(ent, M30_SPREAD* aimSpreadScale, M30_DAMAGE(isPlayer) );
		Bullet_Fire(ent, M30_SPREAD* aimSpreadScale, M30_DAMAGE(isPlayer) );
		Bullet_Fire(ent, M30_SPREAD* aimSpreadScale, M30_DAMAGE(isPlayer) );
		Bullet_Fire(ent, M30_SPREAD* aimSpreadScale, M30_DAMAGE(isPlayer) );
		Bullet_Fire(ent, M30_SPREAD* aimSpreadScale, M30_DAMAGE(isPlayer) );
		Bullet_Fire(ent, M30_SPREAD* aimSpreadScale, M30_DAMAGE(isPlayer) );
		Bullet_Fire(ent, M30_SPREAD* aimSpreadScale, M30_DAMAGE(isPlayer) );
		if (!ent->aiCharacter) {
			vec3_t vec_forward, vec_vangle;
			VectorCopy(ent->client->ps.viewangles, vec_vangle);
			vec_vangle[PITCH] = 0;	// nullify pitch so you can't lightning jump
			AngleVectors(vec_vangle, vec_forward, NULL, NULL);
			 // make it less if in the air
			if (ent->s.groundEntityNum == ENTITYNUM_NONE)
				VectorMA(ent->client->ps.velocity, -8, vec_forward, ent->client->ps.velocity);
			else
				VectorMA(ent->client->ps.velocity, -24, vec_forward, ent->client->ps.velocity);
		}
		break;
	

	case WP_THOMPSON:
		Bullet_Fire( ent, THOMPSON_SPREAD * aimSpreadScale, THOMPSON_DAMAGE(isPlayer) );
		break;
	case WP_PANZERFAUST:
		Weapon_RocketLauncher_Fire( ent, aimSpreadScale );
		break;
	case WP_GRENADE_LAUNCHER:
	case WP_GRENADE_PINEAPPLE:
	case WP_DYNAMITE:
	case WP_POISONGAS:
		weapon_grenadelauncher_fire( ent, ent->s.weapon );
		break;
	case WP_FLAMETHROWER:
		// RF, this is done client-side only now
		//Weapon_LightningFire( ent );
		break;
	case WP_TESLA:
		// push the player back a bit
		if ( !ent->aiCharacter ) {
			vec3_t forward, vangle;
			VectorCopy( ent->client->ps.viewangles, vangle );
			vangle[PITCH] = 0;  // nullify pitch so you can't lightning jump
			AngleVectors( vangle, forward, NULL, NULL );
			// make it less if in the air
			if ( ent->s.groundEntityNum == ENTITYNUM_NONE ) {
				VectorMA( ent->client->ps.velocity, -32, forward, ent->client->ps.velocity );
			} else {
				VectorMA( ent->client->ps.velocity, -100, forward, ent->client->ps.velocity );
			}
		}
		break;

	case WP_HOLYCROSS:
		break;
	case WP_MONSTER_ATTACK1:
		switch ( ent->aiCharacter ) {
		case AICHAR_WARZOMBIE:
			break;
		case AICHAR_ZOMBIE:
			// temp just to show it works
			// G_Printf("ptoo\n");
			weapon_zombiespit( ent );
			break;
		default:
			//G_Printf( "FireWeapon: unknown ai weapon: %s attack1\n", ent->classname );
			// ??? bug ???
			break;
		}

	case WP_MORTAR:
		break;

	default:
// FIXME		G_Error( "Bad ent->s.weapon" );
		break;
	}

	// Ridah
	// DHM - Nerve :: Only in single player
		AICast_RecordWeaponFire( ent );
}


// Load ammo parameters from .weap file
void G_LoadAmmoTable( weapon_t weaponNum )
{
	char *filename;
	int handle;
	pc_token_t token;

	filename = BG_GetWeaponFilename( weaponNum );
	if ( !*filename )
		return;
    
    if ( g_vanilla_guns.integer ) 
	{
	    handle = trap_PC_LoadSource( va( "weapons/vanilla/%s", filename ) );
	} else {
		handle = trap_PC_LoadSource( va( "weapons/%s", filename ) );
	}


	if ( !handle ) {
		//G_Printf( S_COLOR_RED "ERROR: Failed to load weap file %s\n", filename );
		return;
	}

	// Find and parse ammo block in this file
	while ( 1 ) {
		if ( !trap_PC_ReadToken( handle, &token ) ) {
			break;
		}
		if ( !Q_stricmp( token.string, "ammo" ) ) {
			BG_ParseAmmoTable( handle, weaponNum );
			break;
		}
	}

	trap_PC_FreeSource( handle );
}
