/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2002-2025 Q3Rally Team (Per Thormann - q3rally@gmail.com)

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

float colors[4][4] = { 
//		{ 0.2, 1.0, 0.2, 1.0 } , { 1.0, 0.2, 0.2, 1.0 }, {0.5, 0.5, 0.5, 1} };
		{ 1.0f, 0.69f, 0.0f, 1.0f } ,	  // normal
		{ 1.0f, 0.2f, 0.2f, 1.0f },	      // low health
		{ 0.5f, 0.5f, 0.5f, 1.0f },       // weapon firing
		{ 1.0f, 1.0f, 1.0f, 1.0f }		  // health > 100
};


/*
=====================
CG_DrawRearviewMirror
=====================
*/
void CG_DrawRearviewMirror( float x, float y, float w, float h) {
//	static int  lastLowFPSTime;
	int		i;
//	int		fps;
	float	mx, my, mw, mh;
	int		tmp;

	if ( !cg_drawRearView.integer )
		return;

	if (cg.snap->ps.pm_type == PM_INTERMISSION)
		return;

	if (cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_SPECTATOR)
		return;

	mx = x - 8;
	my = y - 7;
	mw = w * 1.0534F;
	mh = h * 1.2F;

	CG_AdjustFrom640( &x, &y, &w, &h );

	cg.mirrorRefdef.x = x;
	cg.mirrorRefdef.y = y;
	cg.mirrorRefdef.width = w;
	cg.mirrorRefdef.height = h;

	cg.mirrorRefdef.fov_x = 70;
	tmp = cg.mirrorRefdef.width / tan( cg.mirrorRefdef.fov_x / 360 * M_PI );
	cg.mirrorRefdef.fov_y = atan2( cg.mirrorRefdef.height, tmp );
	cg.mirrorRefdef.fov_y = cg.mirrorRefdef.fov_y * 360 / M_PI;

	cg.mirrorRefdef.time = cg.time;
	cg.mirrorRefdef.rdflags = 0;

	// add entities and graphics to rearview scene
	if (cg_rearViewRenderLevel.integer & RL_MARKS){
		CG_AddMarks();
	}

	if (cg_rearViewRenderLevel.integer & RL_SMOKE){
		CG_AddLocalEntities();
	}

	if (cg_rearViewRenderLevel.integer & RL_PLAYERS || cg_rearViewRenderLevel.integer & RL_OBJECTS){
		for (i = 0; i < cg.snap->numEntities; i++){
			if (!(cg_rearViewRenderLevel.integer & RL_OBJECTS)){
				// skip non-players
				if ( cg.snap->entities[i].eType != ET_PLAYER ) continue;
			}

			if (!(cg_rearViewRenderLevel.integer & RL_PLAYERS)){
				// skip players
				if ( cg.snap->entities[i].eType == ET_PLAYER ) continue;
			}

			// FIXME: dont re-lerp entity
			CG_AddCEntity( &cg_entities[ cg.snap->entities[ i ].number ] );
		}
	}

	trap_R_RenderScene( &cg.mirrorRefdef );
	CG_DrawPic( mx, my, mw, mh, cgs.media.rearviewMirrorShader );
}

/*
==========================
CG_DrawMMap
Draws the minimap overlay.
==========================
*/

