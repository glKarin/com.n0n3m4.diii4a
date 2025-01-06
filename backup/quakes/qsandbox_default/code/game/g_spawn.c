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

#include "g_local.h"

qboolean	G_SpawnString( const char *key, const char *defaultString, char **out ) {
	int		i;

	if ( !level.spawning ) {
		*out = (char *)defaultString;
//		G_Error( "G_SpawnString() called while not spawning" );
	}

	for ( i = 0 ; i < level.numSpawnVars ; i++ ) {
		if ( !Q_stricmp( key, level.spawnVars[i][0] ) ) {
			*out = level.spawnVars[i][1];
			return qtrue;
		}
	}

	*out = (char *)defaultString;
	return qfalse;
}

qboolean	G_SpawnFloat( const char *key, const char *defaultString, float *out ) {
	char		*s;
	qboolean	present;

	present = G_SpawnString( key, defaultString, &s );
	*out = atof( s );
	return present;
}

qboolean	G_SpawnInt( const char *key, const char *defaultString, int *out ) {
	char		*s;
	qboolean	present;

	present = G_SpawnString( key, defaultString, &s );
	*out = atoi( s );
	return present;
}

qboolean	G_SpawnVector( const char *key, const char *defaultString, float *out ) {
	char		*s;
	qboolean	present;

	present = G_SpawnString( key, defaultString, &s );
	sscanf( s, "%f %f %f", &out[0], &out[1], &out[2] );
	return present;
}

qboolean	G_SpawnVector4( const char *key, const char *defaultString, float *out ) {
	char		*s;
	qboolean	present;

	present = G_SpawnString( key, defaultString, &s );
	sscanf( s, "%f %f %f %f", &out[0], &out[1], &out[2], &out[3] );
	return present;
}


//
// fields are needed for spawning from the entity string
//
typedef enum {
	F_INT, 
	F_FLOAT,
	F_LSTRING,			// string on disk, pointer in memory, TAG_LEVEL
	F_GSTRING,			// string on disk, pointer in memory, TAG_GAME
	F_VECTOR,
	F_ANGLEHACK,
	F_ENTITY,			// index on disk, pointer in memory
	F_ITEM,				// index on disk, pointer in memory
	F_CLIENT,			// index on disk, pointer in memory
	F_IGNORE
} fieldtype_t;

typedef struct
{
	char	*name;
	int		ofs;
	fieldtype_t	type;
	int		flags;
} field_t;

