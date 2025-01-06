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
// cg_ents.c -- present snapshot entities, happens every single frame

#include "cg_local.h"

/*
======================
CG_PositionEntityOnTag

Modifies the entities position and axis by the given
tag location
======================
*/
void CG_PositionEntityOnTag( refEntity_t *entity, const refEntity_t *parent,
							qhandle_t parentModel, char *tagName ) {
	int				i;
	orientation_t	lerped;

	// lerp the tag
	trap_R_LerpTag( &lerped, parentModel, parent->oldframe, parent->frame,
		1.0 - parent->backlerp, tagName );

	// FIXME: allow origin offsets along tag?
	VectorCopy( parent->origin, entity->origin );
	for ( i = 0 ; i < 3 ; i++ ) {
		VectorMA( entity->origin, lerped.origin[i], parent->axis[i], entity->origin );
	}

	// had to cast away the const to avoid compiler problems...
	MatrixMultiply( lerped.axis, ((refEntity_t *)parent)->axis, entity->axis );
	entity->backlerp = parent->backlerp;
}


/*
======================
CG_PositionRotatedEntityOnTag

Modifies the entities position and axis by the given
tag location
======================
*/
void CG_PositionRotatedEntityOnTag( refEntity_t *entity, const refEntity_t *parent,
							qhandle_t parentModel, char *tagName ) {
	int				i;
	orientation_t	lerped;
	vec3_t			tempAxis[3];

//AxisClear( entity->axis );
	// lerp the tag
	trap_R_LerpTag( &lerped, parentModel, parent->oldframe, parent->frame,
		1.0 - parent->backlerp, tagName );

	// FIXME: allow origin offsets along tag?
	VectorCopy( parent->origin, entity->origin );
	for ( i = 0 ; i < 3 ; i++ ) {
		VectorMA( entity->origin, lerped.origin[i], parent->axis[i], entity->origin );
	}

	// had to cast away the const to avoid compiler problems...
	MatrixMultiply( entity->axis, lerped.axis, tempAxis );
	MatrixMultiply( tempAxis, ((refEntity_t *)parent)->axis, entity->axis );
}



/*
==========================================================================

FUNCTIONS CALLED EACH FRAME

==========================================================================
*/

/*
======================
CG_SetEntitySoundPosition

Also called by event processing code
======================
*/
void CG_SetEntitySoundPosition( centity_t *cent ) {
	if ( cent->currentState.solid == SOLID_BMODEL ) {
		vec3_t	origin;
		float	*v;

		v = cgs.inlineModelMidpoints[ cent->currentState.modelindex ];
		VectorAdd( cent->lerpOrigin, v, origin );
		trap_S_UpdateEntityPosition( cent->currentState.number, origin );
	} else {
		trap_S_UpdateEntityPosition( cent->currentState.number, cent->lerpOrigin );
	}
}

/*
==================
CG_EntityEffects

Add continuous entity effects, like local entity emission and lighting
==================
*/
static void CG_EntityEffects( centity_t *cent ) {

	// update sound origins
	CG_SetEntitySoundPosition( cent );

	// add loop sound
	if ( cent->currentState.loopSound ) {
		if (cent->currentState.eType != ET_SPEAKER) {
			trap_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin,
				cgs.gameSounds[ cent->currentState.loopSound ] );
		} else {
			trap_S_AddRealLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin,
				cgs.gameSounds[ cent->currentState.loopSound ] );
		}
	}


	// constant light glow
	if(cent->currentState.constantLight)
	{
		int		cl;
		float	i, r, g, b;

		cl = cent->currentState.constantLight;

		//entityplus: trap_R_AddLightToScene takes rgb values from 0.0 - 1.0, not 0 - 255. Appears to be a bug in vQ3 SDK code.
		r = (float) (cl & 0xFF) / 255.0;
		g = (float) ((cl >> 8) & 0xFF) / 255.0;
		b = (float) ((cl >> 16) & 0xFF) / 255.0;
		i = (float) ((cl >> 24) & 0xFF) * 4.0;
		trap_R_AddLightToScene(cent->lerpOrigin, i*8, r, g, b);
	}

}


