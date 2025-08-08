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
 * name:		cg_weapons.c
 *
 * desc:		events and effects dealing with weapons
 *
*/

#include "cg_local.h"

int wolfkickModel;
int hWeaponSnd;
int hWeaponEchoSnd;
int hflakWeaponSnd;
int notebookModel;
int propellerModel;
int wolfkickSkin; // eugeny


vec3_t ejectBrassCasingOrigin;

//----(SA)
// forward decs
static int getAltWeapon( int weapnum );
int getEquivWeapon( int weapnum );
int CG_WeaponIndex( int weapnum, int *bank, int *cycle );
static qboolean CG_WeaponHasAmmo( int i );
static int maxWeapBanks = MAX_WEAP_BANKS, maxWeapsInBank = MAX_WEAPS_IN_BANK; // JPW NERVE

int weapBanks[MAX_WEAP_BANKS][MAX_WEAPS_IN_BANK] = {
	{0, 0, 0, 0, 0, 0},																						  //	0 (empty)
	{WP_KNIFE, WP_HOLYCROSS, 0, 0, 0},															  //	1
	{WP_LUGER, WP_SILENCER, WP_COLT, WP_AKIMBO, WP_TT33, WP_DUAL_TT33, WP_REVOLVER, WP_HDM},				  //	2
	{WP_MP40, WP_MP34, WP_STEN, WP_THOMPSON, WP_PPSH, 0},													  //	3
	{WP_MAUSER, WP_GARAND, WP_MOSIN, WP_DELISLE, 0, 0},														  //	4
	{WP_G43, WP_M1GARAND, WP_M1941, 0, 0, 0},																  //	5
	{WP_FG42, WP_MP44, WP_BAR, 0, 0, 0},																	  //	6
	{WP_M97, WP_AUTO5, 0, 0, 0},																	  //	7
	{WP_GRENADE_LAUNCHER, WP_GRENADE_PINEAPPLE, WP_DYNAMITE, WP_AIRSTRIKE, WP_POISONGAS, WP_POISONGAS_MEDIC, WP_DYNAMITE_ENG}, //	8
	{WP_PANZERFAUST, WP_FLAMETHROWER, WP_MG42M, WP_BROWNING, 0, 0},											  //	9
	{WP_VENOM, WP_TESLA, 0, 0, 0, 0}																		  //	10
};

// JPW NERVE -- in mutiplayer, characters get knife/special on button 1, pistols on 2, 2-handed on 3
int weapBanksMultiPlayer[MAX_WEAP_BANKS_MP][MAX_WEAPS_IN_BANK_MP] = {
	{0,                     0,                      0,          0,          0,          0,              0,          0           },  // empty bank '0'
	{WP_KNIFE,              0,                      0,          0,          0,          0,              0,          0           },
	{WP_LUGER,              WP_COLT,                0,          0,          0,          0,              0,          0           },
	{WP_MP40,               WP_THOMPSON,            WP_STEN,    WP_MAUSER,  WP_GARAND,  WP_PANZERFAUST, WP_VENOM,   WP_FLAMETHROWER     },
	{WP_GRENADE_LAUNCHER,   WP_GRENADE_PINEAPPLE,   0,          0,          0,          0,              0,          0,          },
	{0,                     0,                      0,          0,          0,          0,              0,          0,          },
	{WP_DYNAMITE,           0,                      0,          0,          0,          0,              0,          0           }
};