field_t fields[] = {
	{"classname", FOFS(classname), F_LSTRING},
	{"origin", FOFS(s.origin), F_VECTOR},
	{"model", FOFS(model), F_LSTRING},
	{"model2", FOFS(model2), F_LSTRING},
	{"spawnflags", FOFS(spawnflags), F_INT},
	{"speed", FOFS(speed), F_FLOAT},
	{"target", FOFS(target), F_LSTRING},
	{"targetname", FOFS(targetname), F_LSTRING},
	{"message", FOFS(message), F_LSTRING},
	{"botname", FOFS(botname), F_LSTRING},
	{"team", FOFS(team), F_LSTRING},
	{"wait", FOFS(wait), F_FLOAT},
	{"random", FOFS(random), F_FLOAT},
	{"count", FOFS(count), F_INT},
	{"playerangle", FOFS(playerangle), F_INT},
	{"price", FOFS(price), F_INT},
	{"health", FOFS(health), F_INT},
	{"light", 0, F_IGNORE},
	{"dmg", FOFS(damage), F_INT},
	{"mtype", FOFS(mtype), F_INT},
	{"mtimeout", FOFS(mtimeout), F_INT},
	{"mhoming", FOFS(mhoming), F_INT},
	{"mspeed", FOFS(mspeed), F_INT},
	{"mbounce", FOFS(mbounce), F_INT},
	{"mdamage", FOFS(mdamage), F_INT},
	{"msdamage", FOFS(msdamage), F_INT},
	{"msradius", FOFS(msradius), F_INT},
	{"mgravity", FOFS(mgravity), F_INT},
	{"mnoclip", FOFS(mnoclip), F_INT},
	{"allowuse", FOFS(allowuse), F_INT},
	{"angles", FOFS(s.angles), F_VECTOR},
	{"angle", FOFS(s.angles), F_ANGLEHACK},
	{"targetShaderName", FOFS(targetShaderName), F_LSTRING},
	{"targetShaderNewName", FOFS(targetShaderNewName), F_LSTRING},
	{"mapname", FOFS(mapname), F_LSTRING},
	{"clientname", FOFS(clientname), F_LSTRING},
	{"teleporterTarget", FOFS(teleporterTarget), F_LSTRING},
	{"deathTarget", FOFS(deathTarget), F_LSTRING},
	{"lootTarget", FOFS(lootTarget), F_LSTRING},
	{"skill", FOFS(skill), F_FLOAT},
	{"overlay", FOFS(overlay), F_LSTRING},
	{"target2", FOFS(target2), F_LSTRING},
	{"damagetarget", FOFS(damagetarget), F_LSTRING},
	{"targetname2", FOFS(targetname2), F_LSTRING},
	{"key", FOFS(key), F_LSTRING},
	{"value", FOFS(value), F_LSTRING},
	{"armor", FOFS(armor), F_INT},
	{"music", FOFS(music), F_LSTRING},
	{"sb_model", FOFS(sb_model), F_LSTRING},
	{"sb_class", FOFS(sb_class), F_LSTRING},
	{"sb_sound", FOFS(sb_sound), F_LSTRING},
	{"sb_coltype", FOFS(sb_coltype), F_INT},
	{"sb_colscale0", FOFS(sb_colscale0), F_FLOAT},
	{"sb_colscale1", FOFS(sb_colscale1), F_FLOAT},
	{"sb_colscale2", FOFS(sb_colscale2), F_FLOAT},
	{"sb_rotate0", FOFS(sb_rotate0), F_FLOAT},
	{"sb_rotate1", FOFS(sb_rotate1), F_FLOAT},
	{"sb_rotate2", FOFS(sb_rotate2), F_FLOAT},
	{"physicsBounce", FOFS(physicsBounce), F_FLOAT},
	{"vehicle", FOFS(vehicle), F_INT},
	{"sb_material", FOFS(sb_material), F_INT},
	{"sb_gravity", FOFS(sb_gravity), F_INT},
	{"sb_phys", FOFS(sb_phys), F_INT},
	{"sb_coll", FOFS(sb_coll), F_INT},
	{"locked", FOFS(locked), F_INT},
	{"sb_red", FOFS(sb_red), F_INT},
	{"sb_green", FOFS(sb_green), F_INT},
	{"sb_blue", FOFS(sb_blue), F_INT},
	{"sb_radius", FOFS(sb_radius), F_INT},
	{"sb_ettype", FOFS(sb_ettype), F_INT},
	{"sb_takedamage", FOFS(sb_takedamage), F_INT},
	{"sb_takedamage2", FOFS(sb_takedamage2), F_INT},
	{"objectType", FOFS(objectType), F_INT},
	
	{"distance", FOFS(distance), F_FLOAT},
	{"type", FOFS(type), F_INT},
	
	{NULL}
};


typedef struct {
	char	*name;
	void	(*spawn)(gentity_t *ent);
} spawn_t;

void SP_info_player_start (gentity_t *ent);
void SP_info_player_deathmatch (gentity_t *ent);
void SP_info_player_intermission (gentity_t *ent);
//For Double Domination:
void SP_info_player_dd (gentity_t *ent);
void SP_info_player_dd_red (gentity_t *ent);
void SP_info_player_dd_blue (gentity_t *ent);
//standard domination:
void SP_domination_point ( gentity_t *ent);

void SP_info_firstplace(gentity_t *ent);
void SP_info_secondplace(gentity_t *ent);
void SP_info_thirdplace(gentity_t *ent);
void SP_info_podium(gentity_t *ent);
void SP_info_waypoint( gentity_t *self );
void SP_info_backpack( gentity_t *self );

void SP_func_plat (gentity_t *ent);
void SP_func_static (gentity_t *ent);
void SP_func_prop (gentity_t *ent);
void SP_func_breakable (gentity_t *ent);
void SP_func_rotating (gentity_t *ent);
void SP_func_bobbing (gentity_t *ent);
void SP_func_pendulum( gentity_t *ent );
void SP_func_button (gentity_t *ent);
void SP_func_door (gentity_t *ent);
void SP_func_train (gentity_t *ent);
void SP_func_timer (gentity_t *self);

void SP_trigger_always (gentity_t *ent);
void SP_trigger_multiple (gentity_t *ent);
void SP_trigger_push (gentity_t *ent);
void SP_trigger_teleport (gentity_t *ent);
void SP_trigger_hurt (gentity_t *ent);

void SP_trigger_death (gentity_t *ent);
void SP_trigger_frag (gentity_t *ent);
void SP_trigger_lock (gentity_t *ent);

