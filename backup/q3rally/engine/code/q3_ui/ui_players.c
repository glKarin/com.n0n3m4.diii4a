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
// ui_players.c

#include "ui_local.h"


#define UI_TIMER_GESTURE		2300
#define UI_TIMER_JUMP			1000
#define UI_TIMER_LAND			130
#define UI_TIMER_WEAPON_SWITCH	300
#define UI_TIMER_ATTACK			500
#define	UI_TIMER_MUZZLE_FLASH	20
#define	UI_TIMER_WEAPON_DELAY	250

#define JUMP_HEIGHT				56

#define SWINGSPEED				0.3f

#define SPIN_SPEED				0.9f
#define COAST_TIME				1000


static int			dp_realtime;
static float		jumpHeight;

// STONELANCE
static float		roll;
static float		yaw;
static float		deltaYaw	= 60;
static float		deltaRoll;

/*
===============
UI_PlayerInfo_SetWeapon
===============
*/
static void UI_PlayerInfo_SetWeapon( playerInfo_t *pi, weapon_t weaponNum ) {
	gitem_t *	item;
	char		path[MAX_QPATH];

	pi->currentWeapon = weaponNum;
tryagain:
	pi->realWeapon = weaponNum;
	pi->weaponModel = 0;
	pi->barrelModel = 0;
	pi->flashModel = 0;

	if ( weaponNum == WP_NONE ) {
		return;
	}

	for ( item = bg_itemlist + 1; item->classname ; item++ ) {
		if ( item->giType != IT_WEAPON ) {
			continue;
		}
		if ( item->giTag == weaponNum ) {
			break;
		}
	}

	if ( item->classname ) {
		pi->weaponModel = trap_R_RegisterModel( item->world_model[0] );
	}

	if( pi->weaponModel == 0 ) {
		if( weaponNum == WP_MACHINEGUN ) {
			weaponNum = WP_NONE;
			goto tryagain;
		}
		weaponNum = WP_MACHINEGUN;
		goto tryagain;
	}

	if ( weaponNum == WP_MACHINEGUN || weaponNum == WP_GAUNTLET || weaponNum == WP_BFG ) {
		strcpy( path, item->world_model[0] );
		COM_StripExtension( path, path, sizeof(path) );
		strcat( path, "_barrel.md3" );
		pi->barrelModel = trap_R_RegisterModel( path );
	}

	strcpy( path, item->world_model[0] );
	COM_StripExtension( path, path, sizeof(path) );
	strcat( path, "_flash.md3" );
	pi->flashModel = trap_R_RegisterModel( path );

	switch( weaponNum ) {
	case WP_GAUNTLET:
		MAKERGB( pi->flashDlightColor, 0.6f, 0.6f, 1 );
		break;

	case WP_MACHINEGUN:
		MAKERGB( pi->flashDlightColor, 1, 1, 0 );
		break;

	case WP_SHOTGUN:
		MAKERGB( pi->flashDlightColor, 1, 1, 0 );
		break;

	case WP_GRENADE_LAUNCHER:
		MAKERGB( pi->flashDlightColor, 1, 0.7f, 0.5f );
		break;

	case WP_ROCKET_LAUNCHER:
		MAKERGB( pi->flashDlightColor, 1, 0.75f, 0 );
		break;

	case WP_LIGHTNING:
		MAKERGB( pi->flashDlightColor, 0.6f, 0.6f, 1 );
		break;

	case WP_RAILGUN:
		MAKERGB( pi->flashDlightColor, 1, 0.5f, 0 );
		break;

	case WP_PLASMAGUN:
		MAKERGB( pi->flashDlightColor, 0.6f, 0.6f, 1 );
		break;

	case WP_BFG:
		MAKERGB( pi->flashDlightColor, 1, 0.7f, 1 );
		break;
	
  case WP_FLAME_THROWER:
	  MAKERGB( pi->flashDlightColor, 0.6f, 0.6f, 1 );
    break;

// STONELANCE
/*
	case WP_GRAPPLING_HOOK:
		MAKERGB( pi->flashDlightColor, 0.6f, 0.6f, 1 );
		break;
*/
// END

	default:
		MAKERGB( pi->flashDlightColor, 1, 1, 1 );
		break;
	}
}


/*
===============
UI_ForceLegsAnim
===============
*/
// STONELANCE - removed
/*
static void UI_ForceLegsAnim( playerInfo_t *pi, int anim ) {
	pi->legsAnim = ( ( pi->legsAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) | anim;

	if ( anim == LEGS_JUMP ) {
		pi->legsAnimationTimer = UI_TIMER_JUMP;
	}
}
*/
// END


/*
===============
UI_SetLegsAnim
===============
*/
// STONELANCE - removed
/*
static void UI_SetLegsAnim( playerInfo_t *pi, int anim ) {
	if ( pi->pendingLegsAnim ) {
		anim = pi->pendingLegsAnim;
		pi->pendingLegsAnim = 0;
	}
	UI_ForceLegsAnim( pi, anim );
}
*/
// END


/*
===============
UI_ForceTorsoAnim
===============
*/
// STONELANCE - removed
/*
static void UI_ForceTorsoAnim( playerInfo_t *pi, int anim ) {
	pi->torsoAnim = ( ( pi->torsoAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) | anim;

	if ( anim == TORSO_GESTURE ) {
		pi->torsoAnimationTimer = UI_TIMER_GESTURE;
	}

	if ( anim == TORSO_ATTACK || anim == TORSO_ATTACK2 ) {
		pi->torsoAnimationTimer = UI_TIMER_ATTACK;
	}
}
*/
// END


/*
===============
UI_SetTorsoAnim
===============
*/
// STONELANCE - removed
/*
static void UI_SetTorsoAnim( playerInfo_t *pi, int anim ) {
	if ( pi->pendingTorsoAnim ) {
		anim = pi->pendingTorsoAnim;
		pi->pendingTorsoAnim = 0;
	}

	UI_ForceTorsoAnim( pi, anim );
}
*/
// END


/*
===============
UI_TorsoSequencing
===============
*/
// STONELANCE - removed
/*
static void UI_TorsoSequencing( playerInfo_t *pi ) {
	int		currentAnim;

	currentAnim = pi->torsoAnim & ~ANIM_TOGGLEBIT;

	if ( pi->weapon != pi->currentWeapon ) {
		if ( currentAnim != TORSO_DROP ) {
			pi->torsoAnimationTimer = UI_TIMER_WEAPON_SWITCH;
			UI_ForceTorsoAnim( pi, TORSO_DROP );
		}
	}

	if ( pi->torsoAnimationTimer > 0 ) {
		return;
	}

	if( currentAnim == TORSO_GESTURE ) {
		UI_SetTorsoAnim( pi, TORSO_STAND );
		return;
	}

	if( currentAnim == TORSO_ATTACK || currentAnim == TORSO_ATTACK2 ) {
		UI_SetTorsoAnim( pi, TORSO_STAND );
		return;
	}

	if ( currentAnim == TORSO_DROP ) {
		UI_PlayerInfo_SetWeapon( pi, pi->weapon );
		pi->torsoAnimationTimer = UI_TIMER_WEAPON_SWITCH;
		UI_ForceTorsoAnim( pi, TORSO_RAISE );
		return;
	}

	if ( currentAnim == TORSO_RAISE ) {
		UI_SetTorsoAnim( pi, TORSO_STAND );
		return;
	}
}
*/
// END


/*
===============
UI_LegsSequencing
===============
*/
// STONELANCE - removed
/*
static void UI_LegsSequencing( playerInfo_t *pi ) {
	int		currentAnim;

	currentAnim = pi->legsAnim & ~ANIM_TOGGLEBIT;

	if ( pi->legsAnimationTimer > 0 ) {
		if ( currentAnim == LEGS_JUMP ) {
			jumpHeight = JUMP_HEIGHT * sin( M_PI * ( UI_TIMER_JUMP - pi->legsAnimationTimer ) / UI_TIMER_JUMP );
		}
		return;
	}

	if ( currentAnim == LEGS_JUMP ) {
		UI_ForceLegsAnim( pi, LEGS_LAND );
		pi->legsAnimationTimer = UI_TIMER_LAND;
		jumpHeight = 0;
		return;
	}

	if ( currentAnim == LEGS_LAND ) {
		UI_SetLegsAnim( pi, LEGS_IDLE );
		return;
	}
}
*/
// END


// STONELANCE
/*
======================
UI_TagExists

Returns true if the tag is not in the model.. ie: not at the vec3_origin
======================
*/
/*
qboolean UI_TagExists( const refEntity_t *parent, qhandle_t parentModel, char *tagName ) {
	orientation_t	lerped;
	
	// lerp the tag
	trap_R_LerpTag( &lerped, parentModel, parent->oldframe, parent->frame,
		1.0 - parent->backlerp, tagName );

	if (VectorLength(lerped.origin))
		return qtrue;
	else
		return qfalse;
}
*/
// uses frame 0
qboolean UI_TagExists( qhandle_t parentModel, char *tagName ) {
	orientation_t	lerped;
	
	// lerp the tag
	trap_CM_LerpTag( &lerped, parentModel, 0, 0, 1.0f, tagName );

	if( VectorLength( lerped.origin ) == 0.0f &&
		lerped.axis[0][0] == 1.0f &&
		lerped.axis[1][1] == 1.0f &&
		lerped.axis[2][2] == 1.0f )
	{
		return qfalse;
	}
	else
		return qtrue;
}


