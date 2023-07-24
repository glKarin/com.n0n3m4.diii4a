// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "../../framework/Licensee.h"

#include "../Entity.h"
#include "../Projectile.h"
#include "../Moveable.h"
#include "../vehicles/Walker.h"
#include "../Player.h"

#include "../botai/Bot.h"
#include "../botai/BotThreadData.h"
#include "../botai/Bot_Common.h"

#include "../rules/GameRules.h"
#include "../rules/UserGroup.h"
#include "../rules/AdminSystem.h"
#include "../WorldSpawn.h"
#include "../Light.h"
#include "../Weapon.h"
#include "../Misc.h"
#include "../anim/Anim_Testmodel.h"
#include "../../idlib/PropertiesImpl.h"
#include "../Camera.h"
#include "../rules/VoteManager.h"
#include "../roles/FireTeams.h"
#include "../guis/UserInterfaceLocal.h"
#include "../guis/UserInterfaceManagerLocal.h"
#include "../proficiency/StatsTracker.h"
#include "../script/Script_Program.h"

#include "../../sdnet/SDNetUser.h"
#include "../../sdnet/SDNetAccount.h"

#include "../docs/wiki.h"

//#include "TypeInfo.h"


/*
==================
Cmd_GetFloatArg
==================
*/
float Cmd_GetFloatArg( const idCmdArgs &args, int &argNum ) {
	const char* value = args.Argv( argNum++ );
	return static_cast< float >( atof( value ) );
}

/*
==================
Cmd_GetIntArg
==================
*/
int Cmd_GetIntArg( const idCmdArgs &args, int &argNum ) {
	const char* value = args.Argv( argNum++ );
	return atoi( value );
}

/*
===================
Cmd_ListScriptObjects_f
===================
*/
void Cmd_ListScriptObjects_f( const idCmdArgs &args ) {
	gameLocal.program->ListScriptObjects();
}

/*
===================
Cmd_WikiEventInfo_f
===================
*/
void Cmd_WikiEventInfo_f( const idCmdArgs &args ) {
	const char* eventName = args.Argv( 1 );

	if ( idStr::Icmp( eventName, "all" ) == 0 ) {
		sdWikiFormatter wiki;
		wiki.ListAllEvents();
		wiki.WriteToFile( "wiki/events.txt" );
		return;
	}

	if ( *eventName != '\0' ) {
		const idEventDef* evt = idEventDef::FindEvent( eventName );
		if ( evt == NULL ) {
			gameLocal.Warning( "Unknown Event '%s'", eventName );
			return;
		}

		if ( !evt->GetAllowFromScript() || evt->GetDescription() == NULL ) {
			gameLocal.Warning( "'%s' is internal only", eventName );
			return;
		}

		sdWikiFormatter wiki;
		wiki.BuildEventInfo( evt );
		wiki.CopyToClipBoard();
		return;
	}

	int total = 0;
	int written = 0;
	for ( int i = 0; i < idEventDef::NumEventCommands(); i++ ) {
		const idEventDef* evt = idEventDef::GetEventCommand( i );
		if ( !evt->GetAllowFromScript() ) {
			continue;
		}

		total++;

		if ( evt->GetDescription() == NULL ) {
			continue;
		}

		written++;

		idStr fileName = va( "wiki/events/%s.txt", evt->GetName() );
		sdWikiFormatter wiki;
		wiki.BuildEventInfo( evt );
		wiki.WriteToFile( fileName.c_str() );
	}

	gameLocal.Printf( "%d events written, %d events total\n", written, total );
}

/*
===================
Cmd_WikiFormatCode_r
===================
*/
void Cmd_WikiFormatCode_r( const char* path ) {
	idFileList* list = fileSystem->ListFiles( path, "script", false, true );

	for ( int i = 0; i < list->GetNumFiles(); i++ ) {
		const char* fileName = list->GetFile( i );
		sdWikiFormatter wiki;
		wiki.FormatCode( fileName );

		idStr outputFileName = va( "wiki/%s", fileName );
		outputFileName.StripFileExtension();
		outputFileName.Append( "_formatted" );
		outputFileName.SetFileExtension( ".txt" );
		wiki.WriteToFile( outputFileName.c_str() );
	}

	fileSystem->FreeFileList( list );

	list = fileSystem->ListFiles( path, "/", false, true );
	for ( int i = 0; i < list->GetNumFiles(); i++ ) {
		Cmd_WikiFormatCode_r( list->GetFile( i ) );
	}

	fileSystem->FreeFileList( list );
}

/*
===================
Cmd_WikiFormatCode_f
===================
*/
void Cmd_WikiFormatCode_f( const idCmdArgs &args ) {
	Cmd_WikiFormatCode_r( "script" );
}

/*
===================
Cmd_WikiScriptTree_f
===================
*/
void Cmd_WikiScriptTree_f( const idCmdArgs &args ) {
	sdWikiFormatter wiki;
	wiki.BuildScriptClassTree( NULL );
	wiki.WriteToFile( "wiki/script/tree.txt" );
}

/*
===================
Cmd_WikiClassInfo_f
===================
*/
void Cmd_WikiClassInfo_f( const idCmdArgs &args ) {
	sdProgram* program = gameLocal.program;
	int count = program->GetNumClasses();
	for ( int i = 0; i < count; i++ ) {
		const sdProgram::sdTypeInfo* type = program->GetClass( i );

		sdWikiFormatter wiki;
		wiki.BuildScriptClassInfo( type );
		wiki.WriteToFile( va( "wiki/script/classes/%s.txt", type->GetName() ) );
	}
}

/*
===================
Cmd_WikiScriptFileList_f
===================
*/
void Cmd_WikiScriptFileList_f( const idCmdArgs &args ) {
	sdWikiFormatter wiki;
	wiki.BuildScriptFileList();
	wiki.WriteToFile( "wiki/script/files.txt" );
}

typedef sdPair< int, idTypeInfo* > typeCount_t;
int G_SortTypes( const typeCount_t* typea, const typeCount_t* typeb ) {
	return typeb->first - typea->first;
}

/*
===================
Cmd_EntityList_f
===================
*/
void Cmd_EntityList_f( const idCmdArgs &args ) {	
	idStr match;
	if ( args.Argc() > 1 ) {
		match = args.Args();
		match.Replace( " ", "" );
	} else {
		match = "";
	}

	int count = 0;
	size_t size = 0;

	idList< typeCount_t > counts;
	counts.SetNum( idClass::GetNumTypes() );
	for ( int i = 0; i < counts.Num(); i++ ) {
		counts[ i ].first	= 0;
		counts[ i ].second	= idClass::GetType( i );
	}

	gameLocal.Printf( "%-4s  %-20s %-20s %s\n", " Num", "EntityDef", "Class", "Name" );
	gameLocal.Printf( "--------------------------------------------------------------------\n" );
	for( int e = 0; e < MAX_GENTITIES; e++ ) {
		idEntity* check = gameLocal.entities[ e ];

		if ( !check ) {
			continue;
		}

		if ( !check->name.Filter( match, true ) ) {
			continue;
		}

		counts[ check->GetType()->typeNum ].first++;

		idStr think;
		if ( check->thinkFlags & TH_THINK ) {
			think += "t";
		}
		if ( check->thinkFlags & TH_PHYSICS ) {
			think += "p";
		}
		if ( check->thinkFlags & TH_RUNSCRIPT ) {
			think += "s";
		}
		if ( check->thinkFlags & TH_ANIMATE ) {
			think += "a";
		}

		gameLocal.Printf( "%4i: %-20s %-20s %s %s\n", e, check->GetEntityDefName(), check->GetClassname(), check->name.c_str(), think.c_str() );

		count++;
		size += check->spawnArgs.Allocated();
	}

	counts.Sort( G_SortTypes );

	for ( int i = 0; counts[ i ].first > 0 && i < counts.Num(); i++ ) {
		gameLocal.Printf( "%d entities of type %s\n", counts[ i ].first, counts[ i ].second->classname );
	}

	gameLocal.Printf( "...%d entities\n...%d bytes of spawnargs\n", count, size );
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
		char	dormant = ' ';
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
Cmd_ExportScript_f
===================
*/
void Cmd_ExportScript_f( const idCmdArgs &args ) {
	if ( gameLocal.GameState() > GAMESTATE_NOMAP ) {
		gameLocal.Printf( "Exiting map to export scripts\n" );
		cmdSystem->BufferCommandText( CMD_EXEC_NOW, "disconnect" );
	}

	idProgram* newProgram = new idProgram();
	newProgram->EnableExport();

	delete gameLocal.program;
	gameLocal.program = newProgram;
	gameLocal.program->Init();
}

/*
===================
Cmd_ReloadScript_f
===================
*/
void Cmd_ReloadScript_f( const idCmdArgs &args ) {
	if ( gameLocal.GameState() > GAMESTATE_NOMAP ) {
		gameLocal.Printf( "Exiting map to reload scripts\n" );
		cmdSystem->BufferCommandText( CMD_EXEC_NOW, "disconnect" );
	}

	gameLocal.LoadScript();
}

/*
==================
KillEntities

Kills all the entities of the given class in a level.
==================
*/
void KillEntities( const idCmdArgs &args, const idTypeInfo &superClass, bool delayed ) {
	idEntity	*ent, *nextEnt;
	idStrList	ignore;
	const char *name;
	int			i;

	if ( gameLocal.isClient ) {
		return;
	}

	if ( !gameLocal.CheatsOk( false ) ) {
		return;
	}

	for( i = 1; i < args.Argc(); i++ ) {
		name = args.Argv( i );
		ignore.Append( name );
	}

	for( ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = nextEnt ) {
		nextEnt = ent->spawnNode.Next();
		if ( ent->IsType( superClass ) ) {
			for( i = 0; i < ignore.Num(); i++ ) {
				if ( ignore[ i ] == ent->name ) {
					break;
				}
			}

			if ( i >= ignore.Num() ) {
				if ( delayed ) {
					ent->PostEventMS( &EV_Remove, 0 );
				} else {
					ent->ProcessEvent( &EV_Remove );
				}
			}
		}
	}
}

/*
==================
KillEntities

Kills all the entities of the given type in a level.
==================
*/
void KillEntities( const idDeclEntityDef* entityDef, bool delayed ) {
	if ( gameLocal.isClient ) {
		return;
	}

	if ( !gameLocal.CheatsOk( false ) ) {
		return;
	}

	idEntity *ent, *nextEnt;
	for( ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = nextEnt ) {
		nextEnt = ent->spawnNode.Next();
		if ( ent->entityDefNumber == entityDef->Index() ) {
			if ( delayed ) {
				ent->PostEventMS( &EV_Remove, 0 );
			} else {
				ent->ProcessEvent( &EV_Remove );
			}
		}
	}
}

/*
==================
ActivateEntityPhysics
==================
*/
void ActivateEntityPhysics( const idCmdArgs &args, const idTypeInfo& superClass ) {
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
				ent->GetPhysics()->Activate();
			}
		}
	}
}