void SP_target_remove_powerups( gentity_t *ent );
void SP_target_give (gentity_t *ent);
void SP_target_delay (gentity_t *ent);
void SP_target_speaker (gentity_t *ent);
void SP_target_print (gentity_t *ent);
void SP_target_laser (gentity_t *self);
void SP_target_character (gentity_t *ent);
void SP_target_score( gentity_t *ent );
void SP_target_clienttarg( gentity_t *ent );
void SP_target_teleporter( gentity_t *ent );
void SP_target_relay (gentity_t *ent);
void SP_target_kill (gentity_t *ent);
void SP_target_position (gentity_t *ent);
void SP_target_location (gentity_t *ent);
void SP_target_push (gentity_t *ent);
void SP_target_logic (gentity_t *ent);
void SP_target_gravity (gentity_t *ent);
void SP_target_mapchange (gentity_t *ent);
void SP_target_botspawn (gentity_t *ent);
void SP_target_unlink (gentity_t *ent);
void SP_target_playerspeed (gentity_t *ent);
void SP_target_debrisemitter (gentity_t *ent);
void SP_target_objective (gentity_t *ent);
void SP_target_skill (gentity_t *ent);
void SP_target_earthquake (gentity_t *ent);
void SP_target_effect (gentity_t *ent);
void SP_target_finish (gentity_t *ent);
void SP_target_modify (gentity_t *ent);
void SP_target_secret (gentity_t *ent);
void SP_target_playerstats (gentity_t *ent);
void SP_target_cutscene (gentity_t *ent);
void SP_target_botremove (gentity_t *ent);
void SP_target_music (gentity_t *ent);
void SP_target_stats (gentity_t *ent);

void SP_script_variable (gentity_t *ent);
void SP_script_layer (gentity_t *ent);
void SP_script_cmd (gentity_t *ent);
void SP_script_menu (gentity_t *ent);
void SP_script_aicontrol (gentity_t *ent);

void SP_light (gentity_t *self);
void SP_info_null (gentity_t *self);
void SP_info_notnull (gentity_t *self);
void SP_info_camp (gentity_t *self);
void SP_info_camera (gentity_t *self);
void SP_path_corner (gentity_t *self);

void SP_misc_teleporter_dest (gentity_t *self);
void SP_misc_model(gentity_t *ent);
void SP_misc_portal_camera(gentity_t *ent);
void SP_misc_portal_surface(gentity_t *ent);

void SP_shooter_rocket( gentity_t *ent );
void SP_shooter_plasma( gentity_t *ent );
void SP_shooter_grenade( gentity_t *ent );
void SP_shooter_bfg( gentity_t *ent );
void SP_shooter_prox( gentity_t *ent );
void SP_shooter_flame( gentity_t *ent );
void SP_shooter_antimatter( gentity_t *ent );
void SP_shooter_custom( gentity_t *ent );

void SP_team_CTF_redplayer( gentity_t *ent );
void SP_team_CTF_blueplayer( gentity_t *ent );

void SP_team_CTF_redspawn( gentity_t *ent );
void SP_team_CTF_bluespawn( gentity_t *ent );

void SP_func_door_rotating( gentity_t *ent );

void SP_team_blueobelisk( gentity_t *ent );
void SP_team_redobelisk( gentity_t *ent );
void SP_team_neutralobelisk( gentity_t *ent );

void SP_item_botroam( gentity_t *ent ) { }

// weather
void SP_rally_weather_rain( gentity_t *ent );
void SP_rally_weather_snow( gentity_t *ent );

