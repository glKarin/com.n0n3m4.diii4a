/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
//
// cg_weapons.c -- events and effects dealing with weapons
#include "cg_local.h"

extern void CG_PlayerFlashlight(centity_t *cent); //karin: missing decl
/*
==========================
CG_MachineGunEjectBrass
==========================
*/
static void CG_MachineGunEjectBrass( centity_t *cent ) {
	localEntity_t	*le;
	refEntity_t		*re;
	vec3_t			velocity, xvelocity;
	vec3_t			offset, xoffset;
	float			waterScale = 1.0f;
	vec3_t			v[3];

	if ( cg_brassTime.integer <= 0 ) {
		return;
	}

	le = CG_AllocLocalEntity();
	re = &le->refEntity;

	velocity[0] = 0;
	velocity[1] = -50 + 40 * crandom();
	velocity[2] = 100 + 50 * crandom();

	le->leType = LE_FRAGMENT;
	le->startTime = cg.time;
	le->endTime = le->startTime + cg_brassTime.integer + ( cg_brassTime.integer / 4 ) * random();

	le->pos.trType = TR_GRAVITY;
	le->pos.trTime = cg.time - (rand()&15);

	AnglesToAxis( cent->lerpAngles, v );

	offset[0] = 8;
	offset[1] = -4;
	offset[2] = 24;

	xoffset[0] = offset[0] * v[0][0] + offset[1] * v[1][0] + offset[2] * v[2][0];
	xoffset[1] = offset[0] * v[0][1] + offset[1] * v[1][1] + offset[2] * v[2][1];
	xoffset[2] = offset[0] * v[0][2] + offset[1] * v[1][2] + offset[2] * v[2][2];
	VectorAdd( cent->lerpOrigin, xoffset, re->origin );

	VectorCopy( re->origin, le->pos.trBase );

	if ( CG_PointContents( re->origin, -1 ) & CONTENTS_WATER ) {
		waterScale = 0.10f;
	}

	xvelocity[0] = velocity[0] * v[0][0] + velocity[1] * v[1][0] + velocity[2] * v[2][0];
	xvelocity[1] = velocity[0] * v[0][1] + velocity[1] * v[1][1] + velocity[2] * v[2][1];
	xvelocity[2] = velocity[0] * v[0][2] + velocity[1] * v[1][2] + velocity[2] * v[2][2];
	VectorScale( xvelocity, waterScale, le->pos.trDelta );

	AxisCopy( axisDefault, re->axis );
	re->hModel = cgs.media.machinegunBrassModel;

	le->bounceFactor = 0.4 * waterScale;

	le->angles.trType = TR_LINEAR;
	le->angles.trTime = cg.time;
	le->angles.trBase[0] = rand()&31;
	le->angles.trBase[1] = rand()&31;
	le->angles.trBase[2] = rand()&31;
	le->angles.trDelta[0] = 2;
	le->angles.trDelta[1] = 1;
	le->angles.trDelta[2] = 0;

	le->leFlags = LEF_TUMBLE;
	le->leBounceSoundType = LEBS_BRASS;
	le->leMarkType = LEMT_NONE;
}

/*
==========================
CG_ShotgunEjectBrass
==========================
*/
static void CG_ShotgunEjectBrass( centity_t *cent ) {
	localEntity_t	*le;
	refEntity_t		*re;
	vec3_t			velocity, xvelocity;
	vec3_t			offset, xoffset;
	vec3_t			v[3];
	int				i;

	if ( cg_brassTime.integer <= 0 ) {
		return;
	}

	for ( i = 0; i < 2; i++ ) {
		float	waterScale = 1.0f;

		le = CG_AllocLocalEntity();
		re = &le->refEntity;

		velocity[0] = 60 + 60 * crandom();
		if ( i == 0 ) {
			velocity[1] = 40 + 10 * crandom();
		} else {
			velocity[1] = -40 + 10 * crandom();
		}
		velocity[2] = 100 + 50 * crandom();

		le->leType = LE_FRAGMENT;
		le->startTime = cg.time;
		le->endTime = le->startTime + cg_brassTime.integer*3 + cg_brassTime.integer * random();

		le->pos.trType = TR_GRAVITY;
		le->pos.trTime = cg.time;

		AnglesToAxis( cent->lerpAngles, v );

		offset[0] = 8;
		offset[1] = 0;
		offset[2] = 24;

		xoffset[0] = offset[0] * v[0][0] + offset[1] * v[1][0] + offset[2] * v[2][0];
		xoffset[1] = offset[0] * v[0][1] + offset[1] * v[1][1] + offset[2] * v[2][1];
		xoffset[2] = offset[0] * v[0][2] + offset[1] * v[1][2] + offset[2] * v[2][2];
		VectorAdd( cent->lerpOrigin, xoffset, re->origin );
		VectorCopy( re->origin, le->pos.trBase );
		if ( CG_PointContents( re->origin, -1 ) & CONTENTS_WATER ) {
			waterScale = 0.10f;
		}

		xvelocity[0] = velocity[0] * v[0][0] + velocity[1] * v[1][0] + velocity[2] * v[2][0];
		xvelocity[1] = velocity[0] * v[0][1] + velocity[1] * v[1][1] + velocity[2] * v[2][1];
		xvelocity[2] = velocity[0] * v[0][2] + velocity[1] * v[1][2] + velocity[2] * v[2][2];
		VectorScale( xvelocity, waterScale, le->pos.trDelta );

		AxisCopy( axisDefault, re->axis );
		re->hModel = cgs.media.shotgunBrassModel;
		le->bounceFactor = 0.3f;

		le->angles.trType = TR_LINEAR;
		le->angles.trTime = cg.time;
		le->angles.trBase[0] = rand()&31;
		le->angles.trBase[1] = rand()&31;
		le->angles.trBase[2] = rand()&31;
		le->angles.trDelta[0] = 1;
		le->angles.trDelta[1] = 0.5;
		le->angles.trDelta[2] = 0;

		le->leFlags = LEF_TUMBLE;
		le->leBounceSoundType = LEBS_SHELL; // LEILEI shell noises
		le->leMarkType = LEMT_NONE;
	}
}



/*
==========================
CG_NailgunEjectBrass
==========================
*/
static void CG_NailgunEjectBrass( centity_t *cent ) {
	localEntity_t	*smoke;
	vec3_t			origin;
	vec3_t			v[3];
	vec3_t			offset;
	vec3_t			xoffset;
	vec3_t			up;

	AnglesToAxis( cent->lerpAngles, v );

	offset[0] = 0;
	offset[1] = -12;
	offset[2] = 24;

	xoffset[0] = offset[0] * v[0][0] + offset[1] * v[1][0] + offset[2] * v[2][0];
	xoffset[1] = offset[0] * v[0][1] + offset[1] * v[1][1] + offset[2] * v[2][1];
	xoffset[2] = offset[0] * v[0][2] + offset[1] * v[1][2] + offset[2] * v[2][2];
	VectorAdd( cent->lerpOrigin, xoffset, origin );

	VectorSet( up, 0, 0, 64 );

	smoke = CG_SmokePuff( origin, up, 32, 1, 1, 1, 0.33f, 700, cg.time, 0, 0, cgs.media.smokePuffShader );
	// use the optimized local entity add
	smoke->leType = LE_SCALE_FADE;
}

/*
==========================
CG_RailTrail
==========================
*/
void CG_RailTrail (clientInfo_t *ci, vec3_t start, vec3_t end) {
	vec3_t axis[36], move, move2, next_move, vec, temp;
	float  len;
	int    i, j, skip;
 
	localEntity_t *le;
	refEntity_t   *re;
 
#define RADIUS   4
#define ROTATION 1
#define SPACING  5
 
	start[2] -= 4;
 
	le = CG_AllocLocalEntity();
	re = &le->refEntity;
 
	le->leType = LE_FADE_RGB;
	le->startTime = cg.time;
	le->endTime = cg.time + cg_railTrailTime.value;
	le->lifeRate = 1.0 / (le->endTime - le->startTime);
 
	re->shaderTime = cg.time / 1000.0f;
	re->reType = RT_RAIL_CORE;
	re->customShader = cgs.media.railCoreShader;
 
	VectorCopy(start, re->origin);
	VectorCopy(end, re->oldorigin);
 
	re->shaderRGBA[0] = ci->color1[0] * 255;
	re->shaderRGBA[1] = ci->color1[1] * 255;
	re->shaderRGBA[2] = ci->color1[2] * 255;
	re->shaderRGBA[3] = 255;

	le->color[0] = ci->color1[0] * 0.75;
	le->color[1] = ci->color1[1] * 0.75;
	le->color[2] = ci->color1[2] * 0.75;
	le->color[3] = 1.0f;

	AxisClear( re->axis );
 
	// nudge down a bit so it isn't exactly in center
	re->origin[2] -= 8;
	re->oldorigin[2] -= 8;
	return;
}

/*
==========================
CG_PhysgunTrail
==========================
*/
void CG_PhysgunTrail (clientInfo_t *ci, vec3_t start, vec3_t end) {
	vec3_t move, move2, next_move, vec, temp;
	float  len;
 
	localEntity_t *le;
	refEntity_t   *re;
 
	le = CG_AllocLocalEntity();
	re = &le->refEntity;
 
	le->leType = LE_FADE_RGB;
	le->startTime = cg.time;
	le->endTime = cg.time + 60;	//Physgun DELAY
	le->lifeRate = 1.0 / (le->endTime - le->startTime);
 
	re->shaderTime = cg.time / 60.0f;
	re->reType = RT_RAIL_CORE;
	re->customShader = cgs.media.railCoreShader;
 
	VectorCopy(start, re->origin);
	VectorCopy(end, re->oldorigin);
 
	re->shaderRGBA[0] = ci->pg_red;
	re->shaderRGBA[1] = ci->pg_green;
	re->shaderRGBA[2] = ci->pg_blue;
	re->shaderRGBA[3] = 255;

	le->color[0] = (ci->pg_red / 255);
	le->color[1] = (ci->pg_green / 255);
	le->color[2] = (ci->pg_blue / 255);
	le->color[3] = 1.0f;

	AxisClear( re->axis );

	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec);
	PerpendicularVector(temp, vec);

	VectorMA(move, 20, vec, move);
	VectorCopy(move, next_move);
	VectorScale (vec, SPACING, vec);
}

/*
==========================
CG_GravitygunTrail
==========================
*/
void CG_GravitygunTrail (clientInfo_t *ci, vec3_t start, vec3_t end) {
	vec3_t move, move2, next_move, vec, temp;
	float  len;
 
	localEntity_t *le;
	refEntity_t   *re;
 
	le = CG_AllocLocalEntity();
	re = &le->refEntity;
 
	le->leType = LE_FADE_RGB;
	le->startTime = cg.time;
	le->endTime = cg.time + 60;	//Physgun DELAY
	le->lifeRate = 1.0 / (le->endTime - le->startTime);
 
	re->shaderTime = cg.time / 60.0f;
	re->reType = RT_RAIL_CORE;
	re->customShader = cgs.media.railCoreShader;
 
	VectorCopy(start, re->origin);
	VectorCopy(end, re->oldorigin);
 
	re->shaderRGBA[0] = 120.0f;
	re->shaderRGBA[1] = 60.0f;
	re->shaderRGBA[2] = 0.0f;
	re->shaderRGBA[3] = 255;

	le->color[0] = (120.0f / 255);
	le->color[1] = (60.0f / 255);
	le->color[2] = (0.0f / 255);
	le->color[3] = 1.0f;

	AxisClear( re->axis );

	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec);
	PerpendicularVector(temp, vec);

	VectorMA(move, 20, vec, move);
	VectorCopy(move, next_move);
	VectorScale (vec, SPACING, vec);
}

/*
==========================
CG_LeiSmokeTrail 
==========================
*/

static void CG_LeiSmokeTrail( centity_t *ent, const weaponInfo_t *wi ) {
	int		step;
	vec3_t	origin, lastPos;
	int		t;
	int		startTime, contents;
	int		lastContents;
	entityState_t	*es;
	vec3_t	up;
	localEntity_t	*smoke;
	int		therando;
	int		theradio;

	if ( cg_noProjectileTrail.integer ) {
		return;
	}

	up[0] = 5 - 10 * crandom();
	up[1] = 5 - 10 * crandom();
	up[2] = 8 - 5 * crandom();

	step = 18;

	es = &ent->currentState;
	startTime = ent->trailTime;
	t = step * ( (startTime + step) / step );

	BG_EvaluateTrajectory( &es->pos, cg.time, origin );
	contents = CG_PointContents( origin, -1 );

	// if object (e.g. grenade) is stationary, don't toss up smoke
	if ( es->pos.trType == TR_STATIONARY ) {
		ent->trailTime = cg.time;
		return;
	}

	BG_EvaluateTrajectory( &es->pos, ent->trailTime, lastPos );
	lastContents = CG_PointContents( lastPos, -1 );

	ent->trailTime = cg.time;

	if ( contents & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ) {
		if ( contents & lastContents & CONTENTS_WATER ) {
			CG_BubbleTrail( lastPos, origin, 8 );
		}
		return;
	}

	for ( ; t <= ent->trailTime ; t += step ) {
		BG_EvaluateTrajectory( &es->pos, t, lastPos );
		therando = crandom() * 4;
		
		theradio =  wi->trailRadius * (rand() * 0.7); // what is this doing here
	if (therando == 3)		smoke = CG_SmokePuff( lastPos, up, 27, 1, 1, 1, 0.9f, wi->wiTrailTime,  t, 0, 0,  cgs.media.lsmkShader1 );
	else if (therando == 1)		smoke = CG_SmokePuff( lastPos, up, 27, 1, 1, 1, 0.9f, wi->wiTrailTime,  t, 0, 0,  cgs.media.lsmkShader2 );
	else	if (therando == 2)	smoke = CG_SmokePuff( lastPos, up, 27, 1, 1, 1, 0.9f, wi->wiTrailTime,  t, 0, 0,  cgs.media.lsmkShader3 );
	else				smoke = CG_SmokePuff( lastPos, up, 27, 1, 1, 1, 0.9f, wi->wiTrailTime,  t, 0, 0,  cgs.media.lsmkShader4 );
		// use the optimized local entity add
		smoke->leType = LE_MOVE_SCALE_FADE;
		//smoke->trType = TR_GRAVITY;
	}

}


