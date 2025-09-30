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

#include "cg_local.h"


void CG_DrawCheckpointLinks(void)
{
	int			i, j;
	centity_t	*cents[100];
	qboolean	checkpointFound;
	int			numCheckpoints = 0;
	vec3_t		handle;

	// Checkpoint limit
	for (i = 0; i < 100; i++)
	{
		checkpointFound = qfalse;
		for (j = 0; j < MAX_GENTITIES; j++)
		{
			cents[i] = &cg_entities[j];
			if (cents[i]->currentState.eType != ET_CHECKPOINT) continue;
			if (cents[i]->currentState.weapon != i+1) continue;

			numCheckpoints++;
			checkpointFound = qtrue;
/*
			if( cents[i]->bezierPos[0] == 0.0f &&
				cents[i]->bezierPos[1] == 0.0f && 
				cents[i]->bezierPos[2] == 0.0f )
			{
				VectorCopy( cents[i]->currentState.origin2, cents[i]->bezierPos );
			}
*/
			break;
		}

		if( !checkpointFound )
			break;
	}

	if( cg.currentBezierPoint == 0 )
		cg.currentBezierPoint = 1;
	if( cg.currentBezierPoint > numCheckpoints )
		cg.currentBezierPoint = 1;
/*
	for (i = 1; i < 40; i++){
		checkpointFound = qfalse;
		for (j = 0; j < MAX_ENTITIES; j++){
			cent = &cg_entities[j];
			if (cent->currentState.eType != ET_CHECKPOINT) continue;
			if (cent->currentState.weapon != i) continue;

			if( cent2 != NULL &&
				cent3 != NULL &&
				cent2->bezierDir[0] == 0.0f &&
				cent2->bezierDir[1] == 0.0f && 
				cent2->bezierDir[2] == 0.0f )
			{
				VectorSubtract( cent3->currentState.origin, cent->currentState.origin, cent->bezierDir );
				VectorScale( cent->bezierDir, -0.35f, cent->bezierDir );
//				cent->bezierDir[0] = 400.0f;
//				Com_Printf( "setting bezier direction %i\n", i );
			}

			if( cent2 != NULL )
			{
//				Com_Printf( "DrawLine: %f, %f, %f\n", cent2->currentState.origin[0], cent2->currentState.origin[1], cent2->currentState.origin[2] );
//				Com_Printf( "      to: %f, %f, %f\n", cent->currentState.origin[0], cent->currentState.origin[1], cent->currentState.origin[2] );
//				CG_Draw3DLine( cent2->currentState.origin, cent->currentState.origin, 1.0f, 0.0f, 0.0f, 1.0f );
				CG_Draw3DBezierCurve( cent2->currentState.origin, cent2->bezierDir, cent->currentState.origin, cent->bezierDir, 16, 1.0f, 0.0f, 0.0f, 1.0f );
			}
			cent3 = cent2;
			cent2 = cent;

			checkpointFound = qtrue;
		}

		if( !checkpointFound )
			break;
	}
*/
	for (i = 0; i <= numCheckpoints; i++)
	{
/*
		if( cents[i]->bezierDir[0] == 0.0f &&
			cents[i]->bezierDir[1] == 0.0f && 
			cents[i]->bezierDir[2] == 0.0f )
		{
			VectorCopy( cents[i]->currentState.angles2, cents[i]->bezierDir );
//			VectorSubtract( cents[(i-1)%numCheckpoints]->bezierPos, cents[(i+1)%numCheckpoints]->bezierPos, cents[i]->bezierDir );
//			VectorScale( cents[i]->bezierDir, -0.35f, cents[i]->bezierDir );
		}
*/
//		Com_Printf( "DrawLine: %f, %f, %f\n", cent2->currentState.origin[0], cent2->currentState.origin[1], cent2->currentState.origin[2] );
//		Com_Printf( "      to: %f, %f, %f\n", cent->currentState.origin[0], cent->currentState.origin[1], cent->currentState.origin[2] );
//		CG_Draw3DLine( cent2->currentState.origin, cent->currentState.origin, 1.0f, 0.0f, 0.0f, 1.0f );
		CG_Draw3DBezierCurve( cents[i]->currentState.origin2, cents[i]->currentState.angles2, cents[(i+1)%numCheckpoints]->currentState.origin2, cents[(i+1)%numCheckpoints]->currentState.angles2, 16, 1.0f, 0.0f, 0.0f, 1.0f );

		VectorAdd( cents[i]->currentState.origin2, cents[i]->currentState.angles2, handle );
		CG_Draw3DLine( cents[i]->currentState.origin2, handle, 0.0f, 0.0f, 1.0f, 1.0f );
	}

	CG_DrawModel( cents[cg.currentBezierPoint-1]->currentState.origin2, trap_R_RegisterModel( "models/test/sphere01.md3" ) );
}