spawn_t	spawns[] = {
	// info entities don't do anything at all, but provide positional
	// information for things controlled by other processes
	{"info_player_start", SP_info_player_start},
	{"info_player_deathmatch", SP_info_player_deathmatch},
	{"info_player_intermission", SP_info_player_intermission},
//Double Domination player spawn:
	{"info_player_dd", SP_info_player_dd},
        {"info_player_dd_red", SP_info_player_dd_red},
        {"info_player_dd_blue", SP_info_player_dd_blue},
//Standard Domination point spawn:
	{"domination_point", SP_domination_point},


	{"info_null", SP_info_null},
	{"info_notnull", SP_info_notnull},		// use target_position instead
	{"info_camp", SP_info_camp},
	{"info_waypoint", SP_info_waypoint},
	{"info_backpack", SP_info_backpack},
	{"info_camera", SP_info_camera},

	{"func_plat", SP_func_plat},
	{"func_button", SP_func_button},
	{"func_door", SP_func_door},
	{"func_static", SP_func_static},
	{"func_prop", SP_func_prop},
	{"func_rotating", SP_func_rotating},
	{"func_bobbing", SP_func_bobbing},
	{"func_pendulum", SP_func_pendulum},
	{"func_train", SP_func_train},
	{"func_group", SP_info_null},
	{"func_timer", SP_func_timer},			// rename trigger_timer?
	{"func_breakable", SP_func_breakable},
	{"func_timer", SP_func_timer},			// rename trigger_timer?

	// Triggers are brush objects that cause an effect when contacted
	// by a living player, usually involving firing targets.
	// While almost everything could be done with
	// a single trigger class and different targets, triggered effects
	// could not be client side predicted (push and teleport).
	{"trigger_always", SP_trigger_always},
	{"trigger_multiple", SP_trigger_multiple},
	{"trigger_push", SP_trigger_push},
	{"trigger_teleport", SP_trigger_teleport},
	{"trigger_hurt", SP_trigger_hurt},
	{"trigger_death", SP_trigger_death},
	{"trigger_frag", SP_trigger_frag},
	{"trigger_lock", SP_trigger_lock},

	// targets perform no action by themselves, but must be triggered
	// by another entity
	{"target_give", SP_target_give},
	{"target_remove_powerups", SP_target_remove_powerups},
	{"target_delay", SP_target_delay},
	{"target_speaker", SP_target_speaker},
	{"target_print", SP_target_print},
	{"target_laser", SP_target_laser},
	{"target_score", SP_target_score},
	{"target_clienttarg", SP_target_clienttarg},
	
	{"target_teleporter", SP_target_teleporter},
	{"target_relay", SP_target_relay},
	{"target_kill", SP_target_kill},
	{"target_position", SP_target_position},
	{"target_location", SP_target_location},
	{"target_push", SP_target_push},
	{"target_logic", SP_target_logic},
	{"target_gravity", SP_target_gravity},
	{"target_mapchange", SP_target_mapchange},
	{"target_botspawn", SP_target_botspawn},
	{"target_unlink", SP_target_unlink},
	{"target_disable", SP_target_unlink},
	{"target_debrisemitter", SP_target_debrisemitter},
	{"target_objective", SP_target_objective},
	{"target_skill", SP_target_skill},
	{"target_earthquake", SP_target_earthquake},
	{"target_effect", SP_target_effect},
	{"target_finish", SP_target_finish},
	{"target_modify", SP_target_modify},
	{"target_secret", SP_target_secret},
	{"target_playerstats", SP_target_playerstats},
	{"target_cutscene", SP_target_cutscene},
	{"target_botremove", SP_target_botremove},
	{"target_music", SP_target_music},
	{"target_stats", SP_target_stats},
	
	{"script_variable", SP_script_variable},
	{"script_layer", SP_script_layer},
	{"script_cmd", SP_script_cmd},
	{"script_menu", SP_script_menu},
	{"script_aicontrol", SP_script_aicontrol},

	{"light", SP_light},
	{"path_corner", SP_path_corner},

	{"misc_teleporter_dest", SP_misc_teleporter_dest},
	{"misc_model", SP_misc_model},
	{"misc_portal_surface", SP_misc_portal_surface},
	{"misc_portal_camera", SP_misc_portal_camera},

	{"shooter_rocket", SP_shooter_rocket},
	{"shooter_grenade", SP_shooter_grenade},
	{"shooter_plasma", SP_shooter_plasma},
	{"shooter_bfg", SP_shooter_bfg},
	{"shooter_prox", SP_shooter_prox},
	{"shooter_flame", SP_shooter_flame},
	{"shooter_antimatter", SP_shooter_antimatter},
	{"shooter_custom", SP_shooter_custom},

	{"team_CTF_redplayer", SP_team_CTF_redplayer},
	{"team_CTF_blueplayer", SP_team_CTF_blueplayer},

	{"team_CTF_redspawn", SP_team_CTF_redspawn},
	{"team_CTF_bluespawn", SP_team_CTF_bluespawn},

	{"func_door_rotating", SP_func_door_rotating},

	{"team_redobelisk", SP_team_redobelisk},
	{"team_blueobelisk", SP_team_blueobelisk},
	{"team_neutralobelisk", SP_team_neutralobelisk},

	{"item_botroam", SP_item_botroam},

	{"environment_rain", SP_rally_weather_rain},
	{"environment_snow", SP_rally_weather_snow},

	{NULL, 0}
};

/*
*****************
G_WLF_GetLeft
*****************
*/
void G_WLK_GetLeft(const char *pszSource, char *pszDest,  int iLen)
{
	int iSize; //size of string we need to copy

	//see how much space we need
	iSize = strlen(pszSource);
	
	//is len less that size?
	if(iLen < iSize)
		iSize = iLen;

	//make a copy of the string
	strcpy(pszDest, pszSource);

	//end the string at iSize
	pszDest[iSize] = '\0';
}