static void CG_LeiPlasmaTrail( centity_t *ent, const weaponInfo_t *wi ) {
	int		step;
	vec3_t	origin, lastPos;
	int		t;
	int		startTime, contents;
	int		lastContents;
	entityState_t	*es;
	vec3_t	up;
	localEntity_t	*smoke;

	if ( cg_noProjectileTrail.integer ) {
		return;
	}

	up[0] = 0;
	up[1] = 0;
	up[2] = 0;

	step = 16;

	es = &ent->currentState;
	startTime = ent->trailTime;
	t = step * ( (startTime + step) / step );

	BG_EvaluateTrajectory( &es->pos, cg.time, origin );
	contents = CG_PointContents( origin, -1 );

	// if object (e.g. grenade) is stationary, don't toss up smoke
	if ( es->pos.trType == TR_STATIONARY ) {
		ent->trailTime = cg.time;
		return;
	}

	BG_EvaluateTrajectory( &es->pos, ent->trailTime, lastPos );
	lastContents = CG_PointContents( lastPos, -1 );

	ent->trailTime = cg.time;

	if ( contents & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ) {
		return;
	}

	for ( ; t <= ent->trailTime ; t += step ) {
		BG_EvaluateTrajectory( &es->pos, t, lastPos );

		smoke = CG_SmokePuff( lastPos, up, 27, 1, 1, 1, 0.9f, wi->wiTrailTime,  t, 0, 0,  cgs.media.lsmkShader1 );
		// use the optimized local entity add
		smoke->leType = LE_SCALE_FADE;
		//smoke->trType = TR_GRAVITY;
	}

}

/*
==========================
CG_NailTrail
==========================
*/
static void CG_NailTrail( centity_t *ent, const weaponInfo_t *wi ) {
	int		step;
	vec3_t	origin, lastPos;
	int		t;
	int		startTime, contents;
	int		lastContents;
	entityState_t	*es;
	vec3_t	up;
	localEntity_t	*smoke;

	if ( cg_noProjectileTrail.integer ) {
		return;
	}

	up[0] = 0;
	up[1] = 0;
	up[2] = 0;

	step = 50;

	es = &ent->currentState;
	startTime = ent->trailTime;
	t = step * ( (startTime + step) / step );

	BG_EvaluateTrajectory( &es->pos, cg.time, origin );
	contents = CG_PointContents( origin, -1 );

	// if object (e.g. grenade) is stationary, don't toss up smoke
	if ( es->pos.trType == TR_STATIONARY ) {
		ent->trailTime = cg.time;
		return;
	}

	BG_EvaluateTrajectory( &es->pos, ent->trailTime, lastPos );
	lastContents = CG_PointContents( lastPos, -1 );

	ent->trailTime = cg.time;

	if ( contents & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ) {
		if ( contents & lastContents & CONTENTS_WATER ) {
			CG_BubbleTrail( lastPos, origin, 8 );
		}
		return;
	}

	for ( ; t <= ent->trailTime ; t += step ) {
		BG_EvaluateTrajectory( &es->pos, t, lastPos );

		smoke = CG_SmokePuff( lastPos, up, 
					  wi->trailRadius, 
					  1, 1, 1, 0.33f,
					  wi->wiTrailTime, 
					  t,
					  0,
					  0, 
					  cgs.media.nailPuffShader );
		// use the optimized local entity add
		smoke->leType = LE_SCALE_FADE;
	}

}

/*
==========================
CG_GrappleTrail
==========================
*/
void CG_GrappleTrail( centity_t *ent, const weaponInfo_t *wi ) {
	vec3_t	origin;
	entityState_t	*es;
	vec3_t			forward, up;
	refEntity_t		beam;

	es = &ent->currentState;

	BG_EvaluateTrajectory( &es->pos, cg.time, origin );
	ent->trailTime = cg.time;

	memset( &beam, 0, sizeof( beam ) );
	//FIXME adjust for muzzle position
	VectorCopy ( cg_entities[ ent->currentState.otherEntityNum ].lerpOrigin, beam.origin );
	beam.origin[2] += 26;
	AngleVectors( cg_entities[ ent->currentState.otherEntityNum ].lerpAngles, forward, NULL, up );
	VectorMA( beam.origin, -6, up, beam.origin );
	VectorCopy( origin, beam.oldorigin );

	if (Distance( beam.origin, beam.oldorigin ) < 64 )
		return; // Don't draw if close

	beam.reType = RT_RAIL_CORE;
	beam.customShader = cgs.media.grappleShader;

	AxisClear( beam.axis );
	beam.shaderRGBA[0] = 0xff;
	beam.shaderRGBA[1] = 0xff;
	beam.shaderRGBA[2] = 0xff;
	beam.shaderRGBA[3] = 0xff;
	trap_R_AddRefEntityToScene( &beam );
}

/*
==========================
CG_GrenadeTrail
==========================
*/
// LEILEI enhancment
static void CG_RocketTrail( centity_t *ent, const weaponInfo_t *wi ) {
	CG_LeiSmokeTrail( ent, wi );
}

static void CG_PlasmaTrail( centity_t *ent, const weaponInfo_t *wi ) {
	CG_LeiPlasmaTrail( ent, wi );
}


static void CG_GrenadeTrail( centity_t *ent, const weaponInfo_t *wi ) {
	CG_RocketTrail( ent, wi );
}