/*
==================
CG_General
==================
*/
static void CG_General( centity_t *cent ) {
	refEntity_t			ent;
	refEntity_t			wheelfr;
	refEntity_t			wheelfl;
	refEntity_t			wheelrr;
	refEntity_t			wheelrl;
	char 				str[MAX_QPATH];
	entityState_t		*s1;
	int					i;
	centity_t 			*player;
	int		cl;
	int		r, g, b;

	s1 = &cent->currentState;

	// if set to invisible, skip
	if (!s1->modelindex) {
		return;
	}

	memset (&ent, 0, sizeof(ent));
	memset (&wheelfr, 0, sizeof(wheelfr));
	memset (&wheelfl, 0, sizeof(wheelfl));
	memset (&wheelrr, 0, sizeof(wheelrr));
	memset (&wheelrl, 0, sizeof(wheelrl));

	// set frame
	ent.frame = s1->frame;
	ent.oldframe = ent.frame;
	ent.backlerp = 0;

	VectorCopy( cent->lerpOrigin, ent.origin);
	VectorCopy( cent->lerpOrigin, ent.oldorigin);

	ent.hModel = cgs.gameModels[s1->modelindex];
	ent.reType = RT_MODEL;
	ent.customSkin = trap_R_RegisterSkin(va("ptex/%s/%i.skin", CG_ConfigString( CS_MODELS+s1->modelindex ), s1->generic2));
	if(s1->generic2 > 0){
	ent.customShader = trap_R_RegisterShader(va("ptex/%s/%i", CG_ConfigString( CS_MODELS+s1->modelindex ), s1->generic2));
	}					
	if(s1->generic2 == 255){	
	if(cg_hide255.integer){		
	ent.customShader = cgs.media.ptexShader[0];
	} else {
	ent.customShader = cgs.media.ptexShader[1];
	}
	}

	cl = cent->currentState.constantLight;
	r = (cl & 0xFF);
	g = ((cl >> 8) & 0xFF);
	b = ((cl >> 16) & 0xFF);
	ent.shaderRGBA[0] = r;
	ent.shaderRGBA[1] = g;
	ent.shaderRGBA[2] = b;
	ent.shaderRGBA[3] = 255;
	
	Com_sprintf(str, sizeof(str), "%s", CG_ConfigString(CS_MODELS + s1->modelindex)); //karin: add %s

	// player model
	if (s1->number == cg.snap->ps.clientNum) {
		ent.renderfx |= RF_THIRD_PERSON;	// only draw from mirrors
	}

	// convert angles to axis
	AnglesToAxis( cent->lerpAngles, ent.axis );

	if(s1->scales[0] != 0.0){
	VectorScale( ent.axis[0], s1->scales[0], ent.axis[0] );}
	if(s1->scales[1] != 0.0){
	VectorScale( ent.axis[1], s1->scales[1], ent.axis[1] );}
	if(s1->scales[2] != 0.0){
	VectorScale( ent.axis[2], s1->scales[2], ent.axis[2] );}

	if(s1->torsoAnim == OT_VEHICLE){
    if (s1->generic1 && s1->generic1-1 == cg.predictedPlayerState.clientNum) {  
		if(VectorLength(cg.predictedPlayerState.velocity) > 5){
        VectorCopy(cg.predictedPlayerState.origin, ent.origin);
        VectorCopy(cg.predictedPlayerState.origin, ent.oldorigin);
        VelocityToAxis(cg.predictedPlayerState.velocity, ent.axis, 1.00f);
		}
    }
	}

	// add to refresh list
	trap_R_AddRefEntityToScene (&ent);
	
	if(s1->torsoAnim == OT_VEHICLE){
	
	trap_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, cgs.media.carengine[s1->legsAnim] );
		
	wheelfr.frame = s1->frame;
	wheelfr.oldframe = wheelfr.frame;
	wheelfr.backlerp = 0;

	VectorCopy( cent->lerpOrigin, wheelfr.origin);
	VectorCopy( cent->lerpOrigin, wheelfr.oldorigin);
	
	wheelfr.hModel = trap_R_RegisterModel_SourceTech( "props/v_wheel" );
	wheelfr.customSkin = ent.customSkin;
	wheelfr.customShader = ent.customShader;
	wheelfr.reType = RT_MODEL;
	
	AnglesToAxis( cent->lerpAngles, wheelfr.axis );
	
	if(s1->scales[0] != 0.0){
	VectorScale( wheelfr.axis[0], s1->scales[0], wheelfr.axis[0] );}
	if(s1->scales[1] != 0.0){
	VectorScale( wheelfr.axis[1], s1->scales[1], wheelfr.axis[1] );}
	if(s1->scales[2] != 0.0){
	VectorScale( wheelfr.axis[2], s1->scales[2], wheelfr.axis[2] );}
	
	CG_PositionEntityOnTag( &wheelfr, &ent, ent.hModel, "tag_wheelfr");


	wheelfl.frame = s1->frame;
	wheelfl.oldframe = wheelfl.frame;
	wheelfl.backlerp = 0;

	VectorCopy( cent->lerpOrigin, wheelfl.origin);
	VectorCopy( cent->lerpOrigin, wheelfl.oldorigin);
	
	wheelfl.hModel = trap_R_RegisterModel_SourceTech( "props/v_wheel" );
	wheelfl.customSkin = ent.customSkin;
	wheelfl.customShader = ent.customShader;
	wheelfl.reType = RT_MODEL;
	
	AnglesToAxis( cent->lerpAngles, wheelfl.axis );
	
	if(s1->scales[0] != 0.0){
	VectorScale( wheelfl.axis[0], s1->scales[0], wheelfl.axis[0] );}
	if(s1->scales[1] != 0.0){
	VectorScale( wheelfl.axis[1], s1->scales[1], wheelfl.axis[1] );}
	if(s1->scales[2] != 0.0){
	VectorScale( wheelfl.axis[2], s1->scales[2], wheelfl.axis[2] );}
	
	CG_PositionEntityOnTag( &wheelfl, &ent, ent.hModel, "tag_wheelfl");
	

	wheelrr.frame = s1->frame;
	wheelrr.oldframe = wheelrr.frame;
	wheelrr.backlerp = 0;

	VectorCopy( cent->lerpOrigin, wheelrr.origin);
	VectorCopy( cent->lerpOrigin, wheelrr.oldorigin);
	
	wheelrr.hModel = trap_R_RegisterModel_SourceTech( "props/v_wheel" );
	wheelrr.customSkin = ent.customSkin;
	wheelrr.customShader = ent.customShader;
	wheelrr.reType = RT_MODEL;
	
	AnglesToAxis( cent->lerpAngles, wheelrr.axis );
	
	if(s1->scales[0] != 0.0){
	VectorScale( wheelrr.axis[0], s1->scales[0], wheelrr.axis[0] );}
	if(s1->scales[1] != 0.0){
	VectorScale( wheelrr.axis[1], s1->scales[1], wheelrr.axis[1] );}
	if(s1->scales[2] != 0.0){
	VectorScale( wheelrr.axis[2], s1->scales[2], wheelrr.axis[2] );}
	
	CG_PositionEntityOnTag( &wheelrr, &ent, ent.hModel, "tag_wheelrr");


	wheelrl.frame = s1->frame;
	wheelrl.oldframe = wheelrl.frame;
	wheelrl.backlerp = 0;

	VectorCopy( cent->lerpOrigin, wheelrl.origin);
	VectorCopy( cent->lerpOrigin, wheelrl.oldorigin);
	
	wheelrl.hModel = trap_R_RegisterModel_SourceTech( "props/v_wheel" );
	wheelrl.customSkin = ent.customSkin;
	wheelrl.customShader = ent.customShader;
	wheelrl.reType = RT_MODEL;
	
	AnglesToAxis( cent->lerpAngles, wheelrl.axis );
	
	if(s1->scales[0] != 0.0){
	VectorScale( wheelrl.axis[0], s1->scales[0], wheelrl.axis[0] );}
	if(s1->scales[1] != 0.0){
	VectorScale( wheelrl.axis[1], s1->scales[1], wheelrl.axis[1] );}
	if(s1->scales[2] != 0.0){
	VectorScale( wheelrl.axis[2], s1->scales[2], wheelrl.axis[2] );}
	
	CG_PositionEntityOnTag( &wheelrl, &ent, ent.hModel, "tag_wheelrl");
	
	trap_R_AddRefEntityToScene (&wheelfr);
	trap_R_AddRefEntityToScene (&wheelfl);
	trap_R_AddRefEntityToScene (&wheelrr);
	trap_R_AddRefEntityToScene (&wheelrl);
	}
}

/*
==================
CG_Speaker

Speaker entities can automatically play sounds
==================
*/
static void CG_Speaker( centity_t *cent ) {
	if ( ! cent->currentState.clientNum ) {	// FIXME: use something other than clientNum...
		return;		// not auto triggering
	}

	if ( cg.time < cent->miscTime ) {
		return;
	}

	trap_S_StartSound (NULL, cent->currentState.number, CHAN_ITEM, cgs.gameSounds[cent->currentState.eventParm] );

	//	ent->s.frame = ent->wait * 10;
	//	ent->s.clientNum = ent->random * 10;
	cent->miscTime = cg.time + cent->currentState.frame * 100 + cent->currentState.clientNum * 100 * crandom();
}

