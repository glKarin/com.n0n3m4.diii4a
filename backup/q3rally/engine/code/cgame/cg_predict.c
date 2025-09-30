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
//
// cg_predict.c -- this file generates cg.predictedPlayerState by either
// interpolating between snapshots from the server or locally predicting
// ahead the client's movement.
// It also handles local physics interaction, like fragments bouncing off walls

#include "cg_local.h"

static	pmove_t		cg_pmove;

static	int			cg_numSolidEntities;
static	centity_t	*cg_solidEntities[MAX_ENTITIES_IN_SNAPSHOT];
static	int			cg_numTriggerEntities;
static	centity_t	*cg_triggerEntities[MAX_ENTITIES_IN_SNAPSHOT];

// Q3Rally Code Start
/*
=================
Com_LogPrintf

Print to the logfile
=================
*/
void QDECL Com_LogPrintf( const char *fmt, ... ) {
	va_list			argptr;
	char			string[1024];
	fileHandle_t	logFile;

	trap_FS_FOpenFile( "cg_physics.log", &logFile, FS_APPEND );

	va_start( argptr, fmt );
	Q_vsnprintf (string, sizeof(string), fmt, argptr);
	va_end( argptr );

	trap_FS_Write( string, strlen( string ), logFile );

	trap_FS_FCloseFile( logFile );
}

/*
================================================================================
CG_DebugDynamics
================================================================================
*/
void CG_DebugDynamics( carBody_t *body, carPoint_t *points, int i ){
	Com_LogPrintf("\n");
	Com_LogPrintf("PM_DebugDynamics: point ORIGIN - %.3f, %.3f, %.3f\n", points[i].r[0], points[i].r[1], points[i].r[2]);
	Com_LogPrintf("PM_DebugDynamics: point VELOCITY - %.3f, %.3f, %.3f\n", points[i].v[0], points[i].v[1], points[i].v[2]);

	Com_LogPrintf("PM_DebugDynamics: point ONGROUND - %i\n", points[i].onGround);
	Com_LogPrintf("PM_DebugDynamics: point NORMAL 1- %.3f, %.3f, %.3f\n", points[i].normals[0][0], points[i].normals[0][1], points[i].normals[0][2]);
	Com_LogPrintf("PM_DebugDynamics: point NORMAL 2- %.3f, %.3f, %.3f\n", points[i].normals[1][0], points[i].normals[1][1], points[i].normals[1][2]);
	Com_LogPrintf("PM_DebugDynamics: point NORMAL 3- %.3f, %.3f, %.3f\n", points[i].normals[2][0], points[i].normals[2][1], points[i].normals[2][2]);
//	Com_LogPrintf("PM_DebugDynamics: point MASS - %.3f\n", points[pm->pDebug-1].mass);

	Com_LogPrintf("PM_DebugDynamics: body ORIGIN - %.3f, %.3f, %.3f\n", body->r[0], body->r[1], body->r[2]);
	Com_LogPrintf("PM_DebugDynamics: body VELOCITY - %.3f, %.3f, %.3f\n", body->v[0], body->v[1], body->v[2]);
	Com_LogPrintf("PM_DebugDynamics: body ANG VELOCITY - %.3f, %.3f, %.3f\n", body->w[0], body->w[1], body->w[2]);

//	Com_LogPrintf("PM_DebugDynamics: body COM - %.3f, %.3f, %.3f\n", body->CoM[0], body->CoM[1], body->CoM[2]);
//	Com_LogPrintf("PM_DebugDynamics: body MASS - %.3f\n", body->mass);

	Com_LogPrintf("Direction vectors ----------------------------------------\n");

	Com_LogPrintf("PM_DebugDynamics: body FORWARD - %.3f, %.3f, %.3f\n", body->forward[0], body->forward[1], body->forward[2]);
	Com_LogPrintf("PM_DebugDynamics: body RIGHT - %.3f, %.3f, %.3f\n", body->right[0], body->right[1], body->right[2]);
	Com_LogPrintf("PM_DebugDynamics: body UP - %.3f, %.3f, %.3f\n", body->up[0], body->up[1], body->up[2]);
	Com_LogPrintf("\n");
}


/*
================================================================================
CG_DebugForces
================================================================================
*/
void CG_DebugForces( carBody_t *body, carPoint_t *points, int i ){
	Com_LogPrintf("\n");
	Com_LogPrintf("PM_DebugForces: point GRAVITY - %.3f, %.3f, %.3f\n", points[i].forces[GRAVITY][0], points[i].forces[GRAVITY][1], points[i].forces[GRAVITY][2]);
	Com_LogPrintf("PM_DebugForces: point NORMAL - %.3f, %.3f, %.3f\n", points[i].forces[NORMAL][0], points[i].forces[NORMAL][1], points[i].forces[NORMAL][2]);
	Com_LogPrintf("PM_DebugForces: point SHOCK - %.3f, %.3f, %.3f\n", points[i].forces[SHOCK][0], points[i].forces[SHOCK][1], points[i].forces[SHOCK][2]);
	Com_LogPrintf("PM_DebugForces: point SPRING - %.3f, %.3f, %.3f\n", points[i].forces[SPRING][0], points[i].forces[SPRING][1], points[i].forces[SPRING][2]);
	Com_LogPrintf("PM_DebugForces: point SWAY_BAR - %.3f, %.3f, %.3f\n", points[i].forces[SWAY_BAR][0], points[i].forces[SWAY_BAR][1], points[i].forces[SWAY_BAR][2]);
	Com_LogPrintf("PM_DebugForces: point ROAD - %.3f, %.3f, %.3f\n", points[i].forces[ROAD][0], points[i].forces[ROAD][1], points[i].forces[ROAD][2]);
	Com_LogPrintf("PM_DebugForces: point INTERNAL - %.3f, %.3f, %.3f\n", points[i].forces[INTERNAL][0], points[i].forces[INTERNAL][1], points[i].forces[INTERNAL][2]);
	Com_LogPrintf("PM_DebugForces: point AIR_FRICTION - %.3f, %.3f, %.3f\n", points[i].forces[AIR_FRICTION][0], points[i].forces[AIR_FRICTION][1], points[i].forces[AIR_FRICTION][2]);

	Com_LogPrintf("PM_DebugForces: point NETFORCE - %.3f, %.3f, %.3f\n", points[i].netForce[0], points[i].netForce[1], points[i].netForce[2]);

	Com_LogPrintf("PM_DebugForces: point fluidDensity - %.6f\n", points[i].fluidDensity);

	Com_LogPrintf("PM_DebugForces: body netForce - %.3f, %.3f, %.3f\n", body->netForce[0], body->netForce[1], body->netForce[2]);
	Com_LogPrintf("PM_DebugForces: body netMoment - %.3f, %.3f, %.3f\n", body->netMoment[0], body->netMoment[1], body->netMoment[2]);
	Com_LogPrintf("\n");
}
// END