void CG_Sparks( const vec3_t origin, const vec3_t normal, const vec3_t direction, const float speed )
{
	vec3_t			velocity;
	localEntity_t	*le;
	refEntity_t		*re;

	VectorCopy( direction, velocity );
	velocity[0] += crandom() * 0.5f;
	velocity[1] += crandom() * 0.5f;
	velocity[2] += random() * 0.5f;
	VectorNormalize( velocity );
	VectorScale( velocity, speed + crandom() * 10.0f, velocity );

	le = CG_AllocLocalEntity();
	re = &le->refEntity;

	le->leType = LE_FRAGMENT;
	le->leFlags = LEF_SCALE_FADE_OUT;
	le->startTime = cg.time;
	le->endTime = le->startTime + 600 + random() * 200;
	le->fadeInTime = le->startTime + ( le->endTime - le->startTime ) * 0.4f;
	le->radius = 2.0f;

	VectorCopy( origin, re->origin );
	re->shaderTime = cg.time / 1000.0f;
	re->reType = RT_SPRITE;
	re->rotation = 0;
	re->radius = 2.0f;
	re->customShader = cgs.media.sparkShader;
	re->shaderRGBA[0] = 0xff;
	re->shaderRGBA[1] = 0xff;
	re->shaderRGBA[2] = 0xff;
	re->shaderRGBA[3] = 0x7f;
	le->color[0] = 1.0f;
	le->color[1] = 1.0f;
	le->color[2] = 1.0f;
	le->color[3] = 0.5f;

	le->pos.trType = TR_GRAVITY;
	VectorCopy( origin, le->pos.trBase );
	VectorCopy( velocity, le->pos.trDelta );
	le->pos.trTime = cg.time;

	le->bounceFactor = 0.4f;

	le->leBounceSoundType = LEBS_BRASS;
	le->leMarkType = LEMT_NONE;
}


qboolean CG_FrictionCalc( const carPoint_t *point, float *sCOF, float *kCOF )
{
	// TODO
/*
	gentity_t	*ent;
	int			entityList[MAX_GENTITIES];
	int			numListedEntities;
	vec3_t		mins, maxs;
	int			i;

	for ( i = 0 ; i < 3 ; i++ ) {
		mins[i] = point->r[i] - point->radius;
		maxs[i] = point->r[i] + point->radius;
	}

	numListedEntities = trap_EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

	for ( i = 0 ; i < numListedEntities ; i++ ) {
		ent = &g_entities[entityList[ i ]];

		if( ent->s.eType != ET_EVENTS + EV_HAZARD ) continue;
		if( ent->s.weapon != HT_OIL ) continue;

		*sCOF = CP_OIL_SCOF;
		*kCOF = CP_OIL_KCOF;

		return qtrue;
	}
*/
	return qfalse;
}