/*
==================
CG_Item
==================
*/
static void CG_Item( centity_t *cent ) {
	refEntity_t		ent;
	entityState_t	*es;
	gitem_t			*item;
	int				msec;
	float			frac;
	float			scale;
	weaponInfo_t	*wi;

	es = &cent->currentState;
	if ( es->modelindex >= bg_numItems ) {
		CG_Error( "Bad item index %i on entity", es->modelindex );
	}

	// if set to invisible, skip
	if ( !es->modelindex || ( es->eFlags & EF_NODRAW ) ) {
		return;
	}

	item = &bg_itemlist[ es->modelindex ];
	if ( cg_simpleItems.integer && item->giType != IT_TEAM ) {
		memset( &ent, 0, sizeof( ent ) );
		ent.reType = RT_SPRITE;
		VectorCopy( cent->lerpOrigin, ent.origin );
		ent.radius = 14;
		ent.customShader = cg_items[es->modelindex].icon;
		ent.shaderRGBA[0] = 255;
		ent.shaderRGBA[1] = 255;
		ent.shaderRGBA[2] = 255;
		ent.shaderRGBA[3] = 255;
		trap_R_AddRefEntityToScene(&ent);
		return;
	}

	// items bob up and down continuously
if(cg_itemstyle.integer == 3){
	scale = 0.005 + cent->currentState.number * 0.00001;
	cent->lerpOrigin[2] += 4 + cos( ( cg.time + 1000 ) *  scale ) * 4;
}

	memset (&ent, 0, sizeof(ent));

	// autorotate at one of two speeds
	if ( item->giType == IT_HEALTH ) {
		VectorCopy( cg.autoAnglesFast, cent->lerpAngles );
		AxisCopy( cg.autoAxisFast, ent.axis );
	} else {
		VectorCopy( cg.autoAngles, cent->lerpAngles );
		AxisCopy( cg.autoAxis, ent.axis );
	}

	wi = NULL;
	// the weapons have their origin where they attatch to player
	// models, so we need to offset them or they will rotate
	// eccentricly
	if ( item->giType == IT_WEAPON ) {
		wi = &cg_weapons[item->giTag];
		cent->lerpOrigin[0] -=
			wi->weaponMidpoint[0] * ent.axis[0][0] +
			wi->weaponMidpoint[1] * ent.axis[1][0] +
			wi->weaponMidpoint[2] * ent.axis[2][0];
		cent->lerpOrigin[1] -=
			wi->weaponMidpoint[0] * ent.axis[0][1] +
			wi->weaponMidpoint[1] * ent.axis[1][1] +
			wi->weaponMidpoint[2] * ent.axis[2][1];
		cent->lerpOrigin[2] -=
			wi->weaponMidpoint[0] * ent.axis[0][2] +
			wi->weaponMidpoint[1] * ent.axis[1][2] +
			wi->weaponMidpoint[2] * ent.axis[2][2];

		cent->lerpOrigin[2] += 8;	// an extra height boost
	}

	ent.hModel = cg_items[es->modelindex].models[0];
	
	ent.reType = RT_MODEL;
	ent.customSkin = trap_R_RegisterSkin(va("ptex/%s/%i.skin", CG_ConfigString( CS_MODELS+es->modelindex ), es->generic2));
	if(es->generic2 > 0){			
	ent.customShader = trap_R_RegisterShader(va("ptex/%s/%i", CG_ConfigString( CS_MODELS+es->modelindex ), es->generic2));
	}					
	if(es->generic2 == 255){	
	if(cg_hide255.integer){		
	ent.customShader = cgs.media.ptexShader[0];
	} else {
	ent.customShader = cgs.media.ptexShader[1];
	}
	}

	VectorCopy( cent->lerpOrigin, ent.origin);
	VectorCopy( cent->lerpOrigin, ent.oldorigin);

	ent.nonNormalizedAxes = qfalse;

	// if just respawned, slowly scale up
	msec = cg.time - cent->miscTime;
	if ( msec >= 0 && msec < ITEM_SCALEUP_TIME ) {
		frac = (float)msec / ITEM_SCALEUP_TIME;
		VectorScale( ent.axis[0], frac, ent.axis[0] );
		VectorScale( ent.axis[1], frac, ent.axis[1] );
		VectorScale( ent.axis[2], frac, ent.axis[2] );
		ent.nonNormalizedAxes = qtrue;
	} else {
		frac = 1.0;
		if(es->scales[0] != 0.0){
		VectorScale( ent.axis[0], es->scales[0], ent.axis[0] );}
		if(es->scales[1] != 0.0){
		VectorScale( ent.axis[1], es->scales[1], ent.axis[1] );}
		if(es->scales[2] != 0.0){
		VectorScale( ent.axis[2], es->scales[2], ent.axis[2] );}
	}

	// increase the size of the weapons when they are presented as items
	if ( item->giType == IT_WEAPON ) {
		VectorScale( ent.axis[0], 1.5, ent.axis[0] );
		VectorScale( ent.axis[1], 1.5, ent.axis[1] );
		VectorScale( ent.axis[2], 1.5, ent.axis[2] );
		ent.nonNormalizedAxes = qtrue;
		//trap_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, cgs.media.weaponHoverSound );
	}

	if ( item->giType == IT_HOLDABLE && item->giTag == HI_KAMIKAZE ) {
		VectorScale( ent.axis[0], 2, ent.axis[0] );
		VectorScale( ent.axis[1], 2, ent.axis[1] );
		VectorScale( ent.axis[2], 2, ent.axis[2] );
		ent.nonNormalizedAxes = qtrue;
	}

	// add to refresh list
	trap_R_AddRefEntityToScene(&ent);

	if ( item->giType == IT_WEAPON && wi->barrelModel ) {
		refEntity_t	barrel;

		memset( &barrel, 0, sizeof( barrel ) );

		barrel.hModel = wi->barrelModel;

		VectorCopy( ent.lightingOrigin, barrel.lightingOrigin );
		barrel.shadowPlane = ent.shadowPlane;
		barrel.renderfx = ent.renderfx;

		CG_PositionRotatedEntityOnTag( &barrel, &ent, wi->weaponModel, "tag_barrel" );

		AxisCopy( ent.axis, barrel.axis );
		barrel.nonNormalizedAxes = ent.nonNormalizedAxes;

		trap_R_AddRefEntityToScene( &barrel );
	}

	// accompanying rings / spheres for powerups
	if ( !cg_simpleItems.integer )
	{
		vec3_t spinAngles;

		VectorClear( spinAngles );

		if ( item->giType == IT_HEALTH || item->giType == IT_POWERUP )
		{
			if ( ( ent.hModel = cg_items[es->modelindex].models[1] ) != 0 )
			{
				if ( item->giType == IT_POWERUP )
				{
					ent.origin[2] += 12;
					spinAngles[1] = ( cg.time & 1023 ) * 360 / -1024.0f;
				}
				AnglesToAxis( spinAngles, ent.axis );

				// scale up if respawning
				if ( frac != 1.0 ) {
					VectorScale( ent.axis[0], frac, ent.axis[0] );
					VectorScale( ent.axis[1], frac, ent.axis[1] );
					VectorScale( ent.axis[2], frac, ent.axis[2] );
					ent.nonNormalizedAxes = qtrue;
				}
				trap_R_AddRefEntityToScene( &ent );
			}
		}
	}
}