/*
==================
Cmd_KillMovables_f

Kills all the moveables in a level.
==================
*/
void Cmd_KillMovables_f( const idCmdArgs &args ) {
	KillEntities( args, idMoveable::Type, true );
}

/*
==================
Cmd_KillWalkers_f
==================
*/
void Cmd_KillWalkers_f( const idCmdArgs &args ) {
	KillEntities( args, sdWalker::Type, true );
}

/*
==================
Cmd_KillTransports_f
==================
*/
void Cmd_KillTransports_f( const idCmdArgs &args ) {
	KillEntities( args, sdTransport::Type, true );
}

/*
==================
Cmd_KillClass_f
==================
*/
void Cmd_KillClass_f( const idCmdArgs &args ) {
	if ( args.Argc() < 2 )  {
		gameLocal.Printf( "usage: killClass 'classname'" );
		return;
	}

	idTypeInfo* info = idClass::GetClass( args.Argv( 1 ) );
	if ( info == NULL ) {
		gameLocal.Warning( "Unknown class '%s'", args.Argv( 1 ) );
		return;
	}

	KillEntities( args, *info, true );
}

/*
==================
Cmd_KillType_f
==================
*/
void Cmd_KillType_f( const idCmdArgs &args ) {
	if( args.Argc() < 2 )  {
		gameLocal.Printf( "usage: killClass 'classname'" );
		return;
	}

	const idDeclEntityDef* entityDef = gameLocal.declEntityDefType[ args.Argv( 1 ) ];
	if ( !entityDef ) {
		gameLocal.Warning( "Unknown type '%s'", args.Argv( 1 ) );
		return;
	}

	KillEntities( entityDef, true );
}

/*
==================
Cmd_ActivateAFs_f
==================
*/
void Cmd_ActivateAFs_f( const idCmdArgs &args ) {
	ActivateEntityPhysics( args, idAFEntity_Base::Type );
}

/*
==================
Cmd_ActivateVehicles_f
==================
*/
void Cmd_ActivateVehicles_f( const idCmdArgs &args ) {
	ActivateEntityPhysics( args, sdTransport::Type );
}

/*
==================
Cmd_KillRagdolls_f

Kills all the ragdolls in a level.
==================
*/
void Cmd_KillRagdolls_f( const idCmdArgs &args ) {
	KillEntities( args, idAFEntity_Generic::Type, true );
}

/*
==================
Cmd_Give_f

Give items to a client
==================
*/
void Cmd_Give_f( const idCmdArgs &args ) {
	const char *name;
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

	if ( give_all || idStr::Icmp( name, "health" ) == 0 )	{
		player->SetHealth( player->GetMaxHealth() );
		if ( !give_all ) {
			return;
		}
	}

	if ( give_all || idStr::Icmp( name, "ammo" ) == 0 ) {
		sdInventory& inventory = player->GetInventory();
		for( int i = 0; i < gameLocal.declAmmoTypeType.Num(); i++ ) {
			inventory.SetAmmo( i, inventory.GetMaxAmmo( i ) );
		}		
		if ( !give_all ) {
			return;
		}
	}

	if ( !idStr::Icmp( name, "class" ) ) {
		const char* className = args.Argv( 2 );
		if ( gameLocal.isClient ) {
			sdReliableClientMessage msg( GAME_RELIABLE_CMESSAGE_GIVECLASS );
			msg.WriteString( className );
			msg.Send();
		} else {
			const sdDeclPlayerClass* pc = gameLocal.declPlayerClassType[ className ];
			if ( pc ) {
				player->GetInventory().GiveClass( pc, true );
			}
		}
	}

	if ( !give_all && !player->Give( args.Argv(1), args.Argv(2) ) ) {
		gameLocal.Printf( "unknown item\n" );
	}
}

/*
==================
Cmd_God_f

Sets client to godmode

argv(0) god
==================
*/
void Cmd_God_f( const idCmdArgs& args ) {
	if ( gameLocal.isClient ) {
		sdReliableClientMessage outMsg( GAME_RELIABLE_CMESSAGE_GOD );
		outMsg.Send();
		return;
	}

	gameLocal.ToggleGodMode( gameLocal.GetLocalPlayer() );
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
	if ( gameLocal.isClient ) {
		sdReliableClientMessage outMsg( GAME_RELIABLE_CMESSAGE_NOCLIP );
		outMsg.Send();
		return;
	}

	gameLocal.ToggleNoclipMode( gameLocal.GetLocalPlayer() );
}

/*
=================
Cmd_Kill_f
=================
*/
void Cmd_Kill_f( const idCmdArgs &args ) {
	if ( gameLocal.isClient ) {
		sdReliableClientMessage outMsg( GAME_RELIABLE_CMESSAGE_KILL );
		outMsg.Send();
	} else {
		idPlayer* player = gameLocal.GetLocalPlayer();
		if ( !player ) {
			return;
		}
		player->Suicide();
	}
}

/*
============
Cmd_Say
============
*/
void Cmd_DoSay( idWStr& text, gameReliableClientMessage_t mode ) {
	if( text.LengthWithoutColors() == 0 ) {
		return;
	}

	if ( text[ text.Length() - 1 ] == '\n' ) {
		text[ text.Length() - 1 ] = '\0';
	}

	if ( gameLocal.isClient ) {
		sdReliableClientMessage outMsg( mode );
		outMsg.WriteString( text.c_str() );
		outMsg.Send();
	} else {
		if ( gameLocal.rules != NULL ) {
			gameLocal.rules->ProcessChatMessage( gameLocal.GetLocalPlayer(), mode, text.c_str() );
		}
	}
}

/*
==================
Cmd_Say
==================
*/
void Cmd_Say( const idCmdArgs &args, gameReliableClientMessage_t mode ) {
	idWStr text;

	if ( args.Argc() < 2 ) {
		gameLocal.Printf( "usage: %s <text>\n", args.Argv( 0 ) );
		return;
	}

	text = va( L"%hs", args.Args() );
	Cmd_DoSay( text, mode );
}

/*
==================
Cmd_Say_f
==================
*/
static void Cmd_Say_f( const idCmdArgs &args ) {
	Cmd_Say( args, GAME_RELIABLE_CMESSAGE_CHAT );
}

/*
==================
Cmd_SayTeam_f
==================
*/
static void Cmd_SayTeam_f( const idCmdArgs &args ) {
	Cmd_Say( args, GAME_RELIABLE_CMESSAGE_TEAM_CHAT );
}

/*
==================
Cmd_SayFireTeam_f
==================
*/
static void Cmd_SayFireTeam_f( const idCmdArgs &args ) {
	Cmd_Say( args, GAME_RELIABLE_CMESSAGE_FIRETEAM_CHAT );
}

/*
==================
Cmd_AddChatLine_f
==================
*/
static void Cmd_AddChatLine_f( const idCmdArgs &args ) {
	if ( gameLocal.rules ) {
		idVec4 color;
		sdProperties::sdFromString( color, sdGameRules::g_chatDefaultColor.GetString() );
		gameLocal.rules->AddChatLine( sdGameRules::CHAT_MODE_DEFAULT, color, L"%hs", args.Argv( 1 ) );
	}	
}

