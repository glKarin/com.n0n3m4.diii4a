/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "sys/platform.h"
#include "idlib/LangDict.h"
#include "framework/async/NetworkSystem.h"
#include "framework/FileSystem.h"

#include "gamesys/TypeInfo.h"
#include "gamesys/SysCvar.h"
#include "script/Script_Thread.h"
#include "ai/AI.h"
#include "anim/Anim_Testmodel.h"
#include "Entity.h"
#include "Moveable.h"
#include "WorldSpawn.h"
#include "Fx.h"
#include "Misc.h"

#include "framework/DeclEntityDef.h"
#include "bc_camerasplice.h"
#include "bc_cryointerior.h"
#include "bc_keypad.h"
#include "bc_maintpanel.h"
#include "bc_tablet.h"
#include "bc_notewall.h"
#include "SecurityCamera.h"
#include "bc_catcage.h"
#include "bc_glasspiece.h"
#include "bc_meta.h"
#include "bc_pirateship.h"
#include "bc_vrvisor.h"
#include "renderer/tr_local.h"

#include "SysCmds.h"

/*
==================
Cmd_GetFloatArg
==================
*/
float Cmd_GetFloatArg( const idCmdArgs &args, int &argNum ) {
	const char *value;

	value = args.Argv( argNum++ );
	return atof( value );
}

/*
===================
Cmd_EntityList_f
===================
*/
void Cmd_EntityList_f( const idCmdArgs &args ) {
	int			e;
	idEntity	*check;
	int			count;
	size_t		size;
	idStr		match;

	if ( args.Argc() > 1 ) {
		match = args.Args();
		match.Replace( " ", "" );
	} else {
		match = "";
	}

	count = 0;
	size = 0;

	gameLocal.Printf( "%-4s  %-20s %-20s %s\n", " Num", "EntityDef", "Class", "Name" );
	gameLocal.Printf( "--------------------------------------------------------------------\n" );
	for( e = 0; e < MAX_GENTITIES; e++ ) {
		check = gameLocal.entities[ e ];

		if ( !check ) {
			continue;
		}

		if ( !check->name.Filter( match, true ) ) {
			continue;
		}

		gameLocal.Printf( "%4i: %-20s %-20s %s\n", e,
			check->GetEntityDefName(), check->GetClassname(), check->name.c_str() );

		count++;
		size += check->spawnArgs.Allocated();
	}

	gameLocal.Printf( "...%d entities\n...%zd bytes of spawnargs\n", count, size );
}

/*
===================
Cmd_ActiveEntityList_f
===================
*/
void Cmd_ActiveEntityList_f( const idCmdArgs &args ) {
	idEntity	*check;
	int			count;

	count = 0;

	gameLocal.Printf( "%-4s  %-20s %-20s %s\n", " Num", "EntityDef", "Class", "Name" );
	gameLocal.Printf( "--------------------------------------------------------------------\n" );
	for( check = gameLocal.activeEntities.Next(); check != NULL; check = check->activeNode.Next() ) {
		char	dormant = check->fl.isDormant ? '-' : ' ';
		gameLocal.Printf( "%4i:%c%-20s %-20s %s\n", check->entityNumber, dormant, check->GetEntityDefName(), check->GetClassname(), check->name.c_str() );
		count++;
	}

	gameLocal.Printf( "...%d active entities\n", count );
}

/*
===================
Cmd_ListSpawnArgs_f
===================
*/
void Cmd_ListSpawnArgs_f( const idCmdArgs &args ) {
	int i;
	idEntity *ent;

	ent = gameLocal.FindEntity( args.Argv( 1 ) );
	if ( !ent ) {
		gameLocal.Printf( "entity not found\n" );
		return;
	}

	for ( i = 0; i < ent->spawnArgs.GetNumKeyVals(); i++ ) {
		const idKeyValue *kv = ent->spawnArgs.GetKeyVal( i );
		gameLocal.Printf( "\"%s\"  " S_COLOR_WHITE "\"%s\"\n", kv->GetKey().c_str(), kv->GetValue().c_str() );
	}
}

/*
===================
Cmd_ReloadScript_f
===================
*/
void Cmd_ReloadScript_f(const idCmdArgs &args) {
	//blendo eric: append reload command
	const char * mapName = gameLocal.GetMapName();
	if (mapName && (strlen(mapName) > 0)) {
		idCmdArgs rl_args;
		rl_args.AppendArg("map");
		rl_args.AppendArg(idStr(mapName).StripPath());
		cmdSystem->BufferCommandArgs(CMD_EXEC_APPEND, rl_args);
	}

	// shutdown the map because entities may point to script objects
	gameLocal.MapShutdown();

	// recompile the scripts
	gameLocal.program.Startup(SCRIPT_DEFAULT);

#ifdef _D3XP
	// loads a game specific main script file
	idStr gamedir;
	int i;
	for (i = 0; i < 2; i++) {
		if (i == 0) {
			gamedir = cvarSystem->GetCVarString("fs_game_base");
		}
		else if (i == 1) {
			gamedir = cvarSystem->GetCVarString("fs_game");
		}
		if (gamedir.Length() > 0) {
			idStr scriptFile = va("script/%s_main.script", gamedir.c_str());
			if (fileSystem->ReadFile(scriptFile.c_str(), NULL) > 0) {
				gameLocal.program.CompileFile(scriptFile.c_str());
				gameLocal.program.FinishCompilation();
			}
		}
	}
#endif

	// error out so that the user can rerun the scripts
	gameLocal.Error("Exiting map to reload scripts");
}

/*
===================
Cmd_Script_f
===================
*/
void Cmd_Script_f( const idCmdArgs &args ) {
	const char *	script;
	idStr			text;
	idStr			funcname;
	static int		funccount = 0;
	idThread *		thread;
	const function_t *func;
	idEntity		*ent;

	if ( !gameLocal.CheatsOk() ) {
		return;
	}

	sprintf( funcname, "ConsoleFunction_%d", funccount++ );

	script = args.Args();
	sprintf( text, "void %s() {%s;}\n", funcname.c_str(), script );
	if ( gameLocal.program.CompileText( "console", text, true ) ) {
		func = gameLocal.program.FindFunction( funcname );
		if ( func ) {
			// set all the entity names in case the user named one in the script that wasn't referenced in the default script
			for( ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() ) {
				gameLocal.program.SetEntity( ent->name, ent );
			}

			thread = new idThread( func );
			thread->Start();
		}
	}
}

/*
==================
KillEntities

Kills all the entities of the given class in a level.
==================
*/
int KillEntities( const idCmdArgs &args, const idTypeInfo &superClass ) {
	idEntity	*ent;
	idStrList	ignore;
	const char *name;
	int			i;

	if ( !gameLocal.GetLocalPlayer() || !gameLocal.CheatsOk( false ) ) {
		return 0;
	}

	for( i = 1; i < args.Argc(); i++ ) {
		name = args.Argv( i );
		ignore.Append( name );
	}

	int counter = 0;

	for( ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() ) {
		if ( ent->IsType( superClass ) ) {
			for( i = 0; i < ignore.Num(); i++ ) {
				if ( ignore[ i ] == ent->name ) {
					break;
				}
			}

			if ( i >= ignore.Num() )
			{
				counter++;
				ent->PostEventMS( &EV_Remove, 0 );
			}
		}
	}
	
	return counter;
}

/*
==================
Cmd_KillMonsters_f

Kills all the monsters in a level.
==================
*/
void Cmd_KillMonsters_f( const idCmdArgs &args ) {

	int counter = 0;

	counter += KillEntities( args, idAI::Type );

	// kill any projectiles as well since they have pointers to the monster that created them
	counter += KillEntities( args, idProjectile::Type );

	common->Printf("Removed %d monsters.\n", counter);
}


//BC
void Cmd_KillPlayerLight_f(const idCmdArgs &args) {
	
	idEntity	*ent;
	for (ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next())
	{
		if (ent->IsType(idLight::Type))
		{
			if (ent->GetBindMaster() == gameLocal.GetLocalPlayer())
			{
				ent->PostEventMS(&EV_Remove, 0);
			}
		}
	}
}

void Cmd_DamageAll_f(const idCmdArgs &args)
{
	idEntity	*ent;
	int			entitiesDamaged = 0;
	for (ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next())
	{
		if (!ent)
			continue;

		if (!ent->fl.takedamage || ent->health <= 0 || ent->IsType(idActor::Type) || ent->IsHidden())
			continue;		

		if (ent->IsType(idGlassPiece::Type))
			continue;

		ent->Damage(NULL, NULL, vec3_zero, "damage_generic", 1.0f, 0);
		entitiesDamaged++;
	}

	common->Printf("Damaged %d entities.\n", entitiesDamaged);
}


/*
==================
Cmd_KillMovables_f

Kills all the moveables in a level.
==================
*/
void Cmd_KillMovables_f( const idCmdArgs &args ) {
	if ( !gameLocal.GetLocalPlayer() || !gameLocal.CheatsOk( false ) ) {
		return;
	}
	KillEntities( args, idMoveable::Type );
}

/*
==================
Cmd_KillRagdolls_f

Kills all the ragdolls in a level.
==================
*/
void Cmd_KillRagdolls_f( const idCmdArgs &args ) {
	if ( !gameLocal.GetLocalPlayer() || !gameLocal.CheatsOk( false ) ) {
		return;
	}
	KillEntities( args, idAFEntity_Generic::Type );
	KillEntities( args, idAFEntity_WithAttachedHead::Type );
}

/*
==================
Cmd_Give_f

Give items to a client
==================
*/
void Cmd_Give_f( const idCmdArgs &args ) {
	const char *name;
	int			i;
	bool		give_all;
	idPlayer	*player;

	player = gameLocal.GetLocalPlayer();
	if ( !player || !gameLocal.CheatsOk() ) {
		return;
	}

	name = args.Argv( 1 );

	if ( idStr::Icmp( name, "all" ) == 0 ) {
		give_all = true;
	} else {
		give_all = false;
	}

	if ( give_all || ( idStr::Cmpn( name, "weapon", 6 ) == 0 ) ) {
		if ( gameLocal.world->spawnArgs.GetBool( "no_Weapons" ) ) {
			gameLocal.world->spawnArgs.SetBool( "no_Weapons", false );
			for( i = 0; i < gameLocal.numClients; i++ ) {
				if ( gameLocal.entities[ i ] ) {
					gameLocal.entities[ i ]->PostEventSec( &EV_Player_SelectWeapon, 0.5f, gameLocal.entities[ i ]->spawnArgs.GetString( "def_weapon1" ) );
				}
			}
		}
	}

	if ( ( idStr::Cmpn( name, "weapon_", 7 ) == 0 ) || ( idStr::Cmpn( name, "item_", 5 ) == 0 ) || ( idStr::Cmpn( name, "ammo_", 5 ) == 0 ) ) {
		player->GiveItem( name );
		return;
	}

	if ( give_all || idStr::Icmp( name, "health" ) == 0 )	{
		player->health = player->inventory.maxHealth;
		if ( !give_all ) {
			return;
		}
	}

	if (idStr::Icmp(name, "mushroom") == 0)
	{
		player->AddBloodMushroom(1, vec3_zero);
		common->Printf("Added 1 mushroom.\n");
		return;
	}

	if (idStr::Icmp(name, "bloodbag") == 0)
	{
		player->inventory.bloodbags++;
		common->Printf("Added 1 bloodbag.\n");
		return;
	}


	// SM: Because of the way hotbar slots work, this type of give all doesn't really work
// 	if ( give_all || idStr::Icmp( name, "weapons" ) == 0 ) {
// 		player->inventory.weapons = 0xffffffff >> ( 32 - MAX_WEAPONS );
// 		player->CacheWeapons();
// 
// 		if ( !give_all ) {
// 			return;
// 		}
// 	}

	if ( give_all || idStr::Icmp( name, "ammo" ) == 0 ) {
		for ( i = 0 ; i < AMMO_NUMTYPES; i++ ) {
			player->inventory.ammo[ i ] = player->inventory.MaxAmmoForAmmoClass( player, idWeapon::GetAmmoNameForNum( ( ammo_t )i ) );
		}
		if ( !give_all ) {
			return;
		}
	}

	if ( give_all || idStr::Icmp( name, "armor" ) == 0 ) {
		player->inventory.armor = player->inventory.maxarmor;
		if ( !give_all ) {
			return;
		}
	}

	if ( idStr::Icmp( name, "berserk" ) == 0 ) {
		player->GivePowerUp( BERSERK, SEC2MS( 30.0f ) );
		return;
	}

	if ( idStr::Icmp( name, "invis" ) == 0 ) {
		player->GivePowerUp( INVISIBILITY, SEC2MS( 30.0f ) );
		return;
	}

#ifdef _D3XP
	if ( idStr::Icmp( name, "invulnerability" ) == 0 ) {
		if ( args.Argc() > 2 ) {
			player->GivePowerUp( INVULNERABILITY, atoi( args.Argv( 2 ) ) );
		}
		else {
			player->GivePowerUp( INVULNERABILITY, 30000 );
		}
		return;
	}

	if ( idStr::Icmp( name, "helltime" ) == 0 ) {
		if ( args.Argc() > 2 ) {
			player->GivePowerUp( HELLTIME, atoi( args.Argv( 2 ) ) );
		}
		else {
			player->GivePowerUp( HELLTIME, 30000 );
		}
		return;
	}

	if ( idStr::Icmp( name, "envirosuit" ) == 0 ) {
		if ( args.Argc() > 2 ) {
			player->GivePowerUp( ENVIROSUIT, atoi( args.Argv( 2 ) ) );
		}
		else {
			player->GivePowerUp( ENVIROSUIT, 30000 );
		}
		return;
	}
#endif
	if ( idStr::Icmp( name, "pda" ) == 0 ) {
		player->GivePDA( args.Argv(2), NULL );
		return;
	}

	if ( idStr::Icmp( name, "video" ) == 0 ) {
		player->GiveVideo( args.Argv(2), NULL );
		return;
	}

	if ( !give_all && !player->Give( args.Argv(1), args.Argv(2), 0 ) ) {
		gameLocal.Printf( "unknown item\n" );
	}
}

/*
==================
Cmd_CenterView_f

Centers the players pitch
==================
*/
void Cmd_CenterView_f( const idCmdArgs &args ) {
	idPlayer	*player;
	idAngles	ang;

	player = gameLocal.GetLocalPlayer();
	if ( !player ) {
		return;
	}

	ang = player->viewAngles;
	ang.pitch = 0.0f;
	player->SetViewAngles( ang );
}

//BC
void Cmd_YawView_f(const idCmdArgs &args)
{
	idPlayer	*player;
	idAngles	ang;

	player = gameLocal.GetLocalPlayer();
	if (!player) {
		return;
	}

	if (args.Argc() <= 1)
	{
		common->Printf("Usage: snap yaw to specific angle.\n");
		return;
	}

	int yaw  = atoi(args.Argv(1));	

	ang = player->viewAngles;
	ang.pitch = 0.0f;
	ang.yaw = yaw;
	player->SetViewAngles(ang);
}


/*
==================
Cmd_God_f

Sets client to godmode

argv(0) god
==================
*/
void Cmd_God_f( const idCmdArgs &args ) {
	const char	*msg;
	idPlayer	*player;

	player = gameLocal.GetLocalPlayer();
	if ( !player || !gameLocal.CheatsOk() ) {
		return;
	}

	if ( player->godmode ) {
		player->godmode = false;
		msg = "godmode OFF\n";
	} else {
		player->godmode = true;
		msg = "godmode ON\n";
	}

	gameLocal.Printf( "%s", msg );
}

/*
==================
Cmd_Notarget_f

Sets client to notarget

argv(0) notarget
==================
*/
void Cmd_Notarget_f( const idCmdArgs &args ) {
	const char	*msg;
	idPlayer	*player;

	player = gameLocal.GetLocalPlayer();
	if ( !player || !gameLocal.CheatsOk() ) {
		return;
	}

	if ( player->fl.notarget ) {
		player->fl.notarget = false;
		msg = "notarget OFF\n";
	} else {
		player->fl.notarget = true;
		msg = "notarget ON\n";
	}

	gameLocal.Printf( "%s", msg );
}

/*
==================
Cmd_Noclip_f

argv(0) noclip
==================
*/
void Cmd_Noclip_f( const idCmdArgs &args ) {
	const char	*msg;
	idPlayer	*player;

	player = gameLocal.GetLocalPlayer();
	if ( !player || !gameLocal.CheatsOk() ) {
		return;
	}

	if ( player->noclip ) {
		msg = "noclip OFF\n";
	} else {
		msg = "noclip ON\n";
	}

	if (!player->noclip)
	{
		player->PerformImpulse(27); //BC unbind the player, if bound.
	}

	player->noclip = !player->noclip;

	//BC
	if (player->noclip && player->GetAirtics() <= 0)
	{
		//turn on noclip.
		player->StopSound(SND_CHANNEL_BODY2, false); //if suffocating, stop the suffocate sound.
	}

	gameLocal.Printf( "%s", msg );
}

/*
=================
Cmd_Kill_f
=================
*/
void Cmd_Kill_f( const idCmdArgs &args ) {
	idPlayer	*player;

	if ( gameLocal.isMultiplayer ) {
		if ( gameLocal.isClient ) {
			idBitMsg	outMsg;
			byte		msgBuf[ MAX_GAME_MESSAGE_SIZE ];
			outMsg.Init( msgBuf, sizeof( msgBuf ) );
			outMsg.WriteByte( GAME_RELIABLE_MESSAGE_KILL );
			networkSystem->ClientSendReliableMessage( outMsg );
		} else {
			player = gameLocal.GetClientByCmdArgs( args );
			if ( !player ) {
				common->Printf( "kill <client nickname> or kill <client index>\n" );
				return;
			}
			player->Kill( false, false );
			cmdSystem->BufferCommandText( CMD_EXEC_NOW, va( "say killed client %d '%s^0'\n", player->entityNumber, gameLocal.userInfo[ player->entityNumber ].GetString( "ui_name" ) ) );
		}
	} else {
		player = gameLocal.GetLocalPlayer();
		if ( !player ) {
			return;
		}
		player->Kill( false, false );
	}
}

/*
=================
Cmd_PlayerModel_f
=================
*/
void Cmd_PlayerModel_f( const idCmdArgs &args ) {
	idPlayer	*player;
	const char *name;
	idVec3		pos;
	idAngles	ang;

	player = gameLocal.GetLocalPlayer();
	if ( !player || !gameLocal.CheatsOk() ) {
		return;
	}

	if ( args.Argc() < 2 ) {
		gameLocal.Printf( "usage: playerModel <modelname>\n" );
		return;
	}

	name = args.Argv( 1 );
	player->spawnArgs.Set( "model", name );

	pos = player->GetPhysics()->GetOrigin();
	ang = player->viewAngles;
	player->SpawnToPoint( pos, ang );
}

/*
==================
Cmd_Say
==================
*/
static void Cmd_Say( bool team, const idCmdArgs &args ) {
	const char *name;
	idStr text;
	const char *cmd = team ? "sayTeam" : "say" ;

	if ( !gameLocal.isMultiplayer ) {
		gameLocal.Printf( "%s can only be used in a multiplayer game\n", cmd );
		return;
	}

	if ( args.Argc() < 2 ) {
		gameLocal.Printf( "usage: %s <text>\n", cmd );
		return;
	}

	text = args.Args();
	if ( text.Length() == 0 ) {
		return;
	}

	if ( text[ text.Length() - 1 ] == '\n' ) {
		text[ text.Length() - 1 ] = '\0';
	}
	name = "player";

	idPlayer *	player;

	// here we need to special case a listen server to use the real client name instead of "server"
	// "server" will only appear on a dedicated server
	if ( gameLocal.isClient || cvarSystem->GetCVarInteger( "net_serverDedicated" ) == 0 ) {
		player = gameLocal.localClientNum >= 0 ? static_cast<idPlayer *>( gameLocal.entities[ gameLocal.localClientNum ] ) : NULL;
		if ( player ) {
			name = player->GetUserInfo()->GetString( "ui_name", "player" );
		}

#ifdef CTF
		// Append the player's location to team chat messages in CTF
		if ( gameLocal.mpGame.IsGametypeFlagBased() && team && player ) {
			idLocationEntity *locationEntity = gameLocal.LocationForPoint( player->GetEyePosition() );

			if ( locationEntity ) {
				idStr temp = "[";
				temp += locationEntity->GetLocation();
				temp += "] ";
				temp += text;
				text = temp;
			}

		}
#endif


	} else {
		name = "server";
	}

	if ( gameLocal.isClient ) {
		idBitMsg	outMsg;
		byte		msgBuf[ 256 ];
		outMsg.Init( msgBuf, sizeof( msgBuf ) );
		outMsg.WriteByte( team ? GAME_RELIABLE_MESSAGE_TCHAT : GAME_RELIABLE_MESSAGE_CHAT );
		outMsg.WriteString( name );
		outMsg.WriteString( text, -1, false );
		networkSystem->ClientSendReliableMessage( outMsg );
	} else {
		gameLocal.mpGame.ProcessChatMessage( gameLocal.localClientNum, team, name, text, NULL );
	}
}