/*
======================
UI_GetTagPosition

Returns the position of the specified tag
======================
*/
void UI_GetTagPosition( const refEntity_t *parent, qhandle_t parentModel, char *tagName, vec3_t origin ) {
	int				i;
	orientation_t	lerped;
	
	// lerp the tag
	trap_CM_LerpTag( &lerped, parentModel, parent->oldframe, parent->frame,
		1.0 - parent->backlerp, tagName );

	// FIXME: allow origin offsets along tag?
	VectorCopy( parent->origin, origin );
	for ( i = 0 ; i < 3 ; i++ ) {
		VectorMA( origin, lerped.origin[i], parent->axis[i], origin );
	}
}
// END


/*
======================
UI_PositionEntityOnTag
======================
*/
static void UI_PositionEntityOnTag( refEntity_t *entity, const refEntity_t *parent, 
							clipHandle_t parentModel, char *tagName ) {
	int				i;
	orientation_t	lerped;
	
	// lerp the tag
	trap_CM_LerpTag( &lerped, parentModel, parent->oldframe, parent->frame,
		1.0 - parent->backlerp, tagName );

	// FIXME: allow origin offsets along tag?
	VectorCopy( parent->origin, entity->origin );
	for ( i = 0 ; i < 3 ; i++ ) {
		VectorMA( entity->origin, lerped.origin[i], parent->axis[i], entity->origin );
	}

	// cast away const because of compiler problems
	MatrixMultiply( lerped.axis, ((refEntity_t*)parent)->axis, entity->axis );
	entity->backlerp = parent->backlerp;
}


/*
======================
UI_PositionRotatedEntityOnTag
======================
*/
static void UI_PositionRotatedEntityOnTag( refEntity_t *entity, const refEntity_t *parent, 
							clipHandle_t parentModel, char *tagName ) {
	int				i;
	orientation_t	lerped;
	vec3_t			tempAxis[3];

	// lerp the tag
	trap_CM_LerpTag( &lerped, parentModel, parent->oldframe, parent->frame,
		1.0 - parent->backlerp, tagName );

	// FIXME: allow origin offsets along tag?
	VectorCopy( parent->origin, entity->origin );
	for ( i = 0 ; i < 3 ; i++ ) {
		VectorMA( entity->origin, lerped.origin[i], parent->axis[i], entity->origin );
	}

	// cast away const because of compiler problems
	MatrixMultiply( entity->axis, ((refEntity_t *)parent)->axis, tempAxis );
	MatrixMultiply( lerped.axis, tempAxis, entity->axis );
}


/*
===============
UI_SetLerpFrameAnimation
===============
*/
// STONELANCE - removed
/*
static void UI_SetLerpFrameAnimation( playerInfo_t *ci, lerpFrame_t *lf, int newAnimation ) {
	animation_t	*anim;

	lf->animationNumber = newAnimation;
	newAnimation &= ~ANIM_TOGGLEBIT;

	if ( newAnimation < 0 || newAnimation >= MAX_ANIMATIONS ) {
		trap_Error( va("Bad animation number: %i", newAnimation) );
	}

	anim = &ci->animations[ newAnimation ];

	lf->animation = anim;
	lf->animationTime = lf->frameTime + anim->initialLerp;
}
*/
// END


/*
===============
UI_RunLerpFrame
===============
*/
// STONELANCE - removed
/*
static void UI_RunLerpFrame( playerInfo_t *ci, lerpFrame_t *lf, int newAnimation ) {
	int			f, numFrames;
	animation_t	*anim;

	// see if the animation sequence is switching
	if ( newAnimation != lf->animationNumber || !lf->animation ) {
		UI_SetLerpFrameAnimation( ci, lf, newAnimation );
	}

	// if we have passed the current frame, move it to
	// oldFrame and calculate a new frame
	if ( dp_realtime >= lf->frameTime ) {
		lf->oldFrame = lf->frame;
		lf->oldFrameTime = lf->frameTime;

		// get the next frame based on the animation
		anim = lf->animation;
		if ( !anim->frameLerp ) {
			return;		// shouldn't happen
		}
		if ( dp_realtime < lf->animationTime ) {
			lf->frameTime = lf->animationTime;		// initial lerp
		} else {
			lf->frameTime = lf->oldFrameTime + anim->frameLerp;
		}
		f = ( lf->frameTime - lf->animationTime ) / anim->frameLerp;

		numFrames = anim->numFrames;
		if (anim->flipflop) {
			numFrames *= 2;
		}
		if ( f >= numFrames ) {
			f -= numFrames;
			if ( anim->loopFrames ) {
				f %= anim->loopFrames;
				f += anim->numFrames - anim->loopFrames;
			} else {
				f = numFrames - 1;
				// the animation is stuck at the end, so it
				// can immediately transition to another sequence
				lf->frameTime = dp_realtime;
			}
		}
		if ( anim->reversed ) {
			lf->frame = anim->firstFrame + anim->numFrames - 1 - f;
		}
		else if (anim->flipflop && f>=anim->numFrames) {
			lf->frame = anim->firstFrame + anim->numFrames - 1 - (f%anim->numFrames);
		}
		else {
			lf->frame = anim->firstFrame + f;
		}
		if ( dp_realtime > lf->frameTime ) {
			lf->frameTime = dp_realtime;
		}
	}

	if ( lf->frameTime > dp_realtime + 200 ) {
		lf->frameTime = dp_realtime;
	}

	if ( lf->oldFrameTime > dp_realtime ) {
		lf->oldFrameTime = dp_realtime;
	}
	// calculate current lerp value
	if ( lf->frameTime == lf->oldFrameTime ) {
		lf->backlerp = 0;
	} else {
		lf->backlerp = 1.0 - (float)( dp_realtime - lf->oldFrameTime ) / ( lf->frameTime - lf->oldFrameTime );
	}
}
*/
// END


/*
===============
UI_PlayerAnimation
===============
*/
// STONELANCE - removed
/*
static void UI_PlayerAnimation( playerInfo_t *pi, int *legsOld, int *legs, float *legsBackLerp,
						int *torsoOld, int *torso, float *torsoBackLerp ) {

	// legs animation
	pi->legsAnimationTimer -= uis.frametime;
	if ( pi->legsAnimationTimer < 0 ) {
		pi->legsAnimationTimer = 0;
	}

	UI_LegsSequencing( pi );

	if ( pi->legs.yawing && ( pi->legsAnim & ~ANIM_TOGGLEBIT ) == LEGS_IDLE ) {
		UI_RunLerpFrame( pi, &pi->legs, LEGS_TURN );
	} else {
		UI_RunLerpFrame( pi, &pi->legs, pi->legsAnim );
	}
	*legsOld = pi->legs.oldFrame;
	*legs = pi->legs.frame;
	*legsBackLerp = pi->legs.backlerp;

	// torso animation
	pi->torsoAnimationTimer -= uis.frametime;
	if ( pi->torsoAnimationTimer < 0 ) {
		pi->torsoAnimationTimer = 0;
	}

	UI_TorsoSequencing( pi );

	UI_RunLerpFrame( pi, &pi->torso, pi->torsoAnim );
	*torsoOld = pi->torso.oldFrame;
	*torso = pi->torso.frame;
	*torsoBackLerp = pi->torso.backlerp;
}
*/
// END


/*
==================
UI_SwingAngles
==================
*/
// STONELANCE - removed
/*
static void UI_SwingAngles( float destination, float swingTolerance, float clampTolerance,
					float speed, float *angle, qboolean *swinging ) {
	float	swing;
	float	move;
	float	scale;

	if ( !*swinging ) {
		// see if a swing should be started
		swing = AngleSubtract( *angle, destination );
		if ( swing > swingTolerance || swing < -swingTolerance ) {
			*swinging = qtrue;
		}
	}

	if ( !*swinging ) {
		return;
	}
	
	// modify the speed depending on the delta
	// so it doesn't seem so linear
	swing = AngleSubtract( destination, *angle );
	scale = fabs( swing );
	if ( scale < swingTolerance * 0.5 ) {
		scale = 0.5;
	} else if ( scale < swingTolerance ) {
		scale = 1.0;
	} else {
		scale = 2.0;
	}

	// swing towards the destination angle
	if ( swing >= 0 ) {
		move = uis.frametime * scale * speed;
		if ( move >= swing ) {
			move = swing;
			*swinging = qfalse;
		}
		*angle = AngleMod( *angle + move );
	} else if ( swing < 0 ) {
		move = uis.frametime * scale * -speed;
		if ( move <= swing ) {
			move = swing;
			*swinging = qfalse;
		}
		*angle = AngleMod( *angle + move );
	}

	// clamp to no more than tolerance
	swing = AngleSubtract( destination, *angle );
	if ( swing > clampTolerance ) {
		*angle = AngleMod( destination - (clampTolerance - 1) );
	} else if ( swing < -clampTolerance ) {
		*angle = AngleMod( destination + (clampTolerance - 1) );
	}
}
*/
// END