/*
===============
CG_Missile
===============
*/
static void CG_Missile( centity_t *cent ) {
	refEntity_t			ent;
	entityState_t		*s1;
	const weaponInfo_t		*weapon;
//	int	col;

	s1 = &cent->currentState;
	if ( s1->generic3 > WEAPONS_NUM ) {
		s1->generic3 = 0;
	}
	weapon = &cg_weapons[s1->generic3];

	// calculate the axis
	VectorCopy( s1->angles, cent->lerpAngles);

	// add trails
	if ( weapon->missileTrailFunc )
	{
		weapon->missileTrailFunc( cent, weapon );
	}

	// add dynamic light
	if ( weapon->missileDlight ) {
		trap_R_AddLightToScene(cent->lerpOrigin, weapon->missileDlight,
			weapon->missileDlightColor[0], weapon->missileDlightColor[1], weapon->missileDlightColor[2] );
	}

	// add missile sound
	if ( weapon->missileSound ) {
		vec3_t	velocity;

		BG_EvaluateTrajectoryDelta( &cent->currentState.pos, cg.time, velocity );

		trap_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, velocity, weapon->missileSound );
	}

	// create the render entity
	memset (&ent, 0, sizeof(ent));
	VectorCopy( cent->lerpOrigin, ent.origin);
	VectorCopy( cent->lerpOrigin, ent.oldorigin);

	if ( cent->currentState.generic3 == WP_PLASMAGUN ) {
		ent.reType = RT_SPRITE;
		ent.radius = 16;
		ent.rotation = 0;
		ent.customShader = cgs.media.plasmaBallShader;
		trap_R_AddRefEntityToScene( &ent );
		return;
	}
	if ( cent->currentState.generic3 == WP_FLAMETHROWER ) {
		ent.reType = RT_SPRITE;
		ent.radius = 20;
		ent.rotation = 1;
		ent.customShader = cgs.media.flameBallShader;
		trap_R_AddRefEntityToScene( &ent );
		return;
	}

	if ( cent->currentState.generic3 == WP_ANTIMATTER ) {
		ent.reType = RT_SPRITE;
		ent.radius = 50;
		ent.rotation = 1;
		ent.customShader = cgs.media.antimatterBallShader;
		trap_R_AddRefEntityToScene( &ent );
		return;
	}
	
	// flicker between two skins
	ent.skinNum = cg.clientFrame & 1;
	ent.hModel = weapon->missileModel;
	ent.renderfx = weapon->missileRenderfx | RF_NOSHADOW;


	if ( cent->currentState.generic3 == WP_PROX_LAUNCHER ) {
		if (s1->generic1 == TEAM_BLUE) {
			ent.hModel = cgs.media.blueProxMine;
		}
	}


	// convert direction of travel into axis
	if ( VectorNormalize2( s1->pos.trDelta, ent.axis[0] ) == 0 ) {
		ent.axis[0][2] = 1;
	}

	// spin as it moves
	if ( s1->pos.trType != TR_STATIONARY ) {
		RotateAroundDirection( ent.axis, cg.time / 4 );
	} else {

		if ( s1->generic3 == WP_PROX_LAUNCHER ) {
			AnglesToAxis( cent->lerpAngles, ent.axis );
		}
		else

		{
			RotateAroundDirection( ent.axis, s1->time );
		}
	}

	// add to refresh list, possibly with quad glow
	CG_AddRefEntityWithPowerups( &ent, s1, TEAM_FREE, qtrue );
}

/*
===============
CG_Grapple

This is called when the grapple is sitting up against the wall
===============
*/
static void CG_Grapple( centity_t *cent ) {
	refEntity_t			ent;
	entityState_t		*s1;
	const weaponInfo_t		*weapon;

	s1 = &cent->currentState;
	if ( s1->generic3 > WEAPONS_NUM ) {
		s1->generic3 = 0;
	}
	weapon = &cg_weapons[s1->generic3];

	// calculate the axis
	VectorCopy( s1->angles, cent->lerpAngles);

 // FIXME add grapple pull sound here..?
	// add missile sound
	if ( weapon->missileSound ) {
		trap_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, weapon->missileSound );
	}


	// Will draw cable if needed
	CG_GrappleTrail ( cent, weapon );

	// create the render entity
	memset (&ent, 0, sizeof(ent));
	VectorCopy( cent->lerpOrigin, ent.origin);
	VectorCopy( cent->lerpOrigin, ent.oldorigin);

	// flicker between two skins
	ent.skinNum = cg.clientFrame & 1;
	ent.hModel = weapon->missileModel;
	ent.renderfx = weapon->missileRenderfx | RF_NOSHADOW;

	// convert direction of travel into axis
	if ( VectorNormalize2( s1->pos.trDelta, ent.axis[0] ) == 0 ) {
		ent.axis[0][2] = 1;
	}

	trap_R_AddRefEntityToScene( &ent );
}
/*
==================
CG_Weather
==================
*/
static void CG_Weather( centity_t *cent ) {
	entityState_t		*s1;

	s1 = &cent->currentState;

//	CG_EffectParse( "T=RAIN,B=5 10,C=0.5,G=0.5 2,BV=0,GV=0 100,W=1 2,D=300" );
	CG_Atmospheric_SetParticles( s1->weapon, s1->powerups, s1->legsAnim );
}
/*
===============
CG_Mover
===============
*/
static void CG_Mover(centity_t *cent) {
    refEntity_t ent;
    entityState_t *s1;
    char modelname[MAX_QPATH];
    int len;
	int		cl;
	int		r, g, b;

    s1 = &cent->currentState;

    // create the render entity
    memset(&ent, 0, sizeof(ent));
    VectorCopy(cent->lerpOrigin, ent.origin);
    VectorCopy(cent->lerpOrigin, ent.oldorigin);
    AnglesToAxis(cent->lerpAngles, ent.axis);

    ent.renderfx = RF_NOSHADOW;

    // get the model, either as a bmodel or a modelindex
    if (s1->solid == SOLID_BMODEL) {
        ent.hModel = cgs.inlineDrawModel[s1->modelindex];
    } else {
        ent.hModel = cgs.gameModels[s1->modelindex];
    }
	
	cl = cent->currentState.constantLight;
	r = (cl & 0xFF);
	g = ((cl >> 8) & 0xFF);
	b = ((cl >> 16) & 0xFF);
	ent.shaderRGBA[0] = r;
	ent.shaderRGBA[1] = g;
	ent.shaderRGBA[2] = b;
	ent.shaderRGBA[3] = 255;

    // Get the model name
    Q_strncpyz(modelname, CG_ConfigString(CS_MODELS + s1->modelindex), sizeof(modelname));

    // Strip extension
    len = strlen(modelname);
    if (len > 4 && !Q_stricmp(modelname + len - 4, ".md3")) {
        modelname[len - 4] = '\0';
    }

    ent.reType = RT_MODEL;

    if (len > 0) {
        ent.customSkin = trap_R_RegisterSkin(va("ptex/%s/%i.skin", modelname, s1->generic2));
        if (s1->generic2 > 0) {
            ent.customShader = trap_R_RegisterShader(va("ptex/%s/%i", modelname, s1->generic2));
        }
        if (s1->generic2 == 255) {
            if (cg_hide255.integer) {
                ent.customShader = cgs.media.ptexShader[0];
            } else {
                ent.customShader = cgs.media.ptexShader[1];
            }
        }
    }
	
	if(s1->scales[0] != 0.0){
	VectorScale( ent.axis[0], s1->scales[0], ent.axis[0] );}
	if(s1->scales[1] != 0.0){
	VectorScale( ent.axis[1], s1->scales[1], ent.axis[1] );}
	if(s1->scales[2] != 0.0){
	VectorScale( ent.axis[2], s1->scales[2], ent.axis[2] );}

    // add to refresh list
    trap_R_AddRefEntityToScene(&ent);

    // add the secondary model
    if (s1->modelindex2) {
        ent.skinNum = 0;
        ent.hModel = cgs.gameModels[s1->modelindex2];
        trap_R_AddRefEntityToScene(&ent);
    }
}