/*
=================
CG_RegisterWeapon

The server says this item is used on this level
=================
*/
void CG_RegisterWeapon( int weaponNum ) {
	weaponInfo_t	*weaponInfo;
	gitem_t			*item, *ammo;
	char			path[MAX_QPATH];
	vec3_t			mins, maxs;
	int				i;

	weaponInfo = &cg_weapons[weaponNum];

	if ( weaponNum == 0 ) {
		return;
	}

	if ( weaponInfo->registered ) {
		return;
	}

	memset( weaponInfo, 0, sizeof( *weaponInfo ) );
	weaponInfo->registered = qtrue;

	for ( item = bg_itemlist + 1 ; item->classname ; item++ ) {
		if ( item->giType == IT_WEAPON && item->giTag == weaponNum ) {
			weaponInfo->item = item;
			break;
		}
	}
	
	if ( !item->classname ) {
		CG_Error( "Couldn't find weapon %i", weaponNum );
		return;
	}
	CG_RegisterItemVisuals( item - bg_itemlist );

	// load cmodel before model so filecache works
	weaponInfo->weaponModel = trap_R_RegisterModel_SourceTech( item->world_model[0] );

	// calc midpoint for rotation
	trap_R_ModelBounds( weaponInfo->weaponModel, mins, maxs );
	for ( i = 0 ; i < 3 ; i++ ) {
		weaponInfo->weaponMidpoint[i] = mins[i] + 0.5 * ( maxs[i] - mins[i] );
	}

	weaponInfo->weaponIcon = trap_R_RegisterShader( item->icon );
	weaponInfo->ammoIcon = trap_R_RegisterShader( item->icon );

	for ( ammo = bg_itemlist + 1 ; ammo->classname ; ammo++ ) {
		if ( ammo->giType == IT_AMMO && ammo->giTag == weaponNum ) {
			break;
		}
	}
	if ( ammo->classname && ammo->world_model[0] ) {
		weaponInfo->ammoModel = trap_R_RegisterModel_SourceTech( ammo->world_model[0] );
	}

	Q_strncpyz( path, item->world_model[0], MAX_QPATH );
	COM_StripExtension(path, path, sizeof(path));
	strcat( path, "_flash.md3" );
	weaponInfo->flashModel = trap_R_RegisterModel_SourceTech( path );

	Q_strncpyz( path, item->world_model[0], MAX_QPATH );
	COM_StripExtension(path, path, sizeof(path));
	strcat( path, "_barrel.md3" );
	weaponInfo->barrelModel = trap_R_RegisterModel_SourceTech( path );

	Q_strncpyz( path, item->world_model[0], MAX_QPATH );
	COM_StripExtension(path, path, sizeof(path));
	strcat( path, "_hand.md3" );
	weaponInfo->handsModel = trap_R_RegisterModel_SourceTech( path );

	if ( !weaponInfo->handsModel ) {
		weaponInfo->handsModel = trap_R_RegisterModel_SourceTech( "models/weapons2/shotgun/shotgun_hand.md3" );
	}

	weaponInfo->loopFireSound = qfalse;

	switch ( weaponNum ) {					//look for this to add new ones - WEAPONS_HYPER
	case WP_GAUNTLET:
		//MAKERGB( weaponInfo->flashDlightColor, 0.6f, 0.6f, 1.0f );
		//weaponInfo->readySound = trap_S_RegisterSound_SourceTech( "sound/weapons/bfg/bfg_humd.wav", qfalse );
		weaponInfo->firingSound = trap_S_RegisterSound_SourceTech( "sound/weapons/melee/fstrun.wav", qfalse );
		weaponInfo->flashSound[0] = trap_S_RegisterSound_SourceTech( "sound/weapons/melee/fstatck.wav", qfalse );
		break;

	case WP_LIGHTNING:
		MAKERGB( weaponInfo->flashDlightColor, 0.6f, 0.6f, 1.0f );
		weaponInfo->readySound = trap_S_RegisterSound_SourceTech( "sound/weapons/melee/fsthum.wav", qfalse );
		weaponInfo->firingSound = trap_S_RegisterSound_SourceTech( "sound/weapons/lightning/lg_hum.wav", qfalse );

		weaponInfo->flashSound[0] = trap_S_RegisterSound_SourceTech( "sound/weapons/lightning/lg_fire.wav", qfalse );
		cgs.media.lightningShader = trap_R_RegisterShader( "lightningBoltNew");
		cgs.media.lightningExplosionModel = trap_R_RegisterModel_SourceTech( "models/weaphits/crackle.md3" );
		cgs.media.sfx_lghit1 = trap_S_RegisterSound_SourceTech( "sound/weapons/lightning/lg_hit.wav", qfalse );
		cgs.media.sfx_lghit2 = trap_S_RegisterSound_SourceTech( "sound/weapons/lightning/lg_hit2.wav", qfalse );
		cgs.media.sfx_lghit3 = trap_S_RegisterSound_SourceTech( "sound/weapons/lightning/lg_hit3.wav", qfalse );

		break;

	case WP_GRAPPLING_HOOK:
		MAKERGB( weaponInfo->flashDlightColor, 0.6f, 0.6f, 1.0f );
		weaponInfo->missileModel = trap_R_RegisterModel_SourceTech( "models/ammo/hook/hook.md3" );
		weaponInfo->missileTrailFunc = CG_GrappleTrail;
		weaponInfo->missileDlight = 0;
		weaponInfo->wiTrailTime = 2000;
		weaponInfo->trailRadius = 64;
		MAKERGB( weaponInfo->missileDlightColor, 1, 0.75f, 0 );
		cgs.media.grappleShader = trap_R_RegisterShader( "grappleRope");
		weaponInfo->flashSound[0] = trap_S_RegisterSound_SourceTech( "sound/weapons/grapple/grapfire.wav", qfalse );
		weaponInfo->missileSound = trap_S_RegisterSound_SourceTech( "sound/weapons/grapple/grappull.wav", qfalse );
        //cgs.media.lightningShader = trap_R_RegisterShader( "lightningBoltNew");
		break;


	case WP_CHAINGUN:
		weaponInfo->firingSound = trap_S_RegisterSound_SourceTech( "sound/weapons/vulcan/wvulfire.wav", qfalse );
		weaponInfo->loopFireSound = qtrue;
		MAKERGB( weaponInfo->flashDlightColor, 1, 1, 0 );
		weaponInfo->flashSound[0] = trap_S_RegisterSound_SourceTech( "sound/weapons/vulcan/vulcanf1b.wav", qfalse );
		weaponInfo->flashSound[1] = trap_S_RegisterSound_SourceTech( "sound/weapons/vulcan/vulcanf2b.wav", qfalse );
		weaponInfo->flashSound[2] = trap_S_RegisterSound_SourceTech( "sound/weapons/vulcan/vulcanf3b.wav", qfalse );
		weaponInfo->flashSound[3] = trap_S_RegisterSound_SourceTech( "sound/weapons/vulcan/vulcanf4b.wav", qfalse );
		weaponInfo->ejectBrassFunc = CG_MachineGunEjectBrass;
		cgs.media.bulletExplosionShader = trap_R_RegisterShader( "bulletExplosion" );
		break;


	case WP_MACHINEGUN:
		MAKERGB( weaponInfo->flashDlightColor, 1, 1, 0 );
		weaponInfo->flashSound[0] = trap_S_RegisterSound_SourceTech( "sound/weapons/machinegun/machgf1b.wav", qfalse );
		weaponInfo->flashSound[1] = trap_S_RegisterSound_SourceTech( "sound/weapons/machinegun/machgf2b.wav", qfalse );
		weaponInfo->flashSound[2] = trap_S_RegisterSound_SourceTech( "sound/weapons/machinegun/machgf3b.wav", qfalse );
		weaponInfo->flashSound[3] = trap_S_RegisterSound_SourceTech( "sound/weapons/machinegun/machgf4b.wav", qfalse );
		weaponInfo->ejectBrassFunc = CG_MachineGunEjectBrass;
		cgs.media.bulletExplosionShader = trap_R_RegisterShader( "bulletExplosion" );
		break;

	case WP_SHOTGUN:
		MAKERGB( weaponInfo->flashDlightColor, 1, 1, 0 );
		weaponInfo->flashSound[0] = trap_S_RegisterSound_SourceTech( "sound/weapons/shotgun/sshotf1b.wav", qfalse );
		weaponInfo->ejectBrassFunc = CG_ShotgunEjectBrass;
		break;

	case WP_ROCKET_LAUNCHER:
		weaponInfo->missileModel = trap_R_RegisterModel_SourceTech( "models/ammo/rocket/rocket.md3" );
		weaponInfo->missileSound = trap_S_RegisterSound_SourceTech( "sound/weapons/rocket/rockfly.wav", qfalse );
		weaponInfo->missileTrailFunc = CG_RocketTrail;
		weaponInfo->missileDlight = 200;
		weaponInfo->wiTrailTime = 2000;
		weaponInfo->trailRadius = 64;
		
		MAKERGB( weaponInfo->missileDlightColor, 1, 0.75f, 0 );
		MAKERGB( weaponInfo->flashDlightColor, 1, 0.75f, 0 );

		weaponInfo->flashSound[0] = trap_S_RegisterSound_SourceTech( "sound/weapons/rocket/rocklf1a.wav", qfalse );
		cgs.media.rocketExplosionShader = trap_R_RegisterShader( "rocketExplosion" );
		break;

	case WP_PROX_LAUNCHER:
		weaponInfo->missileModel = trap_R_RegisterModel_SourceTech( "models/weaphits/proxmine.md3" );
		weaponInfo->missileTrailFunc = CG_GrenadeTrail;
		weaponInfo->wiTrailTime = 700;
		weaponInfo->trailRadius = 32;
		MAKERGB( weaponInfo->flashDlightColor, 1, 0.70f, 0 );
		weaponInfo->flashSound[0] = trap_S_RegisterSound_SourceTech( "sound/weapons/proxmine/wstbfire.wav", qfalse );
		cgs.media.grenadeExplosionShader = trap_R_RegisterShader( "grenadeExplosion" );
		break;

	case WP_GRENADE_LAUNCHER:
		weaponInfo->missileModel = trap_R_RegisterModel_SourceTech( "models/ammo/grenade1.md3" );
		weaponInfo->missileTrailFunc = CG_GrenadeTrail;
		weaponInfo->wiTrailTime = 700;
		weaponInfo->trailRadius = 32;
		MAKERGB( weaponInfo->flashDlightColor, 1, 0.70f, 0 );
		weaponInfo->flashSound[0] = trap_S_RegisterSound_SourceTech( "sound/weapons/grenade/grenlf1a.wav", qfalse );
		cgs.media.grenadeExplosionShader = trap_R_RegisterShader( "grenadeExplosion" );
		break;


	case WP_NAILGUN:
		weaponInfo->ejectBrassFunc = CG_NailgunEjectBrass;
		weaponInfo->missileTrailFunc = CG_NailTrail;
//		weaponInfo->missileSound = trap_S_RegisterSound_SourceTech( "sound/weapons/nailgun/wnalflit.wav", qfalse );
		weaponInfo->trailRadius = 16;
		weaponInfo->wiTrailTime = 250;
		weaponInfo->missileModel = trap_R_RegisterModel_SourceTech( "models/weaphits/nail.md3" );
		MAKERGB( weaponInfo->flashDlightColor, 1, 0.75f, 0 );
		weaponInfo->flashSound[0] = trap_S_RegisterSound_SourceTech( "sound/weapons/nailgun/wnalfire.wav", qfalse );
		break;


	case WP_PLASMAGUN:
//		weaponInfo->missileModel = cgs.media.invulnerabilityPowerupModel;
		weaponInfo->missileTrailFunc = CG_PlasmaTrail;
		weaponInfo->missileSound = trap_S_RegisterSound_SourceTech( "sound/weapons/plasma/lasfly.wav", qfalse );
		MAKERGB( weaponInfo->flashDlightColor, 0.6f, 0.6f, 1.0f );
		weaponInfo->flashSound[0] = trap_S_RegisterSound_SourceTech( "sound/weapons/plasma/hyprbf1a.wav", qfalse );
		cgs.media.plasmaExplosionShader = trap_R_RegisterShader( "plasmaExplosion" );
		cgs.media.railRingsShader = trap_R_RegisterShader( "railDisc" );
		break;

	case WP_RAILGUN:
		weaponInfo->readySound = trap_S_RegisterSound_SourceTech( "sound/weapons/railgun/rg_hum.wav", qfalse );
		MAKERGB( weaponInfo->flashDlightColor, 1, 0.5f, 0 );
		weaponInfo->flashSound[0] = trap_S_RegisterSound_SourceTech( "sound/weapons/railgun/railgf1a.wav", qfalse );
		cgs.media.railExplosionShader = trap_R_RegisterShader( "railExplosion" );
		cgs.media.railRingsShader = trap_R_RegisterShader( "railDisc" );
		cgs.media.railCoreShader = trap_R_RegisterShader( "railCore" );
		break;

	case WP_BFG:
		weaponInfo->readySound = trap_S_RegisterSound_SourceTech( "sound/weapons/bfg/bfg_hum.wav", qfalse );
		MAKERGB( weaponInfo->flashDlightColor, 1, 0.7f, 1 );
		MAKERGB( weaponInfo->missileDlightColor, 0.40f, 1, 0.20f );
		weaponInfo->missileDlight = 200;
		weaponInfo->flashSound[0] = trap_S_RegisterSound_SourceTech( "sound/weapons/bfg/bfg_fire.wav", qfalse );
		cgs.media.bfgExplosionShader = trap_R_RegisterShader( "bfgExplosion" );
		weaponInfo->missileModel = trap_R_RegisterModel_SourceTech( "models/weaphits/bfg.md3" );
		weaponInfo->missileSound = trap_S_RegisterSound_SourceTech( "sound/weapons/rocket/rockfly.wav", qfalse );
		break;
		
	case WP_FLAMETHROWER:
//		weaponInfo->missileModel = cgs.media.invulnerabilityPowerupModel;
		weaponInfo->missileSound = trap_S_RegisterSound_SourceTech( "sound/weapons/flamethrower/lasfly.wav", qfalse );
		MAKERGB( weaponInfo->flashDlightColor, 1.0f, 1.0f, 1.0f );
		weaponInfo->flashSound[0] = trap_S_RegisterSound_SourceTech( "sound/weapons/flamethrower/hyprbf1a.wav", qfalse );
		cgs.media.plasmaExplosionShader = trap_R_RegisterShader( "plasmaExplosion" );
		cgs.media.railRingsShader = trap_R_RegisterShader( "railDisc" );
		break;
		
	case WP_ANTIMATTER:
//		weaponInfo->missileModel = cgs.media.invulnerabilityPowerupModel;
		weaponInfo->missileSound = trap_S_RegisterSound_SourceTech( "sound/weapons/antimatter/lasfly.wav", qfalse );
		MAKERGB( weaponInfo->flashDlightColor, 1.0f, 1.0f, 1.0f );
		weaponInfo->flashSound[0] = trap_S_RegisterSound_SourceTech( "sound/weapons/antimatter/hyprbf1a.wav", qfalse );
		cgs.media.plasmaExplosionShader = trap_R_RegisterShader( "plasmaExplosion" );
		cgs.media.railRingsShader = trap_R_RegisterShader( "railDisc" );
		break;
		
	case WP_TOOLGUN:
		//weaponInfo->missileModel = cgs.media.invulnerabilityPowerupModel;
		//weaponInfo->missileSound = trap_S_RegisterSound_SourceTech( "sound/weapons/antimatter/lasfly.wav", qfalse );
		//MAKERGB( weaponInfo->flashDlightColor, 1.0f, 1.0f, 1.0f );
		weaponInfo->flashSound[0] = trap_S_RegisterSound_SourceTech( "sound/weapons/toolgun/fire1.wav", qfalse );
		weaponInfo->flashSound[1] = trap_S_RegisterSound_SourceTech( "sound/weapons/toolgun/fire2.wav", qfalse );
		cgs.media.plasmaExplosionShader = trap_R_RegisterShader( "plasmaExplosion" );
		cgs.media.lightningShader = trap_R_RegisterShader( "lightningBoltNew");
		//cgs.media.railRingsShader = trap_R_RegisterShader( "railDisc" );
		break;

	case WP_PHYSGUN:
		weaponInfo->flashSound[0] = trap_S_RegisterSound_SourceTech( "sound/misc/silence.wav", qfalse );
		cgs.media.plasmaExplosionShader = trap_R_RegisterShader( "plasmaExplosion" );
		cgs.media.lightningShader = trap_R_RegisterShader( "lightningBoltNew");
		break;
		
	case WP_GRAVITYGUN:
		weaponInfo->flashSound[0] = trap_S_RegisterSound_SourceTech( "sound/misc/silence.wav", qfalse );
		cgs.media.plasmaExplosionShader = trap_R_RegisterShader( "plasmaExplosion" );
		cgs.media.lightningShader = trap_R_RegisterShader( "lightningBoltNew");
		break;

	case WP_THROWER:
		MAKERGB( weaponInfo->flashDlightColor, 1, 1, 0 );
		weaponInfo->flashSound[0] = trap_S_RegisterSound_SourceTech( "sound/weapons3/machinegun/machgf1b.wav", qfalse );
		weaponInfo->flashSound[1] = trap_S_RegisterSound_SourceTech( "sound/weapons3/machinegun/machgf2b.wav", qfalse );
		weaponInfo->flashSound[2] = trap_S_RegisterSound_SourceTech( "sound/weapons3/machinegun/machgf3b.wav", qfalse );
		weaponInfo->flashSound[3] = trap_S_RegisterSound_SourceTech( "sound/weapons3/machinegun/machgf4b.wav", qfalse );
		weaponInfo->ejectBrassFunc = CG_MachineGunEjectBrass;
		break;

	case WP_BOUNCER:
		MAKERGB( weaponInfo->flashDlightColor, 1, 1, 0 );
		weaponInfo->flashSound[0] = trap_S_RegisterSound_SourceTech( "sound/weapons3/shotgun/sshotf1b.wav", qfalse );
		weaponInfo->ejectBrassFunc = CG_ShotgunEjectBrass;
		break;

	case WP_THUNDER:
		MAKERGB( weaponInfo->flashDlightColor, 1, 0.70f, 0 );
		weaponInfo->flashSound[0] = trap_S_RegisterSound_SourceTech( "sound/weapons3/grenade/grenlf1a.wav", qfalse );
		break;

	case WP_EXPLODER:
		weaponInfo->missileModel = trap_R_RegisterModel_SourceTech( "models/ammo/rocket/rocket.md3" );
		weaponInfo->missileSound = trap_S_RegisterSound_SourceTech( "sound/weapons3/rocket/rockfly.wav", qfalse );
		weaponInfo->missileTrailFunc = CG_RocketTrail;
		weaponInfo->missileDlight = 200;
		weaponInfo->wiTrailTime = 2000;
		weaponInfo->trailRadius = 64;
		
		MAKERGB( weaponInfo->missileDlightColor, 1, 0.75f, 0 );
		MAKERGB( weaponInfo->flashDlightColor, 1, 0.75f, 0 );

		weaponInfo->flashSound[0] = trap_S_RegisterSound_SourceTech( "sound/weapons3/rocket/rocklf1a.wav", qfalse );
		cgs.media.rocketExplosionShader = trap_R_RegisterShader( "rocketExplosion" );
		break;

	case WP_KNOCKER:
		MAKERGB( weaponInfo->flashDlightColor, 0.6f, 0.6f, 1.0f );
		weaponInfo->readySound = trap_S_RegisterSound_SourceTech( "sound/weapons3/melee/fsthum.wav", qfalse );
		weaponInfo->firingSound = trap_S_RegisterSound_SourceTech( "sound/weapons3/lightning/lg_hum.wav", qfalse );

		weaponInfo->flashSound[0] = trap_S_RegisterSound_SourceTech( "sound/weapons3/lightning/lg_fire.wav", qfalse );
		cgs.media.lightningShader = trap_R_RegisterShader( "lightningBoltNew");
		cgs.media.lightningExplosionModel = trap_R_RegisterModel_SourceTech( "models/weaphits/crackle.md3" );
		cgs.media.sfx_lghit1 = trap_S_RegisterSound_SourceTech( "sound/weapons3/lightning/lg_hit.wav", qfalse );
		cgs.media.sfx_lghit2 = trap_S_RegisterSound_SourceTech( "sound/weapons3/lightning/lg_hit2.wav", qfalse );
		cgs.media.sfx_lghit3 = trap_S_RegisterSound_SourceTech( "sound/weapons3/lightning/lg_hit3.wav", qfalse );
		break;

	case WP_PROPGUN:
		MAKERGB( weaponInfo->flashDlightColor, 1.0f, 1.0f, 1.0f );
		weaponInfo->flashSound[0] = trap_S_RegisterSound_SourceTech( "sound/weapons3/railgun/railgf1a.wav", qfalse );
		break;

	case WP_REGENERATOR:
		MAKERGB( weaponInfo->flashDlightColor, 1.0f, 1.0f, 1.0f );
		weaponInfo->flashSound[0] = trap_S_RegisterSound_SourceTech( "sound/weapons/antimatter/hyprbf1a.wav", qfalse );
		cgs.media.plasmaExplosionShader = trap_R_RegisterShader( "plasmaExplosion" );
		cgs.media.railRingsShader = trap_R_RegisterShader( "railDisc" );
		break;

	case WP_NUKE:
		weaponInfo->readySound = trap_S_RegisterSound_SourceTech( "sound/weapons/bfg/bfg_hum.wav", qfalse );
		MAKERGB( weaponInfo->flashDlightColor, 1, 0.7f, 1 );
		MAKERGB( weaponInfo->missileDlightColor, 0.40f, 1, 0.20f );
		weaponInfo->missileDlight = 200;
		weaponInfo->flashSound[0] = trap_S_RegisterSound_SourceTech( "sound/weapons/bfg/bfg_fire.wav", qfalse );
		cgs.media.bfgExplosionShader = trap_R_RegisterShader( "bfgExplosion" );
		weaponInfo->missileModel = trap_R_RegisterModel_SourceTech( "models/weaphits/bfg.md3" );
		weaponInfo->missileSound = trap_S_RegisterSound_SourceTech( "sound/weapons/rocket/rockfly.wav", qfalse );
		break;
		
	 default:
		MAKERGB( weaponInfo->flashDlightColor, 1, 1, 1 );
		weaponInfo->flashSound[0] = trap_S_RegisterSound_SourceTech( "sound/weapons/rocket/rocklf1a.wav", qfalse );
		break;
	}
}

