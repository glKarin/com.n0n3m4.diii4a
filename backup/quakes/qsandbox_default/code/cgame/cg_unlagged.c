/*
===========================================================================
Copyright (C) 2006 Neil Toronto.

This file is part of the Unlagged source code.

Unlagged source code is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or (at your
option) any later version.

Unlagged source code is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with Unlagged source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "cg_local.h"

// we'll need these prototypes
void CG_ShotgunPattern( vec3_t origin, vec3_t origin2, int seed, int otherEntNum );
void CG_Bullet( vec3_t end, int sourceEntityNum, vec3_t normal, qboolean flesh, int fleshEntityNum );

// and this as well
//Must be in sync with g_weapon.c
#define MACHINEGUN_SPREAD	200
#define CHAINGUN_SPREAD		600

/*
=======================
CG_PredictWeaponEffects

Draws predicted effects for the railgun, shotgun, and machinegun.  The
lightning gun is done in CG_LightningBolt, since it was just a matter
of setting the right origin and angles.
=======================
*/
void CG_PredictWeaponEffects( centity_t *cent ) {
	vec3_t		muzzlePoint, forward, right, up;
	entityState_t *ent = &cent->currentState;
	clientInfo_t	*ci;
	int				weaphack;

	// if the client isn't us, forget it
	if ( cent->currentState.number != cg.predictedPlayerState.clientNum ) {
		return;
	}

	// if it's not switched on server-side, forget it
	if ( !cgs.delagHitscan ) {
		return;
	}
	
	ci = &cgs.clientinfo[ cent->currentState.clientNum ];
	
	if(ci->swepid >= 1){
	weaphack = ci->swepid;
	} else {
	weaphack = ent->weapon;
	}

	// get the muzzle point
	VectorCopy( cg.predictedPlayerState.origin, muzzlePoint );
	muzzlePoint[2] += cg.predictedPlayerState.viewheight;

	// get forward, right, and up
	AngleVectors( cg.predictedPlayerState.viewangles, forward, right, up );
	VectorMA( muzzlePoint, 14, forward, muzzlePoint );

	// was it a rail attack?
	if ( weaphack == WP_RAILGUN ) {
		// do we have it on for the rail gun?
		if ( cg_delag.integer & 1 || cg_delag.integer & 16 ) {
			trace_t trace;
			vec3_t endPoint;

			// trace forward
			VectorMA( muzzlePoint, 8192, forward, endPoint );

			// find the rail's end point
			CG_Trace( &trace, muzzlePoint, vec3_origin, vec3_origin, endPoint, cg.predictedPlayerState.clientNum, CONTENTS_SOLID );

			// do the magic-number adjustment
			VectorMA( muzzlePoint, 4, right, muzzlePoint );
			VectorMA( muzzlePoint, -1, up, muzzlePoint );

            if(!cg.renderingThirdPerson) {
            	if(cg_drawGun.integer == 2)
					VectorMA(muzzlePoint, 8, cg.refdef.viewaxis[1], muzzlePoint);
            	else if(cg_drawGun.integer == 3)
					VectorMA(muzzlePoint, 4, cg.refdef.viewaxis[1], muzzlePoint);
            }

			// draw a rail trail
			CG_RailTrail( &cgs.clientinfo[cent->currentState.number], muzzlePoint, trace.endpos );
			//Com_Printf( "Predicted rail trail\n" );

			// explosion at end if not SURF_NOIMPACT
			if ( !(trace.surfaceFlags & SURF_NOIMPACT) ) {
				// predict an explosion
				CG_MissileHitWall( weaphack, cg.predictedPlayerState.clientNum, trace.endpos, trace.plane.normal, IMPACTSOUND_DEFAULT );
			}
		}
	}
	// was it a shotgun attack?
	else if ( weaphack == WP_SHOTGUN ) {
		// do we have it on for the shotgun?
		if ( cg_delag.integer & 1 || cg_delag.integer & 4 ) {
			int contents;
			vec3_t endPoint, v;
			vec3_t			up;

			// do everything like the server does

			SnapVector( muzzlePoint );

			VectorScale( forward, 4096, endPoint );
			SnapVector( endPoint );

			VectorSubtract( endPoint, muzzlePoint, v );
			VectorNormalize( v );
			VectorScale( v, 32, v );
			VectorAdd( muzzlePoint, v, v );

			contents = trap_CM_PointContents( muzzlePoint, 0 );
			if ( !( contents & CONTENTS_WATER ) ) {
				VectorSet( up, 0, 0, 8 );
				CG_SmokePuff( v, up, 32, 1, 1, 1, 0.33f, 900, cg.time, 0, LEF_PUFF_DONT_SCALE, cgs.media.shotgunSmokePuffShader );
			}

			// do the shotgun pellets
			CG_ShotgunPattern( muzzlePoint, endPoint, cg.oldTime % 256, cg.predictedPlayerState.clientNum );
			//Com_Printf( "Predicted shotgun pattern\n" );
		}
	}
	// was it a machinegun attack?
	else if ( weaphack == WP_MACHINEGUN ) {
		// do we have it on for the machinegun?
		if ( cg_delag.integer & 1 || cg_delag.integer & 2 ) {
			// the server will use this exact time (it'll be serverTime on that end)
			int seed = cg.oldTime % 256;
			float r, u;
			trace_t tr;
			qboolean flesh;
			int fleshEntityNum = 0;
			vec3_t endPoint;

			// do everything exactly like the server does

			r = Q_random(&seed) * M_PI * 2.0f;
			u = sin(r) * Q_crandom(&seed) * mod_mgspread * 16;
			r = cos(r) * Q_crandom(&seed) * mod_mgspread * 16;

			VectorMA( muzzlePoint, 8192*16, forward, endPoint );
			VectorMA( endPoint, r, right, endPoint );
			VectorMA( endPoint, u, up, endPoint );

			CG_Trace(&tr, muzzlePoint, NULL, NULL, endPoint, cg.predictedPlayerState.clientNum, MASK_SHOT );

			if ( tr.surfaceFlags & SURF_NOIMPACT ) {
				return;
			}

			// snap the endpos to integers, but nudged towards the line
			SnapVectorTowards( tr.endpos, muzzlePoint );

			// do bullet impact
			if ( tr.entityNum < MAX_CLIENTS ) {
				flesh = qtrue;
				fleshEntityNum = tr.entityNum;
			} else {
				flesh = qfalse;
			}

			// do the bullet impact
			CG_Bullet( tr.endpos, cg.predictedPlayerState.clientNum, tr.plane.normal, flesh, fleshEntityNum );
			//Com_Printf( "Predicted bullet\n" );
		}
	}
        // was it a chaingun attack?
	else if ( weaphack == WP_CHAINGUN ) {
		// do we have it on for the machinegun?
		if ( cg_delag.integer & 1 || cg_delag.integer & 2 ) {
			// the server will use this exact time (it'll be serverTime on that end)
			int seed = cg.oldTime % 256;
			float r, u;
			trace_t tr;
			qboolean flesh;
			int fleshEntityNum = 0;
			vec3_t endPoint;

			// do everything exactly like the server does

			r = Q_random(&seed) * M_PI * 2.0f;
			u = sin(r) * Q_crandom(&seed) * mod_cgspread * 16;
			r = cos(r) * Q_crandom(&seed) * mod_cgspread * 16;

			VectorMA( muzzlePoint, 8192*16, forward, endPoint );
			VectorMA( endPoint, r, right, endPoint );
			VectorMA( endPoint, u, up, endPoint );

			CG_Trace(&tr, muzzlePoint, NULL, NULL, endPoint, cg.predictedPlayerState.clientNum, MASK_SHOT );

			if ( tr.surfaceFlags & SURF_NOIMPACT ) {
				return;
			}

			// snap the endpos to integers, but nudged towards the line
			SnapVectorTowards( tr.endpos, muzzlePoint );

			// do bullet impact
			if ( tr.entityNum < MAX_CLIENTS ) {
				flesh = qtrue;
				fleshEntityNum = tr.entityNum;
			} else {
				flesh = qfalse;
			}

			// do the bullet impact
			CG_Bullet( tr.endpos, cg.predictedPlayerState.clientNum, tr.plane.normal, flesh, fleshEntityNum );
			//Com_Printf( "Predicted bullet\n" );
		}
	}
}