/*
======================
UI_MovedirAdjustment
======================
*/
// STONELANCE
/*
static float UI_MovedirAdjustment( playerInfo_t *pi ) {
	vec3_t		relativeAngles;
	vec3_t		moveVector;

	VectorSubtract( pi->viewAngles, pi->moveAngles, relativeAngles );
	AngleVectors( relativeAngles, moveVector, NULL, NULL );
	if ( Q_fabs( moveVector[0] ) < 0.01 ) {
		moveVector[0] = 0.0;
	}
	if ( Q_fabs( moveVector[1] ) < 0.01 ) {
		moveVector[1] = 0.0;
	}

	if ( moveVector[1] == 0 && moveVector[0] > 0 ) {
		return 0;
	}
	if ( moveVector[1] < 0 && moveVector[0] > 0 ) {
		return 22;
	}
	if ( moveVector[1] < 0 && moveVector[0] == 0 ) {
		return 45;
	}
	if ( moveVector[1] < 0 && moveVector[0] < 0 ) {
		return -22;
	}
	if ( moveVector[1] == 0 && moveVector[0] < 0 ) {
		return 0;
	}
	if ( moveVector[1] > 0 && moveVector[0] < 0 ) {
		return 22;
	}
	if ( moveVector[1] > 0 && moveVector[0] == 0 ) {
		return  -45;
	}

	return -22;
}
*/
// END


/*
===============
UI_PlayerAngles
===============
*/
// STONELANCE - removed
/*
static void UI_PlayerAngles( playerInfo_t *pi, vec3_t legs[3], vec3_t torso[3], vec3_t head[3] ) {
	vec3_t		legsAngles, torsoAngles, headAngles;
	float		dest;
	float		adjust;

	VectorCopy( pi->viewAngles, headAngles );
	headAngles[YAW] = AngleMod( headAngles[YAW] );
	VectorClear( legsAngles );
	VectorClear( torsoAngles );

	// --------- yaw -------------

	// allow yaw to drift a bit
	if ( ( pi->legsAnim & ~ANIM_TOGGLEBIT ) != LEGS_IDLE 
		|| ( pi->torsoAnim & ~ANIM_TOGGLEBIT ) != TORSO_STAND  ) {
		// if not standing still, always point all in the same direction
		pi->torso.yawing = qtrue;	// always center
		pi->torso.pitching = qtrue;	// always center
		pi->legs.yawing = qtrue;	// always center
	}

	// adjust legs for movement dir
	adjust = UI_MovedirAdjustment( pi );
	legsAngles[YAW] = headAngles[YAW] + adjust;
	torsoAngles[YAW] = headAngles[YAW] + 0.25 * adjust;


	// torso
	UI_SwingAngles( torsoAngles[YAW], 25, 90, SWINGSPEED, &pi->torso.yawAngle, &pi->torso.yawing );
	UI_SwingAngles( legsAngles[YAW], 40, 90, SWINGSPEED, &pi->legs.yawAngle, &pi->legs.yawing );

	torsoAngles[YAW] = pi->torso.yawAngle;
	legsAngles[YAW] = pi->legs.yawAngle;

	// --------- pitch -------------

	// only show a fraction of the pitch angle in the torso
	if ( headAngles[PITCH] > 180 ) {
		dest = (-360 + headAngles[PITCH]) * 0.75;
	} else {
		dest = headAngles[PITCH] * 0.75;
	}
	UI_SwingAngles( dest, 15, 30, 0.1f, &pi->torso.pitchAngle, &pi->torso.pitching );
	torsoAngles[PITCH] = pi->torso.pitchAngle;

	if ( pi->fixedtorso ) {
		torsoAngles[PITCH] = 0.0f;
	}

	if ( pi->fixedlegs ) {
		legsAngles[YAW] = torsoAngles[YAW];
		legsAngles[PITCH] = 0.0f;
		legsAngles[ROLL] = 0.0f;
	}

	// pull the angles back out of the hierarchial chain
	AnglesSubtract( headAngles, torsoAngles, headAngles );
	AnglesSubtract( torsoAngles, legsAngles, torsoAngles );
	AnglesToAxis( legsAngles, legs );
	AnglesToAxis( torsoAngles, torso );
	AnglesToAxis( headAngles, head );
}
*/
// END


/*
===============
UI_PlayerFloatSprite
===============
*/
static void UI_PlayerFloatSprite( playerInfo_t *pi, vec3_t origin, qhandle_t shader ) {
	refEntity_t		ent;

	memset( &ent, 0, sizeof( ent ) );
	VectorCopy( origin, ent.origin );
	ent.origin[2] += 48;
	ent.reType = RT_SPRITE;
	ent.customShader = shader;
	ent.radius = 10;
	ent.renderfx = 0;
	trap_R_AddRefEntityToScene( &ent );
}


/*
======================
UI_MachinegunSpinAngle
======================
*/
float	UI_MachinegunSpinAngle( playerInfo_t *pi ) {
	int		delta;
	float	angle;
	float	speed;
	int		torsoAnim;

	delta = dp_realtime - pi->barrelTime;
	if ( pi->barrelSpinning ) {
		angle = pi->barrelAngle + delta * SPIN_SPEED;
	} else {
		if ( delta > COAST_TIME ) {
			delta = COAST_TIME;
		}

		speed = 0.5 * ( SPIN_SPEED + (float)( COAST_TIME - delta ) / COAST_TIME );
		angle = pi->barrelAngle + delta * speed;
	}

	torsoAnim = pi->torsoAnim  & ~ANIM_TOGGLEBIT;
	if( torsoAnim == TORSO_ATTACK2 ) {
		torsoAnim = TORSO_ATTACK;
	}
	if ( pi->barrelSpinning == !(torsoAnim == TORSO_ATTACK) ) {
		pi->barrelTime = dp_realtime;
		pi->barrelAngle = AngleMod( angle );
		pi->barrelSpinning = !!(torsoAnim == TORSO_ATTACK);
	}

	return angle;
}