/*
==================
Cmd_Say_f
==================
*/
static void Cmd_Say_f( const idCmdArgs &args ) {
	Cmd_Say( false, args );
}

/*
==================
Cmd_SayTeam_f
==================
*/
static void Cmd_SayTeam_f( const idCmdArgs &args ) {
	Cmd_Say( true, args );
}

/*
==================
Cmd_AddChatLine_f
==================
*/
static void Cmd_AddChatLine_f( const idCmdArgs &args ) {
	gameLocal.mpGame.AddChatLine( args.Argv( 1 ) );
}

/*
==================
Cmd_Kick_f
==================
*/
static void Cmd_Kick_f( const idCmdArgs &args ) {
	idPlayer *player;

	if ( !gameLocal.isMultiplayer ) {
		gameLocal.Printf( "kick can only be used in a multiplayer game\n" );
		return;
	}

	if ( gameLocal.isClient ) {
		gameLocal.Printf( "You have no such power. This is a server command\n" );
		return;
	}

	player = gameLocal.GetClientByCmdArgs( args );
	if ( !player ) {
		gameLocal.Printf( "usage: kick <client nickname> or kick <client index>\n" );
		return;
	}
	cmdSystem->BufferCommandText( CMD_EXEC_NOW, va( "say kicking out client %d '%s^0'\n", player->entityNumber, gameLocal.userInfo[ player->entityNumber ].GetString( "ui_name" ) ) );
	cmdSystem->BufferCommandText( CMD_EXEC_NOW, va( "kick %d\n", player->entityNumber ) );
}

/*
==================
Cmd_GetViewpos_f
==================
*/
void Cmd_GetViewpos_f( const idCmdArgs &args ) {
	idPlayer	*player;
	idVec3		origin;
	idMat3		axis;

	player = gameLocal.GetLocalPlayer();
	if ( !player ) {
		gameLocal.Printf("<none>");
		return;
	}

	const renderView_t *view = player->GetRenderView();
	if ( view ) {
		gameLocal.Printf("%.1f %.1f %.1f    %.0f -%.0f\n", view->vieworg.x, view->vieworg.y, view->vieworg.z, view->viewaxis[0].ToYaw(), view->viewaxis[0].ToPitch());
	} else {
		player->GetViewPos( origin, axis );
		gameLocal.Printf("%.1f %.1f %.1f    %.0f -%.0f\n", origin.x, origin.y, origin.z, axis[0].ToYaw(), axis[0].ToPitch());
	}
}


/*
=================
Cmd_SetViewpos_f
=================
*/
void Cmd_SetViewpos_f( const idCmdArgs &args ) {
	idVec3		origin;
	idAngles	angles;
	int			i;
	idPlayer	*player;

	player = gameLocal.GetLocalPlayer();
	if ( !player || !gameLocal.CheatsOk() ) {
		return;
	}

	if ((args.Argc() > 6) || (args.Argc() <= 3))
	{
		gameLocal.Printf( "usage: setviewpos <x> <y> <z> <yaw> <pitch>\n" );
		return;
	}

	angles.Zero();
	if (args.Argc() >= 5) {
		angles.yaw = atof( args.Argv( 4 ) );
	}

	if (args.Argc() >= 6) {
		angles.pitch = atof(args.Argv(5));
	}

	for ( i = 0 ; i < 3 ; i++ ) {
		origin[i] = atof( args.Argv( i + 1 ) );
	}
	origin.z -= pm_normalviewheight.GetFloat() - 0.25f;

	player->Teleport( origin, angles, NULL );
}

/*
=================
Cmd_Teleport_f
=================
*/
void Cmd_Teleport_f( const idCmdArgs &args ) {
	idVec3		origin;
	idAngles	angles;
	idPlayer	*player;
	idEntity	*ent;

	player = gameLocal.GetLocalPlayer();
	if ( !player || !gameLocal.CheatsOk() ) {
		return;
	}

	if ( args.Argc() != 2 ) {
		gameLocal.Printf( "usage: teleport <name of entity to teleport to>\n" );
		return;
	}

	ent = gameLocal.FindEntity( args.Argv( 1 ) );
	if ( !ent ) {
		gameLocal.Printf( "entity not found\n" );
		return;
	}

	angles.Zero();
	angles.yaw = ent->GetPhysics()->GetAxis()[ 0 ].ToYaw();
	origin = ent->GetPhysics()->GetOrigin();

	player->Teleport( origin, angles, ent );
}

/*
=================
Cmd_Trigger_f
=================
*/
void Cmd_Trigger_f( const idCmdArgs &args ) {
	idVec3		origin;
	idAngles	angles;
	idPlayer	*player;
	idEntity	*ent;

	player = gameLocal.GetLocalPlayer();
	if ( !player || !gameLocal.CheatsOk() ) {
		return;
	}

	if ( args.Argc() != 2 ) {
		gameLocal.Printf( "usage: trigger <name of entity to trigger>\n" );
		return;
	}

	ent = gameLocal.FindEntity( args.Argv( 1 ) );
	if ( !ent ) {
		gameLocal.Printf( "entity not found\n" );
		return;
	}

	ent->Signal( SIG_TRIGGER );
	ent->ProcessEvent( &EV_Activate, player );
	ent->TriggerGuis();
}

/*
===================
Cmd_Spawn_f
===================
*/
void Cmd_Spawn_f( const idCmdArgs &args ) {
	const char *key, *value;
	int			i;
	float		yaw;
	idVec3		org;
	idPlayer	*player;
	idDict		dict;

	player = gameLocal.GetLocalPlayer();
	if ( !player || !gameLocal.CheatsOk( false ) ) {
		return;
	}

	if ( args.Argc() & 1 ) {	// must always have an even number of arguments
		gameLocal.Printf( "usage: spawn classname [key/value pairs]\n" );
		return;
	}

	yaw = player->viewAngles.yaw;

	value = args.Argv( 1 );
	dict.Set( "classname", value );
	dict.Set( "angle", va( "%f", yaw  ) );

	org = player->GetPhysics()->GetOrigin() + idAngles( 0, yaw, 0 ).ToForward() * 80 + idVec3( 0, 0, .3f );
	dict.Set( "origin", org.ToString() );

	for( i = 2; i < args.Argc() - 1; i += 2 ) {

		key = args.Argv( i );
		value = args.Argv( i + 1 );

		dict.Set( key, value );
	}

	idEntity *ent;
	gameLocal.SpawnEntityDef( dict, &ent );

	if (ent)
	{
		//if zMin isn't zero, we need to adjust the position so the item doesn't immediately clip through the ground.
		float zMin = ent->GetPhysics()->GetBounds().zMin();
		if (zMin < 0)
		{
			ent->GetPhysics()->SetOrigin(ent->GetPhysics()->GetOrigin() + idVec3(0, 0, idMath::Fabs(zMin)));
		}
	}
	else
	{
		common->Warning("failed to spawn entity.");
	}
}



/*
==================
Cmd_Damage_f

Damages the specified entity
==================
*/
void Cmd_Damage_f( const idCmdArgs &args ) {
	if ( !gameLocal.GetLocalPlayer() || !gameLocal.CheatsOk( false ) ) {
		return;
	}
	if ( args.Argc() != 3 ) {
		gameLocal.Printf( "usage: damage <name of entity to damage> <damage>\n" );
		return;
	}

	idEntity *ent = gameLocal.FindEntity( args.Argv( 1 ) );
	if ( !ent ) {
		gameLocal.Printf( "entity not found\n" );
		return;
	}

	ent->Damage( gameLocal.world, gameLocal.world, idVec3( 0, 0, 1 ), "damage_moverCrush", atoi( args.Argv( 2 ) ), INVALID_JOINT );
}


/*
==================
Cmd_Remove_f

Removes the specified entity
==================
*/
void Cmd_Remove_f( const idCmdArgs &args ) {
	if ( !gameLocal.GetLocalPlayer() || !gameLocal.CheatsOk( false ) ) {
		return;
	}
	if ( args.Argc() != 2 ) {
		gameLocal.Printf( "usage: remove <name of entity to remove>\n" );
		return;
	}

	idEntity *ent = gameLocal.FindEntity( args.Argv( 1 ) );
	if ( !ent ) {
		gameLocal.Printf( "entity not found\n" );
		return;
	}

	delete ent;
}

/*
===================
Cmd_TestLight_f
===================
*/
void Cmd_TestLight_f( const idCmdArgs &args ) {
	int			i;
	idStr		filename;
	const char *key, *value, *name;
	idPlayer *	player;
	idDict		dict;

	player = gameLocal.GetLocalPlayer();
	if ( !player || !gameLocal.CheatsOk( false ) ) {
		return;
	}

	renderView_t	*rv = player->GetRenderView();

	float fov = tan( idMath::M_DEG2RAD * rv->fov_x / 2 );


	dict.SetMatrix( "rotation", mat3_default );
	dict.SetVector( "origin", rv->vieworg );
	dict.SetVector( "light_target", rv->viewaxis[0] );
	dict.SetVector( "light_right", rv->viewaxis[1] * -fov );
	dict.SetVector( "light_up", rv->viewaxis[2] * fov );
	dict.SetVector( "light_start", rv->viewaxis[0] * 16 );
	dict.SetVector( "light_end", rv->viewaxis[0] * 1000 );

	if ( args.Argc() >= 2 ) {
		value = args.Argv( 1 );
		filename = args.Argv(1);
		filename.DefaultFileExtension( ".tga" );
		dict.Set( "texture", filename );
	}

	dict.Set( "classname", "light" );
	for( i = 2; i < args.Argc() - 1; i += 2 ) {

		key = args.Argv( i );
		value = args.Argv( i + 1 );

		dict.Set( key, value );
	}

	for ( i = 0; i < MAX_GENTITIES; i++ ) {
		name = va( "spawned_light_%d", i );		// not just light_, or it might pick up a prelight shadow
		if ( !gameLocal.FindEntity( name ) ) {
			break;
		}
	}
	dict.Set( "name", name );

	gameLocal.SpawnEntityDef( dict );

	gameLocal.Printf( "Created new light\n");
}

/*
===================
Cmd_TestPointLight_f
===================
*/
void Cmd_TestPointLight_f( const idCmdArgs &args ) {
	const char *key, *value, *name;
	int			i;
	idPlayer	*player;
	idDict		dict;

	player = gameLocal.GetLocalPlayer();
	if ( !player || !gameLocal.CheatsOk( false ) ) {
		return;
	}

	dict.SetVector("origin", player->GetRenderView()->vieworg);

	if ( args.Argc() >= 2 ) {
		value = args.Argv( 1 );
		dict.Set("light", value);
	} else {
		dict.Set("light", "300");
	}

	dict.Set( "classname", "light" );
	for( i = 2; i < args.Argc() - 1; i += 2 ) {

		key = args.Argv( i );
		value = args.Argv( i + 1 );

		dict.Set( key, value );
	}

	for ( i = 0; i < MAX_GENTITIES; i++ ) {
		name = va( "light_%d", i );
		if ( !gameLocal.FindEntity( name ) ) {
			break;
		}
	}
	dict.Set( "name", name );

	gameLocal.SpawnEntityDef( dict );

	gameLocal.Printf( "Created new point light\n");
}

/*
==================
Cmd_PopLight_f
==================
*/
void Cmd_PopLight_f( const idCmdArgs &args ) {
	idEntity	*ent;
	idMapEntity *mapEnt;
	idMapFile	*mapFile = gameLocal.GetLevelMap();
	idLight		*lastLight;
	int			last;

	if ( !gameLocal.CheatsOk() ) {
		return;
	}

	bool removeFromMap = ( args.Argc() > 1 );

	lastLight = NULL;
	last = -1;
	for( ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() ) {
		if ( !ent->IsType( idLight::Type ) ) {
			continue;
		}

		if ( gameLocal.spawnIds[ ent->entityNumber ] > last ) {
			last = gameLocal.spawnIds[ ent->entityNumber ];
			lastLight = static_cast<idLight*>( ent );
		}
	}

	if ( lastLight ) {
		// find map file entity
		mapEnt = mapFile->FindEntity( lastLight->name );

		if ( removeFromMap && mapEnt ) {
			mapFile->RemoveEntity( mapEnt );
		}
		gameLocal.Printf( "Removing light %i\n", lastLight->GetLightDefHandle() );
		delete lastLight;
	} else {
		gameLocal.Printf( "No lights to clear.\n" );
	}

}

/*
====================
Cmd_ClearLights_f
====================
*/
void Cmd_ClearLights_f( const idCmdArgs &args ) {
	idEntity *ent;
	idEntity *next;
	idLight *light;
	idMapEntity *mapEnt;
	idMapFile *mapFile = gameLocal.GetLevelMap();
	int deletemode;
	int lightcounter = 0;

	//bool removeFromMap = ( args.Argc() > 1 );

	if (args.Argc() <= 1)
	{
		gameLocal.Printf("Deletes lights.\n1 = all lights.\n2 = only delete no-shadow lights.\n3 = only delete shadow-emitting lights.\n25 = delete 1/4 of lights.\n50 = delete 1/2 of lights.\n75 = delete 3/4 of lights.");
		return;
	}

	deletemode = atoi(args.Argv(1));

	gameLocal.Printf( "Clearing lights.\n" );
	for( ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = next ) {
		next = ent->spawnNode.Next();
		if ( !ent->IsType( idLight::Type ) ) {
			continue;
		}

		light = static_cast<idLight*>( ent );
		mapEnt = mapFile->FindEntity( light->name );

		//if ( removeFromMap && mapEnt ) {
		//	mapFile->RemoveEntity( mapEnt );
		//}

		if (deletemode == 1)
		{
			//delete everything.			
			//delete light;

			if (light->GetBindMaster() == NULL)
				delete light;
			else
				light->FadeOut(.5f);
		}
		else if (deletemode == 2)
		{
			//if no shadow, then delete it.
			if (light->GetNoShadow() && light->GetBindMaster() == NULL)
			{
				if (light->GetBindMaster() == NULL)
					delete light;
				else
					light->FadeOut(.5f);
			}
		}
		else if (deletemode == 3)
		{
			//if emit shadow, then delete it.
			if (!light->GetNoShadow())
			{
				if (light->GetBindMaster() == NULL)
					delete light;
				else
					light->FadeOut(.5f);
			}
		}
		else if (deletemode == 25)
		{
			if (lightcounter % 4 == 0)
			{
				if (light->GetBindMaster() == NULL)
					delete light;
				else
					light->FadeOut(.5f);
			}

			lightcounter++;
		}
		else if (deletemode == 50)
		{
			if (lightcounter % 2 == 0)
			{
				if (light->GetBindMaster() == NULL)
					delete light;
				else
					light->FadeOut(.5f);
			}

			lightcounter++;
		}
		else if (deletemode == 75)
		{
			if (lightcounter % 2 == 0 || lightcounter % 3 == 0)
			{
				if (light->GetBindMaster() == NULL)
					delete light;
				else
					light->FadeOut(.5f);
			}

			lightcounter++;
		}		
	}
}

void Cmd_ClearDebug_f(const idCmdArgs &args)
{
	gameRenderWorld->DebugClearLines(0);
	gameRenderWorld->DebugClearPolygons(0);
}

void Cmd_KillEntity_f(const idCmdArgs &args)
{
	trace_t tr;

	gameLocal.clip.TracePoint(tr, gameLocal.GetLocalPlayer()->GetEyePosition(),
		gameLocal.GetLocalPlayer()->GetEyePosition() + (gameLocal.GetLocalPlayer()->firstPersonViewAxis[0] * 1024),
		MASK_SHOT_RENDERMODEL, gameLocal.GetLocalPlayer());

	if (tr.fraction < 1 && tr.c.entityNum > 0 && tr.c.entityNum < MAX_GENTITIES - 2)
	{
		if (gameLocal.entities[tr.c.entityNum])
		{
			idBounds entityBounds = gameLocal.entities[tr.c.entityNum]->GetPhysics()->GetAbsBounds();

			gameRenderWorld->DrawText(gameLocal.entities[tr.c.entityNum]->GetName(), gameLocal.entities[tr.c.entityNum]->GetPhysics()->GetOrigin(), .06f, colorRed, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, 10000);
			gameRenderWorld->DebugBounds(colorRed, entityBounds, vec3_origin, 10000);

			gameLocal.entities[tr.c.entityNum]->PostEventMS(&EV_Remove, 0);
		}
	}
}

void Cmd_DamageEntity_f(const idCmdArgs &args)
{
	trace_t tr;

	gameLocal.clip.TracePoint(tr, gameLocal.GetLocalPlayer()->GetEyePosition(),
		gameLocal.GetLocalPlayer()->GetEyePosition() + (gameLocal.GetLocalPlayer()->firstPersonViewAxis[0] * 1024),
		MASK_SHOT_RENDERMODEL, gameLocal.GetLocalPlayer());

	if (tr.fraction < 1 && tr.c.entityNum > 0 && tr.c.entityNum < MAX_GENTITIES - 2)
	{
		if (gameLocal.entities[tr.c.entityNum])
		{
			idBounds entityBounds = gameLocal.entities[tr.c.entityNum]->GetPhysics()->GetAbsBounds();

			gameRenderWorld->DrawText(gameLocal.entities[tr.c.entityNum]->GetName(), gameLocal.entities[tr.c.entityNum]->GetPhysics()->GetOrigin(), .1f, colorRed, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, 10000);
			gameRenderWorld->DebugBounds(colorRed, entityBounds, vec3_origin, 10000);

			gameLocal.entities[tr.c.entityNum]->Damage(gameLocal.GetLocalPlayer(), gameLocal.GetLocalPlayer(), vec3_zero, "damage_1000", 1.0f, 0);
		}
	}
}

void Cmd_DebugCavity_f(const idCmdArgs &args)
{
	idEntity	*ent;
	for (ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next())
	{
		if (!ent)
			continue;

		if (idStr::FindText(ent->GetClassname(), "mushroom", false) >= 0 || idStr::FindText(ent->GetClassname(), "func_static", false) >= 0)
		{
			//valid, so proceed forward...
		}
		else
		{
			continue;
		}

		idLocationEntity *locationEntity = gameLocal.LocationForEntity(ent);
		if (!locationEntity)
		{
			//Entity is not associated with any info_location entity. Draw debug to mark this.
			gameRenderWorld->DebugBounds(colorRed, ent->GetPhysics()->GetAbsBounds(), vec3_origin, 900000);
			gameRenderWorld->DrawText(ent->GetName(), ent->GetPhysics()->GetOrigin(), .15f, colorRed, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, 900000);
		}
	}
}

void Cmd_DebugClipContents_f( const idCmdArgs &args )
{
	idPlayer* player = gameLocal.GetLocalPlayer();
	int clipResult = gameLocal.clip.Contents( player->GetPhysics()->GetOrigin(), NULL, mat3_identity, CONTENTS_SOLID, player );
	if ( clipResult & MASK_SOLID ) {
		gameLocal.Printf( "Player clips\n" );
	} else {
		gameLocal.Printf( "Player DOES NOT clip\n" );
	}
}

/*
==================
Cmd_TestFx_f
==================
*/
void Cmd_TestFx_f( const idCmdArgs &args ) {
	idVec3		offset;
	const char *name;
	idPlayer *	player;
	idDict		dict;

	player = gameLocal.GetLocalPlayer();
	if ( !player || !gameLocal.CheatsOk() ) {
		return;
	}

	// delete the testModel if active
	if ( gameLocal.testFx ) {
		delete gameLocal.testFx;
		gameLocal.testFx = NULL;
	}

	if ( args.Argc() < 2 ) {
		return;
	}

	name = args.Argv( 1 );

	offset = player->GetPhysics()->GetOrigin() + player->viewAngles.ToForward() * 100.0f;

	dict.Set( "origin", offset.ToString() );
	dict.Set( "test", "1");
	dict.Set( "fx", name );
	gameLocal.testFx = ( idEntityFx * )gameLocal.SpawnEntityType( idEntityFx::Type, &dict );
}