/*
===============
CG_Beam

Also called as an event
===============
*/
void CG_Beam( centity_t *cent ) {
	refEntity_t			ent;
	entityState_t		*s1;

	s1 = &cent->currentState;

	// create the render entity
	memset(&ent, 0, sizeof(ent));
	VectorCopy(s1->pos.trBase, ent.origin);
	VectorCopy(s1->origin2, ent.oldorigin);
	AxisClear(ent.axis);
	ent.reType = RT_RAIL_CORE;
	ent.customShader = cgs.media.railCoreShader;
	ent.shaderRGBA[0] = s1->constantLight & 255;
	ent.shaderRGBA[1] = (s1->constantLight >> 8) & 255;
	ent.shaderRGBA[2] = (s1->constantLight >> 16) & 255;
	ent.shaderRGBA[3] = 255;

	ent.renderfx = RF_NOSHADOW;

	// add to refresh list
	trap_R_AddRefEntityToScene(&ent);

	//CG_Particles(origin, count, speed, lifetime, radius, type, r, g, b)
	CG_Particles(s1->origin2, 1, (int)((random() * 50) + 50), 300, 3, PT_GRAVITY, 255, (byte)((random() * 155) + 100), 0);
}


/*
===============
CG_Portal
===============
*/
static void CG_Portal( centity_t *cent ) {
	refEntity_t			ent;
	entityState_t		*s1;

	s1 = &cent->currentState;

	// create the render entity
	memset (&ent, 0, sizeof(ent));
	VectorCopy( cent->lerpOrigin, ent.origin );
	VectorCopy( s1->origin2, ent.oldorigin );
	ByteToDir( s1->eventParm, ent.axis[0] );
	PerpendicularVector( ent.axis[1], ent.axis[0] );

	// negating this tends to get the directions like they want
	// we really should have a camera roll value
	VectorSubtract( vec3_origin, ent.axis[1], ent.axis[1] );

	CrossProduct( ent.axis[0], ent.axis[1], ent.axis[2] );
	ent.reType = RT_PORTALSURFACE;
	ent.oldframe = s1->powerups;
	ent.frame = s1->frame;		// rotation speed
	ent.skinNum = s1->clientNum/256.0 * 360;	// roll offset

	// add to refresh list
	trap_R_AddRefEntityToScene(&ent);
}


/*
=========================
CG_AdjustPositionForMover

Also called by client movement prediction code
=========================
*/
void CG_AdjustPositionForMover( const vec3_t in, int moverNum, int fromTime, int toTime, vec3_t out ) {
	centity_t	*cent;
	vec3_t	oldOrigin, origin, deltaOrigin;
	vec3_t	oldAngles, angles, deltaAngles;

	if ( moverNum <= 0 || moverNum >= ENTITYNUM_MAX_NORMAL ) {
		VectorCopy( in, out );
		return;
	}

	cent = &cg_entities[ moverNum ];
	if ( cent->currentState.eType != ET_MOVER ) {
		VectorCopy( in, out );
		return;
	}

	BG_EvaluateTrajectory( &cent->currentState.pos, fromTime, oldOrigin );
	BG_EvaluateTrajectory( &cent->currentState.apos, fromTime, oldAngles );

	BG_EvaluateTrajectory( &cent->currentState.pos, toTime, origin );
	BG_EvaluateTrajectory( &cent->currentState.apos, toTime, angles );

	VectorSubtract( origin, oldOrigin, deltaOrigin );
	VectorSubtract( angles, oldAngles, deltaAngles );

	VectorAdd( in, deltaOrigin, out );

	// FIXME: origin change when on a rotating object
}


/*
=============================
ST_InterpolateEntityPosition
=============================
*/
static void ST_InterpolateEntityPosition( centity_t *cent ) {
	vec3_t		current, next;
	float		f;

	// it would be an internal error to find an entity that interpolates without
	// a snapshot ahead of the current one
	if ( cg.nextSnap == NULL ) {
		CG_Error( "CG_InterpoateEntityPosition: cg.nextSnap == NULL" );
	}

	f = cg.frameInterpolation;

	// this will linearize a sine or parabolic curve, but it is important
	// to not extrapolate player positions if more recent data is available
	ST_EvaluateTrajectory( &cent->currentState.pos, cg.snap->serverTime, current, cent->currentState.generic3 );
	ST_EvaluateTrajectory( &cent->nextState.pos, cg.nextSnap->serverTime, next, cent->currentState.generic3 );

	cent->lerpOrigin[0] = current[0] + f * ( next[0] - current[0] );
	cent->lerpOrigin[1] = current[1] + f * ( next[1] - current[1] );
	cent->lerpOrigin[2] = current[2] + f * ( next[2] - current[2] );

	ST_EvaluateTrajectory( &cent->currentState.apos, cg.snap->serverTime, current, cent->currentState.generic3 );
	ST_EvaluateTrajectory( &cent->nextState.apos, cg.nextSnap->serverTime, next, cent->currentState.generic3 );

	cent->lerpAngles[0] = LerpAngle( current[0], next[0], f );
	cent->lerpAngles[1] = LerpAngle( current[1], next[1], f );
	cent->lerpAngles[2] = LerpAngle( current[2], next[2], f );

}