void CG_DrawMMap(float x, float y, float w, float h) {
	
    float mx, my, mw, mh, tmp;
    int i;
    
    if (!cg_drawMMap.integer)
		return;

	if (cg.snap->ps.pm_type == PM_INTERMISSION)
		return;

	if (cgs.clientinfo[cg.snap->ps.clientNum].team == TEAM_SPECTATOR)
		return;
	
	// Store original dimensions for overlay shader
	mx = x,
    my = y,
    mw = w * 0.68f;
    mh = h * 0.68f;

    // Convert coordinates for different screen resolutions
	CG_AdjustFrom640(&x, &y, &w, &h);

    // Set up minimap rendering definition
	cg.mmapRefdef.x = x;
	cg.mmapRefdef.y = y;
	cg.mmapRefdef.width = 240;
	cg.mmapRefdef.height = 180;
    cg.mmapRefdef.fov_x = 70;
	
    // Calculate fov_y from fov_x and aspect ratio
	tmp = cg.mmapRefdef.width / tan(cg.mmapRefdef.fov_x / 360.0f * M_PI);
	cg.mmapRefdef.fov_y = atan2(cg.mmapRefdef.height, tmp) * 360.0f / M_PI;

	cg.mmapRefdef.time = cg.time;
	cg.mmapRefdef.rdflags = 0;

    // Optional effects depending on render level
	if (cg_rearViewRenderLevel.integer & RL_MARKS) {
		CG_AddMarks();
	}

	if (cg_rearViewRenderLevel.integer & RL_SMOKE) {
		CG_AddLocalEntities();
	}
	

	if (cg_rearViewRenderLevel.integer & (RL_PLAYERS | RL_OBJECTS)) {
		for (i = 0; i < cg.snap->numEntities; i++) {
			entityState_t *es = &cg.snap->entities[i];

			qboolean isPlayer = (es->eType == ET_PLAYER);

    // Skip non-players if RL_OBJECTS is disabled
			if (!(cg_rearViewRenderLevel.integer & RL_OBJECTS) && !isPlayer)
				continue;

			// Skip players if RL_PLAYERS is disabled
			if (!(cg_rearViewRenderLevel.integer & RL_PLAYERS) && isPlayer)
				continue;

			// Add player or object to minimap
			CG_AddCEntity(&cg_entities[es->number]);

		}
	}
	
    // Render the minimap scene
	trap_R_RenderScene(&cg.mmapRefdef);

	// Draw the minimap overlay background shader
	CG_DrawPic(mx, my, mw, mh, cgs.media.MMapShader);
    
}

/*
========================
CG_DrawArrowToCheckpoint
========================
*/
static float CG_DrawArrowToCheckpoint( float y ) {
	centity_t	*cent;
	//vec3_t		dir;
	vec3_t		forward, origin, angles;
	int			i;
	float		angle1, angle2, angleDiff;
	int			x, w;
	float		fx, fy, fw, fh;
	float		*color;
	refdef_t		refdef;
	refEntity_t		ent;
	vec3_t		mins, maxs, v;

	if (cg_entities[cg.snap->ps.clientNum].finishRaceTime)
		return y;

	for (i = 0; i < MAX_GENTITIES; i++){
		cent = &cg_entities[i];
		if (cent->currentState.eType != ET_CHECKPOINT) continue;
		if (cent->currentState.weapon != cg.snap->ps.stats[STAT_NEXT_CHECKPOINT]) continue;

		break;
	}

	if (i == MAX_GENTITIES)
		return y; // no checkpoints found

//	VectorSubtract(cent->currentState.origin, cg.predictedPlayerState.origin, dir);
//	angle2 = vectoyaw(dir);

	// find the distance from the edge of the bounding box
	trap_R_ModelBounds( cgs.inlineDrawModel[cent->currentState.modelindex], mins, maxs );

	// if the checkpoint was one with no target then mins and maxs are relative to the origin
	if( cent->currentState.frame == 0 )
	{
		VectorAdd( mins, cent->currentState.origin, mins );
		VectorAdd( maxs, cent->currentState.origin, maxs );
	}

	for ( i = 0 ; i < 3 ; i++ ) {
		if ( cg.predictedPlayerState.origin[i] < mins[i] ) {
			v[i] = mins[i] - cg.predictedPlayerState.origin[i];
		} else if ( cg.predictedPlayerState.origin[i] > maxs[i] ) {
			v[i] = maxs[i] - cg.predictedPlayerState.origin[i];
		} else {
			v[i] = 0;
		}
	}
	if( v[0] == 0 && v[1] == 0 && v[2] == 0 )
		angle2 = cg.predictedPlayerState.viewangles[YAW];
	else
		angle2 = vectoyaw(v);

	if (cg_checkpointArrowMode.integer == 1){
		AngleVectors(cg.refdefViewAngles, forward, NULL, NULL);
		angle1 = vectoyaw(forward);

		angleDiff = AngleDifference(angle1, angle2);

// draw arrow:
		VectorSet(origin, 80, 0, 20);
		VectorClear(angles);
		angles[YAW] = -angleDiff;

		fx = 320 - 64;
		fy = 16;
		fw = 128;
		fh = 96;
		CG_AdjustFrom640( &fx, &fy, &fw, &fh );

		memset( &refdef, 0, sizeof( refdef ) );

		memset( &ent, 0, sizeof( ent ) );
		AnglesToAxis( angles, ent.axis );
		VectorCopy( origin, ent.origin );
		VectorCopy( origin, ent.lightingOrigin );
		ent.hModel = cgs.media.checkpointArrow;
//		ent.customSkin = trap_R_RegisterShader("gfx/hud/arrow");

		ent.renderfx = RF_NOSHADOW;		// no stencil shadows

		refdef.rdflags = RDF_NOWORLDMODEL;

		vectoangles( origin, angles );
		AnglesToAxis( angles, refdef.viewaxis );

		refdef.fov_x = 40;
		refdef.fov_y = 30;

		refdef.x = fx;
		refdef.y = fy;
		refdef.width = fw;
		refdef.height = fh;

		refdef.time = cg.time;

		trap_R_ClearScene();
		trap_R_AddRefEntityToScene( &ent );
		trap_R_RenderScene( &refdef );
	}
//end draw arrow

	AngleVectors(cg.predictedPlayerEntity.lerpAngles, forward, NULL, NULL);
	angle1 = vectoyaw(forward);

	angleDiff = AngleDifference(angle1, angle2);

//	if (VectorLength(dir) < 20.0f)
//		return y;

	if (fabs(angleDiff) > 100){
		cg.wrongWayTime = cg.time;
		if( !cg.wrongWayStartTime )
			cg.wrongWayStartTime = cg.time;
	}
	else
		cg.wrongWayStartTime = 0;

	if( !cg.wrongWayStartTime || cg.wrongWayStartTime > cg.time - 2000 )
		return y;

	w = BIGCHAR_WIDTH * CG_DrawStrlen( "WRONG WAY!" );
	x = ( SCREEN_WIDTH - w ) / 2;

	color = CG_FadeColor( cg.wrongWayTime, 300 );
	if ( !color ) {
		return y;
	}
	trap_R_SetColor( color );

/* Developer Mode for Racing Bots

	if (cg_developer.integer)
		CG_Draw3DLine( cent->currentState.origin, cg.snap->ps.origin );
        
*/
	CG_SetScreenPlacement(PLACE_CENTER, PLACE_CENTER);
	CG_DrawStringExt( x, SCREEN_HEIGHT * .30, "WRONG WAY!", color, qfalse, qtrue, BIGCHAR_WIDTH, (int)(BIGCHAR_WIDTH * 1.5), 0 );
	CG_PopScreenPlacement();

	return y;
}