/*
====================
CG_BuildSolidList

When a new cg.snap has been set, this function builds a sublist
of the entities that are actually solid, to make for more
efficient collision detection
====================
*/
void CG_BuildSolidList( void ) {
	int			i;
	centity_t	*cent;
	snapshot_t	*snap;
	entityState_t	*ent;

	cg_numSolidEntities = 0;
	cg_numTriggerEntities = 0;

	if ( cg.nextSnap && !cg.nextFrameTeleport && !cg.thisFrameTeleport ) {
		snap = cg.nextSnap;
	} else {
		snap = cg.snap;
	}

	for ( i = 0 ; i < snap->numEntities ; i++ ) {
		cent = &cg_entities[ snap->entities[ i ].number ];
		ent = &cent->currentState;

		if ( ent->eType == ET_ITEM || ent->eType == ET_PUSH_TRIGGER || ent->eType == ET_TELEPORT_TRIGGER ) {
			cg_triggerEntities[cg_numTriggerEntities] = cent;
			cg_numTriggerEntities++;
			continue;
		}

		if ( cent->nextState.solid ) {
			cg_solidEntities[cg_numSolidEntities] = cent;
			cg_numSolidEntities++;
			continue;
		}
	}
}

/*
====================
CG_ClipMoveToEntities

====================
*/
static void CG_ClipMoveToEntities ( const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end,
							int skipNumber, int mask, trace_t *tr ) {
	int			i, x, zd, zu;
	trace_t		trace;
	entityState_t	*ent;
	clipHandle_t 	cmodel;
	vec3_t		bmins, bmaxs;
	vec3_t		origin, angles;
	centity_t	*cent;

	for ( i = 0 ; i < cg_numSolidEntities ; i++ ) {
		cent = cg_solidEntities[ i ];
		ent = &cent->currentState;

		if ( ent->number == skipNumber ) {
			continue;
		}

		if ( ent->solid == SOLID_BMODEL ) {
			// special value for bmodel
			cmodel = trap_CM_InlineModel( ent->modelindex );
			VectorCopy( cent->lerpAngles, angles );
			BG_EvaluateTrajectory( &cent->currentState.pos, cg.physicsTime, origin );
		} else {
			// encoded bbox
			x = (ent->solid & 255);
			zd = ((ent->solid>>8) & 255);
			zu = ((ent->solid>>16) & 255) - 32;

			bmins[0] = bmins[1] = -x;
			bmaxs[0] = bmaxs[1] = x;
			bmins[2] = -zd;
			bmaxs[2] = zu;

			cmodel = trap_CM_TempBoxModel( bmins, bmaxs );
			VectorCopy( vec3_origin, angles );
			VectorCopy( cent->lerpOrigin, origin );
		}


		trap_CM_TransformedBoxTrace ( &trace, start, end,
			mins, maxs, cmodel,  mask, origin, angles);

		if (trace.allsolid || trace.fraction < tr->fraction) {
			trace.entityNum = ent->number;
			*tr = trace;
		} else if (trace.startsolid) {
			tr->startsolid = qtrue;
		}
		if ( tr->allsolid ) {
			return;
		}
	}
}

/*
================
CG_Trace
================
*/
void	CG_Trace( trace_t *result, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, 
					 int skipNumber, int mask ) {
	trace_t	t;

	trap_CM_BoxTrace ( &t, start, end, mins, maxs, 0, mask);
	t.entityNum = t.fraction != 1.0 ? ENTITYNUM_WORLD : ENTITYNUM_NONE;
	// check all other solid models
	CG_ClipMoveToEntities (start, mins, maxs, end, skipNumber, mask, &t);

	*result = t;
}

/*
================
CG_PointContents
================
*/
int		CG_PointContents( const vec3_t point, int passEntityNum ) {
	int			i;
	entityState_t	*ent;
	centity_t	*cent;
	clipHandle_t cmodel;
	int			contents;

	contents = trap_CM_PointContents (point, 0);

	for ( i = 0 ; i < cg_numSolidEntities ; i++ ) {
		cent = cg_solidEntities[ i ];

		ent = &cent->currentState;

		if ( ent->number == passEntityNum ) {
			continue;
		}

		if (ent->solid != SOLID_BMODEL) { // special value for bmodel
			continue;
		}

		cmodel = trap_CM_InlineModel( ent->modelindex );
		if ( !cmodel ) {
			continue;
		}

		contents |= trap_CM_TransformedPointContents( point, cmodel, cent->lerpOrigin, cent->lerpAngles );
	}

	return contents;
}