/*
=============================
CG_InterpolateEntityPosition
=============================
*/
static void CG_InterpolateEntityPosition( centity_t *cent ) {
	vec3_t		current, next;
	float		f;

	// it would be an internal error to find an entity that interpolates without
	// a snapshot ahead of the current one
	if ( cg.nextSnap == NULL ) {
		CG_Error( "CG_InterpoateEntityPosition: cg.nextSnap == NULL" );
	}

	f = cg.frameInterpolation;

	// this will linearize a sine or parabolic curve, but it is important
	// to not extrapolate player positions if more recent data is available
	BG_EvaluateTrajectory( &cent->currentState.pos, cg.snap->serverTime, current );
	BG_EvaluateTrajectory( &cent->nextState.pos, cg.nextSnap->serverTime, next );

	cent->lerpOrigin[0] = current[0] + f * ( next[0] - current[0] );
	cent->lerpOrigin[1] = current[1] + f * ( next[1] - current[1] );
	cent->lerpOrigin[2] = current[2] + f * ( next[2] - current[2] );

	BG_EvaluateTrajectory( &cent->currentState.apos, cg.snap->serverTime, current );
	BG_EvaluateTrajectory( &cent->nextState.apos, cg.nextSnap->serverTime, next );

	cent->lerpAngles[0] = LerpAngle( current[0], next[0], f );
	cent->lerpAngles[1] = LerpAngle( current[1], next[1], f );
	cent->lerpAngles[2] = LerpAngle( current[2], next[2], f );

}

/*
===============
CG_CalcEntityLerpPositions

===============
*/
static void CG_CalcEntityLerpPositions( centity_t *cent ) {
	clientInfo_t	*ci;
	int				weaphack;
	int timeshift = 0;

	ci = &cgs.clientinfo[ cent->currentState.clientNum ];
	
	weaphack = ci->swepid;

//unlagged - projectile nudge
	// this will be set to how far forward projectiles will be extrapolated
//unlagged - projectile nudge

	if ( cent->interpolate && cent->currentState.pos.trType == TR_INTERPOLATE ) {
		CG_InterpolateEntityPosition( cent );
		return;
	}

	// first see if we can interpolate between two snaps for
	// linear extrapolated clients
	if ( cent->interpolate && cent->currentState.pos.trType == TR_LINEAR_STOP &&
											cent->currentState.number < MAX_CLIENTS) {
		CG_InterpolateEntityPosition( cent );
		return;
	}

//unlagged - timenudge extrapolation
	// interpolating failed (probably no nextSnap), so extrapolate
	// this can also happen if the teleport bit is flipped, but that
	// won't be noticeable
	if ( cent->currentState.number < MAX_CLIENTS &&
			cent->currentState.clientNum != cg.predictedPlayerState.clientNum ) {
		cent->currentState.pos.trType = TR_LINEAR_STOP;
		cent->currentState.pos.trTime = cg.snap->serverTime;
		cent->currentState.pos.trDuration = 1000 / sv_fps.integer;
	}
//unlagged - timenudge extrapolation

//unlagged - projectile nudge
	// if it's a missile but not a grappling hook
	if ( cent->currentState.eType == ET_MISSILE && weaphack != WP_GRAPPLING_HOOK ) {
		// if it's one of ours
		if ( cent->currentState.otherEntityNum == cg.clientNum ) {
			// extrapolate one server frame's worth - this will correct for tiny
			// visual inconsistencies introduced by backward-reconciling all players
			// one server frame before running projectiles
			timeshift = 1000 / sv_fps.integer;
		}
		// if it's not, and it's not a grenade launcher
		else if ( weaphack != WP_GRENADE_LAUNCHER ) {
			// extrapolate based on cg_projectileNudge
			timeshift = cg_projectileNudge.integer + 1000 / sv_fps.integer;
		}
	}

	// just use the current frame and evaluate as best we can
	BG_EvaluateTrajectory( &cent->currentState.pos, cg.time + timeshift, cent->lerpOrigin );
	BG_EvaluateTrajectory( &cent->currentState.apos, cg.time + timeshift, cent->lerpAngles );

	// if there's a time shift
	if ( timeshift != 0 ) {
		trace_t tr;
		vec3_t lastOrigin;

		BG_EvaluateTrajectory( &cent->currentState.pos, cg.time, lastOrigin );

		CG_Trace( &tr, lastOrigin, vec3_origin, vec3_origin, cent->lerpOrigin, cent->currentState.number, MASK_SHOT );

		// don't let the projectile go through the floor
		if ( tr.fraction < 1.0f ) {
			cent->lerpOrigin[0] = lastOrigin[0] + tr.fraction * ( cent->lerpOrigin[0] - lastOrigin[0] );
			cent->lerpOrigin[1] = lastOrigin[1] + tr.fraction * ( cent->lerpOrigin[1] - lastOrigin[1] );
			cent->lerpOrigin[2] = lastOrigin[2] + tr.fraction * ( cent->lerpOrigin[2] - lastOrigin[2] );
		}
	}
//unlagged - projectile nudge

	// adjust for riding a mover if it wasn't rolled into the predicted
	// player state
	if ( cent != &cg.predictedPlayerEntity ) {
		CG_AdjustPositionForMover( cent->lerpOrigin, cent->currentState.groundEntityNum,
		cg.snap->serverTime, cg.time, cent->lerpOrigin );
	}
}