/*
=================
CG_Hazard

Caused by an EV_HAZARD events
=================
*/
void CG_Hazard( int hazard, vec3_t origin, int radius) {
	qhandle_t		mod;
	qhandle_t		mark;
	qhandle_t		shader;
	sfxHandle_t		sfx;
//	float			radius;
	float			light;
	vec3_t			lightColor;
	vec3_t			angles, dir, start, dest;
	localEntity_t	*le;
	qboolean		alphaFade;
	qboolean		isSprite;
	int				duration;
	trace_t			tr;

	VectorCopy(origin, start);
	VectorCopy(origin, dest);
	start[2] += 2;
	dest[2] -= 8000;
	CG_Trace( &tr, start, NULL, NULL, dest, 0, MASK_SOLID );
	VectorCopy(tr.plane.normal, dir);

	radius *= 16;
	if (radius <= 0){
		radius = 64;
	}

	mark = 0;
//	radius = 32;
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

	switch ( hazard )
	{
	case HT_BIO:
		VectorMA(tr.endpos, 0, tr.plane.normal, origin);
//		mod = cgs.media.dishFlashModel;
//		shader = cgs.media.rocketExplosionShader;
//		sfx = cgs.media.sfx_rockexp;
		mark = cgs.media.bioMarkShader;
		radius *= 1.1;
		light = 150;
//		isSprite = qtrue;
//		duration = 2000;
		lightColor[0] = 0;
		lightColor[1] = 0.75;
		lightColor[2] = 0.0;
		break;

	case HT_OIL:
		VectorMA(tr.endpos, 0, tr.plane.normal, origin);
//		mod = cgs.media.dishFlashModel;
//		shader = cgs.media.rocketExplosionShader;
//		sfx = cgs.media.sfx_rockexp;
		mark = cgs.media.oilMarkShader;
//		radius = 64;
//		light = 150;
//		isSprite = qtrue;
//		duration = 2000;
//		lightColor[0] = 0;
//		lightColor[1] = 0.75;
//		lightColor[2] = 0.0;
		break;

	case HT_EXPLOSIVE:
		mod = cgs.media.dishFlashModel;
		shader = cgs.media.rocketExplosionShader;
		sfx = cgs.media.sfx_rockexp;
		mark = cgs.media.burnMarkShader;
		radius = 64;
		light = 300;
		isSprite = qtrue;
		duration = 2000;
		lightColor[0] = 1;
		lightColor[1] = 0.75;
		lightColor[2] = 0.0;
		break;

	case HT_FIRE:
		VectorMA(tr.endpos, 0, tr.plane.normal, origin);
		mod = cgs.media.fireModel;
//		mod = trap_R_RegisterModel( "models/rearfire/flametrail.md3" );
//		shader = cgs.media.flameShader;
//		shader = cgs.media.rocketExplosionShader;
//		sfx = cgs.media.sfx_rockexp;
//		mark = cgs.media.burnMarkShader;
		radius = 64;
		light = 100;
		isSprite = qtrue;
		duration = 10000;
		lightColor[0] = 1;
		lightColor[1] = 0.75;
		lightColor[2] = 0.0;
		break;

	case HT_POISON:
//		mod = cgs.media.dishFlashModel;
//		shader = cgs.media.smokePuffShader;
//		sfx = cgs.media.sfx_rockexp;
//		mark = cgs.media.burnMarkShader;
		radius = 64;
//		isSprite = qtrue;
		duration = 1000;

		dir[0] = crandom() * 120.0F;
		dir[1] = crandom() * 120.0F;
		dir[2] = random() * 40.0F;

		CreateSmokeCloudEntity(origin, dir, 200, radius, duration, 1, 1, 1, 1, cgs.media.smokePuffShader);

		vectoangles(dir, angles);
		angles[YAW] += 120;
		AngleVectors(angles, dir, NULL, NULL);
		CreateSmokeCloudEntity(origin, dir, 200, radius, duration, 1, 1, 1, 1, cgs.media.smokePuffShader);

		angles[YAW] += 120;
		AngleVectors(angles, dir, NULL, NULL);
		CreateSmokeCloudEntity(origin, dir, 200, radius, duration, 1, 1, 1, 1, cgs.media.smokePuffShader);
		break;

	case HT_SMOKE:
//		mod = cgs.media.dishFlashModel;
//		shader = cgs.media.smokePuffShader;
//		sfx = cgs.media.sfx_rockexp;
//		mark = cgs.media.burnMarkShader;
		radius = 96;
//		isSprite = qtrue;
		duration = 2000;

		VectorSet(dir, 0, 0, 1);

		CreateSmokeCloudEntity( origin, dir, 30, radius, duration, 0.75, 0.75, 0.75, 1, cgs.media.smokePuffShader );

		return;

	default:
		return;
	}

	if ( sfx ) {
		trap_S_StartSound( origin, ENTITYNUM_WORLD, CHAN_AUTO, sfx );
	}

	//
	// create the explosion
	//

	if ( mod ) {
		if (hazard == HT_FIRE){
			le = CreateFireEntity( origin, dir, mod, shader, duration );
		}
		else {
			le = CG_MakeExplosion( origin, dir, mod, shader, duration, isSprite );
		}
		le->light = light;
		VectorCopy( lightColor, le->lightColor );
	}


	//
	// impact mark
	//
	if ( mark ){
		alphaFade = (mark == cgs.media.energyMarkShader);	// plasma fades alpha, all others fade color
		CG_ImpactMark( mark, origin, dir, random()*360, 1,1,1,1, alphaFade, radius, qfalse );
	}
}