/*
===============
G_CallSpawn

Finds the spawn function for the entity and calls it,
returning qfalse if not found
===============
*/
qboolean G_CallSpawn( gentity_t *ent ) {
	spawn_t	*s;
	gitem_t	*item;
    char cvarname[128];
    char itemname[128];
	
	if( strcmp(ent->classname, "none") == 0 ) {
	return qfalse;
	}

        //Construct a replace cvar:
		Com_sprintf(cvarname, sizeof(cvarname), "replace_%s", ent->classname);

        //Look an alternative item up:
        trap_Cvar_VariableStringBuffer(cvarname,itemname,sizeof(itemname));
        if(itemname[0]==0) //If nothing found use original
            Com_sprintf(itemname, sizeof(itemname), "%s", ent->classname);
        else
            G_Printf ("%s replaced by %s\n", ent->classname, itemname);

		if( g_gametype.integer == GT_OBELISK ) {
			if( strcmp(itemname, "team_CTF_redflag") == 0 ) {
				Com_sprintf(itemname, sizeof(itemname), "%s", "team_redobelisk");
			}
			if( strcmp(itemname, "team_CTF_blueflag") == 0 ) {
				Com_sprintf(itemname, sizeof(itemname), "%s", "team_blueobelisk");
			}
		}
		if( g_gametype.integer == GT_HARVESTER ) {
			if( strcmp(itemname, "team_CTF_redflag") == 0 ) {
				Com_sprintf(itemname, sizeof(itemname), "%s", "team_redobelisk");
			}
			if( strcmp(itemname, "team_CTF_blueflag") == 0 ) {
				Com_sprintf(itemname, sizeof(itemname), "%s", "team_blueobelisk");
			}
			if( strcmp(itemname, "team_CTF_neutralflag") == 0 ) {
				Com_sprintf(itemname, sizeof(itemname), "%s", "team_neutralobelisk");
			}
		}	
		if( g_gametype.integer == GT_ELIMINATION || g_gametype.integer == GT_FFA || g_gametype.integer == GT_SANDBOX || g_gametype.integer == GT_TEAM || g_gametype.integer == GT_LMS || g_gametype.integer == GT_DOMINATION ) {
			if( strcmp(itemname, "team_CTF_redplayer") == 0 ) {
				Com_sprintf(itemname, sizeof(itemname), "%s", "info_player_deathmatch");
			}
			if( strcmp(itemname, "team_CTF_blueplayer") == 0 ) {
				Com_sprintf(itemname, sizeof(itemname), "%s", "info_player_deathmatch");
			}
			if( strcmp(itemname, "team_CTF_redspawn") == 0 ) {
				Com_sprintf(itemname, sizeof(itemname), "%s", "info_player_deathmatch");
			}
			if( strcmp(itemname, "team_CTF_bluespawn") == 0 ) {
				Com_sprintf(itemname, sizeof(itemname), "%s", "info_player_deathmatch");
			}
			if( strcmp(itemname, "team_CTF_blueflag") == 0 ) {
				Com_sprintf(itemname, sizeof(itemname), "%s", "holdable_portal");
			}
			if( strcmp(itemname, "team_CTF_redflag") == 0 ) {
				Com_sprintf(itemname, sizeof(itemname), "%s", "item_armor_full");
			}
		}
		if(g_gametype.integer == GT_DOUBLE_D) {
			if( strcmp(itemname, "team_CTF_redplayer") == 0 ) {
				Com_sprintf(itemname, sizeof(itemname), "%s", "info_player_deathmatch");
			}
			if( strcmp(itemname, "team_CTF_blueplayer") == 0 ) {
				Com_sprintf(itemname, sizeof(itemname), "%s", "info_player_deathmatch");
			}
			if( strcmp(itemname, "team_CTF_redspawn") == 0 ) {
				Com_sprintf(itemname, sizeof(itemname), "%s", "info_player_deathmatch");
			}
			if( strcmp(itemname, "team_CTF_bluespawn") == 0 ) {
				Com_sprintf(itemname, sizeof(itemname), "%s", "info_player_deathmatch");
			}
		}		


	if ( itemname[0]==0) {
                G_Printf ("G_CallSpawn: NULL classname\n");
		return qfalse;
	}

    if(g_randomItems.integer) {
		if( 
		strcmp(itemname, "team_CTF_redflag") || 
		strcmp(itemname, "team_CTF_blueflag") || 
		strcmp(itemname, "team_CTF_neutralflag") || 
		strcmp(itemname, "info_player_dd") || 
		strcmp(itemname, "info_player_dd_red") || 
		strcmp(itemname, "info_player_dd_blue") ) {
		randomiseitem(ent);
		}
	}

	// check item spawn functions
	for ( item=bg_itemlist+1 ; item->classname ; item++ ) {
		if ( !strcmp(item->classname, itemname) ) {
			G_SpawnItem( ent, item );
			return qtrue;
		}
	}

	// check normal spawn functions
	for ( s=spawns ; s->name ; s++ ) {
		if ( !strcmp(s->name, itemname) ) {
			// found it
			s->spawn(ent);
			return qtrue;
		}
	}
        G_Printf ("%s doesn't have a spawn function\n", itemname);
	return qfalse;
}

