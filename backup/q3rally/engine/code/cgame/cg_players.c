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
//
// cg_players.c -- handle the media and animation for player entities
#include "cg_local.h"

char	*cg_customSoundNames[MAX_CUSTOM_SOUNDS] = {
// Q3Rally Code Start
	"*engine0.wav",
	"*engine1.wav",
	"*engine2.wav",
	"*engine3.wav",
	"*engine4.wav",
	"*engine5.wav",
	"*engine6.wav",
	"*engine7.wav",
	"*engine8.wav",
	"*engine9.wav",
	"*engine10.wav",
	"*horn.wav",
	"*falling1.wav",
/*
	"*death1.wav",
	"*death2.wav",
	"*death3.wav",
	"*jump1.wav",
	"*pain25_1.wav",
	"*pain50_1.wav",
	"*pain75_1.wav",
	"*pain100_1.wav",
	"*falling1.wav",
	"*gasp.wav",
	"*drown.wav",
	"*fall1.wav",
	"*taunt.wav"
*/
// END
};


/*
================
CG_CustomSound

================
*/
sfxHandle_t	CG_CustomSound( int clientNum, const char *soundName ) {
	clientInfo_t *ci;
	int			i;

	if ( soundName[0] != '*' ) {
		return trap_S_RegisterSound( soundName, qfalse );
	}

	if ( clientNum < 0 || clientNum >= MAX_CLIENTS ) {
		clientNum = 0;
	}
	ci = &cgs.clientinfo[ clientNum ];

	for ( i = 0 ; i < MAX_CUSTOM_SOUNDS && cg_customSoundNames[i] ; i++ ) {
		if ( !strcmp( soundName, cg_customSoundNames[i] ) ) {
			return ci->sounds[i];
		}
	}

	CG_Error( "Unknown custom sound: %s", soundName );
	return 0;
}



/*
=============================================================================

CLIENT INFO

=============================================================================
*/

#if 0 // ZTM: Not used by Q3Rally
/*
======================
CG_ParseAnimationFile

Read a configuration file containing animation counts and rates
models/players/visor/animation.cfg, etc
======================
*/
// Q3Rally Code Start (change function to do q3rally related stuff)
static qboolean	CG_ParseAnimationFile( const char *filename, clientInfo_t *ci ) {
	char		*text_p, *prev;
	int			len;
	char		*token;
	char		text[20000];
	fileHandle_t	f;
// Q3Rally Code Start
//	char		texturename[MAX_QPATH];

	// setup default incase this fails for some reason
	Q_strncpyz( ci->plateName, DEFAULT_PLATE, sizeof( ci->plateName ) );
// END

	// load the file
	len = trap_FS_FOpenFile( filename, &f, FS_READ );
	if ( len <= 0 ) {
		return qfalse;
	}
	if ( len >= sizeof( text ) - 1 ) {
		CG_Printf( "File %s too long\n", filename );
		trap_FS_FCloseFile( f );
		return qfalse;
	}
	trap_FS_Read( text, len, f );
	text[len] = 0;
	trap_FS_FCloseFile( f );

	// parse the text
	text_p = text;

	// read optional parameters
	while ( 1 ) {
		prev = text_p;	// so we can unget
		token = COM_Parse( &text_p );
		if ( !token ) {
			break;
		}

/*
		if ( !Q_stricmp( token, "plate" ) ) {
			token = COM_Parse( &text_p );
			if ( !token ) {
				break;
			}

			Com_sprintf( texturename, sizeof( texturename ), "models/players/plates/player%d.tga", ci->clientNum );
			if ( !Q_stricmp( token, "usa" ) ){
				Q_strncpyz( ci->plateName, "plate_usa", sizeof( ci->plateName ) );
				CreateLicensePlateImage(va("models/players/plates/%s_usa.tga", ci->plateSkinName), texturename, ci->name, 10);
			}
			else{
				Q_strncpyz( ci->plateName, "plate_eu", sizeof( ci->plateName ) );
				CreateLicensePlateImage(va("models/players/plates/%s_eu.tga", ci->plateSkinName), texturename, ci->name, 20);
			}

			continue;
		}
		else
*/
		if ( !Q_stricmp( token, "oppositeRoll" ) ) {
			token = COM_Parse( &text_p );
			if ( !token ) {
				ci->oppositeRoll = qfalse;
				break;
			}
			ci->oppositeRoll = atoi( token );
		}
		else {
			break;
		}
	}

	return qtrue;
}
// END
#endif

// SKWID( removed function )
/*
static qboolean	CG_ParseAnimationFile( const char *filename, clientInfo_t *ci ) {
	char		*text_p, *prev;
	int			len;
	int			i;
	char		*token;
	float		fps;
	int			skip;
	char		text[20000];
	fileHandle_t	f;
	animation_t *animations;

	animations = ci->animations;

	// load the file
	len = trap_FS_FOpenFile( filename, &f, FS_READ );
	if ( len <= 0 ) {
		return qfalse;
	}
	if ( len >= sizeof( text ) - 1 ) {
		CG_Printf( "File %s too long\n", filename );
		trap_FS_FCloseFile( f );
		return qfalse;
	}
	trap_FS_Read( text, len, f );
	text[len] = 0;
	trap_FS_FCloseFile( f );

	// parse the text
	text_p = text;
	skip = 0;	// quite the compiler warning

	ci->footsteps = FOOTSTEP_NORMAL;
	VectorClear( ci->headOffset );
	ci->gender = GENDER_MALE;
	ci->fixedlegs = qfalse;
	ci->fixedtorso = qfalse;

	// read optional parameters
	while ( 1 ) {
		prev = text_p;	// so we can unget
		token = COM_Parse( &text_p );
		if ( !token[0] ) {
			break;
		}
		if ( !Q_stricmp( token, "footsteps" ) ) {
			token = COM_Parse( &text_p );
			if ( !token[0] ) {
				break;
			}
			if ( !Q_stricmp( token, "default" ) || !Q_stricmp( token, "normal" ) ) {
				ci->footsteps = FOOTSTEP_NORMAL;
			} else if ( !Q_stricmp( token, "boot" ) ) {
				ci->footsteps = FOOTSTEP_BOOT;
			} else if ( !Q_stricmp( token, "flesh" ) ) {
				ci->footsteps = FOOTSTEP_FLESH;
			} else if ( !Q_stricmp( token, "mech" ) ) {
				ci->footsteps = FOOTSTEP_MECH;
			} else if ( !Q_stricmp( token, "energy" ) ) {
				ci->footsteps = FOOTSTEP_ENERGY;
			} else {
				CG_Printf( "Bad footsteps parm in %s: %s\n", filename, token );
			}
			continue;
		} else if ( !Q_stricmp( token, "headoffset" ) ) {
			for ( i = 0 ; i < 3 ; i++ ) {
				token = COM_Parse( &text_p );
				if ( !token[0] ) {
					break;
				}
				ci->headOffset[i] = atof( token );
			}
			continue;
		} else if ( !Q_stricmp( token, "sex" ) ) {
			token = COM_Parse( &text_p );
			if ( !token[0] ) {
				break;
			}
			if ( token[0] == 'f' || token[0] == 'F' ) {
				ci->gender = GENDER_FEMALE;
			} else if ( token[0] == 'n' || token[0] == 'N' ) {
				ci->gender = GENDER_NEUTER;
			} else {
				ci->gender = GENDER_MALE;
			}
			continue;
		} else if ( !Q_stricmp( token, "fixedlegs" ) ) {
			ci->fixedlegs = qtrue;
			continue;
		} else if ( !Q_stricmp( token, "fixedtorso" ) ) {
			ci->fixedtorso = qtrue;
			continue;
		}

		// if it is a number, start parsing animations
		if ( token[0] >= '0' && token[0] <= '9' ) {
			text_p = prev;	// unget the token
			break;
		}
		Com_Printf( "unknown token '%s' in %s\n", token, filename );
	}

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
		CG_Printf( "Error parsing animation file: %s\n", filename );
		return qfalse;
	}

	// crouch backward animation
	memcpy(&animations[LEGS_BACKCR], &animations[LEGS_WALKCR], sizeof(animation_t));
	animations[LEGS_BACKCR].reversed = qtrue;
	// walk backward animation
	memcpy(&animations[LEGS_BACKWALK], &animations[LEGS_WALK], sizeof(animation_t));
	animations[LEGS_BACKWALK].reversed = qtrue;
	// flag moving fast
	animations[FLAG_RUN].firstFrame = 0;
	animations[FLAG_RUN].numFrames = 16;
	animations[FLAG_RUN].loopFrames = 16;
	animations[FLAG_RUN].frameLerp = 1000 / 15;
	animations[FLAG_RUN].initialLerp = 1000 / 15;
	animations[FLAG_RUN].reversed = qfalse;
	// flag not moving or moving slowly
	animations[FLAG_STAND].firstFrame = 16;
	animations[FLAG_STAND].numFrames = 5;
	animations[FLAG_STAND].loopFrames = 0;
	animations[FLAG_STAND].frameLerp = 1000 / 20;
	animations[FLAG_STAND].initialLerp = 1000 / 20;
	animations[FLAG_STAND].reversed = qfalse;
	// flag speeding up
	animations[FLAG_STAND2RUN].firstFrame = 16;
	animations[FLAG_STAND2RUN].numFrames = 5;
	animations[FLAG_STAND2RUN].loopFrames = 1;
	animations[FLAG_STAND2RUN].frameLerp = 1000 / 15;
	animations[FLAG_STAND2RUN].initialLerp = 1000 / 15;
	animations[FLAG_STAND2RUN].reversed = qtrue;
	//
	// new anims changes
	//
//	animations[TORSO_GETFLAG].flipflop = qtrue;
//	animations[TORSO_GUARDBASE].flipflop = qtrue;
//	animations[TORSO_PATROL].flipflop = qtrue;
//	animations[TORSO_AFFIRMATIVE].flipflop = qtrue;
//	animations[TORSO_NEGATIVE].flipflop = qtrue;
	//
	return qtrue;
}
*/
// END


/*
==========================
CG_FileExists
==========================
*/
// Q3Rally Code Start
/*
static qboolean	CG_FileExists(const char *filename) {
	int len;

	len = trap_FS_FOpenFile( filename, NULL, FS_READ );
	if (len>0) {
		return qtrue;
	}
	return qfalse;
}
*/
// END


/*
==========================
CG_FindClientModelFile
==========================
*/
// Q3Rally Code Start
/*
static qboolean	CG_FindClientModelFile( char *filename, int length, clientInfo_t *ci, const char *teamName, const char *modelName, const char *skinName, const char *base, const char *ext ) {
	char *team, *charactersFolder;
	int i;

	if ( cgs.gametype >= GT_TEAM ) {
		switch ( ci->team ) {
			case TEAM_BLUE: {
				team = "blue";
				break;
			}
			default: {
				team = "red";
				break;
			}
		}
	}
	else {
		team = "default";
	}
	charactersFolder = "";
	while(1) {
		for ( i = 0; i < 2; i++ ) {
			if ( i == 0 && teamName && *teamName ) {
				//								"models/players/characters/james/stroggs/lower_lily_red.skin"
				Com_sprintf( filename, length, "models/players/%s%s/%s%s_%s_%s.%s", charactersFolder, modelName, teamName, base, skinName, team, ext );
			}
			else {
				//								"models/players/characters/james/lower_lily_red.skin"
				Com_sprintf( filename, length, "models/players/%s%s/%s_%s_%s.%s", charactersFolder, modelName, base, skinName, team, ext );
			}
			if ( CG_FileExists( filename ) ) {
				return qtrue;
			}
			if ( cgs.gametype >= GT_TEAM ) {
				if ( i == 0 && teamName && *teamName ) {
					//								"models/players/characters/james/stroggs/lower_red.skin"
					Com_sprintf( filename, length, "models/players/%s%s/%s%s_%s.%s", charactersFolder, modelName, teamName, base, team, ext );
				}
				else {
					//								"models/players/characters/james/lower_red.skin"
					Com_sprintf( filename, length, "models/players/%s%s/%s_%s.%s", charactersFolder, modelName, base, team, ext );
				}
			}
			else {
				if ( i == 0 && teamName && *teamName ) {
					//								"models/players/characters/james/stroggs/lower_lily.skin"
					Com_sprintf( filename, length, "models/players/%s%s/%s%s_%s.%s", charactersFolder, modelName, teamName, base, skinName, ext );
				}
				else {
					//								"models/players/characters/james/lower_lily.skin"
					Com_sprintf( filename, length, "models/players/%s%s/%s_%s.%s", charactersFolder, modelName, base, skinName, ext );
				}
			}
			if ( CG_FileExists( filename ) ) {
				return qtrue;
			}
			if ( !teamName || !*teamName ) {
				break;
			}
		}
		// if tried the heads folder first
		if ( charactersFolder[0] ) {
			break;
		}
		charactersFolder = "characters/";
	}

	return qfalse;
}
*/
// END


/*
==========================
CG_FindClientHeadFile
==========================
*/
// Q3Rally Code Start
/*
static qboolean	CG_FindClientHeadFile( char *filename, int length, clientInfo_t *ci, const char *teamName, const char *headModelName, const char *headSkinName, const char *base, const char *ext ) {
	char *team, *headsFolder;
	int i;

	if ( cgs.gametype >= GT_TEAM ) {
		switch ( ci->team ) {
			case TEAM_BLUE: {
				team = "blue";
				break;
			}
			default: {
				team = "red";
				break;
			}
		}
	}
	else {
		team = "default";
	}

	if ( headModelName[0] == '*' ) {
		headsFolder = "heads/";
		headModelName++;
	}
	else {
		headsFolder = "";
	}
	while(1) {
		for ( i = 0; i < 2; i++ ) {
			if ( i == 0 && teamName && *teamName ) {
				Com_sprintf( filename, length, "models/players/%s%s/%s/%s%s_%s.%s", headsFolder, headModelName, headSkinName, teamName, base, team, ext );
			}
			else {
				Com_sprintf( filename, length, "models/players/%s%s/%s/%s_%s.%s", headsFolder, headModelName, headSkinName, base, team, ext );
			}
			if ( CG_FileExists( filename ) ) {
				return qtrue;
			}
			if ( cgs.gametype >= GT_TEAM ) {
				if ( i == 0 &&  teamName && *teamName ) {
					Com_sprintf( filename, length, "models/players/%s%s/%s%s_%s.%s", headsFolder, headModelName, teamName, base, team, ext );
				}
				else {
					Com_sprintf( filename, length, "models/players/%s%s/%s_%s.%s", headsFolder, headModelName, base, team, ext );
				}
			}
			else {
				if ( i == 0 && teamName && *teamName ) {
					Com_sprintf( filename, length, "models/players/%s%s/%s%s_%s.%s", headsFolder, headModelName, teamName, base, headSkinName, ext );
				}
				else {
					Com_sprintf( filename, length, "models/players/%s%s/%s_%s.%s", headsFolder, headModelName, base, headSkinName, ext );
				}
			}
			if ( CG_FileExists( filename ) ) {
				return qtrue;
			}
			if ( !teamName || !*teamName ) {
				break;
			}
		}
		// if tried the heads folder first
		if ( headsFolder[0] ) {
			break;
		}
		headsFolder = "heads/";
	}

	return qfalse;
}
*/
// END