// takes a 3d coord and change it to screen coords
/*
// Thanks to Golliwog who implemented this bit of code in q3f 
static float CG_SFK_AngleToCoord( float angle, int coordRange, float fov, qboolean reverseCoord ){
	// Take an angle and return the coord it maps to (may be out of visible range)
	// The conversion is: correct fov to degrees: (90.0 / fov), convert to radians: M_PI / 180,
	// obtain tangent (results in -1 - 1, at least in visible coords). The first two steps can
	// be combined, hence the M_PI * 0.5f / fov.
	
	angle = tan( angle * M_PI * 0.5f / fov );
	angle = coordRange * (reverseCoord ? (1.0f - angle) : (1.0f + angle));
	return( angle );
}

code:
--------------------------------------------------------------------------------
	angles[PITCH] = AngleNormalize180( angles[PITCH] - cg.refdefViewAngles[PITCH] );
	adjustedY = CG_SFK_AngleToCoord( angles[PITCH], 240, cg.refdef.fov_y, qfalse );
	angles[YAW] = AngleNormalize180( angles[YAW] - cg.refdefViewAngles[YAW] );
	adjustedX = CG_SFK_AngleToCoord( angles[YAW], 320, cg.refdef.fov_x, qtrue );

*/

/*
================
CG_DrawTimes
================
*/
static float CG_DrawTimes( float y ) {
	centity_t		*cent;
	int			lapTime;
	int			totalTime;
	int			x;
	char		s[128];
	char		*time;

	//ps = &cg.snap->ps;
	cent = &cg_entities[cg.snap->ps.clientNum];
	
	if ( cent->finishRaceTime ){
		lapTime = cent->finishRaceTime - cent->startLapTime;
		totalTime = cent->finishRaceTime - cent->startRaceTime;

	}
	else if ( cent->startRaceTime ){
		lapTime = cg.time - cent->startLapTime;
		totalTime = cg.time - cent->startRaceTime;
	}
	else {
		lapTime = 0;
		totalTime = 0;
		
	}

//
// Best Time
//
  
	if ( cgs.gametype != GT_DERBY && cgs.gametype != GT_LCS ){
		time = getStringForTime( cent->bestLapTime );
		
		Com_sprintf(s, sizeof(s), "B: %s", time);
//		x = 600 - CG_DrawStrlen(s) * TINYCHAR_WIDTH;
        x = 636 - 80;
		CG_FillRect ( x, y, 90, 18, bgColor );
		x+= 10;		
		y+= 4;
		CG_DrawTinyDigitalStringColor( x, y, s, colorWhite);
		y += TINYCHAR_HEIGHT + 4;
	}

//
// Lap Time
//

	

	if ( cgs.gametype != GT_DERBY && cgs.gametype != GT_LCS ){
		time = getStringForTime(lapTime);

		Com_sprintf(s, sizeof(s), "L: %s", time);
//		x = 600 - CG_DrawStrlen(s) * TINYCHAR_WIDTH;
        x = 636 - 80;
        CG_FillRect( x, y, 90, 18, bgColor );
        x+= 10;
        y+= 4;
		CG_DrawTinyDigitalStringColor( x, y, s, colorWhite);
		y += TINYCHAR_HEIGHT + 4;
	}

	

	//
	// Total Time
	//

	time = getStringForTime(totalTime);

	/*
	Com_sprintf(s, sizeof(s), "TOTAL TIME: %s", time);
	x = 630 - CG_DrawStrlen(s) * SMALLCHAR_WIDTH;

	CG_DrawSmallStringColor( x, y, s, colors[0]);
	y += SMALLCHAR_HEIGHT;
	*/

	Com_sprintf(s, sizeof(s), "T: %s", time);

	x = 636 - 80;
	CG_FillRect( x, y, 90, 18, bgColor );
	x += 10;
	y += 4;
	CG_DrawTinyDigitalStringColor( x, y, s, colorWhite);
	y += TINYCHAR_HEIGHT + 4;

	return y;
}