#define MAX_DEBUGLINES	128

typedef struct {
	bool used;
	idVec3 start, end;
	int color;
	bool blink;
	bool arrow;
} gameDebugLine_t;

gameDebugLine_t debugLines[MAX_DEBUGLINES];

/*
==================
Cmd_AddDebugLine_f
==================
*/
static void Cmd_AddDebugLine_f( const idCmdArgs &args ) {
	int i, argNum;
	const char *value;

	if ( !gameLocal.CheatsOk() ) {
		return;
	}

	if ( args.Argc () < 7 ) {
		gameLocal.Printf( "usage: addline <x y z> <x y z> <color>\n" );
		return;
	}
	for ( i = 0; i < MAX_DEBUGLINES; i++ ) {
		if ( !debugLines[i].used ) {
			break;
		}
	}
	if ( i >= MAX_DEBUGLINES ) {
		gameLocal.Printf( "no free debug lines\n" );
		return;
	}
	value = args.Argv( 0 );
	if ( !idStr::Icmp( value, "addarrow" ) ) {
		debugLines[i].arrow = true;
	} else {
		debugLines[i].arrow = false;
	}
	debugLines[i].used = true;
	debugLines[i].blink = false;
	argNum = 1;
	debugLines[i].start.x = Cmd_GetFloatArg( args, argNum );
	debugLines[i].start.y = Cmd_GetFloatArg( args, argNum );
	debugLines[i].start.z = Cmd_GetFloatArg( args, argNum );
	debugLines[i].end.x = Cmd_GetFloatArg( args, argNum );
	debugLines[i].end.y = Cmd_GetFloatArg( args, argNum );
	debugLines[i].end.z = Cmd_GetFloatArg( args, argNum );
	debugLines[i].color = Cmd_GetFloatArg( args, argNum );
}

/*
==================
Cmd_RemoveDebugLine_f
==================
*/
static void Cmd_RemoveDebugLine_f( const idCmdArgs &args ) {
	int i, num;
	const char *value;

	if ( !gameLocal.CheatsOk() ) {
		return;
	}

	if ( args.Argc () < 2 ) {
		gameLocal.Printf( "usage: removeline <num>\n" );
		return;
	}
	value = args.Argv( 1 );
	num = atoi(value);
	for ( i = 0; i < MAX_DEBUGLINES; i++ ) {
		if ( debugLines[i].used ) {
			if ( --num < 0 ) {
				break;
			}
		}
	}
	if ( i >= MAX_DEBUGLINES ) {
		gameLocal.Printf( "line not found\n" );
		return;
	}
	debugLines[i].used = false;
}

/*
==================
Cmd_BlinkDebugLine_f
==================
*/
static void Cmd_BlinkDebugLine_f( const idCmdArgs &args ) {
	int i, num;
	const char *value;

	if ( !gameLocal.CheatsOk() ) {
		return;
	}

	if ( args.Argc () < 2 ) {
		gameLocal.Printf( "usage: blinkline <num>\n" );
		return;
	}
	value = args.Argv( 1 );
	num = atoi( value );
	for ( i = 0; i < MAX_DEBUGLINES; i++ ) {
		if ( debugLines[i].used ) {
			if ( --num < 0 ) {
				break;
			}
		}
	}
	if ( i >= MAX_DEBUGLINES ) {
		gameLocal.Printf( "line not found\n" );
		return;
	}
	debugLines[i].blink = !debugLines[i].blink;
}

/*
==================
PrintFloat
==================
*/
static void PrintFloat( float f ) {
	char buf[128];
	int i;

	for ( i = sprintf( buf, "%3.2f", f ); i < 7; i++ ) {
		buf[i] = ' ';
	}
	buf[i] = '\0';
	gameLocal.Printf( buf );
}

/*
==================
Cmd_ListDebugLines_f
==================
*/
static void Cmd_ListDebugLines_f( const idCmdArgs &args ) {
	int i, num;

	if ( !gameLocal.CheatsOk() ) {
		return;
	}

	num = 0;
	gameLocal.Printf( "line num: x1     y1     z1     x2     y2     z2     c  b  a\n" );
	for ( i = 0; i < MAX_DEBUGLINES; i++ ) {
		if ( debugLines[i].used ) {
			gameLocal.Printf( "line %3d: ", num );
			PrintFloat( debugLines[i].start.x );
			PrintFloat( debugLines[i].start.y );
			PrintFloat( debugLines[i].start.z );
			PrintFloat( debugLines[i].end.x );
			PrintFloat( debugLines[i].end.y );
			PrintFloat( debugLines[i].end.z );
			gameLocal.Printf( "%d  %d  %d\n", debugLines[i].color, debugLines[i].blink, debugLines[i].arrow );
			num++;
		}
	}
	if ( !num ) {
		gameLocal.Printf( "no debug lines\n" );
	}
}

/*
==================
D_DrawDebugLines
==================
*/
void D_DrawDebugLines( void ) {
	int i;
	idVec3 forward, right, up, p1, p2;
	idVec4 color;
	float l;

	for ( i = 0; i < MAX_DEBUGLINES; i++ ) {
		if ( debugLines[i].used ) {
			if ( !debugLines[i].blink || (gameLocal.time & (1<<9)) ) {
				color = idVec4( debugLines[i].color&1, (debugLines[i].color>>1)&1, (debugLines[i].color>>2)&1, 1 );
				gameRenderWorld->DebugLine( color, debugLines[i].start, debugLines[i].end );
				//
				if ( debugLines[i].arrow ) {
					// draw a nice arrow
					forward = debugLines[i].end - debugLines[i].start;
					l = forward.Normalize() * 0.2f;
					forward.NormalVectors( right, up);

					if ( l > 3.0f ) {
						l = 3.0f;
					}
					p1 = debugLines[i].end - l * forward + (l * 0.4f) * right;
					p2 = debugLines[i].end - l * forward - (l * 0.4f) * right;
					gameRenderWorld->DebugLine( color, debugLines[i].end, p1 );
					gameRenderWorld->DebugLine( color, debugLines[i].end, p2 );
					gameRenderWorld->DebugLine( color, p1, p2 );
				}
			}
		}
	}
}

/*
==================
Cmd_ListCollisionModels_f
==================
*/
static void Cmd_ListCollisionModels_f( const idCmdArgs &args ) {
	if ( !gameLocal.CheatsOk() ) {
		return;
	}

	collisionModelManager->ListModels();
}

/*
==================
Cmd_CollisionModelInfo_f
==================
*/
static void Cmd_CollisionModelInfo_f( const idCmdArgs &args ) {
	const char *value;

	if ( !gameLocal.CheatsOk() ) {
		return;
	}

	if ( args.Argc () < 2 ) {
		gameLocal.Printf( "usage: collisionModelInfo <modelNum>\n"
					"use 'all' instead of the model number for accumulated info\n" );
		return;
	}

	value = args.Argv( 1 );
	if ( !idStr::Icmp( value, "all" ) ) {
		collisionModelManager->ModelInfo( -1 );
	} else {
		collisionModelManager->ModelInfo( atoi(value) );
	}
}

/*
==================
Cmd_ExportModels_f
==================
*/
static void Cmd_ExportModels_f( const idCmdArgs &args ) {
	idModelExport	exporter;
	idStr			name;

	// don't allow exporting models when cheats are disabled,
	// but if we're not in the game, it's ok
	if ( gameLocal.GetLocalPlayer() && !gameLocal.CheatsOk( false ) ) {
		return;
	}

	if ( args.Argc() < 2 ) {
		exporter.ExportModels( "def", ".def" );
	} else {
		name = args.Argv( 1 );
		name = "def/" + name;
		name.DefaultFileExtension( ".def" );
		exporter.ExportDefFile( name );
	}
}

/*
==================
Cmd_ReexportModels_f
==================
*/
static void Cmd_ReexportModels_f( const idCmdArgs &args ) {
	idModelExport	exporter;
	idStr			name;

	// don't allow exporting models when cheats are disabled,
	// but if we're not in the game, it's ok
	if ( gameLocal.GetLocalPlayer() && !gameLocal.CheatsOk( false ) ) {
		return;
	}

	idAnimManager::forceExport = true;
	if ( args.Argc() < 2 ) {
		exporter.ExportModels( "def", ".def" );
	} else {
		name = args.Argv( 1 );
		name = "def/" + name;
		name.DefaultFileExtension( ".def" );
		exporter.ExportDefFile( name );
	}
	idAnimManager::forceExport = false;
}

/*
==================
Cmd_ReloadAnims_f
==================
*/
static void Cmd_ReloadAnims_f( const idCmdArgs &args ) {
	// don't allow reloading anims when cheats are disabled,
	// but if we're not in the game, it's ok
	if ( gameLocal.GetLocalPlayer() && !gameLocal.CheatsOk( false ) ) {
		return;
	}

	animationLib.ReloadAnims();
}

/*
==================
Cmd_ListAnims_f
==================
*/
static void Cmd_ListAnims_f( const idCmdArgs &args ) {
	idEntity *		ent;
	int				num;
	size_t			size;
	size_t			alloced;
	idAnimator *	animator;
	const char *	classname;
	const idDict *	dict;
	int				i;

	if ( args.Argc() > 1 ) {
		idAnimator animator;

		classname = args.Argv( 1 );

		dict = gameLocal.FindEntityDefDict( classname, false );
		if ( !dict ) {
			gameLocal.Printf( "Entitydef '%s' not found\n", classname );
			return;
		}
		animator.SetModel( dict->GetString( "model" ) );

		gameLocal.Printf( "----------------\n" );
		num = animator.NumAnims();
		for( i = 0; i < num; i++ ) {
			gameLocal.Printf( "%s\n", animator.AnimFullName( i ) );
		}
		gameLocal.Printf( "%d anims\n", num );
	} else {
		animationLib.ListAnims();

		size = 0;
		num = 0;
		for( ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() ) {
			animator = ent->GetAnimator();
			if ( animator ) {
				alloced = animator->Allocated();
				size += alloced;
				num++;
			}
		}

		gameLocal.Printf( "%zd memory used in %d entity animators\n", size, num );
	}
}

/*
==================
Cmd_AASStats_f
==================
*/
static void Cmd_AASStats_f( const idCmdArgs &args ) {
	int aasNum;

	if ( !gameLocal.CheatsOk() ) {
		return;
	}

	aasNum = aas_test.GetInteger();
	idAAS *aas = gameLocal.GetAAS( aasNum );
	if ( !aas ) {
		gameLocal.Printf( "No aas #%d loaded\n", aasNum );
	} else {
		aas->Stats();
	}
}

/*
==================
Cmd_TestDamage_f
==================
*/
static void Cmd_TestDamage_f( const idCmdArgs &args ) {
	idPlayer *player;
	const char *damageDefName;

	player = gameLocal.GetLocalPlayer();
	if ( !player || !gameLocal.CheatsOk() ) {
		return;
	}

	if ( args.Argc() < 2 || args.Argc() > 3 ) {
		gameLocal.Printf( "usage: testDamage <damageDefName> [angle]\n" );
		return;
	}

	damageDefName = args.Argv( 1 );

	idVec3	dir;
	if ( args.Argc() == 3 ) {
		float angle = atof( args.Argv( 2 ) );

		idMath::SinCos( DEG2RAD( angle ), dir[1], dir[0] );
		dir[2] = 0;
	} else {
		dir.Zero();
	}

	// give the player full health before and after
	// running the damage
	//player->health = player->inventory.maxHealth; //BC
	player->Damage( NULL, NULL, dir, damageDefName, 1.0f, INVALID_JOINT );
	//player->health = player->inventory.maxHealth; //BC
}

/*
==================
Cmd_TestBoneFx_f
==================
*/
static void Cmd_TestBoneFx_f( const idCmdArgs &args ) {
	idPlayer *player;
	const char *bone, *fx;

	player = gameLocal.GetLocalPlayer();
	if ( !player || !gameLocal.CheatsOk() ) {
		return;
	}

	if ( args.Argc() < 3 || args.Argc() > 4 ) {
		gameLocal.Printf( "usage: testBoneFx <fxName> <boneName>\n" );
		return;
	}

	fx = args.Argv( 1 );
	bone = args.Argv( 2 );

	player->StartFxOnBone( fx, bone );
}

/*
==================
Cmd_TestDamage_f
==================
*/
static void Cmd_TestDeath_f( const idCmdArgs &args ) {
	idPlayer *player;

	player = gameLocal.GetLocalPlayer();
	if ( !player || !gameLocal.CheatsOk() ) {
		return;
	}

	idVec3 dir;
	idMath::SinCos( DEG2RAD( 45.0f ), dir[1], dir[0] );
	dir[2] = 0;

	g_testDeath.SetBool( 1 );

	if (args.Argc() >= 2)
	{
		player->SetDefibAvailable(false);
	}

	player->Damage( NULL, NULL, dir, "damage_triggerhurt_1000", 1.0f, INVALID_JOINT );
}

/*
==================
Cmd_WeaponSplat_f
==================
*/
static void Cmd_WeaponSplat_f( const idCmdArgs &args ) {
	idPlayer *player;

	player = gameLocal.GetLocalPlayer();
	if ( !player || !gameLocal.CheatsOk() ) {
		return;
	}

	player->weapon.GetEntity()->BloodSplat( 8.0f );
}

/*
==================
Cmd_SaveSelected_f
==================
*/
static void Cmd_SaveSelected_f( const idCmdArgs &args ) {
	int i;
	idPlayer *player;
	idEntity *s;
	idMapEntity *mapEnt;
	idMapFile *mapFile = gameLocal.GetLevelMap();
	idDict dict;
	idStr mapName;
	const char *name;

	player = gameLocal.GetLocalPlayer();
	if ( !player || !gameLocal.CheatsOk() ) {
		return;
	}

	s = player->dragEntity.GetSelected();
	if ( !s ) {
		gameLocal.Printf( "no entity selected, set g_dragShowSelection 1 to show the current selection\n" );
		return;
	}

	if ( args.Argc() > 1 ) {
		mapName = args.Argv( 1 );
		mapName = "maps/" + mapName;
	}
	else {
		mapName = mapFile->GetName();
	}

	// find map file entity
	mapEnt = mapFile->FindEntity( s->name );
	// create new map file entity if there isn't one for this articulated figure
	if ( !mapEnt ) {
		mapEnt = new idMapEntity();
		mapFile->AddEntity( mapEnt );
		for ( i = 0; i < 9999; i++ ) {
			name = va( "%s_%d", s->GetEntityDefName(), i );
			if ( !gameLocal.FindEntity( name ) ) {
				break;
			}
		}
		s->name = name;
		mapEnt->epairs.Set( "classname", s->GetEntityDefName() );
		mapEnt->epairs.Set( "name", s->name );
	}

	if ( s->IsType( idMoveable::Type ) ) {
		// save the moveable state
		mapEnt->epairs.Set( "origin", s->GetPhysics()->GetOrigin().ToString( 8 ) );
		mapEnt->epairs.Set( "rotation", s->GetPhysics()->GetAxis().ToString( 8 ) );
	}
	else if ( s->IsType( idAFEntity_Generic::Type ) || s->IsType( idAFEntity_WithAttachedHead::Type ) ) {
		// save the articulated figure state
		dict.Clear();
		static_cast<idAFEntity_Base *>(s)->SaveState( dict );
		mapEnt->epairs.Copy( dict );
	}

	// write out the map file
	mapFile->Write( mapName, ".map" );
}

/*
==================
Cmd_DeleteSelected_f
==================
*/
static void Cmd_DeleteSelected_f( const idCmdArgs &args ) {
	idPlayer *player;

	player = gameLocal.GetLocalPlayer();
	if ( !player || !gameLocal.CheatsOk() ) {
		return;
	}

	if ( player ) {
		player->dragEntity.DeleteSelected();
	}
}

/*
==================
Cmd_SaveMoveables_f
==================
*/
static void Cmd_SaveMoveables_f( const idCmdArgs &args ) {

	//BC NOTE: This originally was for idMoveable, and I changed it to instead affect idMoveableItem

	int e, i;
	idMoveableItem *m;
	idMapEntity *mapEnt;
	idMapFile *mapFile = gameLocal.GetLevelMap();
	idStr mapName;
	const char *name;

	if ( !gameLocal.CheatsOk() ) {
		return;
	}

	for( e = 0; e < MAX_GENTITIES; e++ ) {
		m = static_cast<idMoveableItem *>(gameLocal.entities[ e ]);

		if ( !m || !m->IsType(idMoveableItem::Type ) ) {
			continue;
		}

		if ( m->IsBound() ) {
			continue;
		}

		if ( !m->IsAtRest() ) {
			break;
		}
	}

	if ( e < MAX_GENTITIES ) {
		gameLocal.Warning( "map not saved because the moveable entity %s is not at rest", gameLocal.entities[ e ]->name.c_str() );
		return;
	}

	if ( args.Argc() > 1 ) {
		mapName = args.Argv( 1 );
		mapName = "maps/" + mapName;
	}
	else {
		mapName = mapFile->GetName();
	}

	int itemCount = 0;

	for( e = 0; e < MAX_GENTITIES; e++ ) {
		m = static_cast<idMoveableItem *>(gameLocal.entities[ e ]);

		if ( !m || !m->IsType(idMoveableItem::Type ) ) {
			continue;
		}

		if ( m->IsBound() ) {
			continue;
		}

		// find map file entity
		mapEnt = mapFile->FindEntity( m->name );
		// create new map file entity if there isn't one for this articulated figure
		if ( !mapEnt ) {
			mapEnt = new idMapEntity();
			mapFile->AddEntity( mapEnt );
			for ( i = 0; i < 9999; i++ ) {
				name = va( "%s_%d", m->GetEntityDefName(), i );
				if ( !gameLocal.FindEntity( name ) ) {
					break;
				}
			}
			m->name = name;
			mapEnt->epairs.Set( "classname", m->GetEntityDefName() );
			mapEnt->epairs.Set( "name", m->name );
		}
		// save the moveable state
		mapEnt->epairs.Set( "origin", m->GetPhysics()->GetOrigin().ToString( 8 ) );
		mapEnt->epairs.Set( "rotation", m->GetPhysics()->GetAxis().ToString( 8 ) );

		itemCount++;
	}

	common->Printf("SaveMoveables: writing to %d entities.\n", itemCount);

	// write out the map file
	mapFile->Write(mapName, ".map");
}

/*
==================
Cmd_SaveRagdolls_f
==================
*/
static void Cmd_SaveRagdolls_f( const idCmdArgs &args ) {
	int e, i;
	idAFEntity_Base *af;
	idMapEntity *mapEnt;
	idMapFile *mapFile = gameLocal.GetLevelMap();
	idDict dict;
	idStr mapName;
	const char *name;

	if ( !gameLocal.CheatsOk() ) {
		return;
	}

	if ( args.Argc() > 1 ) {
		mapName = args.Argv( 1 );
		mapName = "maps/" + mapName;
	}
	else {
		mapName = mapFile->GetName();
	}

	for( e = 0; e < MAX_GENTITIES; e++ ) {
		af = static_cast<idAFEntity_Base *>(gameLocal.entities[ e ]);

		if ( !af ) {
			continue;
		}

		if ( !af->IsType( idAFEntity_WithAttachedHead::Type ) && !af->IsType( idAFEntity_Generic::Type ) ) {
			continue;
		}

		if ( af->IsBound() ) {
			continue;
		}

		if ( !af->IsAtRest() ) {
			gameLocal.Warning( "the articulated figure for entity %s is not at rest", gameLocal.entities[ e ]->name.c_str() );
		}

		dict.Clear();
		af->SaveState( dict );

		// find map file entity
		mapEnt = mapFile->FindEntity( af->name );
		// create new map file entity if there isn't one for this articulated figure
		if ( !mapEnt ) {
			mapEnt = new idMapEntity();
			mapFile->AddEntity( mapEnt );
			for ( i = 0; i < 9999; i++ ) {
				name = va( "%s_%d", af->GetEntityDefName(), i );
				if ( !gameLocal.FindEntity( name ) ) {
					break;
				}
			}
			af->name = name;
			mapEnt->epairs.Set( "classname", af->GetEntityDefName() );
			mapEnt->epairs.Set( "name", af->name );
		}
		// save the articulated figure state
		mapEnt->epairs.Copy( dict );
	}

	// write out the map file
	mapFile->Write( mapName, ".map" );
}