/*
==============
CG_MachineGunEjectBrassNew
==============
*/
static void CG_MachineGunEjectBrassNew( centity_t *cent ) {
	localEntity_t   *le;
	refEntity_t     *re;
	vec3_t velocity, xvelocity;
	float waterScale = 1.0f;
	vec3_t v[3];

	if ( cg_brassTime.integer <= 0 ) {
		return;
	}

	if ( cent->currentState.weapon == WP_REVOLVER )
		return;

	le = CG_AllocLocalEntity();
	re = &le->refEntity;

	velocity[0] = -50 + 25 * crandom(); // New eject brass RealRTCW
	velocity[1] = -100 + 40 * crandom();
	velocity[2] = 200 + 50 * random();

	le->leType = LE_FRAGMENT;
	le->startTime = cg.time;
	le->endTime = le->startTime + cg_brassTime.integer + ( cg_brassTime.integer / 4 ) * random();

	le->pos.trType = TR_GRAVITY;
	le->pos.trTime = cg.time - ( rand() & 15 );

	AnglesToAxis( cent->lerpAngles, v );

	VectorCopy( ejectBrassCasingOrigin, re->origin );

	VectorCopy( re->origin, le->pos.trBase );

	if ( CG_PointContents( re->origin, -1 ) & ( CONTENTS_WATER | CONTENTS_SLIME ) ) { //----(SA)	modified since slime is no longer deadly
//	if ( CG_PointContents( re->origin, -1 ) & CONTENTS_WATER ) {
		waterScale = 0.10;
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
	le->angles.trBase[0]  = (rand() & 31) + 60;  // new eject brass RealRTCW
	le->angles.trBase[1]  = rand() & 255; 
	le->angles.trBase[2]  = rand() & 31;
	le->angles.trDelta[0] = 2;
	le->angles.trDelta[1] = 1;
	le->angles.trDelta[2] = 0;

	le->leFlags = LEF_TUMBLE;


	{
		int contents;
		vec3_t end;
		VectorCopy( cent->lerpOrigin, end );
		end[2] -= 24;
		contents = trap_CM_PointContents( end, 0 );
		if ( contents & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ) {
			le->leBounceSoundType = LEBS_NONE;
		} else {
			le->leBounceSoundType = LEBS_BRASS;
		}
	}

	le->leMarkType = LEMT_NONE;
}

/*
==========================
CG_MachineGunEjectBrass
==========================
*/

static void CG_MachineGunEjectBrass( centity_t *cent ) {
	localEntity_t   *le;
	refEntity_t     *re;
	vec3_t velocity, xvelocity;
	vec3_t offset, xoffset;
	float waterScale = 1.0f;
	vec3_t v[3];

	if ( cg_brassTime.integer <= 0 ) {
		return;
	}

	if ( !( cg.snap->ps.persistant[PERS_HWEAPON_USE] ) && ( cent->currentState.clientNum == cg.snap->ps.clientNum ) ) {
		CG_MachineGunEjectBrassNew( cent );
		return;
	}

	le = CG_AllocLocalEntity();
	re = &le->refEntity;

	velocity[0]           = -20 + 40 * crandom(); // New eject brass RealRTCW
	velocity[1]           = -150 + 40 * crandom();
	velocity[2]           = 100 + 50 * crandom();

	le->leType = LE_FRAGMENT;
	le->startTime = cg.time;
	le->endTime = le->startTime + cg_brassTime.integer + ( cg_brassTime.integer / 4 ) * random();

	le->pos.trType = TR_GRAVITY;
	le->pos.trTime = cg.time - ( rand() & 15 );

	AnglesToAxis( cent->lerpAngles, v );

	if ( cg.snap->ps.persistant[PERS_HWEAPON_USE] ) {
		offset[0] = 32;
		offset[1] = -4;
		offset[2] = 0;
	} else if ( cg.predictedPlayerState.weapon == WP_MP40 || cg.predictedPlayerState.weapon == WP_THOMPSON )     {
		offset[0] = 20; // Maxx Kaufman offset value
		offset[1] = -4;
		offset[2] = 24;
	} else if ( cg.predictedPlayerState.weapon == WP_VENOM )     {
		offset[0] = 12;
		offset[1] = -4;
		offset[2] = 24;
	} else {
		VectorClear( offset );
	}

	xoffset[0] = offset[0] * v[0][0] + offset[1] * v[1][0] + offset[2] * v[2][0];
	xoffset[1] = offset[0] * v[0][1] + offset[1] * v[1][1] + offset[2] * v[2][1];
	xoffset[2] = offset[0] * v[0][2] + offset[1] * v[1][2] + offset[2] * v[2][2];
	VectorAdd( cent->lerpOrigin, xoffset, re->origin );

	VectorCopy( re->origin, le->pos.trBase );

	if ( CG_PointContents( re->origin, -1 ) & ( CONTENTS_WATER | CONTENTS_SLIME ) ) { //----(SA)	modified since slime is no longer deadly
		waterScale = 0.10;
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
	le->angles.trBase[0] = rand() & 31;
	le->angles.trBase[1] = rand() & 31;
	le->angles.trBase[2] = rand() & 31;
	le->angles.trDelta[0] = 2;
	le->angles.trDelta[1] = 1;
	le->angles.trDelta[2] = 0;

	le->leFlags = LEF_TUMBLE;

	{
		int contents;
		vec3_t end;
		VectorCopy( cent->lerpOrigin, end );
		end[2] -= 24;
		contents = trap_CM_PointContents( end, 0 );
		if ( contents & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ) {
			le->leBounceSoundType = LEBS_NONE;
		} else {
			le->leBounceSoundType = LEBS_BRASS;
		}
	}

	le->leMarkType = LEMT_NONE;
}

static void CG_MachineGunEjectBrassDelay( centity_t *cent, int delay ) {
	CG_AllocDelayedBrass( cent, cg.time + delay, CG_MachineGunEjectBrass );
}

/*
==============
CG_ShotgunEjectBrassNew
==============
*/
static void CG_ShotgunEjectBrassNew( centity_t *cent ) {
	localEntity_t   *le;
	refEntity_t     *re;
	vec3_t velocity, xvelocity;
	float waterScale = 1.0f;
	vec3_t v[3];

	if ( cg_brassTime.integer <= 0 ) {
		return;
	}

	if (cent->currentState.weapon == WP_REVOLVER) // no brass for revolver
		return;

	le = CG_AllocLocalEntity();
	re = &le->refEntity;

	velocity[0] = -50 + 25 * crandom(); // New eject brass RealRTCW
	velocity[1] = -100 + 40 * crandom();
	velocity[2] = 200 + 50 * random();

	le->leType = LE_FRAGMENT;
	le->startTime = cg.time;
	le->endTime = le->startTime + cg_brassTime.integer + ( cg_brassTime.integer / 4 ) * random();

	le->pos.trType = TR_GRAVITY;
	le->pos.trTime = cg.time - ( rand() & 15 );

	AnglesToAxis( cent->lerpAngles, v );

	VectorCopy( ejectBrassCasingOrigin, re->origin );

	VectorCopy( re->origin, le->pos.trBase );

	if ( CG_PointContents( re->origin, -1 ) & ( CONTENTS_WATER | CONTENTS_SLIME ) ) { //----(SA)	modified since slime is no longer deadly
//	if ( CG_PointContents( re->origin, -1 ) & CONTENTS_WATER ) {
		waterScale = 0.10;
	}

	xvelocity[0] = velocity[0] * v[0][0] + velocity[1] * v[1][0] + velocity[2] * v[2][0];
	xvelocity[1] = velocity[0] * v[0][1] + velocity[1] * v[1][1] + velocity[2] * v[2][1];
	xvelocity[2] = velocity[0] * v[0][2] + velocity[1] * v[1][2] + velocity[2] * v[2][2];
	VectorScale( xvelocity, waterScale, le->pos.trDelta );

	AxisCopy( axisDefault, re->axis );
	re->hModel = cgs.media.shotgunBrassModel;

	le->bounceFactor = 0.4 * waterScale;

	le->angles.trType = TR_LINEAR;
	le->angles.trTime = cg.time;
	le->angles.trBase[0]  = (rand() & 31) + 60;  // new eject brass RealRTCW
	le->angles.trBase[1]  = rand() & 255; 
	le->angles.trBase[2]  = rand() & 31;
	le->angles.trDelta[0] = 2;
	le->angles.trDelta[1] = 1;
	le->angles.trDelta[2] = 0;

	le->leFlags = LEF_TUMBLE;


	{
		int contents;
		vec3_t end;
		VectorCopy( cent->lerpOrigin, end );
		end[2] -= 24;
		contents = trap_CM_PointContents( end, 0 );
		if ( contents & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ) {
			le->leBounceSoundType = LEBS_NONE;
		} else {
			le->leBounceSoundType = LEBS_BRASS;
		}
	}

	le->leMarkType = LEMT_NONE;
}

/*
==========================
CG_ShotgunEjectBrass
==========================
*/

static void CG_ShotgunEjectBrass( centity_t *cent ) {
	localEntity_t   *le;
	refEntity_t     *re;
	vec3_t velocity, xvelocity;
	vec3_t offset, xoffset;
	float waterScale = 1.0f;
	vec3_t v[3];

	if ( cg_brassTime.integer <= 0 ) {
		return;
	}

	if ( !( cg.snap->ps.persistant[PERS_HWEAPON_USE] ) && ( cent->currentState.clientNum == cg.snap->ps.clientNum ) ) {
		CG_ShotgunEjectBrassNew( cent );
		return;
	}

	le = CG_AllocLocalEntity();
	re = &le->refEntity;

	velocity[0]           = -20 + 40 * crandom(); // New eject brass RealRTCW
	velocity[1]           = -150 + 40 * crandom();
	velocity[2]           = 100 + 50 * crandom();

	le->leType = LE_FRAGMENT;
	le->startTime = cg.time;
	le->endTime = le->startTime + cg_brassTime.integer + ( cg_brassTime.integer / 4 ) * random();

	le->pos.trType = TR_GRAVITY;
	le->pos.trTime = cg.time - ( rand() & 15 );

	AnglesToAxis( cent->lerpAngles, v );

	if ( cg.snap->ps.persistant[PERS_HWEAPON_USE] ) {
		offset[0] = 32;
		offset[1] = -4;
		offset[2] = 0;
	} else if ( cg.predictedPlayerState.weapon == WP_MP40 || cg.predictedPlayerState.weapon == WP_THOMPSON )     {
		offset[0] = 20; // Maxx Kaufman offset value
		offset[1] = -4;
		offset[2] = 24;
	} else if ( cg.predictedPlayerState.weapon == WP_VENOM )     {
		offset[0] = 12;
		offset[1] = -4;
		offset[2] = 24;
	} else {
		VectorClear( offset );
	}

	xoffset[0] = offset[0] * v[0][0] + offset[1] * v[1][0] + offset[2] * v[2][0];
	xoffset[1] = offset[0] * v[0][1] + offset[1] * v[1][1] + offset[2] * v[2][1];
	xoffset[2] = offset[0] * v[0][2] + offset[1] * v[1][2] + offset[2] * v[2][2];
	VectorAdd( cent->lerpOrigin, xoffset, re->origin );

	VectorCopy( re->origin, le->pos.trBase );

	if ( CG_PointContents( re->origin, -1 ) & ( CONTENTS_WATER | CONTENTS_SLIME ) ) { //----(SA)	modified since slime is no longer deadly
		waterScale = 0.10;
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
	le->angles.trBase[0] = rand() & 31;
	le->angles.trBase[1] = rand() & 31;
	le->angles.trBase[2] = rand() & 31;
	le->angles.trDelta[0] = 2;
	le->angles.trDelta[1] = 1;
	le->angles.trDelta[2] = 0;

	le->leFlags = LEF_TUMBLE;

	{
		int contents;
		vec3_t end;
		VectorCopy( cent->lerpOrigin, end );
		end[2] -= 24;
		contents = trap_CM_PointContents( end, 0 );
		if ( contents & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ) {
			le->leBounceSoundType = LEBS_NONE;
		} else {
			le->leBounceSoundType = LEBS_BRASS;
		}
	}

	le->leMarkType = LEMT_NONE;
}

static void  CG_ShotgunEjectBrassDelay( centity_t *cent, int delay ) {
	CG_AllocDelayedBrass( cent, cg.time + delay, CG_ShotgunEjectBrass );
}

/*
==============
CG_PistolEjectBrassNew
==============
*/
static void CG_PistolEjectBrassNew( centity_t *cent ) {
	localEntity_t   *le;
	refEntity_t     *re;
	vec3_t velocity, xvelocity;
	float waterScale = 1.0f;
	vec3_t v[3];

	if ( cg_brassTime.integer <= 0 ) {
		return;
	}

	if (cent->currentState.weapon == WP_REVOLVER) // no brass for revolver
		return;

	le = CG_AllocLocalEntity();
	re = &le->refEntity;

	velocity[0] = -50 + 25 * crandom(); // New eject brass RealRTCW
	velocity[1] = -100 + 40 * crandom();
	velocity[2] = 200 + 50 * random();

	le->leType = LE_FRAGMENT;
	le->startTime = cg.time;
	le->endTime = le->startTime + cg_brassTime.integer + ( cg_brassTime.integer / 4 ) * random();

	le->pos.trType = TR_GRAVITY;
	le->pos.trTime = cg.time - ( rand() & 15 );

	AnglesToAxis( cent->lerpAngles, v );

	VectorCopy( ejectBrassCasingOrigin, re->origin );

	VectorCopy( re->origin, le->pos.trBase );

	if ( CG_PointContents( re->origin, -1 ) & ( CONTENTS_WATER | CONTENTS_SLIME ) ) { //----(SA)	modified since slime is no longer deadly
//	if ( CG_PointContents( re->origin, -1 ) & CONTENTS_WATER ) {
		waterScale = 0.10;
	}

	xvelocity[0] = velocity[0] * v[0][0] + velocity[1] * v[1][0] + velocity[2] * v[2][0];
	xvelocity[1] = velocity[0] * v[0][1] + velocity[1] * v[1][1] + velocity[2] * v[2][1];
	xvelocity[2] = velocity[0] * v[0][2] + velocity[1] * v[1][2] + velocity[2] * v[2][2];
	VectorScale( xvelocity, waterScale, le->pos.trDelta );

	AxisCopy( axisDefault, re->axis );
	re->hModel = cgs.media.smallgunBrassModel;

	le->bounceFactor = 0.4 * waterScale;

	le->angles.trType = TR_LINEAR;
	le->angles.trTime = cg.time;
	le->angles.trBase[0]  = (rand() & 31) + 60;  // new eject brass RealRTCW
	le->angles.trBase[1]  = rand() & 255; 
	le->angles.trBase[2]  = rand() & 31;
	le->angles.trDelta[0] = 2;
	le->angles.trDelta[1] = 1;
	le->angles.trDelta[2] = 0;

	le->leFlags = LEF_TUMBLE;


	{
		int contents;
		vec3_t end;
		VectorCopy( cent->lerpOrigin, end );
		end[2] -= 24;
		contents = trap_CM_PointContents( end, 0 );
		if ( contents & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ) {
			le->leBounceSoundType = LEBS_NONE;
		} else {
			le->leBounceSoundType = LEBS_BRASS;
		}
	}

	le->leMarkType = LEMT_NONE;
}

/*
==========================
CG_PistolEjectBrass
==========================
*/

static void CG_PistolEjectBrass( centity_t *cent ) {
	localEntity_t   *le;
	refEntity_t     *re;
	vec3_t velocity, xvelocity;
	vec3_t offset, xoffset;
	float waterScale = 1.0f;
	vec3_t v[3];

	if ( cg_brassTime.integer <= 0 ) {
		return;
	}

	if ( !( cg.snap->ps.persistant[PERS_HWEAPON_USE] ) && ( cent->currentState.clientNum == cg.snap->ps.clientNum ) ) {
		CG_PistolEjectBrassNew( cent );
		return;
	}

	le = CG_AllocLocalEntity();
	re = &le->refEntity;

	velocity[0]           = -20 + 40 * crandom(); // New eject brass RealRTCW
	velocity[1]           = -150 + 40 * crandom();
	velocity[2]           = 100 + 50 * crandom();

	le->leType = LE_FRAGMENT;
	le->startTime = cg.time;
	le->endTime = le->startTime + cg_brassTime.integer + ( cg_brassTime.integer / 4 ) * random();

	le->pos.trType = TR_GRAVITY;
	le->pos.trTime = cg.time - ( rand() & 15 );

	AnglesToAxis( cent->lerpAngles, v );

	if ( cg.snap->ps.persistant[PERS_HWEAPON_USE] ) {
		offset[0] = 32;
		offset[1] = -4;
		offset[2] = 0;
	} else if ( cg.predictedPlayerState.weapon == WP_MP40 || cg.predictedPlayerState.weapon == WP_THOMPSON )     {
		offset[0] = 20; // Maxx Kaufman offset value
		offset[1] = -4;
		offset[2] = 24;
	} else if ( cg.predictedPlayerState.weapon == WP_VENOM )     {
		offset[0] = 12;
		offset[1] = -4;
		offset[2] = 24;
	} else {
		VectorClear( offset );
	}

	xoffset[0] = offset[0] * v[0][0] + offset[1] * v[1][0] + offset[2] * v[2][0];
	xoffset[1] = offset[0] * v[0][1] + offset[1] * v[1][1] + offset[2] * v[2][1];
	xoffset[2] = offset[0] * v[0][2] + offset[1] * v[1][2] + offset[2] * v[2][2];
	VectorAdd( cent->lerpOrigin, xoffset, re->origin );

	VectorCopy( re->origin, le->pos.trBase );

	if ( CG_PointContents( re->origin, -1 ) & ( CONTENTS_WATER | CONTENTS_SLIME ) ) { //----(SA)	modified since slime is no longer deadly
		waterScale = 0.10;
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
	le->angles.trBase[0] = rand() & 31;
	le->angles.trBase[1] = rand() & 31;
	le->angles.trBase[2] = rand() & 31;
	le->angles.trDelta[0] = 2;
	le->angles.trDelta[1] = 1;
	le->angles.trDelta[2] = 0;

	le->leFlags = LEF_TUMBLE;

	{
		int contents;
		vec3_t end;
		VectorCopy( cent->lerpOrigin, end );
		end[2] -= 24;
		contents = trap_CM_PointContents( end, 0 );
		if ( contents & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ) {
			le->leBounceSoundType = LEBS_NONE;
		} else {
			le->leBounceSoundType = LEBS_BRASS;
		}
	}

	le->leMarkType = LEMT_NONE;
}

static void CG_PistolEjectBrassDelay( centity_t *cent, int delay ) {
	CG_AllocDelayedBrass( cent, cg.time + delay, CG_PistolEjectBrass );
}

//----(SA)	added
/*
==============
CG_PanzerFaustEjectBrass
	toss the 'used' panzerfaust casing (unit is one-shot, disposable)
==============
*/
static void CG_PanzerFaustEjectBrass( centity_t *cent ) {
	localEntity_t   *le;
	refEntity_t     *re;
	vec3_t velocity, xvelocity;
	vec3_t offset, xoffset;
	float waterScale = 1.0f;
	vec3_t v[3];

	le = CG_AllocLocalEntity();
	re = &le->refEntity;

//	velocity[0] = 16;
//	velocity[1] = -50 + 40 * crandom();
//	velocity[2] = 100 + 50 * crandom();

	velocity[0] = 16;
	velocity[1] = -200;
	velocity[2] = 0;

	le->leType = LE_FRAGMENT;
	le->startTime = cg.time;
//	le->startTime = cg.time + 2000;
	le->endTime = le->startTime + ( cg_brassTime.integer * 8 ) + ( cg_brassTime.integer * random() );

	le->pos.trType = TR_GRAVITY;
	le->pos.trTime = cg.time - ( rand() & 15 );
//	le->pos.trTime = cg.time - 2000;

	AnglesToAxis( cent->lerpAngles, v );

//	offset[0] = 12;
//	offset[1] = -4;
//	offset[2] = 24;

	offset[0] = -24;    // forward
	offset[1] = -4; // left
	offset[2] = 24; // up

	xoffset[0] = offset[0] * v[0][0] + offset[1] * v[1][0] + offset[2] * v[2][0];
	xoffset[1] = offset[0] * v[0][1] + offset[1] * v[1][1] + offset[2] * v[2][1];
	xoffset[2] = offset[0] * v[0][2] + offset[1] * v[1][2] + offset[2] * v[2][2];
	VectorAdd( cent->lerpOrigin, xoffset, re->origin );

	VectorCopy( re->origin, le->pos.trBase );

	if ( CG_PointContents( re->origin, -1 ) & ( CONTENTS_WATER | CONTENTS_SLIME ) ) {
		waterScale = 0.10;
	}

	xvelocity[0] = velocity[0] * v[0][0] + velocity[1] * v[1][0] + velocity[2] * v[2][0];
	xvelocity[1] = velocity[0] * v[0][1] + velocity[1] * v[1][1] + velocity[2] * v[2][1];
	xvelocity[2] = velocity[0] * v[0][2] + velocity[1] * v[1][2] + velocity[2] * v[2][2];
	VectorScale( xvelocity, waterScale, le->pos.trDelta );

	AxisCopy( axisDefault, re->axis );

	// (SA) make it bigger
	le->sizeScale = 3.0f;

	re->hModel = cgs.media.panzerfaustBrassModel;

	le->bounceFactor = 0.4 * waterScale;

	le->angles.trType = TR_LINEAR;
	le->angles.trTime = cg.time;
//	le->angles.trBase[0] = rand()&31;
//	le->angles.trBase[1] = rand()&31;
//	le->angles.trBase[2] = rand()&31;
	le->angles.trBase[0] = 0;
	le->angles.trBase[1] = cent->currentState.apos.trBase[1];   // rotate to match the player
	le->angles.trBase[2] = 0;
//	le->angles.trDelta[0] = 2;
//	le->angles.trDelta[1] = 1;
//	le->angles.trDelta[2] = 0;
	le->angles.trDelta[0] = 0;
	le->angles.trDelta[1] = 0;
	le->angles.trDelta[2] = 0;

	le->leFlags = LEF_TUMBLE | LEF_SMOKING;   // (SA) probably doesn't need to be 'tumble' since it doesn't really rotate much when flying

	le->leBounceSoundType = LEBS_NONE;

	le->leMarkType = LEMT_NONE;
}

static void CG_PanzerFaustEjectBrassDelay( centity_t *cent, int delay ) {
	CG_AllocDelayedBrass( cent, cg.time + delay, CG_PanzerFaustEjectBrass );
}

/*
==============
CG_SpearTrail
	simple bubble trail behind a missile
==============
*/
void CG_SpearTrail( centity_t *ent, const weaponInfo_t *wi ) {
	int contents, lastContents;
	vec3_t origin, lastPos;
	entityState_t   *es;

	es = &ent->currentState;
	BG_EvaluateTrajectory( &es->pos, cg.time, origin );
	contents = CG_PointContents( origin, -1 );

	BG_EvaluateTrajectory( &es->pos, ent->trailTime, lastPos );
	lastContents = CG_PointContents( lastPos, -1 );

	ent->trailTime = cg.time;

	if ( contents & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ) {
		if ( contents & lastContents & CONTENTS_WATER ) {
			CG_BubbleTrail( lastPos, origin, 1, 8 );
		}
	}
}


// JPW NERVE -- LT pyro for marking air strikes
/*
==========================
CG_PyroSmokeTrail
==========================
*/
void CG_PyroSmokeTrail( centity_t *ent, const weaponInfo_t *wi ) {
	int step;
	vec3_t origin, lastPos, dir;
	int startTime;
	entityState_t   *es;
	int t;
	float rnd;
	static float grounddir = 99;

	if ( grounddir == 99 ) { // pick a wind direction -- cheap trick because it can be different
		grounddir = crandom(); // on different clients, but it's all smoke and mirrors anyway
	}
	step = 30;
	es = &ent->currentState;
	startTime = ent->trailTime;
	t = step * ( ( startTime + step ) / step );

	BG_EvaluateTrajectory( &es->pos, cg.time, origin );
	BG_EvaluateTrajectory( &es->pos, ent->trailTime, lastPos );

	ent->trailTime = cg.time;

	// drop fire trail sprites
	for ( ; t <= ent->trailTime ; t += step ) {

		BG_EvaluateTrajectory( &es->pos, t, lastPos );
//		VectorCopy ( ent->lerpOrigin, lastPos );

		if ( ent->currentState.density ) { // corkscrew effect
			vec3_t right;
			vec3_t angles;
			VectorCopy( ent->currentState.apos.trBase, angles );
			angles[ROLL] += cg.time % 360;
			AngleVectors( angles, NULL, right, NULL );
			VectorMA( lastPos, ent->currentState.density, right, lastPos );
		}

		dir[0] = crandom() * 5; // compute offset from flare base
		dir[1] = crandom() * 5;
		dir[2] = 0;
		VectorAdd( lastPos,dir,origin ); // store in origin

		rnd = random();

		dir[0] = random() * 0.25;
		dir[1] = grounddir; // simulate a little wind so it looks natural
		dir[2] = random(); // one direction (so smoke goes side-like)
		VectorNormalize( dir );
		VectorScale( dir,45,dir ); // was 75

		if ( !ent->currentState.otherEntityNum2 ) { // axis team, generate red smoke
			CG_SmokePuff( origin, dir,
							   25 + rnd * 110, // width
							   rnd * 0.5 + 0.5, rnd * 0.5 + 0.5, 1, 0.5,
							   4800 + ( rand() % 2800 ), // duration was 2800+
							   t,
							   0,
							   0,
							   cgs.media.smokePuffShader );
		} else {
			CG_SmokePuff( origin, dir,
							   25 + rnd * 110, // width
							   1.0, rnd * 0.5 + 0.5, rnd * 0.5 + 0.5, 0.5,
							   4800 + ( rand() % 2800 ), // duration was 2800+
							   t,
							   0,
							   0,
							   cgs.media.smokePuffShader );
		}
	}
}
// jpw


// Ridah, new trail effects
/*
==========================
CG_RocketTrail
==========================
*/
void CG_RocketTrail( centity_t *ent, const weaponInfo_t *wi ) {
	int step;
	vec3_t origin, lastPos;
	int contents;
	int lastContents, startTime;
	entityState_t   *es;
	int t;
//	localEntity_t	*le;

	if ( ent->currentState.eType == ET_FLAMEBARREL ) {
		step = 30;
	} else if ( ent->currentState.eType == ET_FP_PARTS ) {
		step = 50;
	} else if ( ent->currentState.eType == ET_RAMJET ) {
		step = 10;
	} else {
		step = 10;
	}

	es = &ent->currentState;
	startTime = ent->trailTime;
	t = step * ( ( startTime + step ) / step );

	BG_EvaluateTrajectory( &es->pos, cg.time, origin );
	contents = CG_PointContents( origin, -1 );

	// if object (e.g. grenade) is stationary, don't toss up smoke
	if ( ( ent->currentState.eType != ET_RAMJET ) && es->pos.trType == TR_STATIONARY ) {
		ent->trailTime = cg.time;
		return;
	}

	BG_EvaluateTrajectory( &es->pos, ent->trailTime, lastPos );
	lastContents = CG_PointContents( lastPos, -1 );

	ent->trailTime = cg.time;

	if ( contents & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ) {
		if ( contents & lastContents & CONTENTS_WATER ) {
			CG_BubbleTrail( lastPos, origin, 3, 8 );
		}
		return;
	}

	// drop fire trail sprites
	for ( ; t <= ent->trailTime ; t += step ) {
		float rnd;

		BG_EvaluateTrajectory( &es->pos, t, lastPos );
		/*
		le = CG_SmokePuff( lastPos, vec3_origin,
					  5,	// width
					  1, 1, 1, 0.33,
					  150 + rand()%350, // duration
					  t,
					  0,
					  cgs.media.flameThrowerhitShader );

		// use the optimized local entity add
		le->leType = LE_SCALE_FADE;
		*/
		rnd = random();
		if ( ent->currentState.eType == ET_FLAMEBARREL ) {
			if ( ( rand() % 100 ) > 50 ) {
				CG_ParticleExplosion( "twiltb2", lastPos, vec3_origin, 100 + (int)( rnd * 400 ), 5, 7 + (int)( rnd * 10 ) ); // fire

			}
			CG_ParticleExplosion( "blacksmokeanim", lastPos, vec3_origin, 800 + (int)( rnd * 1500 ), 5, 12 + (int)( rnd * 30 ) );    // smoke
		} else if ( ent->currentState.eType == ET_FP_PARTS )     {
			if ( ( rand() % 100 ) > 50 ) {
				CG_ParticleExplosion( "twiltb2", lastPos, vec3_origin, 100 + (int)( rnd * 400 ), 5, 7 + (int)( rnd * 10 ) ); // fire

			}
			CG_ParticleExplosion( "blacksmokeanim", lastPos, vec3_origin, 800 + (int)( rnd * 1500 ), 5, 12 + (int)( rnd * 30 ) );    // smoke
		} else if ( ent->currentState.eType == ET_RAMJET )     {
			int duration;

			VectorCopy( ent->lerpOrigin, lastPos );
			duration = 100;
			CG_ParticleExplosion( "twiltb2", lastPos, vec3_origin, duration + (int)( rnd * 100 ), 5, 5 + (int)( rnd * 10 ) ); // fire
			CG_ParticleExplosion( "blacksmokeanim", lastPos, vec3_origin, 400 + (int)( rnd * 750 ), 12, 24 + (int)( rnd * 30 ) );    // smoke
		} else if ( ent->currentState.eType == ET_FIRE_COLUMN || ent->currentState.eType == ET_FIRE_COLUMN_SMOKE )     {
			int duration;
			int sizeStart;
			int sizeEnd;

			//VectorCopy (ent->lerpOrigin, lastPos);

			if ( ent->currentState.density ) { // corkscrew effect
				vec3_t right;
				vec3_t angles;
				VectorCopy( ent->currentState.apos.trBase, angles );
				angles[ROLL] += cg.time % 360;
				AngleVectors( angles, NULL, right, NULL );
				VectorMA( lastPos, ent->currentState.density, right, lastPos );
			}

			duration = ent->currentState.angles[0];
			sizeStart = ent->currentState.angles[1];
			sizeEnd = ent->currentState.angles[2];

			if ( !duration ) {
				duration = 100;
			}

			if ( !sizeStart ) {
				sizeStart = 5;
			}

			if ( !sizeEnd ) {
				sizeEnd = 7;
			}

			CG_ParticleExplosion( "twiltb2", lastPos, vec3_origin, duration + (int)( rnd * 400 ), sizeStart, sizeEnd + (int)( rnd * 10 ) );   // fire

			if ( ent->currentState.eType == ET_FIRE_COLUMN_SMOKE && ( rand() % 100 ) > 50 ) {
				CG_ParticleExplosion( "blacksmokeanim", lastPos, vec3_origin, 800 + (int)( rnd * 1500 ), 5, 12 + (int)( rnd * 30 ) );    // smoke
			}
		} else
		{
			//CG_ParticleExplosion( "twiltb", lastPos, vec3_origin, 300+(int)(rnd*100), 4, 14+(int)(rnd*8) );	// fire
			CG_ParticleExplosion( "blacksmokeanim", lastPos, vec3_origin, 800 + (int)( rnd * 1500 ), 5, 12 + (int)( rnd * 30 ) );    // smoke
		}
	}
/*
	// spawn a smoke junction
	if ((cg.time - ent->lastTrailTime) >= 50 + rand()%50) {
		ent->headJuncIndex = CG_AddSmokeJunc( ent->headJuncIndex,
												cgs.media.smokeTrailShader,
												origin,
												4500, 0.4, 20, 80 );
		ent->lastTrailTime = cg.time;
	}
*/
// done.
}

// Ridah
/*
==========================
CG_GrenadeTrail
==========================
*/
static void CG_GrenadeTrail( centity_t *ent, const weaponInfo_t *wi ) {
	int step;
	vec3_t origin, lastPos;
	int contents;
	int lastContents, startTime;
	entityState_t   *es;
	int t;

	step = 15;  // nice and smooth curves

	es = &ent->currentState;
	startTime = ent->trailTime;
	t = step * ( ( startTime + step ) / step );

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
			CG_BubbleTrail( lastPos, origin, 2, 8 );
		}
		return;
	}

//----(SA)	trying this back on for DM

	// spawn smoke junctions
	for ( ; t <= ent->trailTime ; t += step ) {
		BG_EvaluateTrajectory( &es->pos, t, origin );
		ent->headJuncIndex = CG_AddSmokeJunc( ent->headJuncIndex,
											  cgs.media.smokeTrailShader,
											  origin,
//												1500, 0.3, 10, 50 );
											  1000, 0.3, 2, 20 );
		ent->lastTrailTime = cg.time;
	}
//----(SA)	end
}
// done.



/*
==========================
CG_RailTrail
	SA: re-inserted this as a debug mechanism for bullets
==========================
*/
void CG_RailTrail2( clientInfo_t *ci, vec3_t start, vec3_t end ) {
	localEntity_t   *le;
	refEntity_t     *re;

	le = CG_AllocLocalEntity();
	re = &le->refEntity;

	le->leType = LE_FADE_RGB;
	le->startTime = cg.time;
	le->endTime = cg.time + cg_railTrailTime.value;
	le->lifeRate = 1.0 / ( le->endTime - le->startTime );

	re->shaderTime = cg.time / 1000.0f;
	re->reType = RT_RAIL_CORE;
	re->customShader = cgs.media.railCoreShader;

	VectorCopy( start, re->origin );
	VectorCopy( end, re->oldorigin );

//	// still allow different colors so we can tell AI shots from player shots, etc.
	if ( ci ) {
		le->color[0] = ci->color[0] * 0.75;
		le->color[1] = ci->color[1] * 0.75;
		le->color[2] = ci->color[2] * 0.75;
	} else {
		le->color[0] = 1;
		le->color[1] = 0;
		le->color[2] = 0;
	}
	le->color[3] = 1.0f;

	AxisClear( re->axis );
}

//void CG_RailTrailBox( clientInfo_t *ci, vec3_t start, vec3_t end) {
/*
==============
CG_RailTrail
	modified so we could draw boxes for debugging as well
==============
*/
void CG_RailTrail( clientInfo_t *ci, vec3_t start, vec3_t end, int type ) {  //----(SA)	added 'type'
	vec3_t diff, v1, v2, v3, v4, v5, v6;

	if ( !type ) { // just a line
		CG_RailTrail2( ci, start, end );
		return;
	}

	// type '1' (box)

	VectorSubtract( start, end, diff );

	VectorCopy( start, v1 );
	VectorCopy( start, v2 );
	VectorCopy( start, v3 );
	v1[0] -= diff[0];
	v2[1] -= diff[1];
	v3[2] -= diff[2];
	CG_RailTrail2( ci, start, v1 );
	CG_RailTrail2( ci, start, v2 );
	CG_RailTrail2( ci, start, v3 );

	VectorCopy( end, v4 );
	VectorCopy( end, v5 );
	VectorCopy( end, v6 );
	v4[0] += diff[0];
	v5[1] += diff[1];
	v6[2] += diff[2];
	CG_RailTrail2( ci, end, v4 );
	CG_RailTrail2( ci, end, v5 );
	CG_RailTrail2( ci, end, v6 );

	CG_RailTrail2( ci, v2, v6 );
	CG_RailTrail2( ci, v6, v1 );
	CG_RailTrail2( ci, v1, v5 );

	CG_RailTrail2( ci, v2, v4 );
	CG_RailTrail2( ci, v4, v3 );
	CG_RailTrail2( ci, v3, v5 );

}




/*
======================
CG_ParseWeaponConfig
	read information for weapon animations (first/length/fps)
======================
*/
static qboolean CG_ParseWeaponConfig( const char *filename, weaponInfo_t *wi ) {
	char        *text_p, *prev;
	int len;
	int i;
	float fps;
	char        *token;
	qboolean newfmt = qfalse;       //----(SA)
	char text[20000];
	fileHandle_t f;

	// load the file
	len = trap_FS_FOpenFile( filename, &f, FS_READ );
	if ( len <= 0 ) {
		return qfalse;
	}

	if ( len >= sizeof( text ) - 1 ) {
		CG_Printf( "File %s too long\n", filename );
		return qfalse;
	}

	trap_FS_Read( text, len, f );
	text[len] = 0;
	trap_FS_FCloseFile( f );

	// parse the text
	text_p = text;

	// read optional parameters
	while ( 1 ) {
		prev = text_p;  // so we can unget
		token = COM_Parse( &text_p );
		if ( !token ) {                     // get the variable
			break;
		}
/*		if ( !Q_stricmp( token, "whatever_variable" ) ) {
			token = COM_Parse( &text_p );	// get the value
			if ( !token ) {
				break;
			}
			continue;
		}*/

		if ( !Q_stricmp( token, "newfmt" ) ) {
			newfmt = qtrue;
			continue;
		}

		// if it is a number, start parsing animations
		if ( token[0] >= '0' && token[0] <= '9' ) {
			text_p = prev;  // unget the token
			break;
		}
		Com_Printf( "unknown token in weapon cfg '%s' is %s\n", token, filename );
	}


	for ( i = 0 ; i < MAX_WP_ANIMATIONS  ; i++ ) {

		token = COM_Parse( &text_p );   // first frame
		if ( !token ) {
			break;
		}
		wi->weapAnimations[i].firstFrame = atoi( token );

		token = COM_Parse( &text_p );   // length
		if ( !token ) {
			break;
		}
		wi->weapAnimations[i].numFrames = atoi( token );

		token = COM_Parse( &text_p );   // fps
		if ( !token ) {
			break;
		}
		fps = atof( token );
		if ( fps == 0 ) {
			fps = 1;
		}

		wi->weapAnimations[i].frameLerp = 1000 / fps;
		wi->weapAnimations[i].initialLerp = 1000 / fps;

		token = COM_Parse( &text_p );   // looping frames
		if ( !token ) {
			break;
		}
		wi->weapAnimations[i].loopFrames = atoi( token );
		if ( wi->weapAnimations[i].loopFrames > wi->weapAnimations[i].numFrames ) {
			wi->weapAnimations[i].loopFrames = wi->weapAnimations[i].numFrames;
		} else if ( wi->weapAnimations[i].loopFrames < 0 ) {
			wi->weapAnimations[i].loopFrames = 0;
		}


		// store animation/draw bits in '.moveSpeed'

		wi->weapAnimations[i].moveSpeed = 0;

		if ( newfmt ) {
			token = COM_Parse( &text_p );   // barrel anim bits
			if ( !token ) {
				break;
			}
			wi->weapAnimations[i].moveSpeed = atoi( token );

			token = COM_Parse( &text_p );   // animated weapon
			if ( !token ) {
				break;
			}
			if ( atoi( token ) ) {
				wi->weapAnimations[i].moveSpeed |= ( 1 << W_MAX_PARTS );    // set the bit one higher than can be set by the barrel bits

			}
			token = COM_Parse( &text_p );   // barrel hide bits (so objects can be flagged to not be drawn during all sequences (a reloading hand that comes in from off screen for that one animation for example)
			if ( !token ) {
				break;
			}
			wi->weapAnimations[i].moveSpeed |= ( ( atoi( token ) ) << 8 ); // use 2nd byte for draw bits
		}

	}

	if ( i != MAX_WP_ANIMATIONS ) {
		CG_Printf( "Error parsing weapon animation file: %s", filename );
		return qfalse;
	}


	return qtrue;
}


static qboolean CG_RW_ParseError( int handle, char *format, ... ) {
	int line;
	char filename[128];
	va_list argptr;
	static char string[4096];

	va_start( argptr, format );
	Q_vsnprintf( string, sizeof( string ), format, argptr );
	va_end( argptr );

	filename[0] = '\0';
	line = 0;
	trap_PC_SourceFileAndLine( handle, filename, &line );

	Com_Printf( S_COLOR_RED "ERROR: %s, line %d: %s\n", filename, line, string );

	trap_PC_FreeSource( handle );

	return qfalse;
}

static qboolean CG_RW_ParseWeaponLinkPart( int handle, weaponInfo_t *weaponInfo, modelViewType_t viewType ) {
	pc_token_t token;
	char filename[MAX_QPATH];
	int part;
	partModel_t *partModel;

	if ( !PC_Int_Parse( handle, &part ) ) {
		return CG_RW_ParseError( handle, "expected part index" );
	}

	if ( part < 0 || part >= W_MAX_PARTS ) {
		return CG_RW_ParseError( handle, "part index out of bounds" );
	}

	partModel = &weaponInfo->partModels[viewType][part];

	memset( partModel, 0, sizeof( *partModel ) );

	if ( !trap_PC_ReadToken( handle, &token ) || Q_stricmp( token.string, "{" ) ) {
		return CG_RW_ParseError( handle, "expected '{'" );
	}

	while ( 1 ) {
		if ( !trap_PC_ReadToken( handle, &token ) ) {
			break;
		}

		if ( token.string[0] == '}' ) {
			break;
		}

		if ( !Q_stricmp( token.string, "tag" ) ) {
			if ( !PC_String_ParseNoAlloc( handle, partModel->tagName, sizeof( partModel->tagName ) ) ) {
				return CG_RW_ParseError( handle, "expected tag name" );
			}
		} else if ( !Q_stricmp( token.string, "model" ) ) {
			if ( !PC_String_ParseNoAlloc( handle, filename, sizeof( filename ) ) ) {
				return CG_RW_ParseError( handle, "expected model filename" );
			} else {
				partModel->model = trap_R_RegisterModel( filename );
			}
		} else if ( !Q_stricmp( token.string, "skin" ) ) {
			if ( !PC_String_ParseNoAlloc( handle, filename, sizeof( filename ) ) ) {
				return CG_RW_ParseError( handle, "expected skin filename" );
			} else {
				partModel->skin[0] = trap_R_RegisterSkin( filename );
			}
		} else {
			return CG_RW_ParseError( handle, "unknown token '%s'", token.string );
		}
	}

	return qtrue;
}

static qboolean CG_RW_ParseWeaponLink( int handle, weaponInfo_t *weaponInfo, modelViewType_t viewType ) {
	pc_token_t token;

	if ( !trap_PC_ReadToken( handle, &token ) || Q_stricmp( token.string, "{" ) ) {
		return CG_RW_ParseError( handle, "expected '{'" );
	}

	while ( 1 ) {
		if ( !trap_PC_ReadToken( handle, &token ) ) {
			break;
		}

		if ( token.string[0] == '}' ) {
			break;
		}

		if ( !Q_stricmp( token.string, "part" ) ) {
			if ( !CG_RW_ParseWeaponLinkPart( handle, weaponInfo, viewType ) ) {
				return qfalse;
			}
		} else {
			return CG_RW_ParseError( handle, "unknown token '%s'", token.string );
		}
	}

	return qtrue;
}

static qboolean CG_RW_ParseViewType( int handle, weaponInfo_t *weaponInfo, modelViewType_t viewType ) {
	pc_token_t token;
	char filename[MAX_QPATH];

	if ( !trap_PC_ReadToken( handle, &token ) || Q_stricmp( token.string, "{" ) ) {
		return CG_RW_ParseError( handle, "expected '{'" );
	}

	while ( 1 ) {
		if ( !trap_PC_ReadToken( handle, &token ) ) {
			break;
		}

		if ( token.string[0] == '}' ) {
			break;
		}

		if ( !Q_stricmp( token.string, "model" ) ) {
			if ( !PC_String_ParseNoAlloc( handle, filename, sizeof( filename ) ) ) {
				return CG_RW_ParseError( handle, "expected model filename" );
			} else {
				weaponInfo->weaponModel[viewType].model = trap_R_RegisterModel( filename );
			}
		} else if ( !Q_stricmp( token.string, "skin" ) ) {
			if ( !PC_String_ParseNoAlloc( handle, filename, sizeof( filename ) ) ) {
				return CG_RW_ParseError( handle, "expected skin filename" );
			} else {
				weaponInfo->weaponModel[viewType].skin[0] = trap_R_RegisterSkin( filename );
			}
		} else if ( !Q_stricmp( token.string, "flashModel" ) ) {
			if ( !PC_String_ParseNoAlloc( handle, filename, sizeof( filename ) ) ) {
				return CG_RW_ParseError( handle, "expected flashModel filename" );
			} else {
				weaponInfo->flashModel[viewType] = trap_R_RegisterModel( filename );
			}
		} else if ( !Q_stricmp( token.string, "weaponLink" ) ) {
			if ( !CG_RW_ParseWeaponLink( handle, weaponInfo, viewType ) ) {
				return qfalse;
			}
		} else {
			return CG_RW_ParseError( handle, "unknown token '%s'", token.string );
		}
	}

	return qtrue;
}

static qboolean CG_RW_ParseModModel( int handle, weaponInfo_t *weaponInfo ) {
	char filename[MAX_QPATH];
	int mod;

	if ( !PC_Int_Parse( handle, &mod ) ) {
		return CG_RW_ParseError( handle, "expected mod index" );
	}

	if ( mod < 0 || mod >= 6 ) {
		return CG_RW_ParseError( handle, "mod index out of bounds" );
	}

	if ( !PC_String_ParseNoAlloc( handle, filename, sizeof( filename ) ) ) {
		return CG_RW_ParseError( handle, "expected model filename" );
	} else {
		weaponInfo->modModels[mod] = trap_R_RegisterModel( filename );
		if ( !weaponInfo->modModels[mod] ) {
			// maybe it's a shader
			weaponInfo->modModels[mod] = trap_R_RegisterShader( filename );
		}
	}

	return qtrue;
}

static qboolean CG_RW_ParseClient( int handle, weaponInfo_t *weaponInfo, int weaponNum ) {
	pc_token_t token;
	char filename[MAX_QPATH];
	int i = 0;

	weaponInfo->reloadFullSound = 0;
	weaponInfo->reloadFullSoundFast = 0;
	weaponInfo->reloadSoundFast = 0;
	weaponInfo->reloadSoundAi = 0;
	weaponInfo->flashSoundAi[i] = 0;

	if ( !trap_PC_ReadToken( handle, &token ) || Q_stricmp( token.string, "{" ) ) {
		return CG_RW_ParseError( handle, "expected '{'" );
	}

	while ( 1 ) {
		if ( !trap_PC_ReadToken( handle, &token ) ) {
			break;
		}

		if ( token.string[0] == '}' ) {
			break;
		}

		if ( !Q_stricmp( token.string, "standModel" ) ) {
			if ( !PC_String_ParseNoAlloc( handle, filename, sizeof( filename ) ) ) {
				return CG_RW_ParseError( handle, "expected standModel filename" );
			} else {
				weaponInfo->standModel = trap_R_RegisterModel( filename );
			}
		} else if ( !Q_stricmp( token.string, "droppedAnglesHack" ) ) {
			weaponInfo->droppedAnglesHack = qtrue;
		} else if ( !Q_stricmp( token.string, "pickupModel" ) ) {
			if ( !PC_String_ParseNoAlloc( handle, filename, sizeof( filename ) ) ) {
				return CG_RW_ParseError( handle, "expected pickupModel filename" );
			} else {
				weaponInfo->weaponModel[W_PU_MODEL].model = trap_R_RegisterModel( filename );
			}
		} else if ( !Q_stricmp( token.string, "weaponConfig" ) ) {
			if ( !PC_String_ParseNoAlloc( handle, filename, sizeof( filename ) ) ) {
				return CG_RW_ParseError( handle, "expected weaponConfig filename" );
			} else {
				if ( !CG_ParseWeaponConfig( filename, weaponInfo ) ) {
//					CG_Error( "Couldn't register weapon %i (failed to parse %s)", weaponNum, filename );
				}
			}
		} else if ( !Q_stricmp( token.string, "handsModel" ) ) {
			if (!PC_String_ParseNoAlloc(handle, filename, sizeof(filename)))
			{
				return CG_RW_ParseError(handle, "expected handsModel filename");
			}
			weaponInfo->handsModel = trap_R_RegisterModel(filename);

			char base[128], map[128];
			char handsskin[128], upgradedSkin[128], upgradedMapSkin[128];

			memset(base, 0, sizeof(base));
			memset(map, 0, sizeof(map));
			COM_StripExtension(filename, base, sizeof(base));
			trap_Cvar_VariableStringBuffer("mapname", map, sizeof(map));

			// Map-specific hands skin
			Com_sprintf(handsskin, sizeof(handsskin), "%s_%s.skin", base, map);
			weaponInfo->handsSkin = trap_R_RegisterSkin(handsskin);

			// Generic upgraded skin
			Com_sprintf(upgradedSkin, sizeof(upgradedSkin), "%s_upgraded.skin", base);
			weaponInfo->upgradedSkin = trap_R_RegisterSkin(upgradedSkin);

			// Map-specific upgraded skin
			Com_sprintf(upgradedMapSkin, sizeof(upgradedMapSkin), "%s_upgraded_%s.skin", base, map);
			weaponInfo->upgradedMapSkin = trap_R_RegisterSkin(upgradedMapSkin);
		} else if ( !Q_stricmp( token.string, "flashDlightColor" ) ) {
			if ( !PC_Vec_Parse( handle, &weaponInfo->flashDlightColor ) ) {
				return CG_RW_ParseError( handle, "expected flashDlightColor as r g b" );
			}
		} else if ( !Q_stricmp( token.string, "flashSound" ) ) {
			if ( !PC_String_ParseNoAlloc( handle, filename, sizeof( filename ) ) ) {
				return CG_RW_ParseError( handle, "expected flashSound filename" );
			} else {
				for ( i = 0; i < 4; i++ ) {
					if ( !weaponInfo->flashSound[i] ) {
						weaponInfo->flashSound[i] = trap_S_RegisterSound( filename );
						break;
					}
				}
				if ( i == 4 ) {
					CG_Printf( S_COLOR_YELLOW "WARNING: only up to 4 flashSounds supported per weapon\n" );
				}
			}
		} else if ( !Q_stricmp( token.string, "flashSoundAi" ) ) {
			if ( !PC_String_ParseNoAlloc( handle, filename, sizeof( filename ) ) ) {
				return CG_RW_ParseError( handle, "expected flashSound filename" );
			} else {
				for ( i = 0; i < 4; i++ ) {
					if ( !weaponInfo->flashSoundAi[i] ) {
						weaponInfo->flashSoundAi[i] = trap_S_RegisterSound( filename );
						break;
					}
				}
				if ( i == 4 ) {
					CG_Printf( S_COLOR_YELLOW "WARNING: only up to 4 flashSounds supported per weapon\n" );
				}
			}
		} else if ( !Q_stricmp( token.string, "flashEchoSound" ) ) {
			if ( !PC_String_ParseNoAlloc( handle, filename, sizeof( filename ) ) ) {
				return CG_RW_ParseError( handle, "expected flashEchoSound filename" );
			} else {
				for ( i = 0; i < 4; i++ ) {
					if ( !weaponInfo->flashEchoSound[i] ) {
						weaponInfo->flashEchoSound[i] = trap_S_RegisterSound( filename );
						break;
					}
				}
				if ( i == 4 ) {
					CG_Printf( S_COLOR_YELLOW "WARNING: only up to 4 flashEchoSounds supported per weapon\n" );
				}
			}
		} else if ( !Q_stricmp( token.string, "lastShotSound" ) ) {
			if ( !PC_String_ParseNoAlloc( handle, filename, sizeof( filename ) ) ) {
				return CG_RW_ParseError( handle, "expected lastShotSound filename" );
			} else {
				for ( i = 0; i < 4; i++ ) {
					if ( !weaponInfo->lastShotSound[i] ) {
						weaponInfo->lastShotSound[i] = trap_S_RegisterSound( filename );
						break;
					}
				}
				if ( i == 4 ) {
					CG_Printf( S_COLOR_YELLOW "WARNING: only up to 4 lastShotSound supported per weapon\n" );
				}
			}
		} else if ( !Q_stricmp( token.string, "readySound" ) ) {
			if ( !PC_String_ParseNoAlloc( handle, filename, sizeof( filename ) ) ) {
				return CG_RW_ParseError( handle, "expected readySound filename" );
			} else {
				weaponInfo->readySound = trap_S_RegisterSound( filename );
			}
		} else if ( !Q_stricmp( token.string, "firingSound" ) ) {
			if ( !PC_String_ParseNoAlloc( handle, filename, sizeof( filename ) ) ) {
				return CG_RW_ParseError( handle, "expected firingSound filename" );
			} else {
				weaponInfo->firingSound = trap_S_RegisterSound( filename );
			}
		} else if ( !Q_stricmp( token.string, "overheatSound" ) ) {
			if ( !PC_String_ParseNoAlloc( handle, filename, sizeof( filename ) ) ) {
				return CG_RW_ParseError( handle, "expected overheatSound filename" );
			} else {
				weaponInfo->overheatSound = trap_S_RegisterSound( filename );
			}
		} else if ( !Q_stricmp( token.string, "reloadSound" ) ) {
			if ( !PC_String_ParseNoAlloc( handle, filename, sizeof( filename ) ) ) {
				return CG_RW_ParseError( handle, "expected reloadSound filename" );
			} else {
				weaponInfo->reloadSound = trap_S_RegisterSound( filename );
			}
		} else if ( !Q_stricmp( token.string, "reloadSoundFast" ) ) {
			if ( !PC_String_ParseNoAlloc( handle, filename, sizeof( filename ) ) ) {
				return CG_RW_ParseError( handle, "expected reloadSound filename" );
			} else {
				weaponInfo->reloadSoundFast = trap_S_RegisterSound( filename );
			}
		} else if ( !Q_stricmp( token.string, "reloadFullSound" ) ) {
			if ( !PC_String_ParseNoAlloc( handle, filename, sizeof( filename ) ) ) {
				return CG_RW_ParseError( handle, "expected reloadFullSound filename" );
			} else {
				weaponInfo->reloadFullSound = trap_S_RegisterSound( filename );
			}
		} else if ( !Q_stricmp( token.string, "reloadFullSoundFast" ) ) {
			if ( !PC_String_ParseNoAlloc( handle, filename, sizeof( filename ) ) ) {
				return CG_RW_ParseError( handle, "expected reloadFullSound filename" );
			} else {
				weaponInfo->reloadFullSoundFast = trap_S_RegisterSound( filename );
			}
		} else if ( !Q_stricmp( token.string, "reloadSoundAi" ) ) {
			if ( !PC_String_ParseNoAlloc( handle, filename, sizeof( filename ) ) ) {
				return CG_RW_ParseError( handle, "expected reloadSoundAi filename" );
			} else {
				weaponInfo->reloadSoundAi = trap_S_RegisterSound( filename );
			}
		} else if ( !Q_stricmp( token.string, "reloadFastSound" ) ) {
			if ( !PC_String_ParseNoAlloc( handle, filename, sizeof( filename ) ) ) {
				return CG_RW_ParseError( handle, "expected reloadFastSound filename" );
			} else {
				weaponInfo->reloadFastSound = trap_S_RegisterSound( filename );
			}
		} else if ( !Q_stricmp( token.string, "spinupSound" ) ) {
			if ( !PC_String_ParseNoAlloc( handle, filename, sizeof( filename ) ) ) {
				return CG_RW_ParseError( handle, "expected spinupSound filename" );
			} else {
				weaponInfo->spinupSound = trap_S_RegisterSound( filename );
			}
		} else if ( !Q_stricmp( token.string, "spindownSound" ) ) {
			if ( !PC_String_ParseNoAlloc( handle, filename, sizeof( filename ) ) ) {
				return CG_RW_ParseError( handle, "expected spindownSound filename" );
			} else {
				weaponInfo->spindownSound = trap_S_RegisterSound( filename );
			}
		} else if ( !Q_stricmp( token.string, "switchSound" ) ) {
			if ( !PC_String_ParseNoAlloc( handle, filename, sizeof( filename ) ) ) {
				return CG_RW_ParseError( handle, "expected switchSound filename" );
			} else {
				weaponInfo->switchSound[0] = trap_S_RegisterSound( filename );
			}
		} else if ( !Q_stricmp( token.string, "bounceSound" ) ) {
			if ( !PC_String_ParseNoAlloc( handle, filename, sizeof( filename ) ) ) {
				return CG_RW_ParseError( handle, "expected bounceSound filename" );
			} else {
				weaponInfo->bounceSound = trap_S_RegisterSound( filename );
			}
		} else if ( !Q_stricmp( token.string, "weaponIcon" ) ) {
			if ( !PC_String_ParseNoAlloc( handle, filename, sizeof( filename ) ) ) {
				return CG_RW_ParseError( handle, "expected weaponIcon filename" );
			} else {
				weaponInfo->weaponIcon[0] = trap_R_RegisterShader( filename );
			}
		} else if ( !Q_stricmp( token.string, "weaponSelectedIcon" ) ) {
			if ( !PC_String_ParseNoAlloc( handle, filename, sizeof( filename ) ) ) {
				return CG_RW_ParseError( handle, "expected weaponSelectedIcon filename" );
			} else {
				weaponInfo->weaponIcon[1] = trap_R_RegisterShader( filename );
			}

		} else if ( !Q_stricmp( token.string, "missileModel" ) ) {
			if ( !PC_String_ParseNoAlloc( handle, filename, sizeof( filename ) ) ) {
				return CG_RW_ParseError( handle, "expected missileModel filename" );
			} else {
				weaponInfo->missileModel = trap_R_RegisterModel( filename );
			}
		} else if ( !Q_stricmp( token.string, "missileSound" ) ) {
			if ( !PC_String_ParseNoAlloc( handle, filename, sizeof( filename ) ) ) {
				return CG_RW_ParseError( handle, "expected missileSound filename" );
			} else {
				weaponInfo->missileSound = trap_S_RegisterSound( filename );
			}
		} else if ( !Q_stricmp( token.string, "missileTrailFunc" ) ) {
			if ( !PC_String_ParseNoAlloc( handle, filename, sizeof( filename ) ) ) {
				return CG_RW_ParseError( handle, "expected missileTrailFunc" );
			} else {
				if ( !Q_stricmp( filename, "GrenadeTrail" ) ) {
					weaponInfo->missileTrailFunc = CG_GrenadeTrail;
				} else if ( !Q_stricmp( filename, "RocketTrail" ) ) {
					weaponInfo->missileTrailFunc = CG_RocketTrail;
				} else if ( !Q_stricmp( filename, "PyroSmokeTrail" ) ) {
					weaponInfo->missileTrailFunc = CG_PyroSmokeTrail;
				} 
			}
		} else if ( !Q_stricmp( token.string, "missileDlight" ) ) {
			if ( !PC_Float_Parse( handle, &weaponInfo->missileDlight ) ) {
				return CG_RW_ParseError( handle, "expected missileDlight value" );
			}
		} else if ( !Q_stricmp( token.string, "wiTrailTime" ) ) {
			if ( !PC_Int_Parse( handle, (int *)&weaponInfo->wiTrailTime ) ) {
				return CG_RW_ParseError( handle, "expected wiTrailTime value" );
			}
		} else if ( !Q_stricmp( token.string, "trailRadius" ) ) {
			if ( !PC_Int_Parse( handle, (int *)&weaponInfo->trailRadius ) ) {
				return CG_RW_ParseError( handle, "expected trailRadius value" );
			}
		} else if ( !Q_stricmp( token.string, "missileDlightColor" ) ) {
			if ( !PC_Vec_Parse( handle, &weaponInfo->missileDlightColor ) ) {
				return CG_RW_ParseError( handle, "expected missileDlightColor as r g b" );
			}
		} else if ( !Q_stricmp( token.string, "weaponPosition" ) ) {
			if ( !PC_Vec_Parse( handle, &weaponInfo->weaponPosition ) ) {
			return CG_RW_ParseError( handle, "expected XYZ" );
		}
		} else if ( !Q_stricmp( token.string, "weaponPositionAlt" ) ) {
			if ( !PC_Vec_Parse( handle, &weaponInfo->weaponPositionAlt ) ) {
			return CG_RW_ParseError( handle, "expected XYZ" );
		}
		} else if ( !Q_stricmp( token.string, "weaponPositionAlt2" ) ) {
			if ( !PC_Vec_Parse( handle, &weaponInfo->weaponPositionAlt2 ) ) {
			return CG_RW_ParseError( handle, "expected XYZ" );
		}
		} else if ( !Q_stricmp( token.string, "ejectBrassFunc" ) ) {
			if ( !PC_String_ParseNoAlloc( handle, filename, sizeof( filename ) ) ) {
				return CG_RW_ParseError( handle, "expected ejectBrassFunc" );
			} else {
				if ( !Q_stricmp( filename, "MachineGunEjectBrass" ) ) {
					weaponInfo->ejectBrassFunc = CG_MachineGunEjectBrassDelay;
				} else if ( !Q_stricmp( filename, "PanzerFaustEjectBrass" ) ) {
					weaponInfo->ejectBrassFunc = CG_PanzerFaustEjectBrassDelay;
				} else if ( !Q_stricmp( filename, "PistolEjectBrass" ) ) {
					weaponInfo->ejectBrassFunc = CG_PistolEjectBrassDelay;
				} else if ( !Q_stricmp( filename, "ShotgunEjectBrass" ) ) {
					weaponInfo->ejectBrassFunc = CG_ShotgunEjectBrassDelay;
				}
			}
		} else if ( !Q_stricmp( token.string, "modModel" ) ) {
			if ( !CG_RW_ParseModModel( handle, weaponInfo ) ) {
				return qfalse;
			}
		} else if ( !Q_stricmp( token.string, "firstPerson" ) ) {
			if ( !CG_RW_ParseViewType( handle, weaponInfo, W_FP_MODEL ) ) {
				return qfalse;
			}
		} else if ( !Q_stricmp( token.string, "thirdPerson" ) ) {
			if ( !CG_RW_ParseViewType( handle, weaponInfo, W_TP_MODEL ) ) {
				return qfalse;
			}
		} else {
			return CG_RW_ParseError( handle, "unknown token '%s'", token.string );
		}
	}

    // If reloadFullSound is not set, use reloadSound
    if (weaponInfo->reloadFullSound == 0) {
        weaponInfo->reloadFullSound = weaponInfo->reloadSound;
    }


	if (weaponInfo->reloadFullSoundFast == 0) {
        weaponInfo->reloadFullSoundFast = weaponInfo->reloadSoundFast;
    }

	if (weaponInfo->reloadSoundFast == 0) {
        weaponInfo->reloadSoundFast = weaponInfo->reloadSound;
    }

	if (weaponInfo->reloadSoundAi == 0) {
        weaponInfo->reloadSoundAi = weaponInfo->reloadSound;
    }

	if (weaponInfo->flashSoundAi[i] == 0) {
        weaponInfo->flashSoundAi[i] = weaponInfo->flashSound[i];
    }


	return qtrue;
}

static qboolean CG_RegisterWeaponFromWeaponFile( const char *filename, weaponInfo_t *weaponInfo, int weaponNum ) {
	pc_token_t token;
	int handle;

	handle = trap_PC_LoadSource( filename );

	if ( !handle ) {
		return qfalse;
	}

	if ( !trap_PC_ReadToken( handle, &token ) || Q_stricmp( token.string, "weaponDef" ) ) {
		return CG_RW_ParseError( handle, "expected 'weaponDef'" );
	}

	if ( !trap_PC_ReadToken( handle, &token ) || Q_stricmp( token.string, "{" ) ) {
		return CG_RW_ParseError( handle, "expected '{'" );
	}

	while ( 1 ) {
		if ( !trap_PC_ReadToken( handle, &token ) ) {
			break;
		}

		if ( token.string[0] == '}' ) {
			break;
		}

		if ( !Q_stricmp( token.string, "client" ) ) {
			if ( !CG_RW_ParseClient( handle, weaponInfo, weaponNum ) ) {
				return CG_RW_ParseError( handle, "parsing client block failed" );
			}
		} else if ( !Q_stricmp( token.string, "ammo" ) ) {
			if ( !BG_ParseAmmoTable( handle, weaponNum ) ) {
				return CG_RW_ParseError( handle, "parsing ammo block failed" );
			}
			BG_SetWeaponForSkill( weaponNum, cg_gameSkill.integer );
		} else {
			return CG_RW_ParseError( handle, "unknown token '%s'", token.string );
		}
	}

	trap_PC_FreeSource( handle );

	return qtrue;
}


/*
=================
CG_RegisterWeapon

The server says this item is used on this level
=================
*/
void CG_RegisterWeapon( int weaponNum, qboolean force ) {
	weaponInfo_t    *weaponInfo;
	char            *filename;
	
	if ( weaponNum <= 0 || weaponNum >= WP_NUM_WEAPONS ) {
		return;
	}

	weaponInfo = &cg_weapons[weaponNum];

	if ( weaponInfo->registered && !force ) {
		return;
	}

	memset( weaponInfo, 0, sizeof( *weaponInfo ) );
	weaponInfo->registered = qtrue;

	filename = BG_GetWeaponFilename( weaponNum );
	if ( !*filename )
		return;
     
	if ( cg_vanilla_guns.integer ) {
     	if ( !CG_RegisterWeaponFromWeaponFile( va( "weapons/vanilla/%s", filename ), weaponInfo, weaponNum ) ) {
		//CG_Printf( S_COLOR_RED "WARNING: failed to register media for weapon %i from %s\n", weaponNum, filename );
	}
	} else {
	    if ( !CG_RegisterWeaponFromWeaponFile( va( "weapons/%s", filename ), weaponInfo, weaponNum ) ) {
		//CG_Printf( S_COLOR_RED "WARNING: failed to register media for weapon %i from %s\n", weaponNum, filename );
	}
	}

}
/*
=================
CG_RegisterItemVisuals

The server says this item is used on this level
=================
*/
void CG_RegisterItemVisuals( int itemNum ) {
	itemInfo_t      *itemInfo;
	gitem_t         *item;
	int i;

	itemInfo = &cg_items[ itemNum ];
	if ( itemInfo->registered ) {
		return;
	}

	item = &bg_itemlist[ itemNum ];

	memset( itemInfo, 0, sizeof( *itemInfo ) );

	for ( i = 0; i < MAX_ITEM_MODELS; i++ )
		itemInfo->models[i] = trap_R_RegisterModel( item->world_model[i] );


	itemInfo->icons[0] = trap_R_RegisterShader( item->icon );
	if ( item->giType == IT_HOLDABLE ) {
		// (SA) register alternate icons (since holdables can have multiple uses, they might have different icons to represent how many uses are left)
		for ( i = 1; i < MAX_ITEM_ICONS; i++ )
			itemInfo->icons[i] = trap_R_RegisterShader( va( "%s%i", item->icon, i + 1 ) );
	}

	itemInfo->registered = qtrue;   //----(SA)	moved this down after the registerweapon()
	
	char legskin[128]; //eugeny
	char map[128];

	memset( legskin, 0, sizeof( legskin ) );
	memset( map, 0, sizeof( map ) );
	trap_Cvar_VariableStringBuffer( "mapname", map, sizeof( map ) );
	Com_sprintf( legskin, sizeof( legskin ), "%s_%s.skin", "models/weapons/melee/foot/legs", map );
	wolfkickSkin = trap_R_RegisterSkin( legskin );
	
	wolfkickModel = trap_R_RegisterModel( "models/weapons/melee/foot/v_wolfoot_10f.md3" );
	
	hWeaponSnd = trap_S_RegisterSound( "sound/weapons/mg42/37mm.wav" );
	hWeaponEchoSnd = trap_S_RegisterSound( "sound/weapons/mg42/37mm_far.wav" );

	hflakWeaponSnd = trap_S_RegisterSound( "sound/weapons/flak/flak.wav" );
	notebookModel = trap_R_RegisterModel( "models/mapobjects/book/book.md3" );
	propellerModel = trap_R_RegisterModel( "models/mapobjects/vehicles/m109_prop.md3" );

	maxWeapBanks = MAX_WEAP_BANKS;
	maxWeapsInBank = MAX_WEAPS_IN_BANK;

}


/*
========================================================================================

VIEW WEAPON

========================================================================================
*/


//
// weapon animations
//

/*
==============
CG_GetPartFramesFromWeap
	get animation info from the parent if necessary
==============
*/
qboolean CG_GetPartFramesFromWeap( centity_t *cent, refEntity_t *part, refEntity_t *parent, int partid, weaponInfo_t *wi ) {
	int i;
	int frameoffset = 0;
	animation_t *anim;

	anim = cent->pe.weap.animation;

	if ( partid == W_MAX_PARTS ) {
		return qtrue;   // primary weap model drawn for all frames right now
	}

	// check draw bit
	if ( anim->moveSpeed & ( 1 << ( partid + 8 ) ) ) {    // hide bits are in high byte
		return qfalse;  // not drawn for current sequence
	}

	// find part's start frame for this animation sequence
	for ( i = 0; i < cent->pe.weap.animationNumber; i++ ) {
		if ( wi->weapAnimations[i].moveSpeed & ( 1 << partid ) ) {     // this part has animation for this sequence
			frameoffset += wi->weapAnimations[i].numFrames;
		}
	}

	// now set the correct frame into the part
	if ( anim->moveSpeed & ( 1 << partid ) ) {
		part->backlerp  = parent->backlerp;
		part->oldframe  = frameoffset + ( parent->oldframe - anim->firstFrame );
		part->frame     = frameoffset + ( parent->frame - anim->firstFrame );
	}

	return qtrue;
}


/*
===============
CG_SetWeapLerpFrameAnimation

may include ANIM_TOGGLEBIT
===============
*/
static void CG_SetWeapLerpFrameAnimation( weaponInfo_t *wi, lerpFrame_t *lf, int newAnimation ) {
	animation_t *anim;

	lf->animationNumber = newAnimation;
	newAnimation &= ~ANIM_TOGGLEBIT;

	if ( newAnimation < 0 || newAnimation >= MAX_WP_ANIMATIONS ) {
		CG_Error( "Bad animation number (CG_SWLFA): %i", newAnimation );
	}

	anim = &wi->weapAnimations[ newAnimation ];

	lf->animation       = anim;
	lf->animationTime   = lf->frameTime + anim->initialLerp;

	if ( cg_debugAnim.integer & 2 ) {
		CG_Printf( "Weap Anim: %d\n", newAnimation );
	}
}


/*
===============
CG_ClearWeapLerpFrame
===============
*/
void CG_ClearWeapLerpFrame( weaponInfo_t *wi, lerpFrame_t *lf, int animationNumber ) {
	lf->frameTime = lf->oldFrameTime = cg.time;
	CG_SetWeapLerpFrameAnimation( wi, lf, animationNumber );
	lf->oldFrame = lf->frame = lf->animation->firstFrame;

}


/*
===============
CG_RunWeapLerpFrame

Sets cg.snap, cg.oldFrame, and cg.backlerp
cg.time should be between oldFrameTime and frameTime after exit
===============
*/
static void CG_RunWeapLerpFrame( clientInfo_t *ci, weaponInfo_t *wi, lerpFrame_t *lf, int newAnimation, float speedScale ) {
	int f;
	animation_t *anim;

	// debugging tool to get no animations
	if ( cg_animSpeed.integer == 0 ) {
		lf->oldFrame = lf->frame = lf->backlerp = 0;
		return;
	}

	// see if the animation sequence is switching
	if ( !lf->animation ) {
		CG_ClearWeapLerpFrame( wi, lf, newAnimation );
	} else if ( newAnimation != lf->animationNumber )   {
		if ( ( newAnimation & ~ANIM_TOGGLEBIT ) == WEAP_RAISE ||
			 ( newAnimation & ~ANIM_TOGGLEBIT ) == WEAP_ALTSWITCHFROM ||
			 ( newAnimation & ~ANIM_TOGGLEBIT ) == WEAP_ALTSWITCHTO ) {
			CG_ClearWeapLerpFrame( wi, lf, newAnimation );   // clear when switching to raise (since it should be out of view anyway)
		} else {
			CG_SetWeapLerpFrameAnimation( wi, lf, newAnimation );
		}
	}

	// if we have passed the current frame, move it to
	// oldFrame and calculate a new frame
	if ( cg.time >= lf->frameTime ) {
		lf->oldFrame = lf->frame;
		lf->oldFrameTime = lf->frameTime;

		// get the next frame based on the animation
		anim = lf->animation;
		if ( !anim->frameLerp ) {
			return;     // shouldn't happen
		}
		if ( cg.time < lf->animationTime ) {
			lf->frameTime = lf->animationTime;      // initial lerp
		} else {
			lf->frameTime = lf->oldFrameTime + anim->frameLerp;
		}
		f = ( lf->frameTime - lf->animationTime ) / anim->frameLerp;
		f *= speedScale;        // adjust for haste, etc
		if ( f >= anim->numFrames ) {
			f -= anim->numFrames;
			if ( anim->loopFrames ) {
				f %= anim->loopFrames;
				f += anim->numFrames - anim->loopFrames;
			} else {
				f = anim->numFrames - 1;
				// the animation is stuck at the end, so it
				// can immediately transition to another sequence
				lf->frameTime = cg.time;
			}
		}
		lf->frame = anim->firstFrame + f;
		if ( cg.time > lf->frameTime ) {
			lf->frameTime = cg.time;
			if ( cg_debugAnim.integer ) {
//				CG_Printf( "Clamp lf->frameTime\n" );
			}
		}
	}

	if ( lf->frameTime > cg.time + 200 ) {
		lf->frameTime = cg.time;
	}

	if ( lf->oldFrameTime > cg.time ) {
		lf->oldFrameTime = cg.time;
	}
	// calculate current lerp value
	if ( lf->frameTime == lf->oldFrameTime ) {
		lf->backlerp = 0;
	} else {
		lf->backlerp = 1.0 - (float)( cg.time - lf->oldFrameTime ) / ( lf->frameTime - lf->oldFrameTime );
	}
}



/*
==============
CG_WeaponAnimation
==============
*/

//----(SA)	modified.  this is now client-side only (server does not dictate weapon animation info)
static void CG_WeaponAnimation( playerState_t *ps, weaponInfo_t *weapon, int *weapOld, int *weap, float *weapBackLerp ) {

	centity_t *cent = &cg.predictedPlayerEntity;
	clientInfo_t *ci = &cgs.clientinfo[ ps->clientNum ];

	if ( cg_noPlayerAnims.integer ) {
		*weapOld = *weap = 0;
		return;
	}

	CG_RunWeapLerpFrame( ci, weapon, &cent->pe.weap, ps->weapAnim, 1 );

	*weapOld        = cent->pe.weap.oldFrame;
	*weap           = cent->pe.weap.frame;
	*weapBackLerp   = cent->pe.weap.backlerp;

	if ( cg_debugAnim.integer == 3 ) {
		CG_Printf( "oldframe: %d   frame: %d   backlerp: %f\n", cent->pe.weap.oldFrame, cent->pe.weap.frame, cent->pe.weap.backlerp );
	}
}

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////


// (SA) it wasn't used anyway


/*
==============
CG_CalculateWeaponPosition
==============
*/
static void CG_CalculateWeaponPosition( vec3_t origin, vec3_t angles ) {
	float scale;
	int delta;
	float fracsin, leanscale;

	VectorCopy( cg.refdef.vieworg, origin );
	VectorCopy( cg.refdefViewAngles, angles );

	// adjust 'lean' into weapon
	if ( cg.predictedPlayerState.leanf != 0 ) {
		vec3_t right, up;

		leanscale = 1.0f;

		switch ( cg.predictedPlayerState.weapon ) {
		case WP_GARAND:
			leanscale = 3.0f;
			break;
		case WP_FLAMETHROWER:
		case WP_TESLA:
		case WP_MAUSER:
		case WP_DELISLE:
		case WP_M1941:
		case WP_M1GARAND:
		case WP_M7:
		case WP_HOLYCROSS:
			leanscale = 2.0f;
			break;

		// never adjust
		case WP_KNIFE:
		case WP_GRENADE_LAUNCHER:
		case WP_GRENADE_PINEAPPLE:
			break;

			// adjust when leaning right (in case of reload)
		default:
			if ( cg.predictedPlayerState.leanf > 0 ) {
				leanscale = 1.3f;
			}
			break;
		}

		// reverse the roll on the weapon so it stays relatively level
		angles[ROLL] -= cg.predictedPlayerState.leanf / ( leanscale * 2.0f );
		AngleVectors( angles, NULL, right, up );
		VectorMA( origin, angles[ROLL], right, origin );

		// pitch the gun down a bit to show that firing is not allowed when leaning
		angles[PITCH] += ( fabs( cg.predictedPlayerState.leanf ) / 2.0f );

		// this gives you some impression that the weapon stays in relatively the same
		// position while you lean, so you appear to 'peek' over the weapon
		AngleVectors( cg.refdefViewAngles, NULL, right, NULL );
		VectorMA( origin, -cg.predictedPlayerState.leanf / 4.0f, right, origin );
	}


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
		origin[2] += cg.landChange * 0.25 * delta / LAND_DEFLECT_TIME;
	} else if ( delta < LAND_DEFLECT_TIME + LAND_RETURN_TIME ) {
		origin[2] += cg.landChange * 0.25 *
					 ( LAND_DEFLECT_TIME + LAND_RETURN_TIME - delta ) / LAND_RETURN_TIME;
	}

#if 0
	// drop the weapon when stair climbing
	delta = cg.time - cg.stepTime;
	if ( delta < STEP_TIME / 2 ) {
		origin[2] -= cg.stepChange * 0.25 * delta / ( STEP_TIME / 2 );
	} else if ( delta < STEP_TIME ) {
		origin[2] -= cg.stepChange * 0.25 * ( STEP_TIME - delta ) / ( STEP_TIME / 2 );
	}
#endif

	// idle drift

	if (!cg_vanilla_guns.integer &&
		(cg.snap->ps.weaponstate == WEAPON_FIRING) &&
		(cg.predictedPlayerState.weapon == WP_FLAMETHROWER))
	{
		scale = 15; // apply reduced idle sway for flamethrower
	}
	else
	{
		scale = 80; // default sway
	}
	fracsin = sin( cg.time * 0.001 );
	angles[ROLL] += scale * fracsin * 0.01;
	angles[YAW] += scale * fracsin * 0.01;
	angles[PITCH] += scale * fracsin * 0.01;

	// RF, subtract the kickAngles
	VectorMA( angles, -1.0, cg.kickAngles, angles );

}


// Ridah
/*
===============
CG_FlamethrowerFlame
===============
*/
static void CG_FlamethrowerFlame( centity_t *cent, vec3_t origin ) {

	if ( cent->currentState.weapon != WP_FLAMETHROWER ) {
		return;
	}

	if ( cent->currentState.number == cg.snap->ps.clientNum ) {
		if ( cg.snap->ps.weapon != WP_FLAMETHROWER || ( cg.snap->ps.weaponstate != WEAPON_FIRING && cg.snap->ps.weaponstate != WEAPON_READY ) ) {
			return;
		}
	}

	CG_FireFlameChunks( cent, origin, cent->lerpAngles, 1.0, qtrue, 0 );

	return;
}
// done.

/*
==============
CG_TeslaSpinAngle
==============
*/
//#define TESLA_SPINSPEED .2
//#define TESLA_COASTTIME	2000
#define TESLA_SPINSPEED .05
#define TESLA_IDLESPEED .15
#define TESLA_COASTTIME 1000

static float CG_TeslaSpinAngle( centity_t *cent ) {
	int	delta;
	float	angle;

	delta = cg.time - cent->pe.barrelTime;

	angle = cent->pe.barrelAngle;

	if ( cent->currentState.eFlags & EF_FIRING ) {
		angle += delta * TESLA_SPINSPEED;
	} else {
		angle += delta * TESLA_IDLESPEED;
	}

	cent->pe.barrelAngle = AngleMod( angle );

	cent->pe.barrelTime = cg.time;

	return AngleMod( angle );
}

//----(SA)	added

/*
======================
CG_VenomSpinAngle
======================
*/

#define     VENOM_LOADTIME 2000
#define     VENOM_DELTATIME ( VENOM_LOADTIME / 10 )     // as there are 10 shots to be loaded

#define     SPIN_SPEED  1
#define     COAST_TIME  2000

static float CG_VenomSpinAngle( centity_t *cent ) {
	int delta;
	float angle;
	float speed;
	qboolean firing;

	firing = (qboolean)( cent->currentState.eFlags & EF_FIRING );

	if ( cg.snap->ps.weaponstate != WEAPON_FIRING ) { // (SA) this seems better
		firing = qfalse;
	}

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

	if ( cent->pe.barrelSpinning == !firing ) {
		cent->pe.barrelTime = cg.time;
		cent->pe.barrelAngle = AngleMod( angle );
		cent->pe.barrelSpinning = !!firing;

		// just switching between not spinning and spinning, play the appropriate weapon sound
		if ( cent->pe.barrelSpinning ) {
			if ( cg_weapons[WP_VENOM].spinupSound ) {
				trap_S_StartSound( NULL, cent->currentState.number, CHAN_WEAPON, cg_weapons[WP_VENOM].spinupSound );
			}
		} else {
			if ( cg_weapons[WP_VENOM].spindownSound ) {
				trap_S_StartSound( NULL, cent->currentState.number, CHAN_WEAPON, cg_weapons[WP_VENOM].spindownSound );
			}
		}

	}

	return angle;
}




/*
==============
CG_DrawRealWeapons
==============
*/
qboolean CG_DrawRealWeapons( centity_t *cent ) {

	switch ( cent->currentState.aiChar ) {
	case AICHAR_LOPER:
	case AICHAR_SUPERSOLDIER:       //----(SA)	added
	case AICHAR_SUPERSOLDIER_LAB:   
	case AICHAR_PROTOSOLDIER:
	case AICHAR_ZOMBIE:
	case AICHAR_ZOMBIE_SURV:
	case AICHAR_ZOMBIE_FLAME:
	case AICHAR_ZOMBIE_GHOST:
	case AICHAR_HELGA:      //----(SA)	added	// boss1 is now helga-blob
	case AICHAR_WARZOMBIE:
	case AICHAR_DOG:
	case AICHAR_PRIEST:
	case AICHAR_XSHEPHERD:
		return qfalse;
	}

	return qtrue;
}


/*
========================
CG_AddWeaponWithPowerups
========================
*/
static void CG_AddWeaponWithPowerups( refEntity_t *gun, int powerups, playerState_t *ps, centity_t *cent ) {
    // If ps is NULL, then:
    // - For the local client, use the predicted player state.
    // - For other entities (including AI), cast the entity state to a playerState_t.
    if ( !ps ) {
        if ( cent->currentState.number == cg.snap->ps.clientNum ) {
            ps = &cg.predictedPlayerState;
        } else {
            ps = (playerState_t *)&cent->currentState;
        }
    }
    
    // add powerup effects
    if ( powerups & ( 1 << PW_INVIS ) ) {
        gun->customShader = cgs.media.invisShader;
        trap_R_AddRefEntityToScene( gun );
    } else {
        trap_R_AddRefEntityToScene( gun );

        // blink if time left < 5s, toggling every 200ms for battlesuit
        if ( powerups & ( 1 << PW_BATTLESUIT_SURV ) ) {
            int timeLeft = ps->powerups[PW_BATTLESUIT_SURV] - cg.time;
            if ((timeLeft < 5000) && ((cg.time / 200) % 2)) {
                // skip rendering to blink
            } else {
                gun->customShader = cgs.media.battleWeaponShader;
                trap_R_AddRefEntityToScene( gun );
            }
        }

        // blink for quad powerup
        if ( powerups & ( 1 << PW_QUAD ) ) {
            int timeLeft = ps->powerups[PW_QUAD] - cg.time;
            if ((timeLeft < 5000) && ((cg.time / 200) % 2)) {
                // skip rendering to blink
            } else {
                gun->customShader = cgs.media.quadWeaponShader;
                trap_R_AddRefEntityToScene( gun );
            }
        }

        // blink for vampire powerup
        if ( powerups & ( 1 << PW_VAMPIRE ) ) {
            int timeLeft = ps->powerups[PW_VAMPIRE] - cg.time;
            if ((timeLeft < 5000) && ((cg.time / 200) % 2)) {
                // skip rendering to blink
            } else {
                gun->customShader = cgs.media.redQuadShader;
                trap_R_AddRefEntityToScene( gun );
            }
        }
    }
}

/*
==============
CG_PlayerCrossCoilFire

  TODO: this needs to be fixed for multiplay. entities being hurt need to be sent
  by server to all clients, so they draw the correct effects.
==============
*/
void CG_PlayerCrossCoilFire( centity_t *cent, vec3_t flashorigin ) {

#define	CROSS_LIGHTNING_POINT_TIMEOUT	3000
#define	CROSS_LIGHTNING_MAX_DIST	( cent->currentState.aiChar == AICHAR_SUPERSOLDIER ? TESLA_SUPERSOLDIER_RANGE : HOLYCROSS_RANGE )     // use these to perhaps vary the distance according to aiming
#define	CROSS_LIGHTNING_NORMAL_DIST	( HOLYCROSS_RANGE / 2.0 )
#define	CROSS_MAX_POINT_TESTS			10
#define	CROSS_MAX_POINT_TESTS_PERFRAME	20

	int i, j, pointTests = 0;
	vec3_t testPos, tagPos, vec;
	trace_t tr;
	float maxDist;
	int numPoints;
	vec3_t viewAngles, viewDir;
	int visEnemies[16];
	float visDists[16];
	int visEnemiesSorted[MAX_TESLA_BOLTS];
	int numEnemies, numSorted = 0, best;
	float bestDist;
	centity_t *ctrav;
	vec3_t traceOrg;
	int playerTeam;

	if ( cent->currentState.weapon != WP_HOLYCROSS ) {
		return;
	}
		VectorCopy( cent->lerpAngles, viewAngles );

	AngleVectors( viewAngles, viewDir, NULL, NULL );

	if ( cent->currentState.number == cg.snap->ps.clientNum ) {
		VectorCopy( cg.snap->ps.origin, traceOrg );
		playerTeam = cg.snap->ps.teamNum;
	} else {
		VectorCopy( cent->lerpOrigin, traceOrg );
		playerTeam = cent->currentState.teamNum;
	}

	maxDist = CROSS_LIGHTNING_MAX_DIST;
	numPoints = MAX_TESLA_BOLTS;

	VectorCopy( flashorigin, tagPos );

	// first, build a list of visible enemies that can be hurt by this tesla, then filter by distance
	if ( !cent->pe.teslaDamageApplyTime || cent->pe.teslaDamageApplyTime < cg.time - 200 ) {
		numEnemies = 0;
		// check the local playing client
		VectorSubtract( cg.snap->ps.origin, traceOrg, vec );
		VectorNormalize( vec );
		if ( ( cent != &cg_entities[cg.snap->ps.clientNum] ) &&
			 ( cg.snap->ps.teamNum != playerTeam ) &&
			 ( Distance( tagPos, cg.snap->ps.origin ) < CROSS_LIGHTNING_MAX_DIST ) &&
			 ( DotProduct( viewDir, vec ) > 0.8 ) ) {
			CG_Trace( &tr, traceOrg, NULL, NULL, cg.snap->ps.origin, cg.snap->ps.clientNum, MASK_SHOT & ~( CONTENTS_BODY ) );
			if ( tr.fraction == 1 || tr.entityNum == cg.snap->ps.clientNum ) {
				visDists[numEnemies] = Distance( tagPos, cg.snap->ps.origin );
				visEnemies[numEnemies++] = cg.snap->ps.clientNum;
			} else {    // try head
				VectorCopy( cg.snap->ps.origin, vec );
				vec[2] += cg.snap->ps.viewheight;
				CG_Trace( &tr, tagPos, NULL, NULL, vec, cg.snap->ps.clientNum, MASK_SHOT & ~( CONTENTS_BODY ) );
				if ( tr.fraction == 1 || tr.entityNum == cg.snap->ps.clientNum ) {
					visDists[numEnemies] = Distance( tagPos, cg.snap->ps.origin );
					visEnemies[numEnemies++] = cg.snap->ps.clientNum;
				} else {    // try body, from tag
					VectorCopy( cg.snap->ps.origin, vec );
					CG_Trace( &tr, tagPos, NULL, NULL, vec, cg.snap->ps.clientNum, MASK_SHOT & ~( CONTENTS_BODY ) );
					if ( tr.fraction == 1 || tr.entityNum == cg.snap->ps.clientNum ) {
						visDists[numEnemies] = Distance( tagPos, cg.snap->ps.origin );
						visEnemies[numEnemies++] = cg.snap->ps.clientNum;
					}
				}
			}
		}

		if ( cgs.localServer ) {
			// check for AI's getting hurt (TODO: bot support?)
			for ( ctrav = cg_entities, i = 0; i < cgs.maxclients && numEnemies < 16; ctrav++, i++ ) {
                                                                                                                     //
				if ( ctrav->currentState.aiChar &&
					 ( ctrav != cent ) &&
					 ( ctrav->currentState.teamNum != playerTeam ) &&
					 !( ctrav->currentState.eFlags & EF_DEAD ) &&
					 ctrav->currentValid && // is in the visible frame
					 ( Distance( tagPos, ctrav->lerpOrigin ) < CROSS_LIGHTNING_MAX_DIST ) ) {
					VectorSubtract( ctrav->lerpOrigin, traceOrg, vec );
					VectorNormalize( vec );

					if ( DotProduct( viewDir, vec ) > 0.8 ) {
						CG_Trace( &tr, traceOrg, NULL, NULL, ctrav->lerpOrigin, ctrav->currentState.number, MASK_SHOT & ~CONTENTS_BODY );
						if ( tr.fraction == 1 || tr.entityNum == ctrav->currentState.number ) {
							visDists[numEnemies] = Distance( tagPos, ctrav->lerpOrigin );
							visEnemies[numEnemies++] = ctrav->currentState.number;
						}
					}
				}
			}
		}

		// now sort by distance
		for ( j = 0; j < MAX_TESLA_BOLTS; j++ ) {
			visEnemiesSorted[j] = -1;

			bestDist = 99999;
			best = -1;
			for ( i = 0; i < numEnemies; i++ ) {
				if ( visEnemies[i] < 0 ) {
					continue;
				}
				if ( visDists[i] < bestDist ) {
					bestDist = visDists[i];
					visEnemiesSorted[j] = visEnemies[i];
					best = i;
				}
			}

			if ( best >= 0 ) {
				visEnemies[best] = -1;
				numSorted = j + 1;
			}
		}

		// now fill in the teslaEnemy[]'s
		for ( i = 0; i < MAX_TESLA_BOLTS; i++ ) {
			if ( numSorted && i / numSorted < 1 /*( MAX_TESLA_BOLTS / 3 )*/ ) {  // bolts per enemy
				j = i % numSorted;
				cent->pe.teslaEnemy[i] = visEnemiesSorted[j];
				// apply damage
				CG_ClientDamage( visEnemiesSorted[j], cent->currentState.number, CLDMG_HOLYCROSS );
				// show the effect
				cg_entities[ visEnemiesSorted[j] ].pe.teslaDamagedTime = cg.time;
			} else {
				if ( cent->pe.teslaEnemy[i] >= 0 ) {
					cent->pe.teslaEndPointTimes[i] = 0; // make sure we find a new spot
				}
				cent->pe.teslaEnemy[i] = -1;
			}
		}
		cent->pe.teslaDamageApplyTime = cg.time;
	}

	for ( i = 0; i < numPoints; i++ ) {

		//if (!(rand()%3))
		//	continue;

		VectorSubtract( cent->pe.teslaEndPoints[i], tagPos, vec );
		VectorNormalize( vec );

		// if this point has timed out, find a new spot
		if ( cent->pe.teslaEnemy[i] >= 0 ) {
			// attacking the player
			VectorSet( testPos, 6 * crandom(),
					   6 * crandom(),
					   20 * crandom() - 8 );
			//VectorClear( testPos );
			if ( cent->pe.teslaEnemy[i] != cg.snap->ps.clientNum ) {
				VectorAdd( testPos, cg_entities[cent->pe.teslaEnemy[i]].lerpOrigin, testPos );
			} else {
				VectorAdd( testPos, cg.snap->ps.origin, testPos );
			}
			cent->pe.teslaEndPointTimes[i] = cg.time; // - rand()%(TESLA_LIGHTNING_POINT_TIMEOUT/2);
			VectorCopy( testPos, cent->pe.teslaEndPoints[i] );
		} else if ( ( !cent->pe.teslaEndPointTimes[i] ) ||
					( cent->pe.teslaEndPointTimes[i] > cg.time ) ||
					( cent->pe.teslaEndPointTimes[i] < cg.time - CROSS_LIGHTNING_POINT_TIMEOUT ) ||
					( VectorDistance( tagPos, cent->pe.teslaEndPoints[i] ) > maxDist ) ||
					( DotProduct( viewDir, vec ) < 0.7 ) ) {

			//if (cent->currentState.groundEntityNum == ENTITYNUM_NONE)
			//	continue;	// must be on the ground

			// find a new spot
			for ( j = 0; j < CROSS_MAX_POINT_TESTS; j++ ) {
				VectorSet( testPos, cg.refdef.fov_y * crandom() * 0.5,
						   cg.refdef.fov_x * crandom() * 0.5,
						   0 );
				VectorAdd( viewAngles, testPos, testPos );
				AngleVectors( testPos, vec, NULL, NULL );
				VectorMA( tagPos, CROSS_LIGHTNING_NORMAL_DIST, vec, testPos );
				// try a trace to find a world collision
				CG_Trace( &tr, tagPos, NULL, NULL, testPos, cent->currentState.number, MASK_SHOT & ~CONTENTS_BODY );
				if ( tr.fraction < 1 && tr.entityNum == ENTITYNUM_WORLD && !( tr.surfaceFlags & ( SURF_NOIMPACT | SURF_SKY ) ) ) {
					// found a valid spot!
					cent->pe.teslaEndPointTimes[i] = cg.time - rand() % ( CROSS_LIGHTNING_POINT_TIMEOUT / 2 );
					VectorCopy( tr.endpos, cent->pe.teslaEndPoints[i] );
					break;
				}
				if ( pointTests++ > CROSS_MAX_POINT_TESTS_PERFRAME ) {
					j = CROSS_MAX_POINT_TESTS;
					continue;
				}
			}
			if ( j == CROSS_MAX_POINT_TESTS ) {
				continue;   // just don't draw this point
			}

			// add an impact mark on the wall
			VectorSubtract( cent->pe.teslaEndPoints[i], tagPos, vec );
			VectorNormalize( vec );
			VectorInverse( vec );
			//CG_ImpactMark( cgs.media.lightningHitWallShader, cent->pe.teslaEndPoints[i], vec, random() * 360, 0.2, 0.2, 0.2, 1.0, qtrue, 4, qfalse, 300 );
		}
		//
		// we have a valid lightning point, so draw it
		// sanity check though to make sure it's valid
		if ( VectorDistance( tagPos, cent->pe.teslaEndPoints[i] ) <= maxDist ) {

			CG_DynamicLightningBolt( cgs.media.lightningWaveShader, tagPos, cent->pe.teslaEndPoints[i], 1 + ( ( cg.time % ( ( i + 2 ) * ( i + 3 ) ) ) + i ) % 2, 20 + (float)( i % 3 ) * 5 + 6.0 * random(), ( cent->pe.teslaEnemy[i] < 0 ), 1.0, 0, i * i * 3 );

			// play a zap sound
			if ( cent->pe.lightningSoundTime < cg.time - 200 ) {
				CG_SoundPlayIndexedScript( cgs.media.crossZapScript, cent->pe.teslaEndPoints[i], ENTITYNUM_WORLD );
				CG_SoundPlayIndexedScript( cgs.media.crossZapScript, cent->lerpOrigin, ENTITYNUM_WORLD );
				//trap_S_StartSound( cent->pe.teslaEndPoints[i], ENTITYNUM_WORLD, CHAN_AUTO, cgs.media.lightningSounds[rand()%3] );
				cent->pe.lightningSoundTime = cg.time + rand() % 200;
			}
		}
	}

	//if ( cg.time % 3 ) {  // break it up a bit
		// add the looping sound
		//CG_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, cgs.media.teslaLoopSound, 255 );
	//}

	//if (cent->currentState.weapon == WP_TESLA && ( (cent->pe.weap.animationNumber & ~ANIM_TOGGLEBIT) == WEAP_IDLE1) || (cent->pe.weap.animationNumber & ~ANIM_TOGGLEBIT) == WEAP_IDLE2) {
	//CG_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, cgs.media.teslaLoopSound, 255 );
//}

	// drop a dynamic light out infront of us
	AngleVectors( viewAngles, vec, NULL, NULL );
	VectorMA( tagPos, 300, vec, testPos );
	// try a trace to find a world collision
	CG_Trace( &tr, tagPos, NULL, NULL, testPos, cent->currentState.number, MASK_SOLID );

	if ( ( cg.time / 50 ) % ( 4 + ( cg.time % 4 ) ) == 0 ) {
		// alt light
		trap_R_AddLightToScene( tr.endpos, 256 + 600 * tr.fraction, 0.2, 0, 0, 1 );
	} else if ( ( cg.time / 50 ) % ( 4 + ( cg.time % 4 ) ) == 1 ) {
		// no light
		//trap_R_AddLightToScene( tr.endpos, 128 + 500*tr.fraction, 1, 1, 1, 10 );
	} else {
		// blue light
		trap_R_AddLightToScene( tr.endpos, 256 + 600 * tr.fraction, 0.2, 0, 0, 1 );
	}

	// shake the camera a bit
	CG_StartShakeCamera( 0.05, 200, cent->lerpOrigin, 100 );
}

/*
==============
CG_PlayerTeslaCoilFire

  TODO: this needs to be fixed for multiplay. entities being hurt need to be sent
  by server to all clients, so they draw the correct effects.
==============
*/
void CG_PlayerTeslaCoilFire( centity_t *cent, vec3_t flashorigin ) {

#define	TESLA_LIGHTNING_POINT_TIMEOUT	3000
#define	TESLA_LIGHTNING_MAX_DIST	( cent->currentState.aiChar == AICHAR_SUPERSOLDIER ? TESLA_SUPERSOLDIER_RANGE : TESLA_RANGE )     // use these to perhaps vary the distance according to aiming
#define	TESLA_LIGHTNING_NORMAL_DIST	( TESLA_RANGE / 2.0 )
#define	TESLA_MAX_POINT_TESTS			10
#define	TESLA_MAX_POINT_TESTS_PERFRAME	20

	int i, j, pointTests = 0;
	vec3_t testPos, tagPos, vec;
	trace_t tr;
	float maxDist;
	int numPoints;
	vec3_t viewAngles, viewDir;
	int visEnemies[16];
	float visDists[16];
	int visEnemiesSorted[MAX_TESLA_BOLTS];
	int numEnemies, numSorted = 0, best;
	float bestDist;
	centity_t *ctrav;
	vec3_t traceOrg;
	int playerTeam;

	if ( cent->currentState.weapon != WP_TESLA ) {
		return;
	}

	//if (cent->currentState.number == cg.snap->ps.clientNum)
	//	VectorCopy( cg.snap->ps.viewangles, viewAngles );
	//else
		VectorCopy( cent->lerpAngles, viewAngles );

	AngleVectors( viewAngles, viewDir, NULL, NULL );

	if ( cent->currentState.number == cg.snap->ps.clientNum ) {
		VectorCopy( cg.snap->ps.origin, traceOrg );
		playerTeam = cg.snap->ps.teamNum;
	} else {
		VectorCopy( cent->lerpOrigin, traceOrg );
		playerTeam = cent->currentState.teamNum;
	}

	maxDist = TESLA_LIGHTNING_MAX_DIST;
	numPoints = MAX_TESLA_BOLTS;

	VectorCopy( flashorigin, tagPos );

	// first, build a list of visible enemies that can be hurt by this tesla, then filter by distance
	if ( !cent->pe.teslaDamageApplyTime || cent->pe.teslaDamageApplyTime < cg.time - 200 ) {
		numEnemies = 0;
		// check the local playing client
		VectorSubtract( cg.snap->ps.origin, traceOrg, vec );
		VectorNormalize( vec );
		if ( ( cent != &cg_entities[cg.snap->ps.clientNum] ) &&
			 ( cg.snap->ps.teamNum != playerTeam ) &&
			 ( Distance( tagPos, cg.snap->ps.origin ) < TESLA_LIGHTNING_MAX_DIST ) &&
			 ( DotProduct( viewDir, vec ) > 0.8 ) ) {
			CG_Trace( &tr, traceOrg, NULL, NULL, cg.snap->ps.origin, cg.snap->ps.clientNum, MASK_SHOT & ~( CONTENTS_BODY ) );
			if ( tr.fraction == 1 || tr.entityNum == cg.snap->ps.clientNum ) {
				visDists[numEnemies] = Distance( tagPos, cg.snap->ps.origin );
				visEnemies[numEnemies++] = cg.snap->ps.clientNum;
			} else {    // try head
				VectorCopy( cg.snap->ps.origin, vec );
				vec[2] += cg.snap->ps.viewheight;
				CG_Trace( &tr, tagPos, NULL, NULL, vec, cg.snap->ps.clientNum, MASK_SHOT & ~( CONTENTS_BODY ) );
				if ( tr.fraction == 1 || tr.entityNum == cg.snap->ps.clientNum ) {
					visDists[numEnemies] = Distance( tagPos, cg.snap->ps.origin );
					visEnemies[numEnemies++] = cg.snap->ps.clientNum;
				} else {    // try body, from tag
					VectorCopy( cg.snap->ps.origin, vec );
					CG_Trace( &tr, tagPos, NULL, NULL, vec, cg.snap->ps.clientNum, MASK_SHOT & ~( CONTENTS_BODY ) );
					if ( tr.fraction == 1 || tr.entityNum == cg.snap->ps.clientNum ) {
						visDists[numEnemies] = Distance( tagPos, cg.snap->ps.origin );
						visEnemies[numEnemies++] = cg.snap->ps.clientNum;
					}
				}
			}
		}

		if ( cgs.localServer ) {
			// check for AI's getting hurt (TODO: bot support?)
			for ( ctrav = cg_entities, i = 0; i < cgs.maxclients && numEnemies < 16; ctrav++, i++ ) {                                                                                                                        //
				if ( ctrav->currentState.aiChar &&
					 ( ctrav != cent ) &&
					 ( ctrav->currentState.teamNum != playerTeam ) &&
					 !( ctrav->currentState.eFlags & EF_DEAD ) &&
					 ctrav->currentValid && // is in the visible frame
					 ( Distance( tagPos, ctrav->lerpOrigin ) < TESLA_LIGHTNING_MAX_DIST ) ) {
					VectorSubtract( ctrav->lerpOrigin, traceOrg, vec );
					VectorNormalize( vec );

					if ( DotProduct( viewDir, vec ) > 0.8 ) {
						CG_Trace( &tr, traceOrg, NULL, NULL, ctrav->lerpOrigin, ctrav->currentState.number, MASK_SHOT & ~CONTENTS_BODY );
						if ( tr.fraction == 1 || tr.entityNum == ctrav->currentState.number ) {
							visDists[numEnemies] = Distance( tagPos, ctrav->lerpOrigin );
							visEnemies[numEnemies++] = ctrav->currentState.number;
						}
					}
				}
			}
		}

		// now sort by distance
		for ( j = 0; j < MAX_TESLA_BOLTS; j++ ) {
			visEnemiesSorted[j] = -1;

			bestDist = 99999;
			best = -1;
			for ( i = 0; i < numEnemies; i++ ) {
				if ( visEnemies[i] < 0 ) {
					continue;
				}
				if ( visDists[i] < bestDist ) {
					bestDist = visDists[i];
					visEnemiesSorted[j] = visEnemies[i];
					best = i;
				}
			}

			if ( best >= 0 ) {
				visEnemies[best] = -1;
				numSorted = j + 1;
			}
		}

		// now fill in the teslaEnemy[]'s
		for ( i = 0; i < MAX_TESLA_BOLTS; i++ ) {
			if ( numSorted && i / numSorted < 1 /*( MAX_TESLA_BOLTS / 3 )*/ ) {  // bolts per enemy
				j = i % numSorted;
				cent->pe.teslaEnemy[i] = visEnemiesSorted[j];
				// apply damage
				CG_ClientDamage( visEnemiesSorted[j], cent->currentState.number, CLDMG_TESLA );
				// show the effect
				cg_entities[ visEnemiesSorted[j] ].pe.teslaDamagedTime = cg.time;
			} else {
				if ( cent->pe.teslaEnemy[i] >= 0 ) {
					cent->pe.teslaEndPointTimes[i] = 0; // make sure we find a new spot
				}
				cent->pe.teslaEnemy[i] = -1;
			}
		}
		cent->pe.teslaDamageApplyTime = cg.time;
	}

	for ( i = 0; i < numPoints; i++ ) {

		//if (!(rand()%3))
		//	continue;

		VectorSubtract( cent->pe.teslaEndPoints[i], tagPos, vec );
		VectorNormalize( vec );

		// if this point has timed out, find a new spot
		if ( cent->pe.teslaEnemy[i] >= 0 ) {
			// attacking the player
			VectorSet( testPos, 6 * crandom(),
					   6 * crandom(),
					   20 * crandom() - 8 );
			//VectorClear( testPos );
			if ( cent->pe.teslaEnemy[i] != cg.snap->ps.clientNum ) {
				VectorAdd( testPos, cg_entities[cent->pe.teslaEnemy[i]].lerpOrigin, testPos );
			} else {
				VectorAdd( testPos, cg.snap->ps.origin, testPos );
			}
			cent->pe.teslaEndPointTimes[i] = cg.time; // - rand()%(TESLA_LIGHTNING_POINT_TIMEOUT/2);
			VectorCopy( testPos, cent->pe.teslaEndPoints[i] );
		} else if ( ( !cent->pe.teslaEndPointTimes[i] ) ||
					( cent->pe.teslaEndPointTimes[i] > cg.time ) ||
					( cent->pe.teslaEndPointTimes[i] < cg.time - TESLA_LIGHTNING_POINT_TIMEOUT ) ||
					( VectorDistance( tagPos, cent->pe.teslaEndPoints[i] ) > maxDist ) ||
					( DotProduct( viewDir, vec ) < 0.7 ) ) {

			//if (cent->currentState.groundEntityNum == ENTITYNUM_NONE)
			//	continue;	// must be on the ground

			// find a new spot
			for ( j = 0; j < TESLA_MAX_POINT_TESTS; j++ ) {
				VectorSet( testPos, cg.refdef.fov_y * crandom() * 0.5,
						   cg.refdef.fov_x * crandom() * 0.5,
						   0 );
				VectorAdd( viewAngles, testPos, testPos );
				AngleVectors( testPos, vec, NULL, NULL );
				VectorMA( tagPos, TESLA_LIGHTNING_NORMAL_DIST, vec, testPos );
				// try a trace to find a world collision
				CG_Trace( &tr, tagPos, NULL, NULL, testPos, cent->currentState.number, MASK_SHOT & ~CONTENTS_BODY );
				if ( tr.fraction < 1 && tr.entityNum == ENTITYNUM_WORLD && !( tr.surfaceFlags & ( SURF_NOIMPACT | SURF_SKY ) ) ) {
					// found a valid spot!
					cent->pe.teslaEndPointTimes[i] = cg.time - rand() % ( TESLA_LIGHTNING_POINT_TIMEOUT / 2 );
					VectorCopy( tr.endpos, cent->pe.teslaEndPoints[i] );
					break;
				}
				if ( pointTests++ > TESLA_MAX_POINT_TESTS_PERFRAME ) {
					j = TESLA_MAX_POINT_TESTS;
					continue;
				}
			}
			if ( j == TESLA_MAX_POINT_TESTS ) {
				continue;   // just don't draw this point
			}

			// add an impact mark on the wall
			VectorSubtract( cent->pe.teslaEndPoints[i], tagPos, vec );
			VectorNormalize( vec );
			VectorInverse( vec );
			CG_ImpactMark( cgs.media.lightningHitWallShader, cent->pe.teslaEndPoints[i], vec, random() * 360, 0.2, 0.2, 0.2, 1.0, qtrue, 4, qfalse, 300 );
		}
		//
		// we have a valid lightning point, so draw it
		// sanity check though to make sure it's valid
		if ( VectorDistance( tagPos, cent->pe.teslaEndPoints[i] ) <= maxDist ) {
			CG_DynamicLightningBolt( cgs.media.lightningBoltShader, tagPos, cent->pe.teslaEndPoints[i], 1 + ( ( cg.time % ( ( i + 2 ) * ( i + 3 ) ) ) + i ) % 2, 20 + (float)( i % 3 ) * 5 + 6.0 * random(), ( cent->pe.teslaEnemy[i] < 0 ), 1.0, 0, i * i * 3 );

			// play a zap sound
			if ( cent->pe.lightningSoundTime < cg.time - 200 ) {
				CG_SoundPlayIndexedScript( cgs.media.teslaZapScript, cent->pe.teslaEndPoints[i], ENTITYNUM_WORLD );
				CG_SoundPlayIndexedScript( cgs.media.teslaZapScript, cent->lerpOrigin, ENTITYNUM_WORLD );
				//trap_S_StartSound( cent->pe.teslaEndPoints[i], ENTITYNUM_WORLD, CHAN_AUTO, cgs.media.lightningSounds[rand()%3] );
				cent->pe.lightningSoundTime = cg.time + rand() % 200;
			}
		}
	}

	//if ( cg.time % 3 ) {  // break it up a bit
		// add the looping sound
		//CG_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, cgs.media.teslaLoopSound, 255 );
	//}

	//if (cent->currentState.weapon == WP_TESLA && ( (cent->pe.weap.animationNumber & ~ANIM_TOGGLEBIT) == WEAP_IDLE1) || (cent->pe.weap.animationNumber & ~ANIM_TOGGLEBIT) == WEAP_IDLE2) {
	//CG_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, cgs.media.teslaLoopSound, 255 );
//}

	// drop a dynamic light out infront of us
	AngleVectors( viewAngles, vec, NULL, NULL );
	VectorMA( tagPos, 300, vec, testPos );
	// try a trace to find a world collision
	CG_Trace( &tr, tagPos, NULL, NULL, testPos, cent->currentState.number, MASK_SOLID );

	if ( ( cg.time / 50 ) % ( 4 + ( cg.time % 4 ) ) == 0 ) {
		// alt light
		if ( cgs.gametype == GT_GOTHIC ) {
		trap_R_AddLightToScene( tr.endpos, 256 + 600 * tr.fraction, 0.2, 0, 0, 1 );
		} else {
		trap_R_AddLightToScene( tr.endpos, 256 + 600 * tr.fraction, 0.2, 0.6, 1, 1 );
		}
	} else if ( ( cg.time / 50 ) % ( 4 + ( cg.time % 4 ) ) == 1 ) {
		// no light
		//trap_R_AddLightToScene( tr.endpos, 128 + 500*tr.fraction, 1, 1, 1, 10 );
	} else {
		// blue light
		if ( cgs.gametype == GT_GOTHIC ) {
		trap_R_AddLightToScene( tr.endpos, 256 + 600 * tr.fraction, 0.2, 0, 0, 1 );
		} else {
		trap_R_AddLightToScene( tr.endpos, 256 + 600 * tr.fraction, 0.2, 0.6, 1, 0 );
		}
	}

	// shake the camera a bit
	CG_StartShakeCamera( 0.05, 200, cent->lerpOrigin, 100 );
}


// Ridah
/*
==============
CG_MonsterUsingWeapon
==============
*/
qboolean CG_MonsterUsingWeapon( centity_t *cent, int aiChar, int weaponNum ) {
	return ( cent->currentState.aiChar == aiChar ) && ( cent->currentState.weapon == weaponNum );
}

/*
==============
CG_WeaponIsUpgraded
==============
*/
qboolean CG_WeaponIsUpgraded(weapon_t weaponNum) {
	if (cg.snap->ps.clientNum != cg.clientNum) {
		return qfalse;
	}
	return (cg.snap->ps.weaponUpgraded[weaponNum] != 0);
}

/*
=============
CG_AddPlayerWeapon

Used for both the view weapon (ps is valid) and the world modelother character models (ps is NULL)
The main player will have this called for BOTH cases, so effects like light and
sound should only be done on the world model case.
=============
*/
static qboolean debuggingweapon = qfalse;

void CG_AddPlayerWeapon( refEntity_t *parent, playerState_t *ps, centity_t *cent ) {

	refEntity_t gun;
	refEntity_t barrel;
	refEntity_t flash;
	vec3_t angles;
	weapon_t weaponNum, weapSelect;
	weaponInfo_t    *weapon;
	centity_t   *nonPredictedCent;
	qboolean firing;    // Ridah

	qboolean akimboFire_colt = qfalse;       //----(SA)	added
    qboolean akimboFire_tt33 = qfalse;       

	qboolean playerScaled;
	qboolean drawpart, drawrealweap;
	int i;
	qboolean isPlayer;

	// (SA) might as well have this check consistant throughout the routine
	isPlayer = (qboolean)( cent->currentState.clientNum == cg.snap->ps.clientNum );

	weaponNum = cent->currentState.weapon;
	weapSelect = cg.weaponSelect;

	if ( ps && cg.cameraMode ) {
		return;
	}

	// don't draw any weapons when the binocs are up
	if ( cent->currentState.eFlags & EF_ZOOMING ) {
		if ( isPlayer ) {
			if ( !cg.renderingThirdPerson ) {
				return;
			}
		} else {
			return;
		}
	}

	// don't draw weapon stuff when looking through a scope
	if ( weaponNum == WP_SNOOPERSCOPE || weaponNum == WP_SNIPERRIFLE || weaponNum == WP_FG42SCOPE || weaponNum == WP_DELISLESCOPE || weaponNum == WP_M1941SCOPE ||
		 weapSelect == WP_SNOOPERSCOPE || weapSelect == WP_SNIPERRIFLE || weapSelect == WP_FG42SCOPE || weapSelect == WP_DELISLESCOPE || weapSelect == WP_M1941SCOPE ) {
		if ( isPlayer && !cg.renderingThirdPerson ) {
			return;
		}
	}

	// no weapon when on mg_42
	if ( cent->currentState.eFlags & EF_MG42_ACTIVE ) {
		return;
	}


		//CG_RegisterWeapon( weaponNum, qfalse );
		weapon = &cg_weapons[weaponNum];


	if ( isPlayer ) {
		akimboFire_colt = BG_AkimboFireSequence( weaponNum, cg.predictedPlayerState.ammoclip[WP_AKIMBO], cg.predictedPlayerState.ammoclip[WP_COLT] );
        akimboFire_tt33 = BG_AkimboFireSequence( weaponNum, cg.predictedPlayerState.ammoclip[WP_DUAL_TT33], cg.predictedPlayerState.ammoclip[WP_TT33] );
	} else if ( ps ) {
		akimboFire_colt = BG_AkimboFireSequence( weaponNum, ps->ammoclip[WP_AKIMBO], ps->ammoclip[WP_AKIMBO] );
        akimboFire_tt33 = BG_AkimboFireSequence( weaponNum, ps->ammoclip[WP_DUAL_TT33], ps->ammoclip[WP_DUAL_TT33] );
	}

	// add the weapon
	memset( &gun, 0, sizeof( gun ) );
	VectorCopy( parent->lightingOrigin, gun.lightingOrigin );
	gun.shadowPlane = parent->shadowPlane;
	gun.renderfx = parent->renderfx;

	// set custom shading for railgun refire rate
	if ( ps ) {
		gun.shaderRGBA[0] = 255;
		gun.shaderRGBA[1] = 255;
		gun.shaderRGBA[2] = 255;
		gun.shaderRGBA[3] = 255;
	}

	if (ps)
	{
		gun.hModel = weapon->weaponModel[W_FP_MODEL].model;
	}
	else
	{
		gun.hModel = weapon->weaponModel[W_TP_MODEL].model;
	}

	if ( !gun.hModel ) {
		if ( debuggingweapon ) {
			CG_Printf( "returning due to: !gun.hModel\n" );
		}
		return;
	}

	if ( !ps && cg.snap->ps.pm_flags & PMF_LADDER && isPlayer ) {      //----(SA) player on ladder
		if ( debuggingweapon ) {
			CG_Printf( "returning due to: !ps && cg.snap->ps.pm_flags & PMF_LADDER\n" );
		}
		return;
	}

	if ( !ps ) {
		// add weapon ready sound
		cent->pe.lightningFiring = qfalse;
		if ( ( cent->currentState.eFlags & EF_FIRING ) && weapon->firingSound ) {
			// lightning gun and guantlet make a different sound when fire is held down
			CG_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, weapon->firingSound, 255 );
			cent->pe.lightningFiring = qtrue;
		} else if ( weapon->readySound ) {
			CG_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, weapon->readySound, 255 );
		}
	}
	// RealRTCW
	if (isPlayer) {
	if ( cent->currentState.weapon == WP_TESLA && ( ( ( cent->pe.weap.animationNumber & ~ANIM_TOGGLEBIT ) == WEAP_IDLE1 ) || ( ( cent->pe.weap.animationNumber & ~ANIM_TOGGLEBIT ) == WEAP_IDLE2 ) ) ) {
		CG_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, cgs.media.teslaLoopSound, 100 );
	}
	}

	// Ridah
	firing = ( ( cent->currentState.eFlags & EF_FIRING ) != 0 );

	// Prevent firing flamethrower and tesla in noclip or when leaning or when underwater
	if ( ps && firing && (weaponNum == WP_TESLA || weaponNum == WP_FLAMETHROWER) )
	{
		vec3_t point;
		int cont = 0;
		VectorCopy (ps->origin, point);	// crashes here
		point[2] += ps->viewheight;
		cont = trap_CM_PointContents( point, 0 );

		// player is in noclip mode - no fire
		if ( ps->pm_type == PM_NOCLIP )
			firing = qfalse;
		// player is leaning - no fire
		if ( ps->leanf != 0 )
			firing = qfalse;
		// player is underwater - no fire
		if ( cont & CONTENTS_WATER )
			firing = qfalse;
	}

	CG_PositionEntityOnTag( &gun, parent, "tag_weapon", 0, NULL );

	playerScaled = (qboolean)( cgs.clientinfo[ cent->currentState.clientNum ].playermodelScale[0] != 0 );
	if ( !ps && playerScaled ) {   // don't "un-scale" weap up in 1st person
		for ( i = 0; i < 3; i++ ) {  // scale weapon back up so it doesn't pick up the adjusted scale of the character models.
									 // this will affect any parts attached to the gun as well (barrel/bolt/flash/brass/etc.)
			VectorScale( gun.axis[i], 1.0 / ( cgs.clientinfo[ cent->currentState.clientNum ].playermodelScale[i] ), gun.axis[i] );
		}
	}

	// characters that draw their own special weapon model will not draw the standard ones
	if ( CG_DrawRealWeapons( cent ) ) {
		drawrealweap = qtrue;
	} else {
		drawrealweap = qfalse;
	}

	if ( ps ) {
		drawpart = CG_GetPartFramesFromWeap( cent, &gun, parent, W_MAX_PARTS, weapon );   // W_MAX_PARTS specifies this as the primary view model
	} else {
		drawpart = qtrue;
	}

	if (drawpart && drawrealweap)
	{
		if (isPlayer)
		{
			if (CG_WeaponIsUpgraded(weaponNum))
			{
				if (weapon->upgradedMapSkin)
				{
					gun.customSkin = weapon->upgradedMapSkin;
				}
				else if (weapon->upgradedSkin)
				{
					gun.customSkin = weapon->upgradedSkin;
				}
			}
			else if (weapon->handsSkin)
			{
				gun.customSkin = weapon->handsSkin;
			}
		}
		CG_AddWeaponWithPowerups(&gun, cent->currentState.powerups, ps, cent);
	}

	if ( isPlayer && ps != NULL ) {
		refEntity_t brass;

		// opposite tag in akimbo, since at this point the weapon
		// has fired and the fire seq has switched over
		if ( (weaponNum == WP_AKIMBO  && akimboFire_colt) || (weaponNum == WP_DUAL_TT33  && akimboFire_tt33)) {
			CG_PositionRotatedEntityOnTag( &brass, &gun, "tag_brass2" );
		} else {
			CG_PositionRotatedEntityOnTag( &brass, &gun, "tag_brass" );
		}

		VectorCopy( brass.origin, ejectBrassCasingOrigin );
	}

	memset( &barrel, 0, sizeof( barrel ) );
	VectorCopy( parent->lightingOrigin, barrel.lightingOrigin );
	barrel.shadowPlane = parent->shadowPlane;
	barrel.renderfx = parent->renderfx;

	// add barrels
	// attach generic weapon parts to the first person weapon.
	// if a barrel should be attached for third person, add it in the (!ps) section below
	angles[YAW] = angles[PITCH] = 0;

	if ( ps ) {
		qboolean spunpart;

		for ( i = W_PART_1; i < W_MAX_PARTS; i++ ) {

			spunpart = qfalse;
			barrel.hModel = weapon->partModels[W_FP_MODEL][i].model;

			if (isPlayer)
			{
				if (CG_WeaponIsUpgraded(weaponNum))
				{
					if (weapon->upgradedMapSkin)
					{
						barrel.customSkin = weapon->upgradedMapSkin;
					}
					else if (weapon->upgradedSkin)
					{
						barrel.customSkin = weapon->upgradedSkin;
					}
				}
				else if (weapon->handsSkin)
				{
					barrel.customSkin = weapon->handsSkin;
				}
			}

			// check for spinning
			if ( weaponNum == WP_VENOM ) {
				if ( i == W_PART_1 ) {
					angles[ROLL] = CG_VenomSpinAngle( cent );
					spunpart = qtrue;
				} else if ( i == W_PART_2 )    {
					angles[ROLL] = -CG_VenomSpinAngle( cent );
					spunpart = qtrue;
				}
				// 'blurry' barel when firing
				// (SA) not right now.  at the moment, just spin the belt when firing, no swapout
				else if ( i == W_PART_3 ) {
					if ( ( cent->pe.weap.animationNumber & ~ANIM_TOGGLEBIT ) == WEAP_ATTACK1 ) {
						//barrel.hModel = weapon->wpPartModels_old[W_FP_MODEL][i];
						barrel.hModel = weapon->partModels[W_FP_MODEL][i].model;
						angles[ROLL] = -CG_VenomSpinAngle( cent );
						angles[ROLL] = -( angles[ROLL] / 8.0f );
					} else {
						angles[ROLL] = 0;
					}
					spunpart = qtrue;
				}
			} else if ( (weaponNum == WP_TESLA) || (weaponNum == WP_HOLYCROSS) )  {
				if ( i == W_PART_1 || i == W_PART_2 ) {
					angles[ROLL] = CG_TeslaSpinAngle( cent );
					spunpart = qtrue;
				}
			}

			if ( spunpart ) {
				AnglesToAxis( angles, barrel.axis );
			}
			// end spinning


			if ( barrel.hModel ) {
				if ( i == W_PART_1 ) {
					if ( spunpart ) {
						CG_PositionRotatedEntityOnTag( &barrel, parent, "tag_barrel" );
					} else { CG_PositionEntityOnTag( &barrel, parent, "tag_barrel", 0, NULL );}
				} else {
					if ( spunpart ) {
						CG_PositionRotatedEntityOnTag( &barrel, parent, va( "tag_barrel%d", i + 1 ) );
					} else { CG_PositionEntityOnTag( &barrel, parent, va( "tag_barrel%d", i + 1 ), 0, NULL );}
				}

				drawpart = CG_GetPartFramesFromWeap( cent, &barrel, parent, i, weapon );

				if ( drawpart && drawrealweap ) {
					CG_AddWeaponWithPowerups( &barrel, cent->currentState.powerups, ps, cent );
				}
			}
		}
	} else {    // weapons with barrels drawn in third person
		if ( drawrealweap ) {
			if ( weaponNum == WP_VENOM ) {

				angles[ROLL] = CG_VenomSpinAngle( cent );
				AnglesToAxis( angles, barrel.axis );

				//barrel.hModel = weapon->wpPartModels_old[W_TP_MODEL][W_PART_1];
				barrel.hModel = weapon->partModels[W_TP_MODEL][W_PART_1].model;
				CG_PositionRotatedEntityOnTag( &barrel, &gun, "tag_barrel" );
				CG_AddWeaponWithPowerups( &barrel, cent->currentState.powerups, ps, cent );
			}
		}
	}

	// add the scope model to the rifle if you've got it
	if ( isPlayer && !cg.renderingThirdPerson ) {      // (SA) for now just do it on the first person weapons
		if ( weaponNum == WP_MAUSER ) {
			if ( COM_BitCheck( cg.predictedPlayerState.weapons, WP_SNIPERRIFLE ) ) {
				barrel.hModel = weapon->modModels[0];
				if ( barrel.hModel ) {
					CG_PositionEntityOnTag(&barrel, parent, "tag_scope", 0, NULL);
					CG_AddWeaponWithPowerups( &barrel, cent->currentState.powerups, ps, cent );
				}
			}
		}
	}

		if ( isPlayer && !cg.renderingThirdPerson ) {      // (SA) for now just do it on the first person weapons
		if ( weaponNum == WP_DELISLE ) {
			if ( COM_BitCheck( cg.predictedPlayerState.weapons, WP_DELISLESCOPE ) ) {
				barrel.hModel = weapon->modModels[0];
				if ( barrel.hModel ) {
					CG_PositionEntityOnTag(&barrel, parent, "tag_scope", 0, NULL);
					CG_AddWeaponWithPowerups( &barrel, cent->currentState.powerups, ps, cent );
				}
			}
		}
	}

	
		if ( isPlayer && !cg.renderingThirdPerson ) {      // (SA) for now just do it on the first person weapons
		if ( weaponNum == WP_M1941) {
			if ( COM_BitCheck( cg.predictedPlayerState.weapons, WP_M1941SCOPE ) ) {
				barrel.hModel = weapon->modModels[0];
				if ( barrel.hModel ) {
					CG_PositionEntityOnTag(&barrel, parent, "tag_scope", 0, NULL);
					CG_AddWeaponWithPowerups( &barrel, cent->currentState.powerups, ps, cent );
				}
			}
		}
	}

		if ( isPlayer && !cg.renderingThirdPerson ) {        // (SA) for now just do it on the first person weapons
		if ( weaponNum == WP_M1GARAND || weaponNum == WP_M7 ) {
			if ( (  cg.snap->ps.ammo[BG_FindAmmoForWeapon( WP_M7 )] || cg.snap->ps.ammoclip[BG_FindAmmoForWeapon( WP_M7 )]  ) ) {
					barrel.hModel = weapon->modModels[0];
					if ( barrel.hModel ) {
						CG_PositionEntityOnTag( &barrel, parent, "tag_scope", 0, NULL );
						CG_AddWeaponWithPowerups( &barrel, cent->currentState.powerups, ps, cent );
					}
			}
		}
		}

	// make sure we aren't looking at cg.predictedPlayerEntity for LG
	nonPredictedCent = &cg_entities[cent->currentState.number];

	// if the index of the nonPredictedCent is not the same as the clientNum
	// then this is a fake player (like on the single player podiums), so
	// go ahead and use the cent
	if ( ( nonPredictedCent - cg_entities ) != cent->currentState.clientNum ) {
		nonPredictedCent = cent;
	}


	// add the flash
	memset( &flash, 0, sizeof( flash ) );
	VectorCopy( parent->lightingOrigin, flash.lightingOrigin );
	flash.shadowPlane = parent->shadowPlane;
	flash.renderfx = parent->renderfx;

	if ( ps ) {
		flash.hModel = weapon->flashModel[W_FP_MODEL];
	} else {
		flash.hModel = weapon->flashModel[W_TP_MODEL];
	}

	angles[YAW]     = 0;
	angles[PITCH]   = 0;
	angles[ROLL]    = crandom() * 10;
	AnglesToAxis( angles, flash.axis );

	if ( weaponNum == WP_AKIMBO || weaponNum == WP_DUAL_TT33 )
	{
		if (!ps || cg.renderingThirdPerson)
		{
			if (!cent->akimboFire)
			{
				CG_PositionRotatedEntityOnTag(&flash, parent, "tag_weapon");
				VectorMA(flash.origin, 10, flash.axis[0], flash.origin);
			}
			else
			{
				CG_PositionRotatedEntityOnTag(&flash, &gun, "tag_flash");
			}
		}
		else
		{
			if (!cent->akimboFire)
			{
				CG_PositionRotatedEntityOnTag(&flash, parent, "tag_flash2");

			}
			else
			{
				CG_PositionRotatedEntityOnTag(&flash, parent, "tag_flash");
			}
		}
	}
	else
	{
		CG_PositionRotatedEntityOnTag(&flash, &gun, "tag_flash");
	}

	// store this position for other cgame elements to access
	cent->pe.gunRefEnt = gun;
	cent->pe.gunRefEntFrame = cg.clientFrame;

	if ( ( weaponNum == WP_FLAMETHROWER || weaponNum == WP_TESLA || weaponNum == WP_HOLYCROSS ) && ( nonPredictedCent->currentState.eFlags & EF_FIRING ) ) {
		// continuous flash

	} else {

		// continuous smoke after firing
#define BARREL_SMOKE_TIME 1000

		if ( ps || cg.renderingThirdPerson || !isPlayer ) {
			if ( weaponNum == WP_VENOM || weaponNum == WP_STEN || weaponNum == WP_MG42M || weaponNum == WP_BROWNING ) {
				if ( !cg_paused.integer ) {    // don't add while paused
					// hot smoking gun
					if ( cg.time - cent->overheatTime < 3000 ) {
						if ( !( rand() % 3 ) ) {
							float alpha;
							alpha = 1.0f - ( (float)( cg.time - cent->overheatTime ) / 3000.0f );
							alpha *= 0.25f;     // .25 max alpha
							if ( weaponNum == WP_VENOM ) { // silly thing that makes the smoke off the venom swirlier since it's spinning real fast
								CG_ParticleImpactSmokePuffExtended( cgs.media.smokeParticleShader, flash.origin, tv( 0,0,1 ), 8, 1000, 8, 20, 70, alpha );
							} else {
								CG_ParticleImpactSmokePuffExtended( cgs.media.smokeParticleShader, flash.origin, tv( 0,0,1 ), 8, 1000, 8, 20, 30, alpha );
							}
						}
					}
				}

			} else if ( weaponNum == WP_PANZERFAUST ) {
				if ( !cg_paused.integer ) {    // don't add while paused
					if ( cg.time - cent->muzzleFlashTime < BARREL_SMOKE_TIME ) {
						if ( !( rand() % 5 ) ) {
							float alpha;
							alpha = 1.0f - ( (float)( cg.time - cent->muzzleFlashTime ) / (float)BARREL_SMOKE_TIME ); // what fraction of BARREL_SMOKE_TIME are we at
							alpha *= 0.25f;     // .25 max alpha
							CG_ParticleImpactSmokePuffExtended( cgs.media.smokeParticleShader, flash.origin, tv( 0,0,1 ), 8, 1000, 8, 20, 30, alpha );
						}
					}
				}
			}
		}

		// impulse flash
		if ( cg.time - cent->muzzleFlashTime > MUZZLE_FLASH_TIME ) {
			// Ridah, blue ignition flame if not firing flamer
			if ( weaponNum != WP_FLAMETHROWER && weaponNum != WP_TESLA && weaponNum != WP_HOLYCROSS ) {
				return;
			}
		}

	}

	// weapons that don't need to go any further as they have no flash or light
	if ( weaponNum == WP_GRENADE_LAUNCHER ||
		 weaponNum == WP_GRENADE_PINEAPPLE ||
		 weaponNum == WP_KNIFE ||
		 weaponNum == WP_DYNAMITE ||
		 weaponNum == WP_DYNAMITE_ENG ||
		 weaponNum == WP_M7 ) {
		return;
	}

	if ( weaponNum == WP_STEN ) {  // sten has no muzzleflash
		flash.hModel = 0;
	}

	// weaps with barrel smoke
	if ( ps || cg.renderingThirdPerson || !isPlayer ) {
		if ( !cg_paused.integer ) {    // don't add while paused
			if ( weaponNum == WP_STEN || weaponNum == WP_VENOM ) {
				if ( cg.time - cent->muzzleFlashTime < 100 ) {
//					CG_ParticleImpactSmokePuff (cgs.media.smokeParticleShader, flash.origin);
					CG_ParticleImpactSmokePuffExtended( cgs.media.smokeParticleShader, flash.origin, tv( 0,0,1 ), 8, 500, 8, 20, 30, 0.25f );
				}
			}
		}
	}


	if ( flash.hModel ) {
		if ( weaponNum != WP_FLAMETHROWER && weaponNum != WP_TESLA && weaponNum != WP_HOLYCROSS ) {    //Ridah, hide the flash also for now
			// RF, changed this so the muzzle flash stays onscreen for long enough to be seen
			if ( cg.time - cent->muzzleFlashTime < MUZZLE_FLASH_TIME ) {
//				if ( firing )	// Ridah
					trap_R_AddRefEntityToScene( &flash );
			}
		}
	}

	// Ridah, zombie fires from his head
	if (CG_MonsterUsingWeapon( cent, AICHAR_PRIEST, WP_TESLA )) {
		CG_PositionEntityOnTag( &flash, parent,"tag_weapon", 0, NULL);
	}

	if (CG_MonsterUsingWeapon( cent, AICHAR_XSHEPHERD, WP_VENOM )) {
		CG_PositionEntityOnTag( &flash, parent,"tag_weapon", 0, NULL);
	}

	if ( ps || cg.renderingThirdPerson || !isPlayer ) {

		if ( firing ) {
			// Ridah, Flamethrower effect
			CG_FlamethrowerFlame( cent, flash.origin );

			// RF, Tesla coil
			CG_PlayerTeslaCoilFire( cent, flash.origin );

			CG_PlayerCrossCoilFire( cent, flash.origin );

			// make a dlight for the flash
			if ( weapon->flashDlightColor[0] || weapon->flashDlightColor[1] || weapon->flashDlightColor[2] ) {
				trap_R_AddLightToScene( flash.origin, 200 + ( rand() & 31 ), weapon->flashDlightColor[0],
										weapon->flashDlightColor[1], weapon->flashDlightColor[2], 0 );
			}
		} else {
			if ( weaponNum == WP_FLAMETHROWER ) {
				vec3_t angles;
				AxisToAngles( flash.axis, angles );
				CG_FireFlameChunks( cent, flash.origin, angles, 1.0, qfalse, 0 );
			
			}
		}
	}
}

