
#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"
// RAVEN BEGIN
#include "../ai/AI.h"
#if !defined(__GAME_PROJECTILE_H__)
	#include "../Projectile.h"
#endif
#if !defined(__GAME_WEAPON_H__)
	#include "../Weapon.h"
#endif
#if !defined(__GAME_SPAWNER_H__)
	#include "../spawner.h"
#endif
#if !defined(__GAME_VEHICLE_H__)
	#include "../Vehicle/Vehicle.h"
#endif
#if !defined(__AI_MANAGER_H__)
	#include "../ai/AI_Manager.h"
#endif
#if !defined(__INSTANCE_H__)
	#include "../Instance.h"
#endif
// RAVEN END

#ifdef _WIN32
#include "TypeInfo.h"
#else
#include "NoGameTypeInfo.h"
#endif

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

		if ( !check->name.Filter( match ) ) {
			continue;
		}

		gameLocal.Printf( "%4i: %-20s %-20s %s\n", e,
			check->GetEntityDefName(), check->GetClassname(), check->name.c_str() );

		count++;
		size += check->spawnArgs.Allocated();
	}

	gameLocal.Printf( "...%d entities\n...%d bytes of spawnargs\n", count, size );
}

/*
===================
Cmd_ClientEntityList_f
===================
*/
void Cmd_ClientEntityList_f( const idCmdArgs &args ) {
	int			e;
	rvClientEntity	*check;
	int			count;
	idStr		match;

	if ( args.Argc() > 1 ) {
		match = args.Args();
		match.Replace( " ", "" );
	} else {
		match = "";
	}

	count = 0;

	gameLocal.Printf( "%-4s  %-20s\n", " Num", "Classname" );
	gameLocal.Printf( "--------------------------------------------------------------------\n" );
	for( e = 0; e < MAX_CENTITIES; e++ ) {
		check = gameLocal.clientEntities[ e ];

		idStr name( check->GetClassType().classname );

		if ( !check ) {
			continue;
		}

		if ( !name.Filter( match ) ) {
			continue;
		}
				
		gameLocal.Printf( "%4i: %-20s\n", e, name.c_str() );

		count++;
	}

	gameLocal.Printf( "...%d entities\n", count );
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
		gameLocal.Printf( "\"%s\"  "S_COLOR_WHITE"\"%s\"\n", kv->GetKey().c_str(), kv->GetValue().c_str() );
	}
}

/*
===================
Cmd_ReloadScript_f
===================
*/
void Cmd_ReloadScript_f( const idCmdArgs &args ) {
	// shutdown the map because entities may point to script objects
	gameLocal.MapShutdown();

	// recompile the scripts
	gameLocal.program.Startup( SCRIPT_DEFAULT );

	// error out so that the user can rerun the scripts
	gameLocal.Error( "Exiting map to reload scripts" );
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
// RAVEN BEGIN
// jscott: fixed sprintf to idStr
	text = va( "void %s() {%s;}\n", funcname.c_str(), script );
// RAVEN END
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

// RAVEN BEGIN
// jscott: exports for tracking memory
/*
==================
idGameEdit::ScriptSummary
==================
*/
size_t idGameEdit::ScriptSummary( const idCmdArgs &args ) const {

	return( gameLocal.program.ScriptSummary( args ) );
}

/*
==================
idGameEdit::ClassSummary
==================
*/
size_t idGameEdit::ClassSummary( const idCmdArgs &args ) const {

	common->Printf( "Classes         - %dK\n", idClass::GetUsedMemory() / 1024 );

	return( idClass::GetUsedMemory() / 1024 );
}

/*
==================
idGameEdit::EntitySummary
==================
*/

size_t idGameEdit::EntitySummary( const idCmdArgs &args ) const {

	common->Printf( "CL & SV ents    - %dK\n", gameLocal.GetEntityMemoryUsage () / 1024);

	return gameLocal.GetEntityMemoryUsage() / 1024;
}
// RAVEN END

/*
==================
KillEntities

Kills all the entities of the given class in a level.
==================
*/
void KillEntities( const idCmdArgs &args, const idTypeInfo &superClass ) {
	idEntity	*ent;
	idStrList	ignore;
	const char *name;
	int			i;

	if ( !gameLocal.GetLocalPlayer() || !gameLocal.CheatsOk( false ) ) {
		return;
	}

	for( i = 1; i < args.Argc(); i++ ) {
		name = args.Argv( i );
		ignore.Append( name );
	}

	for( ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() ) {
		if ( ent->IsType( superClass ) ) {
			for( i = 0; i < ignore.Num(); i++ ) {
				if ( ignore[ i ] == ent->name ) {
					break;
				}
			}

			if ( i >= ignore.Num() ) {
				ent->PostEventMS( &EV_Remove, 0 );
			}
		}
	}
}

/*
==================
Cmd_KillMonsters_f

Kills all the monsters in a level.
==================
*/
void Cmd_KillMonsters_f( const idCmdArgs &args ) {
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
	KillEntities( args, idAI::GetClassType() );
// nmckenzie: rvSpawners
	KillEntities( args, rvSpawner::GetClassType() );

	// kill any projectiles as well since they have pointers to the monster that created them
	KillEntities( args, idProjectile::GetClassType() );
// RAVEN END
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
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
	KillEntities( args, idMoveable::GetClassType() );
// RAVEN END
}

// RAVEN BEGIN
// bdube: vehicle code
/*
==================
Cmd_KillVehicles_f
==================
*/
void Cmd_KillVehicles_f( const idCmdArgs &args ) {
	if ( !gameLocal.GetLocalPlayer() || !gameLocal.CheatsOk( false ) ) {
		return;
	}
	
	rvVehicleController::KillVehicles ( );
}

void Cmd_KillMessage_f( const idCmdArgs &args ) {
	if ( !gameLocal.GetLocalPlayer() || !gameLocal.CheatsOk( false ) ) {
		return;
	}
	
	gameLocal.mpGame.SendDeathMessage( gameLocal.GetLocalPlayer(), gameLocal.GetLocalPlayer(), 2, false );
}

void Cmd_APState_f( const idCmdArgs &args ) {
	if ( !gameLocal.GetLocalPlayer() || !gameLocal.CheatsOk( false ) ) {
		return;
	}
	
	for ( int i = 0; i < gameLocal.mpGame.assaultPoints.Num(); i++ ) {
		gameLocal.Printf ( "Assault point #%d: owner: %d\n", gameLocal.mpGame.assaultPoints[i]->GetIndex(), gameLocal.mpGame.assaultPoints[i]->GetOwner() );
	}
}
// RAVEN END

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
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
	KillEntities( args, idAFEntity_Generic::GetClassType() );
	KillEntities( args, idAFEntity_WithAttachedHead::GetClassType() );
// RAVEN END
}