/*
=================
CG_RegisterItemVisuals

The server says this item is used on this level
=================
*/
void CG_RegisterItemVisuals( int itemNum ) {
	itemInfo_t		*itemInfo;
	gitem_t			*item;

	if ( itemNum < 0 || itemNum >= bg_numItems ) {
		CG_Error( "CG_RegisterItemVisuals: itemNum %d out of range [0-%d]", itemNum, bg_numItems-1 );
	}

	itemInfo = &cg_items[ itemNum ];
	if ( itemInfo->registered ) {
		return;
	}

	item = &bg_itemlist[ itemNum ];

	memset( itemInfo, 0, sizeof( &itemInfo ) );
	itemInfo->registered = qtrue;

	itemInfo->models[0] = trap_R_RegisterModel_SourceTech( item->world_model[0] );

	itemInfo->icon = trap_R_RegisterShader( item->icon );

	if ( item->giType == IT_WEAPON ) {
		CG_RegisterWeapon( item->giTag );
	}

	//
	// powerups have an accompanying ring or sphere
	//
	if ( item->giType == IT_POWERUP || item->giType == IT_HEALTH || 
		item->giType == IT_ARMOR || item->giType == IT_HOLDABLE ) {
		if ( item->world_model[1] ) {
			itemInfo->models[1] = trap_R_RegisterModel_SourceTech( item->world_model[1] );
		}
	}
}


/*
========================================================================================

VIEW WEAPON

========================================================================================
*/

/*
=================
CG_MapTorsoToWeaponFrame

=================
*/
static int CG_MapTorsoToWeaponFrame( clientInfo_t *ci, int frame ) {

	// change weapon
	if ( frame >= ci->animations[TORSO_DROP].firstFrame 
		&& frame < ci->animations[TORSO_DROP].firstFrame + 9 ) {
		return frame - ci->animations[TORSO_DROP].firstFrame + 6;
	}

	// stand attack
	if ( frame >= ci->animations[TORSO_ATTACK].firstFrame 
		&& frame < ci->animations[TORSO_ATTACK].firstFrame + 6 ) {
		return 1 + frame - ci->animations[TORSO_ATTACK].firstFrame;
	}

	// stand attack 2
	if ( frame >= ci->animations[TORSO_ATTACK2].firstFrame 
		&& frame < ci->animations[TORSO_ATTACK2].firstFrame + 6 ) {
		return 1 + frame - ci->animations[TORSO_ATTACK2].firstFrame;
	}
	
	return 0;
}


/*
==============
CG_CalculateWeaponPosition
==============
*/
static void CG_CalculateWeaponPosition( vec3_t origin, vec3_t angles ) {
	float	scale;
	int		delta;
	float	fracsin;

	VectorCopy( cg.refdef.vieworg, origin );
	VectorCopy( cg.refdefViewAngles, angles );

	// on odd legs, invert some angles
	if ( cg.bobcycle & 1 ) {
		scale = -cg.xyspeed;
	} else {
		scale = cg.xyspeed;
	}

	// gun angles from bobbing
	angles[ROLL] += scale * cg.bobfracsin * 0.005;
	angles[YAW] += scale * cg.bobfracsin * 0.01;
	angles[PITCH] += cg.xyspeed * cg.bobfracsin * 0.005;

	// drop the weapon when landing
	delta = cg.time - cg.landTime;
	if ( delta < LAND_DEFLECT_TIME ) {
		origin[2] += cg.landChange*0.25 * delta / LAND_DEFLECT_TIME;
	} else if ( delta < LAND_DEFLECT_TIME + LAND_RETURN_TIME ) {
		origin[2] += cg.landChange*0.25 * 
			(LAND_DEFLECT_TIME + LAND_RETURN_TIME - delta) / LAND_RETURN_TIME;
	}

#if 0
	// drop the weapon when stair climbing
	delta = cg.time - cg.stepTime;
	if ( delta < STEP_TIME/2 ) {
		origin[2] -= cg.stepChange*0.25 * delta / (STEP_TIME/2);
	} else if ( delta < STEP_TIME ) {
		origin[2] -= cg.stepChange*0.25 * (STEP_TIME - delta) / (STEP_TIME/2);
	}
#endif

	// idle drift
	scale = cg.xyspeed + 40;
	fracsin = sin( cg.time * 0.001 );
	angles[ROLL] += scale * fracsin * 0.01;
	angles[YAW] += scale * fracsin * 0.01;
	angles[PITCH] += scale * fracsin * 0.01;
}


/*
===============
CG_LightningBolt

Origin will be the exact tag point, which is slightly
different than the muzzle point used for determining hits.
The cent should be the non-predicted cent if it is from the player,
so the endpoint will reflect the simulated strike (lagging the predicted
angle)
===============
*/
static void CG_LightningBolt( centity_t *cent, vec3_t origin ) {
	trace_t  trace;
	refEntity_t  beam;
	vec3_t   forward;
	vec3_t   muzzlePoint, endPoint;
	clientInfo_t	*ci;
	int				weaphack;

	ci = &cgs.clientinfo[ cent->currentState.clientNum ];
	
	weaphack = ci->swepid;

	if (weaphack != WP_LIGHTNING && weaphack != WP_TOOLGUN && weaphack != WP_PHYSGUN && weaphack != WP_GRAVITYGUN) {
		return;
	}

	memset( &beam, 0, sizeof( beam ) );

//unlagged - attack prediction #1
	// if the entity is us, unlagged is on server-side, and we've got it on for the lightning gun
	if ( (cent->currentState.number == cg.predictedPlayerState.clientNum) && cgs.delagHitscan &&
			( cg_delag.integer & 1 || cg_delag.integer & 8 ) ) {
		// always shoot straight forward from our current position
		AngleVectors( cg.predictedPlayerState.viewangles, forward, NULL, NULL );
		VectorCopy( cg.predictedPlayerState.origin, muzzlePoint );
	}
	else
//unlagged - attack prediction #1

	// CPMA  "true" lightning
    if ((cent->currentState.number == cg.predictedPlayerState.clientNum) && (cg_trueLightning.value != 0)) {
		vec3_t angle;
		int i;

//unlagged - true lightning
		// might as well fix up true lightning while we're at it
		vec3_t viewangles;
		VectorCopy( cg.predictedPlayerState.viewangles, viewangles );
//unlagged - true lightning

		for (i = 0; i < 3; i++) {
			float a = cent->lerpAngles[i] - cg.refdefViewAngles[i];
			if (a > 180) {
				a -= 360;
			}
			if (a < -180) {
				a += 360;
			}

			angle[i] = cg.refdefViewAngles[i] + a * (1.0 - cg_trueLightning.value);
			if (angle[i] < 0) {
				angle[i] += 360;
			}
			if (angle[i] > 360) {
				angle[i] -= 360;
			}
		}

		AngleVectors(angle, forward, NULL, NULL );
//unlagged - true lightning
//		VectorCopy(cent->lerpOrigin, muzzlePoint );
//		VectorCopy(cg.refdef.vieworg, muzzlePoint );
		// *this* is the correct origin for true lightning
		VectorCopy(cg.predictedPlayerState.origin, muzzlePoint );
//unlagged - true lightning
	} else {
		// !CPMA
		AngleVectors( cent->lerpAngles, forward, NULL, NULL );
		VectorCopy(cent->lerpOrigin, muzzlePoint );
	}

	// FIXME: crouch
	muzzlePoint[2] += DEFAULT_VIEWHEIGHT;

	VectorMA( muzzlePoint, 14, forward, muzzlePoint );

	// project forward by the lightning range
	if (weaphack == WP_LIGHTNING){
	VectorMA( muzzlePoint, mod_lgrange, forward, endPoint );
	} else if (weaphack == WP_TOOLGUN){
	VectorMA( muzzlePoint, TOOLGUN_RANGE, forward, endPoint );	
	} else if (weaphack == WP_PHYSGUN){
	VectorMA( muzzlePoint, PHYSGUN_RANGE, forward, endPoint );	
	} else if (weaphack == WP_GRAVITYGUN){
	VectorMA( muzzlePoint, GRAVITYGUN_RANGE, forward, endPoint );	
	}

	// see if it hit a wall
	CG_Trace( &trace, muzzlePoint, vec3_origin, vec3_origin, endPoint, 
		cent->currentState.number, MASK_SHOT );

	// this is the endpoint
	VectorCopy( trace.endpos, beam.oldorigin );

	// use the provided origin, even though it may be slightly
	// different than the muzzle origin
	VectorCopy( origin, beam.origin );
	
	if (weaphack == WP_PHYSGUN) {
		CG_PhysgunTrail (ci, origin, trace.endpos);
		return;
	}
	if (weaphack == WP_GRAVITYGUN) {
		CG_GravitygunTrail (ci, origin, trace.endpos);
		return;
	}

	beam.reType = RT_LIGHTNING;
	beam.customShader = cgs.media.lightningShader;
	trap_R_AddRefEntityToScene( &beam );

	// add the impact flare if it hit something
	if ( trace.fraction < 1.0 ) {
		vec3_t	angles;
		vec3_t	dir;

		VectorSubtract( beam.oldorigin, beam.origin, dir );
		VectorNormalize( dir );

		memset( &beam, 0, sizeof( beam ) );
		beam.hModel = cgs.media.lightningExplosionModel;

		VectorMA( trace.endpos, -16, dir, beam.origin );

		// make a random orientation
		angles[0] = rand() % 360;
		angles[1] = rand() % 360;
		angles[2] = rand() % 360;
		AnglesToAxis( angles, beam.axis );
		trap_R_AddRefEntityToScene( &beam );
	}
}

/*
===============
CG_SpawnRailTrail

Origin will be the exact tag point, which is slightly
different than the muzzle point used for determining hits.
===============
*/
static void CG_SpawnRailTrail( centity_t *cent, vec3_t origin ) {
	clientInfo_t	*ci;
	int				weaphack;

	ci = &cgs.clientinfo[ cent->currentState.clientNum ];

	weaphack = ci->swepid;

	if ( weaphack != WP_RAILGUN ) {
		return;
	}
	if ( !cent->pe.railgunFlash ) {
		return;
	}
	cent->pe.railgunFlash = qtrue;
	CG_RailTrail( ci, origin, cent->pe.railgunImpact );
}


/*
======================
CG_MachinegunSpinAngle
======================
*/
#define		SPIN_SPEED	0.9
#define		COAST_TIME	1000
static float	CG_MachinegunSpinAngle( centity_t *cent ) {
	int		delta;
	float	angle;
	float	speed;
	clientInfo_t	*ci;
	int				weaphack;

	ci = &cgs.clientinfo[ cent->currentState.clientNum ];

	weaphack = ci->swepid;

	delta = cg.time - cent->pe.barrelTime;
	if ( cent->pe.barrelSpinning ) {
		angle = cent->pe.barrelAngle + delta * SPIN_SPEED;
	} else {
		if ( delta > COAST_TIME ) {
			delta = COAST_TIME;
		}

		speed = 0.5 * ( SPIN_SPEED + (float)( COAST_TIME - delta ) / COAST_TIME );
		angle = cent->pe.barrelAngle + delta * speed;
	}

	if ( cent->pe.barrelSpinning == !(cent->currentState.eFlags & EF_FIRING) ) {
		cent->pe.barrelTime = cg.time;
		cent->pe.barrelAngle = AngleMod( angle );
		cent->pe.barrelSpinning = !!(cent->currentState.eFlags & EF_FIRING);

		if ( weaphack == WP_CHAINGUN && !cent->pe.barrelSpinning ) {
			trap_S_StartSound( NULL, cent->currentState.number, CHAN_WEAPON, trap_S_RegisterSound_SourceTech( "sound/weapons/vulcan/wvulwind.wav", qfalse ) );
		}

	}

	return angle;
}


/*
========================
CG_AddWeaponWithPowerups
========================
*/
static void CG_AddWeaponWithPowerups( refEntity_t *gun, int powerups ) {
	// add powerup effects
	if ( powerups & ( 1 << PW_INVIS ) ) {
            if( (cgs.dmflags & DF_INVIS) == 0) {
		gun->customShader = cgs.media.invisShader;
		trap_R_AddRefEntityToScene( gun );
            }
	} else {
		trap_R_AddRefEntityToScene( gun );

		if ( powerups & ( 1 << PW_BATTLESUIT ) ) {
			gun->customShader = cgs.media.battleWeaponShader;
			trap_R_AddRefEntityToScene( gun );
		}
		if ( powerups & ( 1 << PW_QUAD ) ) {
			gun->customShader = cgs.media.quadWeaponShader;
			trap_R_AddRefEntityToScene( gun );
		}
	}
}


/*
=============
CG_AddPlayerWeapon

Used for both the view weapon (ps is valid) and the world modelother character models (ps is NULL)
The main player will have this called for BOTH cases, so effects like light and
sound should only be done on the world model case.
=============
*/