void CG_AddPlayerFoot( refEntity_t *parent, playerState_t *ps, centity_t *cent ) {
	refEntity_t wolfkick;
	vec3_t kickangle;
	int frame;

	if ( !( cg.snap->ps.persistant[PERS_WOLFKICK] ) ) {
		return;
	}

	memset( &wolfkick, 0, sizeof( wolfkick ) );

	VectorCopy( parent->lightingOrigin, wolfkick.lightingOrigin );
	wolfkick.shadowPlane = parent->shadowPlane;

	// note to self we want this to lerp and advance frame
	wolfkick.renderfx = RF_DEPTHHACK | RF_FIRST_PERSON;;
	wolfkick.hModel = wolfkickModel; // eugeny

	if ( wolfkickSkin ) {
		wolfkick.customSkin = wolfkickSkin;
	}

	VectorCopy( cg.refdef.vieworg, wolfkick.origin );
	//----(SA)	allow offsets for testing boot model
	if ( cg_gun_x.value ) {
		VectorMA( wolfkick.origin, cg_gun_x.value,  cg.refdef.viewaxis[0], wolfkick.origin );
	}
	if ( cg_gun_y.value ) {
		VectorMA( wolfkick.origin, cg_gun_y.value,  cg.refdef.viewaxis[1], wolfkick.origin );
	}
	if ( cg_gun_z.value ) {
		VectorMA( wolfkick.origin, cg_gun_z.value,  cg.refdef.viewaxis[2], wolfkick.origin );
	}
	//----(SA)	end


	VectorCopy( cg.refdefViewAngles, kickangle );
	if ( kickangle[0] < 0 ) {
		kickangle[0] = 0;                       //----(SA)	avoid "Rockette" syndrome :)
	}
	AnglesToAxis( kickangle, wolfkick.axis );


	frame = cg.snap->ps.persistant[PERS_WOLFKICK];

//	CG_Printf("frame: %d\n", frame);

	wolfkick.frame = frame;
	wolfkick.oldframe = frame - 1;
	wolfkick.backlerp = 1 - cg.frameInterpolation;
	trap_R_AddRefEntityToScene( &wolfkick );

}