/*
==========================
CG_RegisterClientSkin
==========================
*/
// STONELANCE( q3 player replaced with car )
//static qboolean	CG_RegisterClientSkin( clientInfo_t *ci, const char *teamName, const char *modelName, const char *skinName, const char *headModelName, const char *headSkinName ) {
static qboolean	CG_RegisterClientSkin( clientInfo_t *ci, const char *modelName, const char *skinName, const char *rimName, const char *headName ) {
	char		filename[MAX_QPATH];

/*
	Com_sprintf( filename, sizeof( filename ), "models/players/%s/%slower_%s.skin", modelName, teamName, skinName );
	ci->legsSkin = trap_R_RegisterSkin( filename );
	if (!ci->legsSkin) {
		Com_Printf( "Leg skin load failure: %s\n", filename );
	}

	Com_sprintf( filename, sizeof( filename ), "models/players/%s/%supper_%s.skin", modelName, teamName, skinName );
	ci->torsoSkin = trap_R_RegisterSkin( filename );
	if (!ci->torsoSkin) {
		Com_Printf( "Torso skin load failure: %s\n", filename );
	}

	if( headModelName[0] == '*' ) {
		Com_sprintf( filename, sizeof( filename ), "models/players/heads/%s/head_%s.skin", &headModelName[1], headSkinName );
	} else {
		Com_sprintf( filename, sizeof( filename ), "models/players/%s/%shead_%s.skin", headModelName, teamName, headSkinName );
	}
	ci->headSkin = trap_R_RegisterSkin( filename );
	// if the head skin could not be found and we didn't load from the heads folder try to load from there
	if ( !ci->headSkin && headModelName[0] != '*' ) {
		Com_sprintf( filename, sizeof( filename ), "models/players/heads/%s/head_%s.skin", headModelName, headSkinName );
		ci->headSkin = trap_R_RegisterSkin( filename );
	}
	if (!ci->headSkin) {
		Com_Printf( "Head skin load failure: %s\n", filename );
	}
	if ( !ci->legsSkin || !ci->torsoSkin || !ci->headSkin ) {
		return qfalse;
	}
*/

	if( cgs.gametype >= GT_TEAM ){
		switch ( ci->team ) {
			case TEAM_BLUE:
				skinName = "blue";
				break;
			case TEAM_GREEN:
				skinName = "green";
				break;
			case TEAM_YELLOW:
				skinName = "yellow";
				break;
			default:
				skinName = "red";
				break;
		}
	}

	Com_sprintf( filename, sizeof(filename), "models/players/%s/%s.skin", modelName, skinName );
	ci->bodySkin = trap_R_RegisterSkin( filename );
	if( !ci->bodySkin ) {
		Com_Printf( S_COLOR_YELLOW "Q3R Warning: Failed to load car skin: %s\n", filename );

		// try loading default skin
		Com_sprintf( filename, sizeof(filename), "models/players/%s/%s.skin", modelName, DEFAULT_SKIN );
		ci->bodySkin = trap_R_RegisterSkin( filename );
		if( !ci->bodySkin ) {
			Com_Printf( S_COLOR_RED "Q3R Error: Failed to load default car skin: %s\n", filename );
			return qfalse;
		}
	}

	// load players icon
	Com_sprintf( filename, sizeof(filename), "models/players/%s/icon_%s.tga", modelName, skinName );
	ci->modelIcon = trap_R_RegisterShader( filename );

	Com_sprintf( filename, sizeof(filename), "models/players/wheels/%s.skin", rimName );
	ci->wheelSkin = trap_R_RegisterSkin( filename );
	if( !ci->wheelSkin ) {
		Com_Printf( S_COLOR_YELLOW "Q3R Warning: Failed to load wheel skin: %s\n", filename );

		// try loading default skin
		Com_sprintf( filename, sizeof(filename), "models/players/wheels/%s.skin", DEFAULT_RIM );
		ci->wheelSkin = trap_R_RegisterSkin( filename );
		if( !ci->wheelSkin ) {
			Com_Printf( S_COLOR_RED "Q3R Error: Failed to load default wheel skin: %s\n", filename );
			return qfalse;
		}
	}

	Com_sprintf( filename, sizeof(filename), "models/players/heads/%s.skin", headName );
	ci->headSkin = trap_R_RegisterSkin( filename );
	if( !ci->headSkin ) {
		Com_Printf( S_COLOR_YELLOW "Q3R Warning: Failed to load head skin: %s\n", filename );

		// try loading default skin
		Com_sprintf( filename, sizeof(filename), "models/players/heads/%s.skin", DEFAULT_HEAD );
		ci->headSkin = trap_R_RegisterSkin( filename );
		if( !ci->headSkin ) {
			Com_Printf( S_COLOR_RED "Q3R Error: Failed to load default head skin: %s\n", filename );
			return qfalse;
		}
	}

	if (ci->plateSkinName[0]) {
		Com_sprintf( filename, sizeof( filename ), "models/players/plates/player%d.tga", ci->clientNum );
	} else {
		Com_sprintf( filename, sizeof( filename ), "models/players/plates/default.tga" );
	}

	ci->plateShader = trap_R_RegisterShader(filename);
	if( !ci->plateShader ) {
		Com_Printf( S_COLOR_YELLOW "Q3R Warning: Failed to load plate shader: %s\n", filename );
/*
		// try loading default
		Com_sprintf( filename, sizeof(filename), "models/players/plates/%s.tga", DEFAULT_PLATE_SKIN );
		ci->plateShader = trap_R_RegisterShader( filename );
		if( !ci->plateShader ) {
			Com_Printf( S_COLOR_RED "Q3R Error: Failed to load default plate shader: %s\n", filename );
			return qfalse;
		}
*/
	}
// END

	return qtrue;
}

/*
==========================
CG_RegisterClientModelname
==========================
*/
// STONELANCE( q3 player replaced with car )
// static qboolean CG_RegisterClientModelname( clientInfo_t *ci, const char *modelName, const char *skinName, const char *headModelName, const char *headSkinName, const char *teamName ) {
static qboolean CG_RegisterClientModelname( clientInfo_t *ci, const char *modelName, const char *skinName, const char *rimName, const char *headName, const char *teamName ) {
	char	filename[MAX_QPATH];
/*
	const char		*headName;

	char	newTeamName[MAX_QPATH];

	if ( headModelName[0] == '\0' ) {
		headName = modelName;
	}
	else {
		headName = headModelName;
	}
	Com_sprintf( filename, sizeof( filename ), "models/players/%s/lower.md3", modelName );
	ci->legsModel = trap_R_RegisterModel( filename );
	if ( !ci->legsModel ) {
		Com_Printf( "Failed to load model file %s\n", filename );
		return qfalse;
	}

	Com_sprintf( filename, sizeof( filename ), "models/players/%s/upper.md3", modelName );
	ci->torsoModel = trap_R_RegisterModel( filename );
	if ( !ci->torsoModel ) {
		Com_Printf( "Failed to load model file %s\n", filename );
		return qfalse;
	}

	if( headName[0] == '*' ) {
		Com_sprintf( filename, sizeof( filename ), "models/players/heads/%s/%s.md3", &headModelName[1], &headModelName[1] );
	}
	else {
		Com_sprintf( filename, sizeof( filename ), "models/players/%s/head.md3", headName );
	}
	ci->headModel = trap_R_RegisterModel( filename );
	// if the head model could not be found and we didn't load from the heads folder try to load from there
	if ( !ci->headModel && headName[0] != '*' ) {
		Com_sprintf( filename, sizeof( filename ), "models/players/heads/%s/%s.md3", headModelName, headModelName );
		ci->headModel = trap_R_RegisterModel( filename );
	}
	if ( !ci->headModel ) {
		Com_Printf( "Failed to load model file %s\n", filename );
		return qfalse;
	}

	// if any skins failed to load, return failure
	if ( !CG_RegisterClientSkin( ci, teamName, modelName, skinName, headName, headSkinName ) ) {
		if( teamName && *teamName) {
			Com_Printf( "Failed to load skin file: %s : %s : %s, %s : %s\n", teamName, modelName, skinName, headName, headSkinName );
			if( ci->team == TEAM_BLUE ) {
				Com_sprintf(newTeamName, sizeof(newTeamName), "%s/", DEFAULT_BLUETEAM_NAME);
			}
			else {
				Com_sprintf(newTeamName, sizeof(newTeamName), "%s/", DEFAULT_REDTEAM_NAME);
			}
			if ( !CG_RegisterClientSkin( ci, newTeamName, modelName, skinName, headName, headSkinName ) ) {
				Com_Printf( "Failed to load skin file: %s : %s : %s, %s : %s\n", newTeamName, modelName, skinName, headName, headSkinName );
				return qfalse;
			}
		} else {
			Com_Printf( "Failed to load skin file: %s : %s, %s : %s\n", modelName, skinName, headName, headSkinName );
			return qfalse;
		}
	}

*/

	// load the data from the animation config
/*
	Com_sprintf( filename, sizeof( filename ), "models/players/%s/animation.cfg", modelName );
	if ( !CG_ParseAnimationFile( filename, ci ) ) {
		Com_Printf( S_COLOR_YELLOW "Q3R Warning: Failed to load animation file %s\n", filename );
//		return qfalse;
	}
*/

/*
	if( headName[0] == '*' ) {
		Com_sprintf( filename, sizeof( filename ), "models/players/heads/%s/icon_%s.tga", &headName[1], headSkinName );
	} else {
		Com_sprintf( filename, sizeof( filename ), "models/players/%s/icon_%s.tga", headName, headSkinName );
	}
	ci->modelIcon = trap_R_RegisterShaderNoMip( filename );
	// if the model icon could not be found and we didn't load from the heads folder try to load from there
	if ( !ci->modelIcon && headName[0] != '*') {
		Com_sprintf( filename, sizeof( filename ), "models/players/heads/%s/icon_%s.tga", headName, headSkinName );
		ci->modelIcon = trap_R_RegisterShaderNoMip( filename );
	}
	if ( !ci->modelIcon ) {
		Com_Printf( "Failed to load icon file: %s\n", filename );
		return qfalse;
	}
*/

	// load cmodels before models so filecache works
	Com_sprintf( filename, sizeof( filename ), "models/players/%s/body.md3", modelName );
	ci->bodyModel = trap_R_RegisterModel( filename );
	if ( !ci->bodyModel ) {
		Com_Printf( S_COLOR_YELLOW "Q3R Warning: Could not load car body model: %s\n", filename);
		return qfalse;

#if 0 // ZTM: Not used by Q3Rally
		// use default body model
		Com_sprintf( filename, sizeof(filename), "models/players/%s/body.md3", DEFAULT_MODEL );
		ci->bodyModel = trap_R_RegisterModel( filename );
		if( !ci->bodyModel ) {
			Com_Printf( S_COLOR_RED "Q3R Error: Failed to load default car body model: %s\n", filename );
			return qfalse;
		}
		modelName = "sidepipe";

		// STONELANCE - UPDATE team name always valid?
//		skinName = teamName;
//		trap_SendClientCommand("model \"sidepipe/default\"\n");
#endif
	}

	Com_sprintf( filename, sizeof( filename ), "models/players/%s/wheel.md3", modelName );
	ci->wheelModel = trap_R_RegisterModel( filename );
	if ( !ci->wheelModel ) {
		Com_Printf( S_COLOR_YELLOW "Q3R Warning: Could not load wheel model: %s\n", filename);

		// use default wheel model
		Com_sprintf( filename, sizeof(filename), "models/players/%s/wheel.md3", DEFAULT_MODEL );
		ci->wheelModel = trap_R_RegisterModel( filename );
		if( !ci->wheelModel ) {
			Com_Printf( S_COLOR_RED "Q3R Error: Failed to load default wheel model: %s\n", filename );
			return qfalse;
		}
	}

	Com_sprintf( filename, sizeof( filename ), "models/players/heads/%s.md3", headName );
	ci->headModel = trap_R_RegisterModel( filename );
	if ( !ci->headModel ) {
		Com_Printf( S_COLOR_YELLOW "Q3R Warning: Failed to load head model: %s\n", filename );

		// use default head model
		Com_sprintf( filename, sizeof(filename), "models/players/heads/%s.md3", DEFAULT_HEAD );
		ci->headModel = trap_R_RegisterModel( filename );
		if( !ci->headModel ) {
			Com_Printf( S_COLOR_RED "Q3R Error: Failed to load default head model: %s\n", filename );
			return qfalse;
		}
	}

	// figure out plate model
	Com_sprintf( filename, sizeof( filename ), "models/players/plates/player%d.tga", ci->clientNum );
	if ( !Q_stricmpn( ci->plateSkinName, "usa_", 4 ) ){
		Q_strncpyz( ci->plateName, "plate_usa", sizeof( ci->plateName ) );
		CreateLicensePlateImage(va("models/players/plates/%s.tga", ci->plateSkinName), filename, ci->name, 10);
	}
	else{
		Q_strncpyz( ci->plateName, "plate_eu", sizeof( ci->plateName ) );
		CreateLicensePlateImage(va("models/players/plates/%s.tga", ci->plateSkinName), filename, ci->name, 20);
	}

	Com_sprintf( filename, sizeof( filename ), "models/players/plates/%s.md3", ci->plateName );
	ci->plateModel = trap_R_RegisterModel( filename );
	if ( !ci->plateModel ) {
		Com_Printf( S_COLOR_YELLOW "Q3R Warning: Failed to load license plate model: %s\n", filename );
		return qfalse;

/*		// use plate_eu model
		Com_sprintf( filename, sizeof(filename), "models/players/plates/%s.md3", DEFAULT_PLATE );
		ci->plateModel = trap_R_RegisterModel( filename );
		if( !ci->plateModel ) {
			Com_Printf( S_COLOR_RED "Q3R Error: Failed to load default license plate model: %s\n", filename );
			return qfalse;
		}
*/
	}

	// load center suspension
	if (ci->bodyModel && (CG_TagExists(ci->bodyModel, "tag_suspcl") || CG_TagExists(ci->bodyModel, "tag_suspcr"))){
		Com_sprintf( filename, sizeof( filename ), "models/players/%s/suspc.md3", modelName );
		ci->suspCModel = trap_R_RegisterModel( filename );
		if ( !ci->suspCModel ) {
			Com_Printf( S_COLOR_YELLOW "Q3R Warning: Failed to load center suspension model: %s\n", filename );
//			return qfalse;
		}
	}

	// if any skins failed to load, return failure
	if ( !CG_RegisterClientSkin( ci, modelName, skinName, rimName, headName ) ) {
		Com_Printf( S_COLOR_RED "Q3R Error: Failed to load skin files: %s : %s : %s : %s\n", modelName, skinName, rimName, headName );
		return qfalse;
	}
// END

	return qtrue;
}


/*
====================
CG_ColorFromString
====================
*/
static void CG_ColorFromString( const char *v, vec3_t color ) {
	int val;

	VectorClear( color );

	val = atoi( v );

	if ( val < 1 || val > 7 ) {
		VectorSet( color, 1, 1, 1 );
		return;
	}

	if ( val & 1 ) {
		color[2] = 1.0f;
	}
	if ( val & 2 ) {
		color[1] = 1.0f;
	}
	if ( val & 4 ) {
		color[0] = 1.0f;
	}
}

/*
===================
CG_LoadClientInfo

Load it now, taking the disk hits.
This will usually be deferred to a safe time
===================
*/
static void CG_LoadClientInfo( int clientNum, clientInfo_t *ci ) {
	const char	*dir, *fallback;
	int			i, modelloaded;
	const char	*s;
	char		teamname[MAX_QPATH];

	teamname[0] = 0;
#ifdef MISSIONPACK
	if( cgs.gametype >= GT_TEAM) {
		if( ci->team == TEAM_BLUE ) {
			Q_strncpyz(teamname, cg_blueTeamName.string, sizeof(teamname) );
		}
// Q3Rally Code Start

		else if( ci->team == TEAM_GREEN ) {
			Q_strncpyz(teamname, cg_greenTeamName.string, sizeof(teamname) );
		}
		else if( ci->team == TEAM_YELLOW ) {
			Q_strncpyz(teamname, cg_yellowTeamName.string, sizeof(teamname) );
		}

// END
		else {
			Q_strncpyz(teamname, cg_redTeamName.string, sizeof(teamname) );
		}
	}
	if( teamname[0] ) {
		strcat( teamname, "/" );
	}
#endif
	modelloaded = qtrue;

// Q3Rally Code Start
//	if ( !CG_RegisterClientModelname( ci, ci->modelName, ci->skinName, ci->headModelName, ci->headSkinName, teamname ) ) {
	if ( !CG_RegisterClientModelname( ci, ci->modelName, ci->skinName, ci->rimName, ci->headModelName, teamname ) ) {
// END
		if ( cg_buildScript.integer ) {
// Q3Rally Code Start
//			CG_Error( "CG_RegisterClientModelname( %s, %s, %s, %s %s ) failed", ci->modelName, ci->skinName, ci->headModelName, ci->headSkinName, teamname );
			CG_Error( "CG_RegisterClientModelname( %s, %s, %s, %s %s ) failed", ci->modelName, ci->skinName, ci->headModelName, ci->rimName, teamname );
// END
		}

		// fall back
		if( cgs.gametype >= GT_TEAM) {
			// keep skin name
			if( ci->team == TEAM_BLUE ) {
				Q_strncpyz(teamname, DEFAULT_BLUETEAM_NAME, sizeof(teamname) );
			}
            else if( ci->team == TEAM_GREEN ) {
                Q_strncpyz(teamname, DEFAULT_GREENTEAM_NAME, sizeof(teamname) );
            }
            else if( ci->team == TEAM_YELLOW ) {
                Q_strncpyz(teamname, DEFAULT_YELLOWTEAM_NAME, sizeof(teamname) );
            }
            else {
				Q_strncpyz(teamname, DEFAULT_REDTEAM_NAME, sizeof(teamname) );
			}
// Q3Rally Code Start
//			if ( !CG_RegisterClientModelname( ci, DEFAULT_TEAM_MODEL, ci->skinName, DEFAULT_TEAM_HEAD, ci->skinName, teamname ) ) {
			if ( !CG_RegisterClientModelname( ci, DEFAULT_TEAM_MODEL, ci->skinName, ci->rimName, DEFAULT_HEAD, teamname ) ) {
				CG_Error( "DEFAULT_TEAM_MODEL / skin (%s/%s) failed to register", DEFAULT_TEAM_MODEL, ci->skinName );
// END
			}
		} else {
// Q3Rally Code Start
//			if ( !CG_RegisterClientModelname( ci, DEFAULT_MODEL, "default", DEFAULT_MODEL, "default", teamname ) ) {
			if ( !CG_RegisterClientModelname( ci, DEFAULT_MODEL, DEFAULT_SKIN, DEFAULT_RIM, DEFAULT_HEAD, teamname ) ) {
				CG_Error( "DEFAULT_MODEL (%s) failed to register", DEFAULT_MODEL );
// END
			}
		}
		modelloaded = qfalse;
	}

	ci->newAnims = qfalse;
// STONELANCE UPDATE: change this to body maybe?
/*
	if ( ci->torsoModel ) {
		orientation_t tag;
		// if the torso model has the "tag_flag"
		if ( trap_R_LerpTag( &tag, ci->torsoModel, 0, 0, 1, "tag_flag" ) ) {
			ci->newAnims = qtrue;
		}
	}
*/
// END

	// sounds
	dir = ci->modelName;
// Q3Rally Code Start
//	fallback = (cgs.gametype >= GT_TEAM) ? DEFAULT_TEAM_MODEL : DEFAULT_MODEL;

	// UPDATE - remove after player car sounds are added
	fallback = "sarge";
// END

	for ( i = 0 ; i < MAX_CUSTOM_SOUNDS ; i++ ) {
		s = cg_customSoundNames[i];
		if ( !s ) {
			break;
		}
		ci->sounds[i] = 0;
		// if the model didn't load use the sounds of the default model
		if (modelloaded) {
			ci->sounds[i] = trap_S_RegisterSound( va("sound/player/%s/%s", dir, s + 1), qfalse );
		}
		if ( !ci->sounds[i] ) {
			ci->sounds[i] = trap_S_RegisterSound( va("sound/player/%s/%s", DEFAULT_MODEL, s + 1), qfalse );
		}
		if ( !ci->sounds[i] ) {
			ci->sounds[i] = trap_S_RegisterSound( va("sound/player/%s/%s", fallback, s + 1), qfalse );
		}
	}

	ci->deferred = qfalse;

	// reset any existing players and bodies, because they might be in bad
	// frames for this new model
	for ( i = 0 ; i < MAX_GENTITIES ; i++ ) {
		if ( cg_entities[i].currentState.clientNum == clientNum
			&& cg_entities[i].currentState.eType == ET_PLAYER ) {
			CG_ResetPlayerEntity( &cg_entities[i] );
		}
	}
}