/*
===============
UI_AddWheels

Adds the wheels to the car body.
===============
*/
// FIXME: could increase the speed by only looking for components that
// need to be drawn one time, when the model is loaded
static void UI_AddWheels( playerInfo_t *pi, refEntity_t *body, float angle )
{
	refEntity_t		wheel_fl, wheel_fr, wheel_rl, wheel_rr;
	refEntity_t		susp_cl, susp_cr;
//	refEntity_t		susp_fl, susp_fr;
	vec3_t			whlAngles;

	memset( &wheel_fl, 0, sizeof(wheel_fl) );
	memset( &wheel_fr, 0, sizeof(wheel_fr) );
	memset( &wheel_rl, 0, sizeof(wheel_rl) );
	memset( &wheel_rr, 0, sizeof(wheel_rr) );

	VectorClear(whlAngles);
	whlAngles[YAW] = angle;

	VectorCopy( body->lightingOrigin, wheel_fl.lightingOrigin );
	VectorCopy( body->lightingOrigin, wheel_fr.lightingOrigin );
	VectorCopy( body->lightingOrigin, wheel_rl.lightingOrigin );
	VectorCopy( body->lightingOrigin, wheel_rr.lightingOrigin );

	// front left wheel
	wheel_fl.hModel      = pi->wheelModel;
	if (UI_TagExists(body->hModel, "tag_wheelfl") && wheel_fl.hModel){
		whlAngles[ROLL] = 0;

		if( UI_TagExists( wheel_fl.hModel, "tag_polygonwheel" ) )
			wheel_fl.customSkin  = 0;
		else
			wheel_fl.customSkin  = pi->wheelSkin;
		wheel_fl.shadowPlane = body->shadowPlane;
		wheel_fl.renderfx    = body->renderfx;

		AnglesToAxis(whlAngles, wheel_fl.axis);
		UI_PositionRotatedEntityOnTag( &wheel_fl, body, body->hModel, "tag_wheelfl" );

		// move for suspension
//		if (cent->currentState.clientNum == cg.snap->ps.clientNum)
//			VectorMA(wheel_fl.origin, 8.2 - cg.snap->ps.legsTimer / 10.0f, body->axis[2], wheel_fl.origin);
		trap_R_AddRefEntityToScene( &wheel_fl );

		// skids
//		if (!(cent->currentState.eFlags & EF_DEAD))
//			UI_SurfaceEffects(cent2, wheel_fl.origin, body->axis[2], 0);
	}

	// front right wheel
	wheel_fr.hModel      = pi->wheelModel;
	if (UI_TagExists(body->hModel, "tag_wheelfr") && wheel_fr.hModel){
		whlAngles[ROLL] = 0;

		if( UI_TagExists( wheel_fr.hModel, "tag_polygonwheel" ) )
			wheel_fr.customSkin  = 0;
		else
			wheel_fr.customSkin  = pi->wheelSkin;
		wheel_fr.shadowPlane = body->shadowPlane;
		wheel_fr.renderfx    = body->renderfx;

		AnglesToAxis(whlAngles, wheel_fr.axis);
		UI_PositionRotatedEntityOnTag( &wheel_fr, body, body->hModel, "tag_wheelfr" );
		
		// move for suspension
//		if (cent->currentState.clientNum == cg.snap->ps.clientNum)
//			VectorMA(wheel_fr.origin, 8.2 - cg.snap->ps.legsAnim / 10.0f, body->axis[2], wheel_fr.origin);
		trap_R_AddRefEntityToScene( &wheel_fr );

		// skids
//		if (!(cent->currentState.eFlags & EF_DEAD))
//			CG_SurfaceEffects(cent2, wheel_fr.origin, body->axis[2], 1);
	}

	whlAngles[YAW] = 0;

	// rear left wheel
	wheel_rl.hModel      = pi->wheelModel;
	if (UI_TagExists(body->hModel, "tag_wheelrl") && wheel_rl.hModel){
		whlAngles[ROLL] = 0;

		if( UI_TagExists( wheel_rl.hModel, "tag_polygonwheel" ) )
			wheel_rl.customSkin  = 0;
		else
			wheel_rl.customSkin  = pi->wheelSkin;
		wheel_rl.shadowPlane = body->shadowPlane;
		wheel_rl.renderfx    = body->renderfx;

		AnglesToAxis(whlAngles, wheel_rl.axis);
		UI_PositionRotatedEntityOnTag( &wheel_rl, body, body->hModel, "tag_wheelrl" );

		// move for suspension
//		if (cent->currentState.clientNum == cg.snap->ps.clientNum)
//			VectorMA(wheel_rl.origin, 8.2 - cg.snap->ps.torsoTimer / 10.0f, body->axis[2], wheel_rl.origin);
		trap_R_AddRefEntityToScene( &wheel_rl );

		// skids
//		if (!(cent->currentState.eFlags & EF_DEAD))
//			CG_SurfaceEffects(cent2, wheel_rl.origin, body->axis[2], 2);
	}

	// rear right wheel
	wheel_rr.hModel      = pi->wheelModel;
	if (UI_TagExists(body->hModel, "tag_wheelrr") && wheel_rr.hModel){
		whlAngles[ROLL] = 0;

		if( UI_TagExists( wheel_rr.hModel, "tag_polygonwheel" ) )
			wheel_rr.customSkin  = 0;
		else
			wheel_rr.customSkin  = pi->wheelSkin;
		wheel_rr.shadowPlane = body->shadowPlane;
		wheel_rr.renderfx    = body->renderfx;
		AnglesToAxis(whlAngles, wheel_rr.axis);
		UI_PositionRotatedEntityOnTag( &wheel_rr, body, body->hModel, "tag_wheelrr" );

		// move for suspension
//		if (cent->currentState.clientNum == cg.snap->ps.clientNum)
//			VectorMA(wheel_rr.origin, 8.2 - cg.snap->ps.torsoAnim / 10.0f, body->axis[2], wheel_rr.origin);
		trap_R_AddRefEntityToScene( &wheel_rr );

		// skids
//		if (!(cent->currentState.eFlags & EF_DEAD))
//			CG_SurfaceEffects(cent2, wheel_rr.origin, body->axis[2], 3);
	}

	memset( &susp_cl, 0, sizeof(susp_cl) );
	memset( &susp_cr, 0, sizeof(susp_cr) );
//	memset( &susp_fl, 0, sizeof(susp_fl) );
//	memset( &susp_fr, 0, sizeof(susp_fr) );

	susp_cl.hModel      = pi->suspCModel;
	if (UI_TagExists(body->hModel, "tag_suspcl") && susp_cl.hModel){
		susp_cl.shadowPlane = body->shadowPlane;
		susp_cl.renderfx    = body->renderfx;

		VectorClear(whlAngles);
//		whlAngles[PITCH] = -180 / M_PI * atan2((8.2 - cg.snap->ps.legsTimer / 10.0f) - (8.2 - cg.snap->ps.torsoTimer / 10.0f), 2*WHEEL_FORWARD);
		AnglesToAxis(whlAngles, susp_cl.axis);
		UI_PositionRotatedEntityOnTag( &susp_cl, body, body->hModel, "tag_suspcl" );
		
//		if (cent->currentState.clientNum == cg.snap->ps.clientNum)
//			VectorMA(susp_cl.origin, ((8.2 - cg.snap->ps.legsTimer / 10.0f) + (8.2 - cg.snap->ps.torsoTimer / 10.0f))/2, body->axis[2], susp_cl.origin);
		trap_R_AddRefEntityToScene( &susp_cl );
	}

	susp_cr.hModel      = pi->suspCModel;
	if (UI_TagExists(body->hModel, "tag_suspcr") && susp_cr.hModel){
		susp_cr.shadowPlane = body->shadowPlane;
		susp_cr.renderfx    = body->renderfx;

		VectorClear(whlAngles);
//		whlAngles[PITCH] = -180 / M_PI * atan2((8.2 - cg.snap->ps.legsAnim / 10.0f) - (8.2 - cg.snap->ps.torsoAnim / 10.0f), 2*WHEEL_FORWARD);
		AnglesToAxis(whlAngles, susp_cr.axis);
		UI_PositionRotatedEntityOnTag( &susp_cr, body, body->hModel, "tag_suspcr" );
		
//		if (cent->currentState.clientNum == cg.snap->ps.clientNum)
//			VectorMA(susp_cl.origin, ((8.2 - cg.snap->ps.legsAnim / 10.0f) + (8.2 - cg.snap->ps.torsoAnim / 10.0f))/2, body->axis[2], susp_cl.origin);
		trap_R_AddRefEntityToScene( &susp_cr );
	}

/*
	if (CG_TagExists(body->hModel, "tag_suspfr")){
		susp_fr.hModel      = trap_R_RegisterModel("models/players/reaper/suspf.md3");
		susp_fr.customShader = trap_R_RegisterShader("models/players/reaper/parts_blue.tga");
		susp_fr.shadowPlane = body->shadowPlane;
		susp_fr.renderfx    = body->renderfx;

		CG_GetTagPosition(body, body->hModel, "tag_suspfr", susp_fr.origin);
		VectorSubtract(wheel_fr.origin, susp_fr.origin, delta);

		VectorClear(whlAngles);
		whlAngles[PITCH] = -180 / M_PI * atan2(8.2 - cg.snap->ps.legsAnim / 10.0f, -DotProduct(delta, body->axis[1]));
		AnglesToAxis(whlAngles, susp_fr.axis);
		CG_PositionRotatedEntityOnTag( &susp_fr, body, body->hModel, "tag_suspfr" );
		
		trap_R_AddRefEntityToScene( &susp_fr );
	}

	if (UI_TagExists(body->hModel, "tag_suspfl")){
		susp_fl.hModel      = trap_R_RegisterModel("models/players/reaper/suspf.md3");
		susp_fl.customShader = trap_R_RegisterShader("models/players/reaper/parts_blue.tga");
		susp_fl.shadowPlane = body->shadowPlane;
		susp_fl.renderfx    = body->renderfx;

		UI_GetTagPosition(body, body->hModel, "tag_suspfl", susp_fl.origin);
		VectorSubtract(wheel_fl.origin, susp_fl.origin, delta);

		VectorClear(whlAngles);
		whlAngles[PITCH] = 180 / M_PI * atan2(8.2 - cg.snap->ps.legsTimer / 10.0f, DotProduct(delta, body->axis[1]));
		AnglesToAxis(whlAngles, susp_fl.axis);
		UI_PositionRotatedEntityOnTag( &susp_fl, body, body->hModel, "tag_suspfl" );
		
		trap_R_AddRefEntityToScene( &susp_fl );
	}
*/
}
// END