/*
==============
CG_AddViewWeapon

Add the weapon, and flash for the player's view
==============
*/
void CG_AddViewWeapon( playerState_t *ps ) {
	refEntity_t hand;
	vec3_t fovOffset;
	vec3_t angles;
	vec3_t gunoff;
	weaponInfo_t    *weapon;

	if ( ps->persistant[PERS_TEAM] == TEAM_SPECTATOR ) {
		return;
	}

	if ( ps->pm_type == PM_INTERMISSION ) {
		return;
	}

	// no gun if in third person view
	if ( cg.renderingThirdPerson ) {
		return;
	}

	// allow the gun to be completely removed
	if ( !cg_drawGun.integer ) {
/*
		vec3_t origin;

		if ( cg.predictedPlayerState.eFlags & EF_FIRING ) {
			// special hack for lightning gun...
			VectorCopy( cg.refdef.vieworg, origin );
			VectorMA( origin, -8, cg.refdef.viewaxis[2], origin );
			CG_LightningBolt( &cg_entities[ps->clientNum], origin );
		}
*/
		return;
	}

	// don't draw if testing a gun model
	if ( cg.testGun ) {
		return;
	}

	if ( ps->eFlags & EF_MG42_ACTIVE ) {
		return;
	}

	VectorClear(fovOffset);

	if ( cg_fixedAspect.integer ) {
		fovOffset[2] = 0;
	} else if ( cg.fov > 90 ) {
		// drop gun lower at higher fov
		fovOffset[2] = -0.2 * ( cg.fov - 90 ) * cg.refdef.fov_x / cg.fov;
	} else if ( cg.fov < 90 ) {
		// move gun forward at lower fov
		fovOffset[0] = -0.2 * ( cg.fov - 90 ) * cg.refdef.fov_x / cg.fov;
	} else if ( cg_fov.integer > 90 ) {
		// old auto adjust
		fovOffset[2] = -0.2 * ( cg_fov.integer - 90 );
 	}

	memset( &hand, 0, sizeof( hand ) );

	if ( ps->weapon > WP_NONE ) {

			//CG_RegisterWeapon( ps->weapon, qfalse );
			weapon = &cg_weapons[ ps->weapon ];

		// set up gun position
		CG_CalculateWeaponPosition( hand.origin, angles );
            
			// RealRTCW gun position is defined in .weap files if CVAR is active.
            if ( cg_gunPosLock.integer == 1 ) 
			{
		    gunoff[0] = weapon->weaponPosition[0];
			gunoff[1] = weapon->weaponPosition[1];
			gunoff[2] = weapon->weaponPosition[2];
			} 
			else if ( cg_gunPosLock.integer == 2 ) 
			{
			gunoff[0] = weapon->weaponPositionAlt[0];
			gunoff[1] = weapon->weaponPositionAlt[1];
			gunoff[2] = weapon->weaponPositionAlt[2];
			} 
			else if ( cg_gunPosLock.integer == 3 )
			{
			gunoff[0] = weapon->weaponPositionAlt2[0];
			gunoff[1] = weapon->weaponPositionAlt2[1];
			gunoff[2] = weapon->weaponPositionAlt2[2];
			} 
			else
			{
			gunoff[0] = cg_gun_x.value;
			gunoff[1] = cg_gun_y.value;
			gunoff[2] = cg_gun_z.value;
			}

		VectorMA( hand.origin, ( gunoff[0] + fovOffset[0] ), cg.refdef.viewaxis[0], hand.origin );
		VectorMA( hand.origin, ( gunoff[1] + fovOffset[1] ), cg.refdef.viewaxis[1], hand.origin );
		VectorMA( hand.origin, ( gunoff[2] + fovOffset[2] ), cg.refdef.viewaxis[2], hand.origin );

		AnglesToAxis( angles, hand.axis );

		if ( cg_gun_frame.integer ) {
			hand.frame = hand.oldframe = cg_gun_frame.integer;
			hand.backlerp = 0;
		} else {  // get the animation state
			CG_WeaponAnimation( ps, weapon, &hand.oldframe, &hand.frame, &hand.backlerp );   //----(SA)	changed
		}

		VectorCopy( hand.origin, hand.lightingOrigin );

		hand.hModel = weapon->handsModel;
		hand.renderfx = RF_DEPTHHACK | RF_FIRST_PERSON | RF_MINLIGHT;   //----(SA)

		// add everything onto the hand
		CG_AddPlayerWeapon( &hand, ps, &cg.predictedPlayerEntity );
		// Ridah

	}   // end  "if ( ps->weapon > WP_NONE)"

	// Rafael
	// add the foot
	CG_AddPlayerFoot( &hand, ps, &cg.predictedPlayerEntity );

	cg.predictedPlayerEntity.lastWeaponClientFrame = cg.clientFrame;
}