void CG_AddPlayerWeapon( refEntity_t *parent, playerState_t *ps, centity_t *cent, int team, clientInfo_t *ci ) {
	refEntity_t	gun;
	refEntity_t	barrel;
	refEntity_t	flash;
	vec3_t		angles;
	int			weaponNum;
	weaponInfo_t	*weapon;
	centity_t	*nonPredictedCent;
	orientation_t	lerped;

	if ( cg.snap->ps.pm_type == PM_CUTSCENE ) {
		weaponNum = 1;
	} else {
		weaponNum = ci->swepid;
	}

	if(ci->flashlight == 1){
		CG_PlayerFlashlight( &cg_entities[cent->currentState.clientNum] );
	}

	CG_RegisterWeapon( weaponNum );
	weapon = &cg_weapons[weaponNum];

	// add the weapon
	memset( &gun, 0, sizeof( gun ) );
	VectorCopy( parent->lightingOrigin, gun.lightingOrigin );
	gun.shadowPlane = parent->shadowPlane;
	gun.renderfx = parent->renderfx;

	// set custom shading for railgun refire rate
	if ( ps || cent->currentState.clientNum == cg.predictedPlayerState.clientNum ) {
		if ( weaponNum == WP_RAILGUN 
			&& cg.predictedPlayerState.weaponstate == WEAPON_FIRING ) {
			float	f;

			f = (float)cg.predictedPlayerState.weaponTime / 1500;
			gun.shaderRGBA[1] = 0;
			gun.shaderRGBA[0] = 
			gun.shaderRGBA[2] = 255 * ( 1.0 - f );
		} else {
			gun.shaderRGBA[0] = 255;
			gun.shaderRGBA[1] = 100;
			gun.shaderRGBA[2] = 100;
			gun.shaderRGBA[3] = 255;
		}
	}
	
	if ( weaponNum == WP_PHYSGUN ){
		gun.shaderRGBA[0] = ci->pg_red;
		gun.shaderRGBA[1] = ci->pg_green;
		gun.shaderRGBA[2] = ci->pg_blue;
		gun.shaderRGBA[3] = 255;
	}
	if ( weaponNum == WP_GRAVITYGUN ){
		gun.shaderRGBA[0] = 120;
		gun.shaderRGBA[1] = 60;
		gun.shaderRGBA[2] = 0;
		gun.shaderRGBA[3] = 255;
	}

	gun.hModel = weapon->weaponModel;
	if (!gun.hModel) {
		return;
	}

	if ( !ps ) {
		// add weapon ready sound
		cent->pe.lightningFiring = qfalse;
		if ( ( cent->currentState.eFlags & EF_FIRING ) && weapon->firingSound ) {
			// lightning gun and guantlet make a different sound when fire is held down
			trap_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, weapon->firingSound );
			cent->pe.lightningFiring = qtrue;
		} else if ( weapon->readySound && cg.predictedPlayerState.pm_type != PM_CUTSCENE ) {	
			//note: the pm_cutscene check above makes weapon idle noises stop during cutscenes, but it does so for 
			//ALL weapons, including those of bots. Unfortunately this method is called without supplying ps for the 
			//player itself as well. So unfortunately, I cannot differentiate between bots and players which means that
			//either ALL hums play or NO hums play. I've chosen for the latter option during cutscenes.
			trap_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, weapon->readySound );
		}
	}

	trap_R_LerpTag(&lerped, parent->hModel, parent->oldframe, parent->frame,
		1.0 - parent->backlerp, "tag_weapon");
	VectorCopy(parent->origin, gun.origin);

	VectorMA(gun.origin, lerped.origin[0], parent->axis[0], gun.origin);

	// Make weapon appear left-handed for 2 and centered for 3
	if(ps && cg_drawGun.integer == 2)
		VectorMA(gun.origin, -lerped.origin[1], parent->axis[1], gun.origin);
	else if(!ps || cg_drawGun.integer != 3)
       	VectorMA(gun.origin, lerped.origin[1], parent->axis[1], gun.origin);

	VectorMA(gun.origin, lerped.origin[2], parent->axis[2], gun.origin);

	MatrixMultiply(lerped.axis, ((refEntity_t *)parent)->axis, gun.axis);

	gun.backlerp = parent->backlerp;

	CG_AddWeaponWithPowerups( &gun, cent->currentState.powerups );

	if ( weapon->barrelModel ) {
		memset( &barrel, 0, sizeof( barrel ) );
		VectorCopy( parent->lightingOrigin, barrel.lightingOrigin );
		barrel.shadowPlane = parent->shadowPlane;
		barrel.renderfx = parent->renderfx;

		barrel.hModel = weapon->barrelModel;
		angles[YAW] = 0;
		angles[PITCH] = 0;
		angles[ROLL] = CG_MachinegunSpinAngle( cent );
		AnglesToAxis( angles, barrel.axis );

		CG_PositionRotatedEntityOnTag( &barrel, &gun, weapon->weaponModel, "tag_barrel" );

		CG_AddWeaponWithPowerups( &barrel, cent->currentState.powerups );
	}

	// make sure we aren't looking at cg.predictedPlayerEntity for LG
	nonPredictedCent = &cg_entities[cent->currentState.clientNum];

	// if the index of the nonPredictedCent is not the same as the clientNum
	// then this is a fake player (like on teh single player podiums), so
	// go ahead and use the cent
	if( ( nonPredictedCent - cg_entities ) != cent->currentState.clientNum ) {
		nonPredictedCent = cent;
	}

	// add the flash
	if ( ( weaponNum == WP_LIGHTNING || weaponNum == WP_GAUNTLET || weaponNum == WP_GRAPPLING_HOOK || weaponNum == WP_PHYSGUN || weaponNum == WP_GRAVITYGUN )
		&& ( nonPredictedCent->currentState.eFlags & EF_FIRING ) ) 
	{
		// continuous flash
	} else {
		// impulse flash
		if ( cg.time - cent->muzzleFlashTime > MUZZLE_FLASH_TIME && !cent->pe.railgunFlash ) {
			return;
		}
	}

	memset( &flash, 0, sizeof( flash ) );
	VectorCopy( parent->lightingOrigin, flash.lightingOrigin );
	flash.shadowPlane = parent->shadowPlane;
	flash.renderfx = parent->renderfx;

	flash.hModel = weapon->flashModel;
	if (!flash.hModel) {
		return;
	}
	angles[YAW] = 0;
	angles[PITCH] = 0;
	angles[ROLL] = crandom() * 10;
	AnglesToAxis( angles, flash.axis );

	// colorize the railgun blast
	if ( weaponNum == WP_RAILGUN ) {
		clientInfo_t	*ci;

		ci = &cgs.clientinfo[ cent->currentState.clientNum ];
		flash.shaderRGBA[0] = 255 * ci->color1[0];
		flash.shaderRGBA[1] = 255 * ci->color1[1];
		flash.shaderRGBA[2] = 255 * ci->color1[2];
	}

	CG_PositionRotatedEntityOnTag( &flash, &gun, weapon->weaponModel, "tag_flash");
	trap_R_AddRefEntityToScene( &flash );

	if ( ps || cg.renderingThirdPerson ||
		cent->currentState.number != cg.predictedPlayerState.clientNum ) {
		// add lightning bolt
		CG_LightningBolt( nonPredictedCent, flash.origin );

		// add rail trail
		CG_SpawnRailTrail( cent, flash.origin );

		if ( weapon->flashDlightColor[0] || weapon->flashDlightColor[1] || weapon->flashDlightColor[2] ) {
			trap_R_AddLightToScene( flash.origin, 300 + (rand()&31), weapon->flashDlightColor[0],
			weapon->flashDlightColor[1], weapon->flashDlightColor[2] );
		}
	}
}

/*
==============
CG_AddViewWeapon

Add the weapon, and flash for the player's view
==============
*/
void CG_AddViewWeapon( playerState_t *ps ) {
	refEntity_t	hand;
	centity_t	*cent;
	clientInfo_t	*ci;
	float		fovOffset;
	vec3_t		angles;
	weaponInfo_t	*weapon;
	
	if ( ps->persistant[PERS_TEAM] == TEAM_SPECTATOR ) {
		return;
	}

	if ( ps->pm_type == PM_INTERMISSION || ps->pm_type == PM_CUTSCENE ) {
		return;
	}

	// no gun if in third person view or a camera is active
	//if ( cg.renderingThirdPerson || cg.cameraMode) {
	if ( cg.renderingThirdPerson ) {
		return;
	}
	if (cg.renderingEyesPerson) {
		return;
	}

	// allow the gun to be completely removed
	if ( !cg_drawGun.integer ) {
		vec3_t		origin;

		if ( cg.predictedPlayerState.eFlags & EF_FIRING ) {
			// special hack for lightning gun...
			VectorCopy( cg.refdef.vieworg, origin );
			VectorMA( origin, -8, cg.refdef.viewaxis[2], origin );
			CG_LightningBolt( &cg_entities[ps->clientNum], origin );
		}
		return;
	}

	// don't draw if testing a gun model
	if ( cg.testGun ) {
		return;
	}

	// drop gun lower at higher fov
	if ( cg_fov.integer > 90 ) {
		fovOffset = -0.2 * ( cg_fov.integer - 90 );
	} else {
		fovOffset = 0;
	}

	cent = &cg.predictedPlayerEntity;	// &cg_entities[cg.snap->ps.clientNum];
	CG_RegisterWeapon( ps->generic2 );
	weapon = &cg_weapons[ ps->generic2 ];

	memset (&hand, 0, sizeof(hand));

	// set up gun position
	CG_CalculateWeaponPosition( hand.origin, angles );

	VectorMA( hand.origin, cg_gun_x.value, cg.refdef.viewaxis[0], hand.origin );
	VectorMA( hand.origin, cg_gun_y.value, cg.refdef.viewaxis[1], hand.origin );
	VectorMA( hand.origin, (cg_gun_z.value+fovOffset), cg.refdef.viewaxis[2], hand.origin );

	AnglesToAxis( angles, hand.axis );

	// get clientinfo for animation map
	ci = &cgs.clientinfo[ cent->currentState.clientNum ];
	hand.frame = CG_MapTorsoToWeaponFrame( ci, cent->pe.torso.frame );
	hand.oldframe = CG_MapTorsoToWeaponFrame( ci, cent->pe.torso.oldFrame );
	hand.backlerp = cent->pe.torso.backlerp;

	hand.hModel = weapon->handsModel;
	hand.renderfx = RF_DEPTHHACK | RF_FIRST_PERSON ;

	// add everything onto the hand
	CG_AddPlayerWeapon( &hand, ps, &cg.predictedPlayerEntity, ps->persistant[PERS_TEAM], ci );
}

/*
==============================================================================

WEAPON SELECTION

==============================================================================
*/

/*
===================
CG_DrawWeaponSelect
===================
*/
void CG_DrawWeaponSelect( void ) {
	int		i;
	int		count;
	float		*color;
	vec4_t		realColor; 
	int			swepnum; 

	if ( cg.snap->ps.pm_flags & PMF_FOLLOW ) {
		return;
	}

	if ( cg_draw2D.integer == 0 ) {
		return;
	}

	swepnum = cg.snap->ps.generic2;
	// don't display if dead
	if ( cg.predictedPlayerState.stats[STAT_HEALTH] <= 0 ) {
		return;
	}

	if ( cg.showScores ){
		return;
	}

	color = CG_FadeColor( cg.weaponSelectTime, WEAPON_SELECT_TIME );

	//Elimination: Always show weapon bar
	if(cg_alwaysWeaponBar.integer) {
		realColor[0] = 1.0;
		realColor[1] = 1.0;
		realColor[2] = 1.0;
		realColor[3] = 1.0;
		color = realColor;
	}

	if ( !color ) {
		return;
	}
	trap_R_SetColor( color );

	count = 0;
	for ( i = 1; i < WEAPONS_NUM; i++ ) {
		if(cg.swep_listcl[i] >= 1){
			count++;
		}
	}
	
	CG_DrawWeaponBarNew2(count); //FOR VANILLA WEAPONS WEAPONS_HYPER
	trap_R_SetColor(NULL);
	return;
}

/*
===============
CG_DrawWeaponBarNew2
===============
*/

void CG_DrawWeaponBarNew2(int count){
	float scale = 0.60;
	int y = 4;
	int x = 320 - count * (20*scale);
	int i;
	
	trap_GetGlconfig( &cgs.glconfig );
	
	for ( i = 1; i <= WEAPONS_NUM; i++ ) {
		if(!cg.swep_listcl[i]){
		    continue;	
		}
		CG_RegisterWeapon( i );
		// draw weapon icon
		CG_DrawPic( x, y, 32*scale, 32*scale, cg_weapons[i].weaponIcon );

		// draw selection marker
		if ( i == cg.weaponSelect ) {
			//CG_DrawPic( x, y, 32*scale, 32*scale, cgs.media.selectShader );
			CG_DrawPic( x-(4*scale), y-(4*scale), 40*scale, 40*scale, cgs.media.selectShader );
		}

		// no ammo cross on top
		if( cg.swep_listcl[i] == 2 ){
			CG_DrawPic( x, y, 32*scale, 32*scale, cgs.media.noammoShader );
		}

		x += 40*scale;
	}
}

/*
===============
CG_NextWeapon_f
===============
*/
void CG_NextWeapon_f( void ) {
	int		i;
	int		original;

	if(BG_VehicleCheckClass(cg.snap->ps.stats[STAT_VEHICLE])){	//VEHICLE-SYSTEM: weapon lock for 1
		if(!BG_GetVehicleSettings(cg.snap->ps.stats[STAT_VEHICLE], VSET_WEAPON)){
			return;	
		}
	}
	if ( !cg.snap ) {
		return;
	}
	if ( cg.snap->ps.pm_flags & PMF_FOLLOW ) {
		return;
	}
	
	if ( cg.snap->ps.generic2 == WP_PHYSGUN ){
		if( cg.snap->ps.eFlags & EF_FIRING ){
			trap_SendConsoleCommand("physgun_dist 0\n");
			return;
		}
	}

	cg.weaponSelectTime = cg.time;
	original = cg.weaponSelect;

	for ( i = 1; i < WEAPONS_NUM; i++ ) {
	cg.weaponSelect++;
	if ( cg.weaponSelect > WEAPONS_NUM ) {
		cg.weaponSelect = 1;
	}
	if(cg.swep_listcl[cg.weaponSelect] == 1){
		break;
	}
	}
	
	if(cg.weaponSelect == WP_TOOLGUN){
	trap_Cvar_Set("cg_hide255", "0");
	} else {
	trap_Cvar_Set("cg_hide255", "1");
	}
}