/*
==================
Cmd_AddObituaryLine_f
==================
*/
static void Cmd_AddObituaryLine_f( const idCmdArgs &args ) {
	if ( gameLocal.rules ) {
		gameLocal.rules->AddChatLine( sdGameRules::CHAT_MODE_OBITUARY, colorRed , L"%hs", args.Argv( 1 ) );
	}	
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

	idStr output;
	idStr clipOutput;
	const renderView_t *view = player->GetRenderView();
	if ( view != NULL ) {
		idAngles angles = view->viewaxis[0].ToAngles();
		output = va( "(%s) %.2f %.2f %.2f", view->vieworg.ToString(), angles.yaw, angles.pitch, angles.roll );
		clipOutput = va( "%s %.2f %.2f %.2f\n", view->vieworg.ToString(), angles.yaw, angles.pitch, angles.roll );
	} else {
		player->GetViewPos( origin, axis );
		idAngles angles = view->viewaxis[0].ToAngles();
		output = va( "(%s) %.2f %.2f %.2f", origin.ToString(), angles.yaw, angles.pitch, angles.roll );
		clipOutput = va( "%s %.2f %.2f %.2f\n", origin.ToString(), angles.yaw, angles.pitch, angles.roll );
	}

	gameLocal.Printf( "Copying output to clipboard...\n(x y z) yaw pitch roll\n%s\n", output.c_str() );
	sys->SetClipboardData( va( L"%hs", clipOutput.c_str() ) );
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

	if ( ( args.Argc() != 4 ) && ( args.Argc() != 5 ) && ( args.Argc() != 6 ) && (args.Argc() != 7) ) {
		gameLocal.Printf( "usage: setviewpos <x> <y> <z> <yaw> <pitch> <roll>\n" );
		return;
	}

	angles.Zero();
	if ( (args.Argc() == 5) || (args.Argc() == 7) ) {
		angles.yaw = static_cast< float >( atof( args.Argv( 4 ) ) );
	}

	if ( args.Argc() == 7 ) {
		angles.pitch = static_cast< float >( atof( args.Argv( 5 ) ) );
		angles.roll = static_cast< float >( atof( args.Argv( 6 ) ) );
	}

	for ( i = 0 ; i < 3 ; i++ ) {
		origin[i] = static_cast< float >( atof( args.Argv( i + 1 ) ) );
	}
	origin.z -= pm_normalviewheight.GetFloat() - 0.25f;

	player->GetPhysics()->SetOrigin( origin );
	player->SetViewAngles( angles );
//	player->Teleport( origin, angles, NULL );
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

	player->GetPhysics()->SetOrigin( origin );
	player->SetViewAngles( angles );
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

	ent->ProcessEvent( &EV_Activate, player );
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

	if ( gameLocal.isClient ) {
		gameLocal.Printf( "Use networkSpawn in a network game\n" );
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

	gameLocal.SpawnEntityDef( dict, true );
}

/*
===================
Cmd_NetworkSpawn_f
===================
*/
void Cmd_NetworkSpawn_f( const idCmdArgs &args ) {
	if ( gameLocal.isClient ) {
		sdReliableClientMessage outMsg( GAME_RELIABLE_CMESSAGE_NETWORKSPAWN );
		outMsg.WriteString( args.Argv( 1 ) );
		outMsg.Send();
		return;
	}

	gameLocal.NetworkSpawn( args.Argv( 1 ), gameLocal.GetLocalPlayer() );
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

	ent->Damage( gameLocal.world, gameLocal.world, idVec3( 0, 0, 1 ), DAMAGE_FOR_NAME( "damage_moverCrush" ), static_cast< float >( atof( args.Argv( 2 ) ) ), NULL );
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

	ent->ProcessEvent( &EV_Remove );
}

idCVar testLightColor( "testLightColor", "1.0 1.0 1.0", CVAR_ARCHIVE, "the light color to be used for a 'testlight'" );
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


	const char *	s = testLightColor.GetString();
	idVec3			lightColor( 1.f, 1.f, 1.f );

	if ( *s == '0' && (*(s+1) == 'x' || *(s+1) == 'X') ) {
		s += 2;
		if ( idStr::IsHexColor( s ) ) {
			lightColor[0] = ((float)(idStr::HexForChar(*(s)) * 16 + idStr::HexForChar(*(s+1)))) / 255.f;
			lightColor[1] = ((float)(idStr::HexForChar(*(s+2)) * 16 + idStr::HexForChar(*(s+3)))) / 255.f;
			lightColor[2] = ((float)(idStr::HexForChar(*(s+4)) * 16 + idStr::HexForChar(*(s+5)))) / 255.f;
		}
	} else {
		sscanf( s, "%f %f %f", &lightColor[ 0 ], &lightColor[ 1 ], &lightColor[ 2 ] );
	}


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
	dict.SetVector( "_color", lightColor );
	dict.SetBool( "dynamic", true );

	if ( args.Argc() >= 2 ) {
		value = args.Argv( 1 );
		filename = args.Argv(1);
		filename.StripFileExtension();
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

	gameLocal.SpawnEntityDef( dict, true );

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

	dict.SetVector( "origin", player->GetRenderView()->vieworg );
	dict.SetBool( "dynamic", true );

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

	gameLocal.SpawnEntityDef( dict, true );

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

	bool removeFromMap = ( args.Argc() > 1 );

	gameLocal.Printf( "Clearing all lights.\n" );
	for( ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = next ) {
		next = ent->spawnNode.Next();
		if ( !ent->IsType( idLight::Type ) ) {
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

/*
==================
Cmd_TestFx_f
==================
*/
/*
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

#ifdef _XENON
#define MAX_DEBUGLINES	1
#else
#define MAX_DEBUGLINES	128
#endif

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
	debugLines[i].color = Cmd_GetIntArg( args, argNum );
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
	int i;
	idVec3 forward, right, up, p1, p2;
	idVec4 color;
	float l;

	for ( i = 0; i < MAX_DEBUGLINES; i++ ) {
		if ( debugLines[i].used ) {
			if ( !debugLines[i].blink || (gameLocal.time & (1<<9)) ) {
				color = idVec4( static_cast< float >( debugLines[i].color & 1 ), static_cast< float >( (debugLines[i].color>>1)&1 ), static_cast< float >( (debugLines[i].color>>2)&1 ), 1 );
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

#if defined( ID_ALLOW_TOOLS )
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
#endif /* ID_ALLOW_TOOLS */

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
Cmd_ReloadAnims_f
==================
*/
static void Cmd_ReloadGuiGlobals_f( const idCmdArgs &args ) {
	declManager->Reload( false );
	gameLocal.globalProperties.Init();
}

/*
==================
Cmd_ReloadUserGroups_f
==================
*/
static void Cmd_ReloadUserGroups_f( const idCmdArgs &args ) {
	if ( gameLocal.isClient ) {
		return;
	}

	sdUserGroupManager::GetInstance().Init();
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

		gameLocal.Printf( "%d memory used in %d entity animators\n", size, num );
	}
}

/*
==================
Cmd_TestDamage_f
==================
*/
static void Cmd_TestDamage_f( const idCmdArgs &args ) {
	idPlayer *player;

	player = gameLocal.GetLocalPlayer();
	if ( !player || !gameLocal.CheatsOk() ) {
		return;
	}

	if ( args.Argc() < 2 || args.Argc() > 3 ) {
		gameLocal.Printf( "usage: testDamage <damageDefName> [angle]\n" );
		return;
	}

	const sdDeclDamage* damageDecl = gameLocal.declDamageType[ args.Argv( 1 ) ];
	if( !damageDecl ) {		
		gameLocal.Warning( "Cmd_TestDamage_f Invalid Damage Def '%s'", args.Argv( 1 ) );
	}

	idVec3	dir;
	if ( args.Argc() == 3 ) {
		float angle = static_cast< float >( atof( args.Argv( 2 ) ) );

		idMath::SinCos( DEG2RAD( angle ), dir[1], dir[0] );
		dir[2] = 0;
	} else {
		dir.Zero();
	}

	// give the player full health before and after
	// running the damage
	player->SetHealth( player->GetMaxHealth() );
	player->Damage( NULL, NULL, dir, damageDecl, 1.0f, NULL );
	player->SetHealth( player->GetMaxHealth() );
}

/*
==================
Cmd_TestProficiency_f
==================
*/
static void Cmd_TestProficiency_f( const idCmdArgs& args ) {
	const char* profName = args.Argv( 1 );

	const sdDeclProficiencyItem* profItem = gameLocal.declProficiencyItemType[ profName ];
	if ( !profItem ) {
		return;
	}

	idPlayer* localPlayer = gameLocal.GetLocalPlayer();
	if ( localPlayer ) {
		sdProficiencyManager::GetInstance().GiveProficiency( profItem, localPlayer, 1.f, NULL, "Cheat" );
	}
}

/*
==================
Cmd_TestPrecache_f
==================
*/
static void Cmd_TestPrecache_f( const idCmdArgs& args ) {
	if ( args.Argc() != 2 ) {
		gameLocal.Printf( "usage: %s <entityDefName>", args.Argv( 0 ) );
		return;
	}

	const idDeclEntityDef* entityDef = gameLocal.declEntityDefType[ args.Argv( 1 ) ];
	if ( !entityDef ) {
		gameLocal.Warning( "Cmd_TestPrecache_f Invalid Entity Def '%s'", args.Argv( 1 ) );
		return;
	}

	fs_debug.SetBool( true );

	idEntity* other;
	if ( gameLocal.SpawnEntityDef( entityDef->dict, true, &other ) ) {
		other->PostEventMS( &EV_Remove, 0 );
	}

	fs_debug.SetBool( false );
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

	player->Damage( NULL, NULL, dir, DAMAGE_FOR_NAME( "damage_triggerhurt_1000" ), 1.0f, NULL );

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

	s = NULL; // player->dragEntity.GetSelected();
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
	} else if ( s->IsType( idAFEntity_Generic::Type ) ) {
		// save the articulated figure state
		dict.Clear();
		static_cast<idAFEntity_Base *>(s)->SaveState( dict );
		mapEnt->epairs.Copy( dict );
	}

	// write out the map file
	mapFile->Write( mapName, "." ENTITY_FILE_EXT );
}

/*
==================
Cmd_DeleteSelected_f
==================
*/
static void Cmd_DeleteSelected_f( const idCmdArgs &args ) {
/*	idPlayer *player;

	player = gameLocal.GetLocalPlayer();
	if ( !player || !gameLocal.CheatsOk() ) {
		return;
	}

	if ( player ) {
		player->dragEntity.DeleteSelected();
	}*/
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
	const char *name;

	if ( !gameLocal.CheatsOk() ) {
		return;
	}

	for( e = 0; e < MAX_GENTITIES; e++ ) {
		m = static_cast<idMoveable *>(gameLocal.entities[ e ]);

		if ( !m || !m->IsType( idMoveable::Type ) ) {
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

		if ( !m || !m->IsType( idMoveable::Type ) ) {
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
	mapFile->Write( mapName, "." ENTITY_FILE_EXT );
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

		if ( !af->IsType( idAFEntity_Generic::Type ) ) {
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
	mapFile->Write( mapName, "." ENTITY_FILE_EXT );
}

/*
==================
Cmd_BindRagdoll_f
==================
*/
static void Cmd_BindRagdoll_f( const idCmdArgs &args ) {
/*	idPlayer *player;

	player = gameLocal.GetLocalPlayer();
	if ( !player || !gameLocal.CheatsOk() ) {
		return;
	}

	if ( player ) {
		player->dragEntity.BindSelected();
	}*/
}

/*
==================
Cmd_UnbindRagdoll_f
==================
*/
static void Cmd_UnbindRagdoll_f( const idCmdArgs &args ) {
/*	idPlayer *player;

	player = gameLocal.GetLocalPlayer();
	if ( !player || !gameLocal.CheatsOk() ) {
		return;
	}

	if ( player ) {
		player->dragEntity.UnbindSelected();
	}*/
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
Cmd_DisasmScript_f
==================
*/
static void Cmd_DisasmScript_f( const idCmdArgs &args ) {
	gameLocal.program->Disassemble();
}

/*
==================
Cmd_CheckRenderEntityHandles_f
==================
*/
static void Cmd_CheckRenderEntityHandles_f( const idCmdArgs &args ) {
	idEntity::CheckForDuplicateRenderEntityHandles();
}

/*
==================
Cmd_DumpCollsionModelStats_f
==================
*/
static void Cmd_DumpCollsionModelStats_f( const idCmdArgs &args ) {
	collisionModelManager->DumpCollisionModelStats();
}

/*
==================
Cmd_TouchCollision_f
==================
*/
static void Cmd_TouchCollision_f( const idCmdArgs &args ) {
	idCollisionModel* model = collisionModelManager->LoadModel( gameLocal.GetMapName(), args.Argv( 1 ) );
	collisionModelManager->FreeModel( model );
}

/*
==================
Cmd_ListVotes_f
==================
*/
static void Cmd_ListVotes_f( const idCmdArgs &args ) {
	sdVoteManager::GetInstance().ListVotes();
}

/*
==================
Cmd_DumpMonolithicInfo_f
==================
*/
struct monolithicClassInfo_t {
	idStr classDef;
	idStr fileName;
};	
static void Cmd_DumpMonolithicInfo_f( const idCmdArgs &args ) {
	idList< monolithicClassInfo_t > classDefs;

	idFileList* fileList = fileSystem->ListFilesTree( "game", ".h" );
	for ( int i = 0; i < fileList->GetNumFiles(); i++ ) {
		idLexer lexer;
		bool loaded = lexer.LoadFile( fileList->GetFile( i ) );
		assert( loaded );

		while ( !lexer.EndOfFile() ) {
			idStr str;
			lexer.ParseCompleteLine( str );

			if ( str.Find( "CLASS_PROTOTYPE" ) == idStr::INVALID_POSITION && str.Find( "ABSTRACT_PROTOTYPE" ) == idStr::INVALID_POSITION ) {
				continue;
			}

			monolithicClassInfo_t& info = classDefs.Alloc();
			info.classDef = str;
			info.fileName = fileList->GetFile( i );
		}

		lexer.FreeSource();
	}
	fileSystem->FreeFileList( fileList );

	idFile* monolithicDependenciesFile = fileSystem->OpenFileWrite( "monolithic_dependencies.h" );
	idFile* monolithicTypesFile = fileSystem->OpenFileWrite( "monolithic_types.h" );

	idStrList dependencies;

	for ( int i = 0; i < idClass::GetNumTypes(); i++ ) {
		idTypeInfo* type = idClass::GetType( i );

		const char* fileName = NULL;

		for ( int j = 0; j < classDefs.Num(); j++ ) {
			if ( classDefs[ j ].classDef.Find( va( " %s ", type->classname ) ) == idStr::INVALID_POSITION ) {
				continue;
			}

			fileName = classDefs[ j ].fileName.c_str();
			break;
		}

		if ( fileName == NULL ) {
			gameLocal.Warning( "Couldn't find header for class '%s'", type->classname );
		} else {
			idStr strip = fileName;
			dependencies.AddUnique( strip.Right( strip.Length() - 5 ) );
			monolithicTypesFile->WriteFloatString( "volatile static idTypeInfo& %sType = %s::Type;\n", type->classname, type->classname );
		}
	}

	for ( int i = 0; i < dependencies.Num(); i++ ) {
		monolithicDependenciesFile->WriteFloatString( "#include \"%s\"\n", dependencies[ i ].c_str() );
	}

	fileSystem->CloseFile( monolithicDependenciesFile );
	fileSystem->CloseFile( monolithicTypesFile );
}

#ifdef CLIP_DEBUG
/*
==================
Cmd_PrintTraceTimings_f
==================
*/
static void Cmd_PrintTraceTimings_f( const idCmdArgs &args ) {
	gameLocal.clip.PrintTraceTimings();
}

/*
==================
Cmd_ClearTraceTimings_f
==================
*/
static void Cmd_ClearTraceTimings_f( const idCmdArgs &args ) {
	gameLocal.clip.ClearTraceTimings();
}
#endif // CLIP_DEBUG

#ifdef CLIP_DEBUG_EXTREME
/*
==================
Cmd_ToggleTraceLogging_f
==================
*/
static void Cmd_ToggleTraceLogging_f( const idCmdArgs &args ) {
	if ( gameLocal.clip.IsTraceLogging() ) {
		gameLocal.clip.StopTraceLogging();
	} else {
		gameLocal.clip.StartTraceLogging();
	}
}
#endif // CLIP_DEBUG


/*
==================
Cmd_makeEnvMaps_f

	Ok this looks a bit hacky but everything we need for this is already available as cmd's
	and you have to wait two frames anyway it seems for the setviewpos to be communicated 
	to the engine so this is the cleanest way.
==================
*/
static void Cmd_makeEnvMaps_f( const idCmdArgs &args ) {
	const idList< sdEnvDefinition > &envMaps = gameLocal.GetEnvDefinitions();

	if ( cvarSystem->Find("r_mode")->GetInteger() != 3 ) {
		cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "r_mode 3\n" );
		cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "vid_restart\n" );
	}

	for ( int i=0; i<envMaps.Num(); i++ ) {
		cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "wait\n" );
		cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "wait\n" );
		cmdSystem->BufferCommandText( CMD_EXEC_APPEND, va("setviewpos %f %f %f\n", envMaps[i].origin.x, envMaps[i].origin.y, envMaps[i].origin.z ) );
		cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "wait\n" );
		cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "wait\n" );
		cmdSystem->BufferCommandText( CMD_EXEC_APPEND, va("envshot %s %i 16\n", envMaps[i].name.c_str(), envMaps[i].size ) );
	}
}

/*
==================
Cmd_Camera_f
==================
*/
static void Cmd_Camera_f( const idCmdArgs &args ) {
	if ( args.Argc() == 2 ) {
		idEntity* ent = gameLocal.FindEntity( args.Argv( 1 ) );
		if ( !ent ) {
			gameLocal.Printf( "entity not found\n" );
			return;
		} else {
			if ( ent->IsType( sdCamera_Placement::Type ) ) {
				gameLocal.SetCamera( ent->Cast< idCamera >() );
				return;
			} else {
				gameLocal.Printf( "entity is not a camera entity\n" );
				return;
			}
		}
	} else {
		gameLocal.SetCamera( NULL );
	}
}

/*
==================
Cmd_CameraNext_f
==================
*/
static void Cmd_CameraNext_f( const idCmdArgs &args ) {
	idEntity* ent = gameLocal.GetCamera();

	if ( ent && !ent->IsType( sdCamera_Placement::Type ) ) {
		return;
	}

	ent = gameLocal.FindClassType( gameLocal.GetCamera(), sdCamera_Placement::Type );

	if ( ent ) {
		gameLocal.SetCamera( ent->Cast< idCamera >() );
		return;
	} else {
		ent = gameLocal.FindClassType( NULL, sdCamera_Placement::Type );
		if ( ent ) {
			gameLocal.SetCamera( ent->Cast< idCamera >() );
			return;
		}
	}

	gameLocal.Printf( "no camera entities found\n" );
}

/*
==================
Cmd_CameraPrev_f
==================
*/
static void Cmd_CameraPrev_f( const idCmdArgs &args ) {
	idEntity* ent = gameLocal.GetCamera();

	if ( ent && !ent->IsType( sdCamera_Placement::Type ) ) {
		return;
	}

	ent = gameLocal.FindClassTypeReverse( gameLocal.GetCamera(), sdCamera_Placement::Type );

	if ( ent ) {
		gameLocal.SetCamera( ent->Cast< idCamera >() );
		return;
	} else {
		ent = gameLocal.FindClassTypeReverse( NULL, sdCamera_Placement::Type );
		if ( ent ) {
			gameLocal.SetCamera( ent->Cast< idCamera >() );
			return;
		}
	}

	gameLocal.Printf( "no camera entities found\n" );
}

#ifdef DEBUG_SCRIPTS
/*
==================
Cmd_DumpScriptStats_f
==================
*/
void Cmd_DumpScriptStats_f( const idCmdArgs &args ) {
	( ( idProgram* )gameLocal.program )->DumpStats();
}
#endif // DEBUG_SCRIPTS

#include "../misc/PlayerBody.h"

/*
==================
Cmd_TakeViewNote_f
==================
*/
void Cmd_TakeViewNote_f( const idCmdArgs& args ) {
	sdTakeViewNoteMenu* menu = gameLocal.localPlayerProperties.GetViewNoteMenu();
	if ( !menu ) {
		return;
	}
	menu->Enable( true );
}

/*
==================
Cmd_UseWeapon_f
==================
*/
void Cmd_UseWeapon_f( const idCmdArgs& args ) {
	if( args.Argc() < 2 ) {
		gameLocal.Printf( "useWeapon: missing weapon name" );
		return;
	}

	const char* weaponName = args.Argv( 1 );

	if ( gameLocal.isClient ) {
		sdReliableClientMessage msg( GAME_RELIABLE_CMESSAGE_SELECTWEAPON );
		msg.WriteString( weaponName );
		msg.Send();
		return;
	}

	idPlayer* localPlayer = gameLocal.GetLocalPlayer();
	if ( !localPlayer ) {
		return;
	}

	localPlayer->SelectWeaponByName( weaponName );
}

/*
==================
Cmd_Vote_f
==================
*/
void Cmd_Vote_f( const idCmdArgs &args ) {
	const char* message = args.Argv( 1 );

	bool voteResult;
	if ( *message == 'y' ) {
		voteResult = true;
	} else if ( *message == 'n' ) {
		voteResult = false;
	} else {
		gameLocal.Warning( "Unknown vote '%s'", message );
		return;
	}

	idPlayer* localPlayer = gameLocal.GetLocalPlayer();
	if ( localPlayer == NULL ) {
		return;
	}

	sdPlayerVote* vote = sdVoteManager::GetInstance().GetActiveVote( localPlayer );
	if ( !vote ) {
		gameLocal.Printf( "No vote active\n" );
		return;
	}

	if ( gameLocal.isClient ) {
		vote->ClientVote( voteResult );
	} else {
		vote->Vote( localPlayer, voteResult );
	}
}

/*
==================
Cmd_Fireteam_f
==================
*/
void Cmd_Fireteam_f( const idCmdArgs &args ) {
	if ( gameLocal.isClient ) {
		sdReliableClientMessage msg( GAME_RELIABLE_CMESSAGE_FIRETEAM );
		msg.WriteLong( args.Argc() - 1 );
		for ( int i = 1; i < args.Argc(); i++ ) {
			msg.WriteString( args.Argv( i ) );
		}
		msg.Send();
		return;
	}

	idPlayer* player = gameLocal.GetLocalPlayer();
	if ( player ) {
		sdFireTeamManager::GetInstance().PerformCommand( args, player );
	}
}

/*
==================
Cmd_Admin_f
==================
*/
void Cmd_Admin_f( const idCmdArgs &args ) {
	if ( gameLocal.isClient ) {
		sdReliableClientMessage msg( GAME_RELIABLE_CMESSAGE_ADMIN );
		msg.WriteLong( args.Argc() - 1 );
		for ( int i = 1; i < args.Argc(); i++ ) {
			msg.WriteString( args.Argv( i ) );
		}
		msg.Send();
		return;
	}

	sdAdminSystem::GetInstance().PerformCommand( args, NULL );
}

static void ArgCompletion_DefFile( const idCmdArgs &args, void(*callback)( const char *s ) ) {
	cmdSystem->ArgCompletion_FolderExtension( args, callback, "def/", true, ".def", NULL );
}


/*
============
Cmd_ListGUIs_f
============
*/
static void Cmd_ListGUIs_f( const idCmdArgs& args ) {
	uiManager->ListGUIs( args );
}

/*
============
Cmd_CollisionTest_f
============
*/
static void Cmd_CollisionTest_f( const idCmdArgs& args ) {
	bool value = gameLocal.g_cacheDictionaryMedia.GetBool();

	gameLocal.g_cacheDictionaryMedia.SetBool( false );

	for ( int i = 0; i < gameLocal.declEntityDefType.Num(); i++ ) {
		const idDeclEntityDef* def = gameLocal.declEntityDefType[ i ];

		idBounds bounds;

		bool expectNone = false;
		bool hasMins = false;
		bool hasMaxs = false;

		if ( def->dict.GetBool( "noClipModel" ) ) {
			expectNone = true;
		}

		if ( def->dict.GetVector( "mins", NULL, bounds[ 0 ] ) ) {
			hasMins = true;
			if ( expectNone ) {
				gameLocal.Warning( "Entity '%s': Unexpected Mins", def->GetName() );
			}
		}
		
		if ( def->dict.GetVector( "maxs", NULL, bounds[ 1 ] ) ) {
			hasMaxs = true;
			if ( expectNone ) {
				gameLocal.Warning( "Entity '%s': Unexpected Maxs", def->GetName() );
			}
		}

		if ( hasMins != hasMaxs ) {
			gameLocal.Warning( "Entity '%s': Mismatched Mins/Maxs, only one found", def->GetName() );
		} else {
			if ( hasMins == true ) {
				expectNone = true;
			}
		}

		idVec3 size;
		if ( def->dict.GetVector( "size", NULL, size ) ) {
			if ( expectNone ) {
				gameLocal.Warning( "Entity '%s': Unexpected Size", def->GetName() );
			}

			expectNone = true;
		}

		const char* clipModelName = def->dict.GetString( "cm_model" );
		if ( *clipModelName ) {
			gameLocal.clip.PrecacheModel( clipModelName );

			if ( expectNone ) {
				gameLocal.Warning( "Entity '%s': Unexpected Clip Model Found '%s'", def->GetName(), clipModelName );
			}
		}
	}

	if ( value ) {
		gameLocal.g_cacheDictionaryMedia.SetBool( true );
	}
}

/*
============
Cmd_Stats_f
============
*/
static void Cmd_Stats_f( const idCmdArgs& args ) {
	sdStatsTracker::HandleCommand( args );
}

/*
===============
Cmd_GameCrash_f
===============
*/
static void Cmd_GameCrash_f( const idCmdArgs &args ) {
	if ( !gameLocal.IsDeveloper() ) {
		gameLocal.Printf( "gameCrash may only be used in developer mode\n" );
		return;
	}

	*((int*)0) = 0;
}

/*
===============
Cmd_PauseGame_f
===============
*/
static void Cmd_PauseGame_f( const idCmdArgs &args ) {
	if ( gameLocal.isClient ) {
		return;
	}

	gameLocal.SetPaused( true );
}

/*
===============
Cmd_UnPauseGame_f
===============
*/
static void Cmd_UnPauseGame_f( const idCmdArgs &args ) {
	if ( gameLocal.isClient ) {
		return;
	}

	gameLocal.SetPaused( false );
}

#if !defined( SD_DEMO_BUILD )
/*
===============
Cmd_PrintUserGUID_f
===============
*/
static void Cmd_PrintUserGUID_f( const idCmdArgs &args ) {
	sdNetUser* user = networkService->GetActiveUser();
	if ( user == NULL ) {
		gameLocal.Printf( "No Active User Account\n" );
		return;
	}

	if ( user->GetState() != sdNetUser::US_ONLINE ) {
		gameLocal.Printf( "User Not Logged In or is an Offline Account\n" );
		return;
	}

	sdNetClientId clientId;
	sdNetAccount& account = user->GetAccount();
	account.GetNetClientId( clientId );

	const char* guid = va( "%u.%u", clientId.id[ 0 ], clientId.id[ 1 ] );
	gameLocal.Printf( "User Id: '%s'\n", guid );
	sys->SetClipboardData( va( L"%hs", guid ) );
}
#endif /* SD_DEMO_BUILD */

/*
===============
SetSpectateClient
===============
*/
static void SetSpectateClient( const idCmdArgs &args, int argOffset ) {
	if ( args.Argc() != 2 + argOffset ) {
		gameLocal.Printf( "Invalid number of arguments, specify the client name or index\n" );
		return;
	}

	int spectateeNum = -1;

	const char* name = args.Argv( 1 + argOffset );
	if ( idStr::IsNumeric( name ) ) {
		spectateeNum = atoi( name );
		if ( spectateeNum < 0 || spectateeNum >= MAX_CLIENTS ) {
			spectateeNum = -1;
		} else {
			if ( gameLocal.GetClient( spectateeNum ) == NULL ) {
				spectateeNum = -1;
			}
		}
	} else {
		idPlayer* other = gameLocal.GetClientByName( name );
		if ( other != NULL ) {
			spectateeNum = other->entityNumber;
		}
	}

	if ( spectateeNum == -1 ) {
		gameLocal.Printf( "Could not find client %s\n", name );
		return;
	}

	// don't spectate spectators unless you're on a repeater
	if ( !gameLocal.serverIsRepeater ) {
		idPlayer* other = gameLocal.GetClient( spectateeNum );
		if ( other->IsSpectator() ) {
			gameLocal.Printf( "Client %s is a spectator.\n", name );
			return;
		}
	}

	gameLocal.ChangeLocalSpectateClient( spectateeNum );
}

/*
===============
Cmd_SetSpectateClient_f
===============
*/
static void Cmd_SetSpectateClient_f( const idCmdArgs &args ) {
	SetSpectateClient( args, 0 );
}

/*
============
Cmd_Spectate_Completion
============
*/
static void Cmd_Spectate_Completion( const idCmdArgs& args, argCompletionCallback_t callback ) {
	if ( args.Argc() > 1 ) {
		return;
	}

	callback( va( "%s next", args.Argv( 0 ) ) );
	callback( va( "%s prev", args.Argv( 0 ) ) );
	callback( va( "%s objective", args.Argv( 0 ) ) );
	callback( va( "%s client", args.Argv( 0 ) ) );
	callback( va( "%s position", args.Argv( 0 ) ) );
}

/*
===============
Cmd_Spectate_f
===============
*/
static void Cmd_Spectate_f( const idCmdArgs &args ) {
	if ( args.Argc() < 2 ) {
		gameLocal.Printf( "usage: spectate <command> [parameter]\n" );
		gameLocal.Printf( "commands: next      - cycle to the next player\n" );
		gameLocal.Printf( "commands: prev      - cycle to the previous player\n" );
		gameLocal.Printf( "commands: objective - change to the player currently completing the objective/the 'main' planted charge\n" );
		gameLocal.Printf( "commands: client	   - spectate a specific client - parameter can be client name or index\n" );
		gameLocal.Printf( "commands: position  - spectate a specific position & angle - x y z yaw pitch roll\n" );
		return;
	}

	idPlayer* player = gameLocal.GetLocalPlayer();
	if ( !gameLocal.serverIsRepeater ) {
		if ( player == NULL ) {
			gameLocal.Printf( "No local player\n" );
			return;
		}
		if ( !player->IsSpectator() ) {
			gameLocal.Printf( "Not a spectator\n" );
			return;
		}
	}

	const char* commandString = args.Argv( 1 );
	idPlayer::spectateCommand_t command = idPlayer::SPECTATE_INVALID;
	idVec3 origin = vec3_origin;
	idAngles angles = ang_zero;

	if ( !idStr::Icmp( "next", commandString ) ) {
		command = idPlayer::SPECTATE_NEXT;
	} else if ( !idStr::Icmp( "prev", commandString ) ) {
		command = idPlayer::SPECTATE_PREV;
	} else if ( !idStr::Icmp( "objective", commandString ) ) {
		command = idPlayer::SPECTATE_OBJECTIVE;
	} else if ( !idStr::Icmp( "client", commandString ) ) {
		// client does magic stuff
		SetSpectateClient( args, 1 );
		return;
	} else if ( !idStr::Icmp( "position", commandString ) ) {
		command = idPlayer::SPECTATE_POSITION;

		if ( args.Argc() < 5 || args.Argc() > 8 ) {
			gameLocal.Printf( "usage: spectate position <x> <y> <z> <yaw> <pitch> <roll>\n" );
			return;
		}

		for ( int i = 0 ; i < 3 ; i++ ) {
			origin[ i ] = static_cast< float >( atof( args.Argv( i + 2 ) ) );
		}
		origin.z -= pm_normalviewheight.GetFloat() - 0.25f;

		angles.Zero();
		if ( args.Argc() > 5 ) {
			angles.yaw = static_cast< float >( atof( args.Argv( 5 ) ) );
		}
		if ( args.Argc() > 6 ) {
			angles.pitch = static_cast< float >( atof( args.Argv( 6 ) ) );
		}
		if ( args.Argc() > 7 ) {
			angles.roll = static_cast< float >( atof( args.Argv( 7 ) ) );
		}
	}


	if ( command == idPlayer::SPECTATE_INVALID ) {
		gameLocal.Printf( "unknown command '%s'\n", commandString );
		return;
	}

	if ( gameLocal.serverIsRepeater ) {
		// client on a repeater has to do everything locally
		if ( command == idPlayer::SPECTATE_OBJECTIVE ) {
			gameLocal.Printf( "cannot do spectate objective while connected to a repeater.\n" );
		} else if ( command == idPlayer::SPECTATE_POSITION ) {
			gameLocal.ChangeLocalSpectateClient( -1 );
			gameLocal.playerView.SetRepeaterViewPosition( origin, angles );
		} else if ( command == idPlayer::SPECTATE_NEXT || command == idPlayer::SPECTATE_PREV ) {
			// go through and find a new client to spectate
			int delta = 1;
			if ( command == idPlayer::SPECTATE_PREV ) {
				delta = -1;
			}
			int upto = gameLocal.repeaterClientFollowIndex;
			for ( int i = 0; i < MAX_CLIENTS; i++ ) {
				upto = ( upto + delta + MAX_CLIENTS ) % MAX_CLIENTS;
				if ( gameLocal.GetClient( upto ) != NULL ) {
					gameLocal.ChangeLocalSpectateClient( upto );
					break;
				}
			}
		}

	} else if ( !gameLocal.isClient ) {

		// server acts on the command
		player->SpectateCommand( command, origin, angles );
	} else {

		// clients ask server to do it
		sdReliableClientMessage outMsg( GAME_RELIABLE_CMESSAGE_SPECTATECOMMAND );
		outMsg.WriteBits( command, idMath::BitsForInteger( idPlayer::SPECTATE_MAX ) );
		if ( command == idPlayer::SPECTATE_POSITION ) {
			outMsg.WriteVector( origin );
			outMsg.WriteFloat( angles.pitch );
			outMsg.WriteFloat( angles.yaw );
			outMsg.WriteFloat( angles.roll );
		}
		outMsg.Send();
	}
}

/*
============
Cmd_SetSpawnPoint_Completion
============
*/
static void Cmd_SetSpawnPoint_Completion( const idCmdArgs& args, argCompletionCallback_t callback ) {
	if ( args.Argc() > 1 ) {
		return;
	}

	callback( va( "%s next", args.Argv( 0 ) ) );
	callback( va( "%s prev", args.Argv( 0 ) ) );
	callback( va( "%s default", args.Argv( 0 ) ) );
	callback( va( "%s base", args.Argv( 0 ) ) );
}

/*
===============
Cmd_SetSpawnPoint_f
===============
*/
static void Cmd_SetSpawnPoint_f( const idCmdArgs &args ) {
	if ( args.Argc() < 2 ) {
		gameLocal.Printf( "usage: setSpawnPoint <command>\n" );
		gameLocal.Printf( "commands: next      - cycle to the next spawn point\n" );
		gameLocal.Printf( "commands: prev      - cycle to the previous spawn point\n" );
		gameLocal.Printf( "commands: default   - select the default spawn point\n" );
		gameLocal.Printf( "commands: base      - select the lowest priority spawn point (normally the main base)\n" );
		return;
	}

	idPlayer* player = gameLocal.GetLocalPlayer();
	if ( player == NULL ) {
		gameLocal.Printf( "No local player\n" );
		return;
	}

	sdTeamInfo* team = player->GetGameTeam();
	if ( team == NULL ) {
		return;
	}
	
	idEntity* desiredSpawn = NULL;
	idEntity* defaultSpawn = team->GetDefaultSpawn();
	
	const char* commandString = args.Argv( 1 );
	bool next = !idStr::Icmp( "next", commandString );
	bool prev = !idStr::Icmp( "prev", commandString );
	if ( next || prev ) {
		const idEntity* playerSpawnLoc = player->GetSpawnPoint();
		if ( playerSpawnLoc == NULL ) {
			playerSpawnLoc = defaultSpawn;
		}

		int numSpawns = team->GetNumSpawnLocations();
		// find the player's current spawn
		int currentSpawnIndex = 0;
		for ( int i = 0; i < numSpawns; i++ ) {
			if ( team->GetSpawnLocation( i ) == playerSpawnLoc ) {
				currentSpawnIndex = i;
				break;
			}
		}

		// find the previous/next one
		int indexUpto = currentSpawnIndex;
		for ( int i = 0; i < numSpawns; i++ ) {
			if ( prev ) {
				indexUpto = ( indexUpto + 1 ) % numSpawns;
			} else {
				indexUpto = ( indexUpto + numSpawns - 1 ) % numSpawns;
			}

			idEntity* next = team->GetSpawnLocation( indexUpto );
			if ( next != NULL ) {
				desiredSpawn = next;
				break;
			}
		}
	} else if ( !idStr::Icmp( "default", commandString ) ) {
		desiredSpawn = defaultSpawn;

	} else if ( !idStr::Icmp( "base", commandString ) ) {
		desiredSpawn = team->GetSpawnLocation( team->GetNumSpawnLocations() - 1 );
	}

	if ( desiredSpawn != NULL ) {
		if( !gameLocal.isClient ) {
			player->SetSpawnPoint( desiredSpawn );
		} else {
			sdReliableClientMessage msg( GAME_RELIABLE_CMESSAGE_SETSPAWNPOINT );
			msg.WriteLong( gameLocal.GetSpawnId( desiredSpawn ) );
			msg.Send();
		}
	}
}

/*
===============
Cmd_SetIPAutoAuth_f
===============
*/
static void Cmd_SetIPAutoAuth_f( const idCmdArgs& args ) {
	if ( args.Argc() < 3 ) {
		gameLocal.Warning( "You must specify the ip and user group to auth to" );
		return;
	}

	clientGUIDLookup_t lookup;
	lookup.ip = 0;
	lookup.pbid = 0;

	const char* iptext = args.Argv( 1 );
	if ( !sdGUIDFile::IPForString( iptext, lookup.ip ) ) {
		gameLocal.Warning( "Failed to parse IP '%s'", iptext );
		return;
	}

	const char* authGroup = args.Argv( 2 );

	gameLocal.guidFile.SetAutoAuth( lookup, authGroup );
}

/*
===============
Cmd_SetPBAutoAuth_f
===============
*/
static void Cmd_SetPBAutoAuth_f( const idCmdArgs& args ) {
	if ( args.Argc() < 3 ) {
		gameLocal.Warning( "You must specify the pb guid and user group to auth to" );
		return;
	}

	clientGUIDLookup_t lookup;
	lookup.ip = 0;
	lookup.pbid = 0;

	const char* pbtext = args.Argv( 1 );

	if ( !sdGUIDFile::PBGUIDForString( pbtext, lookup.pbid ) ) {
		gameLocal.Warning( "Failed to parse PB GUID '%s'", pbtext );
		return;
	}

	const char* authGroup = args.Argv( 2 );

	gameLocal.guidFile.SetAutoAuth( lookup, authGroup );
}

/*
===============
Cmd_SetGUIDAutoAuth_f
===============
*/
static void Cmd_SetGUIDAutoAuth_f( const idCmdArgs& args ) {
	if ( args.Argc() < 3 ) {
		gameLocal.Warning( "You must specify the game guid and user group to auth to" );
		return;
	}

	clientGUIDLookup_t lookup;
	lookup.ip = 0;
	lookup.pbid = 0;

	const char* guidtext = args.Argv( 1 );

	if ( !sdGUIDFile::GUIDForString( guidtext, lookup.clientId ) ) {
		gameLocal.Warning( "Failed to parse GUID '%s'", guidtext );
		return;
	}

	const char* authGroup = args.Argv( 2 );

	gameLocal.guidFile.SetAutoAuth( lookup, authGroup );
}

/*
===============
Cmd_GetRepeaterStatus_f
===============
*/
static void Cmd_GetRepeaterStatus_f( const idCmdArgs& args ) {
	if ( !gameLocal.isClient ) {
		gameLocal.Printf( "Client is not connected to a server\n" );
		return;
	}

	sdReliableClientMessage msg( GAME_RELIABLE_CMESSAGE_REPEATERSTATUS );
	msg.Send();
}

/*
=================
idGameLocal::InitConsoleCommands

Let the system know about all of our commands
so it can perform tab completion
=================
*/
void idGameLocal::InitConsoleCommands( void ) {
	cmdSystem->AddCommand( "game_memory",			idClass::DisplayInfo_f,		CMD_FL_GAME,				"displays game class info" );
	cmdSystem->AddCommand( "listClasses",			idClass::ListClasses_f,		CMD_FL_GAME,				"lists game classes" );

	cmdSystem->AddCommand( "wikiClassTree",			idClass::WikiClassTree_f,	CMD_FL_GAME,				"dumps wiki formatted entity class information" );
	cmdSystem->AddCommand( "wikiClassPage",			idClass::WikiClassPage_f,	CMD_FL_GAME,				"dumps wiki formatted entity class information" );
	cmdSystem->AddCommand( "wikiEventInfo",			Cmd_WikiEventInfo_f,		CMD_FL_GAME,				"dumps wiki formatted script event information" );
	cmdSystem->AddCommand( "wikiFormatCode",		Cmd_WikiFormatCode_f,		CMD_FL_GAME,				"dumps wiki formatted versions of the script files" );
	cmdSystem->AddCommand( "wikiScriptTree",		Cmd_WikiScriptTree_f,		CMD_FL_GAME,				"dumps wiki formatted script object hierarchy" );
	cmdSystem->AddCommand( "wikiScriptClassPage",	Cmd_WikiClassInfo_f,		CMD_FL_GAME,				"dumps wiki formatted script object information" );
	cmdSystem->AddCommand( "wikiScriptFileList",	Cmd_WikiScriptFileList_f,	CMD_FL_GAME,				"dumps wiki formatted script file list" );

	cmdSystem->AddCommand( "listThreads",			sdProgram::ListThreads_f,	CMD_FL_GAME|CMD_FL_CHEAT,	"lists script threads" );
	cmdSystem->AddCommand( "listScriptObjects",		Cmd_ListScriptObjects_f,	CMD_FL_GAME|CMD_FL_CHEAT,	"lists script objects" );
	cmdSystem->AddCommand( "listEntities",			Cmd_EntityList_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"lists game entities" );
	cmdSystem->AddCommand( "listActiveEntities",	Cmd_ActiveEntityList_f,		CMD_FL_GAME|CMD_FL_CHEAT,	"lists active game entities" );
	cmdSystem->AddCommand( "listSpawnArgs",			Cmd_ListSpawnArgs_f,		CMD_FL_GAME|CMD_FL_CHEAT,	"list the spawn args of an entity", idGameLocal::ArgCompletion_EntityName );
	cmdSystem->AddCommand( "say",					Cmd_Say_f,					CMD_FL_GAME,				"text chat" );
	cmdSystem->AddCommand( "sayTeam",				Cmd_SayTeam_f,				CMD_FL_GAME,				"team text chat" );
	cmdSystem->AddCommand( "sayFireteam",			Cmd_SayFireTeam_f,			CMD_FL_GAME,				"fireteam text chat" );
	cmdSystem->AddCommand( "addChatLine",			Cmd_AddChatLine_f,			CMD_FL_GAME,				"internal use - core to game chat lines" );
	cmdSystem->AddCommand( "addObituaryLine",		Cmd_AddObituaryLine_f,		CMD_FL_GAME,				"internal use - core to add obituary lines" );
	cmdSystem->AddCommand( "give",					Cmd_Give_f,					CMD_FL_GAME|CMD_FL_CHEAT,	"gives one or more items" );
	cmdSystem->AddCommand( "god",					Cmd_God_f,					CMD_FL_GAME|CMD_FL_CHEAT,	"enables god mode" );
	cmdSystem->AddCommand( "notarget",				Cmd_Notarget_f,				CMD_FL_GAME|CMD_FL_CHEAT,	"disables the player as a target" );
	cmdSystem->AddCommand( "noclip",				Cmd_Noclip_f,				CMD_FL_GAME|CMD_FL_CHEAT,	"disables collision detection for the player" );
	cmdSystem->AddCommand( "kill",					Cmd_Kill_f,					CMD_FL_GAME,				"kills the player" );
#ifdef SD_PRIVATE_BETA_BUILD
	cmdSystem->AddCommand( "where",					Cmd_GetViewpos_f,			CMD_FL_GAME,				"prints the current view position (x y z) yaw pitch roll" );
	cmdSystem->AddCommand( "getviewpos",			Cmd_GetViewpos_f,			CMD_FL_GAME,				"prints the current view position (x y z) yaw pitch roll" );
#else
	cmdSystem->AddCommand( "where",					Cmd_GetViewpos_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"prints the current view position (x y z) yaw pitch roll" );
	cmdSystem->AddCommand( "getviewpos",			Cmd_GetViewpos_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"prints the current view position (x y z) yaw pitch roll" );
#endif
	cmdSystem->AddCommand( "setviewpos",			Cmd_SetViewpos_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"sets the current view position (x y z) yaw pitch roll" );
	cmdSystem->AddCommand( "teleport",				Cmd_Teleport_f,				CMD_FL_GAME|CMD_FL_CHEAT,	"teleports the player to an entity location", idGameLocal::ArgCompletion_EntityName );
	cmdSystem->AddCommand( "trigger",				Cmd_Trigger_f,				CMD_FL_GAME|CMD_FL_CHEAT,	"triggers an entity", idGameLocal::ArgCompletion_EntityName );
	cmdSystem->AddCommand( "spawn",					Cmd_Spawn_f,				CMD_FL_GAME|CMD_FL_CHEAT,	"spawns a game entity", idArgCompletionDecl_f< DECLTYPE_ENTITYDEF > );
	cmdSystem->AddCommand( "networkSpawn",			Cmd_NetworkSpawn_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"spawns a game entity during a network game", idArgCompletionDecl_f< DECLTYPE_ENTITYDEF > );
	cmdSystem->AddCommand( "damage",				Cmd_Damage_f,				CMD_FL_GAME|CMD_FL_CHEAT,	"apply damage to an entity", idGameLocal::ArgCompletion_EntityName );
	cmdSystem->AddCommand( "remove",				Cmd_Remove_f,				CMD_FL_GAME|CMD_FL_CHEAT,	"removes an entity", idGameLocal::ArgCompletion_EntityName );
	cmdSystem->AddCommand( "killMoveables",			Cmd_KillMovables_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"removes all moveables" );
	cmdSystem->AddCommand( "killTransports",		Cmd_KillTransports_f,		CMD_FL_GAME|CMD_FL_CHEAT,	"removes all transports" );	
	cmdSystem->AddCommand( "killRagdolls",			Cmd_KillRagdolls_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"removes all ragdolls" );
	cmdSystem->AddCommand( "killClass",				Cmd_KillClass_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"removes all entities of 'class'" );	
	cmdSystem->AddCommand( "killType",				Cmd_KillType_f,				CMD_FL_GAME|CMD_FL_CHEAT,	"removes all entities of 'type'" );	
	cmdSystem->AddCommand( "activateAFs",			Cmd_ActivateAFs_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"activates idAFEntity based entities" );
	cmdSystem->AddCommand( "activateVehicles",		Cmd_ActivateVehicles_f,		CMD_FL_GAME|CMD_FL_CHEAT,	"activates physics on sdTransport based entities" );
	cmdSystem->AddCommand( "addline",				Cmd_AddDebugLine_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"adds a debug line" );
	cmdSystem->AddCommand( "addarrow",				Cmd_AddDebugLine_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"adds a debug arrow" );
	cmdSystem->AddCommand( "removeline",			Cmd_RemoveDebugLine_f,		CMD_FL_GAME|CMD_FL_CHEAT,	"removes a debug line" );
	cmdSystem->AddCommand( "blinkline",				Cmd_BlinkDebugLine_f,		CMD_FL_GAME|CMD_FL_CHEAT,	"blinks a debug line" );
	cmdSystem->AddCommand( "listLines",				Cmd_ListDebugLines_f,		CMD_FL_GAME|CMD_FL_CHEAT,	"lists all debug lines" );
	cmdSystem->AddCommand( "testLight",				Cmd_TestLight_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"tests a light" );
	cmdSystem->AddCommand( "testPointLight",		Cmd_TestPointLight_f,		CMD_FL_GAME|CMD_FL_CHEAT,	"tests a point light" );
	cmdSystem->AddCommand( "popLight",				Cmd_PopLight_f,				CMD_FL_GAME|CMD_FL_CHEAT,	"removes the last created light" );
	cmdSystem->AddCommand( "testDeath",				Cmd_TestDeath_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"tests death" );
	cmdSystem->AddCommand( "testModel",				idTestModel::TestModel_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"tests a model", idTestModel::ArgCompletion_TestModel );
	cmdSystem->AddCommand( "hideSurface",	idTestModel::TestModelHideSurfaceID_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"hides surface for testmodel" );
	cmdSystem->AddCommand( "showSurface",				idTestModel::TestModelShowSurfaceID_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"show surface for testmodel" );
	cmdSystem->AddCommand( "resetSurfaces",				idTestModel::TestModelResetSurfaceID_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"shows all surfaces for testmodel" );
	cmdSystem->AddCommand( "testSkin",				idTestModel::TestSkin_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"tests a skin on an existing testModel", idArgCompletionDecl_f< DECLTYPE_SKIN > );
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
	cmdSystem->AddCommand( "exportScript",			Cmd_ExportScript_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"exports scripts to a compilable format" );
	cmdSystem->AddCommand( "listCollisionModels",	Cmd_ListCollisionModels_f,	CMD_FL_GAME,				"lists collision models" );
	cmdSystem->AddCommand( "listGUIs",				Cmd_ListGUIs_f,				CMD_FL_GAME,				"lists all allocated GUIs" );
	cmdSystem->AddCommand( "collisionModelInfo",	Cmd_CollisionModelInfo_f,	CMD_FL_GAME,				"shows collision model info" );
	cmdSystem->AddCommand( "reloadAnims",			Cmd_ReloadAnims_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"reloads animations" );
	cmdSystem->AddCommand( "reloadGuiGlobals",		Cmd_ReloadGuiGlobals_f,		CMD_FL_GAME|CMD_FL_CHEAT,	"reloads gloabal gui properties" );
	cmdSystem->AddCommand( "reloadUserGroups",		Cmd_ReloadUserGroups_f,		CMD_FL_GAME,				"reloads user groups" );
	cmdSystem->AddCommand( "listAnims",				Cmd_ListAnims_f,			CMD_FL_GAME,				"lists all animations" );
	cmdSystem->AddCommand( "listEvents",			idEvent::ListEvents,		CMD_FL_GAME,				"lists all active events" );
	cmdSystem->AddCommand( "testDamage",			Cmd_TestDamage_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"tests a damage def", idArgCompletionGameDecl_f< DECLTYPE_DAMAGE > );
	cmdSystem->AddCommand( "saveSelected",			Cmd_SaveSelected_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"saves the selected entity to the .map file" );
	cmdSystem->AddCommand( "deleteSelected",		Cmd_DeleteSelected_f,		CMD_FL_GAME|CMD_FL_CHEAT,	"deletes selected entity" );
	cmdSystem->AddCommand( "saveMoveables",			Cmd_SaveMoveables_f,		CMD_FL_GAME|CMD_FL_CHEAT,	"save all moveables to the .map file" );
	cmdSystem->AddCommand( "saveRagdolls",			Cmd_SaveRagdolls_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"save all ragdoll poses to the .map file" );
	cmdSystem->AddCommand( "bindRagdoll",			Cmd_BindRagdoll_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"binds ragdoll at the current drag position" );
	cmdSystem->AddCommand( "unbindRagdoll",			Cmd_UnbindRagdoll_f,		CMD_FL_GAME|CMD_FL_CHEAT,	"unbinds the selected ragdoll" );
	cmdSystem->AddCommand( "clearLights",			Cmd_ClearLights_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"clears all lights" );
	cmdSystem->AddCommand( "gameError",				Cmd_GameError_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"causes a game error" );
	cmdSystem->AddCommand( "makeEnvMaps",			Cmd_makeEnvMaps_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"Recreates the environment maps for the level" );
	cmdSystem->AddCommand( "reportAnimState",		idActor::ReportCurrentState_f,		CMD_FL_GAME|CMD_FL_CHEAT,	"Reports the entity number's current animation state" );

	cmdSystem->AddCommand( "testPrecache",			Cmd_TestPrecache_f,			CMD_FL_GAME,				"Precaches an entitydef, then spawns it, to check for any additional unprecached media", idArgCompletionDecl_f< DECLTYPE_ENTITYDEF > );

	cmdSystem->AddCommand( "testProficiency",		Cmd_TestProficiency_f,		CMD_FL_GAME,				"", idArgCompletionGameDecl_f< DECLTYPE_PROFICIENCYITEM > );

	cmdSystem->AddCommand( "camera",				Cmd_Camera_f,				CMD_FL_GAME|CMD_FL_CHEAT,	"Sets the current view to a named camera entity, or clear the camera if no name is given" );
	cmdSystem->AddCommand( "cameraNext",			Cmd_CameraNext_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"Sets the current view to the next camera found in the map" );
	cmdSystem->AddCommand( "cameraPrev",			Cmd_CameraPrev_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"Sets the current view to the previous camera found in the map" );

	cmdSystem->AddCommand( "admin",					Cmd_Admin_f,				CMD_FL_GAME,				"perform administration commands", sdAdminSystemLocal::CommandCompletion );
	cmdSystem->AddCommand( "callvote",				sdVoteManagerLocal::Callvote_f,		CMD_FL_GAME,		"call a vote to change server settings, etc" );
	cmdSystem->AddCommand( "vote",					Cmd_Vote_f,					CMD_FL_GAME,				"send your vote response" );
	cmdSystem->AddCommand( "fireteam",				Cmd_Fireteam_f,				CMD_FL_GAME,				"perform fireteam related commands", sdFireTeamManagerLocal::CommandCompletion );

	cmdSystem->AddCommand( "useWeapon",				Cmd_UseWeapon_f,			CMD_FL_GAME,				"switch to the named inventory item. this is looked up from the player's team def" );

	cmdSystem->AddCommand( "takeViewNote",			Cmd_TakeViewNote_f,			CMD_FL_GAME,				"" );

	cmdSystem->AddCommand( "collisionTest",			Cmd_CollisionTest_f,		CMD_FL_GAME,				"runs through all entityDefs and checks for erroneous collision data whilst generating cached models" );

	cmdSystem->AddCommand( "stats",					sdStatsTracker::HandleCommand,	CMD_FL_GAME,				"stats debugging tool", sdStatsTracker::CommandCompletion );

#ifdef DEBUG_SCRIPTS
	cmdSystem->AddCommand( "dumpScriptStats",		Cmd_DumpScriptStats_f,		CMD_FL_GAME,				"" );
#endif // DEBUG_SCRIPTS

	cmdSystem->AddCommand( "disasmScript",			Cmd_DisasmScript_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"disassembles script" );

#if defined( ID_ALLOW_TOOLS )
	cmdSystem->AddCommand( "exportmodels",			Cmd_ExportModels_f,			CMD_FL_GAME|CMD_FL_CHEAT,	"exports models", ArgCompletion_DefFile );
	cmdSystem->AddCommand( "reexportModels",		Cmd_ReexportModels_f,		CMD_FL_GAME|CMD_FL_CHEAT,	"reexports models", ArgCompletion_DefFile );
#endif /* ID_ALLOW_TOOLS */

	// multiplayer server commands
	cmdSystem->AddCommand( "serverMapRestart",		idGameLocal::MapRestart_f,	CMD_FL_GAME,				"restart the current game" );
	cmdSystem->AddCommand( "nextMap",				idGameLocal::NextMap_f,		CMD_FL_GAME,				"change to the next map" );
	cmdSystem->AddCommand( "serverStartDemos",		idGameLocal::StartDemos_f,	CMD_FL_GAME,				"forces all clients to start recording demos" );

	cmdSystem->AddCommand( "zoomInCommandMap",		idPlayer::ZoomInCommandMap_f,	CMD_FL_GAME,			"zoom in the command map" );
	cmdSystem->AddCommand( "zoomOutCommandMap",		idPlayer::ZoomOutCommandMap_f,	CMD_FL_GAME,			"zoom out the command map" );

	cmdSystem->AddCommand( "aasStats",				idBot::Cmd_AASStats_f,				CMD_FL_GAME,		"show AAS statistics\n" );
	cmdSystem->AddCommand( "nodeAdd",				idBotNodeGraph::Cmd_NodeAdd_f,		CMD_FL_GAME,		"adds a vehicle nav node to the map for the bots" );
	cmdSystem->AddCommand( "nodeDel",				idBotNodeGraph::Cmd_NodeDel_f,		CMD_FL_GAME,		"deletes the nearest node." );
	cmdSystem->AddCommand( "nodeName",				idBotNodeGraph::Cmd_NodeName_f,		CMD_FL_GAME,		"renames the nearest node (handy for script access).  argument: <newName>" );
	cmdSystem->AddCommand( "nodeTeam",				idBotNodeGraph::Cmd_NodeTeam_f,		CMD_FL_GAME,		"changes the team the nearest node is associated with.  team: <strogg|gdf|none>" );
	cmdSystem->AddCommand( "nodeRadius",			idBotNodeGraph::Cmd_NodeRadius_f,	CMD_FL_GAME,		"sets the radius for the nearest node.  argument: <radius>" );
	cmdSystem->AddCommand( "nodeView",				idBotNodeGraph::Cmd_NodeView_f,		CMD_FL_GAME,		"view a node" );
	cmdSystem->AddCommand( "nodeLink",				idBotNodeGraph::Cmd_CreateLink_f,	CMD_FL_GAME,		"create a link from the last node to this one and back.  pass 'oneway' to make it a one way link.  pass 'clear' to start a new chain of links." );
	cmdSystem->AddCommand( "saveNodes",				idBotNodeGraph::Cmd_SaveNodes_f,	CMD_FL_GAME,		"saves the nav nodes to <mapname>.nav" );
	cmdSystem->AddCommand( "nodeActive",			idBotNodeGraph::Cmd_NodeActive_f,	CMD_FL_GAME,		"sets a node to active, or inactive" );
	cmdSystem->AddCommand( "nodeFlags",				idBotNodeGraph::Cmd_NodeFlags_f,	CMD_FL_GAME,		"sets a node's flags. 0 = NORMAL, 1 = WATER" );
//	cmdSystem->AddCommand( "nodeGenerateFromBotVehicleActions",idBotNodeGraph::Cmd_GenerateFromBotActions_f, CMD_FL_GAME,	"generate nodes from the bot's vehicle camp/roam actions" );

	cmdSystem->AddCommand( "killAllBots",			idBotThreadData::Cmd_KillAllBots_f,		CMD_FL_GAME,	"causes all bots on the server to suicide and respawn" );
	cmdSystem->AddCommand( "resetAllBotsGoals",		idBotThreadData::Cmd_ResetAllBots_f,	CMD_FL_GAME,	"causes all bots to dump their current goals and start fresh" );

	cmdSystem->AddCommand( "dumpToolTips",			sdDeclToolTip::Cmd_DumpTooltips_f,	CMD_FL_GAME,		"dumps out all the tooltips to tooltips.txt" );
	cmdSystem->AddCommand( "clearToolTipCookies",	sdDeclToolTip::Cmd_ClearCookies_f,	CMD_FL_GAME,		"clears all tooltip state cookies, so they will be played again" );

	cmdSystem->AddCommand( "checkRenderEntityHandles",	Cmd_CheckRenderEntityHandles_f,	CMD_FL_GAME,		"" );

	cmdSystem->AddCommand( "dumpCollisionModelStats",	Cmd_DumpCollsionModelStats_f,	CMD_FL_GAME,		"" );
	cmdSystem->AddCommand( "touchCollision",			Cmd_TouchCollision_f,			CMD_FL_GAME,		"" );

	cmdSystem->AddCommand( "listVotes",					Cmd_ListVotes_f,				CMD_FL_GAME,		"" );

	cmdSystem->AddCommand( "dumpMonolithicInfo",		Cmd_DumpMonolithicInfo_f,		CMD_FL_GAME,		"" );

#ifdef CLIP_DEBUG
	cmdSystem->AddCommand( "printTraceTimings",			Cmd_PrintTraceTimings_f,		CMD_FL_GAME,		"" );
	cmdSystem->AddCommand( "clearTraceTimings",			Cmd_ClearTraceTimings_f,		CMD_FL_GAME,		"" );
#endif // CLIP_DEBUG

#ifdef CLIP_DEBUG_EXTREME
	cmdSystem->AddCommand( "toggleTraceLogging",		Cmd_ToggleTraceLogging_f,		CMD_FL_GAME,		"" );
#endif // CLIP_DEBUG_EXTREME

	cmdSystem->AddCommand( "gameCrash",					Cmd_GameCrash_f,				CMD_FL_GAME,		"cause a crash in the game module (dev purposes)" );

	cmdSystem->AddCommand( "pauseGame",					Cmd_PauseGame_f,				CMD_FL_GAME,		"pauses the game" );
	cmdSystem->AddCommand( "unPauseGame",				Cmd_UnPauseGame_f,				CMD_FL_GAME,		"unpauses the game" );

#if !defined( SD_DEMO_BUILD )
	cmdSystem->AddCommand( "printUserGUID",				Cmd_PrintUserGUID_f,			CMD_FL_GAME,		"prints the guid of the currently logged in user" );
#endif /* !SD_DEMO_BUILD */

	cmdSystem->AddCommand( "setSpectateClient",			Cmd_SetSpectateClient_f,		CMD_FL_GAME,		"switches to spectating the client specified, either by name or index" );
	cmdSystem->AddCommand( "spectate",					Cmd_Spectate_f,					CMD_FL_GAME,		"commands to shift spectator positions", Cmd_Spectate_Completion );
	cmdSystem->AddCommand( "setSpawnPoint",				Cmd_SetSpawnPoint_f,			CMD_FL_GAME,		"commands to change which spawn point you have selected", Cmd_SetSpawnPoint_Completion );

	cmdSystem->AddCommand( "setIPAutoAuth",				Cmd_SetIPAutoAuth_f,			CMD_FL_GAME,		"sets auto auth for the given IP" );
	cmdSystem->AddCommand( "setPBAutoAuth",				Cmd_SetPBAutoAuth_f,			CMD_FL_GAME,		"sets auto auth for the given punkbuster guid" );
	cmdSystem->AddCommand( "setGUIDAutoAuth",			Cmd_SetGUIDAutoAuth_f,			CMD_FL_GAME,		"sets auto auth for the given game guid" );

	cmdSystem->AddCommand( "getRepeaterStatus",			Cmd_GetRepeaterStatus_f,		CMD_FL_GAME,		"displays whether there the repeater server is running on the server the client is currently connected to" );
}

/*
=================
idGameLocal::ShutdownConsoleCommands
=================
*/
void idGameLocal::ShutdownConsoleCommands( void ) {
	cmdSystem->RemoveFlaggedCommands( CMD_FL_GAME );
}