/*
==============================================================================

WEAPON SELECTION

==============================================================================
*/

#define WP_ICON_X       38  // new sizes per MK
#define WP_ICON_X_WIDE  72  // new sizes per MK
#define WP_ICON_Y       38
#define WP_ICON_SPACE_Y 10
#define WP_DRAW_X       640 - WP_ICON_X - 4 // 4 is 'selected' border width
#define WP_DRAW_X_WIDE  640 - WP_ICON_X_WIDE - 4
#define WP_DRAW_Y       4

// secondary fire icons
#define WP_ICON_SEC_X   18  // new sizes per MK
#define WP_ICON_SEC_Y   18


/*
===================
CG_DrawWeaponSelect
===================
*/
void CG_DrawWeaponSelect( void ) {
	int i;
	int x, y;
	int curweap, curweapbank = 0, curweapcycle = 0, drawweap;
	int realweap;               // DHM - Nerve
	int bits[MAX_WEAPONS / ( sizeof( int ) * 8 )];
	float       *color;

	// don't display if dead
	if ( cg.predictedPlayerState.stats[STAT_HEALTH] <= 0 ) {
		return;
	}

	if ( !cg.weaponSelect ) {
		return;
	}

	color = CG_FadeColor( cg.weaponSelectTime, WEAPON_SELECT_TIME );
	if ( !color ) {
		return;
	}
	trap_R_SetColor( color );

	if ( cg_fixedAspect.integer == 2 ) {
		CG_SetScreenPlacement(PLACE_RIGHT, PLACE_TOP);
	}

//----(SA)	neither of these overlap the weapon selection area anymore, so let them stay
	// showing weapon select clears pickup item display, but not the blend blob
//	cg.itemPickupTime = 0;

	// also clear holdable list
//	cg.holdableSelectTime = 0;
//----(SA)	end

	// count the number of weapons owned
	memcpy( bits, cg.snap->ps.weapons, sizeof( bits ) );

	curweap = cg.weaponSelect;

	// get bank/cycle of current weapon
	if ( !CG_WeaponIndex( curweap, &curweapbank, &curweapcycle ) ) {

		// weapon selected isn't a primary weapon, so draw the alternates bank
		CG_WeaponIndex( getAltWeapon( curweap ), &curweapbank, &curweapcycle );

	}

	y = WP_DRAW_Y;

	for ( i = 0; i < maxWeapsInBank; i++ ) {

		qboolean wideweap; // is the icon one of the double width ones

		drawweap = weapBanks[curweapbank][i];

		realweap = drawweap;        // DHM - Nerve

		switch ( drawweap ) {
		case WP_THOMPSON:
		case WP_MP40:
		// RealRTCW weapons
		case WP_MP34:
		case WP_PPSH:
		case WP_MOSIN:
		case WP_G43:
		case WP_M1GARAND:
		case WP_BAR:
		case WP_MP44:
		case WP_MG42M:
		case WP_M97:
		case WP_AUTO5:
		case WP_BROWNING:
		case WP_STEN:
		case WP_MAUSER:
		case WP_DELISLE:
		case WP_GARAND:
		case WP_VENOM:
		case WP_TESLA:
		case WP_PANZERFAUST:
		case WP_FLAMETHROWER:
		case WP_FG42:
		case WP_FG42SCOPE:
		case WP_M1941:
			wideweap = qtrue;
			break;
		default:
			wideweap = qfalse;
			break;
		}

		if ( wideweap ) {
			x = WP_DRAW_X_WIDE;
		} else {
			x = WP_DRAW_X;
		}

           if(drawweap && COM_BitCheck( bits, drawweap )) {
			// you've got it, draw it

			CG_RegisterWeapon( drawweap, qfalse );

			if ( wideweap ) {
				// weapon icon
				if ( realweap == curweap ) {
					CG_DrawPic( x, y, WP_ICON_X_WIDE, WP_ICON_Y, cg_weapons[drawweap].weaponIcon[1] );
				} else {
					CG_DrawPic( x, y, WP_ICON_X_WIDE, WP_ICON_Y, cg_weapons[drawweap].weaponIcon[0] );
				}

				// no ammo cross
				if ( !CG_WeaponHasAmmo( realweap ) ) {   // DHM - Nerve
					CG_DrawPic( x, y, WP_ICON_X_WIDE, WP_ICON_Y, cgs.media.noammoShader );
				}
			} else {
				// weapon icon
				if ( realweap == curweap ) {
					CG_DrawPic( x, y, WP_ICON_X, WP_ICON_Y, cg_weapons[drawweap].weaponIcon[1] );
				} else {
					CG_DrawPic( x, y, WP_ICON_X, WP_ICON_Y, cg_weapons[drawweap].weaponIcon[0] );
				}

				// no ammo cross
				if ( !CG_WeaponHasAmmo( realweap ) ) {   // DHM - Nerve
					CG_DrawPic( x, y, WP_ICON_X, WP_ICON_Y, cgs.media.noammoShader );
				}
			}

		} else {
			continue;
		}

		// secondary fire
		if ( wideweap ) {
			x = WP_DRAW_X_WIDE - WP_ICON_SEC_X - 4;
		} else {
			x = WP_DRAW_X - WP_ICON_SEC_X - 4;
		}


		drawweap = getAltWeapon( weapBanks[curweapbank][i] );

		// clear drawweap if getaltweap() returns the same weap as passed in. (no secondary available)

		if ( drawweap == weapBanks[curweapbank][i] ) {
				drawweap = 0;
		}
		

		realweap = drawweap;        // DHM - Nerve

           if(drawweap && COM_BitCheck( bits, drawweap )) {

			CG_RegisterWeapon( drawweap, qfalse );

			// weapon icon
			if ( realweap == cg.weaponSelect ) {
				CG_DrawPic( x, y, WP_ICON_SEC_X, WP_ICON_SEC_Y, cg_weapons[drawweap].weaponIcon[1] );
			} else {
				CG_DrawPic( x, y, WP_ICON_SEC_X, WP_ICON_SEC_Y, cg_weapons[drawweap].weaponIcon[0] );
			}

			// no ammo cross
			if ( !CG_WeaponHasAmmo( realweap ) ) {
				CG_DrawPic( x, y, WP_ICON_SEC_X, WP_ICON_SEC_Y, cgs.media.noammoShader );
			}
		}


		y += ( WP_ICON_Y + WP_ICON_SPACE_Y );
	}
}