/*
===============
CG_PrevWeapon_f
===============
*/
void CG_PrevWeapon_f( void ) {
	int		i;
	int		original;

	if(BG_VehicleCheckClass(cg.snap->ps.stats[STAT_VEHICLE])){	//VEHICLE-SYSTEM: weapon lock for 1
		if(!BG_GetVehicleSettings(cg.snap->ps.stats[STAT_VEHICLE], VSET_WEAPON)){
			return;	
		}
	}
	if ( !cg.snap ) {
		return;
	}
	if ( cg.snap->ps.pm_flags & PMF_FOLLOW ) {
		return;
	}

	if ( cg.snap->ps.generic2 == WP_PHYSGUN ){
		if( cg.snap->ps.eFlags & EF_FIRING ){
			trap_SendConsoleCommand("physgun_dist 1\n");
			return;
		}
	}

	cg.weaponSelectTime = cg.time;
	original = cg.weaponSelect;

	for ( i = 1 ; i < WEAPONS_NUM; i++ ) {
	cg.weaponSelect--;
	if ( cg.weaponSelect < 1 ) {
		cg.weaponSelect = WEAPONS_NUM;
	}
	if(cg.swep_listcl[cg.weaponSelect] == 1){
		break;
	}
	}
	
	if(cg.weaponSelect == WP_TOOLGUN){
	trap_Cvar_Set("cg_hide255", "0");
	} else {
	trap_Cvar_Set("cg_hide255", "1");
	}
}

/*
===============
CG_Weapon_f
===============
*/
void CG_Weapon_f( void ) {
	int		num;

	if ( !cg.snap ) {
		return;
	}
	if ( cg.snap->ps.pm_flags & PMF_FOLLOW ) {
		return;
	}

	num = atoi( CG_Argv( 1 ) );

	cg.weaponSelectTime = cg.time;
	if(!cg.swep_listcl[num]){
		return;		// don't have the weapon
	}

	cg.weaponSelect = num;
	
	if(cg.weaponSelect == WP_TOOLGUN){
	trap_Cvar_Set("cg_hide255", "0");
	} else {
	trap_Cvar_Set("cg_hide255", "1");
	}
}

/*
===================================================================================================

WEAPON EVENTS

===================================================================================================
*/

/*
================
CG_FireWeapon

Caused by an EV_FIRE_WEAPON event
================
*/
void CG_FireWeapon( centity_t *cent ) {
	clientInfo_t	*ci;
	entityState_t *ent;
	int				c;
	weaponInfo_t	*weap;
	int				weaphack;

	if((cgs.gametype == GT_ELIMINATION || cgs.gametype == GT_CTF_ELIMINATION) && cgs.roundStartTime>=cg.time)
		return; //if we havn't started in ELIMINATION then do not fire

	ent = &cent->currentState;
	
	ci = &cgs.clientinfo[ cent->currentState.clientNum ];
	
	if(ci->swepid >= 1){
	weaphack = ci->swepid;
	} else {
	weaphack = ent->weapon;
	}
	
	if ( weaphack == WP_NONE ) {
		return;
	}
	if ( weaphack > WEAPONS_NUM ) {
		CG_Error( "CG_FireWeapon: weaphack > WEAPONS_NUM" );
		return;
	}
	weap = &cg_weapons[ weaphack ];

	// mark the entity as muzzle flashing, so when it is added it will
	// append the flash to the weapon model
	cent->muzzleFlashTime = cg.time;

	// lightning gun only does this this on initial press
	if ( weaphack == WP_LIGHTNING ) {
		if ( cent->pe.lightningFiring ) {
			return;
		}
	}
	
	if ( weaphack == WP_TOOLGUN && cent->currentState.clientNum == cg.snap->ps.clientNum && cg.snap->ps.persistant[PERS_TEAM] != TEAM_SPECTATOR ){
		if(toolgun_mod19.integer == 0){
		trap_SendConsoleCommand("vstr toolgun_toolcmd1\n");
		} else
		if(toolgun_mod19.integer == 1){
		trap_SendConsoleCommand("vstr toolgun_toolcmd2\n");
		} else
		if(toolgun_mod19.integer == 2){
		trap_SendConsoleCommand("vstr toolgun_toolcmd3\n");
		} else
		if(toolgun_mod19.integer == 3){
		trap_SendConsoleCommand("vstr toolgun_toolcmd4\n");
		} else {
		trap_SendConsoleCommand("vstr toolgun_toolcmd1\n");
		}
	}

	// play quad sound if needed
	if ( cent->currentState.powerups & ( 1 << PW_QUAD ) ) {
		trap_S_StartSound (NULL, cent->currentState.number, CHAN_ITEM, cgs.media.quadSound );
	}

	// play a sound
	for ( c = 0 ; c < 4 ; c++ ) {
		if ( !weap->flashSound[c] ) {
			break;
		}
	}
	if ( c > 0 ) {
		c = rand() % c;
		if ( weap->flashSound[c] )
		{
			trap_S_StartSound( NULL, ent->number, CHAN_WEAPON, weap->flashSound[c] );
		}
	}

	// do brass ejection
	if ( weap->ejectBrassFunc && cg_brassTime.integer > 0 ) {
		weap->ejectBrassFunc( cent );
	}

//unlagged - attack prediction #1
	CG_PredictWeaponEffects( cent );
//unlagged - attack prediction #1
}

/*
=================
CG_MissileHitWall

Caused by an EV_MISSILE_MISS event, or directly by local bullet tracing
=================
*/
void CG_MissileHitWall( int weapon, int clientNum, vec3_t origin, vec3_t dir, impactSound_t soundType ) {
	qhandle_t		mod;
	qhandle_t		mark;
	qhandle_t		shader;
	sfxHandle_t		sfx;
	float			radius;
	float			light;
	vec3_t			lightColor;
	localEntity_t	*le;
	int				r;
	qboolean		alphaFade;
	qboolean		isSprite;
	int				duration;
	vec3_t			sprOrg;
	vec3_t			sprVel;
	
	//CG_Printf(va("CG_MissileHitWall: %i\n", weapon));

	mark = 0;
	radius = 32;
	sfx = 0;
	mod = 0;
	shader = 0;
	light = 0;
	lightColor[0] = 1;
	lightColor[1] = 1;
	lightColor[2] = 0;

	// set defaults
	isSprite = qfalse;
	duration = 600;

	switch ( weapon ) {
	default:

	case WP_NAILGUN:
		if( soundType == IMPACTSOUND_FLESH ) {
			sfx = cgs.media.sfx_nghitflesh;
		} else if( soundType == IMPACTSOUND_METAL ) {
			sfx = cgs.media.sfx_nghitmetal;
		} else {
			sfx = cgs.media.sfx_nghit;
		}
		mark = cgs.media.holeMarkShader;
		radius = 12;
		break;

	case WP_LIGHTNING:
		// no explosion at LG impact, it is added with the beam
		r = rand() & 3;
		if ( !cg_paintballMode.integer ) {
		if ( r < 2 ) {
			sfx = cgs.media.sfx_lghit2;
		} else if ( r == 2 ) {
			sfx = cgs.media.sfx_lghit1;
		} else {
			sfx = cgs.media.sfx_lghit3;
		}
		} else {
			if ( r < 2 ) {
				sfx = cgs.media.gibBounce1Sound;
			} else if ( r == 2 ) {
				sfx = cgs.media.gibBounce2Sound;
			} else {
				sfx = cgs.media.gibBounce3Sound;
			}
		}
		mark = cgs.media.holeMarkShader;
		radius = 12;
		break;

	case WP_PROX_LAUNCHER:
		mod = cgs.media.dishFlashModel;
		shader = cgs.media.grenadeExplosionShader;
		sfx = cgs.media.sfx_proxexp;
		mark = cgs.media.burnMarkShader;
		radius = 64;
		light = 300;
		isSprite = qtrue;
		// LEILEI START enhancement
		if (cg_leiEnhancement.integer) {
		// some more fireball, fireball, fireball, fire fire!
		VectorMA( origin, 24, dir, sprOrg );
		VectorScale( dir, 64, sprVel );
		lightColor[0] = 0.7;	// subtler explosion colors
		lightColor[1] = 0.6;
		lightColor[2] = 0.4;
		VectorMA( origin, 4, dir, sprOrg );
		VectorScale( dir, 2, sprVel );
		VectorMA( origin, 4, dir, sprOrg );
		VectorScale( dir, 42, sprVel );
		CG_ParticleExplosion( "explode1", sprOrg, sprVel, 700, 10, 108 );
		VectorMA( origin, 4, dir, sprOrg );
		VectorScale( dir, 82, sprVel );
		CG_ParticleExplosion( "explode1", sprOrg, sprVel, 400, 11, 158 );
		VectorMA( origin, -5, dir, sprOrg );
		VectorScale( dir, 182, sprVel );
		CG_ParticleExplosion( "explode1", sprOrg, sprVel, 600, 47, 88 );
		VectorMA( origin, -5, dir, sprOrg );
		VectorScale( dir, 64, sprVel );
		CG_ParticleExplosion( "explode1", sprOrg, sprVel, 110, 72, 28 );
		mod = 0; // turns off the sprite (unfortunately, disables dlight)
		}
		// LEILEI END enhancement

		break;

	case WP_GRENADE_LAUNCHER:
		mod = cgs.media.dishFlashModel;
		shader = cgs.media.grenadeExplosionShader;
		if ( !cg_paintballMode.integer )
		sfx = cgs.media.sfx_rockexp;
		else
			sfx = cgs.media.gibSound;
		mark = cgs.media.burnMarkShader;
		radius = 64;
		light = 300;
		isSprite = qtrue;
		// LEILEI START enhancement
		if (cg_leiEnhancement.integer) {
		// some more fireball, fireball, fireball, fire fire!
		VectorMA( origin, 24, dir, sprOrg );
		VectorScale( dir, 64, sprVel );
		lightColor[0] = 0.7;	// subtler explosion colors
		lightColor[1] = 0.6;
		lightColor[2] = 0.4;
		VectorMA( origin, 4, dir, sprOrg );
		VectorScale( dir, 2, sprVel );
		VectorMA( origin, 4, dir, sprOrg );
		VectorScale( dir, 42, sprVel );
		CG_ParticleExplosion( "explode1", sprOrg, sprVel, 700, 10, 108 );
		VectorMA( origin, 4, dir, sprOrg );
		VectorScale( dir, 82, sprVel );
		CG_ParticleExplosion( "explode1", sprOrg, sprVel, 400, 11, 158 );
		VectorMA( origin, -5, dir, sprOrg );
		VectorScale( dir, 182, sprVel );
		CG_ParticleExplosion( "explode1", sprOrg, sprVel, 600, 47, 88 );
		VectorMA( origin, -5, dir, sprOrg );
		VectorScale( dir, 64, sprVel );
		CG_ParticleExplosion( "explode1", sprOrg, sprVel, 110, 72, 28 );
		mod = 0; // turns off the sprite (unfortunately, disables dlight)
		}
		// LEILEI END enhancement
		break;
	case WP_ROCKET_LAUNCHER:
		mod = cgs.media.dishFlashModel;
		shader = cgs.media.rocketExplosionShader;
		if ( !cg_paintballMode.integer )
		sfx = cgs.media.sfx_rockexp;
		else
			sfx = cgs.media.gibSound;
		mark = cgs.media.burnMarkShader;
		radius = 64;
		light = 300;
		isSprite = qtrue;
		duration = 1000;
		lightColor[0] = 1;
		lightColor[1] = 0.75;
		lightColor[2] = 0.0;
		// explosion sprite animation
		VectorMA( origin, 24, dir, sprOrg );
		VectorScale( dir, 64, sprVel );		
		CG_ParticleExplosion( "explode1", sprOrg, sprVel, 1400, 20, 30 );
		// LEILEI START enhancement
		if (cg_leiEnhancement.integer) {
		// some more fireball, fireball, fireball, fire fire!
		VectorMA( origin, 24, dir, sprOrg );
		VectorScale( dir, 64, sprVel );
		lightColor[0] = 0.7;	// subtler explosion colors
		lightColor[1] = 0.6;
		lightColor[2] = 0.4;
		VectorMA( origin, 4, dir, sprOrg );
		VectorScale( dir, 2, sprVel );
		VectorMA( origin, 4, dir, sprOrg );
		VectorScale( dir, 42, sprVel );
		CG_ParticleExplosion( "explode1", sprOrg, sprVel, 700, 10, 108 );
		VectorMA( origin, 4, dir, sprOrg );
		VectorScale( dir, 82, sprVel );
		CG_ParticleExplosion( "explode1", sprOrg, sprVel, 400, 11, 158 );
		VectorMA( origin, -5, dir, sprOrg );
		VectorScale( dir, 182, sprVel );
		CG_ParticleExplosion( "explode1", sprOrg, sprVel, 600, 47, 88 );
		VectorMA( origin, -5, dir, sprOrg );
		VectorScale( dir, 64, sprVel );
		CG_ParticleExplosion( "explode1", sprOrg, sprVel, 110, 72, 28 );
		mod = 0; // turns off the sprite (unfortunately, disables dlight)
		}
		// LEILEI END enhancement
		break;
	case WP_RAILGUN:
		mod = cgs.media.ringFlashModel;
		shader = cgs.media.railExplosionShader;
		if ( !cg_paintballMode.integer )
		sfx = cgs.media.sfx_plasmaexp;
		else
			sfx = cgs.media.gibBounce3Sound;
		mark = cgs.media.energyMarkShader;
		radius = 24;
		break;
	case WP_PLASMAGUN:
		mod = cgs.media.ringFlashModel;
		shader = cgs.media.plasmaExplosionShader;
		if ( !cg_paintballMode.integer )
		sfx = cgs.media.sfx_plasmaexp;
		else
			sfx = cgs.media.gibBounce3Sound;
		mark = cgs.media.energyMarkShader;
		radius = 16;
		break;
	case WP_BFG:
		mod = cgs.media.dishFlashModel;
		shader = cgs.media.bfgExplosionShader;
		if ( !cg_paintballMode.integer )
		sfx = cgs.media.sfx_rockexp;
		else
			sfx = cgs.media.gibSound;
		mark = cgs.media.burnMarkShader;
		radius = 32;
		isSprite = qtrue;
		break;
	case WP_FLAMETHROWER:
		sfx = cgs.media.sfx_plasmaexp;
		shader = cgs.media.plasmaExplosionShader;
		mark = cgs.media.burnMarkShader;
		radius = 32;
		break;
	case WP_ANTIMATTER:
		sfx = cgs.media.sfx_plasmaexp;
		shader = cgs.media.plasmaExplosionShader;
		mark = cgs.media.burnMarkShader;
		radius = 1;
		break;
	case WP_TOOLGUN:
		// no explosion at LG impact, it is added with the beam
		r = rand() & 3;
		if ( r < 2 ) {
			sfx = cgs.media.sfx_lghit2;
		} else if ( r == 2 ) {
			sfx = cgs.media.sfx_lghit1;
		} else {
			sfx = cgs.media.sfx_lghit3;
		}
		mark = cgs.media.holeMarkShader;
		radius = 12;
		break;
	case WP_PHYSGUN:
		mod =  0;
		shader = 0;
		sfx = 0;
		mark = 0;
		radius = 0;
		break;
	case WP_GRAVITYGUN:
		mod =  0;
		shader = 0;
		sfx = 0;
		mark = 0;
		radius = 0;
		break;
	case WP_SHOTGUN:
		mod = cgs.media.bulletFlashModel;
		shader = cgs.media.bulletExplosionShader;
		mark = cgs.media.bulletMarkShader;
		sfx = 0;
		radius = 4;
		break;

	case WP_CHAINGUN:
		mod = cgs.media.bulletFlashModel;
		if( soundType == IMPACTSOUND_FLESH ) {
			sfx = cgs.media.sfx_chghitflesh;
		} else if( soundType == IMPACTSOUND_METAL ) {
			sfx = cgs.media.sfx_chghitmetal;
		} else {
			sfx = cgs.media.sfx_chghit;
		}

		mark = cgs.media.bulletMarkShader;
		r = rand() & 3;
		if ( !cg_paintballMode.integer ) {
			if ( r == 0 ) {
				sfx = cgs.media.sfx_ric1;
			} else if ( r == 1 ) {
				sfx = cgs.media.sfx_ric2;
			} else {
				sfx = cgs.media.sfx_ric3;
			}
		} else {
			if ( r == 0 ) {
				sfx = cgs.media.gibBounce1Sound;
			} else if ( r == 1 ) {
				sfx = cgs.media.gibBounce2Sound;
			} else {
				sfx = cgs.media.gibBounce3Sound;
			}
		}

		radius = 8;
		break;

	case WP_MACHINEGUN:
		mod = cgs.media.bulletFlashModel;
		shader = cgs.media.bulletExplosionShader;
		mark = cgs.media.bulletMarkShader;
		r = rand() & 3;
		if ( r == 0 ) {
			sfx = cgs.media.sfx_ric1;
		} else if ( r == 1 ) {
			sfx = cgs.media.sfx_ric2;
		} else {
			sfx = cgs.media.sfx_ric3;
		}

		radius = 8;
		break;
	}

	if ( sfx ) {
		trap_S_StartSound( origin, ENTITYNUM_WORLD, CHAN_AUTO, sfx );
	}

	//
	// create the explosion
	//
	if ( mod ) {
		le = CG_MakeExplosion( origin, dir, 
							   mod,	shader,
							   duration, isSprite );
		le->light = light;
		VectorCopy( lightColor, le->lightColor );
		if ( weapon == WP_RAILGUN ) {
			// colorize with client color
			VectorCopy( cgs.clientinfo[clientNum].color1, le->color );
			le->refEntity.shaderRGBA[0] = le->color[0] * 0xff;
			le->refEntity.shaderRGBA[1] = le->color[1] * 0xff;
			le->refEntity.shaderRGBA[2] = le->color[2] * 0xff;
			le->refEntity.shaderRGBA[3] = 0xff;
		}
	}

	//
	// impact mark
	//
	alphaFade = (mark == cgs.media.energyMarkShader);	// plasma fades alpha, all others fade color
	if ( weapon == WP_RAILGUN ) {
		float	*color;

		// colorize with client color
		color = cgs.clientinfo[clientNum].color1;
		CG_ImpactMark( mark, origin, dir, random()*360, color[0],color[1], color[2],1, alphaFade, radius, qfalse );
	} else {
		CG_ImpactMark( mark, origin, dir, random()*360, 1,1,1,1, alphaFade, radius, qfalse );
	}
}