/*
==================
Cmd_BindRagdoll_f
==================
*/
static void Cmd_BindRagdoll_f( const idCmdArgs &args ) {
	idPlayer *player;

	player = gameLocal.GetLocalPlayer();
	if ( !player || !gameLocal.CheatsOk() ) {
		return;
	}

	if ( player ) {
		player->dragEntity.BindSelected();
	}
}

/*
==================
Cmd_UnbindRagdoll_f
==================
*/
static void Cmd_UnbindRagdoll_f( const idCmdArgs &args ) {
	idPlayer *player;

	player = gameLocal.GetLocalPlayer();
	if ( !player || !gameLocal.CheatsOk() ) {
		return;
	}

	if ( player ) {
		player->dragEntity.UnbindSelected();
	}
}

/*
==================
Cmd_GameError_f
==================
*/
static void Cmd_GameError_f( const idCmdArgs &args ) {
	gameLocal.Error( "game error" );
}

/*
==================
Cmd_SaveLights_f
==================
*/
static void Cmd_SaveLights_f( const idCmdArgs &args ) {
	int e;
	idLight *light;
	idMapEntity *mapEnt;
	idMapFile *mapFile = gameLocal.GetLevelMap();
	idDict dict;
	idStr mapName;

	if ( !gameLocal.CheatsOk() ) {
		return;
	}

	if ( args.Argc() > 1 ) {
		mapName = args.Argv( 1 );
		mapName = "maps/" + mapName;
	}
	else {
		mapName = mapFile->GetName();
	}

	int lightCount = 0;

	for( e = 0; e < MAX_GENTITIES; e++ )
	{
		idVec3 lightColor;
		light = static_cast<idLight*>(gameLocal.entities[ e ]);

		if ( !light || !light->IsType( idLight::Type ) ) {
			continue;
		}

		dict.Clear();
		light->SaveState( &dict );

		// find map file entity
		mapEnt = mapFile->FindEntity( light->name );
		// create new map file entity if there isn't one for this light
		if ( !mapEnt ) {

			continue; //BC if light can't be found in the map file, then just skip it.

			//mapEnt = new idMapEntity();
			//mapFile->AddEntity( mapEnt );
			//for ( i = 0; i < 9999; i++ ) {
			//	name = va( "%s_%d", light->GetEntityDefName(), i );
			//	if ( !gameLocal.FindEntity( name ) ) {
			//		break;
			//	}
			//}
			//light->name = name;
			//mapEnt->epairs.Set( "classname", light->GetEntityDefName() );
			//mapEnt->epairs.Set( "name", light->name );
		}

		//common->Printf("Saving position of %s: %f %f %f\n", light->GetName(), light->GetPhysics()->GetOrigin().x, light->GetPhysics()->GetOrigin().y, light->GetPhysics()->GetOrigin().z);

		// save the light state
		mapEnt->epairs.Copy( dict );

		//BC fields to save.

        if (light->GetBindMaster() == NULL) //only save position/angle if it's not bound to a mover.
        {
            mapEnt->epairs.Set("origin", light->GetPhysics()->GetOrigin().ToString(8));
            mapEnt->epairs.Set("rotation", light->GetPhysics()->GetAxis().ToString(8));
        }

		mapEnt->epairs.Set("noshadows", light->GetNoShadow() ? "1" : "0" );
		mapEnt->epairs.Set("ambient", light->Event_IsAmbient() ? "1" : "0");
		
		light->GetBaseColor(lightColor);
		
		if (lightColor.x > 0 || lightColor.y > 0 || lightColor.z > 0)
			mapEnt->epairs.Set("_color", lightColor.ToString(4));

		mapEnt->epairs.Set("light_radius", light->GetRadius().ToString(4) );

		// SM: Overwrite the light's spawn arguments to the new dictionary so the reset button works
		gameEdit->EntityChangeSpawnArgs(light, &mapEnt->epairs);

		lightCount++;
	}

	common->Printf("SaveLights: writing to %d lights.\n", lightCount);

	// write out the map file
	mapFile->Write( mapName, ".map" );
}


static void Cmd_SaveNotes_f(const idCmdArgs& args)
{
	int e;	
	idMapEntity* mapEnt;
	idMapFile* mapFile = gameLocal.GetLevelMap();
	idDict dict;
	idStr mapName;

	if (!gameLocal.CheatsOk()) {
		return;
	}
	
	mapName = idStr::Format("%s_loc_DO_NOT_CHECK_IN", mapFile->GetName());

	for (e = 0; e < MAX_GENTITIES; e++)
	{
		idEntity* ent = gameLocal.entities[e];

		if (!ent)
			continue;

		if (!ent->spawnArgs.GetBool("is_note"))
			continue;

		dict.Clear();
		dict.Set("gui_parm0", ent->spawnArgs.GetString("gui_parm0"));
		
		mapEnt = mapFile->FindEntity(ent->name);
		mapEnt->epairs.Copy(dict);		
	}

	common->Printf("saving notes to map file.\n");

	// write out the map file
	mapFile->Write(mapName, ".map");
}


/*
==================
Cmd_SaveParticles_f
==================
*/
static void Cmd_SaveParticles_f( const idCmdArgs &args ) {
	int e;
	idEntity *ent;
	idMapEntity *mapEnt;
	idMapFile *mapFile = gameLocal.GetLevelMap();
	idDict dict;
	idStr mapName, strModel;

	if ( !gameLocal.CheatsOk() ) {
		return;
	}

	if ( args.Argc() > 1 ) {
		mapName = args.Argv( 1 );
		mapName = "maps/" + mapName;
	}
	else {
		mapName = mapFile->GetName();
	}

	int particleCount = 0;

	for( e = 0; e < MAX_GENTITIES; e++ ) {

		ent = static_cast<idStaticEntity*> ( gameLocal.entities[ e ] );

		if ( !ent ) {
			continue;
		}

		strModel = ent->spawnArgs.GetString( "model" );
		if ( strModel.Length() && strModel.Find( ".prt") > 0 ) {
			dict.Clear();
			dict.Set( "model", ent->spawnArgs.GetString( "model" ) );
			dict.SetVector( "origin", ent->GetPhysics()->GetOrigin() );

			// find map file entity
			mapEnt = mapFile->FindEntity( ent->name );
			// create new map file entity if there isn't one for this entity
			if ( !mapEnt ) {
				continue;
			}
			// save the particle state
			mapEnt->epairs.Copy( dict );

			particleCount++;
		}
	}

	common->Printf("SaveParticles: writing to %d particles.\n", particleCount);

	// write out the map file
	mapFile->Write( mapName, ".map" );
}


/*
==================
Cmd_DisasmScript_f
==================
*/
static void Cmd_DisasmScript_f( const idCmdArgs &args ) {
	gameLocal.program.Disassemble();
}

/*
==================
Cmd_TestSave_f
==================
*/
static void Cmd_TestSave_f( const idCmdArgs &args ) {
	idFile *f;

	f = fileSystem->OpenFileWrite( "test.sav" );
	gameLocal.SaveGame( f );
	fileSystem->CloseFile( f );
}

/*
==================
Cmd_RecordViewNotes_f
==================
*/
static void Cmd_RecordViewNotes_f( const idCmdArgs &args ) {
	idPlayer *player;
	idVec3 origin;
	idMat3 axis;

	if ( args.Argc() <= 3 ) {
		return;
	}

	player = gameLocal.GetLocalPlayer();
	if ( !player ) {
		return;
	}

	player->GetViewPos( origin, axis );

	// Argv(1) = filename for map (viewnotes/mapname/person)
	// Argv(2) = note number (person0001)
	// Argv(3) = comments

	idStr str = args.Argv(1);
	str.SetFileExtension( ".txt" );

#ifdef _D3XP
	idFile *file = fileSystem->OpenFileAppend( str, false, "fs_cdpath" );
#else
	idFile *file = fileSystem->OpenFileAppend( str );
#endif

	if ( file ) {
		file->WriteFloatString( "\"view\"\t( %s )\t( %s )\r\n", origin.ToString(), axis.ToString() );
		file->WriteFloatString( "\"comments\"\t\"%s: %s\"\r\n\r\n", args.Argv(2), args.Argv(3) );
		fileSystem->CloseFile( file );
	}

	idStr viewComments = args.Argv(1);
	viewComments.StripLeading("viewnotes/");
	viewComments += " -- Loc: ";
	viewComments += origin.ToString();
	viewComments += "\n";
	viewComments += args.Argv(3);
	player->hud->SetStateString( "viewcomments", viewComments );
	player->hud->HandleNamedEvent( "showViewComments" );
}

/*
==================
Cmd_CloseViewNotes_f
==================
*/
static void Cmd_CloseViewNotes_f( const idCmdArgs &args ) {
	idPlayer *player = gameLocal.GetLocalPlayer();

	if ( !player ) {
		return;
	}

	player->hud->SetStateString( "viewcomments", "" );
	player->hud->HandleNamedEvent( "hideViewComments" );
}

/*
==================
Cmd_ShowViewNotes_f
==================
*/
static void Cmd_ShowViewNotes_f( const idCmdArgs &args ) {
	static idLexer parser( LEXFL_ALLOWPATHNAMES | LEXFL_NOSTRINGESCAPECHARS | LEXFL_NOSTRINGCONCAT | LEXFL_NOFATALERRORS );
	idToken	token;
	idPlayer *player;
	idVec3 origin;
	idMat3 axis;

	player = gameLocal.GetLocalPlayer();

	if ( !player ) {
		return;
	}

	if ( !parser.IsLoaded() ) {
		idStr str = "viewnotes/";
		str += gameLocal.GetMapName();
		str.StripFileExtension();
		str += "/";
		if ( args.Argc() > 1 ) {
			str += args.Argv( 1 );
		} else {
			str += "comments";
		}
		str.SetFileExtension( ".txt" );
		if ( !parser.LoadFile( str ) ) {
			gameLocal.Printf( "No view notes for %s\n", gameLocal.GetMapName() );
			return;
		}
	}

	if ( parser.ExpectTokenString( "view" ) && parser.Parse1DMatrix( 3, origin.ToFloatPtr() ) &&
		parser.Parse1DMatrix( 9, axis.ToFloatPtr() ) && parser.ExpectTokenString( "comments" ) && parser.ReadToken( &token ) ) {
		player->hud->SetStateString( "viewcomments", token );
		player->hud->HandleNamedEvent( "showViewComments" );
		player->Teleport( origin, axis.ToAngles(), NULL );
	} else {
		parser.FreeSource();
		player->hud->HandleNamedEvent( "hideViewComments" );
		return;
	}
}

/*
=================
FindEntityGUIs

helper function for Cmd_NextGUI_f.  Checks the passed entity to determine if it
has any valid gui surfaces.
=================
*/
bool FindEntityGUIs( idEntity *ent, const modelSurface_t ** surfaces,  int maxSurfs, int &guiSurfaces ) {
	renderEntity_t			*renderEnt;
	idRenderModel			*renderModel;
	const modelSurface_t	*surf;
	const idMaterial		*shader;
	int						i;

	assert( surfaces != NULL );
	assert( ent != NULL );

	memset( surfaces, 0x00, sizeof( modelSurface_t *) * maxSurfs );
	guiSurfaces = 0;

	renderEnt  = ent->GetRenderEntity();
	renderModel = renderEnt->hModel;
	if ( renderModel == NULL ) {
		return false;
	}

	for( i = 0; i < renderModel->NumSurfaces(); i++ ) {
		surf = renderModel->Surface( i );
		if ( surf == NULL ) {
			continue;
		}
		shader = surf->shader;
		if ( shader == NULL ) {
			continue;
		}
		if ( shader->GetEntityGui() > 0 ) {
			surfaces[ guiSurfaces++ ] = surf;
		}
	}

	return ( guiSurfaces != 0 );
}

/*
=================
Cmd_NextGUI_f
=================
*/
void Cmd_NextGUI_f( const idCmdArgs &args ) {
	idVec3					origin;
	idAngles				angles;
	idPlayer				*player;
	idEntity				*ent;
	int						guiSurfaces;
	bool					newEnt;
	renderEntity_t			*renderEnt;
	int						surfIndex;
	srfTriangles_t			*geom;
	idMat4					modelMatrix;
	idVec3					normal;
	idVec3					center;
	const modelSurface_t	*surfaces[ MAX_RENDERENTITY_GUI ];

	player = gameLocal.GetLocalPlayer();
	if ( !player || !gameLocal.CheatsOk() ) {
		return;
	}

	if ( args.Argc() != 1 ) {
		gameLocal.Printf( "usage: nextgui\n" );
		return;
	}

	// start at the last entity
	ent = gameLocal.lastGUIEnt.GetEntity();

	// see if we have any gui surfaces left to go to on the current entity.
	guiSurfaces = 0;
	newEnt = false;
	if ( ent == NULL ) {
		newEnt = true;
	} else if ( FindEntityGUIs( ent, surfaces, MAX_RENDERENTITY_GUI, guiSurfaces ) == true ) {
		if ( gameLocal.lastGUI >= guiSurfaces ) {
			newEnt = true;
		}
	} else {
		// no actual gui surfaces on this ent, so skip it
		newEnt = true;
	}

	if ( newEnt == true ) {
		// go ahead and skip to the next entity with a gui...
		if ( ent == NULL ) {
			ent = gameLocal.spawnedEntities.Next();
		} else {
			ent = ent->spawnNode.Next();
		}

		for ( ; ent != NULL; ent = ent->spawnNode.Next() ) {
			if ( ent->spawnArgs.GetString( "gui", NULL ) != NULL ) {
				break;
			}

			if ( ent->spawnArgs.GetString( "gui2", NULL ) != NULL ) {
				break;
			}

			if ( ent->spawnArgs.GetString( "gui3", NULL ) != NULL ) {
				break;
			}

			// try the next entity
			gameLocal.lastGUIEnt = ent;
		}

		gameLocal.lastGUIEnt = ent;
		gameLocal.lastGUI = 0;

		if ( !ent ) {
			gameLocal.Printf( "No more gui entities. Starting over...\n" );
			return;
		}
	}

	if ( FindEntityGUIs( ent, surfaces, MAX_RENDERENTITY_GUI, guiSurfaces ) == false ) {
		gameLocal.Printf( "Entity \"%s\" has gui properties but no gui surfaces.\n", ent->name.c_str() );
	}

	if ( guiSurfaces == 0 ) {
		gameLocal.Printf( "Entity \"%s\" has gui properties but no gui surfaces!\n", ent->name.c_str() );
		return;
	}

	gameLocal.Printf( "Teleporting to gui entity \"%s\", gui #%d.\n" , ent->name.c_str (), gameLocal.lastGUI );

	renderEnt = ent->GetRenderEntity();
	surfIndex = gameLocal.lastGUI++;
	geom = surfaces[ surfIndex ]->geometry;
	if ( geom == NULL ) {
		gameLocal.Printf( "Entity \"%s\" has gui surface %d without geometry!\n", ent->name.c_str(), surfIndex );
		return;
	}

	assert( geom->facePlanes != NULL );

	modelMatrix = idMat4( renderEnt->axis, renderEnt->origin );
	normal = geom->facePlanes[ 0 ].Normal() * renderEnt->axis;
	center = geom->bounds.GetCenter() * modelMatrix;

	origin = center + (normal * 32.0f);
	origin.z -= player->EyeHeight();
	normal *= -1.0f;
	angles = normal.ToAngles ();

	//	make sure the player is in noclip
	player->noclip = true;
	player->Teleport( origin, angles, NULL );
}

#ifdef _D3XP
void Cmd_SetActorState_f( const idCmdArgs &args ) {

	if ( args.Argc() != 3 ) {
		common->Printf( "usage: setActorState <entity name> <state>\n" );
		return;
	}

	idEntity* ent;
	ent = gameLocal.FindEntity( args.Argv( 1 ) );
	if ( !ent ) {
		gameLocal.Printf( "entity not found\n" );
		return;
	}


	if(!ent->IsType(idActor::Type)) {
		gameLocal.Printf( "entity not an actor\n" );
		return;
	}

	idActor* actor = (idActor*)ent;
	actor->PostEventMS(&AI_SetState, 0, args.Argv(2));
}
#endif

static void ArgCompletion_DefFile( const idCmdArgs &args, void(*callback)( const char *s ) ) {
	cmdSystem->ArgCompletion_FolderExtension( args, callback, "def/", true, ".def", NULL );
}

/*
===============
Cmd_TestId_f
outputs a string from the string table for the specified id
===============
*/
void Cmd_TestId_f( const idCmdArgs &args ) {
	idStr	id;
	int		i;
	if ( args.Argc() == 1 ) {
		common->Printf( "usage: testid <string id>\n" );
		return;
	}

	for ( i = 1; i < args.Argc(); i++ ) {
		id += args.Argv( i );
	}

	id.ToLower();
	if ( idStr::Cmpn( id, STRTABLE_ID, STRTABLE_ID_LENGTH ) != 0 ) {
		id = STRTABLE_ID + id;
	}
	gameLocal.mpGame.AddChatLine( common->GetLanguageDict()->GetString( id ), "<nothing>", "<nothing>", "<nothing>" );
}




//BC
static void Cmd_HideAll_f(const idCmdArgs &args)
{
	//hide all entities.
	for (int i = 0; i < gameLocal.num_entities; i++)
	{
		if (!gameLocal.entities[i])
			continue;

		gameLocal.entities[i]->Hide();
	}

	cvarSystem->SetCVarString("r_clear", ".7 0 .7"); //make it magenta
}

static void Cmd_ListEntitiesVisible_f(const idCmdArgs &args)
{
	int e, count;
	idEntity	*check;

	count = 0;

	gameLocal.Printf(" \n");
	
	int playerArea = gameRenderWorld->PointInArea(gameLocal.GetLocalPlayer()->GetEyePosition());

	for (e = 0; e < MAX_GENTITIES; e++)
	{
		check = gameLocal.entities[e];

		if (!check) {
			continue;
		}

		if (idStr::Icmp(check->GetClassname(), "idWorldspawn") == 0 || idStr::Icmp(check->GetClassname(), "idMeta") == 0 || idStr::Icmp(check->GetClassname(), "idPlayer") == 0)
			continue;

		if (check->GetPhysics()->GetClipModel()->GetOwner() != NULL)
		{
			if (check->GetPhysics()->GetClipModel()->GetOwner() == gameLocal.GetLocalPlayer())
				continue;
		}

		if (check->GetBindMaster() != NULL)
		{
			if (check->GetBindMaster() == gameLocal.GetLocalPlayer())
				continue;
		}

		if (check->name.Find("__") >= 0 || check->name.Find("player1_weapon") >= 0)
			continue;		

		
		int itemArea = gameLocal.AreaNumForEntity(check);
		if (itemArea != playerArea)
			continue;

		gameLocal.Printf("%-19s %-19s '%s'\n", check->GetEntityDefName(), check->GetClassname(), check->name.c_str());


		gameRenderWorld->DebugArrow(colorRed, gameLocal.GetLocalPlayer()->GetEyePosition(), check->GetPhysics()->GetOrigin(), 2, 60000);
		gameRenderWorld->DrawText(check->name.c_str(), check->GetPhysics()->GetOrigin() + idVec3(0, 0, 4), .15f, colorRed, gameLocal.GetLocalPlayer()->viewAxis, 1, 60000);


		count++;		
	}

	gameLocal.Printf("\n...%d entities\n", count);
	
}