/*
==============
CG_WeaponHasAmmo
	check for ammo
==============
*/
static qboolean CG_WeaponHasAmmo( int i ) {

	if( i == WP_KNIFE ) {
		return qtrue;
	}

	if ( !( cg.predictedPlayerState.ammo[BG_FindAmmoForWeapon( i )] ) &&
		 !( cg.predictedPlayerState.ammoclip[BG_FindClipForWeapon( i )] ) ) {
		return qfalse;
	}

	return qtrue;
}


/*
===============
CG_WeaponSelectable
===============
*/
static qboolean CG_WeaponSelectable( int i ) {

	// allow the player to unselect all weapons
//	if(i == WP_NONE)
//		return qtrue;

	// if holding a melee weapon (chair/shield/etc.) only allow single-handed weapons
	if ( cg.snap->ps.eFlags & EF_MELEE_ACTIVE ) {

    if (ammoTable[i].twoHand == 1) 
	{
		return qfalse;
	}

	}

	// allow switch out of scope for weapons where you fired the last shot while scoped
	// and we left you in that view to see the result of the shot
	switch ( cg.weaponSelect ) {
	case WP_SNOOPERSCOPE:
		if ( i == WP_GARAND ) {
			return qtrue;
		}
		break;
	case WP_SNIPERRIFLE:
		if ( i == WP_MAUSER ) {
			return qtrue;
		}
		break;
	case WP_DELISLESCOPE:
		if ( i == WP_DELISLE ) {
			return qtrue;
		}
		break;
	case WP_M1941SCOPE:
		if ( i == WP_M1941 ) {
			return qtrue;
		}
		break;
	case WP_FG42SCOPE:
		if ( i == WP_FG42 ) {
			return qtrue;
		}
		break;
	default:
		break;
	}


	// check for weapon
	if ( !( COM_BitCheck( cg.predictedPlayerState.weapons, i ) ) ) {
		return qfalse;
	}

	if (!CG_WeaponHasAmmo(i) && (cg_newinventory.integer <= 0 && cg_gameType.integer != GT_SURVIVAL))
	{
		return qfalse;
	}

	return qtrue;
}




/*
==============
CG_WeaponIndex
==============
*/
int CG_WeaponIndex( int weapnum, int *bank, int *cycle ) {
	static int bnk, cyc;

	if ( weapnum <= 0 || weapnum >= WP_NUM_WEAPONS ) {
		if ( bank ) {
			*bank = 0;
		}
		if ( cycle ) {
			*cycle = 0;
		}
		return 0;
	}

	for ( bnk = 0; bnk < maxWeapBanks; bnk++ ) {
		for ( cyc = 0; cyc < maxWeapsInBank; cyc++ ) {

			// end of cycle, go to next bank

				if ( !weapBanks[bnk][cyc] ) {
					break;
				}
				// found the current weapon
				if ( weapBanks[bnk][cyc] == weapnum ) {
					if ( bank ) {
						*bank = bnk;
					}
					if ( cycle ) {
						*cycle = cyc;
					}
					return 1;
				}
		}
	}

	// failed to find the weapon in the table
	// probably an alternate

	return 0;
}



/*
==============
getNextWeapInBank
	Pass in a bank and cycle and this will return the next valid weapon higher in the cycle.
	if the weap passed in is above highest in a cycle (maxWeapsInBank), this will safely loop around
==============
*/
static int getNextWeapInBank( int bank, int cycle ) {

	cycle++;

	cycle = cycle % maxWeapsInBank;

		if ( weapBanks[bank][cycle] ) {    // return next weapon in bank if there is one
			return weapBanks[bank][cycle];
		} else {                            // return first in bank
			return weapBanks[bank][0];
		}
	

}

static int getNextWeapInBankBynum( int weapnum ) {
	int bank, cycle;

	if ( !CG_WeaponIndex( weapnum, &bank, &cycle ) ) {
		return weapnum;
	}

	return getNextWeapInBank( bank, cycle );
}


/*
==============
getPrevWeapInBank
	Pass in a bank and cycle and this will return the next valid weapon lower in the cycle.
	if the weap passed in is the lowest in a cycle (0), this will loop around to the
	top (maxWeapsInBank-1) and start down from there looking for a valid weapon position
==============
*/
static int getPrevWeapInBank( int bank, int cycle ) {
	cycle--;
	if ( cycle < 0 ) {
		cycle = maxWeapsInBank - 1;
	}

		while ( !weapBanks[bank][cycle] ) {
			cycle--;

			if ( cycle < 0 ) {
				cycle = maxWeapsInBank - 1;
			}
		}
		return weapBanks[bank][cycle];

}


static int getPrevWeapInBankBynum( int weapnum ) {
	int bank, cycle;

	if ( !CG_WeaponIndex( weapnum, &bank, &cycle ) ) {
		return weapnum;
	}

	return getPrevWeapInBank( bank, cycle );
}



/*
==============
getNextBankWeap
	Pass in a bank and cycle and this will return the next valid weapon in a higher bank.
	sameBankPosition: if there's a weapon in the next bank at the same cycle,
	return that	(colt returns thompson for example) rather than the lowest weapon
==============
*/
static int getNextBankWeap( int bank, int cycle, qboolean sameBankPosition ) {
	bank++;

	bank = bank % maxWeapBanks;

		if ( sameBankPosition && weapBanks[bank][cycle] ) {
			return weapBanks[bank][cycle];
		} else {
			return weapBanks[bank][0];
		}
}

/*
==============
getPrevBankWeap
	Pass in a bank and cycle and this will return the next valid weapon in a lower bank.
	sameBankPosition: if there's a weapon in the prev bank at the same cycle,
	return that	(thompson returns colt for example) rather than the highest weapon
==============
*/
static int getPrevBankWeap( int bank, int cycle, qboolean sameBankPosition ) {
	int i;

	bank--;

	if ( bank < 0 ) {    // don't go below 0, cycle up to top
		bank += maxWeapBanks;
	}
	bank = bank % maxWeapBanks;

		if ( sameBankPosition && weapBanks[bank][cycle] ) {
			return weapBanks[bank][cycle];
		} else
		{   // find highest weap in bank
			for ( i = maxWeapsInBank - 1; i >= 0; i-- ) {
				if ( weapBanks[bank][i] ) {
					return weapBanks[bank][i];
				}
			}

			// if it gets to here, no valid weaps in this bank, go down another bank
			return getPrevBankWeap( bank, cycle, sameBankPosition );
		}
}

/*
==============
getAltWeapon
==============
*/
static int getAltWeapon( int weapnum ) {
	
	if ( ammoTable[weapnum].weapAlts ) {
		return ammoTable[weapnum].weapAlts;
	}

	return weapnum;
}

/*
==============
getEquivWeapon
	return the id of the opposite team's weapon.
	Passing the weapnum of the mp40 returns the id of the thompson, and likewise
	passing the weapnum of the thompson returns the id of the mp40.
	No equivalent available will return the weapnum passed in.
==============
*/
int getEquivWeapon( int weapnum ) {
	int num = weapnum;

	switch ( weapnum ) {
		// going from german to american
	case WP_LUGER:              num = WP_COLT;              break;
	case WP_MAUSER:             num = WP_GARAND;            break;
	case WP_MP40:               num = WP_THOMPSON;          break;
	case WP_GRENADE_LAUNCHER:   num = WP_GRENADE_PINEAPPLE; break;

		// going from american to german
	case WP_COLT:               num = WP_LUGER;             break;
	case WP_GARAND:             num = WP_MAUSER;            break;
	case WP_THOMPSON:           num = WP_MP40;              break;
	case WP_GRENADE_PINEAPPLE:  num = WP_GRENADE_LAUNCHER;  break;
	}
	return num;
}



/*
==============
CG_WeaponSuggest
==============
*/
void CG_WeaponSuggest( int weap ) {
#if 0 // not currently supported
	int bank, cycle;

	if ( !cg_useSuggestedWeapons.integer ) {
		return;
	}

	cg.weaponSelectTime = cg.time;

	CG_WeaponIndex( weap, &bank, &cycle );    // get location of this weap

	cg.lastWeapSelInBank[bank] = weap;      // make this weap first priority in that bank
#endif
}


/*
==============
CG_SetSniperZoom
==============
*/

void CG_SetSniperZoom( int lastweap, int newweap ) {
	int zoomindex;

	if ( lastweap == newweap ) {
		return;
	}

	cg.zoomval      = 0;
	cg.zoomedScope  = 0;

	// check for fade-outs
	switch ( lastweap ) {
	case WP_SNIPERRIFLE:
//			cg.zoomedScope	= 500;	// TODO: add to zoomTable
//			cg.zoomTime		= cg.time;
		break;
	case WP_SNOOPERSCOPE:
//			cg.zoomedScope	= 500;	// TODO: add to zoomTable
//			cg.zoomTime		= cg.time;
		break;
	case WP_FG42SCOPE:
//			cg.zoomedScope	= 1;	// TODO: add to zoomTable
//			cg.zoomTime		= cg.time;
		break;
	case WP_DELISLESCOPE:
//			cg.zoomedScope	= 1;	// TODO: add to zoomTable
//			cg.zoomTime		= cg.time;
		break;
	case WP_M1941SCOPE:
//			cg.zoomedScope	= 1;	// TODO: add to zoomTable
//			cg.zoomTime		= cg.time;
		break;
	}

	switch ( newweap ) {

	default:
		return;     // no sniper zoom, get out.

	case WP_SNIPERRIFLE:
		cg.zoomval = cg_zoomDefaultSniper.value;
		cg.zoomedScope  = 900;      // TODO: add to zoomTable
		zoomindex = ZOOM_SNIPER;
		break;
	case WP_DELISLESCOPE:
		cg.zoomval = cg_zoomDefaultSniper.value;
		cg.zoomedScope  = 700;      // TODO: add to zoomTable
		zoomindex = ZOOM_SNIPER;
		break;
	case WP_M1941SCOPE:
		cg.zoomval = cg_zoomDefaultSniper.value;
		cg.zoomedScope  = 700;      // TODO: add to zoomTable
		zoomindex = ZOOM_SNIPER;
		break;
	case WP_SNOOPERSCOPE:
		cg.zoomval = cg_zoomDefaultSnooper.value;
		cg.zoomedScope  = 800;      // TODO: add to zoomTable
		zoomindex = ZOOM_SNOOPER;
		break;
	case WP_FG42SCOPE:
		cg.zoomval = cg_zoomDefaultFG.value;
		cg.zoomedScope  = 1;        // TODO: add to zoomTable
		zoomindex = ZOOM_FG42SCOPE;
		break;
	}

	// constrain user preferred fov to weapon limitations
	if ( cg.zoomval > zoomTable[zoomindex][ZOOM_OUT] ) {
		cg.zoomval = zoomTable[zoomindex][ZOOM_OUT];
	}
	if ( cg.zoomval < zoomTable[zoomindex][ZOOM_IN] ) {
		cg.zoomval = zoomTable[zoomindex][ZOOM_IN];
	}

	cg.zoomTime     = cg.time;
}


/*
==============
CG_PlaySwitchSound
	Get special switching sounds if they're there
==============
*/
void CG_PlaySwitchSound( int lastweap, int newweap ) {
//	weaponInfo_t	*weap;
//	weap = &cg_weapons[ ent->weapon ];
	sfxHandle_t switchsound;	
	switchsound = cgs.media.selectSound;

	if ( getAltWeapon( lastweap ) == newweap ) { // alt switch
		switch ( newweap ) {
		case WP_M7:
			switchsound = cg_weapons[newweap].switchSound[0];
			break;
		case WP_M1GARAND:
			if ( cg.predictedPlayerState.ammoclip[lastweap] ) {
				switchsound = cg_weapons[newweap].switchSound[0];
			}
			break;
		default:
			break;
		}
	}

	switch ( newweap ) {
		case WP_KNIFE:
		     switchsound = cg_weapons[newweap].switchSound[0];
			 break;
	}

	trap_S_StartSound( NULL, cg.snap->ps.clientNum, CHAN_WEAPON, switchsound );
}


/*
==============
CG_FinishWeaponChange
==============
*/
void CG_FinishWeaponChange( int lastweap, int newweap ) {
	int newbank;

	cg.weaponSelectTime = cg.time;  // flash the weapon icon

	// remember which weapon in this bank was last selected so when cycling back
	// to this bank, that weap will be highlighted first
	if ( CG_WeaponIndex( newweap, &newbank, NULL ) ) {
		cg.lastWeapSelInBank[newbank] = newweap;
	}

	if ( lastweap == newweap ) {   // no need to do any more than flash the icon
		return;
	}

	CG_PlaySwitchSound( lastweap, newweap );  //----(SA)	added

	CG_SetSniperZoom( lastweap, newweap );

	// setup for a user call to CG_LastWeaponUsed_f()
	if ( lastweap == cg.lastFiredWeapon ) {
		// don't set switchback for some weaps...
		switch ( lastweap ) {
		case WP_SNIPERRIFLE:
		case WP_SNOOPERSCOPE:
		case WP_FG42SCOPE:
		case WP_DELISLESCOPE:
		case WP_M1941SCOPE:
			break;
		default:
			cg.switchbackWeapon = lastweap;
			break;
		}
	} else {
		// if this ended up having the switchback be the same
		// as the new weapon, set the switchback to the prev
		// selected weapon will become the switchback
		if ( cg.switchbackWeapon == newweap ) {
			cg.switchbackWeapon = lastweap;
		}
	}

	cg.weaponSelect     = newweap;
}

/*
==============
CG_AltfireWeapon_f
	for example, switching between WP_MAUSER and WP_SNIPERRIFLE
==============
*/
void CG_AltWeapon_f( void ) {
	int original, num;
	float spd = VectorLength( cg.snap->ps.velocity );

	trap_S_StartSoundEx(NULL, cg.snap->ps.clientNum, CHAN_WEAPON, cgs.media.nullSound, SND_CUTOFF);

	if ( !cg.snap ) {
		return;
	}
	if ( cg.snap->ps.pm_flags & PMF_FOLLOW ) {
		return;
	}

	if ( cg.snap->ps.eFlags & EF_MG42_ACTIVE ) { // no alt-switching when on mg42
		return;
	}

	if ( cg.time - cg.weaponSelectTime < cg_weaponCycleDelay.integer ) {
		return; // force pause so holding it down won't go too fast

	}

	original = cg.weaponSelect;

	num = getAltWeapon( original );

	if ( CG_WeaponSelectable( num ) ) {   // new weapon is valid
		
		switch ( original ) {
		case WP_MAUSER:
		case WP_GARAND:
		case WP_FG42:
		case WP_DELISLE:
		case WP_M1941:
		    if ( spd > 180.0f ) 
			{
				return;
			}
			break;
		}

		CG_FinishWeaponChange( original, num );
	}



		// Arnout: don't allow another weapon switch when we're still swapping the gpg40, to prevent animation breaking
	if ( ( cg.snap->ps.weaponstate == WEAPON_RAISING || cg.snap->ps.weaponstate == WEAPON_DROPPING ) &&
		 ( ( original == WP_M7 || num == WP_M7 ) ) ) {
		return;
	}
}


/*
==============
CG_NextWeap
  switchBanks - curweap is the last in a bank, 'qtrue' means go to the next available bank, 'qfalse' means loop to the head of the bank
==============
*/
void CG_NextWeap( qboolean switchBanks ) {
	int bank = 0, cycle = 0, newbank = 0, newcycle = 0;
	int num, curweap;
	qboolean nextbank = qfalse;     // need to switch to the next bank of weapons?
	int i, j;
	num = curweap = cg.weaponSelect;

	CG_WeaponIndex( curweap, &bank, &cycle );     // get bank/cycle of current weapon

	switch ( num ) {
	case WP_M7:
		curweap = num = WP_M1GARAND;
		break;
	case WP_SNIPERRIFLE:
		curweap = num = WP_MAUSER;
		break;
	case WP_FG42SCOPE:
		curweap = num = WP_FG42;
		break;
	case WP_SNOOPERSCOPE:
		curweap = num = WP_GARAND;
		break;
	}




//	if ( cg_cycleAllWeaps.integer || !switchBanks ) {
	if ( 1 ) {
		for ( i = 0; i < maxWeapsInBank; i++ ) {
			num = getNextWeapInBankBynum( num );
			CG_WeaponIndex( num, NULL, &newcycle );       // get cycle of new weapon.  if it's lower than the original, then it cycled around
			if ( switchBanks ) {
				if ( newcycle <= cycle ) {
					nextbank = qtrue;
					break;
				}
			} else {    // don't switch banks if you get to the end
				if ( num == curweap ) {    // back to start, just leave it where it is
					return;
				}
			}

			if ( CG_WeaponSelectable( num ) ) {
				break;
			} else {
				qboolean found = qfalse;
				switch ( num ) {
				case WP_M1GARAND:
					if ( ( found = CG_WeaponSelectable( WP_M7 ) ) ) {
						num = WP_M7;
					}
					break;
				}

				if ( found ) {
					break;
				}
			}
		}
	} else {
		nextbank = qtrue;
	}
	if ( nextbank ) {
		for ( i = 0; i < maxWeapBanks; i++ ) {
//			if ( cg_cycleAllWeaps.integer ) {
			if ( 1 ) {
				num = getNextBankWeap( bank + i, cycle, qfalse );   // cycling all weaps always starts the next bank at the bottom
			} else {
				if ( cg.lastWeapSelInBank[bank + i + 1] ) {
					num = cg.lastWeapSelInBank[bank + i + 1];
				} else {
					num = getNextBankWeap( bank + i, cycle, qtrue );
				}
			}
			if ( num == 0 ) {
				continue;
			}


			if ( CG_WeaponSelectable( num ) ) { // first entry in bank was selectable, no need to scan the bank
				break;
			} else {
				qboolean found = qfalse;
				switch ( num ) {
				case WP_M1GARAND:
					if ( ( found = CG_WeaponSelectable( WP_M7 ) ) ) {
						num = WP_M7;
					}
					break;
				}

				if ( found ) {
					break;
				}
			}

			CG_WeaponIndex( num, &newbank, &newcycle );   // get the bank of the new weap

			for ( j = newcycle; j < MAX_WEAPS_IN_BANK; j++ ) {
				num = getNextWeapInBank( newbank, j );


				if ( CG_WeaponSelectable( num ) ) { // found selectable weapon
					break;
				} else {
					qboolean found = qfalse;
					switch ( num ) {
					case WP_M1GARAND:
						if ( ( found = CG_WeaponSelectable( WP_M7 ) ) ) {
							num = WP_M7;
						}
						break;
					}

					if ( found ) {
						break;
					}
				}

				num = 0;
			}
			if ( num ) {   // a selectable weapon was found in the current bank
				break;
			}
		}
	}
	CG_FinishWeaponChange( curweap, num ); //----(SA)
}

/*
==============
CG_PrevWeap
  switchBanks - curweap is the last in a bank
		'qtrue'  - go to the next available bank
		'qfalse' - loop to the head of the bank
==============
*/
void CG_PrevWeap( qboolean switchBanks ) {
	int bank = 0, cycle = 0, newbank = 0, newcycle = 0;
	int num, curweap;
	qboolean prevbank = qfalse;     // need to switch to the next bank of weapons?
	int i, j;

	num = curweap = cg.weaponSelect;

	switch ( num ) {
	case WP_M7:
		curweap = num = WP_M1GARAND;
		break;
	case WP_SNIPERRIFLE:
		curweap = num = WP_MAUSER;
		break;
	case WP_FG42SCOPE:
		curweap = num = WP_FG42;
		break;
	case WP_SNOOPERSCOPE:
		curweap = num = WP_GARAND;
		break;
	}

	CG_WeaponIndex( curweap, &bank, &cycle );     // get bank/cycle of current weapon

	if ( 1 ) {

		for ( i = cycle; i >= 0; i-- ) {
			num = getPrevWeapInBankBynum( num );
			CG_WeaponIndex( num, NULL, &newcycle );         // get cycle of new weapon.  if it's greater than the original, then it cycled around
			if ( switchBanks ) {
				if ( newcycle > ( cycle - 1 ) ) {
					prevbank = qtrue;
					break;
				}
			} else {        // don't switch banks if you get to the end
				if ( num == curweap ) {     // back to start, just leave it where it is
					return;
				}
			}

			if ( CG_WeaponSelectable( num ) ) {
				break;
			} else {
				qboolean found = qfalse;
				switch ( num ) {
				case WP_M1GARAND:
					if ( ( found = CG_WeaponSelectable( WP_M7 ) ) ) {
						num = WP_M7;
					}
					break;
				}

				if ( found ) {
					break;
				}
			}
		}
	} else {
		prevbank = qtrue;
	}

	if ( prevbank ) {
		for ( i = 0; i < MAX_WEAP_BANKS; i++ ) {
			if ( cg_cycleAllWeaps.integer ) {
				num = getPrevBankWeap( bank - i, cycle, qfalse );   // cycling all weaps always starts the next bank at the bottom
			} else {
				num = getPrevBankWeap( bank - i, cycle, qtrue );
			}
			if ( num == 0 ) {
				continue;
			}

			if ( CG_WeaponSelectable( num ) ) { // first entry in bank was selectable, no need to scan the bank
				break;
			} else {
				qboolean found = qfalse;
				switch ( num ) {
				case WP_M1GARAND:
					if ( ( found = CG_WeaponSelectable( WP_M7 ) ) ) {
						num = WP_M7;
					}
					break;
				}

				if ( found ) {
					break;
				}
			}

			CG_WeaponIndex( num, &newbank, &newcycle );   // get the bank of the new weap

			for ( j = MAX_WEAPS_IN_BANK; j > 0; j-- ) {
				num = getPrevWeapInBank( newbank, j );

				if ( CG_WeaponSelectable( num ) ) { // found selectable weapon
					break;
				} else {
					qboolean found = qfalse;
					switch ( num ) {
					case WP_M1GARAND:
						if ( ( found = CG_WeaponSelectable( WP_M7 ) ) ) {
							num = WP_M7;
						}
						break;
					}

					if ( found ) {
						break;
					}
				}

				num = 0;
			}

			if ( num ) { // a selectable weapon was found in the current bank
				break;
			}
		}
	}
	CG_FinishWeaponChange( curweap, num ); //----(SA)
}

/*
==============
CG_LastWeaponUsed_f
==============
*/
void CG_LastWeaponUsed_f( void ) {
	if ( cg.time - cg.weaponSelectTime < cg_weaponCycleDelay.integer ) {
		return; // force pause so holding it down won't go too fast
	}

	trap_S_StartSoundEx(NULL, cg.snap->ps.clientNum, CHAN_WEAPON, cgs.media.nullSound, SND_CUTOFF);

	cg.weaponSelectTime = cg.time;  // flash the current weapon icon

	if ( !cg.switchbackWeapon ) {
		cg.switchbackWeapon = cg.weaponSelect;
		return;
	}
	if ( CG_WeaponSelectable( cg.switchbackWeapon ) ) {
		CG_FinishWeaponChange( cg.weaponSelect, cg.switchbackWeapon );
	} else {    // switchback no longer selectable, reset cycle
		cg.switchbackWeapon = 0;
	}
}

/*
==============
CG_NextWeaponInBank_f
==============
*/
void CG_NextWeaponInBank_f( void ) {

	if ( cg.time - cg.weaponSelectTime < cg_weaponCycleDelay.integer ) {
		return; // force pause so holding it down won't go too fast

	}

	trap_S_StartSoundEx(NULL, cg.snap->ps.clientNum, CHAN_WEAPON, cgs.media.nullSound, SND_CUTOFF);


	// this cvar is an option that lets the player use his weapon switching keys (probably the mousewheel)
	// for zooming (binocs/snooper/sniper/etc.)
	if ( cg.zoomval ) {
		if ( cg_useWeapsForZoom.integer == 1 ) {
			CG_ZoomIn_f();
			return;
		} else if ( cg_useWeapsForZoom.integer == 2 ) {
			CG_ZoomOut_f();
			return;
		}
	}

	cg.weaponSelectTime = cg.time;  // flash the current weapon icon

	CG_NextWeap( qfalse );
}

/*
==============
CG_PrevWeaponInBank_f
==============
*/
void CG_PrevWeaponInBank_f( void ) {

	if ( cg.time - cg.weaponSelectTime < cg_weaponCycleDelay.integer ) {
		return; // force pause so holding it down won't go too fast

	}

	trap_S_StartSoundEx(NULL, cg.snap->ps.clientNum, CHAN_WEAPON, cgs.media.nullSound, SND_CUTOFF);

	// this cvar is an option that lets the player use his weapon switching keys (probably the mousewheel)
	// for zooming (binocs/snooper/sniper/etc.)
	if ( cg.zoomval ) {
		if ( cg_useWeapsForZoom.integer == 2 ) {
			CG_ZoomIn_f();
			return;
		} else if ( cg_useWeapsForZoom.integer == 1 ) {
			CG_ZoomOut_f();
			return;
		}
	}

	cg.weaponSelectTime = cg.time;  // flash the current weapon icon

	CG_PrevWeap( qfalse );
}


/*
==============
CG_NextWeapon_f
==============
*/
void CG_NextWeapon_f( void ) {

	if ( !cg.snap ) {
		return;
	}
	if ( cg.snap->ps.pm_flags & PMF_FOLLOW ) {
		return;
	}

	trap_S_StartSoundEx(NULL, cg.snap->ps.clientNum, CHAN_WEAPON, cgs.media.nullSound, SND_CUTOFF);

	// this cvar is an option that lets the player use his weapon switching keys (probably the mousewheel)
	// for zooming (binocs/snooper/sniper/etc.)
	if ( cg.zoomval ) {
		if ( cg_useWeapsForZoom.integer == 1 ) {
			CG_ZoomIn_f();
			return;
		} else if ( cg_useWeapsForZoom.integer == 2 ) {
			CG_ZoomOut_f();
			return;
		}
	}

	if ( cg.time - cg.weaponSelectTime < cg_weaponCycleDelay.integer ) {
		return; // force pause so holding it down won't go too fast

	}
	cg.weaponSelectTime = cg.time;  // flash the current weapon icon

	CG_NextWeap( qtrue );
}


/*
==============
CG_PrevWeapon_f
==============
*/
void CG_PrevWeapon_f( void ) {
	if ( !cg.snap ) {
		return;
	}
	if ( cg.snap->ps.pm_flags & PMF_FOLLOW ) {
		return;
	}

	trap_S_StartSoundEx(NULL, cg.snap->ps.clientNum, CHAN_WEAPON, cgs.media.nullSound, SND_CUTOFF);

	// this cvar is an option that lets the player use his weapon switching keys (probably the mousewheel)
	// for zooming (binocs/snooper/sniper/etc.)
	if ( cg.zoomval ) {
		if ( cg_useWeapsForZoom.integer == 1 ) {
			CG_ZoomOut_f();
			return;
		} else if ( cg_useWeapsForZoom.integer == 2 ) {
			CG_ZoomIn_f();
			return;
		}
	}

	if ( cg.time - cg.weaponSelectTime < cg_weaponCycleDelay.integer ) {
		return; // force pause so holding it down won't go too fast

	}
	cg.weaponSelectTime = cg.time;  // flash the current weapon icon

	CG_PrevWeap( qtrue );

}


/*
==============
CG_WeaponBank_f
	weapon keys are not generally bound directly('bind 1 weapon 1'),
	rather the key is bound to a given bank ('bind 1 weaponbank 1')
==============
*/
void CG_WeaponBank_f( void ) {
	int num, i, curweap;
	int curbank = 0, curcycle = 0, bank = 0, cycle = 0;
	
	trap_S_StartSoundEx(NULL, cg.snap->ps.clientNum, CHAN_WEAPON, cgs.media.nullSound, SND_CUTOFF);

	if ( !cg.snap ) {
		return;
	}

	if ( cg.snap->ps.pm_flags & PMF_FOLLOW ) {
		return;
	}

	if ( cg.time - cg.weaponSelectTime < cg_weaponCycleDelay.integer ) {
		return; // force pause so holding it down won't go too fast

	}
	cg.weaponSelectTime = cg.time;  // flash the current weapon icon

	bank = atoi( CG_Argv( 1 ) );

	if ( bank <= 0 || bank > maxWeapBanks ) {
		return;
	}

	curweap = cg.weaponSelect;
	CG_WeaponIndex( curweap, &curbank, &curcycle );       // get bank/cycle of current weapon

	if ( !cg.lastWeapSelInBank[bank] ) {
		num = weapBanks[bank][0];
		cycle -= 1;   // cycle up to first weap
	} else {
		num = cg.lastWeapSelInBank[bank];
		CG_WeaponIndex( num, &bank, &cycle );
		if ( bank != curbank ) {
			cycle -= 1;
		}

	}

	for ( i = 0; i < maxWeapsInBank; i++ ) {
		num = getNextWeapInBank( bank, cycle + i );

		if ( CG_WeaponSelectable( num ) ) {
			break;
		}
	}

	if ( i == maxWeapsInBank ) {
		return;
	}

	CG_FinishWeaponChange( curweap, num );

}

/*
===============
CG_Weapon_f
===============
*/
void CG_Weapon_f( void ) {
	int num, i, curweap;
	int bank = 0, cycle = 0, newbank = 0, newcycle = 0;
	qboolean banked = qfalse;

	trap_S_StartSoundEx(NULL, cg.snap->ps.clientNum, CHAN_WEAPON, cgs.media.nullSound, SND_CUTOFF);

	if ( !cg.snap ) {
		return;
	}

	if ( cg.snap->ps.pm_flags & PMF_FOLLOW ) {
		return;
	}

	num = atoi( CG_Argv( 1 ) );

	cg.weaponSelectTime = cg.time;  // flash the current weapon icon


	if ( num <= WP_NONE || num > WP_NUM_WEAPONS ) {
		return;
	}

	curweap = cg.weaponSelect;

	CG_WeaponIndex( curweap, &bank, &cycle );     // get bank/cycle of current weapon
	banked = CG_WeaponIndex( num, &newbank, &newcycle );      // get bank/cycle of requested weapon

	// the new weapon was not found in the reglar banks
	// assume the player want's to go directly to it if possible
	if ( !banked ) {
		if ( CG_WeaponSelectable( num ) ) {
			CG_FinishWeaponChange( curweap, num );
			return;
		}
	}

	if ( bank != newbank ) {
		cycle = newcycle - 1;   //	drop down one from the requested weap's cycle so it will
	}
	//	try to initially cycle up to the requested weapon

	for ( i = 0; i < maxWeapsInBank; i++ ) {
		num = getNextWeapInBank( newbank, cycle + i );

		if ( num == curweap ) { // no other weapons in bank
			return;
		}

		if ( CG_WeaponSelectable( num ) ) {
			break;
		}
	}

	if ( i == maxWeapsInBank ) {
		return;
	}

	CG_FinishWeaponChange( curweap, num );
}