/*
=================
CG_MissileHitPlayer
=================
*/
void CG_MissileHitPlayer( int weapon, vec3_t origin, vec3_t dir, int entityNum ) {
// LEILEI ENHANCEMENT
	if (cg_leiEnhancement.integer) {
		CG_SmokePuff( origin, dir, 22, 1, 1, 1, 1.0f, 900, cg.time, 0, 0,  cgs.media.lbldShader1 );
		CG_SpurtBlood( origin, dir, 1);
//		CG_SpurtBlood( origin, dir, 4);
//		CG_SpurtBlood( origin, dir, -12);
		}

	else
	CG_Bleed( origin, entityNum );

	// some weapons will make an explosion with the blood, while
	// others will just make the blood
	switch ( weapon ) {
	case WP_GRENADE_LAUNCHER:
	case WP_ROCKET_LAUNCHER:

	case WP_NAILGUN:
	case WP_CHAINGUN:
	case WP_PROX_LAUNCHER:

		CG_MissileHitWall( weapon, 0, origin, dir, IMPACTSOUND_FLESH );
		break;
	default:
		break;
	}
}



/*
============================================================================

SHOTGUN TRACING

============================================================================
*/

/*
================
CG_ShotgunPellet
================
*/
static void CG_ShotgunPellet( vec3_t start, vec3_t end, int skipNum ) {
	trace_t		tr;
	int sourceContentType, destContentType;

// LEILEI ENHACNEMENT
	localEntity_t	*smoke;
	vec3_t  kapow;

	CG_Trace( &tr, start, NULL, NULL, end, skipNum, MASK_SHOT );

	sourceContentType = CG_PointContents( start, 0 );
	destContentType = CG_PointContents( tr.endpos, 0 );

	// FIXME: should probably move this cruft into CG_BubbleTrail
	if ( sourceContentType == destContentType ) {
		if ( sourceContentType & CONTENTS_WATER ) {
			CG_BubbleTrail( start, tr.endpos, 32 );
		}
	} else if ( sourceContentType & CONTENTS_WATER ) {
		trace_t trace;

		trap_CM_BoxTrace( &trace, end, start, NULL, NULL, 0, CONTENTS_WATER );
		CG_BubbleTrail( start, trace.endpos, 32 );

// LEILEI ENHANCEMENT
				if (cg_leiEnhancement.integer) {
				// Water Splash
					VectorCopy( trace.plane.normal, kapow );
					
					kapow[0] = kapow[0] * (crandom() * 22);
					kapow[1] = kapow[1] * (crandom() * 22);
					kapow[2] = kapow[2] * (crandom() * 65 + 37);
					smoke = CG_SmokePuff( trace.endpos, kapow, 14, 1, 1, 1, 1.0f, 400, cg.time, 0, 0,  cgs.media.lsplShader );
					smoke = CG_SmokePuff( trace.endpos, kapow, 6, 1, 1, 1, 1.0f, 200, cg.time, 0, 0,  cgs.media.lsplShader );
					smoke = CG_SmokePuff( trace.endpos, kapow, 10, 1, 1, 1, 1.0f, 300, cg.time, 0, 0,  cgs.media.lsplShader );
						
				}
// END LEIHANCMENET
	} else if ( destContentType & CONTENTS_WATER ) {
		trace_t trace;

		trap_CM_BoxTrace( &trace, start, end, NULL, NULL, 0, CONTENTS_WATER );
		CG_BubbleTrail( tr.endpos, trace.endpos, 32 );

// LEILEI ENHANCEMENT
				if (cg_leiEnhancement.integer) {
				// Water Splash
					VectorCopy( trace.plane.normal, kapow );
					
					kapow[0] = kapow[0] * (crandom() * 22);
					kapow[1] = kapow[1] * (crandom() * 22);
					kapow[2] = kapow[2] * (crandom() * 65 + 37);
					smoke = CG_SmokePuff( trace.endpos, kapow, 14, 1, 1, 1, 1.0f, 400, cg.time, 0, 0,  cgs.media.lsplShader );
					smoke = CG_SmokePuff( trace.endpos, kapow, 6, 1, 1, 1, 1.0f, 200, cg.time, 0, 0,  cgs.media.lsplShader );
					smoke = CG_SmokePuff( trace.endpos, kapow, 10, 1, 1, 1, 1.0f, 300, cg.time, 0, 0,  cgs.media.lsplShader );
				}
// END LEIHANCMENET
	}

	if (  tr.surfaceFlags & SURF_NOIMPACT ) {
		return;
	}

	if ( cg_entities[tr.entityNum].currentState.eType == ET_PLAYER ) {
		CG_MissileHitPlayer( WP_SHOTGUN, tr.endpos, tr.plane.normal, tr.entityNum );
	} else {
		if ( tr.surfaceFlags & SURF_NOIMPACT ) {
			// SURF_NOIMPACT will not make a flame puff or a mark
			return;
		}
		if ( tr.surfaceFlags & SURF_METALSTEPS ) {
			CG_MissileHitWall( WP_SHOTGUN, 0, tr.endpos, tr.plane.normal, IMPACTSOUND_METAL );
// LEILEI ENHANCEMENT
			if (cg_leiEnhancement.integer) {
					VectorCopy( tr.plane.normal, kapow );

					kapow[0] = kapow[0] * (crandom() * 65 + 37);
					kapow[1] = kapow[1] * (crandom() * 65 + 37);
					kapow[2] = kapow[2] * (crandom() * 65 + 37);
					CG_LeiSparks(tr.endpos, tr.plane.normal, 800, 0, 0, 7);
					CG_LeiSparks(tr.endpos, tr.plane.normal, 800, 0, 0, 3);
					CG_LeiSparks(tr.endpos, tr.plane.normal, 800, 0, 0, 1);
				
				}
// END LEIHANCMENET
		} else {
			CG_MissileHitWall( WP_SHOTGUN, 0, tr.endpos, tr.plane.normal, IMPACTSOUND_DEFAULT );
	
// LEILEI ENHANCEMENT
				if (cg_leiEnhancement.integer) {
					VectorCopy( tr.plane.normal, kapow );

					kapow[0] = kapow[0] * (crandom() * 65 + 37);
					kapow[1] = kapow[1] * (crandom() * 65 + 37);
					kapow[2] = kapow[2] * (crandom() * 65 + 37);
					CG_LeiSparks(tr.endpos, tr.plane.normal, 800, 0, 0, 7);
					CG_LeiSparks(tr.endpos, tr.plane.normal, 800, 0, 0, 2);
					
					smoke = CG_SmokePuff( tr.endpos, kapow, 21, 1, 1, 1, 0.9f, 1200, cg.time, 0, 0,  cgs.media.lsmkShader2 );
					//smoke = CG_SmokePuff( tr.endpos, kapow, 21, 1, 1, 1, 0.9f, 1200, cg.time, 0, 0,  cgs.media.lbumShader1 );
#if 0
					CG_LeiPuff(tr.endpos, kapow, 500, 0, 0, 177, 6);
					CG_LeiPuff(tr.endpos, tr.plane.normal, 500, 0, 0, 127, 12);
					CG_LeiPuff(tr.endpos, tr.plane.normal, 500, 0, 0, 77, 16);
					CG_LeiPuff(tr.endpos, tr.plane.normal, 500, 0, 0, 127, 12);
					CG_LeiPuff(tr.endpos, tr.plane.normal, 500, 0, 0, 77, 16);
					CG_LeiPuff(tr.endpos, tr.plane.normal, 500, 0, 0, 127, 12);
					CG_LeiPuff(tr.endpos, tr.plane.normal, 500, 0, 0, 77, 16);
#endif
				}
// END LEIHANCMENET
		}
	}
}

/*
================
CG_ShotgunPattern

Perform the same traces the server did to locate the
hit splashes
================
*/
//unlagged - attack prediction
// made this non-static for access from cg_unlagged.c
void CG_ShotgunPattern( vec3_t origin, vec3_t origin2, int seed, int otherEntNum ) {
	int			i;
	float		r, u;
	vec3_t		end;
	vec3_t		forward, right, up;

	// derive the right and up vectors from the forward vector, because
	// the client won't have any other information
	VectorNormalize2( origin2, forward );
	PerpendicularVector( right, forward );
	CrossProduct( forward, right, up );

	// generate the "random" spread pattern
	for ( i = 0 ; i < mod_sgcount ; i++ ) {
		r = Q_crandom( &seed ) * mod_sgspread * 16;
		u = Q_crandom( &seed ) * mod_sgspread * 16;
		VectorMA( origin, 8192 * 16, forward, end);
		VectorMA (end, r, right, end);
		VectorMA (end, u, up, end);

		CG_ShotgunPellet( origin, end, otherEntNum );
	}
}