/*
=================
CG_AddBoundingBox

Draws a bounding box around a player.  Called from CG_Player.
=================
*/
/*void CG_AddBoundingBox( centity_t *cent ) {
	polyVert_t verts[4];
	clientInfo_t *ci;
	int i;
	vec3_t mins = {-15, -15, -24};
	vec3_t maxs = {15, 15, 32};
	float extx, exty, extz;
	vec3_t corners[8];
	qhandle_t bboxShader, bboxShader_nocull;

	if ( !cg_drawBBox.integer ) {
		return;
	}

	// don't draw it if it's us in first-person
	if ( cent->currentState.number == cg.predictedPlayerState.clientNum &&
			!cg.renderingThirdPerson ) {
		return;
	}

	// don't draw it for dead players
	if ( cent->currentState.eFlags & EF_DEAD ) {
		return;
	}

	// get the shader handles
	bboxShader = trap_R_RegisterShader( "bbox" );
	bboxShader_nocull = trap_R_RegisterShader( "bbox_nocull" );

	// if they don't exist, forget it
	if ( !bboxShader || !bboxShader_nocull ) {
		return;
	}

	// get the player's client info
	ci = &cgs.clientinfo[cent->currentState.clientNum];

	// if it's us
	if ( cent->currentState.number == cg.predictedPlayerState.clientNum ) {
		// use the view height
		maxs[2] = cg.predictedPlayerState.viewheight + 6;
	}
	else {
		int x, zd, zu;

		// otherwise grab the encoded bounding box
		x = (cent->currentState.solid & 255);
		zd = ((cent->currentState.solid>>8) & 255);
		zu = ((cent->currentState.solid>>16) & 255) - 32;

		mins[0] = mins[1] = -x;
		maxs[0] = maxs[1] = x;
		mins[2] = -zd;
		maxs[2] = zu;
	}

	// get the extents (size)
	extx = maxs[0] - mins[0];
	exty = maxs[1] - mins[1];
	extz = maxs[2] - mins[2];

	
	// set the polygon's texture coordinates
	verts[0].st[0] = 0;
	verts[0].st[1] = 0;
	verts[1].st[0] = 0;
	verts[1].st[1] = 1;
	verts[2].st[0] = 1;
	verts[2].st[1] = 1;
	verts[3].st[0] = 1;
	verts[3].st[1] = 0;

	// set the polygon's vertex colors
	if ( ci->team == TEAM_RED ) {
		for ( i = 0; i < 4; i++ ) {
			verts[i].modulate[0] = 160;
			verts[i].modulate[1] = 0;
			verts[i].modulate[2] = 0;
			verts[i].modulate[3] = 255;
		}
	}
	else if ( ci->team == TEAM_BLUE ) {
		for ( i = 0; i < 4; i++ ) {
			verts[i].modulate[0] = 0;
			verts[i].modulate[1] = 0;
			verts[i].modulate[2] = 192;
			verts[i].modulate[3] = 255;
		}
	}
	else {
		for ( i = 0; i < 4; i++ ) {
			verts[i].modulate[0] = 0;
			verts[i].modulate[1] = 128;
			verts[i].modulate[2] = 0;
			verts[i].modulate[3] = 255;
		}
	}

	VectorAdd( cent->lerpOrigin, maxs, corners[3] );

	VectorCopy( corners[3], corners[2] );
	corners[2][0] -= extx;

	VectorCopy( corners[2], corners[1] );
	corners[1][1] -= exty;

	VectorCopy( corners[1], corners[0] );
	corners[0][0] += extx;

	for ( i = 0; i < 4; i++ ) {
		VectorCopy( corners[i], corners[i + 4] );
		corners[i + 4][2] -= extz;
	}

	// top
	VectorCopy( corners[0], verts[0].xyz );
	VectorCopy( corners[1], verts[1].xyz );
	VectorCopy( corners[2], verts[2].xyz );
	VectorCopy( corners[3], verts[3].xyz );
	trap_R_AddPolyToScene( bboxShader, 4, verts );

	// bottom
	VectorCopy( corners[7], verts[0].xyz );
	VectorCopy( corners[6], verts[1].xyz );
	VectorCopy( corners[5], verts[2].xyz );
	VectorCopy( corners[4], verts[3].xyz );
	trap_R_AddPolyToScene( bboxShader, 4, verts );

	// top side
	VectorCopy( corners[3], verts[0].xyz );
	VectorCopy( corners[2], verts[1].xyz );
	VectorCopy( corners[6], verts[2].xyz );
	VectorCopy( corners[7], verts[3].xyz );
	trap_R_AddPolyToScene( bboxShader_nocull, 4, verts );

	// left side
	VectorCopy( corners[2], verts[0].xyz );
	VectorCopy( corners[1], verts[1].xyz );
	VectorCopy( corners[5], verts[2].xyz );
	VectorCopy( corners[6], verts[3].xyz );
	trap_R_AddPolyToScene( bboxShader_nocull, 4, verts );

	// right side
	VectorCopy( corners[0], verts[0].xyz );
	VectorCopy( corners[3], verts[1].xyz );
	VectorCopy( corners[7], verts[2].xyz );
	VectorCopy( corners[4], verts[3].xyz );
	trap_R_AddPolyToScene( bboxShader_nocull, 4, verts );

	// bottom side
	VectorCopy( corners[1], verts[0].xyz );
	VectorCopy( corners[0], verts[1].xyz );
	VectorCopy( corners[4], verts[2].xyz );
	VectorCopy( corners[5], verts[3].xyz );
	trap_R_AddPolyToScene( bboxShader_nocull, 4, verts );
}*/

/*
================
CG_Cvar_ClampInt

Clamps a cvar between two integer values, returns qtrue if it had to.
================
*/
qboolean CG_Cvar_ClampInt( const char *name, vmCvar_t *vmCvar, int min, int max ) {
	if ( vmCvar->integer > max ) {
		CG_Printf( "Allowed values are %d to %d.\n", min, max );

		Com_sprintf( vmCvar->string, MAX_CVAR_VALUE_STRING, "%d", max );
		vmCvar->value = max;
		vmCvar->integer = max;

		trap_Cvar_Set( name, vmCvar->string );
		return qtrue;
	}

	if ( vmCvar->integer < min ) {
		CG_Printf( "Allowed values are %d to %d.\n", min, max );

		Com_sprintf( vmCvar->string, MAX_CVAR_VALUE_STRING, "%d", min );
		vmCvar->value = min;
		vmCvar->integer = min;

		trap_Cvar_Set( name, vmCvar->string );
		return qtrue;
	}

	return qfalse;
}