/*
===================
CG_OutOfAmmoChange

The current weapon has just run out of ammo
===================
*/
void CG_OutOfAmmoChange( void ) {
	int i;
	int bank = 0, cycle = 0;
	int equiv = WP_NONE;

	//
	// trivial switching
	//

	// if you're using an alt mode weapon, try switching back to the parent
	// otherwise, switch to the equivalent if you've got it
	if ( cg.weaponSelect >= WP_SNIPERRIFLE && cg.weaponSelect <= WP_M7 ) {
		cg.weaponSelect = equiv = getAltWeapon( cg.weaponSelect );    // base any further changes on the parent
		if ( CG_WeaponSelectable( equiv ) ) {    // the parent was selectable, drop back to that
			CG_FinishWeaponChange( cg.predictedPlayerState.weapon, cg.weaponSelect ); //----(SA)
			return;
		}
	}


	// now try the opposite team's equivalent weap
	equiv = getEquivWeapon( cg.weaponSelect );

	if ( equiv != cg.weaponSelect && CG_WeaponSelectable( equiv ) ) {
		cg.weaponSelect = equiv;
		CG_FinishWeaponChange( cg.predictedPlayerState.weapon, cg.weaponSelect ); //----(SA)
		return;
	}

	//
	// more complicated selection
	//

	// didn't have available alternative or equivalent, try another weap in the bank
	CG_WeaponIndex( cg.weaponSelect, &bank, &cycle );     // get bank/cycle of current weapon

	for ( i = cycle; i < maxWeapsInBank; i++ ) {
		equiv = getNextWeapInBank( bank, i );
		if ( CG_WeaponSelectable( equiv ) ) {  // found a reasonable replacement
			cg.weaponSelect = equiv;
			CG_FinishWeaponChange( cg.predictedPlayerState.weapon, cg.weaponSelect ); //----(SA)
			return;
		}
	}

	


	// still nothing available, just go to the next
	// available weap using the regular selection scheme
	CG_NextWeap( qtrue );

}

/*
===================================================================================================

WEAPON EVENTS

===================================================================================================
*/

// Note to self this is dead code
void CG_MG42EFX( centity_t *cent ) {
	vec3_t forward;
	vec3_t point;
	refEntity_t flash;

//	trap_S_StartSound( NULL, cent->currentState.number, CHAN_WEAPON, hWeaponSnd );

	VectorCopy( cent->currentState.origin, point );
	AngleVectors( cent->currentState.angles, forward, NULL, NULL );
	VectorMA( point, 40, forward, point );
	trap_R_AddLightToScene( point, 200 + ( rand() & 31 ),1.0, 0.6, 0.23, 0 );

	memset( &flash, 0, sizeof( flash ) );
	flash.renderfx = RF_LIGHTING_ORIGIN;
	flash.hModel = cgs.media.mg42muzzleflash;

	VectorCopy( point, flash.origin );
	AnglesToAxis( cg.refdefViewAngles, flash.axis );

	trap_R_AddRefEntityToScene( &flash );
}

void CG_FLAKEFX( centity_t *cent, int whichgun ) {
	entityState_t *ent;
	vec3_t forward, right, up;
	vec3_t point;
	refEntity_t flash;

	ent = &cent->currentState;

	VectorCopy( cent->currentState.pos.trBase, point );
	AngleVectors( cent->currentState.apos.trBase, forward, right, up );

	// gun 1 and 2 were switched
	if ( whichgun == 2 ) {
		VectorMA( point, 136, forward, point );
		VectorMA( point, 31, up, point );
		VectorMA( point, 22, right, point );
	} else if ( whichgun == 1 )     {
		VectorMA( point, 136, forward, point );
		VectorMA( point, 31, up, point );
		VectorMA( point, -22, right, point );
	} else if ( whichgun == 3 )     {
		VectorMA( point, 136, forward, point );
		VectorMA( point, 10, up, point );
		VectorMA( point, 22, right, point );
	} else if ( whichgun == 4 )     {
		VectorMA( point, 136, forward, point );
		VectorMA( point, 10, up, point );
		VectorMA( point, -22, right, point );
	}

	trap_R_AddLightToScene( point, 200 + ( rand() & 31 ),1.0, 0.6, 0.23, 0 );

	memset( &flash, 0, sizeof( flash ) );
	flash.renderfx = RF_LIGHTING_ORIGIN;
	flash.hModel = cgs.media.mg42muzzleflash;

	VectorCopy( point, flash.origin );
	AnglesToAxis( cg.refdefViewAngles, flash.axis );

	trap_R_AddRefEntityToScene( &flash );

	trap_S_StartSound( NULL, ent->number, CHAN_WEAPON, hflakWeaponSnd );
}


//----(SA)
/*
==============
CG_MortarEFX
	Right now mostly copied directly from Raf's MG42 FX, but with the optional addtion of smoke
==============
*/
void CG_MortarEFX( centity_t *cent ) {
	refEntity_t flash;

	if ( cent->currentState.density & 1 ) {
		// smoke
		CG_ParticleImpactSmokePuff( cgs.media.smokePuffShader, cent->currentState.origin );
	}

	if ( cent->currentState.density & 2 ) {
		// light
		trap_R_AddLightToScene( cent->currentState.origin, 200 + ( rand() & 31 ), 1.0, 1.0, 1.0, 0 );

		// muzzle flash
		memset( &flash, 0, sizeof( flash ) );
		flash.renderfx = RF_LIGHTING_ORIGIN;
		flash.hModel = cgs.media.mg42muzzleflash;
		VectorCopy( cent->currentState.origin, flash.origin );
		AnglesToAxis( cg.refdefViewAngles, flash.axis );
		trap_R_AddRefEntityToScene( &flash );
	}
}

//----(SA)	end


// RF
/*
==============
CG_WeaponFireRecoil (visual recoil)
==============
*/
void CG_WeaponFireRecoil( int weapon ) {
	float pitchRecoilAdd, pitchAdd;
	float yawRandom;
	vec3_t recoil;
	pitchRecoilAdd = 0;
	pitchAdd = 0;
	yawRandom = 0;

	if ( cg_thirdPerson.integer ) {
		return;
	}
	
	switch ( weapon ) {
	case WP_LUGER:
	case WP_SILENCER:
	case WP_COLT:
	case WP_TT33:
	case WP_AKIMBO:
	case WP_DUAL_TT33:
	   yawRandom = 0.5;
	   pitchRecoilAdd = 2;
	   pitchAdd = 1;
	break;
	case WP_REVOLVER:
	case WP_HDM:
	    pitchAdd = 1;
	    yawRandom = 0.5;
    break;
	case WP_MAUSER:
	case WP_DELISLE:
	case WP_MOSIN:
	case WP_GARAND:
	case WP_G43:
	case WP_M1941:
	case WP_M1GARAND:
		pitchAdd = 1;
		yawRandom = 1; 
	    pitchRecoilAdd = 1.5;   
	break;
	case WP_SNIPERRIFLE:
	case WP_SNOOPERSCOPE:
	case WP_DELISLESCOPE:
	case WP_M1941SCOPE:
		pitchAdd = 0.8;
	break;
	case WP_MP40:
	case WP_MP34:
	case WP_PPSH:
	case WP_THOMPSON:
	case WP_STEN:
		pitchAdd = 1;
		pitchRecoilAdd = 1;  
		yawRandom = 1;
	break;
	case WP_FG42SCOPE:
		pitchAdd = 0.8;
	break;
	case WP_FG42:
	case WP_BAR:
	case WP_MP44:
		pitchAdd = 1;
		pitchRecoilAdd = 1;   
		yawRandom = 1;  
	break;
	case WP_M97:
	case WP_AUTO5:
		pitchRecoilAdd = 1;
		pitchAdd = 8 + rand() % 3;
		yawRandom = 2;
		pitchAdd *= 0.5;
		yawRandom *= 0.5;
	break;
	case WP_MG42M:
	case WP_BROWNING:
		pitchRecoilAdd = pow(random(), 8) * (10 + VectorLength(cg.snap->ps.velocity) / 5);
		pitchAdd = 1 + rand() % 3;
		yawRandom = 1;
		pitchRecoilAdd *= 0.5;
		pitchAdd *= 0.5;
		yawRandom *= 0.5;
	break;
	case WP_PANZERFAUST:
		CG_StartShakeCamera( 0.05, 700, cg.snap->ps.origin, 100 );
		break;
	case WP_VENOM:
		pitchRecoilAdd = pow( random(),8 ) * ( 5 + VectorLength( cg.snap->ps.velocity ) / 5 );
		pitchAdd = ( rand() % 5 ) - 2;
		yawRandom = 1;
		pitchRecoilAdd *= 0.5;
		pitchAdd *= 0.5;
		yawRandom *= 0.5;
	break;
	default:
		return;
	}
	// calc the recoil
	recoil[YAW] = crandom() * yawRandom;
	recoil[ROLL] = -recoil[YAW];    // why not
	recoil[PITCH] = -pitchAdd;
	// scale it up a bit (easier to modify this while tweaking)
	VectorScale( recoil, 30, recoil );
	// set the recoil
	VectorCopy( recoil, cg.kickAVel );
	// set the recoil
	cg.recoilPitch -= pitchRecoilAdd;
}


/*
================
CG_FireWeapon

Caused by an EV_FIRE_WEAPON event

================
*/
void CG_FireWeapon( centity_t *cent, int event ) {
	entityState_t *ent;
	int c;
	weaponInfo_t    *weap;
	sfxHandle_t     *firesound;
	sfxHandle_t     *fireEchosound;

	ent = &cent->currentState;

	// Rafael - mg42
//	if ( (cent->currentState.clientNum == cg.snap->ps.clientNum && cg.snap->ps.persistant[PERS_HWEAPON_USE] ) ||
//		 (cent->currentState.clientNum != cg.snap->ps.clientNum && (cent->currentState.eFlags & EF_MG42_ACTIVE) ) )
	if ( ( cent->currentState.clientNum == cg.snap->ps.clientNum && cg.snap->ps.persistant[PERS_HWEAPON_USE] ) ||
		 ( cent->currentState.eFlags & EF_MG42_ACTIVE ) ) {
		if ( cg.snap->ps.gunfx ) {
			return;
		}

		trap_S_StartSound( NULL, cent->currentState.number, CHAN_WEAPON, hWeaponSnd );
		//trap_S_StartSound( NULL, ent->number, CHAN_WEAPON, hWeaponSnd );
		if ( cg_brassTime.integer > 0 ) {
			CG_MachineGunEjectBrass( cent );
		}

		cent->muzzleFlashTime = cg.time;

		return;
	}

	if ( ent->weapon == WP_NONE ) {
		return;
	}
	if ( ent->weapon >= WP_NUM_WEAPONS ) {
		CG_Error( "CG_FireWeapon: ent->weapon >= WP_NUM_WEAPONS" );
		return;
	}
	weap = &cg_weapons[ ent->weapon ];

	cg.lastFiredWeapon = ent->weapon;   //----(SA)	added

	// mark the entity as muzzle flashing, so when it is added it will
	// append the flash to the weapon model
	cent->muzzleFlashTime = cg.time;

	// RF, kick angles
	if ( ent->number == cg.snap->ps.clientNum ) {
		CG_WeaponFireRecoil( ent->weapon );
	}

	// lightning gun only does this this on initial press
	if ( ent->weapon == WP_FLAMETHROWER ) {
		if ( cent->pe.lightningFiring ) {
			return;
		}
	} else if (   ent->weapon == WP_GRENADE_LAUNCHER ||
				  ent->weapon == WP_GRENADE_PINEAPPLE ||
				  ent->weapon == WP_DYNAMITE ||
				  ent->weapon == WP_AIRSTRIKE ||
				  ent->weapon == WP_POISONGAS || 
				  ent->weapon == WP_POISONGAS_MEDIC ||
				  ent->weapon == WP_DYNAMITE_ENG ) { 
		if ( ent->apos.trBase[0] > 0 ) { // underhand
			return;
		}
	}

	 if ( ent->weapon == WP_M7 ) {
		if ( ent->clientNum == cg.snap->ps.clientNum ) {
			cg.weaponSelect = WP_M1GARAND;
		}
	}

	// play quad sound if needed
	if ( cent->currentState.powerups & ( 1 << PW_QUAD ) ) {
		trap_S_StartSound( NULL, cent->currentState.number, CHAN_ITEM, cgs.media.quadSound );
	}
	
	if ( ( cent->currentState.event & ~EV_EVENT_BITS ) == EV_FIRE_WEAPON_LASTSHOT ) {
		firesound = &weap->lastShotSound[0];
		fireEchosound = &weap->flashEchoSound[0];

		// try to use the lastShotSound, but don't assume it's there.
		// if a weapon without the sound calls it, drop back to regular fire sound

		for ( c = 0; c < 4; c++ ) {
			if ( !firesound[c] ) {
				break;
			}
		}
		if ( !c ) {
			if (cent->currentState.aiChar) {
			   firesound = &weap->flashSoundAi[0];
			} else {
			   firesound = &weap->flashSound[0];
			}
			fireEchosound = &weap->flashEchoSound[0];
		}
	} else {
			if (cent->currentState.aiChar) {
			   firesound = &weap->flashSoundAi[0];
			} else {
			   firesound = &weap->flashSound[0];
			}
		fireEchosound = &weap->flashEchoSound[0];
	}


	// play a sound
	for ( c = 0 ; c < 4 ; c++ ) {
		if ( !firesound[c] ) {
			break;
		}
	}
	if ( c > 0 ) {
		c = rand() % c;
		if ( firesound[c] ) {
			trap_S_StartSound( NULL, ent->number, CHAN_WEAPON, firesound[c] );

			if ( fireEchosound && fireEchosound[c] ) { // check for echo
				centity_t   *cent;
				vec3_t porg, gorg, norm;    // player/gun origin
				float gdist;

				cent = &cg_entities[ent->number];
				VectorCopy( cent->currentState.pos.trBase, gorg );
				VectorCopy( cg.refdef.vieworg, porg );
				VectorSubtract( gorg, porg, norm );
				gdist = VectorNormalize( norm );
				if ( gdist > SOUND_FAR_ECHO_DISTANCE && gdist < SOUND_MAX_WEAPON_DISTANCE ) {   // temp dist.  TODO: use numbers that are weapon specific // RealRTCW was 4096
					// use gorg as the new sound origin
					VectorMA( cg.refdef.vieworg, 64, norm, gorg );    // sound-on-a-stick
					trap_S_StartSound( gorg, ent->number, CHAN_WEAPON, fireEchosound[c] );
				}
			}
		}
	}

	if (cent->currentState.aiChar) {
		return;
	}

	// do brass ejection with special delays for some weapons
	if ( weap->ejectBrassFunc && cg_brassTime.integer > 0 ) {
		   if (cg.predictedPlayerState.ammoclip[cent->currentState.weapon] == 0) {
			   weap->ejectBrassFunc( cent, ammoTable[cent->currentState.weapon].brassDelayEmpty );
		   } else {
			   weap->ejectBrassFunc( cent, ammoTable[cent->currentState.weapon].brassDelay );
		   }
	    }
}


// Ridah
/*
=================
CG_AddSparks
=================
*/
void CG_AddSparks( vec3_t origin, vec3_t dir, int speed, int duration, int count, float randScale ) {
	localEntity_t   *le;
	refEntity_t     *re;
	vec3_t velocity;
	int i;

	for ( i = 0; i < count; i++ ) {
		le = CG_AllocLocalEntity();
		re = &le->refEntity;

		VectorSet( velocity, dir[0] + crandom() * randScale, dir[1] + crandom() * randScale, dir[2] + crandom() * randScale );
		VectorScale( velocity, (float)speed, velocity );

		le->leType = LE_SPARK;
		le->startTime = cg.time;
		le->endTime = le->startTime + duration - (int)( 0.5 * random() * duration );
		le->lastTrailTime = cg.time;

		VectorCopy( origin, re->origin );
		AxisCopy( axisDefault, re->axis );

		le->pos.trType = TR_GRAVITY_LOW;
		VectorCopy( origin, le->pos.trBase );
		VectorMA( le->pos.trBase, 2 + random() * 4, dir, le->pos.trBase );
		VectorCopy( velocity, le->pos.trDelta );
		le->pos.trTime = cg.time;

		le->refEntity.customShader = cgs.media.sparkParticleShader;

		le->bounceFactor = 0.9;

//		le->leBounceSoundType = LEBS_BLOOD;
//		le->leMarkType = LEMT_BLOOD;
	}
}
/*
=================
CG_AddBulletParticles
=================
*/
void CG_AddBulletParticles( vec3_t origin, vec3_t dir, int speed, int duration, int count, float randScale ) {
//	localEntity_t	*le;
//	refEntity_t		*re;
	vec3_t velocity, pos;
	int i;
/*
	// add the falling streaks
	for (i=0; i<count/3; i++) {
		le = CG_AllocLocalEntity();
		re = &le->refEntity;

		VectorSet( velocity, dir[0] + crandom()*randScale, dir[1] + crandom()*randScale, dir[2] + crandom()*randScale );
		VectorScale( velocity, (float)speed*3, velocity );

		le->leType = LE_SPARK;
		le->startTime = cg.time;
		le->endTime = le->startTime + duration - (int)(0.5 * random() * duration);
		le->lastTrailTime = cg.time;

		VectorCopy( origin, re->origin );
		AxisCopy( axisDefault, re->axis );

		le->pos.trType = TR_GRAVITY;
		VectorCopy( origin, le->pos.trBase );
		VectorMA( le->pos.trBase, 2 + random()*4, dir, le->pos.trBase );
		VectorCopy( velocity, le->pos.trDelta );
		le->pos.trTime = cg.time;

		le->refEntity.customShader = cgs.media.bulletParticleTrailShader;
//		le->refEntity.customShader = cgs.media.sparkParticleShader;

		le->bounceFactor = 0.9;

//		le->leBounceSoundType = LEBS_BLOOD;
//		le->leMarkType = LEMT_BLOOD;
	}
*/
	// add the falling particles
	for ( i = 0; i < count; i++ ) {

		VectorSet( velocity, dir[0] + crandom() * randScale, dir[1] + crandom() * randScale, dir[2] + crandom() * randScale );
		VectorScale( velocity, (float)speed, velocity );

		VectorCopy( origin, pos );
		VectorMA( pos, 2 + random() * 4, dir, pos );

		CG_ParticleBulletDebris( pos, velocity, 300 + rand() % 300 );

	}
}

//----(SA)	from MP
/*
=================
CG_AddDirtBulletParticles
=================
*/
void CG_AddDirtBulletParticles( vec3_t origin, vec3_t dir, int speed, int duration, int count, float randScale,
								float width, float height, float alpha, char *shadername ) { // JPW NERVE
	vec3_t velocity, pos;
	int i;

	// add the big falling particle
	VectorSet( velocity, 0, 0, (float)speed );

	VectorCopy( origin, pos );

// JPW NERVE
	CG_ParticleDirtBulletDebris_Core( pos, velocity, duration, width,height, alpha, shadername ); //600 + rand()%300 ); // keep central one
	for ( i = 0; i < count; i++ ) {
		VectorSet( velocity, dir[0] * crandom() * speed * randScale,
				   dir[1] * crandom() * speed * randScale, dir[2] * random() * speed );
		CG_ParticleDirtBulletDebris_Core( pos, velocity, duration + ( rand() % ( duration >> 1 ) ), // dur * 0.5, but int
										  width,height, alpha, shadername );
	}
}
//----(SA)	end

/*
=================
CG_AddDebris
=================
*/
void CG_AddDebris( vec3_t origin, vec3_t dir, int speed, int duration, int count ) {
	localEntity_t   *le;
	refEntity_t     *re;
	vec3_t velocity, unitvel;
	float timeAdd;
	int i;

	for ( i = 0; i < count; i++ ) {
		le = CG_AllocLocalEntity();
		re = &le->refEntity;

		VectorSet( unitvel, dir[0] + crandom() * 0.9, dir[1] + crandom() * 0.9, fabs( dir[2] ) > 0.5 ? dir[2] * ( 0.2 + 0.8 * random() ) : random() * 0.6 );
		VectorScale( unitvel, (float)speed + (float)speed * 0.5 * crandom(), velocity );

		le->leType = LE_DEBRIS;
		le->startTime = cg.time;
		le->endTime = le->startTime + duration + (int)( (float)duration * 0.8 * crandom() );
		le->lastTrailTime = cg.time;

		VectorCopy( origin, re->origin );
		AxisCopy( axisDefault, re->axis );

		le->pos.trType = TR_GRAVITY_LOW;
		VectorCopy( origin, le->pos.trBase );
		VectorCopy( velocity, le->pos.trDelta );
		le->pos.trTime = cg.time;

		timeAdd = 10.0 + random() * 40.0;
		BG_EvaluateTrajectory( &le->pos, cg.time + (int)timeAdd, le->pos.trBase );

		le->bounceFactor = 0.5;

//		if (!rand()%2)
//			le->effectWidth = 0;	// no flame
//		else
		le->effectWidth = 5 + random() * 5;

//		if (rand()%3)
		le->effectFlags |= 1;       // smoke trail


//		le->leBounceSoundType = LEBS_BLOOD;
//		le->leMarkType = LEMT_BLOOD;
	}
}
// done.


/*
==============
CG_WaterRipple
==============
*/
void CG_WaterRipple( qhandle_t shader, vec3_t loc, vec3_t dir, int size, int lifetime ) {
	localEntity_t   *le;
	refEntity_t     *re;

	le = CG_AllocLocalEntity();
	le->leType          = LE_SCALE_FADE;
	le->leFlags         = LEF_PUFF_DONT_SCALE;

	le->startTime       = cg.time;
	le->endTime         = cg.time + lifetime;
	le->lifeRate        = 1.0 / ( le->endTime - le->startTime );

	re = &le->refEntity;
	VectorCopy( loc, re->origin );
	re->shaderTime      = cg.time / 1000.0f;
	re->reType          = RT_SPLASH;
	re->radius          = size;
	re->customShader    = shader;
	re->shaderRGBA[0]   = 0xff;
	re->shaderRGBA[1]   = 0xff;
	re->shaderRGBA[2]   = 0xff;
	re->shaderRGBA[3]   = 0xff;
	le->color[3]        = 1.0;
}