// Q3Rally Code Start
/*
========================
CG_ExtrapolatePlayerState

Generates cg.predictedPlayerState by extrapolating
cg.snap->player_state
========================
*/
static void CG_ExtrapolatePlayerState( qboolean grabAngles ) {
	float			f;
	int				i;
	playerState_t	*out;
	snapshot_t		*prev;

	out = &cg.predictedPlayerState;
	prev = cg.snap;

	*out = cg.snap->ps;

	// if we are still allowing local input, short circuit the view angles
	if ( grabAngles ) {
		usercmd_t	cmd;
		int			cmdNum;

		cmdNum = trap_GetCurrentCmdNumber();
		trap_GetUserCmd( cmdNum, &cmd );

		PM_UpdateViewAngles( out, &cmd, cg_controlMode.integer );
	}

	// if the next frame is a teleport, we can't lerp to it
	if ( cg.thisFrameTeleport ) {
		return;
	}

	f = (float)( cg.time - prev->serverTime ) / 1000.0f;
//	f = (float)( cg.time - prev->ps.commandTime ) / 1000.0f;

//	Com_Printf( "prev->serverTime %i, prev->ps.commandTime %i, f %f, f %f\n", prev->serverTime, prev->ps.commandTime, f, (float)( cg.time - prev->ps.commandTime ) / 1000.0f );

	if ( !grabAngles ) {
		out->damagePitch = prev->ps.damagePitch;
		out->damageYaw = prev->ps.damageYaw;

		out->damageAngles[PITCH] = BYTE2ANGLE(prev->ps.damagePitch);
		out->damageAngles[YAW] = BYTE2ANGLE(prev->ps.damageYaw);
		out->damageAngles[ROLL] = 0;
	}

	for ( i = 0 ; i < 3 ; i++ ) {
		out->origin[i] = prev->ps.origin[i] + f * prev->ps.velocity[i];

		out->viewangles[i] = prev->ps.viewangles[i];

		out->velocity[i] = prev->ps.velocity[i];
		out->angularMomentum[i] = prev->ps.angularMomentum[i];
	}

/*
	VectorRotate(tBody->L, sBody->inverseWorldInertiaTensor, tBody->w);

	m[0][0] = 0;											m[0][1] = (time / 2.0f) * -(sBody->w[2] + tBody->w[2]);	m[0][2] = (time / 2.0f) * (sBody->w[1] + tBody->w[1]);
	m[1][0] = (time / 2.0f) * (sBody->w[2] + tBody->w[2]);	m[1][1] = 0;											m[1][2] = (time / 2.0f) * -(sBody->w[0] + tBody->w[0]);
	m[2][0] = (time / 2.0f) * -(sBody->w[1] + tBody->w[1]);	m[2][1] = (time / 2.0f) * (sBody->w[0] + tBody->w[0]);	m[2][2] = 0;

	MatrixMultiply(m, sBody->t, m2);
	MatrixAdd(sBody->t, m2, tBody->t);

	OrthonormalizeOrientation(tBody->t);
*/
}
// END


/*
========================
CG_InterpolatePlayerState

Generates cg.predictedPlayerState by interpolating between
cg.snap->player_state and cg.nextFrame->player_state
========================
*/
static void CG_InterpolatePlayerState( qboolean grabAngles ) {
	float			f;
	int				i;
	playerState_t	*out;
	snapshot_t		*prev, *next;

	out = &cg.predictedPlayerState;
	prev = cg.snap;
	next = cg.nextSnap;

	*out = cg.snap->ps;

	if ( !next || next->serverTime <= prev->serverTime ) {
// STONELANCE UPDATE - try extrapolating?
		if ( !cg_paused.integer && cg_debugpredict.integer )
			Com_Printf( "CG_InterpolatePlayerState: No next snapshot to interpolate towards.\n" );

		CG_ExtrapolatePlayerState( grabAngles );
// END
		return;
	}

	// if we are still allowing local input, short circuit the view angles
	if ( grabAngles ) {
		usercmd_t	cmd;
		int			cmdNum;

		cmdNum = trap_GetCurrentCmdNumber();
		trap_GetUserCmd( cmdNum, &cmd );

// Q3Rally Code Start
//		PM_UpdateViewAngles( out, &cmd );
		PM_UpdateViewAngles( out, &cmd, cg_controlMode.integer );
// END
	}

	// if the next frame is a teleport, we can't lerp to it
	if ( cg.nextFrameTeleport ) {
		return;
	}

	if ( !next || next->serverTime <= prev->serverTime ) {
// STONELANCE UPDATE - try extrapolating?
//		if ( !cg_paused.integer && cg_debugpredict.integer )
//			Com_Printf( "CG_InterpolatePlayerState: No next snapshot to interpolate towards.\n" );

//		CG_ExtrapolatePlayerState( grabAngles );
// END
		return;
	}

	f = (float)( cg.time - prev->serverTime ) / ( next->serverTime - prev->serverTime );

// Q3Rally Code Start
/*
	i = next->ps.bobCycle;
	if ( i < prev->ps.bobCycle ) {
		i += 256;		// handle wraparound
	}
	out->bobCycle = prev->ps.bobCycle + f * ( i - prev->ps.bobCycle );
*/

	if ( !grabAngles ) {
		out->damagePitch = ANGLE2BYTE(LerpAngle( BYTE2ANGLE(prev->ps.damagePitch), BYTE2ANGLE(next->ps.damagePitch), f ));
		out->damageYaw = ANGLE2BYTE(LerpAngle( BYTE2ANGLE(prev->ps.damageYaw), BYTE2ANGLE(next->ps.damageYaw), f ));

		out->damageAngles[PITCH] = LerpAngle( BYTE2ANGLE(prev->ps.damagePitch), BYTE2ANGLE(next->ps.damagePitch), f );
		out->damageAngles[YAW] = LerpAngle( BYTE2ANGLE(prev->ps.damageYaw), BYTE2ANGLE(next->ps.damageYaw), f );
		out->damageAngles[ROLL] = 0;
	}
// END

	for ( i = 0 ; i < 3 ; i++ ) {
		out->origin[i] = prev->ps.origin[i] + f * (next->ps.origin[i] - prev->ps.origin[i] );
// Q3Rally Code Start
//		if ( !grabAngles ) { // always grab car angles
// END
			out->viewangles[i] = LerpAngle( 
				prev->ps.viewangles[i], next->ps.viewangles[i], f );
// Q3Rally Code Start
//		}
// END
		out->velocity[i] = prev->ps.velocity[i] + 
			f * (next->ps.velocity[i] - prev->ps.velocity[i] );

// STONELANCE - lerp angular momentum (just incase)
		out->angularMomentum[i] = prev->ps.angularMomentum[i] + 
			f * (next->ps.angularMomentum[i] - prev->ps.angularMomentum[i] );
// END
	}

}