/*
===============
G_SandboxSpawn

Finds the spawn function for the entity and calls it,
returning qfalse if not found
===============
*/
qboolean G_SandboxSpawn( gentity_t *ent ) {
	spawn_t	*s;
	gitem_t	*item;

	// check item spawn functions
	for ( item=bg_itemlist+1 ; item->classname ; item++ ) {
		if ( !strcmp(item->classname, ent->classname) ) {
			G_SpawnItem( ent, item );
			return qtrue;
		}
	}

	// check normal spawn functions
	for ( s=spawns ; s->name ; s++ ) {
		if ( !strcmp(s->name, ent->classname) ) {
			// found it
			ent->sb_class = BG_Alloc(sizeof(ent->classname));
			strcpy(ent->sb_class, ent->classname);
			s->spawn(ent);
			ent->classname = "func_prop";
			return qtrue;
		}
	}
	return qfalse;
}

/*
=============
G_NewString

Builds a copy of the string, translating \n to real linefeeds
so message texts can be multi-line
=============
*/
char *G_NewString( const char *string ) {
	char	*newb, *new_p;
	int		i,l;
	
	l = strlen(string) + 1;
    //KK-OAX Changed to Tremulous's BG_Alloc
	newb = BG_Alloc( l );

	new_p = newb;

	// turn \n into a real linefeed
	for ( i=0 ; i< l ; i++ ) {
		if (string[i] == '\\' && i < l-1) {
			i++;
			if (string[i] == 'n') {
				*new_p++ = '\n';
			} else {
				*new_p++ = '\\';
			}
		} else {
			*new_p++ = string[i];
		}
	}
	
	return newb;
}




/*
===============
G_ParseField

Takes a key/value pair and sets the binary values
in a gentity
===============
*/
void G_ParseField( const char *key, const char *value, gentity_t *ent ) {
	field_t	*f;
	byte	*b;
	float	v;
	vec3_t	vec;

	for ( f=fields ; f->name ; f++ ) {
		if ( !Q_stricmp(f->name, key) ) {
			// found it
			b = (byte *)ent;

			switch( f->type ) {
			case F_LSTRING:
				*(char **)(b+f->ofs) = G_NewString (value);
				break;
			case F_VECTOR:
				sscanf (value, "%f %f %f", &vec[0], &vec[1], &vec[2]);
				((float *)(b+f->ofs))[0] = vec[0];
				((float *)(b+f->ofs))[1] = vec[1];
				((float *)(b+f->ofs))[2] = vec[2];
				break;
			case F_INT:
				*(int *)(b+f->ofs) = atoi(value);
				break;
			case F_FLOAT:
				*(float *)(b+f->ofs) = atof(value);
				break;
			case F_ANGLEHACK:
				v = atof(value);
				((float *)(b+f->ofs))[0] = 0;
				((float *)(b+f->ofs))[1] = v;
				((float *)(b+f->ofs))[2] = 0;
				break;
			default:
			case F_IGNORE:
				break;
			}
			return;
		}
	}
}