// RITUAL BEGIN
// squirrel: added DeadZone multiplayer mode
/*
==================
GiveStuffToPlayer

Used by the "give" and "buy" command line cmds
==================
*/
void GiveStuffToPlayer( idPlayer* player, const char* name, const char* value )
{
	int			i;
	bool		give_all;
//	idPlayer* player = gameLocal.GetLocalPlayer();

	if( !player || !name )	{
		return;
	}

	if( !value ) {
		value = "";
	}

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

	if ( ( idStr::Cmpn( name, "weapon_", 7 ) == 0 ) || ( idStr::Cmpn( name, "item_", 5 ) == 0 ) || ( idStr::Cmpn( name, "ammo_", 5 ) == 0 ) || ( idStr::Icmp( name, "ammorefill" ) == 0 ) ) {
		player->GiveItem( name );
		return;
	}

	if ( give_all || idStr::Icmp( name, "health" ) == 0 )	{
		player->health = player->inventory.maxHealth;
		if ( player->IsInVehicle() ) {
			player->GetVehicleController().Give ( "health", "9999" );
		}
		if ( !give_all ) {
			return;
		}
	}

	if ( give_all || idStr::Icmp( name, "weapons" ) == 0 ) {
		player->inventory.weapons = BIT( MAX_WEAPONS ) - 1;
		player->CacheWeapons();

		if ( !give_all ) {
			return;
		}
	}

	if ( give_all || idStr::Icmp( name, "ammo" ) == 0 ) {
// RAVEN BEGIN
// bdube: define changed
		for ( i = 0 ; i < MAX_AMMOTYPES; i++ ) {
			player->inventory.ammo[ i ] = player->inventory.MaxAmmoForAmmoClass( player, rvWeapon::GetAmmoNameForIndex( i ) );
// RAVEN END		
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
// RAVEN BEGIN
	if (idStr::Icmp(name, "quad") == 0) {
		player->GivePowerUp( POWERUP_QUADDAMAGE, SEC2MS( 30.0f ) );
		return;
	}

	if ( idStr::Icmp( name, "invis" ) == 0 ) {
		player->GivePowerUp( POWERUP_INVISIBILITY, SEC2MS( 30.0f ) );
		return;
	}

	if ( idStr::Icmp( name, "regen" ) == 0 ) {
		player->GivePowerUp( POWERUP_REGENERATION, SEC2MS( 30.0f ) );
		return;
	}

	if ( idStr::Icmp( name, "haste" ) == 0 ) {
		player->GivePowerUp( POWERUP_HASTE, SEC2MS( 30.0f ) );
		return;
	}

	if (idStr::Icmp(name, "ammoregen") == 0) {
		player->GivePowerUp( POWERUP_AMMOREGEN, -1 );
		return;
	}
	
	if (idStr::Icmp(name, "scout") == 0) {
		player->GivePowerUp( POWERUP_SCOUT, -1 );
		return;
	}

	if (idStr::Icmp(name, "doubler") == 0) {
		player->GivePowerUp( POWERUP_DOUBLER, -1 );
		return;
	}

	if (idStr::Icmp(name, "guard") == 0) {
		player->GivePowerUp( POWERUP_GUARD, -1 );
		return;
	}
// RAVEN END

	if ( !idStr::Icmp ( name, "wpmod_all" ) ) {
		player->GiveWeaponMods ( 0xFFFFFFFF );
		return;
	} else if ( !idStr::Cmpn( name, "wpmod_", 6 ) ) {
		player->GiveWeaponMod(name);
		return;
	}

	if ( !idStr::Cmpn( name, "stroggmod_", 10 ) ) {
		player->Give ( name, "" );
		return;
	}

	if ( !give_all && !player->Give( name, value ) ) {
		gameLocal.Printf( "unknown item\n" );
	}
}

/*
==================
Cmd_Give_f

Give items to a client
==================
*/
void Cmd_Give_f( const idCmdArgs &args ) {
	idPlayer	*player;

	player = gameLocal.GetLocalPlayer();
	if ( !player || !gameLocal.CheatsOk() ) {
		return;
	}

	GiveStuffToPlayer( player, args.Argv(1), args.Argv(2) );
}
// RITUAL END

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

/*
==================
Cmd_God_f

Sets client to godmode

argv(0) god
==================
*/
void Cmd_God_f( const idCmdArgs &args ) {
	char		*msg;
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
Cmd_Undying_f

Sets client to undying

argv(0) undying
==================
*/
void Cmd_Undying_f( const idCmdArgs &args ) {
	char		*msg;
	idPlayer	*player;

	player = gameLocal.GetLocalPlayer();
	if ( !player || !gameLocal.CheatsOk() ) {
		return;
	}

	if ( player->undying ) {
		player->undying = false;
		msg = "undying OFF\n";
	} else {
		player->undying = true;
		msg = "undying ON\n";
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
	char		*msg;
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
	char		*msg;
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
	player->noclip = !player->noclip;

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
				gameLocal.Printf( "kill <client nickname> or kill <client index>\n" );
				return;
			}
			player->Kill( false, false );
// RAVEN BEGIN
// rhummer: localized this string.. (killed client)
			cmdSystem->BufferCommandText( CMD_EXEC_NOW, va( "say %s %d '%s^0'\n", common->GetLocalizedString( "#str_108022" ), player->entityNumber, gameLocal.userInfo[ player->entityNumber ].GetString( "ui_name" ) ) );
// RAVEN END
		}
	} else {
		player = gameLocal.GetLocalPlayer();
		if ( !player ) {
			return;
		}
		player->Kill( false, false );
	}
}

// RAVEN BEGIN
// bdube: jump points
/*
=================
Cmd_DebugJump_f
=================
*/
void Cmd_DebugJump_f( const idCmdArgs &args ) {
	if (args.Argc() > 1) {
		// going to a specific jump point as specified by second argument
		gameDebug.JumpTo ( args.Argv( 1 ) );
	} else {
		// just go to next jump point as specified
		gameDebug.JumpNext ( );
	}
}

/*
=================
Cmd_DebugNextJumpPoint_f
=================
*/
void Cmd_DebugNextJumpPoint_f( const idCmdArgs &args ) { 	
	// just go to next jump point as specified
	gameDebug.JumpNext ( );
}

/*
=================
Cmd_DebugPrevJumpPoint_f
=================
*/
void Cmd_DebugPrevJumpPoint_f( const idCmdArgs &args ) {
	// just go to previous jump point as specified
	gameDebug.JumpPrev ( );
}

/*
=================
Cmd_AASExtractTactical_f
=================
*/
void Cmd_AASExtractTactical_f( const idCmdArgs &args ) {
	if (gameLocal.GetLocalPlayer())
	{
		gameLocal.GetLocalPlayer()->aasSensor->SearchDebug();
	}
}

/*
=================
Cmd_CallScriptFunc_f
=================
*/
void Cmd_CallScriptFunc_f( const idCmdArgs& args ) {
	if( args.Argc() <= 1 ) {
		gameLocal.Printf( "usage: call <retKey> FuncName <parm1> <Parm2>...\n" );
		return;
	}

	idDict returnDict;
	rvScriptFuncUtility util;

	if( !util.Init(args) ) {
		return;
	}

	util.CallFunc( &returnDict );

	if( util.ReturnsAVal() && util.GetReturnKey() && util.GetReturnKey()[0] ) {
		gameLocal.Printf( "%s: %s\n", util.GetReturnKey(), returnDict.GetString(util.GetReturnKey()) );
	}
}

void Cmd_SetPlayerGravity_f( const idCmdArgs& args ) {
	if( args.Argc() <= 1 ) {
		gameLocal.Printf( "usage: setPlayerGravity 'x_magnitude y_magnitude z_magnitude\n" );
		return;
	}

	idPlayer* player = gameLocal.GetLocalPlayer();
	if( !player ) {
		return;
	}

	idVec3 gravity;
	sscanf( args.Argv(1), "%f %f %f", &gravity.x, &gravity.y, &gravity.z );
	player->GetPhysics()->SetGravity( gravity );
}
// RAVEN END

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
// RAVEN BEGIN
// bdube: make sure text was specified
	if ( text.Length() == 0 ) {
		gameLocal.Printf( "usage: %s <text>\n", cmd );
		return;
	}
	
	text.Replace( "&bsl;", "\\" );
	text.Replace( "&amp;", "&" );

// asalmon: check to see if the text passes the live decency standard
#ifdef _XENON
	if(!Sys_VerifyString(text.c_str()))
	{
		gameLocal.Printf( "Your message did not pass Xbox decency standards\n");
		return;
	}
#endif


// ddynerman: team speak only in team games
	if ( team && !gameLocal.IsTeamGame() ) {
		team = false;
	}
// RAVEN END
	if ( text[ text.Length() - 1 ] == '\n' ) {
		text[ text.Length() - 1 ] = '\0';
	}
	name = "player";

	idPlayer *	player;

	// here we need to special case a listen server to use the real client name instead of "server"
	// "server" will only appear on a dedicated server
	if ( gameLocal.isClient || cvarSystem->GetCVarInteger( "net_serverDedicated" ) == 0 ) {
		player = gameLocal.GetLocalPlayer();
		if ( player ) {
			if ( player->GetUserInfo() ) {
				name = player->GetUserInfo()->GetString( "ui_name", "player" );
			}

// RAVEN BEGIN
// mekberg: activate the mphud gui so the time is right before receiving the chat message
			if ( player->mphud ) {
				player->mphud->Activate( true, gameLocal.time );
			}

			if (player->IsFakeClient())
			{
				name = cvarSystem->GetCVarString( "ui_name" );
			}
		}
// RAVEN END
	} else {
		name = "server";
	}

	if ( gameLocal.isClient ) {
		idBitMsg	outMsg;
		byte		msgBuf[ 256 ];
		outMsg.Init( msgBuf, sizeof( msgBuf ) );
		outMsg.WriteByte( team ? GAME_RELIABLE_MESSAGE_TCHAT : GAME_RELIABLE_MESSAGE_CHAT );
		outMsg.WriteString( name );
		outMsg.WriteString( text );
		outMsg.WriteString( "" );
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
		return;
	}

	const renderView_t *view = player->GetRenderView();
	if ( view ) {
		gameLocal.Printf( "(%s) %.1f\n", view->vieworg.ToString(), view->viewaxis[0].ToYaw() );
	} else {
		player->GetViewPos( origin, axis );
		gameLocal.Printf( "(%s) %.1f\n", origin.ToString(), axis[0].ToYaw() );
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

	if ( ( args.Argc() != 4 ) && ( args.Argc() != 5 ) ) {
		gameLocal.Printf( "usage: setviewpos <x> <y> <z> <yaw>\n" );
		return;
	}

	angles.Zero();
	if ( args.Argc() == 5 ) {
		angles.yaw = atof( args.Argv( 4 ) );
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
#ifndef _MPBETA
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
	dict.Set( "angle", va( "%f", yaw + 180 ) );

	org = player->GetPhysics()->GetOrigin() + idAngles( 0, yaw, 0 ).ToForward() * 80 + idVec3( 0, 0, 1 );
	dict.Set( "origin", org.ToString() );

	for( i = 2; i < args.Argc() - 1; i += 2 ) {

		key = args.Argv( i );
		value = args.Argv( i + 1 );

		dict.Set( key, value );
	}

// RAVEN BEGIN
// kfuller: want to know the name of the entity I spawned
	idEntity *newEnt = NULL;
	gameLocal.SpawnEntityDef( dict, &newEnt );

	if (newEnt)	{
		gameLocal.Printf("spawned entity '%s'\n", newEnt->name.c_str());
	}
// RAVEN END
#endif // !_MPBETA
}

// RAVEN BEGIN
// ddynerman: MP spawning command for performance testing
/*
===================
Cmd_EvaluateMPPerformance_f
===================
*/
void Cmd_EvaluateMPPerformance_f( const idCmdArgs &args ) {
	float		yaw;
	idVec3		org;
	idPlayer	*player;
	idDict		dict;

	player = gameLocal.GetLocalPlayer();
	if ( !player || !gameLocal.CheatsOk( false ) ) {
		return;
	}

	int num = 15;

	if ( args.Argc() > 1 ) {
		num = atoi( args.Argv( 1 ) );
	}

	float angleStep = 360.0f / num;

	const char* className = "char_marine";
	
	yaw = player->viewAngles.yaw;

	for( int i = 0; i < num; i++ ) {
		dict.Set( "classname", className );
		dict.Set( "angle", va( "%f", yaw + 180 ) );

		org = player->GetPhysics()->GetOrigin() + idAngles( 0, yaw + (i * angleStep), 0 ).ToForward() * 120 + idVec3( 0, 0, 1 );
		dict.Set( "origin", org.ToString() );

		idEntity *newEnt = NULL;
		gameLocal.SpawnEntityDef( dict, &newEnt );

		if (newEnt)	{
			gameLocal.Printf("spawned entity '%s'\n", newEnt->name.c_str());
		}
	}
}
// RAVEN END


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
Cmd_Flashlight_f

Toggles flashlight on specified entity
==================
*/
void Cmd_Flashlight_f( const idCmdArgs &args ) {
	if ( gameLocal.IsMultiplayer() || !gameLocal.GetLocalPlayer() || !gameLocal.CheatsOk( false ) ) {
		return;
	}
	if ( args.Argc() != 3 ) {
		gameLocal.Printf( "usage: flashight <name of entity to damage> <0 = off, 1 = on>\n" );
		return;
	}

	idEntity *ent = gameLocal.FindEntity( args.Argv( 1 ) );
	if ( !ent || !ent->IsType( idActor::GetClassType() ) ) {
		gameLocal.Printf( "entity not found or not an actor\n" );
		return;
	}
	ent->ProcessEvent( &AI_Flashlight, atoi( args.Argv( 2 ) ) );
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
==================
Cmd_AI_DebugFilter_f

Makes the targeted entity the only one ai_debugMove & ai_debugTactical cares about
==================
*/
void Cmd_AI_DebugFilter_f( const idCmdArgs &args ) {
	idPlayer* player = gameLocal.GetLocalPlayer();
	if ( !player ) {
		return;
	}
	idEntity *ent = NULL;
	if ( args.Argc() != 2 ) 
	{
		//trace ahead
		trace_t	trace;
		idVec3 start = player->GetEyePosition();
		idVec3 end = start + player->viewAngles.ToForward() * 2048.0f;
		gameLocal.TracePoint( player, trace, start, end, MASK_SHOT_RENDERMODEL, player );
		ent = gameLocal.GetTraceEntity( trace );
	}
	else
	{	
		idEntity *ent = gameLocal.FindEntity( args.Argv( 1 ) );
		if ( !ent ) {
			gameLocal.Printf( "entity not found\n" );
			return;
		}
	}

	if ( !ent || !ent->IsType( idAI::GetClassType() ) ) {
		ai_debugFilterString.SetString( "" );
	} else {
		ai_debugFilterString.SetString( ent->GetName() );
	}
}

/*
===================
Cmd_TestLight_f
===================
*/
void Cmd_TestLight_f( const idCmdArgs &args ) {
	int			i;
	idStr		filename;
	const char *key, *value, *name = NULL;
	idPlayer *	player;
	idDict		dict;

	player = gameLocal.GetLocalPlayer();
	if ( !player || !gameLocal.CheatsOk( false ) ) {
		return;
	}

	renderView_t	*rv = player->GetRenderView();

	float fov = idMath::Tan( idMath::M_DEG2RAD * rv->fov_x / 2 );

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
	const char *key, *value, *name = NULL;
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
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
		if ( !ent->IsType( idLight::GetClassType() ) ) {
// RAVEN END
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

	bool removeFromMap = ( args.Argc() > 1 );

	gameLocal.Printf( "Clearing all lights.\n" );
	for( ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = next ) {
		next = ent->spawnNode.Next();
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
		if ( !ent->IsType( idLight::GetClassType() ) ) {
// RAVEN END
			continue;
		}

		light = static_cast<idLight*>( ent );
		mapEnt = mapFile->FindEntity( light->name );

		if ( removeFromMap && mapEnt ) {
			mapFile->RemoveEntity( mapEnt );
		}

		delete light;
	}
}

// RAVEN BEGIN
// bdube: not using id effects
/*
==================
Cmd_TestFx_f
==================
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
*/
// RAVEN END

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
	char buf[128], i;

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
// RAVEN BEGIN
// ddynerman: this eats about 5k us in release
#ifdef _DEBUG
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
#else
	return;
#endif
// RAVEN END
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

// RAVEN BEGIN
// mekberg: disable non pre-cached warnings
	fileSystem->SetIsFileLoadingAllowed( true );

// jsinger: animationLib changed to a pointer
	animationLib->ReloadAnims();

// mekberg: enable non pre-cached warnings
	fileSystem->SetIsFileLoadingAllowed( false );
// RAVEN END
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
// RAVEN BEGIN
// rjohnson: more output for animators
		idFile *FH = fileSystem->OpenFileAppend( "animations.txt" );
		for( i = 0; i < num; i++ ) {
			for( int j = 0; ; j++ ) {
				const char *fileName = animator.AnimMD5Name( i, j );

				if ( !fileName[0] ) {
					break;
				}
				gameLocal.Printf( "%s\t%s\n", animator.AnimFullName( i ), animator.AnimMD5Name( i, 0 ) );
				if ( FH ) {
					FH->Printf( "%s\t%s\n", animator.AnimFullName( i ), animator.AnimMD5Name( i, 0 ) );
				}
			}
		}
		gameLocal.Printf( "%d anims\n", num );
		fileSystem->CloseFile( FH );
// RAVEN END
	} else {
// RAVEN BEGIN
// jsinger: animationLib changed to a pointer
		animationLib->ListAnims();
// RAVEN END

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

		gameLocal.Printf( "%d memory used in %d entity animators\n", size, num );
	}
}

// RAVEN BEGIN
// jscott: export for memory tracking
/*
================
idGameEdit::ListAnims
================
*/
void idGameEdit::PrintMemInfo( MemInfo *mi ) {

	int		i, count, totalSize;
	idAAS	*aas;

	totalSize = 0;
	count = 0;
	for( i = 0; i < gameLocal.GetNumAAS(); i++ ) {

		aas = gameLocal.GetAAS( i );
		if( aas ) {

			totalSize += aas->StatsSummary();
			count++;
		}
	}

	mi->aasAssetsTotal = totalSize;
	mi->aasAssetsCount = count;

	// jsinger: animationLib changed to a pointer
	animationLib->PrintMemInfo( mi );
}
// RAVEN END

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
	player->health = player->inventory.maxHealth;
	player->Damage( NULL, NULL, dir, damageDefName, 1.0f, INVALID_JOINT );
	player->health = player->inventory.maxHealth;
}

/*
==================
Cmd_TestBoneFx_f
==================
*/
// RAVEN BEGIN
// bdube: not using
/*
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
*/
// RAVEN END

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
	player->Damage( NULL, NULL, dir, "damage_triggerhurt_1000", 1.0f, INVALID_JOINT );
	if ( args.Argc() >= 2) {
		player->SpawnGibs( dir, "damage_triggerhurt_1000" );
	}

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

	player->weapon->BloodSplat( 2.0f );
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
	const char *name = NULL;

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

// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
	if ( s->IsType( idMoveable::GetClassType() ) ) {
// RAVEN END
		// save the moveable state
		mapEnt->epairs.Set( "origin", s->GetPhysics()->GetOrigin().ToString( 8 ) );
		mapEnt->epairs.Set( "rotation", s->GetPhysics()->GetAxis().ToString( 8 ) );
	}
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
	else if ( s->IsType( idAFEntity_Generic::GetClassType() ) || s->IsType( idAFEntity_WithAttachedHead::GetClassType() ) ) {
// RAVEN END
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
	int e, i;
	idMoveable *m;
	idMapEntity *mapEnt;
	idMapFile *mapFile = gameLocal.GetLevelMap();
	idStr mapName;
	const char *name = NULL;

	if ( !gameLocal.CheatsOk() ) {
		return;
	}

	for( e = 0; e < MAX_GENTITIES; e++ ) {
		m = static_cast<idMoveable *>(gameLocal.entities[ e ]);

// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
		if ( !m || !m->IsType( idMoveable::GetClassType() ) ) {
// RAVEN END
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

	for( e = 0; e < MAX_GENTITIES; e++ ) {
		m = static_cast<idMoveable *>(gameLocal.entities[ e ]);

// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
		if ( !m )
		{
			continue;
		}
// jdischler: need to check for idMoveableItem as well
		if ( !m->IsType( idMoveable::GetClassType()) && !m->IsType( idMoveableItem::GetClassType()) )
		{
// RAVEN END
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
	}

	// write out the map file
	mapFile->Write( mapName, ".map" );
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
	const char *name = NULL;

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

// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
		if ( !af->IsType( idAFEntity_WithAttachedHead::GetClassType() ) && !af->IsType( idAFEntity_Generic::GetClassType() ) ) {
// RAVEN END
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

// RAVEN BEGIN
// rjohnson: entity usage stats
/*
==================
Cmd_ListEntityStats_f
==================
*/
static void Cmd_ListEntityStats_f( const idCmdArgs &args ) {
	gameLocal.ListEntityStats( args );
}
// RAVEN END

/*
==================
Cmd_SaveLights_f
==================
*/
static void Cmd_SaveLights_f( const idCmdArgs &args ) {
	int e, i;
	idLight *light;
	idMapEntity *mapEnt;
	idMapFile *mapFile = gameLocal.GetLevelMap();
	idDict dict;
	idStr mapName;
	const char *name = NULL;

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
		light = static_cast<idLight*>(gameLocal.entities[ e ]);

// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
		if ( !light || !light->IsType( idLight::GetClassType() ) ) {
// RAVEN END
			continue;
		}

		dict.Clear();
		light->SaveState( &dict );

		// find map file entity
		mapEnt = mapFile->FindEntity( light->name );
		// create new map file entity if there isn't one for this light
		if ( !mapEnt ) {
			mapEnt = new idMapEntity();
			mapFile->AddEntity( mapEnt );
			for ( i = 0; i < 9999; i++ ) {
				name = va( "%s_%d", light->GetEntityDefName(), i );
				if ( !gameLocal.FindEntity( name ) ) {
					break;
				}
			}
			light->name = name;
			mapEnt->epairs.Set( "classname", light->GetEntityDefName() );
			mapEnt->epairs.Set( "name", light->name );
		}
		// save the light state
		mapEnt->epairs.Copy( dict );
	}

	// write out the map file
	mapFile->Write( mapName, ".map" );
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
		}
	}

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

// RAVEN BEGIN
#if 0
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
	idFile *file = fileSystem->OpenFileAppend( str );
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
#endif
// RAVEN END

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
	if ( idStr::Cmpn( id, STRTABLE_ID, STRTABLE_ID_LENGTH ) != 0 ) {
		id = STRTABLE_ID + id;
	}
	gameLocal.mpGame.AddChatLine( common->GetLocalizedString( id ), "<nothing>", "<nothing>", "<nothing>" );	
}

// RAVEN BEGIN
// ddynerman: instance testing commands
void Cmd_SetInstance_f( const idCmdArgs& args ) {
	if( !gameLocal.isServer ) {
		gameLocal.Warning( "Cmd_SetInstance_f(): setInstance can only be run on a server\n" );
		return;
	}

	if ( args.Argc() <= 2 ) {
		common->Printf( "usage: setInstance <client> <instance id>\n" );
		return;
	}

	int client = atoi( args.Argv( 1 ) );
	int instanceID = atoi( args.Argv( 2 ) );

	if( client < 0 || client >= MAX_CLIENTS || !gameLocal.entities[ client ] ) {
		gameLocal.Warning( "Cmd_SetInstance_f(): Invalid clientnum %d\n", client );
		return;
	}

	idPlayer* player = (idPlayer*)gameLocal.entities[ client ];

	gameLocal.Printf( "Cmd_SetInstance_f(): Switching %d: %s to instance %d\n", client, gameLocal.userInfo[ client ].GetString( "ui_name", "unknown" ), instanceID );

	player->JoinInstance( instanceID );
}

void Cmd_ListInstances_f( const idCmdArgs& args ) {
	if( !gameLocal.isServer ) {
		gameLocal.Warning( "Cmd_ListInstances_f(): listInstances can only be run on a server\n" );
		return;
	}

	for( int i = 0; i < gameLocal.GetNumInstances(); i++ ) {
		gameLocal.Printf("Instance %d:\n", i );
		for( int j = 0; j < MAX_CLIENTS; j++ ) {
			idPlayer* player = (idPlayer*)gameLocal.entities[ j ];
			if( player && player->GetInstance() == i ) {
				gameLocal.Printf( "\t%d: %s\n", j, player->GetUserInfo()->GetString( "ui_name" ) );
			}
		}
	}
}

void Cmd_AddIcon_f( const idCmdArgs& args ) {
	if ( args.Argc() <= 1 ) {
		common->Printf( "usage: addIcon <client>\n" );
		return;
	}
	
	int client = atoi( args.Argv( 1 ) );
	
	iconManager->AddIcon( client, "textures/mp/awards/capture" );
}
// RAVEN END

// RITUAL BEGIN
// squirrel: Mode-agnostic buymenus
void Cmd_ToggleBuyMenu_f( const idCmdArgs& args ) {
	idPlayer* player = gameLocal.GetLocalPlayer();
	if ( player && player->CanBuy() )
	{
		gameLocal.mpGame.OpenLocalBuyMenu();
	}
}

void Cmd_BuyItem_f( const idCmdArgs& args ) {
	idPlayer* player = gameLocal.GetLocalPlayer();
	if ( !player ) {
		common->Printf( "ERROR: Cmd_BuyItem_f() failed, since GetLocalPlayer() was NULL.\n", player );
		return;
	}

	player->GenerateImpulseForBuyAttempt( args.Argv(1) );
}
// RITUAL END

void Cmd_PlayerEmote_f( const idCmdArgs& args ) {
	if( gameLocal.GetLocalPlayer() == NULL ) {
		gameLocal.Warning( "Cmd_Emote_f() - local player is NULL" );
		return;
	}

	if ( args.Argc() <= 1 ) {
		gameLocal.Printf( "usage: emote <emote>\n" );
		gameLocal.Printf( "\ttry 'taunt', 'salute', 'grab_a'\n" );
		return;
	}

	if( !idStr::Icmp( args.Argv( 1 ), "taunt" ) ) {
		gameLocal.GetLocalPlayer()->SetEmote( PE_TAUNT );
	} else if( !idStr::Icmp( args.Argv( 1 ), "grab_a" ) ) {
		gameLocal.GetLocalPlayer()->SetEmote( PE_GRAB_A );
	} else if( !idStr::Icmp( args.Argv( 1 ), "grab_b" ) ) {
		gameLocal.GetLocalPlayer()->SetEmote( PE_GRAB_B );
	} else if( !idStr::Icmp( args.Argv( 1), "salute" ) ) {
		gameLocal.GetLocalPlayer()->SetEmote( PE_SALUTE );
	} else if( !idStr::Icmp( args.Argv( 1), "cheer" ) ) {
		gameLocal.GetLocalPlayer()->SetEmote( PE_CHEER );
	} else {
		gameLocal.Printf( "Invalid emote '%s'\n", args.Argv( 1 ) );
		gameLocal.Printf( "\ttry 'taunt', 'salute', 'grab'\n" );
	}
	
}

// mekberg: added
void Cmd_SetPMCVars_f ( const idCmdArgs &args ) {
	if ( gameLocal.isMultiplayer ) {
		return;
	}

	if ( gameLocal.GetLocalPlayer( ) ) {
		gameLocal.GetLocalPlayer( )->SetPMCVars( );
	}
}

void Cmd_FadeSound_f( const idCmdArgs &args )	{

	if( args.Argc() < 2)	{
		return;
	}

	float fadeDB = 0.0f;
	float fadeTime = 0.0f;

	idStr _fadeDB = args.Argv( 1);
	fadeDB = atof( _fadeDB );

	idStr _fadeTime = args.Argv( 2);
	fadeTime = atof( _fadeTime );

	soundSystem->FadeSoundClasses( SOUNDWORLD_GAME, SOUND_CLASS_MUSICAL, 0.0f - fadeDB, fadeTime );

}

void Cmd_TestClientModel_f( const idCmdArgs& args ) {
	rvClientEntity* face;
	const idDict* dict = gameLocal.FindEntityDefDict( "player_marine_client", false );

	gameLocal.SpawnClientEntityDef( *dict, &face, false );

//	face = new rvClientAFEntity( *dict );
//	face->Spawn( dict );
//	face->SetOrigin( vec3_zero );
//	face->SetAxis( mat3_identity );
//	face->SetModel( "model_player_marine" );
}


// RAVEN END

void Cmd_CheckSave_f( const idCmdArgs &args );

void Cmd_ShuffleTeams_f( const idCmdArgs& args ) {
	gameLocal.mpGame.ShuffleTeams();
}

#ifndef _FINAL
void Cmd_ClientOverflowReliable_f( const idCmdArgs& args ) {
	idBitMsg	outMsg;
	byte		msgBuf[ 114688 ];

	for( int i = 0; i < 10; i++ ) {
		outMsg.Init( msgBuf, sizeof( msgBuf ) );
		outMsg.WriteByte( -1 );
		for( int j = 0; j < 8190; j++ ) {
			outMsg.WriteByte( j );
		}
		networkSystem->ClientSendReliableMessage( outMsg );
	}
}
#endif

void Cmd_ListMaps_f( const idCmdArgs& args ) {
	gameLocal.mpGame.ListMaps();
}

/*
=================
idGameLocal::InitConsoleCommands

Let the system know about all of our commands
so it can perform tab completion
=================
*/
void idGameLocal::InitConsoleCommands( void ) {
// RAVEN BEGIN
// jscott: typeinfo gone - didn't work, it was unfinished
//	cmdSystem->AddCommand( "listTypeInfo",			ListTypeInfo_f,				CMD_FL_GAME,				"list type info" );
//	cmdSystem->AddCommand( "writeGameState",		WriteGameState_f,			CMD_FL_GAME,				"write game state" );
//	cmdSystem->AddCommand( "testSaveGame",			TestSaveGame_f,				CMD_FL_GAME|CMD_FL_CHEAT,	"test a save game for a level" );
// RAVEN END
	cmdSystem->AddCommand( "game_memory",			idClass::DisplayInfo_f,		CMD_FL_GAME,				"displays game class info" );
	cmdSystem->AddCommand( "listClasses",			idClass::ListClasses_f,		CMD_FL_GAME,				"lists game classes" );
	cmdSystem->AddCommand( "listThreads",			idThread::ListThreads_f,	CMD_FL_GAME|CMD_FL_CHEAT,	"lists script threads" );
	cmdSystem->AddCommand( "listEntities",			Cmd_EntityList_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"lists game entities" );
	cmdSystem->AddCommand( "listClientEntities",	Cmd_ClientEntityList_f,		CMD_FL_GAME|CMD_FL_CHEAT,	"lists client entities" );
	cmdSystem->AddCommand( "listActiveEntities",	Cmd_ActiveEntityList_f,		CMD_FL_GAME|CMD_FL_CHEAT,	"lists active game entities" );
	cmdSystem->AddCommand( "listMonsters",			idAI::List_f,				CMD_FL_GAME|CMD_FL_CHEAT,	"lists monsters" );
	cmdSystem->AddCommand( "listSpawnArgs",			Cmd_ListSpawnArgs_f,		CMD_FL_GAME|CMD_FL_CHEAT,	"list the spawn args of an entity", idGameLocal::ArgCompletion_EntityName );
	cmdSystem->AddCommand( "listMaps",				Cmd_ListMaps_f,				CMD_FL_GAME,				"list the available maps" );
	cmdSystem->AddCommand( "say",					Cmd_Say_f,					CMD_FL_GAME,				"text chat" );
	cmdSystem->AddCommand( "sayTeam",				Cmd_SayTeam_f,				CMD_FL_GAME,				"team text chat" );
	cmdSystem->AddCommand( "addChatLine",			Cmd_AddChatLine_f,			CMD_FL_GAME,				"internal use - core to game chat lines" );
	cmdSystem->AddCommand( "gameKick",				Cmd_Kick_f,					CMD_FL_GAME,				"same as kick, but recognizes player names" );
	cmdSystem->AddCommand( "give",					Cmd_Give_f,					CMD_FL_GAME|CMD_FL_CHEAT,	"gives one or more items" );
	cmdSystem->AddCommand( "centerview",			Cmd_CenterView_f,			CMD_FL_GAME,				"centers the view" );
	cmdSystem->AddCommand( "god",					Cmd_God_f,					CMD_FL_GAME|CMD_FL_CHEAT,	"enables god mode" );
	cmdSystem->AddCommand( "undying",				Cmd_Undying_f,				CMD_FL_GAME|CMD_FL_CHEAT,	"enables undying mode (take damage down to 1 health, but do not die)" );
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
	cmdSystem->AddCommand( "flashlight",			Cmd_Flashlight_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"toggle actor's flashlight", idGameLocal::ArgCompletion_AIName );
	
	cmdSystem->AddCommand( "shuffleTeams",			Cmd_ShuffleTeams_f,			CMD_FL_GAME,				"shuffle teams" );
// RAVEN BEGIN
// bdube: not using id effect system
//	cmdSystem->AddCommand( "testFx",				Cmd_TestFx_f,				CMD_FL_GAME|CMD_FL_CHEAT,	"tests an FX system", idCmdSystem::ArgCompletion_Decl<DECL_FX> );
//	cmdSystem->AddCommand( "testBoneFx",			Cmd_TestBoneFx_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"tests an FX system bound to a joint", idCmdSystem::ArgCompletion_Decl<DECL_FX> );
// RAVEN END
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
	cmdSystem->AddCommand( "saveParticles",			Cmd_SaveParticles_f,		CMD_FL_GAME|CMD_FL_CHEAT,	"saves all lights to the .map file" );
	cmdSystem->AddCommand( "clearLights",			Cmd_ClearLights_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"clears all lights" );
	cmdSystem->AddCommand( "gameError",				Cmd_GameError_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"causes a game error" );

// RAVEN BEGIN
// rjohnson: entity usage stats
	cmdSystem->AddCommand( "listEntityStats",		Cmd_ListEntityStats_f,		CMD_FL_GAME|CMD_FL_CHEAT,	"lists global entity stats" );
// ddynerman: mp spawning test command
	cmdSystem->AddCommand( "evaluateMPPerformance",	Cmd_EvaluateMPPerformance_f,CMD_FL_GAME|CMD_FL_CHEAT,	"spawns serveral player models", idCmdSystem::ArgCompletion_Decl<DECL_ENTITYDEF> );
	cmdSystem->AddCommand( "listMapEntities",		idGameLocal::Cmd_PrintMapEntityNumbers_f,		CMD_FL_GAME|CMD_FL_CHEAT,	"lists map entity numbers" );
	cmdSystem->AddCommand( "listSpawnIds",			idGameLocal::Cmd_PrintSpawnIds_f,		CMD_FL_GAME|CMD_FL_CHEAT,	"lists map entity numbers" );
// RAVEN END

#ifndef	ID_DEMO_BUILD
	cmdSystem->AddCommand( "disasmScript",			Cmd_DisasmScript_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"disassembles script" );
// RAVEN BEGIN
// rjohnson: removed old not taking system
/*
	cmdSystem->AddCommand( "recordViewNotes",		Cmd_RecordViewNotes_f,		CMD_FL_GAME|CMD_FL_CHEAT,	"record the current view position with notes" );
	cmdSystem->AddCommand( "showViewNotes",			Cmd_ShowViewNotes_f,		CMD_FL_GAME|CMD_FL_CHEAT,	"show any view notes for the current map, successive calls will cycle to the next note" );
	cmdSystem->AddCommand( "closeViewNotes",		Cmd_CloseViewNotes_f,		CMD_FL_GAME|CMD_FL_CHEAT,	"close the view showing any notes for this map" );
*/
// RAVEN END
	cmdSystem->AddCommand( "exportmodels",			Cmd_ExportModels_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"exports models", ArgCompletion_DefFile );

	// multiplayer client commands ( replaces old impulses stuff )
	//cmdSystem->AddCommand( "clientDropWeapon",		idMultiplayerGame::DropWeapon_f, CMD_FL_GAME,			"drop current weapon" );
	cmdSystem->AddCommand( "clientMessageMode",		idMultiplayerGame::MessageMode_f, CMD_FL_GAME,			"ingame gui message mode" );
	// FIXME: implement
	cmdSystem->AddCommand( "clientVote",			idMultiplayerGame::Vote_f,	CMD_FL_GAME,				"cast your vote: clientVote yes | no" );
	cmdSystem->AddCommand( "clientCallVote",		idMultiplayerGame::CallVote_f,	CMD_FL_GAME,			"call a vote: clientCallVote si_.. proposed_value" );
	cmdSystem->AddCommand( "clientVoiceChat",		idMultiplayerGame::VoiceChat_f,	CMD_FL_GAME,			"voice chats: clientVoiceChat <sound shader>" );
	cmdSystem->AddCommand( "clientVoiceChatTeam",	idMultiplayerGame::VoiceChatTeam_f,	CMD_FL_GAME,		"team voice chats: clientVoiceChat <sound shader>" );
// RAVEN BEGIN
	// jshepard
	cmdSystem->AddCommand( "forceTeamChange",				idMultiplayerGame::ForceTeamChange_f,			CMD_FL_GAME,		"force team change: forceTeamChange <id>" );
	cmdSystem->AddCommand( "removeClientFromBanList",		idMultiplayerGame::RemoveClientFromBanList_f,	CMD_FL_GAME,		"removes a client id from the ban list: removeClientFromBanList <client id>" );

#ifndef _XBOX
// shouchard:  more voice chat stuff (non-XBOX)
	cmdSystem->AddCommand( "clientvoicemute",		idMultiplayerGame::VoiceMute_f, CMD_FL_GAME,			"mute the specified player's incoming voicechat" );
	cmdSystem->AddCommand( "clientvoiceunmute",		idMultiplayerGame::VoiceUnmute_f, CMD_FL_GAME,			"unmute the specified player's incoming voicechat" );
#endif // _XBOX
// RAVEN END

	// multiplayer server commands
	cmdSystem->AddCommand( "verifyServerSettings",	idGameLocal::VerifyServerSettings_f,	CMD_FL_GAME,	"verifies the game type can be played on the map" );
	cmdSystem->AddCommand( "serverMapRestart",		idGameLocal::MapRestart_f,	CMD_FL_GAME,				"restart the current game" );
	cmdSystem->AddCommand( "serverForceReady",		idMultiplayerGame::ForceReady_f,CMD_FL_GAME,				"force all players ready" );
	cmdSystem->AddCommand( "serverNextMap",			idGameLocal::NextMap_f,		CMD_FL_GAME,				"change to the next map" );
#endif

	cmdSystem->AddCommand( "CheckTeamBalance", idMultiplayerGame::CheckTeamBalance_f, CMD_FL_GAME, "helper for team switching in the guis - <team to switch to> <named event for yes> <named event for no> <named event for same team>" );

	// localization help commands
	cmdSystem->AddCommand( "nextGUI",				Cmd_NextGUI_f,				CMD_FL_GAME|CMD_FL_CHEAT,	"teleport the player to the next func_static with a gui" );
	cmdSystem->AddCommand( "testid",				Cmd_TestId_f,				CMD_FL_GAME|CMD_FL_CHEAT,	"output the string for the specified id." );

// RAVEN BEGIN
// bdube: vehicle code
	cmdSystem->AddCommand( "killVehicles",			Cmd_KillVehicles_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"kills all vehicles" );
	cmdSystem->AddCommand( "killMessage",			Cmd_KillMessage_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"prints a fake death message" );
	cmdSystem->AddCommand( "apState",				Cmd_APState_f,				CMD_FL_GAME|CMD_FL_CHEAT,	"prints AP state" );
// bdube: jump points
	cmdSystem->AddCommand( "jump",					Cmd_DebugJump_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"jumps to a specific debug jump point" );
	cmdSystem->AddCommand( "nextjumppoint",			Cmd_DebugNextJumpPoint_f,	CMD_FL_GAME|CMD_FL_CHEAT,	"jumps to the next debug jump point " );
	cmdSystem->AddCommand( "prevjumppoint",			Cmd_DebugPrevJumpPoint_f,	CMD_FL_GAME|CMD_FL_CHEAT,	"jumps to the previous debug jump point" );
// cdr: Added Extract Tactical
	cmdSystem->AddCommand( "extract_tactical",		Cmd_AASExtractTactical_f,	CMD_FL_GAME,				"pulls tactical information for the current position." );
// RAVEN END

// RAVEN BEGIN
// abahr
	cmdSystem->AddCommand( "call",					Cmd_CallScriptFunc_f,		CMD_FL_GAME|CMD_FL_CHEAT,	"calls script function and prints out return val" );
	cmdSystem->AddCommand( "setPlayerGravity",		Cmd_SetPlayerGravity_f,		CMD_FL_GAME|CMD_FL_CHEAT,	"sets players local gravity" );
// cdr
	cmdSystem->AddCommand( "ai_debugFilter",		Cmd_AI_DebugFilter_f,		CMD_FL_GAME,				"ai_debugMove and ai_debugTactical only work on the specified entity (if none, does one you're looking at)", idGameLocal::ArgCompletion_AIName );
// ddynerman: multiple arena/CW stuff
	cmdSystem->AddCommand( "setInstance",			Cmd_SetInstance_f,			CMD_FL_GAME,				"sets a player's world instance" );
	cmdSystem->AddCommand( "addIcon",				Cmd_AddIcon_f,				CMD_FL_GAME|CMD_FL_CHEAT,	"adds a test icon" );
	cmdSystem->AddCommand( "listInstances",			Cmd_ListInstances_f,		CMD_FL_GAME,				"lists instances" );
// ddynerman: emote anims
	cmdSystem->AddCommand( "emote",					Cmd_PlayerEmote_f,			CMD_FL_GAME,				"plays an emote" );

	cmdSystem->AddCommand( "checkSave",				Cmd_CheckSave_f,			CMD_FL_GAME,				"tests save system" );

// jshepard: fade music in / out
	cmdSystem->AddCommand( "fadeSound",				Cmd_FadeSound_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"fades all sound by X decibles over Y seconds" );

// mekberg: added.
	cmdSystem->AddCommand( "setPMCVars",			Cmd_SetPMCVars_f,			CMD_FL_GAME,				"Resets player movement cvars" );

	cmdSystem->AddCommand( "testClientModel",		Cmd_TestClientModel_f,		CMD_FL_GAME,				"" );
#ifndef _FINAL
	cmdSystem->AddCommand( "clientOverflowReliable", Cmd_ClientOverflowReliable_f, CMD_FL_GAME,				"" );
#endif
// RAVEN END
// RITUAL START
// squirrel: Mode-agnostic buymenus
	cmdSystem->AddCommand( "buyMenu",				Cmd_ToggleBuyMenu_f,		CMD_FL_GAME,				"Toggle buy menu (if in a buy zone and the game type supports it)" );
	cmdSystem->AddCommand( "buy",					Cmd_BuyItem_f,				CMD_FL_GAME,				"Buy an item (if in a buy zone and the game type supports it)" );
// RITUAL END

}

/*
=================
idGameLocal::ShutdownConsoleCommands
=================
*/
void idGameLocal::ShutdownConsoleCommands( void ) {
	cmdSystem->RemoveFlaggedCommands( CMD_FL_GAME );
}