/*
================
CG_DrawLaps
================
*/
static float CG_DrawLaps( float y ) {
	centity_t		*cent;
	//playerState_t	*ps;
	int			curLap;
	int			numLaps;
	char		s[64];
	int			x;

	//ps = &cg.snap->ps;
	cent = &cg_entities[cg.snap->ps.clientNum];

	curLap = cent->currentLap;
	numLaps = cgs.laplimit;

	Com_sprintf(s, sizeof(s), "LAP: %i/%i", curLap, numLaps);

	x = 636 - 80;
	CG_FillRect( x, y, 90, 18, bgColor );
	x += 10;
	y += 4;
	CG_DrawTinyDigitalStringColor( x, y, s, colorWhite);
	y += TINYCHAR_HEIGHT + 4;

	return y;
}


/*
======================
CG_DrawCurrentPosition
======================
*/
static float CG_DrawCurrentPosition( float y ) {
	centity_t		*cent;
	//playerState_t	*ps;
	int			pos;
	char		s[64];
	float		x, width, height;
	//float		foreground[4] = { 0, 0, 0.75, 1.0 };

	//ps = &cg.snap->ps;
	cent = &cg_entities[cg.snap->ps.clientNum];

	pos = cent->currentPosition;

	Com_sprintf(s, sizeof(s), "POS: ");

	x = 636 - 80;
	width = 90;
	height = 18;

	CG_FillRect( x, y, width, height, bgColor );

	x += 10;
	y += 4;

	CG_DrawTinyDigitalStringColor( x, y, s, colorWhite);

	x += TINYCHAR_WIDTH * 5;

	CG_DrawTinyDigitalStringColor( x, y, va("%i/%i", pos, cgs.numRacers), colorWhite);

	y += 20;

	return y;
}