/*
===============
ST_CalcEntityLerpPositions

===============
*/
static void ST_CalcEntityLerpPositions( centity_t *cent ) {
	clientInfo_t	*ci;
	int				weaphack;
	int timeshift = 0;

	ci = &cgs.clientinfo[ cent->currentState.clientNum ];

	weaphack = ci->swepid;

//unlagged - projectile nudge
	// this will be set to how far forward projectiles will be extrapolated
//unlagged - projectile nudge

	if ( cent->interpolate && cent->currentState.pos.trType == TR_INTERPOLATE ) {
		ST_InterpolateEntityPosition( cent );
		return;
	}

	// first see if we can interpolate between two snaps for
	// linear extrapolated clients
	if ( cent->interpolate && cent->currentState.pos.trType == TR_LINEAR_STOP &&
											cent->currentState.number < MAX_CLIENTS) {
		ST_InterpolateEntityPosition( cent );
		return;
	}

//unlagged - timenudge extrapolation
	// interpolating failed (probably no nextSnap), so extrapolate
	// this can also happen if the teleport bit is flipped, but that
	// won't be noticeable
	if ( cent->currentState.number < MAX_CLIENTS &&
			cent->currentState.clientNum != cg.predictedPlayerState.clientNum ) {
		cent->currentState.pos.trType = TR_LINEAR_STOP;
		cent->currentState.pos.trTime = cg.snap->serverTime;
		cent->currentState.pos.trDuration = 1000 / sv_fps.integer;
	}
//unlagged - timenudge extrapolation

//unlagged - projectile nudge
	// if it's a missile but not a grappling hook
	if ( cent->currentState.eType == ET_MISSILE && weaphack != WP_GRAPPLING_HOOK ) {
		// if it's one of ours
		if ( cent->currentState.otherEntityNum == cg.clientNum ) {
			// extrapolate one server frame's worth - this will correct for tiny
			// visual inconsistencies introduced by backward-reconciling all players
			// one server frame before running projectiles
			timeshift = 1000 / sv_fps.integer;
		}
		// if it's not, and it's not a grenade launcher
		else if ( weaphack != WP_GRENADE_LAUNCHER ) {
			// extrapolate based on cg_projectileNudge
			timeshift = cg_projectileNudge.integer + 1000 / sv_fps.integer;
		}
	}

	// just use the current frame and evaluate as best we can
	ST_EvaluateTrajectory( &cent->currentState.pos, cg.time + timeshift, cent->lerpOrigin, cent->currentState.generic3 );
	ST_EvaluateTrajectory( &cent->currentState.apos, cg.time + timeshift, cent->lerpAngles, cent->currentState.generic3 );

	// if there's a time shift
	if ( timeshift != 0 ) {
		trace_t tr;
		vec3_t lastOrigin;

		ST_EvaluateTrajectory( &cent->currentState.pos, cg.time, lastOrigin, cent->currentState.generic3 );

		CG_Trace( &tr, lastOrigin, vec3_origin, vec3_origin, cent->lerpOrigin, cent->currentState.number, MASK_SHOT );

		// don't let the projectile go through the floor
		if ( tr.fraction < 1.0f ) {
			cent->lerpOrigin[0] = lastOrigin[0] + tr.fraction * ( cent->lerpOrigin[0] - lastOrigin[0] );
			cent->lerpOrigin[1] = lastOrigin[1] + tr.fraction * ( cent->lerpOrigin[1] - lastOrigin[1] );
			cent->lerpOrigin[2] = lastOrigin[2] + tr.fraction * ( cent->lerpOrigin[2] - lastOrigin[2] );
		}
	}
//unlagged - projectile nudge

	// adjust for riding a mover if it wasn't rolled into the predicted
	// player state
	if ( cent != &cg.predictedPlayerEntity ) {
		CG_AdjustPositionForMover( cent->lerpOrigin, cent->currentState.groundEntityNum,
		cg.snap->serverTime, cg.time, cent->lerpOrigin );
	}
}

/*
===============
CG_TeamBase
===============
*/
static void CG_TeamBase( centity_t *cent ) {
	refEntity_t model;

	vec3_t angles;
	int t, h;
	float c;

	if ( cgs.gametype == GT_CTF || cgs.gametype == GT_1FCTF ) {
//#else
//	if ( cgs.gametype == GT_CTF) {

		// show the flag base
		memset(&model, 0, sizeof(model));
		model.reType = RT_MODEL;
		VectorCopy( cent->lerpOrigin, model.lightingOrigin );
		VectorCopy( cent->lerpOrigin, model.origin );
		AnglesToAxis( cent->currentState.angles, model.axis );
		if ( cent->currentState.modelindex == TEAM_RED ) {
			model.hModel = cgs.media.redFlagBaseModel;
		}
		else if ( cent->currentState.modelindex == TEAM_BLUE ) {
			model.hModel = cgs.media.blueFlagBaseModel;
		}
		else {
			model.hModel = cgs.media.neutralFlagBaseModel;
		}
		trap_R_AddRefEntityToScene( &model );
	}

	else if ( cgs.gametype == GT_OBELISK ) {
		// show the obelisk
		memset(&model, 0, sizeof(model));
		model.reType = RT_MODEL;
		VectorCopy( cent->lerpOrigin, model.lightingOrigin );
		VectorCopy( cent->lerpOrigin, model.origin );
		AnglesToAxis( cent->currentState.angles, model.axis );

		model.hModel = cgs.media.overloadBaseModel;
		trap_R_AddRefEntityToScene( &model );
		// if hit
		if ( cent->currentState.frame == 1) {
			// show hit model
			// modelindex2 is the health value of the obelisk
			c = cent->currentState.modelindex2;
			model.shaderRGBA[0] = 0xff;
			model.shaderRGBA[1] = c;
			model.shaderRGBA[2] = c;
			model.shaderRGBA[3] = 0xff;
			//
			model.hModel = cgs.media.overloadEnergyModel;
			trap_R_AddRefEntityToScene( &model );
		}
		// if respawning
		if ( cent->currentState.frame == 2) {
			if ( !cent->miscTime ) {
				cent->miscTime = cg.time;
			}
			t = cg.time - cent->miscTime;
			h = (cg_obeliskRespawnDelay.integer - 5) * 1000;
			//
			if (t > h) {
				c = (float) (t - h) / h;
				if (c > 1)
					c = 1;
			}
			else {
				c = 0;
			}
			// show the lights
			AnglesToAxis( cent->currentState.angles, model.axis );
			//
			model.shaderRGBA[0] = c * 0xff;
			model.shaderRGBA[1] = c * 0xff;
			model.shaderRGBA[2] = c * 0xff;
			model.shaderRGBA[3] = c * 0xff;

			model.hModel = cgs.media.overloadLightsModel;
			trap_R_AddRefEntityToScene( &model );
			// show the target
			if (t > h) {
				if ( !cent->muzzleFlashTime ) {
					trap_S_StartSound (cent->lerpOrigin, ENTITYNUM_NONE, CHAN_BODY,  cgs.media.obeliskRespawnSound);
					cent->muzzleFlashTime = 1;
				}
				VectorCopy(cent->currentState.angles, angles);
				angles[YAW] += (float) 16 * acos(1-c) * 180 / M_PI;
				AnglesToAxis( angles, model.axis );

				VectorScale( model.axis[0], c, model.axis[0]);
				VectorScale( model.axis[1], c, model.axis[1]);
				VectorScale( model.axis[2], c, model.axis[2]);

				model.shaderRGBA[0] = 0xff;
				model.shaderRGBA[1] = 0xff;
				model.shaderRGBA[2] = 0xff;
				model.shaderRGBA[3] = 0xff;
				//
				model.origin[2] += 56;
				model.hModel = cgs.media.overloadTargetModel;
				trap_R_AddRefEntityToScene( &model );
			}
			else {
				//FIXME: show animated smoke
			}
		}
		else {
			cent->miscTime = 0;
			cent->muzzleFlashTime = 0;
			// modelindex2 is the health value of the obelisk
			c = cent->currentState.modelindex2;
			model.shaderRGBA[0] = 0xff;
			model.shaderRGBA[1] = c;
			model.shaderRGBA[2] = c;
			model.shaderRGBA[3] = 0xff;
			// show the lights
			model.hModel = cgs.media.overloadLightsModel;
			trap_R_AddRefEntityToScene( &model );
			// show the target
			model.origin[2] += 56;
			model.hModel = cgs.media.overloadTargetModel;
			trap_R_AddRefEntityToScene( &model );
		}
	}
	else if ( cgs.gametype == GT_HARVESTER ) {
		// show harvester model
		memset(&model, 0, sizeof(model));
		model.reType = RT_MODEL;
		VectorCopy( cent->lerpOrigin, model.lightingOrigin );
		VectorCopy( cent->lerpOrigin, model.origin );
		AnglesToAxis( cent->currentState.angles, model.axis );

		if ( cent->currentState.modelindex == TEAM_RED ) {
			model.hModel = cgs.media.harvesterModel;
			model.customSkin = cgs.media.harvesterRedSkin;
		}
		else if ( cent->currentState.modelindex == TEAM_BLUE ) {
			model.hModel = cgs.media.harvesterModel;
			model.customSkin = cgs.media.harvesterBlueSkin;
		}
		else {
			model.hModel = cgs.media.harvesterNeutralModel;
			model.customSkin = 0;
		}
		trap_R_AddRefEntityToScene( &model );
	}

}