/*
======================
CG_CopyClientInfoModel
======================
*/
static void CG_CopyClientInfoModel( clientInfo_t *from, clientInfo_t *to ) {
	VectorCopy( from->headOffset, to->headOffset );
	to->footsteps = from->footsteps;
	to->gender = from->gender;

// SKWID( replaced player with car )
	to->bodyModel = from->bodyModel;
	to->bodySkin = from->bodySkin;
	to->wheelModel = from->wheelModel;
	to->wheelSkin = from->wheelSkin;
/*
	to->legsModel = from->legsModel;
	to->legsSkin = from->legsSkin;
	to->torsoModel = from->torsoModel;
	to->torsoSkin = from->torsoSkin;
*/
// END
	to->headModel = from->headModel;
// Q3Rally Code Start
//	to->headSkin = from->headSkin;

	to->plateModel = from->plateModel;
	to->plateShader = from->plateShader;

	to->clientNum = from->clientNum;
// END
	to->modelIcon = from->modelIcon;

	to->newAnims = from->newAnims;

// SKWID( no more need for animations )
//	memcpy( to->animations, from->animations, sizeof( to->animations ) );
// END
	memcpy( to->sounds, from->sounds, sizeof( to->sounds ) );
}

/*
======================
CG_ScanForExistingClientInfo
======================
*/
static qboolean CG_ScanForExistingClientInfo( clientInfo_t *ci ) {
	int		i;
	clientInfo_t	*match;

	for ( i = 0 ; i < cgs.maxclients ; i++ ) {
		match = &cgs.clientinfo[ i ];
		if ( !match->infoValid ) {
			continue;
		}
		if ( match->deferred ) {
			continue;
		}
		if ( !Q_stricmp( ci->modelName, match->modelName )
			&& !Q_stricmp( ci->skinName, match->skinName )
			&& !Q_stricmp( ci->headModelName, match->headModelName )
// Q3Rally Code Start
			&& !Q_stricmp( ci->rimName, match->rimName )
			&& !Q_stricmp( ci->plateName, match->plateName )
			&& !Q_stricmp( ci->plateSkinName, match->plateSkinName )
//			&& !Q_stricmp( ci->headSkinName, match->headSkinName ) 
// END
			&& !Q_stricmp( ci->blueTeam, match->blueTeam ) 
			&& !Q_stricmp( ci->redTeam, match->redTeam )
			&& (cgs.gametype < GT_TEAM || ci->team == match->team) ) {
			// this clientinfo is identical, so use its handles

			ci->deferred = qfalse;

			CG_CopyClientInfoModel( match, ci );

			return qtrue;
		}
	}

	// nothing matches, so defer the load
	return qfalse;
}

/*
======================
CG_SetDeferredClientInfo

We aren't going to load it now, so grab some other
client's info to use until we have some spare time.
======================
*/
static void CG_SetDeferredClientInfo( int clientNum, clientInfo_t *ci ) {
	int		i;
	clientInfo_t	*match;

	// if someone else is already the same models and skins we
	// can just load the client info
	for ( i = 0 ; i < cgs.maxclients ; i++ ) {
		match = &cgs.clientinfo[ i ];
		if ( !match->infoValid || match->deferred ) {
			continue;
		}
		if ( Q_stricmp( ci->skinName, match->skinName ) ||
			 Q_stricmp( ci->modelName, match->modelName ) ||
//			 Q_stricmp( ci->headModelName, match->headModelName ) ||
//			 Q_stricmp( ci->headSkinName, match->headSkinName ) ||
// Q3Rally Code Start
			 Q_stricmp( ci->rimName, match->rimName ) ||
			 Q_stricmp( ci->plateSkinName, match->plateSkinName ) ||
// END
			 (cgs.gametype >= GT_TEAM && ci->team != match->team) ) {
			continue;
		}

		// just load the real info cause it uses the same models and skins
		CG_LoadClientInfo( clientNum, ci );
		return;
	}

	// if we are in teamplay, only grab a model if the skin is correct
	if ( cgs.gametype >= GT_TEAM ) {
		for ( i = 0 ; i < cgs.maxclients ; i++ ) {
			match = &cgs.clientinfo[ i ];
			if ( !match->infoValid || match->deferred ) {
				continue;
			}
			if ( Q_stricmp( ci->skinName, match->skinName ) ||
				(cgs.gametype >= GT_TEAM && ci->team != match->team) ) {
				continue;
			}
			ci->deferred = qtrue;
			CG_CopyClientInfoModel( match, ci );
			return;
		}
		// load the full model, because we don't ever want to show
		// an improper team skin.  This will cause a hitch for the first
		// player, when the second enters.  Combat shouldn't be going on
		// yet, so it shouldn't matter
		CG_LoadClientInfo( clientNum, ci );
		return;
	}

	// find the first valid clientinfo and grab its stuff
	for ( i = 0 ; i < cgs.maxclients ; i++ ) {
		match = &cgs.clientinfo[ i ];
		if ( !match->infoValid ) {
			continue;
		}

		ci->deferred = qtrue;
		CG_CopyClientInfoModel( match, ci );
		return;
	}

	// we should never get here...
	CG_Printf( "CG_SetDeferredClientInfo: no valid clients!\n" );

	CG_LoadClientInfo( clientNum, ci );
}


/*
======================
CG_NewClientInfo
======================
*/
void CG_NewClientInfo( int clientNum ) {
	clientInfo_t *ci;
	clientInfo_t newInfo;
	const char	*configstring;
	const char	*v;
	char		*slash;

	ci = &cgs.clientinfo[clientNum];

	configstring = CG_ConfigString( clientNum + CS_PLAYERS );
	if ( !configstring[0] ) {
		memset( ci, 0, sizeof( *ci ) );
		return;		// player just left
	}

	// build into a temp buffer so the defer checks can use
	// the old value
	memset( &newInfo, 0, sizeof( newInfo ) );
// Q3Rally Code Start
	newInfo.clientNum = clientNum;
// END

	// isolate the player's name
	v = Info_ValueForKey(configstring, "n");
	Q_strncpyz( newInfo.name, v, sizeof( newInfo.name ) );

	// colors
	v = Info_ValueForKey( configstring, "c1" );
	CG_ColorFromString( v, newInfo.color1 );

	newInfo.c1RGBA[0] = 255 * newInfo.color1[0];
	newInfo.c1RGBA[1] = 255 * newInfo.color1[1];
	newInfo.c1RGBA[2] = 255 * newInfo.color1[2];
	newInfo.c1RGBA[3] = 255;

	v = Info_ValueForKey( configstring, "c2" );
	CG_ColorFromString( v, newInfo.color2 );

	newInfo.c2RGBA[0] = 255 * newInfo.color2[0];
	newInfo.c2RGBA[1] = 255 * newInfo.color2[1];
	newInfo.c2RGBA[2] = 255 * newInfo.color2[2];
	newInfo.c2RGBA[3] = 255;

	// bot skill
	v = Info_ValueForKey( configstring, "skill" );
	newInfo.botSkill = atoi( v );

	// handicap
	v = Info_ValueForKey( configstring, "hc" );
	newInfo.handicap = atoi( v );

	// wins
	v = Info_ValueForKey( configstring, "w" );
	newInfo.wins = atoi( v );

	// losses
	v = Info_ValueForKey( configstring, "l" );
	newInfo.losses = atoi( v );

	// team
	v = Info_ValueForKey( configstring, "t" );
	newInfo.team = atoi( v );

	// team task
	v = Info_ValueForKey( configstring, "tt" );
	newInfo.teamTask = atoi(v);

	// team leader
	v = Info_ValueForKey( configstring, "tl" );
	newInfo.teamLeader = atoi(v);

	v = Info_ValueForKey( configstring, "g_redteam" );
	Q_strncpyz(newInfo.redTeam, v, MAX_TEAMNAME);

	v = Info_ValueForKey( configstring, "g_blueteam" );
	Q_strncpyz(newInfo.blueTeam, v, MAX_TEAMNAME);

	// model
	v = Info_ValueForKey( configstring, "model" );
	if ( cg_forceModel.integer ) {
		// forcemodel makes everyone use a single model
		// to prevent load hitches
		char modelStr[MAX_QPATH];
		char *skin;

		if( cgs.gametype >= GT_TEAM ) {
			Q_strncpyz( newInfo.modelName, DEFAULT_TEAM_MODEL, sizeof( newInfo.modelName ) );
// Q3Rally Code Start
//			Q_strncpyz( newInfo.skinName, "default", sizeof( newInfo.skinName ) );
			Q_strncpyz( newInfo.skinName, DEFAULT_SKIN, sizeof( newInfo.skinName ) );
// END
		} else {
			trap_Cvar_VariableStringBuffer( "model", modelStr, sizeof( modelStr ) );
			if ( ( skin = strchr( modelStr, '/' ) ) == NULL) {
// Q3Rally Code Start
//				skin = "default";
				skin = DEFAULT_SKIN;
// END
			} else {
				*skin++ = 0;
			}

			Q_strncpyz( newInfo.skinName, skin, sizeof( newInfo.skinName ) );
			Q_strncpyz( newInfo.modelName, modelStr, sizeof( newInfo.modelName ) );
		}

		if ( cgs.gametype >= GT_TEAM ) {
			// keep skin name
			slash = strchr( v, '/' );
			if ( slash ) {
				Q_strncpyz( newInfo.skinName, slash + 1, sizeof( newInfo.skinName ) );
			}
		}

// Q3Rally Code Start
		trap_Cvar_VariableStringBuffer( "rim", ci->rimName, sizeof(ci->rimName));
		trap_Cvar_VariableStringBuffer( "plate", ci->plateSkinName, sizeof(ci->plateSkinName));
// END
	} else {
		Q_strncpyz( newInfo.modelName, v, sizeof( newInfo.modelName ) );

		slash = strchr( newInfo.modelName, '/' );
		if ( !slash ) {
			// modelName didn not include a skin name
// Q3Rally Code Start
//			Q_strncpyz( newInfo.skinName, "default", sizeof( newInfo.skinName ) );
			Q_strncpyz( newInfo.skinName, DEFAULT_SKIN, sizeof( newInfo.skinName ) );
// END
		} else {
			Q_strncpyz( newInfo.skinName, slash + 1, sizeof( newInfo.skinName ) );
			// truncate modelName
			*slash = 0;
		}

// Q3Rally Code Start
		v = Info_ValueForKey( configstring, "rim" );
		Q_strncpyz( newInfo.rimName, v, sizeof( newInfo.rimName ) );

		v = Info_ValueForKey( configstring, "plate" );
		Q_strncpyz( newInfo.plateSkinName, v, sizeof( newInfo.plateSkinName ) );
// END
	}

// Q3Rally Code Start
	v = Info_ValueForKey( configstring, "cm" );
	newInfo.controlMode = atoi(v);

	v = Info_ValueForKey( configstring, "ms" );
	newInfo.manualShift = atoi(v);
// END

	// head model
	v = Info_ValueForKey( configstring, "hmodel" );
	if ( cg_forceModel.integer ) {
		// forcemodel makes everyone use a single model
		// to prevent load hitches
		char modelStr[MAX_QPATH];

// Q3Rally Code Start (removed)
/*
		char *skin;

		if( cgs.gametype >= GT_TEAM ) {
			Q_strncpyz( newInfo.headModelName, DEFAULT_TEAM_HEAD, sizeof( newInfo.headModelName ) );
			Q_strncpyz( newInfo.headSkinName, "default", sizeof( newInfo.headSkinName ) );
		} else {
			trap_Cvar_VariableStringBuffer( "headmodel", modelStr, sizeof( modelStr ) );
*/
			trap_Cvar_VariableStringBuffer( "head", modelStr, sizeof( modelStr ) );
/*
			if ( ( skin = strchr( modelStr, '/' ) ) == NULL) {
				skin = "default";
			} else {
				*skin++ = 0;
			}
			Q_strncpyz( newInfo.headSkinName, skin, sizeof( newInfo.headSkinName ) );
*/
			Q_strncpyz( newInfo.headModelName, modelStr, sizeof( newInfo.headModelName ) );
//		}
// END

// STONELANCE (removed)
/*
		if ( cgs.gametype >= GT_TEAM ) {
			// keep skin name
			slash = strchr( v, '/' );
			if ( slash ) {
				Q_strncpyz( newInfo.headSkinName, slash + 1, sizeof( newInfo.headSkinName ) );
			}
		}
*/
// END
	} else {
		Q_strncpyz( newInfo.headModelName, v, sizeof( newInfo.headModelName ) );

		slash = strchr( newInfo.headModelName, '/' );
// STONELANCE (removed)
/*
		if ( !slash ) {
			// modelName didn not include a skin name
			Q_strncpyz( newInfo.headSkinName, "default", sizeof( newInfo.headSkinName ) );
		} else {
			Q_strncpyz( newInfo.headSkinName, slash + 1, sizeof( newInfo.headSkinName ) );
			// truncate modelName
			*slash = 0;
		}
*/
		// hack off any extra crap if there is a /
		if ( slash ) {
			// truncate modelName
			*slash = 0;
		}
// END
	}

	// scan for an existing clientinfo that matches this modelname
	// so we can avoid loading checks if possible
	if ( !CG_ScanForExistingClientInfo( &newInfo ) ) {
		qboolean	forceDefer;

		forceDefer = trap_MemoryRemaining() < 4000000;

		// if we are defering loads, just have it pick the first valid
		if ( forceDefer || (cg_deferPlayers.integer && !cg_buildScript.integer && !cg.loading ) ) {
			// keep whatever they had if it won't violate team skins
			CG_SetDeferredClientInfo( clientNum, &newInfo );
			// if we are low on memory, leave them with this model
			if ( forceDefer ) {
				CG_Printf( "Memory is low. Using deferred model.\n" );
				newInfo.deferred = qfalse;
			}
		} else {
			CG_LoadClientInfo( clientNum, &newInfo );
		}
	}

	// replace whatever was there with the new one
	newInfo.infoValid = qtrue;
	*ci = newInfo;
}



/*
======================
CG_LoadDeferredPlayers

Called each frame when a player is dead
and the scoreboard is up
so deferred players can be loaded
======================
*/
void CG_LoadDeferredPlayers( void ) {
	int		i;
	clientInfo_t	*ci;

	// scan for a deferred player to load
	for ( i = 0, ci = cgs.clientinfo ; i < cgs.maxclients ; i++, ci++ ) {
		if ( ci->infoValid && ci->deferred ) {
			// if we are low on memory, leave it deferred
			if ( trap_MemoryRemaining() < 4000000 ) {
				CG_Printf( "Memory is low. Using deferred model.\n" );
				ci->deferred = qfalse;
				continue;
			}
			CG_LoadClientInfo( i, ci );
//			break;
		}
	}
}

/*
=============================================================================

PLAYER ANIMATION

=============================================================================
*/


/*
===============
CG_SetLerpFrameAnimation

may include ANIM_TOGGLEBIT
===============
*/
// SKWID( removed function )
/*
static void CG_SetLerpFrameAnimation( clientInfo_t *ci, lerpFrame_t *lf, int newAnimation ) {
	animation_t	*anim;

	lf->animationNumber = newAnimation;
	newAnimation &= ~ANIM_TOGGLEBIT;

	if ( newAnimation < 0 || newAnimation >= MAX_TOTALANIMATIONS ) {
		CG_Error( "Bad animation number: %i", newAnimation );
	}

	anim = &ci->animations[ newAnimation ];

	lf->animation = anim;
	lf->animationTime = lf->frameTime + anim->initialLerp;

	if ( cg_debugAnim.integer ) {
		CG_Printf( "Anim: %i\n", newAnimation );
	}
}
*/
// END

/*
===============
CG_RunLerpFrame

Sets cg.snap, cg.oldFrame, and cg.backlerp
cg.time should be between oldFrameTime and frameTime after exit
===============
*/
// SKWID( removed this function )
/*
static void CG_RunLerpFrame( clientInfo_t *ci, lerpFrame_t *lf, int newAnimation, float speedScale ) {
	int			f, numFrames;
	animation_t	*anim;

	// debugging tool to get no animations
	if ( cg_animSpeed.integer == 0 ) {
		lf->oldFrame = lf->frame = lf->backlerp = 0;
		return;
	}

	// see if the animation sequence is switching
	if ( newAnimation != lf->animationNumber || !lf->animation ) {
		CG_SetLerpFrameAnimation( ci, lf, newAnimation );
	}

	// if we have passed the current frame, move it to
	// oldFrame and calculate a new frame
	if ( cg.time >= lf->frameTime ) {
		lf->oldFrame = lf->frame;
		lf->oldFrameTime = lf->frameTime;

		// get the next frame based on the animation
		anim = lf->animation;
		if ( !anim->frameLerp ) {
			return;		// shouldn't happen
		}
		if ( cg.time < lf->animationTime ) {
			lf->frameTime = lf->animationTime;		// initial lerp
		} else {
			lf->frameTime = lf->oldFrameTime + anim->frameLerp;
		}
		f = ( lf->frameTime - lf->animationTime ) / anim->frameLerp;
		f *= speedScale;		// adjust for haste, etc

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
				lf->frameTime = cg.time;
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
		if ( cg.time > lf->frameTime ) {
			lf->frameTime = cg.time;
			if ( cg_debugAnim.integer ) {
				CG_Printf( "Clamp lf->frameTime\n");
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
*/
// END

/*
===============
CG_ClearLerpFrame
===============
*/
// SKWID( removed function )
/*
static void CG_ClearLerpFrame( clientInfo_t *ci, lerpFrame_t *lf, int animationNumber ) {
	lf->frameTime = lf->oldFrameTime = cg.time;
	CG_SetLerpFrameAnimation( ci, lf, animationNumber );
	lf->oldFrame = lf->frame = lf->animation->firstFrame;
}
*/
// END