static void Cmd_DoImpulse(const idCmdArgs &args)
{
	if ((args.Argc() < 2))
	{
		idStr impulseList = "usage: impulse <number>\n";

		//gameLocal.Printf("%-19s %-19s '%s'\n", check->GetEntityDefName(), check->GetClassname(), check->name.c_str());

		impulseList += va("%-38s %s\n", "13: make player enter fall state.",		"23: activate ventpurge.");
		impulseList += va("%-38s %s\n", "14: select next weapon.",					"24: increase escalation/end game.");
		impulseList += va("%-38s %s\n", "15: select previous weapon.",				"25: mission victory.");
		impulseList += va("%-38s %s\n", "16: -",									"26: display restart screen.");
		impulseList += va("%-38s %s\n", "17: -",									"27: unfreeze/unbind player.");
		impulseList += va("%-38s %s\n", "18: reset player camera pitch.",			"28: show event log.");
		impulseList += va("%-38s %s\n", "19: -",									"29: infofeed/fanfare UI message");
		impulseList += va("%-38s %s\n", "20: lockdown/unlockdown all airlocks.",	"30: inventory debug.");
		impulseList += va("%-38s %s\n", "21: jockey-ride who you're looking at.",	"31: unlock an inventory slot.");
		impulseList += va("%-38s %s\n", "22: show infowatch.",						"32: start a radio checkin.");
		
		gameLocal.Printf(impulseList);
		return;
	}

	int integer = atoi(args.Argv(1));
	if (integer == 0)
	{
		Cmd_DoImpulse(idCmdArgs());
		return;
	}

	gameLocal.GetLocalPlayer()->PerformImpulse(integer);

	return;
}

static void Cmd_SecurityCameraUnlock(const idCmdArgs &args)
{
	idEntity *ent;
	for (ent = gameLocal.securitycameraEntities.Next(); ent != NULL; ent = ent->securitycameraNode.Next())
	{
		if (!ent)
			continue;

		if (ent->IsType(idSecurityCamera::Type))
		{
			static_cast<idSecurityCamera *>(ent)->IsSpliced = true;
			common->Printf("Unlocking securitycamera: '%s'\n", ent->GetName());
		}
	}
}

static void Cmd_ListByName(const idCmdArgs& args)
{
	if ((args.Argc() < 2))
	{
		gameLocal.Printf("usage: listbyname <entityname>\n");
		return;
	}

	idEntity* ent;
	for (ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next())
	{
		if (!ent)
			return;

		const char* entname = ent->spawnArgs.GetString("name");
		if (entname[0] == '\0')
			continue;

		if (idStr::Icmp(entname, args.Args()) == 0)
		{
			gameRenderWorld->DebugBounds(colorGreen, ent->GetPhysics()->GetAbsBounds(), vec3_origin, 900000);
			gameRenderWorld->DrawText( ent->GetName(), ent->GetPhysics()->GetOrigin(), .1f, colorGreen, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, 900000);
			ent->ListByClassNameDebugDraw();
		}
	}
}

static void Cmd_ListByClassname(const idCmdArgs &args)
{
	if ((args.Argc() < 2))
	{
		gameLocal.Printf("usage: listbyclassname <classname>\n");
		return;
	}

	bool doWildcardsearch = false;
	idStr searchValue = args.Args();
	if (searchValue.Find('*') >= 0)
	{
		searchValue.Strip('*'); //remove the wildcard.
		searchValue.StripTrailingWhitespace();
		searchValue.StripLeading(" ");
		doWildcardsearch = true;
	}

	int counter = 0;

	idEntity *ent;
	for (ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next())
	{
		if (!ent)
			return;

		idStr classname = ent->spawnArgs.GetString("classname");
		
		bool isMatch  = false;

		if (doWildcardsearch)
		{
			if (classname.Find(searchValue.c_str()) >= 0 && classname.Length() > 0)
			{
				isMatch = true;
			}
		}
		else
		{
			if (idStr::Icmp(classname, searchValue) == 0)
			{
				isMatch = true;
			}
		}

		if (isMatch )
		{
			gameRenderWorld->DebugBounds(colorGreen, ent->GetPhysics()->GetAbsBounds(), vec3_origin, 900000);
			gameRenderWorld->DrawText(va("#%d %s", counter, ent->GetName()), ent->GetPhysics()->GetOrigin(), .1f, colorGreen, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, 900000);
			ent->ListByClassNameDebugDraw();
			counter++;
		}
	}

	gameLocal.Printf("Found %d instances of '%s'\n", counter, searchValue.c_str());
}

static void Cmd_Debugarrow_f(const idCmdArgs &args)
{
	// blendo eric: added more formatting and return info upon execution
	idVec3 arrowPos;

	if ((args.Argc() < 4))
	{
		gameLocal.Printf("usage: debugarrow <x> <y> <z>\n");
		return;
	}

	int posIndex = 0;
	int argIndex = 1;
	while (posIndex < 3 && argIndex < args.Argc())
	{
		idStr curStr = args.Argv(argIndex);
		if (curStr.IsNumeric())
		{
			arrowPos[posIndex] = atof(curStr.c_str());
			posIndex++;
		}
		argIndex++;
	}

	if (posIndex == 3)
	{
		gameLocal.Printf("drawing at: %f , %f , %f\n", arrowPos.x, arrowPos.y, arrowPos.z);
		gameRenderWorld->DebugArrow(colorGreen, arrowPos + idVec3(0, 0, 512), arrowPos, 8, 30000);
	}
	else
	{
		gameLocal.Warning("Could not format position!\n");
	}
}

static void Cmd_ToggleShadow_f(const idCmdArgs &args)
{
	trace_t tr;

	gameLocal.clip.TracePoint(tr, gameLocal.GetLocalPlayer()->GetEyePosition(),
		gameLocal.GetLocalPlayer()->GetEyePosition() + (gameLocal.GetLocalPlayer()->firstPersonViewAxis[0] * 1024),
		MASK_SHOT_RENDERMODEL, gameLocal.GetLocalPlayer());

	if (tr.fraction < 1 && tr.c.entityNum > 0 && tr.c.entityNum < MAX_GENTITIES - 2)
	{
		if (gameLocal.entities[tr.c.entityNum])
		{
			gameRenderWorld->DebugArrow(colorGreen, tr.endpos + (tr.c.normal * 8), tr.endpos, 2, 100);

			gameLocal.entities[tr.c.entityNum]->GetRenderEntity()->noShadow = !gameLocal.entities[tr.c.entityNum]->GetRenderEntity()->noShadow;
			gameLocal.entities[tr.c.entityNum]->UpdateVisuals();
		}
	}
}


static void Cmd_EntityNumber_f(const idCmdArgs &args)
{
	bool success = false;
	trace_t tr;

	gameLocal.clip.TracePoint(tr, gameLocal.GetLocalPlayer()->GetEyePosition(),
		gameLocal.GetLocalPlayer()->GetEyePosition() + (gameLocal.GetLocalPlayer()->firstPersonViewAxis[0] * 1024),
		MASK_SHOT_RENDERMODEL, gameLocal.GetLocalPlayer());

	if (tr.fraction < 1 && tr.c.entityNum > 0 && tr.c.entityNum < MAX_GENTITIES - 2)
	{
		if (gameLocal.entities[tr.c.entityNum])
		{
			gameRenderWorld->DrawText(gameLocal.entities[tr.c.entityNum]->GetName(), gameLocal.entities[tr.c.entityNum]->GetPhysics()->GetAbsBounds().GetCenter() + idVec3(0,0,12), 0.3f, colorWhite, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, 10000);
			gameRenderWorld->DrawText(va("%d", tr.c.entityNum), gameLocal.entities[tr.c.entityNum]->GetPhysics()->GetAbsBounds().GetCenter() + idVec3(0,0,-12), 0.7f, colorWhite, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, 10000);

			gameRenderWorld->DebugBounds(colorGreen, gameLocal.entities[tr.c.entityNum]->GetPhysics()->GetBounds(), gameLocal.entities[tr.c.entityNum]->GetPhysics()->GetOrigin(), 10000);
			success = true;
		}
	}

	if (!success)
	{
		common->Printf("g_entitynumber: print entity number of entity that you're looking at.\n");
	}
}

void Cmd_TestDecal_f(const idCmdArgs &args)
{
	if (args.Argc() < 2)
	{
		gameLocal.Printf("usage: testdecal <decalname> <size (optional)>\n");
		return;
	}

	const char *decalname = args.Argv(1);

	float decalSize = 40.0f;
	if (args.Argc() > 2)
	{
		decalSize = atof(args.Argv(2));
	}

	idVec3 forward = gameLocal.GetLocalPlayer()->viewAngles.ToForward();
	idVec3 playerEyePos = gameLocal.GetLocalPlayer()->GetEyePosition();

	trace_t groundTr;
	gameLocal.clip.TracePoint(groundTr, playerEyePos, playerEyePos + forward * 1024, MASK_SOLID, gameLocal.GetLocalPlayer());
	if (groundTr.fraction < 1)
	{
		gameLocal.ProjectDecal(groundTr.endpos, -groundTr.c.normal, 8.0f, true, decalSize, decalname);
	}
}

//Hotreload from DARKMOD
void Cmd_ReloadMap_f(const idCmdArgs& args)
{
	gameLocal.HotReloadMap();
}


void Cmd_ReloadLights_f(const idCmdArgs& args)
{
	gameLocal.ReloadLights();
}

void Cmd_Respawn_f( const idCmdArgs& args )
{
	idPlayer* player = gameLocal.GetLocalPlayer();
	if ( player )
	{
		player->HealAllWounds();
		player->Spawn();
	}
	else
	{
		gameLocal.Warning( "Cannot respawn player as there is no player!\n" );
	}
}

void Cmd_CallScript_f(const idCmdArgs& args)
{
	if (args.Argc() < 2)
	{
		gameLocal.Printf("usage: call <scriptfunctionname> <arg1> <arg2> <arg3>...\nPlease note that only entity arguments are currently supported.\n");
		return;
	}

	idStr functionName = args.Argv(1);

	gameLocal.RunMapScript(functionName, args);
}

void Cmd_EventLog_DumpToFile_f( const idCmdArgs& args )
{
    common->Printf("Starting eventlog dump...\n");

	gameLocal.InitEventLogFile( false );
	gameLocal.CloseEventLogFile();

	common->Printf("Eventlog .log dump done.\n");
}

void Cmd_DebugInventory_f( const idCmdArgs& args )
{
	idPlayer* player = gameLocal.GetLocalPlayer();
	if ( player )
	{
		int index = -1;
		if (args.Argc() > 0)
		{
			index = atoi(args.Argv(1));
		}

		player->DebugPrintInventory(index);
	}
}

void Cmd_DebugEndMission_f(const idCmdArgs& args)
{
	idEntity *metaEnt = gameLocal.FindEntity("meta1");

	if (metaEnt)
	{
		static_cast<idMeta *>(metaEnt)->StartPostGame();
	}
}

void Cmd_DebugTestParticle(const idCmdArgs& args)
{
	if (args.Argc() <= 1)
	{
		common->Printf("Usage: testparticle <particlename.prt>\n");
		return;
	}

	trace_t tr;
	gameLocal.clip.TracePoint(tr, gameLocal.GetLocalPlayer()->firstPersonViewOrigin, gameLocal.GetLocalPlayer()->firstPersonViewOrigin + gameLocal.GetLocalPlayer()->viewAngles.ToForward() * 100.0f, MASK_SOLID, NULL);


	idStr particlename = args.Argv(1);	
	gameLocal.DoParticle(particlename.c_str(), tr.endpos, vec3_origin, false);

}

void Cmd_DebugFuseboxUnlockAll(const idCmdArgs& args)
{
	//Unlock all fuseboxes.

	for (idEntity* ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next())
	{
		if (ent->IsType(idMaintPanel::Type))
		{
			static_cast<idMaintPanel*>(ent)->Unlock(false);
			ent->DoFrob(0, NULL);
		}
	}
}

void Cmd_DebugLocationCheck_f(const idCmdArgs& args)
{
	int count = 0;
	for (idEntity* ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next())
	{
		idLocationEntity* locEnt = gameLocal.LocationForEntity(ent);

		if (locEnt == nullptr)
		{
			gameLocal.Warning("no location for: '%s' (%.1f %.1f %.1f)", ent->GetName(), ent->GetPhysics()->GetOrigin().x, ent->GetName(), ent->GetPhysics()->GetOrigin().y, ent->GetName(), ent->GetPhysics()->GetOrigin().z);
			count++;
		}				
	}
	common->Printf("Found %d entities with no location.\n", count);
}

void Cmd_DebugSetCombatState(const idCmdArgs& args)
{
	if (args.Argc() <= 1)
	{
		common->Printf("Usage: debugSetCombatState (0,1)\n");
		return;
	}

	int desiredValue = atoi(args.Argv(1));
	if (desiredValue > 0)
		desiredValue = 2;

	idEntity* metaEnt = gameLocal.FindEntity("meta1");
	if (metaEnt)
	{
		static_cast<idMeta*>(metaEnt)->Event_SetCombatState(desiredValue);
	}
}


//Hide debug buttons at game start. Show the debug buttons if the player uses the DebugButtons command.
void Cmd_DebugButtons(const idCmdArgs& args)
{
	//Show/unhide the debug buttons.
	for (idEntity* ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next())
	{
		if (ent == nullptr)
			continue;

		if (ent->entityNumber <= 0 || ent->entityNumber >= ENTITYNUM_WORLD - 1)
			continue;

		if (!ent->spawnArgs.GetBool("debugbutton"))
			continue;

		ent->Show();
	}
}

//BC 3-11-2025: this is just a wrapper to make it easier for players to reset settings.
void Cmd_ResetSettings(const idCmdArgs& args)
{
	common->Printf("Settings have been reset to default settings.\n");
	cmdSystem->BufferCommandText(CMD_EXEC_NOW, "exec default.cfg");
}

void Cmd_DebugClearAchievements(const idCmdArgs& args)
{
	if (common->g_SteamUtilities && common->g_SteamUtilities->IsSteamInitialized())
	{
		common->g_SteamUtilities->ResetAchievements();
	}
}

void Cmd_TestSubtitle(const idCmdArgs& args)
{
	if (args.Argc() != 2) {
		common->Printf("Usage: testSubtitle <file>\n");
		return;
	}

	gameLocal.voManager.SayVO(gameLocal.GetLocalPlayer(), args.Argv(1), VO_CATEGORY_NARRATIVE);
}


void Cmd_LocateEntityViaName(const idCmdArgs& args)
{
	idStr		match;
	if (args.Argc() > 1) {
		match = args.Args();
	}
	else {
		common->Printf("Usage: locateentityvianame <substring>\n");
		return;
	}

	common->Printf("\n--- entity substring search for '%s' ---\n", match.c_str());
	int counter = 0;
	for (int i = 0; i < MAX_GENTITIES; i++)
	{
		idEntity* check = gameLocal.entities[i];

		if (!check) {
			continue;
		}

		if (check->name.Find(match, false) < 0 && match.Length() > 0)
		{
			continue;
		}

		//Draw debug.
		idBounds entityBounds = check->GetPhysics()->GetAbsBounds();
		gameRenderWorld->DebugBounds(colorGreen, entityBounds, vec3_origin, 500000);
		idAngles drawAngle = gameLocal.GetLocalPlayer()->viewAngles;
		gameRenderWorld->DrawText(check->name.c_str(), check->GetPhysics()->GetOrigin() + idVec3(0, 0, 60), 0.2f, colorGreen, drawAngle.ToMat3(), 1, 500000); //text string

		common->Printf("%d. Found match: %s\n", counter+1, check->name.c_str());
		counter++;

		gameRenderWorld->DebugArrow(colorGreen, check->GetPhysics()->GetOrigin() + idVec3(0, 0, 256), check->GetPhysics()->GetOrigin() + idVec3(0, 0, 70), 16, 500000);

		if (check->GetEntityDefName() != NULL)
		{
			if (check->GetEntityDefName()[0] != '\0')
			{
				gameRenderWorld->DrawText(idStr::Format("entitydef: %s", check->GetEntityDefName()).c_str(), check->GetPhysics()->GetOrigin() + idVec3(0, 0, 60), 0.2f, colorGreen, drawAngle.ToMat3(), 1, 500000); //text string
			}
		}

		if (check->GetClassname() != NULL)
		{
			if (check->GetClassname()[0] != '\0')
			{
				gameRenderWorld->DrawText(idStr::Format("classname: %s", check->GetClassname()).c_str(), check->GetPhysics()->GetOrigin() + idVec3(0, 0, 50), 0.2f, colorGreen, drawAngle.ToMat3(), 1, 500000); //text string
			}
		}

		idStr modelname = check->spawnArgs.GetString("model");
		if (modelname.Length() > 0)
		{			
			gameRenderWorld->DrawText(idStr::Format("model: %s", modelname.c_str()), check->GetPhysics()->GetOrigin() + idVec3(0, 0, 40), 0.2f, colorGreen, drawAngle.ToMat3(), 1, 500000); //text string
			
		}




	}


	common->Printf("\nFound %d matches.", counter);
}

void Cmd_DebugCryoExit(const idCmdArgs& args)
{
	//Player in cryo pod. Exit cryo pod.
	if (static_cast<idMeta*>(gameLocal.metaEnt.GetEntity())->GetPlayerExitedCryopod())
		return;

	//Find the cryo pod.
	for (idEntity* ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next())
	{
		if (!ent)
			continue;

		if (!ent->IsType(idCryointerior::Type))
			continue;

		common->Printf("Stresstest: frobbing cryointerior.\n");
		static_cast<idCryointerior*>(ent)->DoFrob(PEEKFROB_INDEX, gameLocal.GetLocalPlayer());
		return;
	}
}

void Cmd_DebugVisorComplete(const idCmdArgs& args)
{
	trace_t tr;

	gameLocal.clip.TracePoint(tr, gameLocal.GetLocalPlayer()->GetEyePosition(),
		gameLocal.GetLocalPlayer()->GetEyePosition() + (gameLocal.GetLocalPlayer()->firstPersonViewAxis[0] * 1024),
		MASK_SHOT_RENDERMODEL, gameLocal.GetLocalPlayer());

	if (tr.fraction < 1 && tr.c.entityNum > 0 && tr.c.entityNum < MAX_GENTITIES - 2)
	{
		if (gameLocal.entities[tr.c.entityNum])
		{
			if (gameLocal.entities[tr.c.entityNum]->IsType(idVRVisor::Type))
			{
				if (gameLocal.RunMapScript("debug_vrdone")) //Hardcoded to hub level.
				{
					idBounds entityBounds = gameLocal.entities[tr.c.entityNum]->GetPhysics()->GetAbsBounds();
					gameRenderWorld->DebugBounds(colorGreen, entityBounds, vec3_origin, 10000);
					static_cast<idVRVisor*>(gameLocal.entities[tr.c.entityNum])->SetExitVisor(); // Mark visor complete.
				}
				else
				{
					common->Warning("debugvisorcomplete: unable to run map script 'debug_vrdone'");
				}
			}
		}
	}	
}

void Cmd_DebugLevelMilestoneSet(const idCmdArgs& args)
{
	if (args.Argc() <= 1)
	{
		common->Printf("Usage: debugLevelMilestoneSet <milestoneIndex (0,1,2)>");
		return;
	}

	int desiredIdx = atoi(args.Argv(1));

	idEntity* metaEnt = gameLocal.FindEntity("meta1");
	if (metaEnt)
	{
		static_cast<idMeta*>(metaEnt)->SetMilestone(desiredIdx);
	}
}



void Cmd_DebugFuseboxCodes_f(const idCmdArgs& args)
{
	common->Printf("\n---- Fusebox codes ----\n");

	for (idEntity* ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next())
	{
		if (ent->IsType(idKeypad::Type))
		{
			idStr maintpanelType = "???";
			if (ent->targets.Num() > 0)
			{
				maintpanelType = ent->targets[0].GetEntity()->GetEntityDefName();
			}

			common->Printf("%-24s %s\n", maintpanelType.c_str(), ent->spawnArgs.GetString("code"));

			idAngles drawDir = ent->GetPhysics()->GetAxis().ToAngles();
			drawDir.roll = 0;
			drawDir.yaw += 180;
			drawDir.pitch = 0;

			gameRenderWorld->DrawText(ent->spawnArgs.GetString("code"), ent->GetPhysics()->GetOrigin() + idVec3(0, 0, 4), .5f, colorGreen, drawDir.ToMat3(), 1, 9000000);
		}
	}

	common->Printf("\n");
}

void Cmd_DebugRepairRoom_f(const idCmdArgs& args)
{
	idPlayer* player = gameLocal.GetLocalPlayer();	
	if (player == NULL)
	{
		common->Warning("No player found.");
		return;
	}

	idEntity *metaEnt = gameLocal.FindEntity("meta1");
	if (metaEnt == NULL)
	{
		common->Warning("No meta ent found.");
		return;
	}

	static_cast<idMeta *>(metaEnt)->RepairEntitiesInRoom(player->GetEyePosition());
}