/*
===================
G_SpawnGEntityFromSpawnVars

Spawn an entity and fill in all of the level fields from
level.spawnVars[], then call the class specfic spawn function
===================
*/
void G_SpawnGEntityFromSpawnVars( void ) {
	int			i;
	gentity_t	*ent;
	char		*s, *value, *gametypeName;
	static char *gametypeNames[] = {"sandbox", "ffa", "single", "tournament", "team", "ctf", "oneflag", "obelisk", "harvester", "elimination", "ctf", "lms", "dd", "dom"};

	// get the next free entity
	ent = G_Spawn();

	for ( i = 0 ; i < level.numSpawnVars ; i++ ) {
		G_ParseField( level.spawnVars[i][0], level.spawnVars[i][1], ent );
	}

	// check for "notsingle" flag
	if ( g_gametype.integer == GT_SINGLE ) {
		G_SpawnInt( "notsingle", "0", &i );
		if ( i ) {
			G_FreeEntity( ent );
			return;
		}
	}
	if ( g_gametype.integer >= GT_TEAM && !g_ffa_gt ) {
		G_SpawnInt( "notteam", "0", &i );
		if ( i ) {
			G_FreeEntity( ent );
			return;
		}
	} else {
		G_SpawnInt( "notfree", "0", &i );
		if ( i ) {
			G_FreeEntity( ent );
			return;
		}
	}

	G_SpawnInt( "notta", "0", &i );
	if ( i ) {
		G_FreeEntity( ent );
		return;
	}

        if( G_SpawnString( "!gametype", NULL, &value ) ) {
		if( g_gametype.integer >= GT_SANDBOX && g_gametype.integer < GT_MAX_GAME_TYPE ) {
			gametypeName = gametypeNames[g_gametype.integer];

			s = strstr( value, gametypeName );
			if( s ) {
				G_FreeEntity( ent );
				return;
			}
		}
	}

	if( G_SpawnString( "gametype", NULL, &value ) ) {
		if( g_gametype.integer >= GT_SANDBOX && g_gametype.integer < GT_MAX_GAME_TYPE ) {
			gametypeName = gametypeNames[g_gametype.integer];

			s = strstr( value, gametypeName );
			if( !s ) {
				G_FreeEntity( ent );
				return;
			}
		}
	}

	// move editor origin to pos
	VectorCopy( ent->s.origin, ent->s.pos.trBase );
	VectorCopy( ent->s.origin, ent->r.currentOrigin );

	// if we didn't get a classname, don't bother spawning anything
	if ( !G_CallSpawn( ent ) ) {
		G_FreeEntity( ent );
	}
}



/*
====================
G_AddSpawnVarToken
====================
*/
char *G_AddSpawnVarToken( const char *string ) {
	int		l;
	char	*dest;

	l = strlen( string );
	if ( level.numSpawnVarChars + l + 1 > MAX_SPAWN_VARS_CHARS ) {
		G_Error( "G_AddSpawnVarToken: MAX_SPAWN_VARS" );
	}

	dest = level.spawnVarChars + level.numSpawnVarChars;
	memcpy( dest, string, l+1 );

	level.numSpawnVarChars += l + 1;

	return dest;
}

/*
====================
G_ParseSpawnVars

Parses a brace bounded set of key / value pairs out of the
level's entity strings into level.spawnVars[]

This does not actually spawn an entity.
====================
*/
qboolean G_ParseSpawnVars( void ) {
	char		keyname[MAX_TOKEN_CHARS];
	char		com_token[MAX_TOKEN_CHARS];

	level.numSpawnVars = 0;
	level.numSpawnVarChars = 0;

	// parse the opening brace
	if ( !trap_GetEntityToken( com_token, sizeof( com_token ) ) ) {
		// end of spawn string
		return qfalse;
	}
	if ( com_token[0] != '{' ) {
		G_Error( "G_ParseSpawnVars: found %s when expecting {",com_token );
	}

	// go through all the key / value pairs
	while ( 1 ) {	
		// parse key
		if ( !trap_GetEntityToken( keyname, sizeof( keyname ) ) ) {
			G_Error( "G_ParseSpawnVars: EOF without closing brace" );
		}

		if ( keyname[0] == '}' ) {
			break;
		}
		
		// parse value	
		if ( !trap_GetEntityToken( com_token, sizeof( com_token ) ) ) {
			G_Error( "G_ParseSpawnVars: EOF without closing brace" );
		}

		if ( com_token[0] == '}' ) {
			G_Error( "G_ParseSpawnVars: closing brace without data" );
		}
		if ( level.numSpawnVars == MAX_SPAWN_VARS ) {
			G_Error( "G_ParseSpawnVars: MAX_SPAWN_VARS" );
		}
		level.spawnVars[ level.numSpawnVars ][0] = G_AddSpawnVarToken( keyname );
		level.spawnVars[ level.numSpawnVars ][1] = G_AddSpawnVarToken( com_token );
		level.numSpawnVars++;
	}

	return qtrue;
}