/*
===============
UI_DrawPlayer
===============
*/
void UI_DrawPlayer( float x, float y, float w, float h, playerInfo_t *pi, int time ) {
	refdef_t		refdef;
// STONELANCE
	refEntity_t		body;
/*
	refEntity_t		legs;
	refEntity_t		torso;
*/
	refEntity_t		plate;
	refEntity_t		turbo;
	refEntity_t		headlight;
	refEntity_t		brakelight;
	refEntity_t		reverselight;

	char			filename[MAX_QPATH];
	vec3_t			angles;
	int				i;
// END
	refEntity_t		head;
	refEntity_t		gun;
	refEntity_t		barrel;
	refEntity_t		flash;
	vec3_t			origin;
	int				renderfx;
	vec3_t			mins = {-16, -16, -24};
	vec3_t			maxs = {16, 16, 32};
//	float			len;
	float			xx;

// STONELANCE
//	if ( !pi->legsModel || !pi->torsoModel || !pi->headModel || !pi->animations[0].numFrames ) {
//		return;
//	}

	if (uis.spinView){
		deltaYaw = ((uis.cursorx - uis.cursorpx) / (uis.frametime / 1000.0f)) / 5.0f;
		deltaRoll = ((uis.cursory - uis.cursorpy) / (uis.frametime / 1000.0f)) / 10.0f;
	}

	yaw += deltaYaw * (uis.frametime / 1000.0f);
	roll += deltaRoll * (uis.frametime / 1000.0f);

	yaw = AngleNormalize360(yaw);
	roll = AngleNormalize180(roll);

	if (fabs(deltaYaw) > 60.0f)
		deltaYaw *= 0.99f;

	if (deltaRoll != 0.0f)
		deltaRoll *= 0.98f;

	if (roll != 0.00)
		roll *= 0.95f;

	pi->moveAngles[YAW] = yaw;
	pi->moveAngles[ROLL] = roll;
// END

	dp_realtime = time;

	if ( pi->pendingWeapon != WP_NUM_WEAPONS && dp_realtime > pi->weaponTimer ) {
		pi->weapon = pi->pendingWeapon;
		pi->lastWeapon = pi->pendingWeapon;
		pi->pendingWeapon = WP_NUM_WEAPONS;
		pi->weaponTimer = 0;
		if( pi->currentWeapon != pi->weapon ) {
			trap_S_StartLocalSound( weaponChangeSound, CHAN_LOCAL );
		}

		// Q3 changes weapon model at end of TORSO_DROP animation
		// but Q3Rally doesn't use it so just change weapon now.
		UI_PlayerInfo_SetWeapon( pi, pi->weapon );
	}

	UI_AdjustFrom640( &x, &y, &w, &h );

	y -= jumpHeight;

	memset( &refdef, 0, sizeof( refdef ) );
// STONELANCE
	memset( &body, 0, sizeof(body) );
/*
	memset( &legs, 0, sizeof(legs) );
	memset( &torso, 0, sizeof(torso) );
*/
	memset( &plate, 0, sizeof(plate) );
	memset( &headlight, 0, sizeof(headlight) );
	memset( &brakelight, 0, sizeof(brakelight) );
	memset( &reverselight, 0, sizeof(reverselight) );
	memset( &turbo, 0, sizeof(turbo) );
// END
	memset( &head, 0, sizeof(head) );

	refdef.rdflags = RDF_NOWORLDMODEL;

	AxisClear( refdef.viewaxis );

// STONELANCE
	if (uis.mainMenu){
		VectorClear(angles);
		angles[PITCH] = 20;
		AnglesToAxis(angles, refdef.viewaxis);
		refdef.vieworg[2] += 0;
	}
// END

	refdef.x = x;
	refdef.y = y;
	refdef.width = w;
	refdef.height = h;

// STONELANCE
	refdef.fov_x = 90;
	xx = refdef.width / tan( refdef.fov_x / 360 * M_PI );
//	refdef.fov_x = (int)((float)refdef.width / uis.xscale / 640.0f * 90.0f);
//	xx = refdef.width / uis.xscale / tan( refdef.fov_x / 360 * M_PI );
// END

	refdef.fov_y = atan2( refdef.height, xx );
	refdef.fov_y *= ( 360 / M_PI );

// STONELANCE
/*
	refdef.fov_x = (int)((float)refdef.width / uis.xscale / 640.0f * 90.0f);
	xx = refdef.width / uis.xscale / tan( refdef.fov_x / 360 * M_PI );
	refdef.fov_y = atan2( refdef.height / uis.yscale, xx );
 	refdef.fov_y *= ( 360 / M_PI );

	// calculate distance so the player nearly fills the box
	len = 0.7 * ( maxs[2] - mins[2] );		
	origin[0] = len / tan( DEG2RAD(refdef.fov_x) * 0.5 );
	origin[1] = 0.5 * ( mins[1] + maxs[1] );
	origin[2] = -0.5 * ( mins[2] + maxs[2] );
*/

	origin[0] = 0;
	origin[1] = 0.5 * ( mins[1] + maxs[1] );
	origin[2] = -0.5 * ( mins[2] + maxs[2] );

	if (uis.mainMenu){
		origin[2] = 10;
	}

	VectorMA(origin, 96, refdef.viewaxis[0], origin);
// END

	refdef.time = dp_realtime;

	trap_R_ClearScene();

// STONELANCE
/*
	// get the rotation information
	UI_PlayerAngles( pi, legs.axis, torso.axis, head.axis );
	
	// get the animation state (after rotation, to allow feet shuffle)
	UI_PlayerAnimation( pi, &legs.oldframe, &legs.frame, &legs.backlerp,
		 &torso.oldframe, &torso.frame, &torso.backlerp );
*/
// END

	renderfx = RF_LIGHTING_ORIGIN | RF_NOSHADOW;

// STONELANCE
	body.hModel = pi->bodyModel;
	if( !body.hModel)
		return;
	body.customSkin = pi->bodySkin;

	VectorCopy( origin, body.origin );
	VectorCopy( origin, body.lightingOrigin );
	body.renderfx = renderfx;
	VectorCopy( body.origin, body.oldorigin );

	angles[YAW] = pi->moveAngles[YAW];
	angles[PITCH] = 0;
	angles[ROLL] = pi->moveAngles[ROLL];
	AnglesToAxis( angles, body.axis );

	trap_R_AddRefEntityToScene( &body );

	// add the car wheels
	UI_AddWheels(pi, &body, pi->wheelAngle);

/*
	//
	// add the legs
	//
	legs.hModel = pi->legsModel;
	legs.customSkin = pi->legsSkin;

	VectorCopy( origin, legs.origin );

	VectorCopy( origin, legs.lightingOrigin );
	legs.renderfx = renderfx;
	VectorCopy (legs.origin, legs.oldorigin);

	trap_R_AddRefEntityToScene( &legs );

	if (!legs.hModel) {
		return;
	}

	//
	// add the torso
	//
	torso.hModel = pi->torsoModel;
	if (!torso.hModel) {
		return;
	}

	torso.customSkin = pi->torsoSkin;

	VectorCopy( origin, torso.lightingOrigin );

	UI_PositionRotatedEntityOnTag( &torso, &legs, pi->legsModel, "tag_torso");

	torso.renderfx = renderfx;

	trap_R_AddRefEntityToScene( &torso );

	//
	// add the head
	//
	head.hModel = pi->headModel;
	if (!head.hModel) {
		return;
	}
	head.customSkin = pi->headSkin;

	VectorCopy( origin, head.lightingOrigin );

	UI_PositionRotatedEntityOnTag( &head, &torso, pi->torsoModel, "tag_head");

	head.renderfx = renderfx;

	trap_R_AddRefEntityToScene( &head );
*/

	//
	// add the head
	//

	if ( UI_TagExists(pi->bodyModel, "tag_head") ){
		head.hModel = pi->headModel;
		if (!head.hModel) {
			return;
		}
		head.customSkin = pi->headSkin;

		VectorCopy(pi->viewAngles, angles);
		AnglesToAxis(angles, head.axis);

		VectorCopy( origin, head.lightingOrigin );
		UI_PositionRotatedEntityOnTag( &head, &body, pi->bodyModel, "tag_head");
		head.renderfx = renderfx;

		// check for head tag, if no tag the length of delta should be 0
		trap_R_AddRefEntityToScene( &head );
	}

	//
	// add the license plate
	//
	if (UI_TagExists(pi->bodyModel, "tag_plate")){
		plate.frame = 0;
		plate.hModel = pi->plateModel;
		if (!plate.hModel) {
			return;
		}

		plate.customShader = pi->plateShader;

		VectorCopy( origin, plate.lightingOrigin );
		UI_PositionEntityOnTag( &plate, &body, pi->bodyModel, "tag_plate");
		plate.renderfx = renderfx;

		trap_R_AddRefEntityToScene( &plate );
	}

	//
	// add the headlights
	//
	if (pi->headLights){
		headlight.hModel = uis.headLightGlow;
		if (!headlight.hModel) {
			return;
		}

		VectorCopy( origin, headlight.lightingOrigin );
		headlight.renderfx = renderfx;

		for (i = 0; i < 4; i++){
			Com_sprintf(filename, sizeof(filename), "tag_hlite%d", i+1);
			if (!UI_TagExists(pi->bodyModel, filename)) continue;

			UI_PositionEntityOnTag( &headlight, &body, pi->bodyModel, filename);
			trap_R_AddRefEntityToScene( &headlight );
		}
	}

	//
	// add the brakelights
	//
	if (pi->brake){
		brakelight.hModel = uis.brakeLightGlow;
		if (!brakelight.hModel) {
			return;
		}

		VectorCopy( origin, brakelight.lightingOrigin );
		brakelight.renderfx = renderfx;

		for (i = 0; i < 3; i++){
			Com_sprintf(filename, sizeof(filename), "tag_blite%d", i+1);
			if (!UI_TagExists(pi->bodyModel, filename)) continue;

			UI_PositionEntityOnTag( &brakelight, &body, pi->bodyModel, filename);
			trap_R_AddRefEntityToScene( &brakelight );
		}
	}

	//
	// add the reverselights
	//
	if (pi->reverse){
		reverselight.hModel = uis.reverseLightGlow;
		if (!reverselight.hModel) {
			return;
		}

		VectorCopy( origin, reverselight.lightingOrigin );
		reverselight.renderfx = renderfx;

		for (i = 0; i < 2; i++){
			Com_sprintf(filename, sizeof(filename), "tag_rlite%d", i+1);
			if (!UI_TagExists(pi->bodyModel, filename)) continue;

			UI_PositionEntityOnTag( &reverselight, &body, pi->bodyModel, filename);
			trap_R_AddRefEntityToScene( &reverselight );
		}
	}

	//
	// add turbo flame
	//
	if ( pi->turbo && UI_TagExists(pi->bodyModel, "tag_turbo")){
		turbo.hModel = uis.turboModel;
		if (!turbo.hModel) {
			return;
		}

		VectorCopy( origin, turbo.lightingOrigin );
		UI_PositionEntityOnTag( &turbo, &body, pi->bodyModel, "tag_turbo");
		turbo.renderfx = renderfx;

		trap_R_AddRefEntityToScene( &turbo );
	}
// END


	//
	// add the gun
	//
	if ( pi->currentWeapon != WP_NONE ) {
		memset( &gun, 0, sizeof(gun) );
		gun.hModel = pi->weaponModel;
		if( pi->currentWeapon == WP_RAILGUN ) {
			Byte4Copy( pi->c1RGBA, gun.shaderRGBA );
		}
		else {
			Byte4Copy( colorWhite, gun.shaderRGBA );
		}
		VectorCopy( origin, gun.lightingOrigin );
// STONELANCE
//		UI_PositionEntityOnTag( &gun, &torso, pi->torsoModel, "tag_weapon");
		UI_PositionEntityOnTag( &gun, &body, pi->bodyModel, "tag_weapon");
// END
		gun.renderfx = renderfx;
		trap_R_AddRefEntityToScene( &gun );
	}

	//
	// add the spinning barrel
	//
	if ( pi->realWeapon == WP_MACHINEGUN || pi->realWeapon == WP_GAUNTLET || pi->realWeapon == WP_BFG ) {
		vec3_t	angles;

		memset( &barrel, 0, sizeof(barrel) );
		VectorCopy( origin, barrel.lightingOrigin );
		barrel.renderfx = renderfx;

		barrel.hModel = pi->barrelModel;
		angles[YAW] = 0;
		angles[PITCH] = 0;
		angles[ROLL] = UI_MachinegunSpinAngle( pi );
		if( pi->realWeapon == WP_GAUNTLET || pi->realWeapon == WP_BFG ) {
			angles[PITCH] = angles[ROLL];
			angles[ROLL] = 0;
		}
		AnglesToAxis( angles, barrel.axis );

		UI_PositionRotatedEntityOnTag( &barrel, &gun, pi->weaponModel, "tag_barrel");

		trap_R_AddRefEntityToScene( &barrel );
	}

	//
	// add muzzle flash
	//
	if ( dp_realtime <= pi->muzzleFlashTime ) {
		if ( pi->flashModel ) {
			memset( &flash, 0, sizeof(flash) );
			flash.hModel = pi->flashModel;
			if( pi->currentWeapon == WP_RAILGUN ) {
				Byte4Copy( pi->c1RGBA, flash.shaderRGBA );
			}
			else {
				Byte4Copy( colorWhite, flash.shaderRGBA );
			}
			VectorCopy( origin, flash.lightingOrigin );
			UI_PositionEntityOnTag( &flash, &gun, pi->weaponModel, "tag_flash");
			flash.renderfx = renderfx;
			trap_R_AddRefEntityToScene( &flash );
		}

		// make a dlight for the flash
		if ( pi->flashDlightColor[0] || pi->flashDlightColor[1] || pi->flashDlightColor[2] ) {
			trap_R_AddLightToScene( flash.origin, 200 + (rand()&31), pi->flashDlightColor[0],
				pi->flashDlightColor[1], pi->flashDlightColor[2] );
		}
	}

	//
	// add the chat icon
	//
	if ( pi->chat ) {
		UI_PlayerFloatSprite( pi, origin, trap_R_RegisterShaderNoMip( "sprites/balloon3" ) );
	}

	//
	// add an accent light
	//
	origin[0] -= 100;	// + = behind, - = in front
	origin[1] += 100;	// + = left, - = right
	origin[2] += 100;	// + = above, - = below
	trap_R_AddLightToScene( origin, 500, 1.0, 1.0, 1.0 );

	origin[0] -= 100;
	origin[1] -= 100;
	origin[2] -= 100;
	trap_R_AddLightToScene( origin, 500, 1.0, 0.0, 0.0 );

	trap_R_RenderScene( &refdef );
}