/*
=================
CG_MissileHitWall

Caused by an EV_MISSILE_MISS event, or directly by local bullet tracing

ClientNum is a dummy field used to define what sort of effect to spawn
=================
*/
void CG_MissileHitWall( int weapon, int clientNum, vec3_t origin, vec3_t dir, int surfFlags ) { //	(SA) modified to send missilehitwall surface parameters
	qhandle_t mod;
	qhandle_t mark;
	qhandle_t shader;
	sfxHandle_t sfx, sfx2;
	float radius;
	float light;
	float sfx2range = 0; 
	vec3_t lightColor, tmpv, tmpv2;
	trace_t trace;
	localEntity_t   *le;
	int r;
	qboolean alphaFade = qfalse;
	qboolean isSprite;
	int duration;
	int lightOverdraw;
	vec3_t sprOrg;
	vec3_t sprVel = { 0 };
	int i,j;

	float shakeAmt;
	int shakeDur, shakeRad;
	shakeAmt = 0;
	shakeDur = shakeRad = 0;

	mark = 0;
	radius = 32;
	sfx = 0;
	sfx2 = 0;
	mod = 0;
	shader = 0;
	light = 0;
	lightColor[0] = 1;
	lightColor[1] = 1;
	lightColor[2] = 0;
	lightOverdraw = 0;

	// set defaults
	isSprite = qfalse;
	duration = 600;

	if ( surfFlags & SURF_SKY ) {
		return;
	}

	switch ( weapon ) {
	case WP_KNIFE:
		sfx     = cgs.media.sfx_knifehit[4];    // different values for different types (stone/metal/wood/etc.)
		mark    = cgs.media.bulletMarkShader;
		radius  = 1 + rand() % 2;

		CG_AddBulletParticles( origin, dir,
							   20,      // speed
							   800,     // duration
							   3 + rand() % 6, // count
							   1.0 );   // rand scale
		break;

	case WP_LUGER:
	case WP_AKIMBO: 
	case WP_DUAL_TT33:
	case WP_COLT:
	case WP_MAUSER:
	case WP_DELISLE:
	case WP_DELISLESCOPE:
	case WP_M1941SCOPE:
	case WP_GARAND:
	case WP_SNIPERRIFLE:
	case WP_SNOOPERSCOPE:
	case WP_MP40:
	case WP_MP34:
	case WP_TT33:
	case WP_HDM:
	case WP_PPSH:
	case WP_MOSIN:
	case WP_G43:
	case WP_M1GARAND:
	case WP_BAR:
	case WP_MP44:
	case WP_MG42M:
	case WP_BROWNING:
	case WP_M97:
	case WP_AUTO5:
	case WP_REVOLVER:
	case WP_FG42:
	case WP_FG42SCOPE:
	case WP_THOMPSON:
	case WP_STEN:
	case WP_SILENCER:
	case WP_VENOM:
	case WP_M1941:

		r = rand() & 31;

		if ( r == 3 ) {
			sfx = cgs.media.sfx_ric1;
		} else if ( r == 2 ) {
			sfx = cgs.media.sfx_ric2;
		} else if ( r == 1 ) {
			sfx = cgs.media.sfx_ric3;
		}

		// clientNum is a dummy field used to define what sort of effect to spawn

		if ( !clientNum ) {
			// RF, why is this here? we need sparks if clientNum = 0, used for warzombie
			// if ( sfx )
			CG_AddSparks( origin, dir,
						  350,      // speed
						  200,      // duration
						  15 + rand() % 7,  // count
						  0.2 );    // rand scale
		} else if ( clientNum == 1 )      { // just do a little smoke puff
			vec3_t d, o;
			VectorMA( origin, 12, dir, o );
			VectorScale( dir, 7, d );
			d[2] += 16;

			// just snow
			if ( surfFlags & SURF_SNOW ) {
				CG_AddDirtBulletParticles( origin, dir,
										   120, // speed
										   900, // duration
										   3, // count
										   0.6f,
										   20,
										   4,
										   0.3f,
										   "water_splash" ); // rand scale
			}

			// grass/gravel
			if ( surfFlags & ( SURF_GRASS | SURF_GRAVEL ) ) {
				CG_AddDirtBulletParticles( origin, dir,
										   190, // speed
										   800, // duration
										   3, // count
										   0.1,
										   60,
										   10,
										   0.5,
										   "dirt_splash" ); // rand scale
			} else {
				CG_ParticleImpactSmokePuff( cgs.media.smokeParticleShader, o );

				// some debris particles
				CG_AddBulletParticles( origin, dir,
									   20, // speed
									   800, // duration
									   3 + rand() % 6, // count
									   1.0 ); // rand scale

				// just do a little one
				if ( sfx && ( rand() % 3 == 0 ) ) {
					CG_AddSparks( origin, dir,
								  450,      // speed
								  300,      // duration
								  3 + rand() % 3,   // count
								  0.5 );    // rand scale
				}
			}
		} else if ( clientNum == 2 )      {
			sfx = 0;
			mark = 0;

			// (SA) needed to do the CG_WaterRipple using a localent since I needed the timer reset on the shader for each shot
			CG_WaterRipple( cgs.media.wakeMarkShaderAnim, origin, tv( 0, 0, 1 ), 32, 1000 );

			CG_AddDirtBulletParticles( origin, dir,
									   190, // speed
									   900, // duration
									   5, // count
									   0.5, // rand scale
									   50, // w
									   12, // h
									   0.125, // alpha
									   "water_splash" );

			break;  // (SA) testing

#if 0
			// play a water splash
			mod = cgs.media.waterSplashModel;
			shader = cgs.media.waterSplashShader;
			duration = 250;
#endif
		}

		// Ridah, optimization, only spawn the bullet hole if we are close
		// enough to see it, this way we can leave other marks around a lot
		// longer, since most of the time we can't actually see the bullet holes
// (SA) small modification.  only do this for non-rifles (so you can see your shots hitting when you're zooming with a rifle scope)
		if ( weapon == WP_FG42SCOPE || weapon == WP_SNIPERRIFLE || weapon == WP_SNOOPERSCOPE || weapon == WP_DELISLESCOPE || weapon == WP_M1941SCOPE || ( Distance( cg.refdef.vieworg, origin ) < 384 ) ) {

			if ( clientNum ) {

				// mark and sound can potentially use the surface for override values

				mark = cgs.media.bulletMarkShader;  // default
				alphaFade = qtrue;      // max made the bullet mark alpha (he'll make everything in the game out of 1024 textures, all with alpha blend funcs yet...)
				radius = 1.5f + rand() % 2;   // slightly larger for DM

				if ( surfFlags & SURF_METAL ) {
					sfx = cgs.media.sfx_bullet_metalhit[rand() % 3];
					mark = cgs.media.bulletMarkShaderMetal;
					alphaFade = qtrue;
				} else if ( surfFlags & SURF_WOOD ) {
					sfx = cgs.media.sfx_bullet_woodhit[rand() % 3];
					mark = cgs.media.bulletMarkShaderWood;
					alphaFade = qtrue;
					radius += 1;    // experimenting with different mark sizes per surface


				} else if ( surfFlags & SURF_CERAMIC ) {
					sfx = cgs.media.sfx_bullet_ceramichit[rand() % 3];
					mark = cgs.media.bulletMarkShaderCeramic;
					alphaFade = qtrue;
					radius += 2;    // experimenting with different mark sizes per surface

				} else if ( surfFlags & SURF_GLASS ) {
					sfx = cgs.media.sfx_bullet_glasshit[rand() % 3];
					mark = cgs.media.bulletMarkShaderGlass;
					alphaFade = qtrue;
				} else if ( surfFlags & SURF_GRASS ) {

				} else if ( surfFlags & SURF_GRAVEL ) {

				} else if ( surfFlags & SURF_SNOW ) {

				} else if ( surfFlags & SURF_ROOF ) {

				} else if ( surfFlags & SURF_CARPET ) {

				}

			}
		}
		break;


	case WP_MORTAR:
		sfx = cgs.media.sfx_rockexp;
		mark = cgs.media.burnMarkShader;
		sfx2 = cgs.media.sfx_rockexpDist;
		sfx2range = 1200;
		radius = 64;
		light = 300;
		isSprite = qtrue;
		duration = 1000;
		lightColor[0] = 0.75;
		lightColor[1] = 0.5;
		lightColor[2] = 0.1;

		shakeAmt = 0.15f;
		shakeDur = 600;
		shakeRad = 700;

		VectorScale( dir, 16, sprVel );
		for ( i = 0; i < 5; i++ ) {
			for ( j = 0; j < 3; j++ )
				sprOrg[j] = origin[j] + 64 * dir[j] + 24 * crandom();
			sprVel[2] += rand() % 50;
			CG_ParticleExplosion( "blacksmokeanimb", sprOrg, sprVel, 3500 + rand() % 250, 10, 250 + rand() % 60 );
		}

		VectorMA( origin, 24, dir, sprOrg );
		VectorScale( dir, 64, sprVel );
		CG_ParticleExplosion( "expblue", sprOrg, sprVel, 1000, 20, 300 );
		break;

	case WP_DYNAMITE:
		shader = cgs.media.rocketExplosionShader;
		sfx = cgs.media.sfx_dynamiteexp;
		sfx2 = cgs.media.sfx_dynamiteexpDist;
		sfx2range = 400;
		mark = cgs.media.burnMarkShader;
		radius = 64;
		light = 300;
		isSprite = qtrue;
		duration = 1000;
		lightColor[0] = 0.75;
		lightColor[1] = 0.5;
		lightColor[2] = 0.1;

		shakeAmt = 0.25f;
		shakeDur = 2800;
		shakeRad = 8192;
			for ( i = 0; i < 5; i++ ) {
				for ( j = 0; j < 3; j++ )
					sprOrg[j] = origin[j] + 64 * dir[j] + 24 * crandom();
				sprVel[2] += rand() % 50;
				CG_ParticleExplosion( "blacksmokeanimb", sprOrg, sprVel,
									  3500 + rand() % 250,          // duration
									  10,                           // startsize
									  250 + rand() % 60 );     
									       // endsize
			}
			VectorMA( origin, 16, dir, sprOrg );
			VectorScale( dir, 100, sprVel );

			// trying this one just for now just for variety
			CG_ParticleExplosion( "explode1", sprOrg, sprVel,
								  1200,         // duration
								  9,            // startsize
								  300 );        // endsize

			CG_AddDebris( origin, dir,
						  280,              // speed
						  1400,             // duration
						  7 + rand() % 2 ); // count
		break;
	case WP_DYNAMITE_ENG:
		shader = cgs.media.rocketExplosionShader;
		sfx = cgs.media.sfx_dynamiteexp;
		sfx2 = cgs.media.sfx_dynamiteexpDist;
		sfx2range = 400;
		mark = cgs.media.burnMarkShader;
		radius = 64;
		light = 300;
		isSprite = qtrue;
		duration = 1000;
		lightColor[0] = 0.75;
		lightColor[1] = 0.5;
		lightColor[2] = 0.1;

		shakeAmt = 0.25f;
		shakeDur = 2800;
		shakeRad = 8192;
			for ( i = 0; i < 5; i++ ) {
				for ( j = 0; j < 3; j++ )
					sprOrg[j] = origin[j] + 64 * dir[j] + 24 * crandom();
				sprVel[2] += rand() % 50;
				CG_ParticleExplosion( "blacksmokeanimb", sprOrg, sprVel,
									  3500 + rand() % 250,          // duration
									  10,                           // startsize
									  250 + rand() % 60 );     
									       // endsize
			}
			VectorMA( origin, 16, dir, sprOrg );
			VectorScale( dir, 100, sprVel );

			// trying this one just for now just for variety
			CG_ParticleExplosion( "explode1", sprOrg, sprVel,
								  1200,         // duration
								  9,            // startsize
								  300 );        // endsize

			CG_AddDebris( origin, dir,
						  280,              // speed
						  1400,             // duration
						  7 + rand() % 2 ); // count
		break;

	case WP_GRENADE_LAUNCHER:
	case WP_GRENADE_PINEAPPLE:
	case WP_M7:
		shader = cgs.media.rocketExplosionShader;       // copied from RL
		sfx = cgs.media.sfx_grenexp;
		sfx2 = cgs.media.sfx_grenexpDist;
		sfx2range = 400;
		mark = cgs.media.burnMarkShader;
		radius = 64;
		light = 300;
		isSprite = qtrue;
		duration = 1000;
		lightColor[0] = 0.75;
		lightColor[1] = 0.5;
		lightColor[2] = 0.1;
		shakeAmt = 0.15f;
		shakeDur = 1000;
		shakeRad = 600;

		// Ridah, explosion sprite animation
		VectorMA( origin, 16, dir, sprOrg );
		VectorScale( dir, 100, sprVel );

		if ( CG_PointContents( origin, 0 ) & CONTENTS_WATER ) {
			sfx = cgs.media.sfx_grenexpWater;

			VectorCopy( origin,tmpv );
			tmpv[2] += 10000;

			trap_CM_BoxTrace( &trace, tmpv,origin, NULL, NULL, 0, MASK_WATER );
			CG_WaterRipple( cgs.media.wakeMarkShaderAnim, trace.endpos, dir, 150, 1000 );

			CG_AddDirtBulletParticles( trace.endpos, dir, 400, 900, 15, 0.5, 256,128, 0.125, "water_splash" );
		} else {
			VectorCopy( origin,tmpv );
			tmpv[2] += 20;
			VectorCopy( origin,tmpv2 );
			tmpv2[2] -= 20;
			trap_CM_BoxTrace( &trace,tmpv,tmpv2,NULL,NULL,0,MASK_SHOT );
			if ( trace.surfaceFlags & SURF_GRASS || trace.surfaceFlags & SURF_GRAVEL ) {
				CG_AddDirtBulletParticles( origin, dir, 400, 2000, 10, 0.5, 200,75, 0.25, "dirt_splash" );
			}
			CG_ParticleExplosion( "expblue", sprOrg, sprVel, 700, 20, 160 );
			CG_AddDebris( origin, dir, 280, 1400, 7 + rand() % 2 );
		}
		break;
	case VERYBIGEXPLOSION:
	case WP_PANZERFAUST:
    case WP_AIRSTRIKE:
	case WP_ARTY:
		sfx = cgs.media.sfx_rockexp;
		sfx2 = cgs.media.sfx_rockexpDist;
		sfx2range = 800;
		mark = cgs.media.burnMarkShader;
		radius = 64;
		light = 600;
		isSprite = qtrue;
		duration = 1000;
		lightColor[0] = 0.75;
		lightColor[1] = 0.5;
		lightColor[2] = 0.1;
		// explosion sprite animation
		VectorMA( origin, 24, dir, sprOrg );
		VectorScale( dir, 64, sprVel );
		// cam shake
		if ( weapon == VERYBIGEXPLOSION ) {
			shakeAmt = 0.2f;
			shakeDur = 1000;
			shakeRad = 1200;
		} else {
			shakeAmt = 0.15f;
			shakeDur = 800;
			shakeRad = 1000;
		}
			if ( weapon == VERYBIGEXPLOSION ) {
				CG_ParticleExplosion( "explode1", sprOrg, sprVel, 1200, 20, 300 );
			} else {
				CG_ParticleExplosion( "explode1", sprOrg, sprVel, 1400, 40, 70 );
			}
			// NOTE: these must all have the same duration, so that we are less likely to use a wider range of images per scene
			for ( i = 0; i < 4; i++ ) {
				if ( weapon == VERYBIGEXPLOSION ) {
					for ( j = 0; j < 3; j++ ) sprOrg[j] = origin[j] + 32 * dir[j] + 32 * crandom();
					CG_ParticleExplosion( "explode1", sprOrg, sprVel, 1200, 40, 160 + rand() % 120 );
				} else if ( i < 2 ) {
					for ( j = 0; j < 3; j++ ) sprOrg[j] = origin[j] + 24 * dir[j] + 16 * crandom();
					CG_ParticleExplosion( "explode1", sprOrg, sprVel, 1400, 15, 40 + rand() % 30 );
				}
			}
			// Ridah, throw some debris
			CG_AddDebris( origin, dir,
						  120,      // speed
						  2000,  //350,	// duration
						  // 15 + rand()%5 );	// count
						  7 + rand() % 2 ); // count
		break;

	default:
	case WP_FLAMETHROWER:
		return;
		break;
	}

	if ( sfx ) {
		trap_S_StartSound( origin, ENTITYNUM_WORLD, CHAN_AUTO, sfx );
	}

		if ( sfx2 ) {  // distant sounds for weapons with a broadcast fire sound (so you /always/ hear dynamite explosions)
		vec3_t porg, gorg, norm;    // player/gun origin
		float gdist;

		VectorCopy( origin, gorg );
		VectorCopy( cg.refdef_current->vieworg, porg );
		VectorSubtract( gorg, porg, norm );
		gdist = VectorNormalize( norm );
		if ( gdist > 1200 && gdist < 8000 ) {  // 1200 is max cam shakey dist (2*600) use gorg as the new sound origin
			VectorMA( cg.refdef_current->vieworg, sfx2range, norm, gorg ); // JPW NERVE non-distance falloff makes more sense; sfx2range was gdist*0.2
			// sfx2range is variable to give us minimum volume control different explosion sizes (see mortar, panzerfaust, and grenade)
			trap_S_StartSoundEx( gorg, ENTITYNUM_WORLD, CHAN_WEAPON, sfx2, SND_NOCUT );
		}
	}

	//
	// camera shake
	//
	if ( shakeAmt ) {
		CG_StartShakeCamera( shakeAmt, shakeDur, origin, shakeRad );
	}


	//
	// create the explosion
	//
	if ( mod ) {
		le = CG_MakeExplosion( origin, dir,
							   mod, shader,
							   duration, isSprite );
		le->light = light;
		// Ridah
		le->lightOverdraw = lightOverdraw;
		VectorCopy( lightColor, le->lightColor );
	}

	//
	// impact mark
	//
	if ( mark ) {
		CG_ImpactMark( mark, origin, dir, random() * 360, 1,1,1,1, alphaFade, radius, qfalse, -1 );
	}
}

/*
==============
CG_MissileHitWallSmall
==============
*/
void CG_MissileHitWallSmall( int weapon, int clientNum, vec3_t origin, vec3_t dir ) {
	qhandle_t mod;
	qhandle_t mark;
	qhandle_t shader;
	sfxHandle_t sfx;
	float radius;
	float light;
	vec3_t lightColor;
	localEntity_t   *le;
//	int				r;
	qboolean alphaFade;
	qboolean isSprite;
	int duration;
	// Ridah
	int lightOverdraw;
	vec3_t sprOrg;
	vec3_t sprVel;
//	int				i,j;

	mod = 0;
	lightColor[0] = 1;
	lightColor[1] = 1;
	lightColor[2] = 0;
	// Ridah
	lightOverdraw = 0;

	// set defaults
	shader = cgs.media.rocketExplosionShader;       // copied from RL
	sfx = cgs.media.sfx_barrelexp;
	mark = cgs.media.burnMarkShader;
	radius = 64;
	light = 300;
	isSprite = qtrue;
	duration = 1000;
	lightColor[0] = 0.75;
	lightColor[1] = 0.5;
	lightColor[2] = 0.1;

	// Ridah, explosion sprite animation
	VectorMA( origin, 16, dir, sprOrg );
	VectorScale( dir, 64, sprVel );

	CG_ParticleExplosion( "explode1", sprOrg, sprVel, 600, 6, 50 );

	// Ridah, throw some debris
	CG_AddDebris( origin, dir,
				  280,          // speed
				  1400,         // duration
				  // 15 + rand()%5 );	// count
				  7 + rand() % 2 );     // count

	if ( sfx ) {
		trap_S_StartSound( origin, ENTITYNUM_WORLD, CHAN_AUTO, sfx );
	}

	//
	// create the explosion
	//
	if ( mod ) {
		le = CG_MakeExplosion( origin, dir,
							   mod, shader,
							   duration, isSprite );
		le->light = light;
		// Ridah
		le->lightOverdraw = lightOverdraw;
		VectorCopy( lightColor, le->lightColor );
	}

	//
	// impact mark
	//
	alphaFade = ( mark == cgs.media.energyMarkShader );   // plasma fades alpha, all others fade color
	// CG_ImpactMark( mark, origin, dir, random()*360, 1,1,1,1, alphaFade, radius, qfalse, 60000 );
	CG_ImpactMark( mark, origin, dir, random() * 360, 1,1,1,1, alphaFade, radius, qfalse, INT_MAX );

	CG_StartShakeCamera( 0.05, 300, origin, 300 );
}

/*
=================
CG_MissileHitPlayer
=================
*/
void CG_MissileHitPlayer( centity_t *cent, int weapon, vec3_t origin, vec3_t dir, int entityNum ) {
	int i;

	CG_Bleed( origin, entityNum );

	// some weapons will make an explosion with the blood, while
	// others will just make the blood
	switch ( weapon ) {
		// knives just make the flesh hit sound.  no other effects
	case WP_KNIFE:
		i = rand() % 4;
		if ( cgs.media.sfx_knifehit[i] ) {
			trap_S_StartSound( origin, cent->currentState.number, CHAN_WEAPON, cgs.media.sfx_knifehit[i] );
		}

		if ( cent->currentState.number == cg.snap->ps.clientNum ) {
			CG_StartShakeCamera( 0.03, 500, origin, 100 );
		}
		break;

	case WP_GRENADE_LAUNCHER:
	case WP_PANZERFAUST:
		// this shake is /on top/ of the shake from the impact (done in CG_MissileHitWall)
		CG_StartShakeCamera( 0.1, 500, origin, 100 );
		CG_MissileHitWall( weapon, 0, origin, dir, 0 );     //	(SA) modified to send missilehitwall surface parameters
		break;
	default:
		break;
	}
}



/*
============================================================================

VENOM GUN TRACING

============================================================================
*/

/*
================
CG_VenomPellet
================
*/
static void CG_VenomPellet( vec3_t start, vec3_t end, int skipNum ) {
}


//----(SA)	all changes to venom below should be mine
#define DEFAULT_VENOM_COUNT 10
//#define DEFAULT_VENOM_SPREAD 20
//#define DEFAULT_VENOM_SPREAD 400
#define DEFAULT_VENOM_SPREAD 700

/*
================
CG_VenomPattern

Perform the same traces the server did to locate the
hit splashes (FIXME: random seed isn't synced anymore)

  (SA)	right now this is random like a shotgun.  I want to make it more
		organized so that the pattern is more of a circle (with some degree of randomness)
================
*/
static void CG_VenomPattern( vec3_t origin, vec3_t origin2, int otherEntNum ) {
	int i;
	float r, u;
	vec3_t end;
	vec3_t forward, right, up;

	// derive the right and up vectors from the forward vector, because
	// the client won't have any other information
	VectorNormalize2( origin2, forward );
	PerpendicularVector( right, forward );
	CrossProduct( forward, right, up );

	// generate the "random" spread pattern
	for ( i = 0 ; i < DEFAULT_VENOM_COUNT ; i++ ) {
		r = crandom() * DEFAULT_VENOM_SPREAD;
		u = crandom() * DEFAULT_VENOM_SPREAD;
		VectorMA( origin, 8192, forward, end );
		VectorMA( end, r, right, end );
		VectorMA( end, u, up, end );

		CG_VenomPellet( origin, end, otherEntNum );
	}
}

/*
==============
CG_VenomFire
==============
*/
void CG_VenomFire( entityState_t *es, qboolean fullmode ) {
	vec3_t v;
	int contents;

	VectorSubtract( es->origin2, es->pos.trBase, v );
	VectorNormalize( v );
	VectorScale( v, 32, v );
	VectorAdd( es->pos.trBase, v, v );
	if ( cgs.glconfig.hardwareType != GLHW_RAGEPRO ) {
		// ragepro can't alpha fade, so don't even bother with smoke
		vec3_t up;

		contents = CG_PointContents( es->pos.trBase, 0 );
		if ( !( contents & CONTENTS_WATER ) ) {
			VectorSet( up, 0, 0, 32 );
			if ( fullmode ) {
				CG_SmokePuff( v, up, 24, 1, 1, 1, 0.33, 1200, cg.time, 0, 0, cgs.media.shotgunSmokePuffShader );    // LEF_PUFF_DONT_SCALE
			}

		}
	}
	if ( fullmode ) {
		CG_VenomPattern( es->pos.trBase, es->origin2, es->otherEntityNum );
	}
}

/*
============================================================================

BULLETS

============================================================================
*/

/*
===============
CG_SpawnTracer
===============
*/
void CG_SpawnTracer( int sourceEnt, vec3_t pstart, vec3_t pend ) {
	localEntity_t   *le;
	float dist;
	vec3_t dir, ofs;
	orientation_t or;
	vec3_t start, end;

	VectorCopy( pstart, start );
	VectorCopy( pend, end );

	VectorSubtract( end, start, dir );
	dist = VectorNormalize( dir );

	if ( dist < 2.0 * cg_tracerLength.value ) {
		return; // segment isnt long enough, dont bother

	}
	if ( sourceEnt < cgs.maxclients ) {
		// for visual purposes, find the actual tag_weapon for this client
		// and offset the start and end accordingly
		if ( cg_entities[sourceEnt].currentState.eFlags & EF_MG42_ACTIVE ) {   // mounted
			start[2] -= 32; // (SA) hack to get the tracer down below the barrel FIXME: do properly
		} else {
			if ( CG_GetWeaponTag( sourceEnt, "tag_flash", &or ) ) {
				VectorSubtract( or.origin, start, ofs );
				if ( VectorLength( ofs ) < 64 ) {
					VectorAdd( start, ofs, start );
				}
			}
		}
	}

	// subtract the length of the tracer from the end point, so we dont go through the end point
	VectorMA( end, -cg_tracerLength.value, dir, end );
	dist = VectorDistance( start, end );

	le = CG_AllocLocalEntity();
	le->leType = LE_MOVING_TRACER;
	le->startTime = cg.time - ( cg.frametime ? ( rand() % cg.frametime ) / 2 : 0 );
	le->endTime = le->startTime + 1000.0 * dist / cg_tracerSpeed.value;

	le->pos.trType = TR_LINEAR;
	le->pos.trTime = le->startTime;
	VectorCopy( start, le->pos.trBase );
	VectorScale( dir, cg_tracerSpeed.value, le->pos.trDelta );
}

/*
===============
CG_DrawTracer
===============
*/
void CG_DrawTracer( vec3_t start, vec3_t finish ) {
	vec3_t forward, right;
	polyVert_t verts[4];
	vec3_t line;

	VectorSubtract( finish, start, forward );

	line[0] = DotProduct( forward, cg.refdef.viewaxis[1] );
	line[1] = DotProduct( forward, cg.refdef.viewaxis[2] );

	VectorScale( cg.refdef.viewaxis[1], line[1], right );
	VectorMA( right, -line[0], cg.refdef.viewaxis[2], right );
	VectorNormalize( right );

	VectorMA( finish, cg_tracerWidth.value, right, verts[0].xyz );
	verts[0].st[0] = 1;
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
	verts[2].st[0] = 0;
	verts[2].st[1] = 0;
	verts[2].modulate[0] = 255;
	verts[2].modulate[1] = 255;
	verts[2].modulate[2] = 255;
	verts[2].modulate[3] = 255;

	VectorMA( start, cg_tracerWidth.value, right, verts[3].xyz );
	verts[3].st[0] = 0;
	verts[3].st[1] = 1;
	verts[3].modulate[0] = 255;
	verts[3].modulate[1] = 255;
	verts[3].modulate[2] = 255;
	verts[3].modulate[3] = 255;

	trap_R_AddPolyToScene( cgs.media.tracerShader, 4, verts );
}

/*
===============
CG_Tracer
===============
*/
void CG_Tracer( vec3_t source, vec3_t dest, int sparks ) {
	float len, begin, end;
	vec3_t start, finish;
	vec3_t forward;

	// tracer
	VectorSubtract( dest, source, forward );
	len = VectorNormalize( forward );

	// start at least a little ways from the muzzle
	if ( len < 100 && !sparks ) {
		return;
	}
	begin = 50 + random() * ( len - 60 );
	end = begin + cg_tracerLength.value;
	if ( end > len ) {
		end = len;
	}
	VectorMA( source, begin, forward, start );
	VectorMA( source, end, forward, finish );

	CG_DrawTracer( start, finish );
}


/*
======================
CG_CalcMuzzlePoint
======================
*/
static qboolean CG_CalcMuzzlePoint( int entityNum, vec3_t muzzle ) {
	vec3_t forward, right, up;
	centity_t   *cent;

	if ( entityNum == cg.snap->ps.clientNum ) {
		VectorCopy( cg.snap->ps.origin, muzzle );
		muzzle[2] += cg.snap->ps.viewheight;
		AngleVectors( cg.snap->ps.viewangles, forward, NULL, NULL );
		VectorMA( muzzle, 14, forward, muzzle );
		return qtrue;
	}

	cent = &cg_entities[entityNum];

	VectorCopy( cent->currentState.pos.trBase, muzzle );

	AngleVectors( cent->currentState.apos.trBase, forward, right, up );
	muzzle[2] += DEFAULT_VIEWHEIGHT;

	VectorMA( muzzle, 14, forward, muzzle );

	return qtrue;
}

/*
======================
CG_Bullet

Renders bullet effects.
======================
*/
void CG_Bullet( vec3_t end, int sourceEntityNum, vec3_t normal, qboolean flesh, int fleshEntityNum, qboolean wolfkick, int otherEntNum2 ) {
	trace_t trace;
	int sourceContentType = 0, destContentType = 0; // TTimo: init
	vec3_t dir;
	vec3_t start, trend, tmp;      // JPW
	static int lastBloodSpat;

	// if the shooter is currently valid, calc a source point and possibly
	// do trail effects
	if ( sourceEntityNum >= 0 && cg_tracerChance.value > 0 ) {
		if ( CG_CalcMuzzlePoint( sourceEntityNum, start ) ) {
			sourceContentType = CG_PointContents( start, 0 );
			destContentType = CG_PointContents( end, 0 );

			// do a complete bubble trail if necessary
			if ( ( sourceContentType == destContentType ) && ( sourceContentType & CONTENTS_WATER ) ) {
				CG_BubbleTrail( start, end, .5, 8 );
			}
			// bubble trail from water into air
			else if ( ( sourceContentType & CONTENTS_WATER ) ) {
				trap_CM_BoxTrace( &trace, end, start, NULL, NULL, 0, CONTENTS_WATER );
				CG_BubbleTrail( start, trace.endpos, .5, 8 );
			}
			// bubble trail from air into water
			else if ( ( destContentType & CONTENTS_WATER ) ) {
				// only add bubbles if effect is close to viewer
				if ( Distance( cg.snap->ps.origin, end ) < 1024 ) {
					trap_CM_BoxTrace( &trace, start, end, NULL, NULL, 0, CONTENTS_WATER );
					CG_BubbleTrail( end, trace.endpos, .5, 8 );
				}
			}

			// if not flesh, then do a moving tracer
			if ( flesh ) {
				// draw a tracer
				if ( !wolfkick && random() < cg_tracerChance.value ) {
					CG_Tracer( start, end, 0 );
				}
			} else {    // (not flesh)
				if ( otherEntNum2 >= 0 && otherEntNum2 != ENTITYNUM_NONE ) {
					CG_SpawnTracer( otherEntNum2, start, end );
				} else {
					CG_SpawnTracer( sourceEntityNum, start, end );
				}
			}
		}
	}

	// impact splash and mark
	if ( flesh ) {
		vec3_t origin;
		int aiType;

		aiType = cg_entities[fleshEntityNum].currentState.aiChar;

		if ( fleshEntityNum < MAX_CLIENTS ) {
			CG_Bleed( end, fleshEntityNum );
		}

		// play the bullet hit flesh sound
		// HACK, if this is not us getting hit, make it quieter
		if ( fleshEntityNum == cg.snap->ps.clientNum ) {

			// (SA) TODO: for metal guys, make metal a flag rather than an aitype check?
			if ( aiType == AICHAR_PROTOSOLDIER ||
			     aiType == AICHAR_SUPERSOLDIER_LAB ||
				 aiType == AICHAR_SUPERSOLDIER ) {
				CG_SoundPlayIndexedScript( cgs.media.bulletHitFleshMetalScript, NULL, fleshEntityNum );
			} else {
				CG_SoundPlayIndexedScript( cgs.media.bulletHitFleshScript, NULL, fleshEntityNum );
			}
		} else {
			VectorSubtract( cg_entities[fleshEntityNum].lerpOrigin, cg.snap->ps.origin, origin );
			VectorMA( cg.snap->ps.origin, 3, origin, origin );
			if ( aiType == AICHAR_PROTOSOLDIER ||
			     aiType == AICHAR_SUPERSOLDIER_LAB ||
				 aiType == AICHAR_SUPERSOLDIER ) {
				CG_SoundPlayIndexedScript( cgs.media.bulletHitFleshMetalScript, origin, ENTITYNUM_WORLD );
			} else {
				CG_SoundPlayIndexedScript( cgs.media.bulletHitFleshScript, origin, ENTITYNUM_WORLD );
			}
		}

		// if we haven't dropped a blood spat in a while, check if this is a good scenario
		if ( lastBloodSpat > cg.time || lastBloodSpat < cg.time - 500 ) {
			if ( CG_CalcMuzzlePoint( sourceEntityNum, start ) ) {
				VectorSubtract( end, start, dir );
				VectorNormalize( dir );
				VectorMA( end, 128, dir, trend );
				trap_CM_BoxTrace( &trace, end, trend, NULL, NULL, 0, MASK_SHOT & ~CONTENTS_BODY );

				if ( trace.fraction < 1 ) {
					CG_ImpactMark( cgs.media.bloodDotShaders[rand() % 5], trace.endpos, trace.plane.normal, random() * 360,
								   1,1,1,1, qtrue, 15 + random() * 20, qfalse, cg_bloodTime.integer * 1000 );
					lastBloodSpat = cg.time;
				} else if ( lastBloodSpat < cg.time - 1000 ) {
					// drop one on the ground?
					VectorCopy( end, trend );
					trend[2] -= 64;
					trap_CM_BoxTrace( &trace, end, trend, NULL, NULL, 0, MASK_SHOT & ~CONTENTS_BODY );

					if ( trace.fraction < 1 ) {
						CG_ImpactMark( cgs.media.bloodDotShaders[rand() % 5], trace.endpos, trace.plane.normal, random() * 360,
									   1,1,1,1, qtrue, 15 + random() * 10, qfalse, cg_bloodTime.integer * 1000 );
						lastBloodSpat = cg.time;
					}
				}
			}
		}

	} else {    // (not flesh)
		int fromweap;
		fromweap = cg_entities[sourceEntityNum].currentState.weapon;

		if ( !fromweap || cg_entities[sourceEntityNum].currentState.eFlags & EF_MG42_ACTIVE ) { // mounted
			fromweap = WP_MP40;
		}

		// TODO: not sure what kind of effect were going to do
		if ( wolfkick ) {
			return;
		}

		if ( CG_CalcMuzzlePoint( sourceEntityNum, start )
			 || cg.snap->ps.persistant[PERS_HWEAPON_USE] ) {
			vec3_t start2;
			VectorSubtract( end, start, dir );
			VectorNormalize( dir );
			VectorMA( end, -4, dir, start2 );   // back off a little so it doesn't start in solid
			VectorMA( end, 64, dir, dir );
			trap_CM_BoxTrace( &trace, start2, dir, NULL, NULL, 0, MASK_SHOT );

			if ( ( trace.surfaceFlags & SURF_METAL ) || !( rand() % 10 ) || ( otherEntNum2 != ENTITYNUM_NONE ) ) {
				// JPW NERVE compute new spark direction from normal & dir (rotate -dir 180 degrees about normal)

				// (SA) NOTE: isn't this done by the server and sent along in the 'normal' that's passed into this routine? (1107 g_weapon.c)

				VectorScale( dir,-1.0f,tmp );
				RotatePointAroundVector( tmp,normal,tmp,180.0f );
				CG_MissileHitWall( fromweap, 0, end, tmp, trace.surfaceFlags ); // sparks	//	(SA) modified to send missilehitwall surface parameters
// jpw
//				CG_MissileHitWall( fromweap, 0, end, normal, trace.surfaceFlags );	// sparks	//	(SA) modified to send missilehitwall surface parameters
			}
			if ( !( sourceContentType & CONTENTS_WATER ) && ( destContentType & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ) ) {   // only when shooting /into/ water
				trap_CM_BoxTrace( &trace, start, end, NULL, NULL, 0, MASK_WATER );
//				CG_Trace(&trace, start, NULL, NULL, end, -1, MASK_WATER);
//				if (!(trace.surfaceFlags & SURF_NOMARKS)) {	// check to see if the surface should draw splashes
				CG_MissileHitWall( fromweap, 2, trace.endpos, trace.plane.normal, trace.surfaceFlags );
//				}
			} else {
				CG_MissileHitWall( fromweap, 1, end, normal, trace.surfaceFlags );   // smoke puff	//	(SA) modified to send missilehitwall surface parameters

				if ( 0 ) {
					localEntity_t   *le;
					le = CG_AllocLocalEntity();
					le->leType = LE_EMITTER;
					le->startTime = cg.time;
					le->endTime = le->startTime + 20000;
					le->pos.trType = TR_STATIONARY;
					VectorCopy( end, le->pos.trBase );
					VectorCopy( normal, le->angles.trBase );
					le->ownerNum = 0;
				}
			}
		}
	}
}

/*
============
CG_ClientDamage
============
*/
void CG_ClientDamage( int entnum, int enemynum, int id ) {
	if ( id > CLDMG_MAX ) {
		CG_Error( "CG_ClientDamage: unknown damage type: %i\n", id );
	}
	trap_SendClientCommand( va( "cld %i %i %i", entnum, enemynum, id ) );
}