void Cmd_DebugItemTally_f(const idCmdArgs& args)
{
	//if there's a substring search filter.
	idStr		match;
	if (args.Argc() > 1) {
		match = args.Args();
		match.Replace(" ", "");
	}
	else {
		match = "";
	}

	idList<idStr> itemTallyList;
	idMapFile* mapFile = gameLocal.GetLevelMap();
	if (mapFile)
	{
		common->Printf("-- DebugItemTally: '%s' --\n\n", mapFile->GetName());
	}

	for (int i = 0; i < MAX_GENTITIES; i++)
	{
		idEntity* check = gameLocal.entities[i];

		if (!check) {
			continue;
		}

		if (check->name.Find(match, false) < 0 && match.Length() > 0)
		{
			continue;
		}

		if (check->GetEntityDefName() == NULL)
			continue;

		if (check->GetEntityDefName()[0] == '\0')
			continue;

		if (idStr::Cmp(check->GetEntityDefName(), "*unknown*") == 0)
			continue;

		itemTallyList.Append(check->GetEntityDefName());
	}

	itemTallyList.Sort();

	idStr lastEntityName = "";
	int uniqueEntityTypes = 0;

	for (int i = 0; i < itemTallyList.Num(); i++)
	{
		int count = 0;

		for (int k = 0; k < itemTallyList.Num(); k++)
		{
			if (idStr::Icmp(itemTallyList[i], itemTallyList[k]) == 0)
				count++;
		}

		gameLocal.Printf("  %-30s Count: %d\n", itemTallyList[i].c_str(), count);

		if (idStr::Icmp(itemTallyList[i], lastEntityName) == 0)
			continue;

		lastEntityName = itemTallyList[i];
		uniqueEntityTypes++;
	}

	
	gameLocal.Printf("\nTotal entity types: %d. Total items: %d (Filter: '%s')\n", uniqueEntityTypes, itemTallyList.Num(), match.c_str());
}

void Cmd_DebugItemPlacement_f(const idCmdArgs& args)
{
	//if there's a substring search filter.
	idStr		match;
	if (args.Argc() > 1) {
		match = args.Args();
		match.Replace(" ", "");
	}
	else {
		match = "";
	}

	idList<idStr> itemTallyList;
	idMapFile* mapFile = gameLocal.GetLevelMap();
	if (mapFile)
	{
		common->Printf("-- DebugItemPlacement: '%s' --\n\n", mapFile->GetName());
	}

	int locationCount = 0;
	int itemCount = 0;

	for (idEntity *locEnt = gameLocal.spawnedEntities.Next(); locEnt != NULL; locEnt = locEnt->spawnNode.Next())
	{
		if (!locEnt->IsType(idLocationEntity::Type))
			continue;
		
		gameLocal.Printf("\nLOCATION #%d: %s\n", locationCount + 1, static_cast<idLocationEntity*>(locEnt)->GetLocation());
		int currentRoomLocNumber = locEnt->entityNumber;
		locationCount++;

		for (int i = 0; i < MAX_GENTITIES; i++)
		{
			idEntity* check = gameLocal.entities[i];

			if (!check) {
				continue;
			}

			if (check->name.Find(match, false) < 0 && match.Length() > 0)
			{
				continue;
			}

			if (check->GetEntityDefName() == NULL)
				continue;

			if (check->GetEntityDefName()[0] == '\0')
				continue;

			if (idStr::Cmp(check->GetEntityDefName(), "*unknown*") == 0)
				continue;

			idLocationEntity* itemLoc = gameLocal.LocationForEntity(check);
			if (!itemLoc)
				continue;

			if (itemLoc->entityNumber != currentRoomLocNumber)
				continue;				

			gameLocal.Printf("  %-24s (%s)\n", check->GetEntityDefName(), check->name.c_str());			
			itemCount++;
		}
	}

	gameLocal.Printf("\nTotal rooms: %d. Total items: %d (Filter: '%s')\n", locationCount, itemCount, match.c_str());
	
}

void Cmd_DebugEventLog_f(const idCmdArgs& args)
{
	#define ENTRIES_TO_MAKE 200
	for (int i = 0; i < ENTRIES_TO_MAKE; i++)
	{
		int randomInt = gameLocal.random.RandomInt(100, 999);

		idStr str;
		if (randomInt % 5 == 0)
			str = idStr::Format("%d %s", randomInt, common->GetLanguageDict()->GetString("#str_dispatch_a_checkin_start_01"));
		else if (randomInt % 3 == 0)
			str = idStr::Format("%d %s", randomInt, common->GetLanguageDict()->GetString("#str_dispatch_a_ventpurge_01"));
		else
			str = idStr::Format("%d", randomInt);

		gameLocal.AddEventLog(str.c_str(), vec3_zero);
	}

	common->Printf("Created %d event log entries.\n", ENTRIES_TO_MAKE);
}

//BC 4-25-2025: basic help command for someone new to the console
void Cmd_Help(const idCmdArgs& args)
{
	common->Printf("\nHi. You're in the Skin Deep developer console. Some hotkeys:\n - ctrl+alt+tilde = open console.\n - tilde          = close console.\n - up/down arrow  = command history.\n - tab            = autocomplete.\n\nTo exit to main menu, type in: disconnect\nTo exit to desktop, type in:   quit\n");
}

//BC 4-21-2025: debug to "unstuck" a player who may be stuck in geometry
void Cmd_UnstuckPlayer(const idCmdArgs& args)
{

	//#1: First, attempt to just find a nearby spot that has clearance.
	common->Printf("Attempting to unstuck player...\n");

	idVec3 originalPos = gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin();
	#define UNSTUCK_CANDIDATESPOT_COUNT 37
	idVec3 candidateSpots[] =
	{
		originalPos + idVec3(0,0,1),

		originalPos + idVec3(32,0,0),
		originalPos + idVec3(-32,0,0),
		originalPos + idVec3(0,32,0),
		originalPos + idVec3(0,-32,0),

		originalPos + idVec3(32,0,32),
		originalPos + idVec3(-32,0,32),
		originalPos + idVec3(0,32,32),
		originalPos + idVec3(0,-32,32),

		originalPos + idVec3(32,0,-32),
		originalPos + idVec3(-32,0,-32),
		originalPos + idVec3(0,32,-32),
		originalPos + idVec3(0,-32,-32),

		originalPos + idVec3(48,0,0),
		originalPos + idVec3(-48,0,0),
		originalPos + idVec3(0,48,0),
		originalPos + idVec3(0,-48,0),

		originalPos + idVec3(48,0,48),
		originalPos + idVec3(-48,0,48),
		originalPos + idVec3(0,48,48),
		originalPos + idVec3(0,-48,48),

		originalPos + idVec3(48,0,-48),
		originalPos + idVec3(-48,0,-48),
		originalPos + idVec3(0,48,-48),
		originalPos + idVec3(0,-48,-48),

		originalPos + idVec3(96,0,0),
		originalPos + idVec3(-96,0,0),
		originalPos + idVec3(0,96,0),
		originalPos + idVec3(0,-96,0),

		originalPos + idVec3(96,0,96),
		originalPos + idVec3(-96,0,96),
		originalPos + idVec3(0,96,96),
		originalPos + idVec3(0,-96,96),

		originalPos + idVec3(96,0,-96),
		originalPos + idVec3(-96,0,-96),
		originalPos + idVec3(0,96,-96),
		originalPos + idVec3(0,-96,-96),
	};

	idBounds playerbounds;
	playerbounds = gameLocal.GetLocalPlayer()->GetPhysics()->GetBounds();
	playerbounds.ExpandSelf(1.0f); //expand a little for safety sake.

	int bestCandidateIndex = -1;
	for (int i = 0; i < UNSTUCK_CANDIDATESPOT_COUNT; i++)
	{
		gameRenderWorld->DebugArrowSimple(candidateSpots[i]);

		//check if starting point is inside geometry.
		int penetrationContents = gameLocal.clip.Contents(candidateSpots[i], NULL, mat3_identity, CONTENTS_SOLID, NULL);
		if (penetrationContents & MASK_SOLID)
		{
			continue; //If it starts in solid, then exit.
		}

		//check bounds clearance.
		trace_t spawnTr;
		gameLocal.clip.TraceBounds(spawnTr, candidateSpots[i], candidateSpots[i], playerbounds, MASK_SOLID, NULL);
		if (spawnTr.fraction < 1)
		{
			continue;
		}

		//check if spot has a Location. We do this because we don't want to move the player into any empty pocket/crevice that is
		//not considered part of the playable area.
		idLocationEntity* locEnt = gameLocal.LocationForPoint(candidateSpots[i]);
		if (locEnt == nullptr)
		{
			continue;
		}

		//area is clear.
		bestCandidateIndex = i;
		break;
	}

	if (bestCandidateIndex >= 0)
	{
		//Found a valid spot.
		common->Printf("Found valid unstuck spot (nearby).\n");
		gameLocal.GetLocalPlayer()->GetPhysics()->SetOrigin(candidateSpots[bestCandidateIndex]);
		return;
	}


	//#2: fallback to using the level's location entities. Find the nearest one, as the crow flies.
	//This only applies to non-vignette levels.
	int closestLocDistance = 99999999;
	idEntity* closestLocation = nullptr;
	for (idEntity* ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next())
	{
		if (!ent)
			continue;

		if (!ent->IsType(idLocationEntity::Type))
			continue;

		float currentDist = (ent->GetPhysics()->GetOrigin() - gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin()).LengthFast();
		if (currentDist < closestLocDistance)
		{
			trace_t spawnTr;
			gameLocal.clip.TraceBounds(spawnTr, ent->GetPhysics()->GetOrigin(), ent->GetPhysics()->GetOrigin(), playerbounds, MASK_SOLID, NULL);
			if (spawnTr.fraction >= 1)
			{
				//area is clear.
				closestLocDistance = currentDist;
				closestLocation = ent;
			}			
		}
	}

	if (closestLocation != nullptr)
	{
		//Found a valid spot.
		common->Printf("Found valid unstuck spot (at location).\n");
		gameLocal.GetLocalPlayer()->GetPhysics()->SetOrigin(closestLocation->GetPhysics()->GetOrigin());
		return;
	}


	//#3: fallback to using the path corner
	int closestPathDistance = 99999999;
	idEntity* closestPath = nullptr;
	for (idEntity* ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next())
	{
		if (!ent)
			continue;

		if (!ent->IsType(idPathCorner::Type))
			continue;

		float currentDist = (ent->GetPhysics()->GetOrigin() - gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin()).LengthFast();
		if (currentDist < closestPathDistance)
		{
			trace_t spawnTr;
			gameLocal.clip.TraceBounds(spawnTr, ent->GetPhysics()->GetOrigin(), ent->GetPhysics()->GetOrigin(), playerbounds, MASK_SOLID, NULL);
			if (spawnTr.fraction >= 1)
			{
				//area is clear.
				closestPathDistance = currentDist;
				closestPath = ent;
			}			
		}
	}

	if (closestPath != nullptr)
	{
		//Found a valid spot.
		common->Printf("Found valid unstuck spot (at path_corner).\n");
		gameLocal.GetLocalPlayer()->GetPhysics()->SetOrigin(closestPath->GetPhysics()->GetOrigin());
		return;
	}

	//#4: fallback to using lights (desperation time)
	int closestLightDistance = 99999999;
	idEntity* closestLight = nullptr;
	for (idEntity* ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next())
	{
		if (!ent)
			continue;

		if (!ent->IsType(idPathCorner::Type))
			continue;

		float currentDist = (ent->GetPhysics()->GetOrigin() - gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin()).LengthFast();
		if (currentDist < closestLightDistance)
		{
			trace_t spawnTr;
			gameLocal.clip.TraceBounds(spawnTr, ent->GetPhysics()->GetOrigin(), ent->GetPhysics()->GetOrigin(), playerbounds, MASK_SOLID, NULL);
			if (spawnTr.fraction >= 1)
			{
				//area is clear.
				closestLightDistance = currentDist;
				closestLight = ent;
			}
		}
	}

	if (closestLight != nullptr)
	{
		//Found a valid spot.
		common->Printf("Found valid unstuck spot (at light).\n");
		gameLocal.GetLocalPlayer()->GetPhysics()->SetOrigin(closestLight->GetPhysics()->GetOrigin());
		return;
	}


	common->Printf("Failed to unstuck player. Please load a savegame or restart the level.\n");
}

int SortTypeInfoByLocID(const int* a, const int* b)
{
	return idStr::Icmp(gameLocal.entities[*a]->locID, gameLocal.entities[*b]->locID);
}

idList<int> GetAllNoteIndexes(idStr filter)
{
	idList<int> index;
	for (idEntity* ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next())
	{
		if (!ent)
			continue;

		if (!ent->IsType(idNoteWall::Type) && !ent->IsType(idTablet::Type))
			continue;

		if (ent->IsType(idNoteWall::Type))
		{
			if (static_cast<idNoteWall*>(ent)->IsMemorypalaceClone())
				continue;
		}

		if (filter.Length() > 0)
		{
			idStr stringName = ent->locID;
			if (stringName.Find(filter, false) < 0)
			{
				continue;
			}
		}

		if (ent->IsHidden())
		{
			ent->Show();

			//If note is hidden, note is probably part of a camerasplice transcript

			//Iterate over all security cameras.
			idEntity* cameraEnt = nullptr;
			for (idEntity* cameraCandidate = gameLocal.spawnedEntities.Next(); cameraCandidate != NULL; cameraCandidate = cameraCandidate->spawnNode.Next())
			{
				if (!cameraCandidate)
					continue;

				if (!cameraCandidate->IsType(idSecurityCamera::Type) || cameraCandidate->targets.Num() <= 0)
					continue;

				if (cameraCandidate->targets[0].GetEntity()->entityNumber == ent->entityNumber)
				{
					//found camera.
					cameraEnt = cameraCandidate;
					break;
				}				
			}

			if (cameraEnt == nullptr)
			{
				gameRenderWorld->DebugTextSimple("HIDDEN NOTE???", ent->GetPhysics()->GetOrigin() + idVec3(0, 0, 16), 500000, colorRed);
				continue;
			}

			//We have a camera. Now find the splice it belongs to.
			for (idEntity* spliceCandidate = gameLocal.spawnedEntities.Next(); spliceCandidate != NULL; spliceCandidate = spliceCandidate->spawnNode.Next())
			{
				if (!spliceCandidate)
					continue;

				if (!spliceCandidate->IsType(idCameraSplice::Type))
					continue;

				if (!static_cast<idCameraSplice*>(spliceCandidate)->assignedCamera.IsValid())
					continue;

				if (static_cast<idCameraSplice*>(spliceCandidate)->assignedCamera.GetEntity()->entityNumber == cameraEnt->entityNumber)
				{
					//found the camera splice the note is associated with.

					idVec3 forward;
					spliceCandidate->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, nullptr, nullptr);
					ent->SetOrigin(spliceCandidate->GetPhysics()->GetOrigin() + (forward * 16));
					ent->SetAxis(spliceCandidate->GetPhysics()->GetAxis());
				}
			}

			
		}

		//Print console.
		index.Append(ent->entityNumber);
	}

	index.Sort(SortTypeInfoByLocID); //Sort by the localization string id.

	return index;
}


void TeleportToViaEntityNumber(int entityNumber)
{
	if (entityNumber < 0 || entityNumber >= MAX_GENTITIES - 2)
		return;

	idEntity* noteEnt = gameLocal.entities[entityNumber];

	//Get note location.
	idVec3 notePos = noteEnt->GetPhysics()->GetOrigin();

	idVec3 forward;
	noteEnt->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, nullptr, nullptr);

	notePos = notePos + (forward * 64);
	trace_t tr;
	gameLocal.clip.TracePoint(tr, notePos, notePos + idVec3(0, 0, -80), MASK_SOLID, NULL);
	if (tr.fraction < 1)
		notePos = tr.endpos;

	idVec3 targetDir = noteEnt->GetPhysics()->GetOrigin() - idVec3(notePos.x, notePos.y, notePos.z + 64);
	idAngles targetAngle = targetDir.ToAngles();

	//teleport to the appropriate note.
	idPlayer* player;
	player = gameLocal.GetLocalPlayer();
	player->Teleport(notePos, targetAngle, NULL);

	idVec3 viewUp;
	player->viewAngles.ToVectors(nullptr, nullptr, &viewUp);

	gameRenderWorld->DrawText(noteEnt->locID.c_str(), noteEnt->GetPhysics()->GetOrigin() + viewUp * 8, 0.1f, colorGreen, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, 2000);
	gameRenderWorld->DebugBounds(colorGreen, idBounds(idVec3(-6, -6, -6), idVec3(6, 6, 6)), noteEnt->GetPhysics()->GetOrigin(), 2000);
}

void TeleportToNote(int orderIndex)
{
	idList<int> index = GetAllNoteIndexes("");

	if (index.Num() <= 0)
		return;

	if (orderIndex >= index.Num())
		orderIndex = 0;
	else if (orderIndex < 0)
		orderIndex = index.Num() - 1;

	gameLocal.lastDebugNoteIndex = orderIndex; //keep track of currently active index.

	int noteIndex = index[orderIndex];
	idEntity* noteEnt = gameLocal.entities[noteIndex];

	//Get note location.
	idVec3 notePos = noteEnt->GetPhysics()->GetOrigin();

	idVec3 forward;
	noteEnt->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, nullptr, nullptr);

	notePos = notePos + (forward * 64);
	trace_t tr;
	gameLocal.clip.TracePoint(tr, notePos, notePos + idVec3(0, 0, -80), MASK_SOLID, NULL);
	if (tr.fraction < 1)
		notePos = tr.endpos;

	idVec3 targetDir = noteEnt->GetPhysics()->GetOrigin() - idVec3(notePos.x, notePos.y, notePos.z + 64);
	idAngles targetAngle = targetDir.ToAngles();

	//teleport to the appropriate note.
	idPlayer* player;
	player = gameLocal.GetLocalPlayer();
	player->Teleport(notePos, targetAngle, NULL);

	idVec3 viewUp;
	player->viewAngles.ToVectors(nullptr, nullptr, &viewUp);

	common->Printf("#%-2d: %s\n", gameLocal.lastDebugNoteIndex + 1, noteEnt->locID.c_str());
	gameRenderWorld->DrawText(noteEnt->locID.c_str(), noteEnt->GetPhysics()->GetOrigin() + viewUp * 8, 0.1f, colorGreen, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, 2000);
	gameRenderWorld->DebugBounds(colorGreen, idBounds(idVec3(-6, -6, -6), idVec3(6, 6, 6)), noteEnt->GetPhysics()->GetOrigin(), 2000);
}

void TeleportToNoteViaDelta(int indexDelta)
{
	gameLocal.lastDebugNoteIndex += indexDelta;

	TeleportToNote(gameLocal.lastDebugNoteIndex);	
}

void Cmd_DebugNotesNext_f(const idCmdArgs& args)
{
	if (args.Argc() >= 2)
	{
		int desiredIdx = atoi(args.Argv(1));
		TeleportToNote(desiredIdx - 1);
		return;
	}

	TeleportToNoteViaDelta(1);
}

void Cmd_DebugNotesPrev_f(const idCmdArgs& args)
{
	if (args.Argc() >= 2)
	{
		int desiredIdx = atoi(args.Argv(1));
		TeleportToNote(desiredIdx - 1);
		return;
	}

	TeleportToNoteViaDelta(-1);
}