/*
========================
CG_DrawCarAheadAndBehind
========================
*/
static float CG_DrawCarAheadAndBehind( float y ) {
	centity_t	*cent, *other;
	char		player[64];
	int			i, j, num;
	float		x, width, height;
	int			startPos, endPos;
	char		s[64];
	float		background[4] = { 0, 0, 0, 0.5 };
	float		selected[4] = { 0.75, 0.0, 0.0, 0.5 };

	//ps = &cg.snap->ps;
	cent = &cg_entities[cg.snap->ps.clientNum];

	startPos = cent->currentPosition - 4 < 1 ? 1 : cent->currentPosition - 4;
	endPos = startPos + 8 > cgs.numRacers ? cgs.numRacers : startPos + 8;
	startPos = endPos - 8 < 1 ? 1 : endPos - 8;

	x = 636 - 80;
	width = 90;
	height = TINYCHAR_HEIGHT;

	for (i = startPos; i <= endPos; i++){
		num = -1;
		for (j = 0; j < cgs.maxclients; j++){
			other = &cg_entities[j];
			if (!other) continue;

			if (other->currentPosition == i){
				num = other->currentState.clientNum;
			}
		}

		if (num < 0 || num > cgs.maxclients) continue;

		if (num == cent->currentState.clientNum){
			CG_FillRect( x, y, width, height, selected );
		}
		else {
			CG_FillRect( x, y, width, height, background );
		}

		Q_strncpyz(player, cgs.clientinfo[num].name, 16 );
		Com_sprintf(s, sizeof(s), "%i-%s", cg_entities[num].currentPosition, player);
		CG_DrawTinyDigitalStringColor( x, y, s, colorWhite);

		y += TINYCHAR_HEIGHT;

	}

	return y;
}

#if 0
/*
====================
CG_DrawHUD_DerbyList
====================

void CG_DrawHUD_DerbyList(float x, float y){
	int			i;
	vec4_t		color;
	centity_t	*cent;
	char		*time;
	float		playTime;

	// draw heading
	CG_FillRect(x, y, 536, 18, bgColor);

	// name
	CG_DrawTinyStringColor( x + 42, y, "PLAYER:", colorWhite);

	// time
	CG_DrawTinyStringColor( x + 206, y, "TIME:", colorWhite);

	// dmg dealt
	CG_DrawTinyStringColor( x + 294, y, "DMG DEALT:", colorWhite);

	// dmg taken
	CG_DrawTinyStringColor( x + 442, y, "DMG TAKEN:", colorWhite);

	y += 20;

	// draw top 8 players
	for (i = 0; i < 8; i++){
		if (cg.scores[i].scoreFlags < 0) continue; // score is not valid so skip it

		cent = &cg_entities[cg.scores[i].client];
		if (!cent) continue;

		CG_FillRect(x, y, 536, 18, bgColor);

		Vector4Copy(colorWhite, color);
		if (cg.scores[i].client == cg.snap->ps.clientNum){
			if (cg.snap->ps.stats[STAT_HEALTH] <= 0 || cgs.clientinfo[cg.scores[i].client].team == TEAM_SPECTATOR)
				Vector4Copy(colorMdGrey, color);
		}
		else if (cent->currentState.eFlags & EF_DEAD || cgs.clientinfo[cg.scores[i].client].team == TEAM_SPECTATOR){
			Vector4Copy(colorMdGrey, color);
		}

		playTime = 0;
		if (cent->finishRaceTime){
			playTime = cent->finishRaceTime - cent->startLapTime;
		}
		else if (cent->startRaceTime){
			playTime = cg.time - cent->startLapTime;
		}
		time = getStringForTime(playTime);

		// num
		CG_DrawTinyStringColor( x + 6, y, va("0%i", (i+1)), color);

		// name
		CG_DrawTinyStringColor( x + 42, y, cgs.clientinfo[cg.scores[i].client].name, color);

		// time
		CG_DrawTinyStringColor( x + 192, y, time, color);

		// dmg dealt
		CG_DrawTinyStringColor( x + 326, y, va("%i", cg.scores[i].damageDealt), color);

		// dmg taken
		CG_DrawTinyStringColor( x + 474, y, va("%i", cg.scores[i].damageTaken), color);

		y += 20;
	}
}
*/
#endif