/*
===================
CG_TouchItem
===================
*/
static void CG_TouchItem( centity_t *cent ) {
	gitem_t		*item;

	if ( !cg_predictItems.integer ) {
		return;
	}
	if ( !BG_PlayerTouchesItem( &cg.predictedPlayerState, &cent->currentState, cg.time ) ) {
		return;
	}

	// never pick an item up twice in a prediction
	if ( cent->miscTime == cg.time ) {
		return;
	}

	if ( !BG_CanItemBeGrabbed( cgs.gametype, &cent->currentState, &cg.predictedPlayerState ) ) {
		return;		// can't hold it
	}

	item = &bg_itemlist[ cent->currentState.modelindex ];

	// Special case for flags.  
	// We don't predict touching our own flag
#ifdef MISSIONPACK
	if( cgs.gametype == GT_1FCTF ) {
		if( item->giType == IT_TEAM && item->giTag != PW_NEUTRALFLAG ) {
			return;
		}
	}
#endif
	if( cgs.gametype == GT_CTF ) {
		if (cg.predictedPlayerState.persistant[PERS_TEAM] == TEAM_RED &&
			item->giType == IT_TEAM && item->giTag == PW_REDFLAG)
			return;
		if (cg.predictedPlayerState.persistant[PERS_TEAM] == TEAM_BLUE &&
			item->giType == IT_TEAM && item->giTag == PW_BLUEFLAG)
			return;
	}

	// grab it
	BG_AddPredictableEventToPlayerstate( EV_ITEM_PICKUP, cent->currentState.modelindex , &cg.predictedPlayerState);

	// remove it from the frame so it won't be drawn
	cent->currentState.eFlags |= EF_NODRAW;

	// don't touch it again this prediction
	cent->miscTime = cg.time;

	// if it's a weapon, give them some predicted ammo so the autoswitch will work
	if ( item->giType == IT_WEAPON ) {
		cg.predictedPlayerState.stats[ STAT_WEAPONS ] |= 1 << item->giTag;
		if ( !cg.predictedPlayerState.ammo[ item->giTag ] ) {
			cg.predictedPlayerState.ammo[ item->giTag ] = 1;
		}
	}
}


/*
=========================
CG_TouchTriggerPrediction

Predict push triggers and items
=========================
*/
static void CG_TouchTriggerPrediction( void ) {
	int			i;
	trace_t		trace;
	entityState_t	*ent;
	clipHandle_t cmodel;
	centity_t	*cent;
	qboolean	spectator;

	// dead clients don't activate triggers
	if ( cg.predictedPlayerState.stats[STAT_HEALTH] <= 0 ) {
		return;
	}

	spectator = ( cg.predictedPlayerState.pm_type == PM_SPECTATOR );

	if ( cg.predictedPlayerState.pm_type != PM_NORMAL && !spectator ) {
		return;
	}

	for ( i = 0 ; i < cg_numTriggerEntities ; i++ ) {
		cent = cg_triggerEntities[ i ];
		ent = &cent->currentState;

		if ( ent->eType == ET_ITEM && !spectator ) {
			CG_TouchItem( cent );
			continue;
		}

		if ( ent->solid != SOLID_BMODEL ) {
			continue;
		}

		cmodel = trap_CM_InlineModel( ent->modelindex );
		if ( !cmodel ) {
			continue;
		}

		trap_CM_BoxTrace( &trace, cg.predictedPlayerState.origin, cg.predictedPlayerState.origin, 
			cg_pmove.mins, cg_pmove.maxs, cmodel, -1 );

		if ( !trace.startsolid ) {
			continue;
		}

		if ( ent->eType == ET_TELEPORT_TRIGGER ) {
			cg.hyperspace = qtrue;
		} else if ( ent->eType == ET_PUSH_TRIGGER ) {
// Q3Rally Code Start
//			BG_TouchJumpPad( &cg.predictedPlayerState, ent );
			BG_TouchJumpPad( &cg.car, &cg.predictedPlayerState, ent );
// END
		}
	}

	// if we didn't touch a jump pad this pmove frame
	if ( cg.predictedPlayerState.jumppad_frame != cg.predictedPlayerState.pmove_framecount ) {
		cg.predictedPlayerState.jumppad_frame = 0;
		cg.predictedPlayerState.jumppad_ent = 0;
	}
}


// Q3Rally Code Start
void CG_UpdateCarFromPS ( playerState_t *ps ) {

	if ( !cg_paused.integer && cg_debugpredict.integer )
		Com_Printf( "CG_UpdateCarFromPS\n" );

	cg.car.rpm = ps->stats[STAT_RPM];
	cg.car.gear = ps->stats[STAT_GEAR];

	VectorCopy(ps->origin, cg.car.sBody.r);
	VectorCopy(ps->velocity, cg.car.sBody.v);
	AnglesToOrientation(ps->viewangles, cg.car.sBody.t);
//	AnglesToQuaternion(ps->viewangles, cg.car.sBody.q);

	VectorCopy(ps->angularMomentum, cg.car.sBody.L);

	PM_CalculateSecondaryQuantities( &cg.car, &cg.car.sBody, cg.car.sPoints );
}