/*
===============
CG_AddCEntity

===============
*/
static void CG_AddCEntity( centity_t *cent ) {
	// event-only entities will have been dealt with already
	if ( cent->currentState.eType >= ET_EVENTS ) {
		return;
	}

	// calculate the current origin
	if(cent->currentState.torsoAnim){
		ST_CalcEntityLerpPositions( cent );
	} else {
		CG_CalcEntityLerpPositions( cent );
	}

	// add automatic effects
	CG_EntityEffects( cent );

	switch ( cent->currentState.eType ) {
	default:
		CG_Error( "Bad entity type: %i\n", cent->currentState.eType );
		break;
	case ET_INVISIBLE:
	case ET_PUSH_TRIGGER:
	case ET_TELEPORT_TRIGGER:
		break;
	case ET_GENERAL:
		CG_General( cent );
		break;
	case ET_PLAYER:
		CG_Player( cent );
		break;
	case ET_ITEM:
		CG_Item( cent );
		break;
	case ET_MISSILE:
		CG_Missile( cent );
		break;
	case ET_MOVER:
		CG_Mover( cent );
		break;
	case ET_BEAM:
		CG_Beam( cent );
		break;
	case ET_PORTAL:
		CG_Portal( cent );
		break;
	case ET_SPEAKER:
		CG_Speaker( cent );
		break;
	case ET_GRAPPLE:
		CG_Grapple( cent );
		break;
	case ET_WEATHER:
		CG_Weather( cent );
		break;
	case ET_TEAM:
		CG_TeamBase( cent );
		break;
	}
}

/*
===============
CG_AddPacketEntities

===============
*/
void CG_AddPacketEntities( void ) {
	int					num;
	centity_t			*cent;
	playerState_t		*ps;

	// set cg.frameInterpolation
	if ( cg.nextSnap ) {
		int		delta;

		delta = (cg.nextSnap->serverTime - cg.snap->serverTime);
		if ( delta == 0 ) {
			cg.frameInterpolation = 0;
		} else {
			cg.frameInterpolation = (float)( cg.time - cg.snap->serverTime ) / delta;
		}
	} else {
		cg.frameInterpolation = 0;	// actually, it should never be used, because
									// no entities should be marked as interpolating
	}

	// the auto-rotating items will all have the same axis
if(cg_itemstyle.integer == 3){
	cg.autoAngles[0] = 0;
	cg.autoAngles[1] = ( cg.time & 2047 ) * 360 / 2048.0;
	cg.autoAngles[2] = 0;

	cg.autoAnglesFast[0] = 0;
	cg.autoAnglesFast[1] = ( cg.time & 1023 ) * 360 / 1024.0f;
	cg.autoAnglesFast[2] = 0;
}

if(cg_itemstyle.integer == 2){
	cg.autoAngles[0] = 0;
	cg.autoAngles[1] = ( cg.time & 4095 ) * 360 / 4096.0;
	cg.autoAngles[2] = 0;

	cg.autoAnglesFast[0] = 0;
	cg.autoAnglesFast[1] = ( cg.time & 4095 ) * 360 / 4096.0f;
	cg.autoAnglesFast[2] = 0;
}

if(cg_itemstyle.integer == 1){
	cg.autoAngles[0] = 0;
	cg.autoAngles[1] = 0;
	cg.autoAngles[2] = 0;

	cg.autoAnglesFast[0] = 0;
	cg.autoAnglesFast[1] = 0;
	cg.autoAnglesFast[2] = 0;
}

	AnglesToAxis( cg.autoAngles, cg.autoAxis );
	AnglesToAxis( cg.autoAnglesFast, cg.autoAxisFast );

	// generate and add the entity from the playerstate
	ps = &cg.predictedPlayerState;
	BG_PlayerStateToEntityState( ps, &cg.predictedPlayerEntity.currentState, qfalse );
	CG_AddCEntity( &cg.predictedPlayerEntity );

	// lerp the non-predicted value for lightning gun origins
	CG_CalcEntityLerpPositions( &cg_entities[ cg.snap->ps.clientNum ] );

//unlagged - early transitioning
	if ( cg.nextSnap ) {
		// pre-add some of the entities sent over by the server
		// we have data for them and they don't need to interpolate
		for ( num = 0 ; num < cg.nextSnap->numEntities ; num++ ) {
			cent = &cg_entities[ cg.nextSnap->entities[ num ].number ];
			if ( cent->nextState.eType == ET_MISSILE || cent->nextState.eType == ET_GENERAL ) {
				// transition it immediately and add it
				CG_TransitionEntity( cent );
				cent->interpolate = qtrue;
				CG_AddCEntity( cent );
			}
		}
	}
//unlagged - early transitioning

	// add each entity sent over by the server
	for ( num = 0 ; num < cg.snap->numEntities ; num++ ) {
		cent = &cg_entities[ cg.snap->entities[ num ].number ];
//unlagged - early transitioning
		if ( !cg.nextSnap || (cent->nextState.eType != ET_MISSILE && cent->nextState.eType != ET_GENERAL) ) {
//unlagged - early transitioning
			CG_AddCEntity( cent );
		} //Also unlagged
	}
}