/*
==========================
UI_RegisterClientSkin
==========================
*/
// STONELANCE
//static qboolean UI_RegisterClientSkin( playerInfo_t *pi, const char *modelName, const char *skinName ) {
static qboolean	UI_RegisterClientSkin( playerInfo_t *pi, const char *modelName, const char *skinName, const char *rimName, const char *headName, const char *plateName ) {
// END
	char		filename[MAX_QPATH];

// STONELANCE
/*
	Com_sprintf( filename, sizeof( filename ), "models/players/%s/lower_%s.skin", modelName, skinName );
	pi->legsSkin = trap_R_RegisterSkin( filename );

	Com_sprintf( filename, sizeof( filename ), "models/players/%s/upper_%s.skin", modelName, skinName );
	pi->torsoSkin = trap_R_RegisterSkin( filename );

	Com_sprintf( filename, sizeof( filename ), "models/players/%s/head_%s.skin", modelName, skinName );
	pi->headSkin = trap_R_RegisterSkin( filename );

	if ( !pi->legsSkin || !pi->torsoSkin || !pi->headSkin ) {
		return qfalse;
	}
*/

	Com_sprintf( filename, sizeof(filename), "models/players/%s/%s.skin", modelName, skinName );
	pi->bodySkin = trap_R_RegisterSkin( filename );
	if( !pi->bodySkin ) {
		Com_Printf( S_COLOR_YELLOW "Q3R Warning: Failed to load car skin: %s\n", filename );

		// try loading default skin
		Com_sprintf( filename, sizeof(filename), "models/players/%s/%s.skin", modelName, DEFAULT_SKIN );
		pi->bodySkin = trap_R_RegisterSkin( filename );
		if( !pi->bodySkin ) {
			Com_Printf( S_COLOR_RED "Q3R Error: Failed to load default car skin: %s\n", filename );
			return qfalse;
		}
	}

	// load players icon
//	Com_sprintf( filename, sizeof(filename), "models/players/%s/icon_%s.tga", modelName, skinName );
//	pi->modelIcon = trap_R_RegisterShader( filename );

	Com_sprintf( filename, sizeof(filename), "models/players/wheels/%s.skin", rimName );
	pi->wheelSkin = trap_R_RegisterSkin( filename );
	if( !pi->wheelSkin ) {
		Com_Printf( S_COLOR_YELLOW "Q3R Warning: Failed to load wheel skin: %s\n", filename );

		// try loading default skin
		Com_sprintf( filename, sizeof(filename), "models/players/wheels/%s.skin", DEFAULT_RIM );
		pi->wheelSkin = trap_R_RegisterSkin( filename );
		if( !pi->wheelSkin ) {
			Com_Printf( S_COLOR_RED "Q3R Error: Failed to load default wheel skin: %s\n", filename );
			return qfalse;
		}
	}

	Com_sprintf( filename, sizeof(filename), "models/players/heads/%s.skin", headName );
	pi->headSkin = trap_R_RegisterSkin( filename );
	if( !pi->headSkin ) {
		Com_Printf( S_COLOR_YELLOW "Q3R Warning: Failed to load head skin: %s\n", filename );

		// try loading default skin
		Com_sprintf( filename, sizeof(filename), "models/players/heads/%s.skin", DEFAULT_HEAD );
		pi->headSkin = trap_R_RegisterSkin( filename );
		if( !pi->headSkin ) {
			Com_Printf( S_COLOR_RED "Q3R Error: Failed to load default head skin: %s\n", filename );
			return qfalse;
		}
	}

	Com_sprintf( filename, sizeof( filename ), "models/players/plates/%s", plateName );
	pi->plateShader = trap_R_RegisterShaderNoMip(filename);
	if( !pi->plateShader ) {
		Com_Printf( S_COLOR_YELLOW "Q3R Warning: Failed to load plate shader: %s\n", filename );
	}
// END

	return qtrue;
}