// *******************************************************
//		Drawing Tools
// *******************************************************


/*
======================
CG_TagExists

Returns true if the tag is not in the model.. ie: not at the vec3_origin
======================
*/
/*
qboolean CG_TagExists( const refEntity_t *parent, qhandle_t parentModel, char *tagName ) {
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
qboolean CG_TagExists( qhandle_t parentModel, char *tagName ) {
	orientation_t	lerped;
	
	// lerp the tag
	return trap_R_LerpTag( &lerped, parentModel, 0, 0,	1.0, tagName );
}


/*
======================
CG_GetTagPosition

Returns the position of the specified tag
======================
*/
void CG_GetTagPosition( const refEntity_t *parent, qhandle_t parentModel, char *tagName, vec3_t origin ) {
	int				i;
	orientation_t	lerped;
	
	// lerp the tag
	trap_R_LerpTag( &lerped, parentModel, parent->oldframe, parent->frame,
		1.0 - parent->backlerp, tagName );

	// FIXME: allow origin offsets along tag?
	VectorCopy( parent->origin, origin );
	for ( i = 0 ; i < 3 ; i++ ) {
		VectorMA( origin, lerped.origin[i], parent->axis[i], origin );
	}
}


/*
====================
CreateFireEntity

  Creates fire localEntities
====================
*/
localEntity_t *CreateFireEntity( vec3_t origin, vec3_t dir, 
								qhandle_t hModel, qhandle_t shader,
								int msec ) {
	localEntity_t	*ex;
	int				offset;
	vec3_t			newOrigin;

	if ( msec <= 0 ) {
		CG_Error( "CreateFireEntity: msec = %i", msec );
	}

	// skew the time a bit so they aren't all in sync
	offset = rand() & 63;

	ex = CG_AllocLocalEntity();
	ex->leType = LE_EXPLOSION;

	// randomly rotate sprite orientation
	ex->refEntity.rotation = 0;
	AxisClear(ex->refEntity.axis);
	VectorCopy(origin, newOrigin);

	ex->startTime = cg.time - offset;
	ex->endTime = ex->startTime + msec;

	// bias the time so all shader effects start correctly
	ex->refEntity.shaderTime = ex->startTime / 1000.0f;

	ex->refEntity.hModel = hModel;
	if (shader)
		ex->refEntity.customShader = shader;

	// set origin
	VectorCopy( newOrigin, ex->refEntity.origin );
	VectorCopy( newOrigin, ex->refEntity.oldorigin );

	ex->color[0] = ex->color[1] = ex->color[2] = 1.0;

	return ex;
}


/*
=================
CreateSmokeCloudEntity

Creates special q3r smoke puff entities
=================
*/
void CreateSmokeCloudEntity(vec3_t origin, vec3_t vel, float speed, int radius, int duration, float r, float g, float b, float a, qhandle_t hShader ){
	vec3_t			velocity;

	VectorNormalize2(vel, velocity);
	velocity[0] += crandom() * 0.2f;
	velocity[1] += crandom() * 0.2f;
	velocity[2] += crandom() * 0.2f;
	VectorNormalize(velocity);
	VectorScale(velocity, speed, velocity);

	CG_SmokePuff( origin, velocity,
				   radius,
				   r, g, b, a,
				   duration,
				   cg.time,
				   0,
				   0,
				   hShader );
}


// *******************************************************
//		Team Tools
// *******************************************************


/*
================
TeamCount

Returns number of players on a team
================
*/
team_t TeamCount( int ignoreClientNum, int team ) {
	int		i;
	int		count = 0;

	for ( i = 0 ; i < cgs.maxclients ; i++ ) {
		if ( i == ignoreClientNum )	continue;
		if ( !cgs.clientinfo[i].infoValid )	continue;
		if ( cgs.clientinfo[i].team == team ) count++;
	}

	return count;
}