void CG_UpdateWheelsFromSnapshot( snapshot_t *snap )
{
	int				i;
	centity_t		*cent;
	entityState_t	*s1;

	for( i = 0; i < snap->numEntities; i++ )
	{
		if( snap->entities[i].eType != ET_AUXENT ) continue;

		cent = &cg_entities[ cg.snap->entities[ i ].number ];
		s1 = &cent->currentState;

		if ( s1->otherEntityNum != cg.snap->ps.clientNum ) continue;

//		Com_Printf( "updating wheel %i\n", s1->otherEntityNum2 );

//		cg_entities[s1->otherEntityNum].wheelSpeeds[s1->otherEntityNum2] = s1->apos.trDelta[0];
//		cg_entities[s1->otherEntityNum].wheelSkidding[s1->otherEntityNum2] = s1->frame;
//		cg_entities[s1->otherEntityNum].steeringAngle = s1->apos.trDelta[1];

		cg.car.sPoints[s1->otherEntityNum2].w = s1->apos.trDelta[0];
		cg.car.sPoints[s1->otherEntityNum2].slipping = s1->frame;
		cg.car.wheelAngle = s1->apos.trDelta[1];
		VectorCopy(s1->pos.trBase, cg.car.sPoints[s1->otherEntityNum2].r);
		VectorCopy(s1->pos.trDelta, cg.car.sPoints[s1->otherEntityNum2].v);

		VectorCopy(s1->origin2, cg.car.sPoints[s1->otherEntityNum2].normals[0]);
		cg.car.sPoints[s1->otherEntityNum2].onGround = s1->groundEntityNum;
	}
}
// END


/*
=================
CG_PredictPlayerState

Generates cg.predictedPlayerState for the current cg.time
cg.predictedPlayerState is guaranteed to be valid after exiting.

For demo playback, this will be an interpolation between two valid
playerState_t.

For normal gameplay, it will be the result of predicted usercmd_t on
top of the most recent playerState_t received from the server.

Each new snapshot will usually have one or more new usercmd over the last,
but we simulate all unacknowledged commands each time, not just the new ones.
This means that on an internet connection, quite a few pmoves may be issued
each frame.

OPTIMIZE: don't re-simulate unless the newly arrived snapshot playerState_t
differs from the predicted one.  Would require saving all intermediate
playerState_t during prediction.

We detect prediction errors and allow them to be decayed off over several frames
to ease the jerk.
=================
*/