/*
======================
UI_ParseAnimationFile
======================
*/
#if 0
// STONELANCE
// static qboolean UI_ParseAnimationFile( const char *filename, playerInfo_t *pi ) {
static qboolean UI_ParseAnimationFile( playerInfo_t *pi, const char *filename ) {
// END
	char		*text_p, *prev;
	int			len;
	int			i;
	char		*token;
	float		fps;
	int			skip;
	char		text[20000];
	fileHandle_t	f;
//	animation_t *animations;
// STONELANCE
//	char		texturename[MAX_QPATH];

	// setup default incase this fails for some reason
	Q_strncpyz( pi->plateName, DEFAULT_PLATE, sizeof( pi->plateName ) );

	// FIXME: fix the plate loading
	return qfalse;

//	animations = pi->animations;

//	memset( animations, 0, sizeof( animation_t ) * MAX_ANIMATIONS );

//	pi->fixedlegs = qfalse;
//	pi->fixedtorso = qfalse;
// END

	// load the file
	len = trap_FS_FOpenFile( filename, &f, FS_READ );
	if ( len <= 0 ) {
		return qfalse;
	}
	if ( len >= ( sizeof( text ) - 1 ) ) {
		Com_Printf( "File %s too long\n", filename );
		return qfalse;
	}
	trap_FS_Read( text, len, f );
	text[len] = 0;
	trap_FS_FCloseFile( f );

	// parse the text
	text_p = text;
	skip = 0;	// quite the compiler warning

	// read optional parameters
	while ( 1 ) {
		prev = text_p;	// so we can unget
		token = COM_Parse( &text_p );
		if ( !token ) {
			break;
		}

// STONELANCE
/*
		if ( !Q_stricmp( token, "footsteps" ) ) {
			token = COM_Parse( &text_p );
			if ( !token ) {
				break;
			}
			continue;
		} else if ( !Q_stricmp( token, "headoffset" ) ) {
			for ( i = 0 ; i < 3 ; i++ ) {
				token = COM_Parse( &text_p );
				if ( !token ) {
					break;
				}
			}
			continue;
		} else if ( !Q_stricmp( token, "sex" ) ) {
			token = COM_Parse( &text_p );
			if ( !token ) {
				break;
			}
			continue;
		} else if ( !Q_stricmp( token, "fixedlegs" ) ) {
			pi->fixedlegs = qtrue;
			continue;
		} else if ( !Q_stricmp( token, "fixedtorso" ) ) {
			pi->fixedtorso = qtrue;
			continue;
		}

		// if it is a number, start parsing animations
		if ( token[0] >= '0' && token[0] <= '9' ) {
			text_p = prev;	// unget the token
			break;
		}
*/

/*
		if ( !Q_stricmp( token, "plate" ) ) {
			token = COM_Parse( &text_p );
			if ( !token[0] ) {
				break;
			}

//			Com_sprintf( texturename, sizeof( texturename ), "models/players/plates/player%d.tga", 0 );
			if ( !Q_stricmp( token, "usa" ) ){
				Q_strncpyz( pi->plateName, "plate_usa", sizeof( pi->plateName ) );
//				CreateLicensePlateImage(va("models/players/plates/%s_usa.tga", pi->plateSkinName), texturename, pi->name, 10);
			}
			else{
				Q_strncpyz( pi->plateName, "plate_eu", sizeof( pi->plateName ) );
//				CreateLicensePlateImage(va("models/players/plates/%s_eu.tga", pi->plateSkinName), texturename, pi->name, 20);
			}

			continue;
		}
		else
*/
		if ( !Q_stricmp( token, "oppositeRoll" ) ) {
			continue;
/*
			token = COM_Parse( &text_p );
			if ( !token[0] ) {
				pi->oppositeRoll = qfalse;
				break;
			}
			pi->oppositeRoll = atoi( token );
*/
		}
		else
// END
		Com_Printf( "unknown token '%s' is %s\n", token, filename );
	}

// STONELANCE
/*
	// read information for each frame
	for ( i = 0 ; i < MAX_ANIMATIONS ; i++ ) {

		token = COM_Parse( &text_p );
		if ( !token[0] ) {
			if( i >= TORSO_GETFLAG && i <= TORSO_NEGATIVE ) {
				animations[i].firstFrame = animations[TORSO_GESTURE].firstFrame;
				animations[i].frameLerp = animations[TORSO_GESTURE].frameLerp;
				animations[i].initialLerp = animations[TORSO_GESTURE].initialLerp;
				animations[i].loopFrames = animations[TORSO_GESTURE].loopFrames;
				animations[i].numFrames = animations[TORSO_GESTURE].numFrames;
				animations[i].reversed = qfalse;
				animations[i].flipflop = qfalse;
				continue;
			}
			break;
		}
		animations[i].firstFrame = atoi( token );
		// leg only frames are adjusted to not count the upper body only frames
		if ( i == LEGS_WALKCR ) {
			skip = animations[LEGS_WALKCR].firstFrame - animations[TORSO_GESTURE].firstFrame;
		}
		if ( i >= LEGS_WALKCR && i<TORSO_GETFLAG) {
			animations[i].firstFrame -= skip;
		}

		token = COM_Parse( &text_p );
		if ( !token[0] ) {
			break;
		}
		animations[i].numFrames = atoi( token );

		animations[i].reversed = qfalse;
		animations[i].flipflop = qfalse;
		// if numFrames is negative the animation is reversed
		if (animations[i].numFrames < 0) {
			animations[i].numFrames = -animations[i].numFrames;
			animations[i].reversed = qtrue;
		}

		token = COM_Parse( &text_p );
		if ( !token[0] ) {
			break;
		}
		animations[i].loopFrames = atoi( token );

		token = COM_Parse( &text_p );
		if ( !token[0] ) {
			break;
		}
		fps = atof( token );
		if ( fps == 0 ) {
			fps = 1;
		}
		animations[i].frameLerp = 1000 / fps;
		animations[i].initialLerp = 1000 / fps;
	}

	if ( i != MAX_ANIMATIONS ) {
		Com_Printf( "Error parsing animation file: %s\n", filename );
		return qfalse;
	}
*/
// END

	return qtrue;
}
#endif


/*
==========================
UI_RegisterClientModelname
==========================
*/
// STONELANCE
// qboolean UI_RegisterClientModelname( playerInfo_t *pi, const char *modelSkinName ) {
qboolean UI_RegisterClientModelname( playerInfo_t *pi,  const char *modelSkinName, const char *rimName, const char *headName, const char *plateName ) {
// END
	char		modelName[MAX_QPATH];
	char		skinName[MAX_QPATH];
	char		filename[MAX_QPATH];
	char		*slash;

// STONELANCE
//	pi->torsoModel = 0;
//	pi->headModel = 0;
// END

	if ( !modelSkinName[0] ) {
		return qfalse;
	}
	Q_strncpyz( modelName, modelSkinName, sizeof( modelName ) );

	slash = strchr( modelName, '/' );
	if ( !slash ) {
		// modelName did not include a skin name
// STONELANCE
//		Q_strncpyz( skinName, "default", sizeof( skinName ) );
		Q_strncpyz( skinName, DEFAULT_SKIN, sizeof( skinName ) );
// END
	} else {
		Q_strncpyz( skinName, slash + 1, sizeof( skinName ) );
		// truncate modelName
		*slash = 0;
	}

// STONELANCE
/*7
	// load cmodels before models so filecache works

	Com_sprintf( filename, sizeof( filename ), "models/players/%s/lower.md3", modelName );
	pi->legsModel = trap_R_RegisterModel( filename );
	if ( !pi->legsModel ) {
		Com_Printf( "Failed to load model file %s\n", filename );
		return qfalse;
	}

	Com_sprintf( filename, sizeof( filename ), "models/players/%s/upper.md3", modelName );
	pi->torsoModel = trap_R_RegisterModel( filename );
	if ( !pi->torsoModel ) {
		Com_Printf( "Failed to load model file %s\n", filename );
		return qfalse;
	}

	Com_sprintf( filename, sizeof( filename ), "models/players/%s/head.md3", modelName );
	pi->headModel = trap_R_RegisterModel( filename );
	if ( !pi->headModel ) {
		Com_Printf( "Failed to load model file %s\n", filename );
		return qfalse;
	}

	// if any skins failed to load, fall back to default
	if ( !UI_RegisterClientSkin( pi, modelName, skinName ) ) {
		if ( !UI_RegisterClientSkin( pi, modelName, "default" ) ) {
			Com_Printf( "Failed to load skin file: %s : %s\n", modelName, skinName );
			return qfalse;
		}
	}
// END

	// load the animations
	Com_sprintf( filename, sizeof( filename ), "models/players/%s/animation.cfg", modelName );
// STONELANCE
//	if ( !UI_ParseAnimationFile( filename, pi ) ) {
	if ( !UI_ParseAnimationFile( pi, filename ) ) {
// END
		Com_Printf( "Failed to load animation file %s\n", filename );
// STONELANCE
//		return qfalse;
// END
	}

// STONELANCE
	*/

	// load cmodels before models so filecache works
	Com_sprintf( filename, sizeof( filename ), "models/players/%s/body.md3", modelName );
	pi->bodyModel = trap_R_RegisterModel( filename );
	if ( !pi->bodyModel ) {
		Com_Printf( S_COLOR_YELLOW "Q3R Warning: Could not load car body model: %s\n", filename);
		return qfalse;
	}

	Com_sprintf( filename, sizeof( filename ), "models/players/%s/wheel.md3", modelName );
	pi->wheelModel = trap_R_RegisterModel( filename );
	if ( !pi->wheelModel ) {
		Com_Printf( S_COLOR_YELLOW "Q3R Warning: Could not load wheel model: %s\n", filename);

		// use default wheel model
		Com_sprintf( filename, sizeof(filename), "models/players/%s/wheel.md3", DEFAULT_MODEL );
		pi->wheelModel = trap_R_RegisterModel( filename );
		if( !pi->wheelModel ) {
			Com_Printf( S_COLOR_RED "Q3R Error: Failed to load default wheel model: %s\n", filename );
			return qfalse;
		}
	}

	Com_sprintf( filename, sizeof( filename ), "models/players/heads/%s.md3", headName );
	pi->headModel = trap_R_RegisterModel( filename );
	if ( !pi->headModel ) {
		Com_Printf( S_COLOR_YELLOW "Q3R Warning: Failed to load head model: %s\n", filename );

		// use default wheel model
		Com_sprintf( filename, sizeof(filename), "models/players/heads/%s.md3", DEFAULT_HEAD );
		pi->headModel = trap_R_RegisterModel( filename );
		if( !pi->headModel ) {
			Com_Printf( S_COLOR_RED "Q3R Error: Failed to load default head model: %s\n", filename );
			return qfalse;
		}
	}

	// figure out plate model
//	Com_sprintf( filename, sizeof( filename ), "models/players/plates/player%d.tga", ci->clientNum );
	if ( !Q_stricmpn( plateName, "usa_", 4 ) ){
		Q_strncpyz( pi->plateName, "plate_usa", sizeof( pi->plateName ) );
//		CreateLicensePlateImage(va("models/players/plates/%s.tga", ci->plateSkinName), filename, ci->name, 10);
	}
	else{
		Q_strncpyz( pi->plateName, "plate_eu", sizeof( pi->plateName ) );
//		CreateLicensePlateImage(va("models/players/plates/%s.tga", ci->plateSkinName), filename, ci->name, 20);
	}

	Com_sprintf( filename, sizeof( filename ), "models/players/plates/%s.md3", pi->plateName );
	pi->plateModel = trap_R_RegisterModel( filename );
	if ( !pi->plateModel ) {
		Com_Printf( S_COLOR_YELLOW "Q3R Warning: Failed to load license plate model: %s\n", filename );

		// use plate_eu model
		Com_sprintf( filename, sizeof(filename), "models/players/plates/%s.md3", DEFAULT_PLATE );
		pi->plateModel = trap_R_RegisterModel( filename );
		if( !pi->plateModel ) {
			Com_Printf( S_COLOR_RED "Q3R Error: Failed to load default license plate model: %s\n", filename );
			return qfalse;
		}
	}

	// load center suspension
	if (pi->bodyModel && (UI_TagExists(pi->bodyModel, "tag_suspcl") || UI_TagExists(pi->bodyModel, "tag_suspcr"))){
		Com_sprintf( filename, sizeof( filename ), "models/players/%s/suspc.md3", modelName );
		pi->suspCModel = trap_R_RegisterModel( filename );
		if ( !pi->suspCModel ) {
			Com_Printf( S_COLOR_YELLOW "Q3R Warning: Failed to load center suspension model: %s\n", filename );
//			return qfalse;
		}
	}

	// if any skins failed to load, return failure
	if ( !UI_RegisterClientSkin( pi, modelName, skinName, rimName, headName, plateName ) ) {
		Com_Printf( S_COLOR_RED "Q3R Error: Failed to load skin files: %s : %s : %s : %s\n", modelName, skinName, rimName, headName );
		return qfalse;
	}
// END

	return qtrue;
}