/*
===============
CG_PlayerAnimation
===============
*/
// SKWID( removed function )
/*
static void CG_PlayerAnimation( centity_t *cent, int *legsOld, int *legs, float *legsBackLerp,
						int *torsoOld, int *torso, float *torsoBackLerp ) {
	clientInfo_t	*ci;
	int				clientNum;
	float			speedScale;

	clientNum = cent->currentState.clientNum;

	if ( cg_noPlayerAnims.integer ) {
		*legsOld = *legs = *torsoOld = *torso = 0;
		return;
	}

	if ( cent->currentState.powerups & ( 1 << PW_HASTE ) ) {
		speedScale = 1.5;
	} else {
		speedScale = 1;
	}

	ci = &cgs.clientinfo[ clientNum ];

	// do the shuffle turn frames locally
	if ( cent->pe.legs.yawing && ( cent->currentState.legsAnim & ~ANIM_TOGGLEBIT ) == LEGS_IDLE ) {
		CG_RunLerpFrame( ci, &cent->pe.legs, LEGS_TURN, speedScale );
	} else {
		CG_RunLerpFrame( ci, &cent->pe.legs, cent->currentState.legsAnim, speedScale );
	}

	*legsOld = cent->pe.legs.oldFrame;
	*legs = cent->pe.legs.frame;
	*legsBackLerp = cent->pe.legs.backlerp;

	CG_RunLerpFrame( ci, &cent->pe.torso, cent->currentState.torsoAnim, speedScale );

	*torsoOld = cent->pe.torso.oldFrame;
	*torso = cent->pe.torso.frame;
	*torsoBackLerp = cent->pe.torso.backlerp;
}
*/
// END

/*
=============================================================================

PLAYER ANGLES

=============================================================================
*/