// Q3Rally Code Start
extern char *eventnames[];
// END
void CG_PredictPlayerState( void ) {
	int			cmdNum, current;
	playerState_t	oldPlayerState;
	qboolean	moved;
	usercmd_t	oldestCmd;
	usercmd_t	latestCmd;
// Q3Rally Code Start
	usercmd_t	nextCmd;
	centity_t	*cent;
	int			i;
	int			count, skipcount;
// END

	cg.hyperspace = qfalse;	// will be set if touching a trigger_teleport

	// if this is the first frame we must guarantee
	// predictedPlayerState is valid even if there is some
	// other error condition
	if ( !cg.validPPS ) {
		cg.validPPS = qtrue;
		cg.predictedPlayerState = cg.snap->ps;
// Q3Rally Code Start
//		Com_Printf( "2 predictedPlayerState damageYaw %d\n", cg.predictedPlayerState.damageYaw );

		CG_UpdateCarFromPS( &cg.predictedPlayerState );
// END
	}


	// demo playback just copies the moves
// Q3Rally Code Start
	if( cg.snap->ps.pm_flags & PMF_OBSERVE )
	{
		// dont interpolate because we are using velocity for distance
		return;
	}
// END

	if ( cg.demoPlayback || (cg.snap->ps.pm_flags & PMF_FOLLOW) ) {
		CG_InterpolatePlayerState( qfalse );
		return;
	}

	// non-predicting local movement will grab the latest angles
	if ( cg_nopredict.integer || cg_synchronousClients.integer ) {
		CG_InterpolatePlayerState( qtrue );
		return;
	}

// Q3Rally Code Start
//	if (cg_paused.integer) {
//		current = trap_GetCurrentCmdNumber();
//		trap_GetUserCmd( current, &latestCmd );
//		PM_UpdateViewAngles( &cg.predictedPlayerState, &latestCmd, cg_controlMode.integer );
//		return;
//	}
// END

// Q3Rally Code Start
//		Com_Printf( "Command time %i\n", cg.predictedPlayerState.commandTime );
// END

	// prepare for pmove
	cg_pmove.ps = &cg.predictedPlayerState;
	cg_pmove.trace = CG_Trace;
	cg_pmove.pointcontents = CG_PointContents;
	cg_pmove.frictionFunc = CG_FrictionCalc;
// Q3Rally Code Start
/*
	if ( cg_pmove.ps->pm_type == PM_DEAD ) {
		cg_pmove.tracemask = MASK_PLAYERSOLID & ~CONTENTS_BODY;
	}
	else {
		cg_pmove.tracemask = MASK_PLAYERSOLID;
*/
		// dont predict collisions with other cars
		cg_pmove.tracemask = MASK_PLAYERSOLID & ~CONTENTS_BODY;
//	}
// END
	if ( cg.snap->ps.persistant[PERS_TEAM] == TEAM_SPECTATOR ) {
		cg_pmove.tracemask &= ~CONTENTS_BODY;	// spectators can fly through bodies
	}
	cg_pmove.noFootsteps = ( cgs.dmflags & DF_NO_FOOTSTEPS ) > 0;

	// save the state before the pmove so we can detect transitions
	oldPlayerState = cg.predictedPlayerState;

	current = trap_GetCurrentCmdNumber();

	// if we don't have the commands right after the snapshot, we
	// can't accurately predict a current position, so just freeze at
	// the last good position we had
	cmdNum = current - CMD_BACKUP + 1;
	trap_GetUserCmd( cmdNum, &oldestCmd );
	if ( oldestCmd.serverTime > cg.snap->ps.commandTime 
		&& oldestCmd.serverTime < cg.time ) {	// special check for map_restart
		if ( cg_showmiss.integer ) {
			CG_Printf ("exceeded PACKET_BACKUP on commands\n");
		}
		return;
	}

	// get the latest command so we can know which commands are from previous map_restarts
	trap_GetUserCmd( current, &latestCmd );

	// get the most recent information we have, even if
	// the server time is beyond our current cg.time,
	// because predicted player positions are going to 
	// be ahead of everything else anyway
	if ( cg.nextSnap && !cg.nextFrameTeleport && !cg.thisFrameTeleport ) {
// Q3Rally Code Start
		// if we do this then we also need to use the wheel positions at cg.nextsnap time
		// and not the current time
//		cg.predictedPlayerState = cg.nextSnap->ps;
//		CG_UpdateWheelsFromSnapshot( cg.nextSnap );
//		CG_UpdateCarFromPS( &cg.predictedPlayerState );
//		Com_Printf( "nextSnap valid\n" );

		cg.predictedPlayerState.externalEvent = cg.nextSnap->ps.externalEvent;
		cg.predictedPlayerState.externalEventParm = cg.nextSnap->ps.externalEventParm;
		cg.predictedPlayerState.externalEventTime = cg.nextSnap->ps.externalEventTime;
		cg.predictedPlayerState.entityEventSequence = cg.nextSnap->ps.entityEventSequence;
		cg.predictedPlayerState.eventSequence = cg.nextSnap->ps.eventSequence;
		cg.predictedPlayerState.events[0] = cg.nextSnap->ps.events[0];
		cg.predictedPlayerState.events[1] = cg.nextSnap->ps.events[1];
		cg.predictedPlayerState.eventParms[0] = cg.nextSnap->ps.eventParms[0];
		cg.predictedPlayerState.eventParms[1] = cg.nextSnap->ps.eventParms[1];

		// TODO: copy all persistant and stats values
		memcpy( cg.predictedPlayerState.persistant, cg.nextSnap->ps.persistant, sizeof(cg.predictedPlayerState.persistant) );
		memcpy( cg.predictedPlayerState.stats, cg.nextSnap->ps.stats, sizeof(cg.predictedPlayerState.stats) );

//		cg.predictedPlayerState.pm_type = cg.nextSnap->ps.pm_type;
// END
		cg.physicsTime = cg.nextSnap->serverTime;

// Q3Rally Code Start
//		Com_Printf( "3 predictedPlayerState damageYaw %d\n", cg.predictedPlayerState.damageYaw );

//		if ( cg_pmove.ps->pm_type == PM_NORMAL ||
//			cg_pmove.ps->pm_type == PM_DEAD)
//			cg.predictedPlayerState.commandTime = oldPlayerState.commandTime;

		// Since we already know the next state we can just interpolate.
		// There is no need to predict if we already know where the server
		// says we will be!
/*
		if ( !cg_paused.integer && cg_debugpredict.integer )
			Com_Printf( "Interpolating\n" );

		CG_InterpolatePlayerState( qtrue );

		// UPDATE: need this?
		CG_UpdateCarFromPS( &cg.predictedPlayerState );

		CG_TransitionPlayerState( &cg.predictedPlayerState, &oldPlayerState );

		return;
*/
//		CG_UpdateCarFromPS( &cg.predictedPlayerState );
// END
	} else {
// Q3Rally Code Start
		// if we do this then we also need to use the wheel positions at cg.snap time
		// and not the current time
//		cg.predictedPlayerState = cg.snap->ps;
//		CG_UpdateWheelsFromSnapshot( cg.snap );
//		CG_UpdateCarFromPS( &cg.predictedPlayerState );

		cg.predictedPlayerState.externalEvent = cg.snap->ps.externalEvent;
		cg.predictedPlayerState.externalEventParm = cg.snap->ps.externalEventParm;
		cg.predictedPlayerState.externalEventTime = cg.snap->ps.externalEventTime;
		cg.predictedPlayerState.entityEventSequence = cg.snap->ps.entityEventSequence;
		cg.predictedPlayerState.eventSequence = cg.snap->ps.eventSequence;
		cg.predictedPlayerState.events[0] = cg.snap->ps.events[0];
		cg.predictedPlayerState.events[1] = cg.snap->ps.events[1];
		cg.predictedPlayerState.eventParms[0] = cg.snap->ps.eventParms[0];
		cg.predictedPlayerState.eventParms[1] = cg.snap->ps.eventParms[1];

		// TODO: copy all persistant and stats values
		memcpy( cg.predictedPlayerState.persistant, cg.snap->ps.persistant, sizeof(cg.predictedPlayerState.persistant) );
		memcpy( cg.predictedPlayerState.stats, cg.snap->ps.stats, sizeof(cg.predictedPlayerState.stats) );

//		cg.predictedPlayerState.pm_type = cg.snap->ps.pm_type;
// END
		cg.physicsTime = cg.snap->serverTime;
//
// Q3Rally Code Start
//		Com_Printf( "4 predictedPlayerState damageYaw %d\n", cg.predictedPlayerState.damageYaw );

//		if ( cg_pmove.ps->pm_type == PM_NORMAL ||
//			cg_pmove.ps->pm_type == PM_DEAD)
//			cg.predictedPlayerState.commandTime = oldPlayerState.commandTime;
/*
		// UPDATE: extrapolate instead of predicting?
		Com_Printf( "Extrapolating\n" );
		CG_ExtrapolatePlayerState( qtrue );

		CG_UpdateCarFromPS( &cg.predictedPlayerState );

		CG_TransitionPlayerState( &cg.predictedPlayerState, &oldPlayerState );

		return;
*/
//		CG_UpdateCarFromPS( &cg.predictedPlayerState );
// END
	}

// Q3Rally Code Start
//	if ( !cg_paused.integer && cg_debugpredict.integer )
//		Com_Printf( "Predicting\n" );
// END

	if ( pmove_msec.integer < 8 ) {
		trap_Cvar_Set("pmove_msec", "8");
		trap_Cvar_Update(&pmove_msec);
	}
	else if (pmove_msec.integer > 33) {
		trap_Cvar_Set("pmove_msec", "33");
		trap_Cvar_Update(&pmove_msec);
	}

	cg_pmove.pmove_fixed = pmove_fixed.integer;// | cg_pmove_fixed.integer;
	cg_pmove.pmove_msec = pmove_msec.integer;

	// run cmds
	moved = qfalse;
// Q3Rally Code Start
//	if ( cg_pmove.ps->pm_type == PM_NORMAL ||
//		cg_pmove.ps->pm_type == PM_DEAD)
//		cmdNum = current;
//	else
		cmdNum = current - CMD_BACKUP + 1;

	cg_pmove.car = &cg.car;
	cg_pmove.client = qtrue;
	cg_pmove.pDebug = cg.pDebug;
	cg_pmove.controlMode = cgs.clientinfo[cg.predictedPlayerState.clientNum].controlMode;
	cg_pmove.manualShift = cgs.clientinfo[cg.predictedPlayerState.clientNum].manualShift;

	cg_pmove.car_spring = 120;
	cg_pmove.car_shock_up = 12;
	cg_pmove.car_shock_down = 11;
	cg_pmove.car_swaybar = 20;
	cg_pmove.car_wheel = 2400;
	cg_pmove.car_wheel_damp = 140;

	cg_pmove.car_frontweight_dist = 0.5f;
	cg_pmove.car_IT_xScale = 1.0f;
	cg_pmove.car_IT_yScale = 1.0f;
	cg_pmove.car_IT_zScale = 1.0f;
	cg_pmove.car_body_elasticity = CP_BODY_ELASTICITY;

	cg_pmove.car_air_cof = CP_AIR_COF;
	cg_pmove.car_air_frac_to_df = CP_FRAC_TO_DF;
	cg_pmove.car_friction_scale = 1.1f;

//	for ( cmdNum = current - CMD_BACKUP + 1 ; cmdNum <= current ; cmdNum++ ) {
	count = 0;
	skipcount = 0;
	for ( ; cmdNum <= current ; cmdNum++ ) {
// END
		// get the command
		trap_GetUserCmd( cmdNum, &cg_pmove.cmd );

		// don't do anything if the time is before the snapshot player time
		if ( cg_pmove.cmd.serverTime <= cg.predictedPlayerState.commandTime ) {
			continue;
		}

		// don't do anything if the command was from a previous map_restart
		if ( cg_pmove.cmd.serverTime > latestCmd.serverTime ) {
			continue;
		}

// Q3Rally Code Start
		// only skip 10 times (this might be the source of the infinite loop)
		if( cmdNum != current /*&& skipcount < 10*/ )
		{
			trap_GetUserCmd( cmdNum+1, &nextCmd );
			
			// if the current command is the same as the next command then just skip this one
			if( nextCmd.angles[YAW] == cg_pmove.cmd.angles[YAW] &&
				// dont worry about pitch and roll changes in the view because the
				// physics dont use them for movement
//				nextCmd.angles[PITCH] == cg_pmove.cmd.angles[PITCH] &&
//				nextCmd.angles[ROLL] == cg_pmove.cmd.angles[ROLL] &&
				nextCmd.buttons == cg_pmove.cmd.buttons &&
				nextCmd.weapon == cg_pmove.cmd.weapon &&
				nextCmd.forwardmove == cg_pmove.cmd.forwardmove &&
				// rightmove isnt used so dont worry about it
//				nextCmd.rightmove == cg_pmove.cmd.rightmove &&
				nextCmd.upmove == cg_pmove.cmd.upmove )
			{
//				Com_Printf( "Skipping command\n" );
				skipcount++;
				continue;
			}
		}

//		Com_Printf( "Server time %i <- Command time %i\n", cg_pmove.cmd.serverTime, cg.predictedPlayerState.commandTime );
/*
		Com_Printf( "angles %i %i %i\n", cg_pmove.cmd.angles[0], cg_pmove.cmd.angles[1], cg_pmove.cmd.angles[2] );
		Com_Printf( "buttons %i\n", cg_pmove.cmd.buttons );
		Com_Printf( "forwardmove %i\n", cg_pmove.cmd.forwardmove );
		Com_Printf( "rightmove %i\n", cg_pmove.cmd.rightmove );
		Com_Printf( "upmove %i\n", cg_pmove.cmd.upmove );
		Com_Printf( "weapon %i\n", cg_pmove.cmd.weapon );
		Com_Printf( "serverTime %i\n", cg_pmove.cmd.serverTime );
*/
// END


// Q3Rally Code Start
		if ((isRallyRace() || cgs.gametype == GT_DERBY || cgs.gametype == GT_LCS) && !cg_entities[cg.snap->ps.clientNum].startRaceTime){
			cg_pmove.cmd.buttons = BUTTON_HANDBRAKE;

			cg_pmove.cmd.forwardmove = 0;
//			cg_pmove.cmd.rightmove = 0;
			cg_pmove.cmd.upmove = 0;
		}

		if (cg_entities[cg.snap->ps.clientNum].finishRaceTime &&
			cg_entities[cg.snap->ps.clientNum].finishRaceTime + 500 < cg.time &&
			!cg.intermissionStarted){

			cg.predictedPlayerState.weapon = WP_NONE;
//			cg.predictedPlayerState.powerups = 0;

			cg_pmove.cmd.weapon = cg.predictedPlayerState.weapon;
			cg_pmove.cmd.buttons = BUTTON_HANDBRAKE;
			cg_pmove.cmd.forwardmove = 0;
//			cg_pmove.cmd.rightmove = 0;
			cg_pmove.cmd.upmove = 0;
		}

		if (isRallyNonDMRace()/* TEMP DERBY || cgs.gametype == GT_DERBY*/){
			cg_pmove.cmd.weapon = cg.predictedPlayerState.weapon = WP_NONE;
		}
// END

// Q3Rally Code Start
//		if ( cg_pmove.pmove_fixed ) {
//			PM_UpdateViewAngles( cg_pmove.ps, &cg_pmove.cmd );
			// FIXME: this only has to be done because it turns out that the damageYaw
			// and damagePitch cannot be used for view angles because they are only 8 bits
			// over the net.
			// This function overwrites the damageYaw and damagePitch values based on
			// the command and the delta_angles of the player
			// This also sets damageAngles which is not reduced to 8-bit
			PM_UpdateViewAngles( cg_pmove.ps, &cg_pmove.cmd, cg_controlMode.integer );
//		}
// END

		// check for a prediction error from last frame
		// on a lan, this will often be the exact value
		// from the snapshot, but on a wan we will have
		// to predict several commands to get to the point
		// we want to compare
		if ( cg.predictedPlayerState.commandTime == oldPlayerState.commandTime ) {
			vec3_t	delta;
			float	len;

			if ( cg.thisFrameTeleport ) {
				// a teleport will not cause an error decay
				VectorClear( cg.predictedError );
				if ( cg_showmiss.integer ) {
					CG_Printf( "PredictionTeleport\n" );
				}
				cg.thisFrameTeleport = qfalse;
			} else {
				vec3_t adjusted, new_angles;
				CG_AdjustPositionForMover( cg.predictedPlayerState.origin, 
				cg.predictedPlayerState.groundEntityNum, cg.physicsTime, cg.oldTime, adjusted, cg.predictedPlayerState.viewangles, new_angles);

// Q3Rally Code Start
/*
				if ( cg_showmiss.integer ) {
					if (!VectorCompare( oldPlayerState.origin, adjusted )) {
						CG_Printf("prediction error\n");
					}
				}
*/
// END
				VectorSubtract( oldPlayerState.origin, adjusted, delta );
				len = VectorLength( delta );
				if ( len > 0.1 ) {
					if ( cg_showmiss.integer ) {
						CG_Printf("Prediction miss: %f\n", len);
					}
					if ( cg_errorDecay.integer ) {
						int		t;
						float	f;

						t = cg.time - cg.predictedErrorTime;
						f = ( cg_errorDecay.value - t ) / cg_errorDecay.value;
						if ( f < 0 ) {
							f = 0;
						}
						if ( f > 0 && cg_showmiss.integer ) {
							CG_Printf("Double prediction decay: %f\n", f);
						}
						VectorScale( cg.predictedError, f, cg.predictedError );
					} else {
						VectorClear( cg.predictedError );
					}
					VectorAdd( delta, cg.predictedError, cg.predictedError );
					cg.predictedErrorTime = cg.oldTime;

// Q3Rally Code Start
					if ( cg_showmiss.integer ) {
						len = VectorLength( cg.predictedError );
						CG_Printf("Total predictedError: %f\n", len );
					}
// END
				}
			}
		}

		// don't predict gauntlet firing, which is only supposed to happen
		// when it actually inflicts damage
		cg_pmove.gauntletHit = qfalse;

		// UPDATE: check this
		if ( cg_pmove.pmove_fixed ) {
			cg_pmove.cmd.serverTime = ((cg_pmove.cmd.serverTime + pmove_msec.integer-1) / pmove_msec.integer) * pmove_msec.integer;
		}

		Pmove (&cg_pmove);

		moved = qtrue;

		// add push trigger movement effects
		CG_TouchTriggerPrediction();

		// check for predictable events that changed from previous predictions
		//CG_CheckChangedPredictableEvents(&cg.predictedPlayerState);

// Q3Rally Code Start
		count++;
// END
	}

// Q3Rally Code Start
	cent = &cg_entities[cg.snap->ps.clientNum];
	cent->steeringAngle = cg_pmove.car->wheelAngle;
	for (i = 0; i < FIRST_FRAME_POINT; i++){
		cent->wheelSpeeds[i] = cg_pmove.car->sPoints[i].w;
		cent->wheelSkidding[i] = cg_pmove.car->sPoints[i].slipping;
	}
// END

// Q3Rally Code Start
//	Com_Printf( "Server time %i <- Command time %i\n", cg_pmove.cmd.serverTime, cg.predictedPlayerState.commandTime );
//	Com_Printf( "Num predicted commands %i, skipped %i\n", count, skipcount );
// END

	if ( cg_showmiss.integer > 1 ) {
		CG_Printf( "[%i : %i] ", cg_pmove.cmd.serverTime, cg.time );
	}

	if ( !moved ) {
		if ( cg_showmiss.integer ) {
//			CG_Printf( "not moved\n" );
		}
		return;
	}

	// adjust for the movement of the groundentity
	CG_AdjustPositionForMover( cg.predictedPlayerState.origin, 
		cg.predictedPlayerState.groundEntityNum, 
		cg.physicsTime, cg.time, cg.predictedPlayerState.origin, cg.predictedPlayerState.viewangles, cg.predictedPlayerState.viewangles);

	if ( cg_showmiss.integer ) {
		if (cg.predictedPlayerState.eventSequence > oldPlayerState.eventSequence + MAX_PS_EVENTS) {
			CG_Printf("WARNING: dropped event\n");
		}
	}

// Q3Rally Code Start
//	Com_Printf( "Event 0: %i - %s\n", cg.predictedPlayerState.events[0] & ~EV_EVENT_BITS, eventnames[cg.predictedPlayerState.events[0] & ~EV_EVENT_BITS] );
//	Com_Printf( "Event 1: %i - %s\n", cg.predictedPlayerState.events[1] & ~EV_EVENT_BITS, eventnames[cg.predictedPlayerState.events[1] & ~EV_EVENT_BITS] );
//	Com_Printf( "Event x: %i - %s\n", cg.predictedPlayerState.externalEvent & ~EV_EVENT_BITS, eventnames[cg.predictedPlayerState.externalEvent & ~EV_EVENT_BITS] );
// END

	// fire events and other transition triggered things
	CG_TransitionPlayerState( &cg.predictedPlayerState, &oldPlayerState );

// Q3Rally Code Start
	if ( cg_showmiss.integer || ( !cg_paused.integer && cg_debugpredict.integer ) ) {
//	if ( cg_showmiss.integer ) {
// END
		if (cg.eventSequence > cg.predictedPlayerState.eventSequence) {
			CG_Printf("WARNING: double event\n");
			cg.eventSequence = cg.predictedPlayerState.eventSequence;
		}
	}
}