/*
===============
UI_PlayerInfo_SetModel
===============
*/
// STONELANCE
// void UI_PlayerInfo_SetModel( playerInfo_t *pi, const char *model ) {
void UI_PlayerInfo_SetModel( playerInfo_t *pi, const char *model, const char *rim, const char *head, const char *plate ) {
// END
	memset( pi, 0, sizeof(*pi) );
// STONELANCE0
//	UI_RegisterClientModelname( pi, model );
	UI_RegisterClientModelname( pi, model, rim, head, plate );
// END
	pi->weapon = WP_MACHINEGUN;
	pi->currentWeapon = pi->weapon;
	pi->lastWeapon = pi->weapon;
	pi->pendingWeapon = WP_NUM_WEAPONS;
	pi->weaponTimer = 0;
	pi->chat = qfalse;
	pi->newModel = qtrue;
	UI_PlayerInfo_SetWeapon( pi, pi->weapon );
}


/*
===============
UI_PlayerInfo_SetInfo
===============
*/
void UI_PlayerInfo_SetInfo( playerInfo_t *pi, int legsAnim, int torsoAnim, vec3_t viewAngles, vec3_t moveAngles, weapon_t weaponNumber, qboolean chat ) {
// STONELANCE
//	int			currentAnim;
// END
	weapon_t	weaponNum;
	int			c;

	pi->chat = chat;

	c = (int)trap_Cvar_VariableValue( "color1" );
 
	VectorClear( pi->color1 );

	if( c < 1 || c > 7 ) {
		VectorSet( pi->color1, 1, 1, 1 );
	}
	else {
		if( c & 1 ) {
			pi->color1[2] = 1.0f;
		}

		if( c & 2 ) {
			pi->color1[1] = 1.0f;
		}

		if( c & 4 ) {
			pi->color1[0] = 1.0f;
		}
	}

	pi->c1RGBA[0] = 255 * pi->color1[0];
	pi->c1RGBA[1] = 255 * pi->color1[1];
	pi->c1RGBA[2] = 255 * pi->color1[2];
	pi->c1RGBA[3] = 255;

	// view angles
	VectorCopy( viewAngles, pi->viewAngles );

	// move angles
	VectorCopy( moveAngles, pi->moveAngles );

	if ( pi->newModel ) {
		pi->newModel = qfalse;

// STONELANCE
/*
		jumpHeight = 0;
		pi->pendingLegsAnim = 0;
		UI_ForceLegsAnim( pi, legsAnim );
		pi->legs.yawAngle = viewAngles[YAW];
		pi->legs.yawing = qfalse;

		pi->pendingTorsoAnim = 0;
		UI_ForceTorsoAnim( pi, torsoAnim );
		pi->torso.yawAngle = viewAngles[YAW];
		pi->torso.yawing = qfalse;
*/
// END

		if ( weaponNumber != WP_NUM_WEAPONS ) {
			pi->weapon = weaponNumber;
			pi->currentWeapon = weaponNumber;
			pi->lastWeapon = weaponNumber;
			pi->pendingWeapon = WP_NUM_WEAPONS;
			pi->weaponTimer = 0;
			UI_PlayerInfo_SetWeapon( pi, pi->weapon );
		}

		return;
	}

	// weapon
	if ( weaponNumber == WP_NUM_WEAPONS ) {
		pi->pendingWeapon = WP_NUM_WEAPONS;
		pi->weaponTimer = 0;
	}
	else if ( weaponNumber != WP_NONE ) {
		pi->pendingWeapon = weaponNumber;
		pi->weaponTimer = dp_realtime + UI_TIMER_WEAPON_DELAY;
	}
	weaponNum = pi->lastWeapon;
	pi->weapon = weaponNum;

// STONELANCE
/*
	if ( torsoAnim == BOTH_DEATH1 || legsAnim == BOTH_DEATH1 ) {
		torsoAnim = legsAnim = BOTH_DEATH1;
		pi->weapon = pi->currentWeapon = WP_NONE;
		UI_PlayerInfo_SetWeapon( pi, pi->weapon );

		jumpHeight = 0;
		pi->pendingLegsAnim = 0;
		UI_ForceLegsAnim( pi, legsAnim );

		pi->pendingTorsoAnim = 0;
		UI_ForceTorsoAnim( pi, torsoAnim );

		return;
	}

	// leg animation
	currentAnim = pi->legsAnim & ~ANIM_TOGGLEBIT;
	if ( legsAnim != LEGS_JUMP && ( currentAnim == LEGS_JUMP || currentAnim == LEGS_LAND ) ) {
		pi->pendingLegsAnim = legsAnim;
	}
	else if ( legsAnim != currentAnim ) {
		jumpHeight = 0;
		pi->pendingLegsAnim = 0;
		UI_ForceLegsAnim( pi, legsAnim );
	}

	// torso animation
	if ( torsoAnim == TORSO_STAND || torsoAnim == TORSO_STAND2 ) {
		if ( weaponNum == WP_NONE || weaponNum == WP_GAUNTLET ) {
			torsoAnim = TORSO_STAND2;
		}
		else {
			torsoAnim = TORSO_STAND;
		}
	}

	if ( torsoAnim == TORSO_ATTACK || torsoAnim == TORSO_ATTACK2 ) {
		if ( weaponNum == WP_NONE || weaponNum == WP_GAUNTLET ) {
			torsoAnim = TORSO_ATTACK2;
		}
		else {
			torsoAnim = TORSO_ATTACK;
		}
		pi->muzzleFlashTime = dp_realtime + UI_TIMER_MUZZLE_FLASH;
		//FIXME play firing sound here
	}

	currentAnim = pi->torsoAnim & ~ANIM_TOGGLEBIT;

	if ( weaponNum != pi->currentWeapon || currentAnim == TORSO_RAISE || currentAnim == TORSO_DROP ) {
		pi->pendingTorsoAnim = torsoAnim;
	}
	else if ( ( currentAnim == TORSO_GESTURE || currentAnim == TORSO_ATTACK ) && ( torsoAnim != currentAnim ) ) {
		pi->pendingTorsoAnim = torsoAnim;
	}
	else if ( torsoAnim != currentAnim ) {
		pi->pendingTorsoAnim = 0;
		UI_ForceTorsoAnim( pi, torsoAnim );
	}
*/
// END
}