/*QUAKED worldspawn (0 0 0) ?

Every map should have exactly one worldspawn.
"music"		music wav file
"gravity"	800 is default gravity
"message"	Text to print during connection process
*/
void SP_worldspawn( void ) {
	char	*s;
	char	*music;
	int		 number;
	
	number = rand() % 14 + 1;
	
	if(number == 1){ music = "music/soundtrack1"; }
	if(number == 2){ music = "music/soundtrack2"; }
	if(number == 3){ music = "music/soundtrack3"; }
	if(number == 4){ music = "music/soundtrack4"; }
	if(number == 5){ music = "music/soundtrack5"; }
	if(number == 6){ music = "music/soundtrack6"; }
	if(number == 7){ music = "music/soundtrack7"; }
	if(number == 8){ music = "music/soundtrack8"; }
	if(number == 9){ music = "music/soundtrack9"; }
	if(number == 10){ music = "music/soundtrack10"; }
	if(number == 11){ music = "music/soundtrack11"; }
	if(number == 12){ music = "music/soundtrack12"; }
	if(number == 13){ music = "music/soundtrack13"; }
	if(number == 14){ music = "music/soundtrack14"; }


	G_SpawnString( "classname", "", &s );
	if ( Q_stricmp( s, "worldspawn" ) ) {
		G_Error( "SP_worldspawn: The first entity isn't 'worldspawn'" );
	}

	// make some data visible to connecting client
	trap_SetConfigstring( CS_GAME_VERSION, GAME_VERSION );

	trap_SetConfigstring( CS_LEVEL_START_TIME, va("%i", level.startTime ) );
	

	if ( *g_music.string && Q_stricmp( g_music.string, "none" ) ) {
		trap_SetConfigstring( CS_MUSIC, g_music.string );
	} else {
		G_SpawnString( "music", music, &s );   
		trap_SetConfigstring( CS_MUSIC, s );
	}
    	G_SpawnString( "scoreboardmusic", "", &s );
	trap_SetConfigstring( CS_SCOREBOARDMUSIC, s );

	G_SpawnString( "deathmusic", "", &s );
	trap_SetConfigstring( CS_DEATHMUSIC, s );

	G_SpawnString( "playermodel", "", &s );
	trap_SetConfigstring( CS_PLAYERMODEL, s );

	G_SpawnString( "playerheadmodel", "", &s );
	trap_SetConfigstring( CS_PLAYERHEADMODEL, s );

	G_SpawnString( "objectivesoverlay", "menu/objectives/overlay.tga", &s );
	trap_SetConfigstring( CS_OBJECTIVESOVERLAY, s );

	G_SpawnString( "message", "", &s );
	trap_SetConfigstring( CS_MESSAGE, s );				// map specific message

	trap_SetConfigstring( CS_MOTD, g_motd.string );		// message of the day

	G_SpawnString( "gravity", "800", &s );
	trap_Cvar_Set( "g_gravity", s );

	G_SpawnString( "enableDust", "0", &s );
	trap_Cvar_Set( "g_enableDust", s );
    
    G_SpawnString( "enableSnow", "0", &s );
    trap_Cvar_Set( "g_enableSnow", s );
    
	G_SpawnString( "enableBreath", "0", &s );
	trap_Cvar_Set( "g_enableBreath", s );

	g_entities[ENTITYNUM_WORLD].s.number = ENTITYNUM_WORLD;
        g_entities[ENTITYNUM_WORLD].r.ownerNum = ENTITYNUM_NONE;
	g_entities[ENTITYNUM_WORLD].classname = "worldspawn";

        g_entities[ENTITYNUM_NONE].s.number = ENTITYNUM_NONE;
        g_entities[ENTITYNUM_NONE].r.ownerNum = ENTITYNUM_NONE;
        g_entities[ENTITYNUM_NONE].classname = "nothing";
        
	// see if we want a warmup time
	trap_SetConfigstring( CS_WARMUP, "" );
	if ( g_restarted.integer ) {
		trap_Cvar_Set( "g_restarted", "0" );
		level.warmupTime = 0;
	} else if ( g_doWarmup.integer ) { // Turn it on
		level.warmupTime = -1;
		trap_SetConfigstring( CS_WARMUP, va("%i", level.warmupTime) );
		G_LogPrintf( "Warmup:\n" );
	}

}


/*
==============
G_SpawnEntitiesFromString

Parses textual entity definitions out of an entstring and spawns gentities.
==============
*/
void G_SpawnEntitiesFromString( void ) {
	// allow calls to G_Spawn*()
	level.spawning = qtrue;
	level.numSpawnVars = 0;

	// the worldspawn is not an actual entity, but it still
	// has a "spawn" function to perform any global setup
	// needed by a level (setting configstrings or cvars, etc)
	if ( !G_ParseSpawnVars() ) {
		G_Error( "SpawnEntities: no entities" );
	}
	SP_worldspawn();

	// parse ents
	while( G_ParseSpawnVars() ) {
		G_SpawnGEntityFromSpawnVars();
	}	

  G_LevelLoadComplete();

	level.spawning = qfalse;			// any future calls to G_Spawn*() will be errors
}