int GetTeamAtRank(int rank){
	int		i, j, count;
	int		ranks[4];
	int		counts[4];

	for (i = 0; i < 4; i++){
		counts[i] = TeamCount(-1, TEAM_RED + i);
		ranks[i] = 0;
	}

	for (i = 0; i < 4; i++){
		if (!counts[i]) continue;

		count = 0;
		for (j = 0; j < 4; j++){
			if (!counts[j]) continue;

			if (isRallyRace()){
				if (cg.teamTimes[i] > cg.teamTimes[j]) count++;
			}
			else if (cg.teamScores[i] < cg.teamScores[j]) count++;
		}

//		Com_Printf("teamTimes for team %i = %i, rank %i\n", i, cg.teamTimes[i], count);
//		Com_Printf("teamTimes (%i, %i, %i, %i)\n", cg.teamTimes[0], cg.teamTimes[1], cg.teamTimes[2], cg.teamTimes[3]);

		while(count < 4 && ranks[count]) count++; // rank is taken so move to the next one

		if (count < 4)
			ranks[count] = TEAM_RED + i;
	}

	if (cgs.gametype == GT_CTF && rank > 2){
		return -1;
	}
	else {
		return ranks[rank-1];
	}
}

qboolean TiedWinner( void ){
	int			i, winner;
	qboolean	tied;

	tied = qfalse;
	winner = GetTeamAtRank(1) - TEAM_RED;
	for (i = 0; i < 4; i++){
		if (i == winner) continue;
		if (!TeamCount(-1, TEAM_RED + i)) continue;

		if ((isRallyRace() && cg.teamTimes[winner] == cg.teamTimes[i])
			|| (!isRallyRace() && cg.teamScores[winner] == cg.teamScores[i])){
			tied = qtrue;
			break;
		}
	}

	return tied;
}


#define MAX_REFLECTION_IMAGE_SIZE	20000
qboolean CG_CopyLevelReflectionImage( const char *filename ){
	fileHandle_t	imageFile;
	int				len;
	// FIXME: use malloc?
	byte			data[MAX_REFLECTION_IMAGE_SIZE];

	// load image
	len = trap_FS_FOpenFile( filename, &imageFile, FS_READ );

	if ( !imageFile ){
		Com_Printf( S_COLOR_YELLOW "Q3R Warning: Could not open %s to copy for level reflection mapping.\n", filename);
		return qfalse;
	}
	if ( !len ){
		Com_Printf( S_COLOR_YELLOW "Q3R Warning: Could not open %s: File with 0 length.\n", filename);
		return qfalse;
	}
	if ( len > MAX_REFLECTION_IMAGE_SIZE){
		Com_Printf( S_COLOR_YELLOW "Q3R Warning: Could not open %s: File size exceeds %i bytes.\n", filename, MAX_REFLECTION_IMAGE_SIZE );
		return qfalse;
	}

	trap_FS_Read( data, len, imageFile );
	trap_FS_FCloseFile( imageFile );

	// save image
	trap_FS_FOpenFile( "textures/reflect/reflect.jpg", &imageFile, FS_WRITE );
	if ( !imageFile ){
		Com_Printf( S_COLOR_YELLOW "Q3R Warning: Could not open %s to write level reflection mapping.\n", "textures/reflect/reflect.jpg");
		return qfalse;
	}
	trap_FS_Write( data, len, imageFile );
	trap_FS_FCloseFile( imageFile );

	return qtrue;
}

void CG_DrawModel( vec3_t start, qhandle_t model )
{
	refEntity_t		re;

	memset( &re, 0, sizeof( refEntity_t ) );
  
	re.shaderTime = cg.time / 1000.0f;
	re.reType = RT_MODEL;
	re.renderfx = RF_NOSHADOW;
	re.frame = 1;
	re.hModel = model;
 
	VectorCopy( start, re.origin );
 
	re.shaderRGBA[0] = 255;
    re.shaderRGBA[1] = 255;
    re.shaderRGBA[2] = 255;
    re.shaderRGBA[3] = 255;

	AxisClear( re.axis );

	trap_R_AddRefEntityToScene( &re );
}