void Cmd_DebugNotesLocate_f(const idCmdArgs& args)
{
	idStr filter = "";
	//if (args.Argc() >= 2)
	//{
	//	//filter = args.Argv(1); //BC remove filter functionality
	//
	//	//teleport to the note number
	//	int desiredIdx = atoi(args.Argv(1));
	//	TeleportToNote(desiredIdx - 1);
	//	return;
	//}

	if (args.Argc() >= 2)
	{
		filter = args.Argv(1);
	}

	idList<int> index = GetAllNoteIndexes(filter);



	if (index.Num() <= 0)
		return;

	gameRenderWorld->DebugClearLines(0);
	gameRenderWorld->DebugClearPolygons(0);

	if (index.Num() == 1) //only one result....
	{
		//teleport to note.
		TeleportToViaEntityNumber(index[0]);
	}

	int count = 0;

	for (int i = 0; i < index.Num(); i++)
	{
		idEntity* ent = gameLocal.entities[index[i]];

		idStr stringName = ent->locID;
		if (stringName.Length() <= 0)
		{
			stringName = "[no string name]";
		}

		idStr locName;
		idLocationEntity* locEnt = gameLocal.LocationForEntity(ent);
		if (locEnt)
		{
			locName = locEnt->GetLocation();
		}
		else
		{
			locName = "[no location]";
		}

		common->Printf("#%-2d: %-29s %-15s %s\n", count + 1, stringName.c_str(), locName.c_str(), ent->GetName());

		//Draw arrow.
		gameRenderWorld->DebugArrow(colorGreen, ent->GetPhysics()->GetOrigin() + idVec3(0, 0, 64), ent->GetPhysics()->GetOrigin(), 4, 90000000);

		//Draw text.
		idAngles drawAngle = gameLocal.GetLocalPlayer()->viewAngles;
		drawAngle.pitch = 0;
		drawAngle.roll = 0;
		gameRenderWorld->DrawText(idStr::Format("#%d", count + 1).c_str(), ent->GetPhysics()->GetOrigin() + idVec3(0, 0, 90), 1.5f, colorGreen, drawAngle.ToMat3(), 1, 90000000); //number.

		gameRenderWorld->DrawText(stringName.c_str(), ent->GetPhysics()->GetOrigin() + idVec3(0, 0, 65), 0.2f, colorGreen, drawAngle.ToMat3(), 1, 90000000); //text string

		count++;
	}
}

void Cmd_DebugReadAllNotes_f(const idCmdArgs& args)
{
	int count = 0;
	for (idEntity *ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next())
	{
		if (!ent)
			continue;
	
		if (ent->IsType(idNoteWall::Type))
		{
			if (static_cast<idNoteWall*>(ent)->IsMemorypalaceClone())
				continue;

			static_cast<idNoteWall*>(ent)->SetRead();
			count++;
		}

		if (ent->IsType(idTablet::Type))
		{
			static_cast<idTablet*>(ent)->SetRead();
			count++;
		}		
	}

	common->Printf("Notes found: %d\n", count);
}

extern bool UnicodeCharacterZeroSpace(uint32 ch);
extern idCVar sys_lang;

//BC 3-25-2025: iterate over every string in the game. Print a warning if a given string has invalid characters.
//Can use https://www.cogsci.ed.ac.uk/~richard/utf-8.cgi?input=845&mode=decimal to identify glyphs.
void Cmd_LocalizeValidateFont_f(const idCmdArgs& args)
{
	bool isVerbose = false;
	if (args.Argc() >= 2)
	{
		idStr verbose = args.Argv(1);
		if (verbose == "verbose")
		{
			isVerbose = true;
		}
	}

	// Try to register each font (this is hardcoded, can change if we really want to)
	// This will automatically do the language font substitutions
	tr.RegisterFont("inocua");
	tr.RegisterFont("octin");
	tr.RegisterFont("octinbk");
	tr.RegisterFont("petme");
	tr.RegisterFont("roboto_condensed");
	tr.RegisterFont("sofia");
	tr.RegisterFont("teko");

	const idLangDict* strTable = common->GetLanguageDict();
	idHashTable<idList<int>> missingCodeTable;

	if (isVerbose)
	{
		common->Printf("-----------------------------------------------\n");
		common->Printf("Finding missing glyphs for sys_lang '%s'\n", sys_lang.GetString());
		common->Printf("-----------------------------------------------\n");
		common->Printf("font,key,index,decimal code point\n");
	}

	int totalKeys = strTable->GetNumKeyVals();
	for (int i = 0; i < tr.fonts.Num(); i++)
	{
		class idFont* font = tr.fonts[i];

		// Skip aliases because these don't actually have the font info
		if (!font || font->GetAlias())
			continue;

		idList<int> missingCodes;

		for (int j = 0; j < totalKeys; j++)
		{
			//Get the STRING value.
			idStr currentStr = strTable->GetKeyVal(j)->value;
			int charIndex = 0;
			while (charIndex < currentStr.Length()) {
				uint32 textChar = currentStr.UTF8Char(charIndex);

				// See if we need to start a new line.
				if (textChar == '\n' || textChar == '\r' || charIndex == currentStr.Length()) {
					if (charIndex < currentStr.Length()) {
						// New line character and we still have more text to read.
						char nextChar = currentStr[charIndex + 1];
						if ((textChar == '\n' && nextChar == '\r') || (textChar == '\r' && nextChar == '\n')) {
							// Just absorb extra newlines.
							textChar = currentStr.UTF8Char(charIndex);
						}
					}
				}

				// Check for escape colors
				if (textChar == C_COLOR_ESCAPE && charIndex < currentStr.Length()) {
					textChar = currentStr.UTF8Char(charIndex);
					textChar = currentStr.UTF8Char(charIndex);
				}

				// If the character isn't a new line then add it to the text buffer.
				if (textChar != '\n' && textChar != '\r' && !UnicodeCharacterZeroSpace(textChar)) {
					// If the glyph index is -1, that means it's not found in the font
					if (font->GetGlyphIndex(textChar) == -1)
					{
						if (isVerbose)
							common->Printf("%s,%s,%d,%d\n", font->GetName(), strTable->GetKeyVal(j)->key.c_str(), charIndex, textChar);
						
						missingCodes.AddUnique(textChar);
					}
				}
			}
		}

		if (missingCodes.Num() > 0)
			missingCodeTable.Set(font->GetName(), missingCodes);
	}

	common->Printf("-----------------------------------------------\n");
	common->Printf("Summary of missing glyphs for sys_lang '%s'\n", sys_lang.GetString());
	common->Printf("-----------------------------------------------\n");
	common->Printf("font,decimal code point\n");

	for (int i = 0; i < missingCodeTable.Num(); i++)
	{
		idStr key;
		idList<int> value;
		missingCodeTable.GetIndex(i, &key, &value);
		for (int j = 0; j < value.Num(); j++)
		{
			common->Printf("%s,%d\n", key.c_str(), value[j]);
		}
	}

}

void Cmd_LocalizeNotes_f(const idCmdArgs& args)
{
	int totalNotes = 0;
	int changedNotes = 0;

	if (args.Argc() <= 1)
	{
		common->Printf("Usage: localizeNotes <shortmapname>\n");
		return;
	}

	idStr shortmapname = args.Argv(1);
	idStrList guientitynames;
	for (idEntity* ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next())
	{
		if (!ent)
			continue;

		if (!ent->spawnArgs.GetBool("is_note"))
			continue;

		totalNotes++;

		guientitynames.Append(ent->GetName());
		
		changedNotes++;
	}

	//Sort the entities alphabetically, to group them up in a way that has some kind of order.
	guientitynames.Sort();

	//Start the CSV file.
	idStr csvFilename = idStr::Format("loc_notes_%s.csv", shortmapname.c_str());
	idFile* file = fileSystem->OpenFileWrite(csvFilename.c_str());

	common->Printf("============ GUI_PARM0 CHANGES ============\n");
	for (int i = 0; i < guientitynames.Num(); i++)
	{
		idEntity* ent = gameLocal.FindEntity(guientitynames[i]);

		if (!ent)
		{
			gameLocal.Error("failed to find: %s\n", guientitynames[i].c_str());
			return;
		}

		//make a little marker if the note was NOT localized.
		bool isAlreadyLocalized = idStr::FindText(ent->locID, "#str_") >= 0;

		idStr newKeyName = idStr::Format("#str_notes_%s_note%02d", shortmapname.c_str(), (i+1));
		file->Printf("%s|%s\n", newKeyName.c_str(), ent->spawnArgs.GetString("gui_parm0"));
		ent->spawnArgs.Set("gui_parm0", newKeyName);


		common->Printf("%-1s %-30s %s\n", (isAlreadyLocalized ? "" : "!"), guientitynames[i].c_str(), newKeyName.c_str());
	}

	fileSystem->CloseFile(file);

	common->Printf("\n");

	common->Printf("Notes found: %d\n\n", totalNotes);

	common->Printf("* Pipe-delimited csv written to file: %s\n", csvFilename.c_str());
	common->Printf("  (found in: C:/Users/YOURNAME/Documents/My Games/skindeep/base)\n\n");

	common->Printf("Press ctrl+shift+c to copy this console output.\n");

	//common->Printf("* Map file '%s' also saved with new loc IDs.\n\n", gameLocal.GetMapName());	
	//Cmd_SaveNotes_f(args);
}

void Cmd_DebugEscalation_f(const idCmdArgs& args)
{
	idEntity *metaEnt = gameLocal.FindEntity("meta1");
	
	if (!metaEnt)
		return;

	int totalcages = 0;
	int openedCages = 0;
	for (idEntity* cageEnt = gameLocal.catcageEntities.Next(); cageEnt != NULL; cageEnt = cageEnt->catcageNode.Next())
	{
		if (!cageEnt)
			continue;

		if (!cageEnt->IsType(idCatcage::Type))
			continue;

		totalcages++;

		if (static_cast<idCatcage *>(cageEnt)->IsOpened())
		{
			openedCages++;
		}
		else
		{
			static_cast<idCatcage *>(cageEnt)->ReleaseCat();
			return;
		}
	}

	if (openedCages < totalcages)
		return;
	



    //static_cast<idMeta *>(metaEnt)->StartReinforcementsSequence();
	if (static_cast<idMeta *>(metaEnt)->SpawnPirateShip())
		return;

	//static_cast<idMeta *>(metaEnt)->StartPostGame();

	for (idEntity* ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next())
	{
		if (!ent)
			continue;

		if (ent->IsType(idPirateship::Type))
		{
			static_cast<idPirateship *>(ent)->DebugFastForward();
		}
	}
}


void Cmd_DebugStressTest_f(const idCmdArgs& args)
{
	//First, see if stresstest is active. If so, delete it.

	common->Printf("** StressTest debug command **\n");

	for (idEntity *ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next())
	{
		if (!ent)
			continue;

		if (ent->name.Find("idStressTester_target_stresstester", false) >= 0)
		{
			//Found it.
			//ent->PostEventMS(&EV_Remove, 0);
			//cvarSystem->SetCVarBool("aas_randomPullPlayer", false);
			//common->Printf("Stresstest deactivated.");
			//return;

			//BC 4-5-2025: to simplify stresstest's interactions with save/load, have it just always delete pre-existing stresstest and re-create it.
			ent->PostEventMS(&EV_Remove, 0);
		}
	}

	const idDeclEntityDef* stressDef;
	stressDef = gameLocal.FindEntityDef("target_stresstester", false);
	if (stressDef)
	{
		idEntity* stressEnt;
		gameLocal.SpawnEntityDef(stressDef->dict, &stressEnt, false);
		common->Printf("");
	}
}

void Cmd_DebugPersistentClear(const idCmdArgs& args)
{
	common->Printf("Persistent level info: cleared.");
	gameLocal.persistentLevelInfo.Clear();
}

void Cmd_DebugPersistentSet(const idCmdArgs& args)
{
	if (args.Argc() <= 2)
	{
		common->Printf("Usage: debugPersistentSet <variablename> <value>\n");
		return;
	}

	idStr variablename = args.Argv(1);
	idStr valuename = args.Argv(2);

	common->Printf("Setting '%s' = '%s'\n", variablename.c_str(), valuename.c_str());
	gameLocal.persistentLevelInfo.Set(variablename.c_str(), valuename.c_str());
}

void Cmd_DebugLevelProgressionSet(const idCmdArgs& args)
{
	if (args.Argc() <= 1)
	{
		common->Printf("Usage: debugLevelProgressionSet <value>\n");
		common->Printf("(levelProgressIndex = %d)\n", gameLocal.GetLocalPlayer()->GetLevelProgressionIndex());
		return;
	}

	if (gameLocal.GetLocalPlayer() == nullptr)
	{
		gameLocal.Warning("Can only use 'debugLevelProgressionSet' when in a map.");
		return;
	}

	int value = atoi(args.Argv(1));
	common->Printf("Setting levelProgressIndex = '%d'\n", value);
	gameLocal.GetLocalPlayer()->DebugSetLevelProgressionIndex(value);
}



void Cmd_DebugEmailClear(const idCmdArgs& args)
{
	common->Printf("Clearing email memory.\n");
	int count = declManager->GetNumDecls(DECL_PDA); //total email count.
	for (int i = 0; i < count; i++)
	{
		const idDeclPDA* pda = static_cast<const idDeclPDA*>(declManager->DeclByIndex(DECL_PDA, i));
		pda->ResetEmails();
	}
}



//Debug: give every email in the game.
void Cmd_DebugEmailGiveAll(const idCmdArgs& args)
{
	idList<idStr> emailnames;

	int count = declManager->GetNumDecls(DECL_EMAIL); //total email count.
	for (int i = 0; i < count; i++)
	{
		const idDeclEntityDef* mapDef = static_cast<const idDeclEntityDef*>(declManager->DeclByIndex(DECL_EMAIL, i));
		if (mapDef)
		{
			idStr emailname = mapDef->GetName();
			if (emailname.Length() > 0)
			{
				emailnames.Append(emailname.c_str());
				
			}
		}
	}

	if (emailnames.Num() <= 0)
	{
		common->Warning("no emails found.");
		return;
	}

	//Sort by alphabetical.
	emailnames.Sort();
	for (int i = 0; i < emailnames.Num(); i++)
	{
		common->Printf("Giving email: '%s'\n", emailnames[i].c_str());
		gameLocal.GetLocalPlayer()->GiveEmail(emailnames[i].c_str());
	}
}

//Debug: pickpocket an item instantly.
void Cmd_DebugPickpocket(const idCmdArgs& args)
{
	if (args.Argc() <= 1)
	{
		common->Printf("Usage: debugpickpocket <entityname>\n");
		return;
	}

	idStr entityname = args.Argv(1);
	common->Printf("Pickpocketed entity: '%s'\n", entityname.c_str());

	idEntity* ent = gameLocal.FindEntity(args.Argv(1));
	if (ent)
	{
		gameLocal.GetLocalPlayer()->DoPickpocketSuccess(ent);
	}
	else
	{
		common->Warning("Failed to find pickpocket entity: '%s'", entityname.c_str());
	}
}

void Cmd_DebugEmailGive(const idCmdArgs& args)
{
	if (args.Argc() <= 1)
	{
		common->Printf("Usage: debugEmailGive <emailname>\n");
		return;
	}

	idStr emailname = args.Argv(1);
	common->Printf("Giving email: '%s'\n", emailname.c_str());
	gameLocal.GetLocalPlayer()->GiveEmail(emailname.c_str());
}