/*
============
CG_DrawSpeed
============
*/
static float CG_DrawSpeed( float y ) {
	playerState_t	*ps;
	int				vel_speed;
	vec3_t			forward, origin, angles, mins, maxs;
	int				x, yorg;
	float			x2, y2, w, h;
	refdef_t		refdef;
	refEntity_t		ent;

	x = 630;
	yorg = y;

	ps = &cg.predictedPlayerState;

	AngleVectors( ps->viewangles, forward, NULL, NULL );

	// use actual speed
	vel_speed = (int)fabs( Q3VelocityToRL( DotProduct(ps->velocity, forward) ) );
/*
#ifdef Q3_VM
	if (ps->stats[STAT_GEAR] == -1)
		vel_speed = (int)fabs(10.0f * Q3UnitsToRL(WHEEL_RADIUS * min(cg_entities[cg.snap->ps.clientNum].wheelSpeeds[0], cg_entities[cg.snap->ps.clientNum].wheelSpeeds[1])));
	else
		vel_speed = (int)fabs(10.0f * Q3UnitsToRL(WHEEL_RADIUS * max(cg_entities[cg.snap->ps.clientNum].wheelSpeeds[0], cg_entities[cg.snap->ps.clientNum].wheelSpeeds[1])));
#else
	if (ps->stats[STAT_GEAR] == -1)
		vel_speed = (int)fabs(Q3UnitsToRL(WHEEL_RADIUS * min(cg_entities[cg.snap->ps.clientNum].wheelSpeeds[0], cg_entities[cg.snap->ps.clientNum].wheelSpeeds[1])));
	else
		vel_speed = (int)fabs(Q3UnitsToRL(WHEEL_RADIUS * max(cg_entities[cg.snap->ps.clientNum].wheelSpeeds[0], cg_entities[cg.snap->ps.clientNum].wheelSpeeds[1])));
#endif
*/

	// draw speedometer here
	x2 = x - 96;
	y2 = y - 96;
	CG_DrawPic( x2, y2, 96, 96, trap_R_RegisterShaderNoMip("gfx/hud/gauge01"));
	

	// draw digital speed
	x -= 48 + (CG_DrawStrlen(va("%i", vel_speed)) * SMALLCHAR_WIDTH) / 2;
	y -= 28;
	CG_DrawSmallDigitalStringColor( x, y, va("%i", vel_speed), colorWhite);

	// draw needle

	w = h = 96;
	CG_AdjustFrom640( &x2, &y2, &w, &h );

	memset( &refdef, 0, sizeof( refdef ) );
	memset( &ent, 0, sizeof( ent ) );

	ent.hModel = trap_R_RegisterModel("gfx/hud/needle.md3");
	ent.customShader = trap_R_RegisterShader("gfx/hud/needle01");
	ent.renderfx = RF_NOSHADOW;		// no stencil shadows

	trap_R_ModelBounds(ent.hModel, mins, maxs);

	// origin[2] = -0.5 * ( mins[2] + maxs[2] );
	origin[2] = 0;
	origin[1] = 0.5 * ( mins[1] + maxs[1] );
	origin[0] = ( maxs[2] - mins[2] ) / 0.268;

	VectorClear(angles);
	angles[YAW] -= 90;
	angles[PITCH] = -150.0f + (300.0f * vel_speed / 200.0f);
	AnglesToAxis( angles, ent.axis );
	VectorCopy(origin, ent.origin);

	refdef.rdflags = RDF_NOWORLDMODEL;

	AxisClear( refdef.viewaxis );

	refdef.fov_x = 30;
	refdef.fov_y = 30;

	refdef.x = x2;
	refdef.y = y2;
	refdef.width = w;
	refdef.height = h;

	refdef.time = cg.time;

	trap_R_ClearScene();
	trap_R_AddRefEntityToScene( &ent );
	trap_R_RenderScene( &refdef );

	// draw center here
	x = 630;
	y = yorg;

	x -= 60;
	y -= 60;
	CG_DrawPic( x, y, 24, 24, trap_R_RegisterShaderNoMip("gfx/hud/center01"));

	// draw gear over center of gauge
	if ( cg.predictedPlayerState.stats[STAT_GEAR] == -1 )
		CG_DrawSmallDigitalStringColor( x+10, y+4, "R", colorWhite);
	else if ( cg.predictedPlayerState.stats[STAT_GEAR] == 0 )
		CG_DrawSmallDigitalStringColor( x+10, y+4, "N", colorWhite);
	else
		CG_DrawSmallDigitalStringColor( x+10, y+4, va("%i", cg.predictedPlayerState.stats[STAT_GEAR]), colorWhite);

	y -= 39;

	y -= SMALLCHAR_HEIGHT;

	return y;
}