/*
==============
CG_ShotgunFire
==============
*/
void CG_ShotgunFire( entityState_t *es ) {
	vec3_t	v;
	vec3_t			up;
	vec3_t			forward;
	int		contents;

	VectorSubtract( es->origin2, es->pos.trBase, v );
	VectorNormalize( v );
	VectorScale( v, 32, v );
	VectorAdd( es->pos.trBase, v, v );

	contents = CG_PointContents( es->pos.trBase, 0 );
	if ( !( contents & CONTENTS_WATER ) ) {
		VectorSet( up, 0, 0, 8 );
			if (cg_leiEnhancement.integer) {
				// Shotgun puffy
				CG_LeiSparks(v, forward, 1500, 0, 0, 7);
				CG_LeiSparks(v, forward, 1500, 0, 0, 7);
				CG_LeiSparks(v, forward, 1500, 0, 0, 7);
				CG_LeiSparks(v, forward, 1500, 0, 0, 7);
				CG_LeiSparks(v, forward, 1500, 0, 0, 7);
				CG_LeiSparks(v, forward, 1500, 0, 0, 7);
				CG_SmokePuff( v, up, 32, 1, 1, 1, 0.33f, 900, cg.time, 0, LEF_PUFF_DONT_SCALE, cgs.media.shotgunSmokePuffShader );
			}
	}
	CG_ShotgunPattern( es->pos.trBase, es->origin2, es->eventParm, es->otherEntityNum );
}

/*
============================================================================

BULLETS

============================================================================
*/


/*
===============
CG_Tracer
===============
*/
void CG_Tracer( vec3_t source, vec3_t dest ) {
	vec3_t		forward, right;
	polyVert_t	verts[4];
	vec3_t		line;
	float		len, begin, end;
	vec3_t		start, finish;
	vec3_t		midpoint;

	// tracer
	VectorSubtract( dest, source, forward );
	len = VectorNormalize( forward );

	// start at least a little ways from the muzzle
	if ( len < 100 ) {
		return;
	}
	begin = 50 + random() * (len - 60);
	end = begin + cg_tracerLength.value;
	if ( end > len ) {
		end = len;
	}
	VectorMA( source, begin, forward, start );
	VectorMA( source, end, forward, finish );

	line[0] = DotProduct( forward, cg.refdef.viewaxis[1] );
	line[1] = DotProduct( forward, cg.refdef.viewaxis[2] );

	VectorScale( cg.refdef.viewaxis[1], line[1], right );
	VectorMA( right, -line[0], cg.refdef.viewaxis[2], right );
	VectorNormalize( right );

	VectorMA( finish, cg_tracerWidth.value, right, verts[0].xyz );
	verts[0].st[0] = 0;
	verts[0].st[1] = 1;
	verts[0].modulate[0] = 255;
	verts[0].modulate[1] = 255;
	verts[0].modulate[2] = 255;
	verts[0].modulate[3] = 255;

	VectorMA( finish, -cg_tracerWidth.value, right, verts[1].xyz );
	verts[1].st[0] = 1;
	verts[1].st[1] = 0;
	verts[1].modulate[0] = 255;
	verts[1].modulate[1] = 255;
	verts[1].modulate[2] = 255;
	verts[1].modulate[3] = 255;

	VectorMA( start, -cg_tracerWidth.value, right, verts[2].xyz );
	verts[2].st[0] = 1;
	verts[2].st[1] = 1;
	verts[2].modulate[0] = 255;
	verts[2].modulate[1] = 255;
	verts[2].modulate[2] = 255;
	verts[2].modulate[3] = 255;

	VectorMA( start, cg_tracerWidth.value, right, verts[3].xyz );
	verts[3].st[0] = 0;
	verts[3].st[1] = 0;
	verts[3].modulate[0] = 255;
	verts[3].modulate[1] = 255;
	verts[3].modulate[2] = 255;
	verts[3].modulate[3] = 255;

	trap_R_AddPolyToScene( cgs.media.tracerShader, 4, verts );

	midpoint[0] = ( start[0] + finish[0] ) * 0.5;
	midpoint[1] = ( start[1] + finish[1] ) * 0.5;
	midpoint[2] = ( start[2] + finish[2] ) * 0.5;

	// add the tracer sound
	trap_S_StartSound( midpoint, ENTITYNUM_WORLD, CHAN_AUTO, cgs.media.tracerSound );

}

/*
======================
CG_CalcMuzzlePoint
======================
*/
static qboolean	CG_CalcMuzzlePoint( int entityNum, vec3_t muzzle ) {
	vec3_t		forward;
	centity_t	*cent;
	int			anim;

	if ( entityNum == cg.snap->ps.clientNum ) {
		VectorCopy( cg.snap->ps.origin, muzzle );
		muzzle[2] += cg.snap->ps.viewheight;
		AngleVectors( cg.snap->ps.viewangles, forward, NULL, NULL );
		VectorMA( muzzle, 14, forward, muzzle );
		return qtrue;
	}

	cent = &cg_entities[entityNum];
	if ( !cent->currentValid ) {
		return qfalse;
	}

	VectorCopy( cent->currentState.pos.trBase, muzzle );

	AngleVectors( cent->currentState.apos.trBase, forward, NULL, NULL );
	anim = cent->currentState.legsAnim & ~ANIM_TOGGLEBIT;
	if ( anim == LEGS_WALKCR || anim == LEGS_IDLECR ) {
		muzzle[2] += CROUCH_VIEWHEIGHT;
	} else {
		muzzle[2] += DEFAULT_VIEWHEIGHT;
	}

	VectorMA( muzzle, 14, forward, muzzle );

	return qtrue;

}

void CG_PlayerFlashlight(centity_t *cent) {
    vec3_t forward, right, up;
    vec3_t start, end;
    trace_t trace;
    trace_t traceambient;
    float distance, lightIntensity, correctedIntensity;
    float minRadius = 32.0f;      // Minimum radius for focused light
    float maxRadius = 300.0f;    // Maximum radius for spread-out light
	float lightRadius = 600.0f;    // Maximum radius for spread-out light
    float maxBrightness = 0.32f;  // Maximum brightness intensity
    float minBrightness = 0.01f;  // Minimum brightness intensity
	float radius;

    AngleVectors(cent->lerpAngles, forward, right, up);
    VectorCopy(cent->lerpOrigin, start);
    VectorMA(start, lightRadius, forward, end);

    CG_Trace(&trace, start, NULL, NULL, end, cent->currentState.number, MASK_SHOT);
    CG_Trace(&traceambient, start, NULL, NULL, end, cent->currentState.number, MASK_SHOT);

    VectorMA(traceambient.endpos, -24, forward, traceambient.endpos);
    VectorMA(trace.endpos, -8, forward, trace.endpos);
    distance = VectorDistance(start, traceambient.endpos);

    // Calculate light intensity, clamped between minBrightness and maxBrightness
    lightIntensity = 1.0f - (distance / lightRadius);
    if (lightIntensity < 0.0f) {
        lightIntensity = 0.0f;
    }
    lightIntensity = lightIntensity * (maxBrightness - minBrightness) + minBrightness;
    if (lightIntensity > maxBrightness) {
        lightIntensity = maxBrightness;
    } else if (lightIntensity < minBrightness) {
        lightIntensity = minBrightness;
    }

    // Calculate radius based on distance, scaling between minRadius and maxRadius
    radius = minRadius + (distance / lightRadius) * (maxRadius - minRadius);
    if (radius > maxRadius) {
        radius = maxRadius;
    } else if (radius < minRadius) {
        radius = minRadius;
    }

    // Scale correctedIntensity with clamped lightIntensity
    correctedIntensity = (lightIntensity * 0.5f)+0.03;

    // Add light with adjustable radius and intensity limits
    trap_R_AddLinearLightToScene(start, traceambient.endpos, radius, correctedIntensity, correctedIntensity, correctedIntensity);
    trap_R_AddLightToScene(trace.endpos, radius*1.25, correctedIntensity, correctedIntensity, correctedIntensity);
}

/*
======================
CG_Bullet

Renders bullet effects.
======================
*/
void CG_Bullet( vec3_t end, int sourceEntityNum, vec3_t normal, qboolean flesh, int fleshEntityNum ) {
	trace_t trace;
	int sourceContentType, destContentType;
	vec3_t		start;
// LEILEI ENHACNEMENT
	localEntity_t	*smoke;
	vec3_t	kapew;	
	vec3_t  kapow;

	// if the shooter is currently valid, calc a source point and possibly
	// do trail effects
	if ( sourceEntityNum >= 0 && cg_tracerChance.value > 0 ) {
		if ( CG_CalcMuzzlePoint( sourceEntityNum, start ) ) {
			sourceContentType = CG_PointContents( start, 0 );
			destContentType = CG_PointContents( end, 0 );

			// do a complete bubble trail if necessary
			if ( ( sourceContentType == destContentType ) && ( sourceContentType & CONTENTS_WATER ) ) {
				CG_BubbleTrail( start, end, 32 );
			}
			// bubble trail from water into air
			else if ( ( sourceContentType & CONTENTS_WATER ) ) {
				trap_CM_BoxTrace( &trace, end, start, NULL, NULL, 0, CONTENTS_WATER );
				CG_BubbleTrail( start, trace.endpos, 32 );


// LEILEI ENHANCEMENT
				if (cg_leiEnhancement.integer) {
				// Water Splash
					VectorCopy( trace.plane.normal, kapow );
					kapow[0] = kapow[0] * (crandom() * 22);
					kapow[1] = kapow[1] * (crandom() * 22);
					kapow[2] = kapow[2] * (crandom() * 65 + 37);
					smoke = CG_SmokePuff( trace.endpos, kapow, 14, 1, 1, 1, 1.0f, 400, cg.time, 0, 0,  cgs.media.lsplShader );
					smoke = CG_SmokePuff( trace.endpos, kapow, 6, 1, 1, 1, 1.0f, 200, cg.time, 0, 0,  cgs.media.lsplShader );
					smoke = CG_SmokePuff( trace.endpos, kapow, 10, 1, 1, 1, 1.0f, 300, cg.time, 0, 0,  cgs.media.lsplShader );
				//	CG_LeiSplash2(trace.endpos, kapow, 900, 0, 0, 444);
						
				}
// END LEIHANCMENET


			}
			// bubble trail from air into water
			else if ( ( destContentType & CONTENTS_WATER ) ) {
				trap_CM_BoxTrace( &trace, start, end, NULL, NULL, 0, CONTENTS_WATER );
				CG_BubbleTrail( trace.endpos, end, 32 );

// LEILEI ENHANCEMENT
				if (cg_leiEnhancement.integer) {
				// Water Splash
					VectorCopy( trace.plane.normal, kapow );
					
					kapow[0] = kapow[0] * (crandom() * 22);
					kapow[1] = kapow[1] * (crandom() * 22);
					kapow[2] = kapow[2] * (crandom() * 65 + 37);
					smoke = CG_SmokePuff( trace.endpos, kapow, 14, 1, 1, 1, 1.0f, 400, cg.time, 0, 0,  cgs.media.lsplShader );
					smoke = CG_SmokePuff( trace.endpos, kapow, 6, 1, 1, 1, 1.0f, 200, cg.time, 0, 0,  cgs.media.lsplShader );
					smoke = CG_SmokePuff( trace.endpos, kapow, 10, 1, 1, 1, 1.0f, 300, cg.time, 0, 0,  cgs.media.lsplShader );
			//CG_LeiSplash2(trace.endpos, kapow, 500, 0, 0, 1);
				}
// END LEIHANCMENET
			}

			// draw a tracer
			if ( random() < cg_tracerChance.value ) {
				CG_Tracer( start, end );
			}
		}
	}

	// impact splash and mark
	if ( flesh ) {
// LEILEI ENHANCEMENT
	if (cg_leiEnhancement.integer) {
if ( cg_blood.integer ) {

		
						// Blood Hack
				VectorCopy( normal, kapow );
					
				kapow[0] = kapow[0] * (crandom() * 65 + 37);
				kapow[1] = kapow[1] * (crandom() * 65 + 37);
				kapow[2] = kapow[2] * (crandom() * 65 + 37);
				VectorCopy( kapow, kapew );

				kapew[0] = kapew[0] * (crandom() * 2 + 37);
				kapew[1] = kapew[1] * (crandom() * 2 + 37);
				kapew[2] = kapew[2] * (crandom() * 2 + 37);

		CG_SmokePuff( end, kapow, 6, 1, 1, 1, 1.0f, 600, cg.time, 0, 0,  cgs.media.lbldShader1 );
//		CG_SpurtBlood( end, kapow, 2);
		CG_SpurtBlood( end, kapew, 1);
		//CG_Particle_Bleed(cgs.media.lbldShader1,kapew,'0 0 0', 0, 100);
//		CG_Particle_Bleed(cgs.media.lbldShader1,kapew,kapow, 0, 100);
//		CG_Particle_BloodCloud(self,end,'0 0 0');

}
		}

	else
		CG_Bleed( end, fleshEntityNum );
	} else {
		CG_MissileHitWall( WP_MACHINEGUN, 0, end, normal, IMPACTSOUND_DEFAULT );

// LEILEI ENHANCEMENT
				if (cg_leiEnhancement.integer) {

				// Smoke puff
					VectorCopy( normal, kapow );
					
					kapow[0] = kapow[0] * (crandom() * 65 + 37);
					kapow[1] = kapow[1] * (crandom() * 65 + 37);
					kapow[2] = kapow[2] * (crandom() * 65 + 37);
					VectorCopy( kapow, kapew );

					kapew[0] = kapew[0] * (crandom() * 65 + 37);
					kapew[1] = kapew[1] * (crandom() * 65 + 37);
					kapew[2] = kapew[2] * (crandom() * 65 + 37);


					smoke = CG_SmokePuff( end, kapow, 14, 1, 1, 1, 1.0f, 600, cg.time, 0, 0,  cgs.media.lsmkShader1 );
			//		CG_LeiSparks(end, normal, 600, 0, 0, 177);
			//		CG_LeiSparks(end, normal, 600, 0, 0, 155);
			//		CG_LeiSparks(end, normal, 600, 0, 0, 444);
			//		CG_LeiSparks(trace.endpos, trace.plane.normal, 800, 0, 0, 7);
			//		CG_LeiSparks(trace.endpos, trace.plane.normal, 800, 0, 0, 3);
			//		CG_LeiSparks(trace.endpos, trace.plane.normal, 800, 0, 0, 1);

				}
// END LEIHANCMENET
	}

}