/*
=================
idGameLocal::InitConsoleCommands

Let the system know about all of our commands
so it can perform tab completion
=================
*/
void idGameLocal::InitConsoleCommands( void ) {
	cmdSystem->AddCommand( "listTypeInfo",			ListTypeInfo_f,				CMD_FL_GAME,				"list type info" );
	cmdSystem->AddCommand( "writeGameState",		WriteGameState_f,			CMD_FL_GAME,				"write game state" );
	cmdSystem->AddCommand( "testSaveGame",			TestSaveGame_f,				CMD_FL_GAME|CMD_FL_CHEAT,	"test a save game for a level" );
	cmdSystem->AddCommand( "game_memory",			idClass::DisplayInfo_f,		CMD_FL_GAME,				"displays game class info" );
	cmdSystem->AddCommand( "listClasses",			idClass::ListClasses_f,		CMD_FL_GAME,				"lists game classes" );
	cmdSystem->AddCommand( "listThreads",			idThread::ListThreads_f,	CMD_FL_GAME|CMD_FL_CHEAT,	"lists script threads" );
	cmdSystem->AddCommand( "listEntities",			Cmd_EntityList_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"lists game entities" );
	cmdSystem->AddCommand( "listActiveEntities",	Cmd_ActiveEntityList_f,		CMD_FL_GAME|CMD_FL_CHEAT,	"lists active game entities" );
	cmdSystem->AddCommand( "listMonsters",			idAI::List_f,				CMD_FL_GAME|CMD_FL_CHEAT,	"lists monsters" );
	cmdSystem->AddCommand( "listSpawnArgs",			Cmd_ListSpawnArgs_f,		CMD_FL_GAME|CMD_FL_CHEAT,	"list the spawn args of an entity", idGameLocal::ArgCompletion_EntityName );
	cmdSystem->AddCommand( "say",					Cmd_Say_f,					CMD_FL_GAME,				"text chat" );
	cmdSystem->AddCommand( "sayTeam",				Cmd_SayTeam_f,				CMD_FL_GAME,				"team text chat" );
	cmdSystem->AddCommand( "addChatLine",			Cmd_AddChatLine_f,			CMD_FL_GAME,				"internal use - core to game chat lines" );
	cmdSystem->AddCommand( "gameKick",				Cmd_Kick_f,					CMD_FL_GAME,				"same as kick, but recognizes player names" );
	cmdSystem->AddCommand( "give",					Cmd_Give_f,					CMD_FL_GAME|CMD_FL_CHEAT,	"gives one or more items" );
	cmdSystem->AddCommand( "centerview",			Cmd_CenterView_f,			CMD_FL_GAME,				"centers the view" );
	cmdSystem->AddCommand( "god",					Cmd_God_f,					CMD_FL_GAME|CMD_FL_CHEAT,	"enables god mode" );
	cmdSystem->AddCommand( "notarget",				Cmd_Notarget_f,				CMD_FL_GAME|CMD_FL_CHEAT,	"disables the player as a target" );
	cmdSystem->AddCommand( "noclip",				Cmd_Noclip_f,				CMD_FL_GAME|CMD_FL_CHEAT,	"disables collision detection for the player" );
	cmdSystem->AddCommand( "kill",					Cmd_Kill_f,					CMD_FL_GAME,				"kills the player" );
	cmdSystem->AddCommand( "where",					Cmd_GetViewpos_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"prints the current view position" );
	cmdSystem->AddCommand( "getviewpos",			Cmd_GetViewpos_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"prints the current view position" );
	cmdSystem->AddCommand( "setviewpos",			Cmd_SetViewpos_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"sets the current view position" );
	cmdSystem->AddCommand( "teleport",				Cmd_Teleport_f,				CMD_FL_GAME|CMD_FL_CHEAT,	"teleports the player to an entity location", idGameLocal::ArgCompletion_EntityName );
	cmdSystem->AddCommand( "trigger",				Cmd_Trigger_f,				CMD_FL_GAME|CMD_FL_CHEAT,	"triggers an entity", idGameLocal::ArgCompletion_EntityName );
	cmdSystem->AddCommand( "spawn",					Cmd_Spawn_f,				CMD_FL_GAME|CMD_FL_CHEAT,	"spawns a game entity", idCmdSystem::ArgCompletion_Decl<DECL_ENTITYDEF> );
	cmdSystem->AddCommand( "damage",				Cmd_Damage_f,				CMD_FL_GAME|CMD_FL_CHEAT,	"apply damage to an entity", idGameLocal::ArgCompletion_EntityName );
	cmdSystem->AddCommand( "remove",				Cmd_Remove_f,				CMD_FL_GAME|CMD_FL_CHEAT,	"removes an entity", idGameLocal::ArgCompletion_EntityName );
	cmdSystem->AddCommand( "killMonsters",			Cmd_KillMonsters_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"removes all monsters" );
	cmdSystem->AddCommand( "killMoveables",			Cmd_KillMovables_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"removes all moveables" );
	cmdSystem->AddCommand( "killRagdolls",			Cmd_KillRagdolls_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"removes all ragdolls" );
	cmdSystem->AddCommand( "addline",				Cmd_AddDebugLine_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"adds a debug line" );
	cmdSystem->AddCommand( "addarrow",				Cmd_AddDebugLine_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"adds a debug arrow" );
	cmdSystem->AddCommand( "removeline",			Cmd_RemoveDebugLine_f,		CMD_FL_GAME|CMD_FL_CHEAT,	"removes a debug line" );
	cmdSystem->AddCommand( "blinkline",				Cmd_BlinkDebugLine_f,		CMD_FL_GAME|CMD_FL_CHEAT,	"blinks a debug line" );
	cmdSystem->AddCommand( "listLines",				Cmd_ListDebugLines_f,		CMD_FL_GAME|CMD_FL_CHEAT,	"lists all debug lines" );
	cmdSystem->AddCommand( "playerModel",			Cmd_PlayerModel_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"sets the given model on the player", idCmdSystem::ArgCompletion_Decl<DECL_MODELDEF> );
	cmdSystem->AddCommand( "testFx",				Cmd_TestFx_f,				CMD_FL_GAME|CMD_FL_CHEAT,	"tests an FX system", idCmdSystem::ArgCompletion_Decl<DECL_FX> );
	cmdSystem->AddCommand( "testBoneFx",			Cmd_TestBoneFx_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"tests an FX system bound to a joint", idCmdSystem::ArgCompletion_Decl<DECL_FX> );
	cmdSystem->AddCommand( "testLight",				Cmd_TestLight_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"tests a light" );
	cmdSystem->AddCommand( "testPointLight",		Cmd_TestPointLight_f,		CMD_FL_GAME|CMD_FL_CHEAT,	"tests a point light" );
	cmdSystem->AddCommand( "popLight",				Cmd_PopLight_f,				CMD_FL_GAME|CMD_FL_CHEAT,	"removes the last created light" );
	cmdSystem->AddCommand( "testDeath",				Cmd_TestDeath_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"tests death" );
	cmdSystem->AddCommand( "testSave",				Cmd_TestSave_f,				CMD_FL_GAME|CMD_FL_CHEAT,	"writes out a test savegame" );
	cmdSystem->AddCommand( "testModel",				idTestModel::TestModel_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"tests a model", idTestModel::ArgCompletion_TestModel );
	cmdSystem->AddCommand( "testSkin",				idTestModel::TestSkin_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"tests a skin on an existing testModel", idCmdSystem::ArgCompletion_Decl<DECL_SKIN> );
	cmdSystem->AddCommand( "testShaderParm",		idTestModel::TestShaderParm_f,		CMD_FL_GAME|CMD_FL_CHEAT,	"sets a shaderParm on an existing testModel" );
	cmdSystem->AddCommand( "keepTestModel",			idTestModel::KeepTestModel_f,		CMD_FL_GAME|CMD_FL_CHEAT,	"keeps the last test model in the game" );
	cmdSystem->AddCommand( "testAnim",				idTestModel::TestAnim_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"tests an animation", idTestModel::ArgCompletion_TestAnim );
	cmdSystem->AddCommand( "testParticleStopTime",	idTestModel::TestParticleStopTime_f,CMD_FL_GAME|CMD_FL_CHEAT,	"tests particle stop time on a test model" );
	cmdSystem->AddCommand( "nextAnim",				idTestModel::TestModelNextAnim_f,	CMD_FL_GAME|CMD_FL_CHEAT,	"shows next animation on test model" );
	cmdSystem->AddCommand( "prevAnim",				idTestModel::TestModelPrevAnim_f,	CMD_FL_GAME|CMD_FL_CHEAT,	"shows previous animation on test model" );
	cmdSystem->AddCommand( "nextFrame",				idTestModel::TestModelNextFrame_f,	CMD_FL_GAME|CMD_FL_CHEAT,	"shows next animation frame on test model" );
	cmdSystem->AddCommand( "prevFrame",				idTestModel::TestModelPrevFrame_f,	CMD_FL_GAME|CMD_FL_CHEAT,	"shows previous animation frame on test model" );
	cmdSystem->AddCommand( "testBlend",				idTestModel::TestBlend_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"tests animation blending" );
	cmdSystem->AddCommand( "reloadScript",			Cmd_ReloadScript_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"reloads scripts" );
	cmdSystem->AddCommand( "script",				Cmd_Script_f,				CMD_FL_GAME|CMD_FL_CHEAT,	"executes a line of script" );
	cmdSystem->AddCommand( "listCollisionModels",	Cmd_ListCollisionModels_f,	CMD_FL_GAME,				"lists collision models" );
	cmdSystem->AddCommand( "collisionModelInfo",	Cmd_CollisionModelInfo_f,	CMD_FL_GAME,				"shows collision model info" );
	cmdSystem->AddCommand( "reexportmodels",		Cmd_ReexportModels_f,		CMD_FL_GAME|CMD_FL_CHEAT,	"reexports models", ArgCompletion_DefFile );
	cmdSystem->AddCommand( "reloadanims",			Cmd_ReloadAnims_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"reloads animations" );
	cmdSystem->AddCommand( "listAnims",				Cmd_ListAnims_f,			CMD_FL_GAME,				"lists all animations" );
	cmdSystem->AddCommand( "aasStats",				Cmd_AASStats_f,				CMD_FL_GAME,				"shows AAS stats" );
	cmdSystem->AddCommand( "testDamage",			Cmd_TestDamage_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"tests a damage def", idCmdSystem::ArgCompletion_Decl<DECL_ENTITYDEF> );
	cmdSystem->AddCommand( "weaponSplat",			Cmd_WeaponSplat_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"projects a blood splat on the player weapon" );
	cmdSystem->AddCommand( "saveSelected",			Cmd_SaveSelected_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"saves the selected entity to the .map file" );
	cmdSystem->AddCommand( "deleteSelected",		Cmd_DeleteSelected_f,		CMD_FL_GAME|CMD_FL_CHEAT,	"deletes selected entity" );
	cmdSystem->AddCommand( "saveMoveables",			Cmd_SaveMoveables_f,		CMD_FL_GAME|CMD_FL_CHEAT,	"save all moveables to the .map file" );
	cmdSystem->AddCommand( "saveRagdolls",			Cmd_SaveRagdolls_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"save all ragdoll poses to the .map file" );
	cmdSystem->AddCommand( "bindRagdoll",			Cmd_BindRagdoll_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"binds ragdoll at the current drag position" );
	cmdSystem->AddCommand( "unbindRagdoll",			Cmd_UnbindRagdoll_f,		CMD_FL_GAME|CMD_FL_CHEAT,	"unbinds the selected ragdoll" );
	cmdSystem->AddCommand( "saveLights",			Cmd_SaveLights_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"saves all lights to the .map file" );
	cmdSystem->AddCommand( "saveParticles",			Cmd_SaveParticles_f,		CMD_FL_GAME|CMD_FL_CHEAT,	"saves all particles to the .map file" );
	cmdSystem->AddCommand( "clearLights",			Cmd_ClearLights_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"clears all lights. 1 = only hide noshadow lights." );	
	cmdSystem->AddCommand( "gameError",				Cmd_GameError_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"causes a game error" );	


	cmdSystem->AddCommand( "disasmScript",			Cmd_DisasmScript_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"disassembles script" );
	cmdSystem->AddCommand( "recordViewNotes",		Cmd_RecordViewNotes_f,		CMD_FL_GAME|CMD_FL_CHEAT,	"record the current view position with notes" );
	cmdSystem->AddCommand( "showViewNotes",			Cmd_ShowViewNotes_f,		CMD_FL_GAME|CMD_FL_CHEAT,	"show any view notes for the current map, successive calls will cycle to the next note" );
	cmdSystem->AddCommand( "closeViewNotes",		Cmd_CloseViewNotes_f,		CMD_FL_GAME|CMD_FL_CHEAT,	"close the view showing any notes for this map" );
	cmdSystem->AddCommand( "exportmodels",			Cmd_ExportModels_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"exports models", ArgCompletion_DefFile );

	// multiplayer client commands ( replaces old impulses stuff )
	cmdSystem->AddCommand( "clientDropWeapon",		idMultiplayerGame::DropWeapon_f, CMD_FL_GAME,			"drop current weapon" );
	cmdSystem->AddCommand( "clientMessageMode",		idMultiplayerGame::MessageMode_f, CMD_FL_GAME,			"ingame gui message mode" );
	// FIXME: implement
//	cmdSystem->AddCommand( "clientVote",			idMultiplayerGame::Vote_f,	CMD_FL_GAME,				"cast your vote: clientVote yes | no" );
//	cmdSystem->AddCommand( "clientCallVote",		idMultiplayerGame::CallVote_f,	CMD_FL_GAME,			"call a vote: clientCallVote si_.. proposed_value" );
	cmdSystem->AddCommand( "clientVoiceChat",		idMultiplayerGame::VoiceChat_f,	CMD_FL_GAME,			"voice chats: clientVoiceChat <sound shader>" );
	cmdSystem->AddCommand( "clientVoiceChatTeam",	idMultiplayerGame::VoiceChatTeam_f,	CMD_FL_GAME,		"team voice chats: clientVoiceChat <sound shader>" );

	// multiplayer server commands
	cmdSystem->AddCommand( "serverMapRestart",		idGameLocal::MapRestart_f,	CMD_FL_GAME,				"restart the current game" );
	cmdSystem->AddCommand( "serverForceReady",	idMultiplayerGame::ForceReady_f,CMD_FL_GAME,				"force all players ready" );
	cmdSystem->AddCommand( "serverNextMap",			idGameLocal::NextMap_f,		CMD_FL_GAME,				"change to the next map" );

	// localization help commands
	cmdSystem->AddCommand( "nextGUI",				Cmd_NextGUI_f,				CMD_FL_GAME|CMD_FL_CHEAT,	"teleport the player to the next func_static with a gui" );
	cmdSystem->AddCommand( "testid",				Cmd_TestId_f,				CMD_FL_GAME|CMD_FL_CHEAT,	"output the string for the specified id." );
	cmdSystem->AddCommand( "testLocStringID",		Cmd_TestId_f,				CMD_FL_GAME | CMD_FL_CHEAT, "output the string for the specified id."); //This is just a duplicate, as "testID" is not very descriptive.

#ifdef _D3XP
	cmdSystem->AddCommand( "setActorState",			Cmd_SetActorState_f,		CMD_FL_GAME|CMD_FL_CHEAT,	"Manually sets an actors script state", idGameLocal::ArgCompletion_EntityName );
#endif


	//BC
	cmdSystem->AddCommand("debugArrow",				Cmd_Debugarrow_f, CMD_FL_GAME, "Draw debug arrow at x y z");
	cmdSystem->AddCommand("hideall",				Cmd_HideAll_f, CMD_FL_GAME, "Hide all entities.");
	cmdSystem->AddCommand("toggleshadow",			Cmd_ToggleShadow_f, CMD_FL_GAME | CMD_FL_CHEAT, "Toggle shadows of entity you're looking at.");
	cmdSystem->AddCommand("g_entitynumber",			Cmd_EntityNumber_f, CMD_FL_GAME, "Print entity number of entity that you're looking at.");
	cmdSystem->AddCommand("listEntitiesVisible",	Cmd_ListEntitiesVisible_f, CMD_FL_GAME, "List entities that are currently in your PVS.");
	cmdSystem->AddCommand("damageAll",				Cmd_DamageAll_f, CMD_FL_GAME | CMD_FL_CHEAT, "Apply generic damage to every entity in map.");
	cmdSystem->AddCommand("testdecal",				Cmd_TestDecal_f, CMD_FL_GAME | CMD_FL_CHEAT, "Create decal at crosshair location.", idCmdSystem::ArgCompletion_Decl<DECL_MATERIAL>);
	cmdSystem->AddCommand("clearDebug",				Cmd_ClearDebug_f, CMD_FL_GAME, "clears all debug lines.");
	cmdSystem->AddCommand("killEntity",				Cmd_KillEntity_f, CMD_FL_GAME, "deletes whatever is at crosshair.");
	cmdSystem->AddCommand("debugCavity",			Cmd_DebugCavity_f, CMD_FL_GAME, "Checks if all entities are within a space that has a location entity.");
	cmdSystem->AddCommand("debugClipContents",		Cmd_DebugClipContents_f, CMD_FL_GAME, "Returns result of player clip contents at position" );
	cmdSystem->AddCommand("listByClassname",		Cmd_ListByClassname, CMD_FL_GAME, "List and display entities by classname.", idCmdSystem::ArgCompletion_Decl<DECL_ENTITYDEF>);
	cmdSystem->AddCommand("listByName",				Cmd_ListByName, CMD_FL_GAME, "List and display entities by name.");
	cmdSystem->AddCommand("g_securitycameraunlock", Cmd_SecurityCameraUnlock, CMD_FL_GAME, "Unlock all security cameras.");
	cmdSystem->AddCommand("impulse",				Cmd_DoImpulse, CMD_FL_GAME, "Debug for impulse commands. No parameter = list all impulses.");
	cmdSystem->AddCommand("respawn",				Cmd_Respawn_f, CMD_FL_GAME | CMD_FL_CHEAT, "Respawn player");
	cmdSystem->AddCommand("call",					Cmd_CallScript_f, CMD_FL_GAME | CMD_FL_CHEAT, "Call a map script function.");
	cmdSystem->AddCommand("damageEntity",			Cmd_DamageEntity_f, CMD_FL_GAME, "damages whatever is at crosshair.");
	cmdSystem->AddCommand("debugInventory",			Cmd_DebugInventory_f, CMD_FL_GAME, "prints info about player inventory for debugging.");
	cmdSystem->AddCommand("debugEndMission",		Cmd_DebugEndMission_f, CMD_FL_GAME | CMD_FL_CHEAT, "Ends mission.");
	cmdSystem->AddCommand("debugEscalation",		Cmd_DebugEscalation_f, CMD_FL_GAME, "Proceeds to next mission stage.");
	cmdSystem->AddCommand("debugNotesReadAll",		Cmd_DebugReadAllNotes_f, CMD_FL_GAME | CMD_FL_CHEAT, "Reads all notes in level.");
	cmdSystem->AddCommand("debugNotesLocate",		Cmd_DebugNotesLocate_f, CMD_FL_GAME | CMD_FL_CHEAT, "Identify note locations. Can add argument for loc string substring search.");
	cmdSystem->AddCommand("debugNotesNext",			Cmd_DebugNotesNext_f, CMD_FL_GAME | CMD_FL_CHEAT, "Teleports player to next note.");
	cmdSystem->AddCommand("debugNotesPrev",			Cmd_DebugNotesPrev_f, CMD_FL_GAME | CMD_FL_CHEAT, "Teleports player to previous note.");
	cmdSystem->AddCommand("localizeNotes",			Cmd_LocalizeNotes_f, CMD_FL_GAME | CMD_FL_CHEAT, "Localize notes in level.");
	cmdSystem->AddCommand("localizeValidateFont",	Cmd_LocalizeValidateFont_f, CMD_FL_GAME | CMD_FL_CHEAT, "Validate the font has all needed characters for strings.");
	cmdSystem->AddCommand("debugEventLog",			Cmd_DebugEventLog_f, CMD_FL_GAME, "Creates random eventlog events.");
	cmdSystem->AddCommand("debugRepairRoom",		Cmd_DebugRepairRoom_f, CMD_FL_GAME, "Summons repairbot to repair things in room.");
	cmdSystem->AddCommand("debugItemPlacement",		Cmd_DebugItemPlacement_f, CMD_FL_GAME | CMD_FL_CHEAT, "Prints item placement metrics.");
	cmdSystem->AddCommand("debugItemTally",			Cmd_DebugItemTally_f, CMD_FL_GAME | CMD_FL_CHEAT, "Prints item quantity metrics.");
	cmdSystem->AddCommand("yawview",				Cmd_YawView_f, CMD_FL_GAME, "Snaps yaw to specific angle.");
	cmdSystem->AddCommand("eventlog_dumptofile",	Cmd_EventLog_DumpToFile_f, CMD_FL_GAME, "Dump event log to a file");
	cmdSystem->AddCommand("stresstest",				Cmd_DebugStressTest_f, CMD_FL_GAME | CMD_FL_CHEAT, "Toggles the stress test robot.");
	cmdSystem->AddCommand("debugEmailClear",		Cmd_DebugEmailClear, CMD_FL_GAME | CMD_FL_CHEAT, "Forgets all emails.");
	cmdSystem->AddCommand("debugEmailGiveAll",		Cmd_DebugEmailGiveAll, CMD_FL_GAME | CMD_FL_CHEAT, "Give all emails.");
	cmdSystem->AddCommand("debugEmailGive",			Cmd_DebugEmailGive, CMD_FL_GAME | CMD_FL_CHEAT, "Give 1 specific email.");
	cmdSystem->AddCommand("debugPersistentClear",	Cmd_DebugPersistentClear, CMD_FL_GAME | CMD_FL_CHEAT, "Clear persistent arg info.");
	cmdSystem->AddCommand("debugPersistentSet",		Cmd_DebugPersistentSet, CMD_FL_GAME | CMD_FL_CHEAT, "Set persistent arg info.");
	cmdSystem->AddCommand("debugLevelProgressionSet", Cmd_DebugLevelProgressionSet, CMD_FL_GAME | CMD_FL_CHEAT, "Set level progress index.");
	cmdSystem->AddCommand("debugFuseboxCodes",		Cmd_DebugFuseboxCodes_f, CMD_FL_GAME | CMD_FL_CHEAT, "Reads all notes in level.");
	cmdSystem->AddCommand("debugLevelMilestoneSet", Cmd_DebugLevelMilestoneSet, CMD_FL_GAME | CMD_FL_CHEAT, "Set milestone for current level.");
	cmdSystem->AddCommand("debugFuseboxUnlockAll",	Cmd_DebugFuseboxUnlockAll, CMD_FL_GAME | CMD_FL_CHEAT, "unlock all fuseboxes.");
	cmdSystem->AddCommand("debugPickpocket",		Cmd_DebugPickpocket, CMD_FL_GAME | CMD_FL_CHEAT, "Pickpocket an entity in the world.");
	cmdSystem->AddCommand("testparticle",			Cmd_DebugTestParticle, CMD_FL_GAME | CMD_FL_CHEAT, "test particle effect.", idCmdSystem::ArgCompletion_Decl<DECL_PARTICLE>);
	cmdSystem->AddCommand("debugSetCombatState",	Cmd_DebugSetCombatState, CMD_FL_GAME | CMD_FL_CHEAT, "Set combat state on/off.");
	cmdSystem->AddCommand("debugVisorComplete",		Cmd_DebugVisorComplete, CMD_FL_GAME | CMD_FL_CHEAT, "Set vr visor to be marked complete.");
	cmdSystem->AddCommand("debugButtons",			Cmd_DebugButtons, CMD_FL_GAME | CMD_FL_CHEAT, "Show/unhide the in-game debug buttons.");
	cmdSystem->AddCommand("debugCryoExit",			Cmd_DebugCryoExit, CMD_FL_GAME | CMD_FL_CHEAT, "Force exit of cryo pod.");

	cmdSystem->AddCommand("locateEntityViaName",	Cmd_LocateEntityViaName, CMD_FL_GAME | CMD_FL_CHEAT, "Locate entity via its name.");
	cmdSystem->AddCommand("locateEntityViaDef",		Cmd_ListByClassname, CMD_FL_GAME | CMD_FL_CHEAT, "Locate entity via its def name.", idCmdSystem::ArgCompletion_Decl<DECL_ENTITYDEF>);
	cmdSystem->AddCommand("testSubtitle",			Cmd_TestSubtitle, CMD_FL_GAME | CMD_FL_CHEAT, "test subtitle", idCmdSystem::ArgCompletion_SoundName);

	cmdSystem->AddCommand("debugLocationCheck",		Cmd_DebugLocationCheck_f, CMD_FL_GAME, "Validates room location info for all entities.");

	cmdSystem->AddCommand("debugClearAchievements",	Cmd_DebugClearAchievements, CMD_FL_GAME | CMD_FL_CHEAT, "Reset steam achievements.");

	cmdSystem->AddCommand("resetSettings",			Cmd_ResetSettings, CMD_FL_GAME , "Reset your settings to default.");
	cmdSystem->AddCommand("unstuck",				Cmd_UnstuckPlayer, CMD_FL_GAME, "Debug to un-stuck player if they're trapped in geometry.");
	cmdSystem->AddCommand("help",					Cmd_Help, CMD_FL_GAME, "Help command.");


	//Hotreload from DARKMOD
	cmdSystem->AddCommand("reloadMap",				Cmd_ReloadMap_f, CMD_FL_GAME, "Reload .map file and try to update running game accordingly.");	
	cmdSystem->AddCommand("reloadLights",			Cmd_ReloadLights_f, CMD_FL_GAME, "Reload .map file light entities.");
	cmdSystem->AddCommand("killPlayerLight",		Cmd_KillPlayerLight_f, CMD_FL_GAME | CMD_FL_CHEAT, "remove ambient light attached to player.");
}

/*
=================
idGameLocal::ShutdownConsoleCommands
=================
*/
void idGameLocal::ShutdownConsoleCommands( void ) {
	cmdSystem->RemoveFlaggedCommands( CMD_FL_GAME );

}