#if 0 // ZTM: Not used by Q3Rally
/*
==================
CG_SwingAngles
==================
*/
static void CG_SwingAngles( float destination, float swingTolerance, float clampTolerance,
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
		move = cg.frametime * scale * speed;
		if ( move >= swing ) {
			move = swing;
			*swinging = qfalse;
		}
		*angle = AngleMod( *angle + move );
	} else if ( swing < 0 ) {
		move = cg.frametime * scale * -speed;
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

/*
=================
CG_AddPainTwitch
=================
*/
static void CG_AddPainTwitch( centity_t *cent, vec3_t torsoAngles ) {
	int		t;
	float	f;

	t = cg.time - cent->pe.painTime;
	if ( t >= PAIN_TWITCH_TIME ) {
		return;
	}

	f = 1.0 - (float)t / PAIN_TWITCH_TIME;

	if ( cent->pe.painDirection ) {
		torsoAngles[ROLL] += 20 * f;
	} else {
		torsoAngles[ROLL] -= 20 * f;
	}
}
#endif


/*
===============
CG_PlayerAngles

Handles separate torso motion

  legs pivot based on direction of movement

  head always looks exactly at cent->lerpAngles

  if motion < 20 degrees, show in head only
  if < 45 degrees, also show in torso
===============
*/
// SKWID( removed function )
/*
static void CG_PlayerAngles( centity_t *cent, vec3_t legs[3], vec3_t torso[3], vec3_t head[3] ) {
	vec3_t		legsAngles, torsoAngles, headAngles;
	float		dest;
	static	int	movementOffsets[8] = { 0, 22, 45, -22, 0, 22, -45, -22 };
	vec3_t		velocity;
	float		speed;
	int			dir, clientNum;
	clientInfo_t	*ci;

	VectorCopy( cent->lerpAngles, headAngles );
	headAngles[YAW] = AngleMod( headAngles[YAW] );
	VectorClear( legsAngles );
	VectorClear( torsoAngles );

	// --------- yaw -------------

	// allow yaw to drift a bit
	if ( ( cent->currentState.legsAnim & ~ANIM_TOGGLEBIT ) != LEGS_IDLE 
		|| ((cent->currentState.torsoAnim & ~ANIM_TOGGLEBIT) != TORSO_STAND 
		&& (cent->currentState.torsoAnim & ~ANIM_TOGGLEBIT) != TORSO_STAND2)) {
		// if not standing still, always point all in the same direction
		cent->pe.torso.yawing = qtrue;	// always center
		cent->pe.torso.pitching = qtrue;	// always center
		cent->pe.legs.yawing = qtrue;	// always center
	}

	// adjust legs for movement dir
	if ( cent->currentState.eFlags & EF_DEAD ) {
		// don't let dead bodies twitch
		dir = 0;
	} else {
		dir = cent->currentState.angles2[YAW];
		if ( dir < 0 || dir > 7 ) {
			CG_Error( "Bad player movement angle" );
		}
	}
	legsAngles[YAW] = headAngles[YAW] + movementOffsets[ dir ];
	torsoAngles[YAW] = headAngles[YAW] + 0.25 * movementOffsets[ dir ];

	// torso
	CG_SwingAngles( torsoAngles[YAW], 25, 90, cg_swingSpeed.value, &cent->pe.torso.yawAngle, &cent->pe.torso.yawing );
	CG_SwingAngles( legsAngles[YAW], 40, 90, cg_swingSpeed.value, &cent->pe.legs.yawAngle, &cent->pe.legs.yawing );

	torsoAngles[YAW] = cent->pe.torso.yawAngle;
	legsAngles[YAW] = cent->pe.legs.yawAngle;


	// --------- pitch -------------

	// only show a fraction of the pitch angle in the torso
	if ( headAngles[PITCH] > 180 ) {
		dest = (-360 + headAngles[PITCH]) * 0.75f;
	} else {
		dest = headAngles[PITCH] * 0.75f;
	}
	CG_SwingAngles( dest, 15, 30, 0.1f, &cent->pe.torso.pitchAngle, &cent->pe.torso.pitching );
	torsoAngles[PITCH] = cent->pe.torso.pitchAngle;

	//
	clientNum = cent->currentState.clientNum;
	if ( clientNum >= 0 && clientNum < MAX_CLIENTS ) {
		ci = &cgs.clientinfo[ clientNum ];
		if ( ci->fixedtorso ) {
			torsoAngles[PITCH] = 0.0f;
		}
	}

	// --------- roll -------------


	// lean towards the direction of travel
	VectorCopy( cent->currentState.pos.trDelta, velocity );
	speed = VectorNormalize( velocity );
	if ( speed ) {
		vec3_t	axis[3];
		float	side;

		speed *= 0.05f;

		AnglesToAxis( legsAngles, axis );
		side = speed * DotProduct( velocity, axis[1] );
		legsAngles[ROLL] -= side;

		side = speed * DotProduct( velocity, axis[0] );
		legsAngles[PITCH] += side;
	}

	//
	clientNum = cent->currentState.clientNum;
	if ( clientNum >= 0 && clientNum < MAX_CLIENTS ) {
		ci = &cgs.clientinfo[ clientNum ];
		if ( ci->fixedlegs ) {
			legsAngles[YAW] = torsoAngles[YAW];
			legsAngles[PITCH] = 0.0f;
			legsAngles[ROLL] = 0.0f;
		}
	}

	// pain twitch
	CG_AddPainTwitch( cent, torsoAngles );

	// pull the angles back out of the hierarchial chain
	AnglesSubtract( headAngles, torsoAngles, headAngles );
	AnglesSubtract( torsoAngles, legsAngles, torsoAngles );
	AnglesToAxis( legsAngles, legs );
	AnglesToAxis( torsoAngles, torso );
	AnglesToAxis( headAngles, head );
}
*/
// END


//==========================================================================

/*
===============
CG_HasteTrail
===============
*/
// STONELANCE - removed function
/*
static void CG_HasteTrail( centity_t *cent ) {
	localEntity_t	*smoke;
	vec3_t			origin;
	int				anim;

	if ( cent->trailTime > cg.time ) {
		return;
	}

// SKWID( removed animations )
//	anim = cent->pe.legs.animationNumber & ~ANIM_TOGGLEBIT;
//	if ( anim != LEGS_RUN && anim != LEGS_BACK ) {
//		return;
//	}
// END

	cent->trailTime += 100;
	if ( cent->trailTime < cg.time ) {
		cent->trailTime = cg.time;
	}

	VectorCopy( cent->lerpOrigin, origin );
	origin[2] -= 16;

	smoke = CG_SmokePuff( origin, vec3_origin, 
				  8, 
				  1, 1, 1, 1,
				  500, 
				  cg.time,
				  0,
				  0,
				  cgs.media.hastePuffShader );

	// use the optimized local entity add
	smoke->leType = LE_SCALE_FADE;
}
*/
// END

#ifdef MISSIONPACK
/*
===============
CG_BreathPuffs
===============
*/
static void CG_BreathPuffs( centity_t *cent, refEntity_t *head) {
	clientInfo_t *ci;
	vec3_t up, origin;
	int contents;

	ci = &cgs.clientinfo[ cent->currentState.number ];

	if (!cg_enableBreath.integer) {
		return;
	}
	if ( cent->currentState.number == cg.snap->ps.clientNum && !cg.renderingThirdPerson) {
		return;
	}
	if ( cent->currentState.eFlags & EF_DEAD ) {
		return;
	}
	contents = CG_PointContents( head->origin, 0 );
	if ( contents & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ) {
		return;
	}
	if ( ci->breathPuffTime > cg.time ) {
		return;
	}

	VectorSet( up, 0, 0, 8 );
	VectorMA(head->origin, 8, head->axis[0], origin);
	VectorMA(origin, -4, head->axis[2], origin);
	CG_SmokePuff( origin, up, 16, 1, 1, 1, 0.66f, 1500, cg.time, cg.time + 400, LEF_PUFF_DONT_SCALE, cgs.media.shotgunSmokePuffShader );
	ci->breathPuffTime = cg.time + 2000;
}

/*
===============
CG_DustTrail
===============
*/
static void CG_DustTrail( centity_t *cent ) {
	int				anim;
	vec3_t end, vel;
	trace_t tr;

	if (!cg_enableDust.integer)
		return;

	if ( cent->dustTrailTime > cg.time ) {
		return;
	}

	cent->dustTrailTime += 40;
	if ( cent->dustTrailTime < cg.time ) {
		cent->dustTrailTime = cg.time;
	}

	VectorCopy(cent->currentState.pos.trBase, end);
	end[2] -= 64;
	CG_Trace( &tr, cent->currentState.pos.trBase, NULL, NULL, end, cent->currentState.number, MASK_PLAYERSOLID );

	if ( !(tr.surfaceFlags & SURF_DUST) )
		return;

	VectorCopy( cent->currentState.pos.trBase, end );
	end[2] -= 16;

	VectorSet(vel, 0, 0, -30);
	CG_SmokePuff( end, vel,
				  24,
				  .8f, .8f, 0.7f, 0.33f,
				  500,
				  cg.time,
				  0,
				  0,
				  cgs.media.dustPuffShader );
}

// #endif

/*
===============
CG_SnowTrail
===============
*/
static void CG_SnowTrail( centity_t *cent ) {
//	int				anim;
	vec3_t end, vel;
	trace_t tr;

	if (!cg_enableSnow.integer)
		return;

	if ( cent->snowTrailTime > cg.time ) {
		return;
	}

	cent->snowTrailTime += 40;
	if ( cent->snowTrailTime < cg.time ) {
		cent->snowTrailTime = cg.time;
	}

	VectorCopy(cent->currentState.pos.trBase, end);
	end[2] -= 64;
	CG_Trace( &tr, cent->currentState.pos.trBase, NULL, NULL, end, cent->currentState.number, MASK_PLAYERSOLID );

	if ( !(tr.surfaceFlags & SURF_SNOW) )
		return;

	VectorCopy( cent->currentState.pos.trBase, end );
	end[2] -= 16;

	VectorSet(vel, 0, 0, -30);
	CG_SmokePuff( end, vel,
				  24,
				  .8f, .8f, 0.7f, 0.33f,
				  500,
				  cg.time,
				  0,
				  0,
				  cgs.media.snowPuffShader );
}



/*
===============
CG_SandTrail
===============
*/
static void CG_SandTrail( centity_t *cent ) {
//	int				anim;
	vec3_t end, vel;
	trace_t tr;

	if (!cg_enableSand.integer)
		return;

	if ( cent->sandTrailTime > cg.time ) {
		return;
	}

	cent->sandTrailTime += 40;
	if ( cent->sandTrailTime < cg.time ) {
		cent->sandTrailTime = cg.time;
	}

	VectorCopy(cent->currentState.pos.trBase, end);
	end[2] -= 64;
	CG_Trace( &tr, cent->currentState.pos.trBase, NULL, NULL, end, cent->currentState.number, MASK_PLAYERSOLID );

	if ( !(tr.surfaceFlags & SURF_SAND) )
		return;

	VectorCopy( cent->currentState.pos.trBase, end );
	end[2] -= 16;

	VectorSet(vel, 0, 0, -30);
	CG_SmokePuff( end, vel,
				  24,
				  .8f, .8f, 0.7f, 0.33f,
				  500,
				  cg.time,
				  0,
				  0,
// FIX THIS !!!   cgs.media.sandPuffShader );
				  cgs.media.snowPuffShader );
}

 #endif

/*
===============
CG_TrailItem
===============
*/
static void CG_TrailItem( centity_t *cent, qhandle_t hModel ) {
	refEntity_t		ent;
	vec3_t			angles;
	vec3_t			axis[3];

	VectorCopy( cent->lerpAngles, angles );
	angles[PITCH] = 0;
	angles[ROLL] = 0;
	AnglesToAxis( angles, axis );

	memset( &ent, 0, sizeof( ent ) );
	VectorMA( cent->lerpOrigin, -16, axis[0], ent.origin );
	ent.origin[2] += 16;
// Q3Rally Code Start
//	angles[YAW] += 90;
// END
	AnglesToAxis( angles, ent.axis );

	ent.hModel = hModel;
	trap_R_AddRefEntityToScene( &ent );
}


/*
===============
CG_PlayerFlag
===============
*/
static void CG_PlayerFlag( centity_t *cent, qhandle_t hSkin, refEntity_t *torso ) {
#if 0 // Not supported by Q3Rally
	clientInfo_t	*ci;
	refEntity_t	pole;
	refEntity_t	flag;
	vec3_t		angles, dir;
	int			legsAnim, flagAnim, updateangles;
	float		angle, d;

	// show the flag pole model
	memset( &pole, 0, sizeof(pole) );
	pole.hModel = cgs.media.flagPoleModel;
	VectorCopy( torso->lightingOrigin, pole.lightingOrigin );
	pole.shadowPlane = torso->shadowPlane;
	pole.renderfx = torso->renderfx;
	CG_PositionEntityOnTag( &pole, torso, torso->hModel, "tag_flag" );
	trap_R_AddRefEntityToScene( &pole );

	// show the flag model
	memset( &flag, 0, sizeof(flag) );
	flag.hModel = cgs.media.flagFlapModel;
	flag.customSkin = hSkin;
	VectorCopy( torso->lightingOrigin, flag.lightingOrigin );
	flag.shadowPlane = torso->shadowPlane;
	flag.renderfx = torso->renderfx;

	VectorClear(angles);

	updateangles = qfalse;
	legsAnim = cent->currentState.legsAnim & ~ANIM_TOGGLEBIT;
	if( legsAnim == LEGS_IDLE || legsAnim == LEGS_IDLECR ) {
		flagAnim = FLAG_STAND;
	} else if ( legsAnim == LEGS_WALK || legsAnim == LEGS_WALKCR ) {
		flagAnim = FLAG_STAND;
		updateangles = qtrue;
	} else {
		flagAnim = FLAG_RUN;
		updateangles = qtrue;
	}

	if ( updateangles ) {

		VectorCopy( cent->currentState.pos.trDelta, dir );
		// add gravity
		dir[2] += 100;
		VectorNormalize( dir );
		d = DotProduct(pole.axis[2], dir);
		// if there is enough movement orthogonal to the flag pole
		if (fabs(d) < 0.9) {
			//
			d = DotProduct(pole.axis[0], dir);
			if (d > 1.0f) {
				d = 1.0f;
			}
			else if (d < -1.0f) {
				d = -1.0f;
			}
			angle = Q_acos(d);

			d = DotProduct(pole.axis[1], dir);
			if (d < 0) {
				angles[YAW] = 360 - angle * 180 / M_PI;
			}
			else {
				angles[YAW] = angle * 180 / M_PI;
			}
			if (angles[YAW] < 0)
				angles[YAW] += 360;
			if (angles[YAW] > 360)
				angles[YAW] -= 360;

			//vectoangles( cent->currentState.pos.trDelta, tmpangles );
			//angles[YAW] = tmpangles[YAW] + 45 - cent->pe.torso.yawAngle;
			// change the yaw angle
			CG_SwingAngles( angles[YAW], 25, 90, 0.15f, &cent->pe.flag.yawAngle, &cent->pe.flag.yawing );
		}

		/*
		d = DotProduct(pole.axis[2], dir);
		angle = Q_acos(d);

		d = DotProduct(pole.axis[1], dir);
		if (d < 0) {
			angle = 360 - angle * 180 / M_PI;
		}
		else {
			angle = angle * 180 / M_PI;
		}
		if (angle > 340 && angle < 20) {
			flagAnim = FLAG_RUNUP;
		}
		if (angle > 160 && angle < 200) {
			flagAnim = FLAG_RUNDOWN;
		}
		*/
	}

	// set the yaw angle
	angles[YAW] = cent->pe.flag.yawAngle;
	// lerp the flag animation frames
	ci = &cgs.clientinfo[ cent->currentState.clientNum ];
// STONELANCE (removed function)
//	CG_RunLerpFrame( ci, &cent->pe.flag, flagAnim, 1 );
// END
	flag.oldframe = cent->pe.flag.oldFrame;
	flag.frame = cent->pe.flag.frame;
	flag.backlerp = cent->pe.flag.backlerp;

	AnglesToAxis( angles, flag.axis );
	CG_PositionRotatedEntityOnTag( &flag, &pole, pole.hModel, "tag_flag" );

	trap_R_AddRefEntityToScene( &flag );
#endif
}


#ifdef MISSIONPACK
/*
===============
CG_PlayerTokens
===============
*/
static void CG_PlayerTokens( centity_t *cent, int renderfx ) {
	int			tokens, i, j;
	float		angle;
	refEntity_t	ent;
	vec3_t		dir, origin;
	skulltrail_t *trail;
	if ( cent->currentState.number >= MAX_CLIENTS ) {
		return;
	}
	trail = &cg.skulltrails[cent->currentState.number];
	tokens = cent->currentState.generic1;
	if ( !tokens ) {
		trail->numpositions = 0;
		return;
	}

	if ( tokens > MAX_SKULLTRAIL ) {
		tokens = MAX_SKULLTRAIL;
	}

	// add skulls if there are more than last time
	for (i = 0; i < tokens - trail->numpositions; i++) {
		for (j = trail->numpositions; j > 0; j--) {
			VectorCopy(trail->positions[j-1], trail->positions[j]);
		}
		VectorCopy(cent->lerpOrigin, trail->positions[0]);
	}
	trail->numpositions = tokens;

	// move all the skulls along the trail
	VectorCopy(cent->lerpOrigin, origin);
	for (i = 0; i < trail->numpositions; i++) {
		VectorSubtract(trail->positions[i], origin, dir);
		if (VectorNormalize(dir) > 30) {
			VectorMA(origin, 30, dir, trail->positions[i]);
		}
		VectorCopy(trail->positions[i], origin);
	}

	memset( &ent, 0, sizeof( ent ) );
	if( cgs.clientinfo[ cent->currentState.clientNum ].team == TEAM_BLUE ) {
		ent.hModel = cgs.media.redCubeModel;
	} else {
		ent.hModel = cgs.media.blueCubeModel;
	}
	ent.renderfx = renderfx;

	VectorCopy(cent->lerpOrigin, origin);
	for (i = 0; i < trail->numpositions; i++) {
		VectorSubtract(origin, trail->positions[i], ent.axis[0]);
		ent.axis[0][2] = 0;
		VectorNormalize(ent.axis[0]);
		VectorSet(ent.axis[2], 0, 0, 1);
		CrossProduct(ent.axis[0], ent.axis[2], ent.axis[1]);

		VectorCopy(trail->positions[i], ent.origin);
		angle = (((cg.time + 500 * MAX_SKULLTRAIL - 500 * i) / 16) & 255) * (M_PI * 2) / 255;
		ent.origin[2] += sin(angle) * 10;
		trap_R_AddRefEntityToScene( &ent );
		VectorCopy(trail->positions[i], origin);
	}
}
#endif


/*
===============
CG_PlayerPowerups
===============
*/
static void CG_PlayerPowerups( centity_t *cent, refEntity_t *torso ) {
	int		powerups;
	clientInfo_t	*ci;

	powerups = cent->currentState.powerups;
	if ( !powerups ) {
		return;
	}

	// quad gives a dlight
	if ( powerups & ( 1 << PW_QUAD ) ) {
//		trap_R_AddLightToScene( cent->lerpOrigin, 200 + (rand()&31), 0.2f, 0.2f, 1.0 );
        trap_R_AddLightToScene( cent->lerpOrigin, 200 + (rand()&31), 0.63f, 0.13f, 0.94f );
	}

	// flight plays a looped sound
// STOENLANCE
/*
	if ( powerups & ( 1 << PW_FLIGHT ) ) {
		trap_S_AddLoopingSound( cent->currentState.number, cent->lerpOrigin, vec3_origin, cgs.media.flightSound );
	}
*/
// END

	ci = &cgs.clientinfo[ cent->currentState.clientNum ];
	// redflag
	if ( powerups & ( 1 << PW_REDFLAG ) ) {
		if (ci->newAnims) {
			CG_PlayerFlag( cent, cgs.media.redFlagFlapSkin, torso );
		}
		else {
			CG_TrailItem( cent, cgs.media.redFlagModel );
		}
		trap_R_AddLightToScene( cent->lerpOrigin, 200 + (rand()&31), 1.0, 0.2f, 0.2f );
	}

	// blueflag
	if ( powerups & ( 1 << PW_BLUEFLAG ) ) {
		if (ci->newAnims){
			CG_PlayerFlag( cent, cgs.media.blueFlagFlapSkin, torso );
		}
		else {
			CG_TrailItem( cent, cgs.media.blueFlagModel );
		}
		trap_R_AddLightToScene( cent->lerpOrigin, 200 + (rand()&31), 0.2f, 0.2f, 1.0 );
	}

/* finish in 0.5

    // greenflag
	if ( powerups & ( 1 << PW_GREENFLAG ) ) {
		if (ci->newAnims){
			CG_PlayerFlag( cent, cgs.media.greenFlagFlapSkin, torso );
		}
		else {
			CG_TrailItem( cent, cgs.media.greenFlagModel );
		}
		trap_R_AddLightToScene( cent->lerpOrigin, 200 + (rand()&31), 0.2f, 1.0, 0.2f );
	}

    // yellowflag
	if ( powerups & ( 1 << PW_YELLOWFLAG ) ) {
		if (ci->newAnims){
			CG_PlayerFlag( cent, cgs.media.yellowFlagFlapSkin, torso );
		}
		else {
			CG_TrailItem( cent, cgs.media.yellowFlagModel );
		}
		trap_R_AddLightToScene( cent->lerpOrigin, 200 + (rand()&31), 1.0, 1.0, 0.2f );
	}

// end finish in 0.5
*/

	// neutralflag
	if ( powerups & ( 1 << PW_NEUTRALFLAG ) ) {
		if (ci->newAnims) {
			CG_PlayerFlag( cent, cgs.media.neutralFlagFlapSkin, torso );
		}
		else {
			CG_TrailItem( cent, cgs.media.neutralFlagModel );
		}
		trap_R_AddLightToScene( cent->lerpOrigin, 200 + (rand()&31), 1.0, 1.0, 1.0 );
	}

	// haste leaves smoke trails
// STONELANCE - no smoke from haste
/*
	if ( powerups & ( 1 << PW_HASTE ) ) {
		CG_HasteTrail( cent );
	}
*/
// END
}


/*
===============
CG_PlayerFloatSprite

Float a sprite over the player's head
===============
*/
static void CG_PlayerFloatSprite( centity_t *cent, qhandle_t shader ) {
	int				rf;
	refEntity_t		ent;

	if ( cent->currentState.number == cg.snap->ps.clientNum && !cg.renderingThirdPerson ) {
		rf = RF_THIRD_PERSON;		// only show in mirrors
	} else {
		rf = 0;
	}

	memset( &ent, 0, sizeof( ent ) );
	VectorCopy( cent->lerpOrigin, ent.origin );
	ent.origin[2] += 48;
	ent.reType = RT_SPRITE;
	ent.customShader = shader;
	ent.radius = 10;
	ent.renderfx = rf;
	ent.shaderRGBA[0] = 255;
	ent.shaderRGBA[1] = 255;
	ent.shaderRGBA[2] = 255;
	ent.shaderRGBA[3] = 255;
	trap_R_AddRefEntityToScene( &ent );
}


// Q3Rally Code Start
/*
===============
CG_PlayerFloatSpriteField

Float a sprite over the player's head
===============
*/
static void CG_PlayerFloatSpriteField( centity_t *cent, int value ) {
	char	num[16], *ptr;
	int		l;
	int		frame;
	int				rf;
	refEntity_t		ent;
	int		width, radius;
	vec3_t	right;

	radius = 10;
	width = 2;

	AngleVectors(cent->currentState.apos.trBase, NULL, right, NULL);

	switch ( width ) {
	case 1:
		value = value > 9 ? 9 : value;
		value = value < 0 ? 0 : value;
		break;
	case 2:
		value = value > 99 ? 99 : value;
		value = value < -9 ? -9 : value;
		break;
	case 3:
		value = value > 999 ? 999 : value;
		value = value < -99 ? -99 : value;
		break;
	case 4:
		value = value > 9999 ? 9999 : value;
		value = value < -999 ? -999 : value;
		break;
	}

	if ( cent->currentState.number == cg.snap->ps.clientNum && !cg.renderingThirdPerson ) {
		rf = RF_THIRD_PERSON;		// only show in mirrors
	} else {
		rf = 0;
	}

	memset( &ent, 0, sizeof( ent ) );
	VectorCopy( cent->lerpOrigin, ent.origin );
	ent.origin[2] += 48;
	ent.reType = RT_SPRITE;
	ent.radius = radius;
	ent.renderfx = rf;
	ent.shaderRGBA[0] = 0;
	ent.shaderRGBA[1] = 255;
	ent.shaderRGBA[2] = 0;
	ent.shaderRGBA[3] = 127;
	ent.shaderTime = cg.time / 1000.0f;

	Com_sprintf (num, sizeof(num), "%i", value);
	l = strlen(num);
	if (l > width)
		l = width;

	if (l == width){
		VectorMA(ent.origin, -radius / 2, right, ent.origin);
	}

	ptr = num;
	while (*ptr && l)
	{
		if (*ptr == '-')
			frame = STAT_MINUS;
		else
			frame = *ptr -'0';

		ent.customShader = cgs.media.numberShaders[frame];
		trap_R_AddRefEntityToScene( &ent );

		VectorMA(ent.origin, radius, right, ent.origin);
		ptr++;
		l--;
	}
}
// END

/*
===============
CG_PlayerSprites

Float sprites over the player's head
===============
*/
static void CG_PlayerSprites( centity_t *cent ) {
	int		team;

	if ( cent->currentState.eFlags & EF_CONNECTION ) {
		CG_PlayerFloatSprite( cent, cgs.media.connectionShader );
		return;
	}

	if ( cent->currentState.eFlags & EF_TALK ) {
		CG_PlayerFloatSprite( cent, cgs.media.balloonShader );
		return;
	}

	if ( cent->currentState.eFlags & EF_AWARD_IMPRESSIVE ) {
		CG_PlayerFloatSprite( cent, cgs.media.medalImpressive );
		return;
	}
    
    if ( cent->currentState.eFlags & EF_AWARD_IMPRESSIVETELEFRAG ) {
		CG_PlayerFloatSprite( cent, cgs.media.medalImpressiveTelefrag );
		return;
	}

	if ( cent->currentState.eFlags & EF_AWARD_EXCELLENT ) {
		CG_PlayerFloatSprite( cent, cgs.media.medalExcellent );
		return;
	}

	if ( cent->currentState.eFlags & EF_AWARD_GAUNTLET ) {
		CG_PlayerFloatSprite( cent, cgs.media.medalGauntlet );
		return;
	}

	if ( cent->currentState.eFlags & EF_AWARD_DEFEND ) {
		CG_PlayerFloatSprite( cent, cgs.media.medalDefend );
		return;
	}

	if ( cent->currentState.eFlags & EF_AWARD_ASSIST ) {
		CG_PlayerFloatSprite( cent, cgs.media.medalAssist );
		return;
	}

	if ( cent->currentState.eFlags & EF_AWARD_CAP ) {
		CG_PlayerFloatSprite( cent, cgs.media.medalCapture );
		return;
	}

// Q3Rally Code Start
	// UPDATE - enable this
	if ( cg_drawPositionSprites.integer && isRallyRace() &&
//	if ( isRallyRace() &&
		cg_entities[cent->currentState.number].currentPosition >= 1 &&
		cg_entities[cent->currentState.number].positionChangeTime + 5000 > cg.time) {
		CG_PlayerFloatSpriteField( cent, cg_entities[cent->currentState.number].currentPosition );
		return;
	}
// END

	team = cgs.clientinfo[ cent->currentState.clientNum ].team;
	if ( !(cent->currentState.eFlags & EF_DEAD) && 
		cg.snap->ps.persistant[PERS_TEAM] == team &&
// STONELANCE - dont draw friend sprite over yourself
		cent->currentState.clientNum != cg.snap->ps.clientNum &&
// END
		cgs.gametype >= GT_TEAM) {
		if (cg_drawFriend.integer) {
			CG_PlayerFloatSprite( cent, cgs.media.friendShader );
		}
		return;
	}
}

/*
===============
CG_PlayerShadow

Returns the Z component of the surface being shadowed

  should it return a full plane instead of a Z?
===============
*/
#define	SHADOW_DISTANCE		128
static qboolean CG_PlayerShadow( centity_t *cent, float *shadowPlane ) {
	vec3_t		end, mins = {-15, -15, 0}, maxs = {15, 15, 2};
// Q3Rally Code Start
	vec3_t		forward;
// END
	trace_t		trace;
	float		alpha;

	*shadowPlane = 0;

	if ( cg_shadows.integer == 0 ) {
		return qfalse;
	}

	// no shadows when invisible
	if ( cent->currentState.powerups & ( 1 << PW_INVIS ) ) {
		return qfalse;
	}

	// send a trace down from the player to the ground
	VectorCopy( cent->lerpOrigin, end );
	end[2] -= SHADOW_DISTANCE;

	trap_CM_BoxTrace( &trace, cent->lerpOrigin, end, mins, maxs, 0, MASK_PLAYERSOLID );

	// no shadow if too high
	if ( trace.fraction == 1.0 || trace.startsolid || trace.allsolid ) {
		return qfalse;
	}

	*shadowPlane = trace.endpos[2] + 1;

	if ( cg_shadows.integer != 1 ) {	// no mark for stencil or projection shadows
		return qtrue;
	}

	// fade the shadow out with height
	alpha = 1.0 - trace.fraction;

	// hack / FPE - bogus planes?
	//assert( DotProduct( trace.plane.normal, trace.plane.normal ) != 0.0f ) 

	// add the mark as a temporary, so it goes directly to the renderer
	// without taking a spot in the cg_marks array
// Q3Rally Code Start
	AngleVectors( cent->lerpAngles, forward, NULL, NULL );
	CG_ImpactMark2( cgs.media.shadowMarkShader, trace.endpos, trace.plane.normal, 
		forward, alpha, alpha, alpha, 1, qfalse, 64, qtrue );
// END
// SKWID( replaced player with car )
//	CG_ImpactMark( cgs.media.shadowMarkShader, trace.endpos, trace.plane.normal, 
//		cent->pe.legs.yawAngle, alpha,alpha,alpha,1, qfalse, 24, qtrue );
// END

	return qtrue;
}


/*
===============
CG_PlayerSplash

Draw a mark at the water surface
===============
*/
static void CG_PlayerSplash( centity_t *cent ) {
	vec3_t		start, end;
	trace_t		trace;
	int			contents;
	polyVert_t	verts[4];

	if ( !cg_shadows.integer ) {
		return;
	}

	VectorCopy( cent->lerpOrigin, end );
	end[2] -= 24;

	// if the feet aren't in liquid, don't make a mark
	// this won't handle moving water brushes, but they wouldn't draw right anyway...
	contents = CG_PointContents( end, 0 );
	if ( !( contents & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ) ) {
		return;
	}

	VectorCopy( cent->lerpOrigin, start );
	start[2] += 32;

	// if the head isn't out of liquid, don't make a mark
	contents = CG_PointContents( start, 0 );
	if ( contents & ( CONTENTS_SOLID | CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ) {
		return;
	}

	// trace down to find the surface
	trap_CM_BoxTrace( &trace, start, end, NULL, NULL, 0, ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) );

	if ( trace.fraction == 1.0 ) {
		return;
	}

	// create a mark polygon
	VectorCopy( trace.endpos, verts[0].xyz );
	verts[0].xyz[0] -= 32;
	verts[0].xyz[1] -= 32;
	verts[0].st[0] = 0;
	verts[0].st[1] = 0;
	verts[0].modulate[0] = 255;
	verts[0].modulate[1] = 255;
	verts[0].modulate[2] = 255;
	verts[0].modulate[3] = 255;

	VectorCopy( trace.endpos, verts[1].xyz );
	verts[1].xyz[0] -= 32;
	verts[1].xyz[1] += 32;
	verts[1].st[0] = 0;
	verts[1].st[1] = 1;
	verts[1].modulate[0] = 255;
	verts[1].modulate[1] = 255;
	verts[1].modulate[2] = 255;
	verts[1].modulate[3] = 255;

	VectorCopy( trace.endpos, verts[2].xyz );
	verts[2].xyz[0] += 32;
	verts[2].xyz[1] += 32;
	verts[2].st[0] = 1;
	verts[2].st[1] = 1;
	verts[2].modulate[0] = 255;
	verts[2].modulate[1] = 255;
	verts[2].modulate[2] = 255;
	verts[2].modulate[3] = 255;

	VectorCopy( trace.endpos, verts[3].xyz );
	verts[3].xyz[0] += 32;
	verts[3].xyz[1] -= 32;
	verts[3].st[0] = 1;
	verts[3].st[1] = 0;
	verts[3].modulate[0] = 255;
	verts[3].modulate[1] = 255;
	verts[3].modulate[2] = 255;
	verts[3].modulate[3] = 255;

	trap_R_AddPolyToScene( cgs.media.wakeMarkShader, 4, verts );
}



/*
===============
CG_AddRefEntityWithPowerups

Adds a piece with modifications or duplications for powerups
Also called by CG_Missile for quad rockets, but nobody can tell...
===============
*/
void CG_AddRefEntityWithPowerups( refEntity_t *ent, entityState_t *state, int team ) {

	if ( state->powerups & ( 1 << PW_INVIS ) ) {
		ent->customShader = cgs.media.invisShader;
		trap_R_AddRefEntityToScene( ent );
	} else {
		/*
		if ( state->eFlags & EF_KAMIKAZE ) {
			if (team == TEAM_BLUE)
				ent->customShader = cgs.media.blueKamikazeShader;
			else
				ent->customShader = cgs.media.redKamikazeShader;
			trap_R_AddRefEntityToScene( ent );
		}
		else {*/
			trap_R_AddRefEntityToScene( ent );
		//}

		if ( state->powerups & ( 1 << PW_QUAD ) )
		{
			if (team == TEAM_RED)
				ent->customShader = cgs.media.redQuadShader;
			else
				ent->customShader = cgs.media.quadShader;
			trap_R_AddRefEntityToScene( ent );
		}
		if ( state->powerups & ( 1 << PW_REGEN ) ) {
			if ( ( ( cg.time / 100 ) % 10 ) == 1 ) {
				ent->customShader = cgs.media.regenShader;
				trap_R_AddRefEntityToScene( ent );
			}
		}
// Q3Rally Code Start
		if ( state->powerups & ( 1 << PW_HASTE ) ) {
			ent->customShader = cgs.media.hasteShader;
			trap_R_AddRefEntityToScene( ent );
		}

		if ( state->powerups & ( 1 << PW_SHIELD ) ) {
			ent->customShader = cgs.media.shieldShader;
			trap_R_AddRefEntityToScene( ent );
		}
// END
		if ( state->powerups & ( 1 << PW_BATTLESUIT ) ) {
			ent->customShader = cgs.media.battleSuitShader;
			trap_R_AddRefEntityToScene( ent );
		}
	}
}


// STONELANCE( new functions )
static float surfaceColors[4][4] = { 
		{ 0.0f, 0.2f, 0.0f, 0.3f },		// grass
		{ 0.2f, 0.13f, 0.036f, 0.3f },		// dirt
		{ 0.95f, 0.95f, 0.95f, 0.3f },		// ashpalt
		{ 0.4f, 0.05f, 0.0f, 0.3f }		// fleshy
};

/*
===============
CG_AddSplash

Ripped from CG_PlayerSplash.
Draws a splash effect for the tire if it is in the water and
returns true.  Returns false if the tire is not in the water.

===============
*/
static qboolean CG_AddSplash( centity_t *cent, vec3_t origin ) {
	vec3_t		start, end;
	trace_t		trace;
	int			contents;
	polyVert_t	verts[4];

	VectorCopy( origin, end );
	end[2] -= 10;

	// if the bottom isn't in liquid, don't make a mark
	contents = trap_CM_PointContents( end, 0 );
	if ( !( contents & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ) ) {
		return qfalse;
	}

	VectorCopy( origin, start );
	start[2] += 10;

	// if the top isn't out of liquid, don't make a mark
	contents = trap_CM_PointContents( start, 0 );
	if ( contents & ( CONTENTS_SOLID | CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ) {
//		Com_Printf("Entire tire in water\n");
		return qtrue;
	}

	// trace down to find the surface
	trap_CM_BoxTrace( &trace, start, end, NULL, NULL, 0, ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) );

	if ( trace.fraction == 1.0 ) {
		return qfalse;
	}

//	Com_Printf("Bottom of tire in water\n");

	// create a mark polygon
	VectorCopy( trace.endpos, verts[0].xyz );
	verts[0].xyz[0] -= 32;
	verts[0].xyz[1] -= 32;
	verts[0].st[0] = 0;
	verts[0].st[1] = 0;
	verts[0].modulate[0] = 255;
	verts[0].modulate[1] = 255;
	verts[0].modulate[2] = 255;
	verts[0].modulate[3] = 255;

	VectorCopy( trace.endpos, verts[1].xyz );
	verts[1].xyz[0] -= 32;
	verts[1].xyz[1] += 32;
	verts[1].st[0] = 0;
	verts[1].st[1] = 1;
	verts[1].modulate[0] = 255;
	verts[1].modulate[1] = 255;
	verts[1].modulate[2] = 255;
	verts[1].modulate[3] = 255;

	VectorCopy( trace.endpos, verts[2].xyz );
	verts[2].xyz[0] += 32;
	verts[2].xyz[1] += 32;
	verts[2].st[0] = 1;
	verts[2].st[1] = 1;
	verts[2].modulate[0] = 255;
	verts[2].modulate[1] = 255;
	verts[2].modulate[2] = 255;
	verts[2].modulate[3] = 255;

	VectorCopy( trace.endpos, verts[3].xyz );
	verts[3].xyz[0] += 32;
	verts[3].xyz[1] -= 32;
	verts[3].st[0] = 1;
	verts[3].st[1] = 0;
	verts[3].modulate[0] = 255;
	verts[3].modulate[1] = 255;
	verts[3].modulate[2] = 255;
	verts[3].modulate[3] = 255;

	trap_R_AddPolyToScene( cgs.media.wakeMarkShader, 4, verts );

	return qtrue;
}

/*
===============
CG_SurfaceEffects

Draw skidmarks, dust, smoke, and tire splashes
===============
*/
static void CG_SurfaceEffects( centity_t *cent, vec3_t curOrigin, vec3_t up, int tireNum ){
	vec3_t			delta, origin, end;
	int				colorIndex;
	float			length;
	qhandle_t		shader;
	trace_t			tr;

	VectorMA(curOrigin, -(WHEEL_RADIUS * 2.0f), up, end);
	CG_Trace(&tr, curOrigin, NULL, NULL, end, cent->currentState.number, CONTENTS_SOLID);

	if (tr.surfaceFlags & SURF_SKY)
		return;

	// teleportFlag - teleported so dont use old skid origin
	if ( !CG_AddSplash(cent, curOrigin) && tr.fraction != 1.0 && !cent->teleportFlag ){

		// FIXME: add snow and leaves
		if (tr.surfaceFlags & SURF_SLICK){
			shader = -1;
			colorIndex = 2;
		}
		else if (tr.surfaceFlags & SURF_GRASS){
			shader = cgs.media.SMGrassShader;
			colorIndex = 0;
		}
		else if (tr.surfaceFlags & SURF_DUST){
			shader = cgs.media.SMDirtShader;
			colorIndex = 1;
		}
        else if (tr.surfaceFlags & SURF_SNOW){
			shader = cgs.media.SMDirtShader;
			colorIndex = 1;
		}
		else if (tr.surfaceFlags & SURF_DIRT) {
			shader = cgs.media.SMDirtShader;
			colorIndex = 1;
		}
		else if (tr.surfaceFlags & SURF_FLESH){
			shader = cgs.media.SMFleshShader;
			colorIndex = 3;
		}
		else if (tr.surfaceFlags & SURF_GRAVEL){ // like dirt but without so much dust
			shader = cgs.media.SMDirtShader;
			colorIndex = 1;
		}
		else {
			shader = cgs.media.SMAsphaltShader;
			colorIndex = 2;
		}

		VectorSubtract(cent->lastSkidOrigin[tireNum], curOrigin, delta);
		if ( cent->wheelSkidding[tireNum] && shader >= 0 ){
			length = VectorLength(delta) / 2;

			// create smoke even if we arent moving because the car is being stopped from moving
			if (cent->smokeTime[tireNum] < cg.time){
				if (tr.surfaceFlags & SURF_DUST)
					CreateSmokeCloudEntity(tr.endpos, up, 20, 48, 2000, surfaceColors[colorIndex][0], surfaceColors[colorIndex][1], surfaceColors[colorIndex][2], surfaceColors[colorIndex][3], cgs.media.smokePuffShader);
				else
					CreateSmokeCloudEntity(tr.endpos, up, 20, 12, 1000, surfaceColors[colorIndex][0], surfaceColors[colorIndex][1], surfaceColors[colorIndex][2], surfaceColors[colorIndex][3], cgs.media.smokePuffShader);

				cent->smokeTime[tireNum] = cg.time + 100;
			}

			if (length < cg_minSkidLength.integer){
				return;
			}

			trap_S_AddRealLoopingSound( cent->currentState.clientNum, origin, cent->currentState.pos.trDelta, trap_S_RegisterSound( "sound/rally/car/skid.wav", qfalse ) );

			if( cent->skidSoundTime + 500 < cg.time )
			{
				trap_S_StartSound( origin, cent->currentState.clientNum, CHAN_VOICE, trap_S_RegisterSound( "sound/rally/car/skid.wav", qfalse ) );
				cent->skidSoundTime = cg.time;
			}

			VectorMA(curOrigin, 1/2.0F, delta, origin);
			VectorNormalize(delta);

			CG_SkidMark( shader, origin, tr.plane.normal, delta, 1,1,1,1, qfalse, 8.0F, length, qfalse );
		}
		else if ( tr.surfaceFlags & SURF_DUST ){
			if ( VectorLength(delta) > 5 && cent->smokeTime[tireNum] < cg.time ){
				CreateSmokeCloudEntity( tr.endpos, up, 20, 36, 1500, surfaceColors[colorIndex][0] * 1.3f, surfaceColors[colorIndex][1] * 1.3f, surfaceColors[colorIndex][2] * 1.3f, 0.8f, cgs.media.dustPuffShader);
				cent->smokeTime[tireNum] = cg.time + 100;
			}
		}
	}

	VectorCopy(curOrigin, cent->lastSkidOrigin[tireNum]);
}

void CG_PlayerEngineSmoke ( centity_t *cent, refEntity_t *body ){
	vec4_t	color;
	vec3_t	origin;
	vec3_t	dir = {0, 0, 1};
	float	radius = 32;

	if (cent->engineSmokeTime + 100 > cg.time)
		return;

	if( cent->currentState.eFlags & EF_SMOKE_DARK || cent->currentState.eFlags & EF_DEAD ){
		radius = 64;

		color[0] = color[1] = color[2] = 0.5f;
		color[3] = 1.0f;
	}
	else if( cent->currentState.eFlags & EF_SMOKE_LIGHT ){
		radius = 32;

		color[0] = color[1] = color[2] = 0.9f;
		color[3] = 1.0f;
	}
	else
		return;

	CG_GetTagPosition( body, body->hModel, "tag_smoke", origin );

	CreateSmokeCloudEntity( origin, dir, 75, radius, 1000, color[0], color[1], color[2], color[3], cgs.media.smokePuffShader );

	cent->engineSmokeTime = cg.time;
}
// END


// SKWID( new function )
/*
===============
CG_AddWheels

Adds the wheels to the car body.
===============
*/
// Q3Rally Code Start
// FIXME: could increase the speed by only looking for components that
// need to be drawn once when the model is loaded
static void CG_AddWheels( centity_t *cent, refEntity_t *body, float angle )
{
	clientInfo_t	*ci;
	centity_t		*cent2;
	refEntity_t		wheel_fl, wheel_fr, wheel_rl, wheel_rr;
	refEntity_t		susp_cl, susp_cr;
//	refEntity_t		susp_fl, susp_fr;
	vec3_t			whlAngles;
	float			vel;
	int				i;
	float			springLength;
	qboolean		corpse;

	ci = &cgs.clientinfo[cent->currentState.clientNum];
	cent2 = &cg_entities[cent->currentState.clientNum];
// END

	memset( &wheel_fl, 0, sizeof(wheel_fl) );
	memset( &wheel_fr, 0, sizeof(wheel_fr) );
	memset( &wheel_rl, 0, sizeof(wheel_rl) );
	memset( &wheel_rr, 0, sizeof(wheel_rr) );

	corpse = (cent->currentState.eFlags & EF_DEAD) && !(cent2->currentState.eFlags & EF_DEAD);

// Q3Rally Code Start
	vel = DotProduct(cent2->currentState.pos.trDelta, body->axis[0]);
	if( cent->currentState.clientNum != cg.snap->ps.clientNum )
	{
		cent2->wheelSpeeds[3] = cent2->wheelSpeeds[2] = cent2->wheelSpeeds[1] = cent2->wheelSpeeds[0] = -vel / WHEEL_RADIUS;
	}

	if ( !corpse ){
		for (i = 0; i < 4; i++){
			cent2->wheelAngles[i] += 180.0f / M_PI * cent2->wheelSpeeds[i] * (cg.frametime / 1000.0f);
			cent2->wheelAngles[i] = AngleNormalize360(cent2->wheelAngles[i]);
		}
	}

	VectorClear(whlAngles);
	if ( !corpse )
		whlAngles[YAW] = angle;
// END

	VectorCopy( body->lightingOrigin, wheel_fl.lightingOrigin );
	VectorCopy( body->lightingOrigin, wheel_fr.lightingOrigin );
	VectorCopy( body->lightingOrigin, wheel_rl.lightingOrigin );
	VectorCopy( body->lightingOrigin, wheel_rr.lightingOrigin );

// Q3Rally Code Start
	wheel_fl.hModel      = ci->wheelModel;
	if ( CG_TagExists( body->hModel, "tag_wheelfl" ) && wheel_fl.hModel ){
		if ( !corpse )
			whlAngles[ROLL] = -cent2->wheelAngles[0];

		if( CG_TagExists( wheel_fl.hModel, "tag_polygonwheel" ) )
			wheel_fl.customSkin  = 0;
		else
			wheel_fl.customSkin  = ci->wheelSkin;
// END
		wheel_fl.shadowPlane = body->shadowPlane;
		wheel_fl.renderfx    = body->renderfx;
// Q3Rally Code Start
		AnglesToAxis(whlAngles, wheel_fl.axis);
		CG_PositionRotatedEntityOnTag( &wheel_fl, body, body->hModel, "tag_wheelfl" );
		
		if (!corpse && (cent->currentState.clientNum == cg.snap->ps.clientNum)){
//			VectorMA(wheel_fl.origin, 8.2 - CP_SPRING_MINLEN - cg.snap->ps.legsTimer / CP_SPRING_SCALE, body->axis[2], wheel_fl.origin);
			VectorMA(wheel_fl.origin, 8.2 - CP_SPRING_MINLEN - cg.predictedPlayerState.legsTimer / CP_SPRING_SCALE, body->axis[2], wheel_fl.origin);
		}
		CG_AddRefEntityWithPowerups( &wheel_fl, &cent->currentState, ci->team );

		// skids
		if (!(cent->currentState.eFlags & EF_DEAD))
			CG_SurfaceEffects(cent2, wheel_fl.origin, body->axis[2], 0);
	}
// END

// Q3Rally Code Start
	wheel_fr.hModel      = ci->wheelModel;
	if (CG_TagExists(body->hModel, "tag_wheelfr") && wheel_fr.hModel){
		if ( !corpse )
			whlAngles[ROLL] = cent2->wheelAngles[1];

		if( CG_TagExists( wheel_fr.hModel, "tag_polygonwheel" ) )
			wheel_fr.customSkin  = 0;
		else
			wheel_fr.customSkin  = ci->wheelSkin;
// END
		wheel_fr.shadowPlane = body->shadowPlane;
		wheel_fr.renderfx    = body->renderfx;
// Q3Rally Code Start
		AnglesToAxis(whlAngles, wheel_fr.axis);
		CG_PositionRotatedEntityOnTag( &wheel_fr, body, body->hModel, "tag_wheelfr" );
		
		if (!corpse && (cent->currentState.clientNum == cg.snap->ps.clientNum))
		{
			springLength = 8.2 - CP_SPRING_MINLEN - cg.predictedPlayerState.legsAnim / CP_SPRING_SCALE;
//			VectorMA(wheel_fr.origin, 8.2 - CP_SPRING_MINLEN - cg.snap->ps.legsAnim / CP_SPRING_SCALE, body->axis[2], wheel_fr.origin);
//			VectorMA(wheel_fr.origin, 8.2 - CP_SPRING_MINLEN - cg.predictedPlayerState.legsAnim / CP_SPRING_SCALE, body->axis[2], wheel_fr.origin);
			VectorMA(wheel_fr.origin, springLength, body->axis[2], wheel_fr.origin);
		}
		CG_AddRefEntityWithPowerups( &wheel_fr, &cent->currentState, ci->team );

		// skids
		if (!(cent->currentState.eFlags & EF_DEAD))
			CG_SurfaceEffects(cent2, wheel_fr.origin, body->axis[2], 1);
	}

	whlAngles[YAW] = 0;
	wheel_rl.hModel      = ci->wheelModel;
	if (CG_TagExists(body->hModel, "tag_wheelrl") && wheel_rl.hModel){
		if ( !corpse )
			whlAngles[ROLL] = -cent2->wheelAngles[2];

		if( CG_TagExists( wheel_rl.hModel, "tag_polygonwheel" ) )
			wheel_rl.customSkin  = 0;
		else
			wheel_rl.customSkin  = ci->wheelSkin;
// END
		wheel_rl.shadowPlane = body->shadowPlane;
		wheel_rl.renderfx    = body->renderfx;
// Q3Rally Code Start
		AnglesToAxis(whlAngles, wheel_rl.axis);
		CG_PositionRotatedEntityOnTag( &wheel_rl, body, body->hModel, "tag_wheelrl" );
		
		if (!corpse && (cent->currentState.clientNum == cg.snap->ps.clientNum))
		{
//			VectorMA(wheel_rl.origin, 8.2 - CP_SPRING_MINLEN - cg.snap->ps.torsoTimer / CP_SPRING_SCALE, body->axis[2], wheel_rl.origin);
			VectorMA(wheel_rl.origin, 8.2 - CP_SPRING_MINLEN - cg.predictedPlayerState.torsoTimer / CP_SPRING_SCALE, body->axis[2], wheel_rl.origin);
		}
		CG_AddRefEntityWithPowerups( &wheel_rl, &cent->currentState, ci->team );

		// skids
		if (!(cent->currentState.eFlags & EF_DEAD))
			CG_SurfaceEffects(cent2, wheel_rl.origin, body->axis[2], 2);
	}
// END

// Q3Rally Code Start
	wheel_rr.hModel      = ci->wheelModel;
	if (CG_TagExists(body->hModel, "tag_wheelrr") && wheel_rr.hModel){
		if ( !corpse )
			whlAngles[ROLL] = cent2->wheelAngles[3];

		if( CG_TagExists( wheel_rr.hModel, "tag_polygonwheel" ) )
			wheel_rr.customSkin  = 0;
		else
			wheel_rr.customSkin  = ci->wheelSkin;
// END
		wheel_rr.shadowPlane = body->shadowPlane;
		wheel_rr.renderfx    = body->renderfx;
// Q3Rally Code Start
		AnglesToAxis(whlAngles, wheel_rr.axis);
		CG_PositionRotatedEntityOnTag( &wheel_rr, body, body->hModel, "tag_wheelrr" );

		if (!corpse && (cent->currentState.clientNum == cg.snap->ps.clientNum))
		{
			springLength = 8.2 - CP_SPRING_MINLEN - cg.predictedPlayerState.torsoAnim / CP_SPRING_SCALE;
//			VectorMA(wheel_rr.origin, 8.2 - CP_SPRING_MINLEN - cg.snap->ps.torsoAnim / CP_SPRING_SCALE, body->axis[2], wheel_rr.origin);
//			VectorMA(wheel_rr.origin, 8.2 - CP_SPRING_MINLEN - cg.predictedPlayerState.torsoAnim / CP_SPRING_SCALE, body->axis[2], wheel_rr.origin);
			VectorMA(wheel_rr.origin, springLength, body->axis[2], wheel_rr.origin);
		}
		CG_AddRefEntityWithPowerups( &wheel_rr, &cent->currentState, ci->team );

		// skids
		if (!(cent->currentState.eFlags & EF_DEAD))
			CG_SurfaceEffects(cent2, wheel_rr.origin, body->axis[2], 3);
	}

	memset( &susp_cl, 0, sizeof(susp_cl) );
	memset( &susp_cr, 0, sizeof(susp_cr) );
//	memset( &susp_fl, 0, sizeof(susp_fl) );
//	memset( &susp_fr, 0, sizeof(susp_fr) );

	susp_cl.hModel      = ci->suspCModel;
	if (CG_TagExists(body->hModel, "tag_suspcl") && susp_cl.hModel){
		susp_cl.shadowPlane = body->shadowPlane;
		susp_cl.renderfx    = body->renderfx;

		VectorClear(whlAngles);
		whlAngles[PITCH] = -M_180_PI * atan2((8.2 - cg.snap->ps.legsTimer / 10.0f) - (8.2 - cg.snap->ps.torsoTimer / 10.0f), 2*WHEEL_FORWARD);
		AnglesToAxis(whlAngles, susp_cl.axis);
		CG_PositionRotatedEntityOnTag( &susp_cl, body, body->hModel, "tag_suspcl" );
		
		if (!corpse && (cent->currentState.clientNum == cg.snap->ps.clientNum)){
//			VectorMA(susp_cl.origin, ((8.2 - cg.snap->ps.legsTimer / 10.0f) + (8.2 - cg.snap->ps.torsoTimer / 10.0f))/2, body->axis[2], susp_cl.origin);
			VectorMA(susp_cl.origin, ((8.2 - cg.predictedPlayerState.legsTimer / 10.0f) + (8.2 - cg.predictedPlayerState.torsoTimer / 10.0f))/2, body->axis[2], susp_cl.origin);
		}
		trap_R_AddRefEntityToScene( &susp_cl );
	}

	susp_cr.hModel      = ci->suspCModel;
	if (CG_TagExists(body->hModel, "tag_suspcr") && susp_cr.hModel){
		susp_cr.shadowPlane = body->shadowPlane;
		susp_cr.renderfx    = body->renderfx;

		VectorClear(whlAngles);
		whlAngles[PITCH] = -M_180_PI * atan2((8.2 - cg.snap->ps.legsAnim / 10.0f) - (8.2 - cg.snap->ps.torsoAnim / 10.0f), 2*WHEEL_FORWARD);
		AnglesToAxis(whlAngles, susp_cr.axis);
		CG_PositionRotatedEntityOnTag( &susp_cr, body, body->hModel, "tag_suspcr" );
		
		if (!corpse && (cent->currentState.clientNum == cg.snap->ps.clientNum)){
//			VectorMA(susp_cr.origin, ((8.2 - cg.snap->ps.legsAnim / 10.0f) + (8.2 - cg.snap->ps.torsoAnim / 10.0f))/2, body->axis[2], susp_cr.origin);
			VectorMA(susp_cr.origin, ((8.2 - cg.predictedPlayerState.legsAnim / 10.0f) + (8.2 - cg.predictedPlayerState.torsoAnim / 10.0f))/2, body->axis[2], susp_cr.origin);
		}
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
		whlAngles[PITCH] = -M_180_PI * atan2(8.2 - cg.snap->ps.legsAnim / 10.0f, -DotProduct(delta, body->axis[1]));
		AnglesToAxis(whlAngles, susp_fr.axis);
		CG_PositionRotatedEntityOnTag( &susp_fr, body, body->hModel, "tag_suspfr" );
		
		trap_R_AddRefEntityToScene( &susp_fr );
	}

	if (CG_TagExists(body->hModel, "tag_suspfl")){
		susp_fl.hModel      = trap_R_RegisterModel("models/players/reaper/suspf.md3");
		susp_fl.customShader = trap_R_RegisterShader("models/players/reaper/parts_blue.tga");
		susp_fl.shadowPlane = body->shadowPlane;
		susp_fl.renderfx    = body->renderfx;

		CG_GetTagPosition(body, body->hModel, "tag_suspfl", susp_fl.origin);
		VectorSubtract(wheel_fl.origin, susp_fl.origin, delta);

		VectorClear(whlAngles);
		whlAngles[PITCH] = M_180_PI * atan2(8.2 - cg.snap->ps.legsTimer / 10.0f, DotProduct(delta, body->axis[1]));
		AnglesToAxis(whlAngles, susp_fl.axis);
		CG_PositionRotatedEntityOnTag( &susp_fl, body, body->hModel, "tag_suspfl" );
		
		trap_R_AddRefEntityToScene( &susp_fl );
	}
*/
	cent2->teleportFlag = 0;
// END
}
// END

/*
=================
CG_LightVerts
=================
*/
int CG_LightVerts( vec3_t normal, int numVerts, polyVert_t *verts )
{
	int				i, j;
	float			incoming;
	vec3_t			ambientLight;
	vec3_t			lightDir;
	vec3_t			directedLight;

	trap_R_LightForPoint( verts[0].xyz, ambientLight, directedLight, lightDir );

	for (i = 0; i < numVerts; i++) {
		incoming = DotProduct (normal, lightDir);
		if ( incoming <= 0 ) {
			verts[i].modulate[0] = ambientLight[0];
			verts[i].modulate[1] = ambientLight[1];
			verts[i].modulate[2] = ambientLight[2];
			verts[i].modulate[3] = 255;
			continue;
		} 
		j = ( ambientLight[0] + incoming * directedLight[0] );
		if ( j > 255 ) {
			j = 255;
		}
		verts[i].modulate[0] = j;

		j = ( ambientLight[1] + incoming * directedLight[1] );
		if ( j > 255 ) {
			j = 255;
		}
		verts[i].modulate[1] = j;

		j = ( ambientLight[2] + incoming * directedLight[2] );
		if ( j > 255 ) {
			j = 255;
		}
		verts[i].modulate[2] = j;

		verts[i].modulate[3] = 255;
	}
	return qtrue;
}

/*
===============
CG_Player
===============
*/
void CG_Player( centity_t *cent ) {
	clientInfo_t	*ci;
// SKWID( replaced player with car )
	refEntity_t		body;
/*
	refEntity_t		legs;
	refEntity_t		torso;
*/
	refEntity_t		head;
	refEntity_t		plate;
	refEntity_t		turbo;
	refEntity_t		headlight;
	refEntity_t		brakelight;
	refEntity_t		reverselight;
	refEntity_t		emergencylight;
// END
// Q3Rally Code Start
	centity_t		*other;
	char			filename[MAX_QPATH];
	vec3_t			angles;
	float			wheelAngle;
	int				i;
// END
	int				clientNum;
	int				renderfx;
	qboolean		shadow;
	float			shadowPlane;
#ifdef MISSIONPACK
	refEntity_t		skull;
	refEntity_t		powerup;
	int				t;
	float			c;
	float			angle;
// Q3Rally Code Start
//	vec3_t			dir;
#endif
	vec3_t			dir;
// END

	// the client number is stored in clientNum.  It can't be derived
	// from the entity number, because a single client may have
	// multiple corpses on the level using the same clientinfo
	clientNum = cent->currentState.clientNum;
	if ( clientNum < 0 || clientNum >= MAX_CLIENTS ) {
		CG_Error( "Bad clientNum on player entity");
	}
	ci = &cgs.clientinfo[ clientNum ];

	// it is possible to see corpses from disconnected players that may
	// not have valid clientinfo
	if ( !ci->infoValid ) {
		return;
	}

	// get the player model information
	renderfx = 0;
	if ( cent->currentState.number == cg.snap->ps.clientNum) {
		if (!cg.renderingThirdPerson) {
			renderfx = RF_THIRD_PERSON;			// only draw in mirrors
		} else {
			if (cg_cameraMode.integer) {
				return;
			}
		}
	}


// SKWID( replaced player with car )
	memset( &body, 0, sizeof(body) );
/*
	memset( &legs, 0, sizeof(legs) );
	memset( &torso, 0, sizeof(torso) );
*/
	memset( &head, 0, sizeof(head) );
	memset( &plate, 0, sizeof(plate) );
	memset( &headlight, 0, sizeof(headlight) );
	memset( &brakelight, 0, sizeof(brakelight) );
	memset( &reverselight, 0, sizeof(reverselight) );
	memset( &emergencylight, 0, sizeof(emergencylight) );
	memset( &turbo, 0, sizeof(turbo) );
// END

// SKWID( removed animation )
/*
	// get the rotation information
	CG_PlayerAngles( cent, legs.axis, torso.axis, head.axis );
	
	// get the animation state (after rotation, to allow feet shuffle)
	CG_PlayerAnimation( cent, &legs.oldframe, &legs.frame, &legs.backlerp,
		 &torso.oldframe, &torso.frame, &torso.backlerp );
*/
// END

	// add the talk baloon or disconnect icon
	CG_PlayerSprites( cent );

	// add the shadow
	shadow = CG_PlayerShadow( cent, &shadowPlane );

	// add a water splash if partially in and out of water
	CG_PlayerSplash( cent );

	if ( cg_shadows.integer == 3 && shadow ) {
		renderfx |= RF_SHADOW_PLANE;
	}
	renderfx |= RF_LIGHTING_ORIGIN;			// use the same origin for all
#ifdef MISSIONPACK
	if( cgs.gametype == GT_HARVESTER ) {
		CG_PlayerTokens( cent, renderfx );
	}
#endif

// SKWID( replaced player with car )
	body.hModel = ci->bodyModel;
	if( !body.hModel)
		return;
	body.customSkin = ci->bodySkin;

// Q3Rally Code Start
//	if ( cg_entities[clientNum].finishRaceTime ) {
//		body.customShader = cgs.media.ghostCarShader;
//	}
// END

	VectorCopy( cent->lerpOrigin, body.origin );
	VectorCopy( cent->lerpOrigin, body.lightingOrigin );
	body.shadowPlane = shadowPlane;
	body.renderfx = renderfx;
	VectorCopy( body.origin, body.oldorigin );

// Q3Rally Code Start
	// UPDATE - need to use lerpAngles?
	VectorCopy( cent->lerpAngles, angles);
	if (ci->oppositeRoll)
		angles[ROLL] *= -1.0f;
	AnglesToAxis( angles, body.axis );

//	if (cg_entities[clientNum].finishRaceTime)
//		trap_R_AddRefEntityToScene( &body );
//	else
		CG_AddRefEntityWithPowerups( &body, &cent->currentState, ci->team );


	// engine sounds

	if( cent->currentState.clientNum == cg.predictedPlayerState.clientNum &&
		cg_engineSounds.integer &&
		cent->engineSoundTime + cg_engineSoundDelay.integer < cg.time )
	{
		int index = (int) (10.0f * (cg.predictedPlayerState.stats[STAT_RPM] - CP_RPM_MIN) / (CP_RPM_MAX - CP_RPM_MIN));

		trap_S_StartSound( cg.predictedPlayerState.origin,
						cg.predictedPlayerState.clientNum,
						CHAN_VOICE,
						cgs.clientinfo[cg.predictedPlayerState.clientNum].sounds[index] );

		cent->engineSoundTime = cg.time;
	}

	if( cent->currentState.clientNum == cg.predictedPlayerState.clientNum &&
		cg_engineSounds.integer )
	{
		int index = (int) (10.0f * (cg.predictedPlayerState.stats[STAT_RPM] - CP_RPM_MIN) / (CP_RPM_MAX - CP_RPM_MIN));

		trap_S_AddLoopingSound( cent->currentState.number + 32,
								cent->lerpOrigin,
								vec3_origin,
								cgs.clientinfo[cent->currentState.clientNum].sounds[index] );

//		trap_S_StopLoopingSound( cg.predictedPlayerState.clientNum );
//		trap_S_AddLoopingSound( cg_entities[cg.predictedPlayerState.clientNum].currentState.number,
//								cg.predictedPlayerState.origin,
//								cg.predictedPlayerState.velocity,
//								cgs.clientinfo[cg.predictedPlayerState.clientNum].sounds[index] );
	}

	if (ci->controlMode == CT_MOUSE){
		wheelAngle = WheelAngle(cent->currentState.apos.trBase[YAW], cent->currentState.angles2[YAW]);
	}
	else {
		wheelAngle = cent->currentState.angles2[YAW];
	}

	// add the car wheels
//	CG_AddWheels(cent, &body, wheelAngle);
	// use this so proper angles are used when dead
	if( cent->currentState.clientNum == cg.snap->ps.clientNum )
		CG_AddWheels( cent, &body, cg_entities[cent->currentState.clientNum].steeringAngle );
	else
		CG_AddWheels( cent, &body, wheelAngle );

	CG_PlayerEngineSmoke(cent, &body);
// END

// Q3Rally Code Start
#if 0
	//
	// add the legs
	//
	legs.hModel = ci->legsModel;
	legs.customSkin = ci->legsSkin;

	VectorCopy( cent->lerpOrigin, legs.origin );

	VectorCopy( cent->lerpOrigin, legs.lightingOrigin );
	legs.shadowPlane = shadowPlane;
	legs.renderfx = renderfx;
	VectorCopy (legs.origin, legs.oldorigin);	// don't positionally lerp at all

	CG_AddRefEntityWithPowerups( &legs, &cent->currentState, ci->team );

	// if the model failed, allow the default nullmodel to be displayed
	if (!legs.hModel) {
		return;
	}

	//
	// add the torso
	//
	torso.hModel = ci->torsoModel;
	if (!torso.hModel) {
		return;
	}

	torso.customSkin = ci->torsoSkin;

	VectorCopy( cent->lerpOrigin, torso.lightingOrigin );

	CG_PositionRotatedEntityOnTag( &torso, &legs, ci->legsModel, "tag_torso");

	torso.shadowPlane = shadowPlane;
	torso.renderfx = renderfx;

	CG_AddRefEntityWithPowerups( &torso, &cent->currentState, ci->team );

#ifdef MISSIONPACK
	if ( cent->currentState.eFlags & EF_KAMIKAZE ) {

		memset( &skull, 0, sizeof(skull) );

		VectorCopy( cent->lerpOrigin, skull.lightingOrigin );
		skull.shadowPlane = shadowPlane;
		skull.renderfx = renderfx;

		if ( cent->currentState.eFlags & EF_DEAD ) {
			// one skull bobbing above the dead body
			angle = ((cg.time / 7) & 255) * (M_PI * 2) / 255;
			if (angle > M_PI * 2)
				angle -= (float)M_PI * 2;
			dir[0] = sin(angle) * 20;
			dir[1] = cos(angle) * 20;
			angle = ((cg.time / 4) & 255) * (M_PI * 2) / 255;
			dir[2] = 15 + sin(angle) * 8;
			VectorAdd(torso.origin, dir, skull.origin);
			
			dir[2] = 0;
			VectorCopy(dir, skull.axis[1]);
			VectorNormalize(skull.axis[1]);
			VectorSet(skull.axis[2], 0, 0, 1);
			CrossProduct(skull.axis[1], skull.axis[2], skull.axis[0]);

			skull.hModel = cgs.media.kamikazeHeadModel;
			trap_R_AddRefEntityToScene( &skull );
			skull.hModel = cgs.media.kamikazeHeadTrail;
			trap_R_AddRefEntityToScene( &skull );
		}
		else {
			// three skulls spinning around the player
			angle = ((cg.time / 4) & 255) * (M_PI * 2) / 255;
			dir[0] = cos(angle) * 20;
			dir[1] = sin(angle) * 20;
			dir[2] = cos(angle) * 20;
			VectorAdd(torso.origin, dir, skull.origin);

			angles[0] = sin(angle) * 30;
			angles[1] = (angle * 180 / M_PI) + 90;
			if (angles[1] > 360)
				angles[1] -= 360;
			angles[2] = 0;
			AnglesToAxis( angles, skull.axis );

			/*
			dir[2] = 0;
			VectorInverse(dir);
			VectorCopy(dir, skull.axis[1]);
			VectorNormalize(skull.axis[1]);
			VectorSet(skull.axis[2], 0, 0, 1);
			CrossProduct(skull.axis[1], skull.axis[2], skull.axis[0]);
			*/

			skull.hModel = cgs.media.kamikazeHeadModel;
			trap_R_AddRefEntityToScene( &skull );
			// flip the trail because this skull is spinning in the other direction
			VectorInverse(skull.axis[1]);
			skull.hModel = cgs.media.kamikazeHeadTrail;
			trap_R_AddRefEntityToScene( &skull );

			angle = ((cg.time / 4) & 255) * (M_PI * 2) / 255 + M_PI;
			if (angle > M_PI * 2)
				angle -= (float)M_PI * 2;
			dir[0] = sin(angle) * 20;
			dir[1] = cos(angle) * 20;
			dir[2] = cos(angle) * 20;
			VectorAdd(torso.origin, dir, skull.origin);

			angles[0] = cos(angle - 0.5 * M_PI) * 30;
			angles[1] = 360 - (angle * 180 / M_PI);
			if (angles[1] > 360)
				angles[1] -= 360;
			angles[2] = 0;
			AnglesToAxis( angles, skull.axis );

			/*
			dir[2] = 0;
			VectorCopy(dir, skull.axis[1]);
			VectorNormalize(skull.axis[1]);
			VectorSet(skull.axis[2], 0, 0, 1);
			CrossProduct(skull.axis[1], skull.axis[2], skull.axis[0]);
			*/

			skull.hModel = cgs.media.kamikazeHeadModel;
			trap_R_AddRefEntityToScene( &skull );
			skull.hModel = cgs.media.kamikazeHeadTrail;
			trap_R_AddRefEntityToScene( &skull );

			angle = ((cg.time / 3) & 255) * (M_PI * 2) / 255 + 0.5 * M_PI;
			if (angle > M_PI * 2)
				angle -= (float)M_PI * 2;
			dir[0] = sin(angle) * 20;
			dir[1] = cos(angle) * 20;
			dir[2] = 0;
			VectorAdd(torso.origin, dir, skull.origin);
			
			VectorCopy(dir, skull.axis[1]);
			VectorNormalize(skull.axis[1]);
			VectorSet(skull.axis[2], 0, 0, 1);
			CrossProduct(skull.axis[1], skull.axis[2], skull.axis[0]);

			skull.hModel = cgs.media.kamikazeHeadModel;
			trap_R_AddRefEntityToScene( &skull );
			skull.hModel = cgs.media.kamikazeHeadTrail;
			trap_R_AddRefEntityToScene( &skull );
		}
	}

	if ( cent->currentState.powerups & ( 1 << PW_GUARD ) ) {
		memcpy(&powerup, &torso, sizeof(torso));
		powerup.hModel = cgs.media.guardPowerupModel;
		powerup.frame = 0;
		powerup.oldframe = 0;
		powerup.customSkin = 0;
		trap_R_AddRefEntityToScene( &powerup );
	}
	if ( cent->currentState.powerups & ( 1 << PW_SCOUT ) ) {
		memcpy(&powerup, &torso, sizeof(torso));
		powerup.hModel = cgs.media.scoutPowerupModel;
		powerup.frame = 0;
		powerup.oldframe = 0;
		powerup.customSkin = 0;
		trap_R_AddRefEntityToScene( &powerup );
	}
	if ( cent->currentState.powerups & ( 1 << PW_DOUBLER ) ) {
		memcpy(&powerup, &torso, sizeof(torso));
		powerup.hModel = cgs.media.doublerPowerupModel;
		powerup.frame = 0;
		powerup.oldframe = 0;
		powerup.customSkin = 0;
		trap_R_AddRefEntityToScene( &powerup );
	}
	if ( cent->currentState.powerups & ( 1 << PW_AMMOREGEN ) ) {
		memcpy(&powerup, &torso, sizeof(torso));
		powerup.hModel = cgs.media.ammoRegenPowerupModel;
		powerup.frame = 0;
		powerup.oldframe = 0;
		powerup.customSkin = 0;
		trap_R_AddRefEntityToScene( &powerup );
	}
	if ( cent->currentState.powerups & ( 1 << PW_INVULNERABILITY ) ) {
		if ( !ci->invulnerabilityStartTime ) {
			ci->invulnerabilityStartTime = cg.time;
		}
		ci->invulnerabilityStopTime = cg.time;
	}
	else {
		ci->invulnerabilityStartTime = 0;
	}
	if ( (cent->currentState.powerups & ( 1 << PW_INVULNERABILITY ) ) ||
		cg.time - ci->invulnerabilityStopTime < 250 ) {

		memcpy(&powerup, &torso, sizeof(torso));
		powerup.hModel = cgs.media.invulnerabilityPowerupModel;
		powerup.frame = 0;
		powerup.oldframe = 0;
		powerup.customSkin = 0;
		// always draw
		powerup.renderfx &= ~RF_THIRD_PERSON;
		VectorCopy(cent->lerpOrigin, powerup.origin);

		if ( cg.time - ci->invulnerabilityStartTime < 250 ) {
			c = (float) (cg.time - ci->invulnerabilityStartTime) / 250;
		}
		else if (cg.time - ci->invulnerabilityStopTime < 250 ) {
			c = (float) (250 - (cg.time - ci->invulnerabilityStopTime)) / 250;
		}
		else {
			c = 1;
		}
		VectorSet( powerup.axis[0], c, 0, 0 );
		VectorSet( powerup.axis[1], 0, c, 0 );
		VectorSet( powerup.axis[2], 0, 0, c );
		trap_R_AddRefEntityToScene( &powerup );
	}

	t = cg.time - ci->medkitUsageTime;
	if ( ci->medkitUsageTime && t < 500 ) {
		memcpy(&powerup, &torso, sizeof(torso));
		powerup.hModel = cgs.media.medkitUsageModel;
		powerup.frame = 0;
		powerup.oldframe = 0;
		powerup.customSkin = 0;
		// always draw
		powerup.renderfx &= ~RF_THIRD_PERSON;
		VectorClear(angles);
		AnglesToAxis(angles, powerup.axis);
		VectorCopy(cent->lerpOrigin, powerup.origin);
		powerup.origin[2] += -24 + (float) t * 80 / 500;
		if ( t > 400 ) {
			c = (float) (t - 1000) * 0xff / 100;
			powerup.shaderRGBA[0] = 0xff - c;
			powerup.shaderRGBA[1] = 0xff - c;
			powerup.shaderRGBA[2] = 0xff - c;
			powerup.shaderRGBA[3] = 0xff - c;
		}
		else {
			powerup.shaderRGBA[0] = 0xff;
			powerup.shaderRGBA[1] = 0xff;
			powerup.shaderRGBA[2] = 0xff;
			powerup.shaderRGBA[3] = 0xff;
		}
		trap_R_AddRefEntityToScene( &powerup );
	}
#endif // MISSIONPACK
#endif // Q3Rally

// STONELANCE( add head support )
/*
	//
	// add the head
	//
	head.hModel = ci->headModel;
	if (!head.hModel) {
		return;
	}
	head.customSkin = ci->headSkin;

	VectorCopy( cent->lerpOrigin, head.lightingOrigin );

	CG_PositionRotatedEntityOnTag( &head, &torso, ci->torsoModel, "tag_head");

	head.shadowPlane = shadowPlane;
	head.renderfx = renderfx;

	CG_AddRefEntityWithPowerups( &head, &cent->currentState, ci->team );
*/

	//
	// add the head
	//
	if ( CG_TagExists(ci->bodyModel, "tag_head") ){
		head.hModel = ci->headModel;
		if (!head.hModel) {
			return;
		}
		head.customSkin = ci->headSkin;

		VectorCopy( cent->currentState.angles2, angles );
		if (ci->controlMode == CT_MOUSE){
			angles[YAW] = Com_Clamp(-45, 45, AngleNormalize180(cent->currentState.apos.trBase[YAW] - angles[YAW]));
		}
		AnglesToAxis(angles, head.axis);

//		if ( cg_entities[clientNum].finishRaceTime ) {
//			head.customShader = cgs.media.ghostCarShader;
//		}

		VectorCopy( cent->lerpOrigin, head.lightingOrigin );
		CG_PositionRotatedEntityOnTag( &head, &body, ci->bodyModel, "tag_head");
		head.shadowPlane = shadowPlane;
		head.renderfx = renderfx;

		// check for head tag, if no tag the length of delta should be 0
//		if (cg_entities[clientNum].finishRaceTime)
//			trap_R_AddRefEntityToScene( &head );
//		else
			CG_AddRefEntityWithPowerups( &head, &cent->currentState, ci->team );
	}

	//
	// add the license plate
	//
	if (!(cent->currentState.powerups & ( 1 << PW_INVIS )) && CG_TagExists(ci->bodyModel, "tag_plate")){
		plate.frame = 0;
		plate.hModel = ci->plateModel;
		if (!plate.hModel) {
			return;
		}

		plate.customShader = ci->plateShader;

		VectorCopy( cent->lerpOrigin, plate.lightingOrigin );
		CG_PositionEntityOnTag( &plate, &body, ci->bodyModel, "tag_plate");
		plate.shadowPlane = shadowPlane;
		plate.renderfx = renderfx;

/*
		plate.radius = 20;
		plate.shaderRGBA[0] = 255;
		plate.shaderRGBA[1] = 255;
		plate.shaderRGBA[2] = 255;
		plate.shaderRGBA[3] = 255;
		plate.shaderTime = cg.time / 1000.0f;
*/
		trap_R_AddRefEntityToScene( &plate );
	}

	//
	// add the headlights
	//
/*
	VectorMA(body.origin, 200, body.axis[0], dir);
	VectorMA(dir, 75, body.axis[1], dir);
	trap_R_AddAdditiveLightToScene( dir, 200, 0.3f, 0.3f, 0.3f );
	VectorMA(dir, -150, body.axis[1], dir);
	trap_R_AddAdditiveLightToScene( dir, 200, 0.3f, 0.3f, 0.3f );
*/
	if (/*cg_entities[cent->currentState.clientNum].hl_left->light &&*/
		!(cent->currentState.powerups & ( 1 << PW_INVIS ))){

		headlight.hModel = cgs.media.headLightGlow;
		if (!headlight.hModel) {
			return;
		}

		VectorCopy( cent->lerpOrigin, headlight.lightingOrigin );
		headlight.shadowPlane = shadowPlane;
		headlight.renderfx = renderfx;

		for (i = 0; i < 4; i++){
			Com_sprintf(filename, sizeof(filename), "tag_hlite%d", i+1);
			if (!CG_TagExists(ci->bodyModel, filename)) continue;

			CG_PositionEntityOnTag( &headlight, &body, ci->bodyModel, filename);
			trap_R_AddRefEntityToScene( &headlight );
		}
	}

	//
	// add the red and blue lights for emergency vehicles
	//
	if (/*cg_entities[cent->currentState.clientNum].hl_left->light &&*/
		!(cent->currentState.powerups & ( 1 << PW_INVIS ))){

		VectorCopy( cent->lerpOrigin, emergencylight.lightingOrigin );
		emergencylight.shadowPlane = shadowPlane;
		emergencylight.renderfx = renderfx;

		// red
		if ( CG_TagExists(ci->bodyModel, "tag_redlight") ){
			emergencylight.hModel = trap_R_RegisterModel( "gfx/flares/red_lite.md3" );
			if (!emergencylight.hModel) {
				return;
			}

			CG_PositionEntityOnTag( &emergencylight, &body, ci->bodyModel, "tag_redlight");
			trap_R_AddRefEntityToScene( &emergencylight );
		}

		// blue
		if ( CG_TagExists(ci->bodyModel, "tag_bluelight") ){
			emergencylight.hModel = trap_R_RegisterModel( "gfx/flares/blue_lite.md3" );
			if (!emergencylight.hModel) {
				return;
			}

			CG_PositionEntityOnTag( &emergencylight, &body, ci->bodyModel, "tag_bluelight");
			trap_R_AddRefEntityToScene( &emergencylight );
		}

		// yellow
		if ( CG_TagExists(ci->bodyModel, "tag_yellowlight") ) {
			emergencylight.hModel = trap_R_RegisterModel( "gfx/flares/yellow_lite.md3" );
			if (!emergencylight.hModel) {
				return;
			}

			CG_PositionEntityOnTag( &emergencylight, &body, ci->bodyModel, "tag_yellowlight");
			trap_R_AddRefEntityToScene( &emergencylight );
		}
	}

	//
	// add the brakelights
	//
	if ((cg_entities[cent->currentState.clientNum].currentState.eFlags & EF_BRAKE) &&
		!(cent->currentState.powerups & ( 1 << PW_INVIS ))){
		brakelight.hModel = cgs.media.brakeLightGlow;
		if (!brakelight.hModel) {
			return;
		}

		VectorCopy( cent->lerpOrigin, brakelight.lightingOrigin );
		brakelight.shadowPlane = shadowPlane;
		brakelight.renderfx = renderfx;

		for (i = 0; i < 3; i++){
			Com_sprintf(filename, sizeof(filename), "tag_blite%d", i+1);
			if (!CG_TagExists(ci->bodyModel, filename)) continue;

			CG_PositionEntityOnTag( &brakelight, &body, ci->bodyModel, filename);
			trap_R_AddRefEntityToScene( &brakelight );
		}
	}

	//
	// add the reverselights
	//
	if ((cg_entities[cent->currentState.clientNum].currentState.eFlags & EF_REVERSE) &&
		VectorLength(cent->currentState.pos.trDelta) > 10.0f && 
		!(cent->currentState.powerups & ( 1 << PW_INVIS ))){
		reverselight.hModel = cgs.media.reverseLightGlow;
		if (!reverselight.hModel) {
			return;
		}

		VectorCopy( cent->lerpOrigin, reverselight.lightingOrigin );
		reverselight.shadowPlane = shadowPlane;
		reverselight.renderfx = renderfx;

		for (i = 0; i < 2; i++){
			Com_sprintf(filename, sizeof(filename), "tag_rlite%d", i+1);
			if (!CG_TagExists(ci->bodyModel, filename)) continue;

			CG_PositionEntityOnTag( &reverselight, &body, ci->bodyModel, filename);
			trap_R_AddRefEntityToScene( &reverselight );
		}
	}


	//
	// add turbo flame
	//
	if (cent->currentState.powerups & ( 1 << PW_TURBO ) && CG_TagExists(ci->bodyModel, "tag_turbo")){
		turbo.hModel = cgs.media.turboModel;
		if (!turbo.hModel) {
			return;
		}

		VectorCopy( cent->lerpOrigin, turbo.lightingOrigin );
		CG_PositionEntityOnTag( &turbo, &body, ci->bodyModel, "tag_turbo");
		turbo.shadowPlane = shadowPlane;
		turbo.renderfx = renderfx;

		trap_R_AddRefEntityToScene( &turbo );
	}

	// floating arrow to checkpoint
	if (cg_checkpointArrowMode.integer == 2 && cent->currentState.clientNum == cg.snap->ps.clientNum){
		for (i = 0; i < MAX_GENTITIES; i++) {
			other = &cg_entities[i];
			if (other->currentState.eType != ET_CHECKPOINT) continue;
			if (other->currentState.weapon != cg.snap->ps.stats[STAT_NEXT_CHECKPOINT]) continue;

			break;
		}

		if (i != MAX_GENTITIES){
			turbo.hModel = cgs.media.checkpointArrow;
			if (!turbo.hModel) {
				return;
			}

			VectorSubtract(other->currentState.origin, cent->lerpOrigin, dir);

			VectorClear(angles);
			angles[YAW] = vectoyaw(dir);
			AnglesToAxis(angles, turbo.axis);

			VectorScale(turbo.axis[0], 0.5f, turbo.axis[0]);
			VectorScale(turbo.axis[1], 0.5f, turbo.axis[1]);
			turbo.nonNormalizedAxes = qtrue;

			VectorMA( cent->lerpOrigin, 24, turbo.axis[2], turbo.origin );
			VectorCopy( cent->lerpOrigin, turbo.lightingOrigin );
			turbo.shadowPlane = shadowPlane;
			turbo.renderfx = renderfx;

			trap_R_AddRefEntityToScene( &turbo );
		}
	}
    
 
/*
#ifdef MISSIONPACK
	CG_BreathPuffs(cent, &head);

	CG_DustTrail(cent);
#endif
*/
// END

	//
	// add the gun / barrel / flash
	//
// Q3Rally Code Start
//	CG_AddPlayerWeapon( &torso, NULL, cent, ci->team );
	CG_AddPlayerWeapon( &body, NULL, cent, ci->team );
// END

	// add powerups floating behind the player
// Q3Rally Code Start
//	CG_PlayerPowerups( cent, &torso );
	CG_PlayerPowerups( cent, &body );
// END
}


//=====================================================================

/*
===============
CG_ResetPlayerEntity

A player just came into view or teleported, so reset all animation info
===============
*/
void CG_ResetPlayerEntity( centity_t *cent ) {
	cent->errorTime = -99999;		// guarantee no error decay added
	cent->extrapolated = qfalse;	

// SKWID( removed functions )
//	CG_ClearLerpFrame( &cgs.clientinfo[ cent->currentState.clientNum ], &cent->pe.legs, cent->currentState.legsAnim );
//	CG_ClearLerpFrame( &cgs.clientinfo[ cent->currentState.clientNum ], &cent->pe.torso, cent->currentState.torsoAnim );
// END

	BG_EvaluateTrajectory( &cent->currentState.pos, cg.time, cent->lerpOrigin );
	BG_EvaluateTrajectory( &cent->currentState.apos, cg.time, cent->lerpAngles );

	VectorCopy( cent->lerpOrigin, cent->rawOrigin );
	VectorCopy( cent->lerpAngles, cent->rawAngles );

// SKWID( replace player with car )
	memset( &cent->pe.body, 0, sizeof( cent->pe.body ) );
	cent->pe.body.yawAngle		= cent->rawAngles[YAW];
	cent->pe.body.yawing		= qfalse;
	cent->pe.body.pitchAngle	= cent->rawAngles[PITCH];
	cent->pe.body.pitching		= qfalse;

/*
	memset( &cent->pe.legs, 0, sizeof( cent->pe.legs ) );
	cent->pe.legs.yawAngle = cent->rawAngles[YAW];
	cent->pe.legs.yawing = qfalse;
	cent->pe.legs.pitchAngle = 0;
	cent->pe.legs.pitching = qfalse;

	memset( &cent->pe.torso, 0, sizeof( cent->pe.torso ) );
	cent->pe.torso.yawAngle = cent->rawAngles[YAW];
	cent->pe.torso.yawing = qfalse;
	cent->pe.torso.pitchAngle = cent->rawAngles[PITCH];
	cent->pe.torso.pitching = qfalse;

	if ( cg_debugPosition.integer ) {
		CG_Printf("%i ResetPlayerEntity yaw=%f\n", cent->currentState.number, cent->pe.torso.yawAngle );
	}
*/
// END
}