/*
static float CG_DrawSDKMessage( float y ) {
	int			x, w;
	vec4_t		bg_color;

	switch (cgs.clientinfo[cg.snap->ps.clientNum].team){
	case TEAM_RED:
		Vector4Copy(colorRed, bg_color);
		bg_color[3] = 0.5;
		break;

	case TEAM_BLUE:
		Vector4Copy(colorBlue, bg_color);
		bg_color[3] = 0.5;
		break;

	case TEAM_GREEN:
		Vector4Copy(colorGreen, bg_color);
		bg_color[3] = 0.5;
		break;

	case TEAM_YELLOW:
		Vector4Copy(colorYellow, bg_color);
		bg_color[3] = 0.5;
		break;
	
	default:
		Vector4Copy(bgColor, bg_color);
	}

	x = 4;
	w = (CG_DrawStrlen("Not the finished game.") * TINYCHAR_WIDTH);

	y -= 3*TINYCHAR_HEIGHT+2;

	CG_FillRect( x, y, w, 3*TINYCHAR_HEIGHT+2, bg_color );

	CG_DrawTinyStringColor( x, y, Q3_VERSION, colorWhite);
	y += TINYCHAR_HEIGHT;
	CG_DrawTinyStringColor( x, y, "Beta Version", colorWhite);
	y += TINYCHAR_HEIGHT;
	CG_DrawTinyStringColor( x, y, "Not the finished game.", colorWhite);
	y += TINYCHAR_HEIGHT;

	y -= 3*TINYCHAR_HEIGHT+2;

	return y;
}
*/

#if 0
/*
===========
CG_DrawGear
===========
*/
 static float CG_DrawGear( float y ) {
	CG_DrawSmallDigitalStringColor( 560, y, va("Gear: %d", cg.predictedPlayerState.stats[STAT_GEAR]), colors[0]);
	y -= SMALLCHAR_HEIGHT;
	CG_DrawTinyDigitalStringColor( 560, y, va("RPM: %d", cg.predictedPlayerState.stats[STAT_RPM]), colorWhite);
	y -= SMALLCHAR_HEIGHT;
	return y;
}
#endif


/*
	// for translating a 3d point to the screen ?
	planeCameraDist = 50;
	VectorSubtract(targetv, cg.refdef.viewaxis, dir);
	viewdist = DotProduct(dir, cg.refdef.viewaxis[0]);
	planedist = viewdist - planeCameraDist;
	VectorMA(targetv, -planedist, cg.refdef.viewaxis[0], pointOnPlane);
	VectorMA(pointOnPlane, -(planedist / viewdist) * DotProduct(dir, cg.refdef.viewaxis[1]), cg.refdef.viewaxis[1], pointOnPlane);
	VectorMA(pointOnPlane, -(planedist / viewdist) * DotProduct(dir, cg.refdef.viewaxis[2]), cg.refdef.viewaxis[2], pointOnPlane);
*/

float CG_DrawUpperRightHUD( float y ) {
	int		i;

	// FIXME: this should be moved somewhere else
	cgs.numRacers = 0;
	for ( i = 0 ; i < cgs.maxclients ; i++ ) {
		if (!cgs.clientinfo[i].infoValid) continue;
		if (cgs.clientinfo[i].team == TEAM_SPECTATOR) continue;
		if (cg.scores[i].ping == -1) continue;

		cgs.numRacers++;
	}

	if (cgs.clientinfo[cg.snap->ps.clientNum].team != TEAM_SPECTATOR){
		if (isRallyRace()){
			y = CG_DrawArrowToCheckpoint( y );
			y = CG_DrawTimes( y );
			y = CG_DrawLaps( y );
			y = CG_DrawCurrentPosition( y );
			y = CG_DrawCarAheadAndBehind( y );
		}
		else if (cgs.gametype == GT_DERBY || cgs.gametype == GT_LCS )
			y = CG_DrawTimes( y );
// 0.5
//			CG_DrawHUD_DerbyList(44, 130);
			
	}

	if (!isRallyNonDMRace() && cgs.gametype != GT_DERBY && cgs.gametype != GT_LCS){
		y = CG_DrawScores( 636, y );
	}

	return y;
}


float CG_DrawLowerRightHUD( float y ) {
	if (cgs.clientinfo[cg.snap->ps.clientNum].team != TEAM_SPECTATOR){
		y = CG_DrawSpeed( y );
//		y = CG_DrawGear( y );
	}

	return y;
}


float CG_DrawLowerLeftHUD( float y ) {
	// check if there is rear ammo being displayed
	int		i;

	y += 36;
	for (i = RWP_SMOKE; i < WP_NUM_WEAPONS; i++){
		if (cg.snap->ps.stats[STAT_WEAPONS] & ( 1 << i )){
			if (cg.snap->ps.ammo[ i ]){
				y -= 36;
				break;
			}
		}
	}
	
// Comment this out in the full release Version

// y = CG_DrawSDKMessage( y );

	return y;
 }