void CG_Draw3DLine( vec3_t start, vec3_t end, float r, float g, float b, float a )
{
	refEntity_t		re;

	memset( &re, 0, sizeof( refEntity_t ) );
  
	re.shaderTime = cg.time / 1000.0f;
	re.reType = RT_RAIL_CORE;
	re.renderfx = RF_NOSHADOW;
	re.customShader = cgs.media.sparkShader;
	re.frame = 1;
 
	VectorCopy( end, re.origin );
	VectorCopy( start, re.oldorigin );
 
	re.shaderRGBA[0] = r * 255;
    re.shaderRGBA[1] = g * 255;
    re.shaderRGBA[2] = b * 255;
    re.shaderRGBA[3] = a * 255;

	AxisClear( re.axis );

	trap_R_AddRefEntityToScene( &re );
}


void CG_GetPointOnCurveBetweenCheckpoints( vec3_t start, vec3_t startHandle, vec3_t end, vec3_t endHandle, float f, vec3_t origin )
{
	VectorScale( start, (1-f)*(1-f)*(1-f), origin );
	VectorMA( origin, 3*f*(1-f)*(1-f), startHandle, origin );
	VectorMA( origin, 3*f*f*(1-f), endHandle, origin );
	VectorMA( origin, f*f*f, end, origin );
}


void CG_Draw3DBezierCurve( vec3_t start, vec3_t startDir, vec3_t end, vec3_t endDir, int numDivisions, float r, float g, float b, float a )
{
	refEntity_t		re;
	int				i;
	vec3_t			startHandle, endHandle;
	float			f;

	memset( &re, 0, sizeof( refEntity_t ) );
  
	re.shaderTime = cg.time / 1000.0f;
	re.reType = RT_RAIL_CORE;
	re.renderfx = RF_NOSHADOW;
	re.customShader = cgs.media.sparkShader;
	re.frame = 1;
 
//	VectorCopy( end, re.origin );
//	VectorCopy( start, re.oldorigin );
 
	re.shaderRGBA[0] = r * 255;
    re.shaderRGBA[1] = g * 255;
    re.shaderRGBA[2] = b * 255;
    re.shaderRGBA[3] = a * 255;

	AxisClear( re.axis );

	VectorAdd( start, startDir, startHandle );
	VectorMA( end, -1, endDir, endHandle );

	VectorCopy( start, re.oldorigin );
	for( i = 1; i <= numDivisions; i++ )
	{
		f = i / (float)numDivisions;

		CG_GetPointOnCurveBetweenCheckpoints( start, startHandle, end, endHandle, f, re.origin );
//		VectorScale( start, (1-f)*(1-f)*(1-f), re.origin );
//		VectorMA( re.origin, 3*f*(1-f)*(1-f), startHandle, re.origin );
//		VectorMA( re.origin, 3*f*f*(1-f), endHandle, re.origin );
//		VectorMA( re.origin, f*f*f, end, re.origin );

		trap_R_AddRefEntityToScene( &re );

		VectorCopy( re.origin, re.oldorigin );
	}
}


// *******************************************************
//		Misc Tools
// *******************************************************

float Q3VelocityToRL( float length ) {
	if ( cg_metricUnits.integer ){
		return length / CP_M_2_QU * 3.6f;
	}
	else {
		return length / CP_FT_2_QU * 3600.0f / 5280.0f;
	}
}

float Q3DistanceToRL( float length ) {
	if ( cg_metricUnits.integer ){
		return length / CP_M_2_QU / 1000.0f;
	}
	else {
		return length / CP_FT_2_QU / 5280.0f;
	}
}

qboolean isRallyRace( void ){
	return (cgs.gametype == GT_RACING
		|| cgs.gametype == GT_RACING_DM
		|| cgs.gametype == GT_TEAM_RACING
		|| cgs.gametype == GT_TEAM_RACING_DM);
}

qboolean isRallyNonDMRace( void ){
	return (cgs.gametype == GT_RACING
		|| cgs.gametype == GT_TEAM_RACING);
}

/*
=================
isRaceObserver
=================
*/
qboolean isRaceObserver( int clientNum ){
	return (cg_entities[clientNum].finishRaceTime && cg_entities[clientNum].finishRaceTime + RACE_OBSERVER_DELAY < cg.time);
}

qboolean CG_InsideBox( vec3_t mins, vec3_t maxs, vec3_t pos ){
	if (pos[0] < mins[0] || pos[0] > maxs[0]) return qfalse;
	if (pos[1] < mins[1] || pos[1] > maxs[1]) return qfalse;
	if (pos[2] < mins[2] || pos[2] > maxs[2]) return qfalse;

	return qtrue;
}
