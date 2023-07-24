// Copyright (C) 2007 Id Software, Inc.
//


#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "UserInterfaceLocal.h"
#include "UserInterfaceExpressions.h"
#include "UserInterfaceManagerLocal.h"

//#include "../interfaces/ResupplyInterface.h"
#include "../proficiency/StatsTracker.h"

#include "../Player.h"
#include "../Weapon.h"

#include "../rules/GameRules.h"
#include "../rules/VoteManager.h"
#include "../roles/FireTeams.h"
#include "../roles/Tasks.h"
#include "../script/Script_Helper.h"
#include "../script/Script_ScriptObject.h"
#include "../gamesys/SysCmds.h"

using namespace sdProperties;

/*
============
sdUserInterfaceLocal::Script_GetTeamPlayerCount
============
*/
void sdUserInterfaceLocal::Script_GetTeamPlayerCount( sdUIFunctionStack& stack ) {
	idStr teamName;
	stack.Pop( teamName );	

	if( !gameLocal.rules ) {
		stack.Push( 0 );
		return;
	}

	int index = gameLocal.rules->TeamIndexForName( teamName );
	if( index == -1 ) {
		stack.Push( 0 );
		return;
	}

	sdTeamInfo* team = NULL;
	if( index > 0 ) {
		team = &sdTeamManager::GetInstance().GetTeamByIndex( index - 1 );
	}

	int count = 0;
	for( int i = 0; i < gameLocal.numClients; i++ ) {
		idPlayer* client = gameLocal.GetClient( i );
		if( !client || client->GetGameTeam() != team )  {
			continue;
		}

		count++;
	}
	stack.Push( count );
}

/*
============
sdUserInterfaceLocal::Script_GetSpecatorList
============
*/
idCVar g_debugSpecatorList( "g_debugSpecatorList", "0", CVAR_GAME | CVAR_INTEGER, "fills the spectator list with fake players" );

void sdUserInterfaceLocal::Script_GetSpecatorList( sdUIFunctionStack& stack ) {
	sdStringBuilder_Heap spectators;
	for( int i = 0; i < g_debugSpecatorList.GetInteger(); i++ ) {
		if( spectators.Length() > 0 ) {
			spectators += ", ";
		}
		spectators += va( "Spectator %i", i );
	}

	idStaticList< idPlayer*, 32 > players;

	for( int i = 0; i < gameLocal.numClients; i++ ) {
		idPlayer* client = gameLocal.GetClient( i );
		if( !client || ( !client->IsSpectator() ) )  {
			continue;
		}
		if( client->GetUserInfo().name.IsEmpty() ) {
			continue;
		}

		players.Append( client );
	}

	for( int i = 0; i < players.Num(); i++ ) {
		if( spectators.Length() > 0 ) {
			spectators += ", ";
		}
		spectators += players[ i ]->GetUserInfo().name;
	}
	stack.Push( spectators.c_str() );
}

/*
============
sdUserInterfaceLocal::Script_UpdateLimboProficiency
============
*/
void sdUserInterfaceLocal::Script_UpdateLimboProficiency( sdUIFunctionStack& stack ) {
	idStr className;
	stack.Pop( className );

	gameLocal.limboProperties.SetProficiencySource( className );	
}

/*
============
sdUserInterfaceLocal::EnumerateItemNodeForBank
jrad -	this only allows the selection from within a single optional package
if we ever allow additional optional packages both this function and the limbo menu will need to be updated
if package is -1, all available optional items will be returned
============
*/
void sdUserInterfaceLocal::EnumerateItemNodeForBank( idList< itemBankPair_t >& items, int slot, const sdDeclItemPackageNode& node, int packageIndex ) {
	idPlayer* localPlayer = gameLocal.GetLocalPlayer();
	if ( localPlayer != NULL ) {
		if ( !node.GetRequirements().Check( localPlayer ) ) {
			return;
		}
	}

	for ( int i = 0; i < node.GetItems().Num(); i++ ) {
		const sdDeclInvItem* item = node.GetItems()[ i ];
		if ( item->GetWeaponMenuIgnore() ) {
			continue;
		}

		if ( item->GetSlot() == slot ) {
			itemBankPair_t info( item, packageIndex );
			items.Append( info );
		}
	}

	for ( int i = 0; i < node.GetNodes().Num(); i++ ) {
		EnumerateItemNodeForBank( items, slot, *node.GetNodes()[ i ], packageIndex );
	}
}

/*
============
sdUserInterfaceLocal::EnumerateItemsForBank
jrad -	this only allows the selection from within a single optional package
if we ever allow additional optional packages both this function and the limbo menu will need to be updated
if package is -1, all available optional items will be returned
============
*/
void sdUserInterfaceLocal::EnumerateItemsForBank( idList< itemBankPair_t >& items, const char* className, int slot, int package ) {
	items.Clear();

	idPlayer* localPlayer = gameLocal.GetLocalPlayer();
	if ( localPlayer == NULL ) {
		return;
	}

	const sdDeclPlayerClass* playerClass = gameLocal.declPlayerClassType[ className ];
	if ( playerClass == NULL ) {
		return;
	}

	if( playerClass->GetPackage() == NULL ) {
		return;
	}

	// add any default (non-selectable) items
	EnumerateItemNodeForBank( items, slot, playerClass->GetPackage()->GetItemRoot(), -1 );

	if ( playerClass->GetNumOptions() > 0 ) {
		const sdDeclPlayerClass::optionList_t& packageOptions = playerClass->GetOption( 0 );
		if ( package < 0 ) {
			for ( int i = 0; i < packageOptions.Num(); i++ ) {
				EnumerateItemNodeForBank( items, slot, packageOptions[ i ]->GetItemRoot(), i );
			}
		} else if ( package < packageOptions.Num() ) {
			EnumerateItemNodeForBank( items, slot, packageOptions[ package ]->GetItemRoot(), package );
		}
	}
}


/*
============
sdUserInterfaceLocal::Script_GetWeaponData
============
*/
void sdUserInterfaceLocal::Script_GetWeaponData( sdUIFunctionStack& stack ) {
	idStr className;
	int slot;
	int item;
	int package;
	idStr key;

	stack.Pop( className );
	stack.Pop( slot );
	stack.Pop( item );
	stack.Pop( package );
	stack.Pop( key );

	idList< itemBankPair_t > items;
	EnumerateItemsForBank( items, className, slot, package );

	if ( item >= 0 && item < items.Num() ) {
		stack.Push( items[ item ].first->GetData().GetString( key ) );
	} else {
		stack.Push( "" );
	}
}


/*
============
sdUserInterfaceLocal::Script_GetWeaponBankForName
============
*/
void sdUserInterfaceLocal::Script_GetWeaponBankForName( sdUIFunctionStack& stack ) {
	idStr name;
	stack.Pop( name );

	int bankNum = -1;
	if ( name.Length() > 0 ) {
		const sdDeclInvSlot* slot = gameLocal.declInvSlotType.LocalFind( name, false );
		if( slot ) {			
			bankNum = slot->GetBank() + 1;
		}
	}
	if( bankNum == -1 ) {
		gameLocal.Warning( "sdUserInterfaceLocal::Script_GetWeaponBankForName: '%s' unknown inventory slot '%s'", GetName(), name.c_str() );
	}
	stack.Push( bankNum );
}

/*
============
sdUserInterfaceLocal::Script_GetNumWeaponPackages
============
*/
void sdUserInterfaceLocal::Script_GetNumWeaponPackages( sdUIFunctionStack& stack ) {
	idStr className;
	stack.Pop( className );

	idPlayer* localPlayer = gameLocal.GetLocalPlayer();
	if ( localPlayer == NULL ) {
		stack.Push( 0 );
		return;
	}

	const sdDeclPlayerClass* playerClass = gameLocal.declPlayerClassType.LocalFind( className, false );
	if ( playerClass == NULL || playerClass->GetNumOptions() == 0 ) {
		stack.Push( 0 );
		return;
	}

	int count = 0;
	const sdDeclPlayerClass::optionList_t& options = playerClass->GetOption( 0 );
	for ( int i = 0; i < options.Num(); i++ ) {
		if ( localPlayer->GetInventory().CheckItems( options[ i ] ) ) {
			count++;
		}
	}
	stack.Push( count );
}

/*
============
sdUserInterfaceLocal::Script_GetRoleCountForTeam
============
*/
void sdUserInterfaceLocal::Script_GetRoleCountForTeam( sdUIFunctionStack& stack ) {
	idStr teamName;
	idStr roleName;
	stack.Pop( teamName );
	stack.Pop( roleName );

	sdTeamInfo* team = sdTeamManager::GetInstance().GetTeamSafe( teamName );
	if( !team ) {
		gameLocal.Warning( "sdUserInterfaceLocal::Script_GetRoleCountForTeam: '%s' Unknown team '%s'", GetName(), teamName.c_str() );
		return;
	}

	// jrad - we do the counting manually, rather than using gameLocal.ClassCount() to avoid causing 
	// pre-caching of the player class declarations

	int count = 0;
	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		idPlayer* player = gameLocal.GetClient( i );
		if ( player == NULL ) {
			continue;
		}

		if ( team && player->GetGameTeam() != team ) {
			continue;
		}

		const sdInventory& inv = player->GetInventory();

		const sdDeclPlayerClass* pc = inv.GetClass();
		if( ( pc && ( roleName.Icmp( pc->GetName() ) == 0 ) ) ) {
			count++;	
		}
	}

	stack.Push( count );
}

/*
============
sdUserInterfaceLocal::Script_GetEquivalentClass
============
*/
void sdUserInterfaceLocal::Script_GetEquivalentClass( sdUIFunctionStack& stack ) {
	idStr currentTeam;
	idStr targetTeam;
	idStr currentClass;

	stack.Pop( currentTeam );
	stack.Pop( targetTeam );
	stack.Pop( currentClass );

	if( targetTeam.IsEmpty() || gameLocal.rules == NULL ) {
		stack.Push( "" );
		return;
	}

	int srcIndex = gameLocal.rules->TeamIndexForName( currentTeam );
	int targetIndex = gameLocal.rules->TeamIndexForName( targetTeam );

	// going from spec/nothing to a team, find the default class
	if( srcIndex == 0 || srcIndex == -1 && targetIndex > 0) {
		const sdTeamInfo& srcTeamObj = sdTeamManager::GetInstance().GetTeamByIndex( targetIndex - 1 );
		const sdDeclPlayerClass* pc = srcTeamObj.GetDefaultClass();
		if( pc != NULL ) {
			stack.Push( pc->GetName() );
		} else {
			stack.Push( "" );
		}
		return;
	}
	if( srcIndex == 0 || targetIndex == 0 ) {
		stack.Push( "" );
		return;
	}

	const sdDeclPlayerClass* currentClassObj = gameLocal.declPlayerClassType.LocalFind( currentClass, false );
	const sdTeamInfo& srcTeamObj = sdTeamManager::GetInstance().GetTeamByIndex( srcIndex - 1 );
	const sdTeamInfo& targetTeamObj = sdTeamManager::GetInstance().GetTeamByIndex( targetIndex - 1 );

	if( currentClassObj == NULL ) {
		currentClassObj = srcTeamObj.GetDefaultClass();
		if( currentClassObj == NULL ) {
			return;
		}
	}

	const sdDeclPlayerClass* remappedClass = srcTeamObj.GetEquivalentClass( *currentClassObj, targetTeamObj );
	if( remappedClass == NULL  ) {
		gameLocal.Warning( "sdUserInterfaceLocal::Script_GetEquivalentClass: could not find equivalent class for '%s' on team '%s'", currentClass.c_str(), targetTeam.c_str() );
		stack.Push( "" );
	}

	stack.Push( remappedClass->GetName() );
}

/*
============
sdUserInterfaceLocal::Script_GetClassSkin
============
*/
void sdUserInterfaceLocal::Script_GetClassSkin( sdUIFunctionStack& stack ) {
	idStr className;
	stack.Pop( className );

	const idDeclSkin* skin = NULL;

	const sdDeclPlayerClass* currentClassObj = gameLocal.declPlayerClassType[ className ];
	if ( currentClassObj != NULL ) {
		skin = sdInventory::SkinForClass( currentClassObj );
	}

	stack.Push( skin ? skin->GetName() : "" );
}

/*
============
sdUserInterfaceLocal::Script_ToggleReady
============
*/
void sdUserInterfaceLocal::Script_ToggleReady( sdUIFunctionStack& stack ) {
	idPlayer* p = gameLocal.GetLocalPlayer();
	if( p != NULL ) {
		p->PerformImpulse( UCI_READY );
	}
}

/*
============
sdUserInterfaceLocal::Script_ExecVote
============
*/
void sdUserInterfaceLocal::Script_ExecVote( sdUIFunctionStack& stack ) {
	sdVoteManager::GetInstance().ExecVote( this );
}

/*
============
sdUserInterfaceLocal::Script_GetCommandMapTitle
============
*/
void sdUserInterfaceLocal::Script_GetCommandMapTitle( sdUIFunctionStack& stack ) {
	int id;
	stack.Pop( id );

	const sdPlayZone* playZone = gameLocal.GetChoosablePlayZone( id );
	if( playZone == NULL || playZone->GetTitle() == NULL ) {
		stack.Push( declHolder.declLocStrType.LocalFind( "blank" )->Index() );
		return;
	}
	stack.Push( playZone->GetTitle()->Index() );
}

/*
============
sdUserInterfaceLocal::Script_GetPersistentRankInfo
============
*/
void sdUserInterfaceLocal::Script_GetPersistentRankInfo( sdUIFunctionStack& stack ) {
	idStr type;
	stack.Pop( type );

	gameLocal.rankInfo.CreateData( sdGlobalStatsTracker::GetInstance().GetLocalStatsHash(), sdGlobalStatsTracker::GetInstance().GetLocalStats(), gameLocal.rankScratchInfo );

	const sdDeclRank* rank = gameLocal.FindRankForLevel( gameLocal.rankScratchInfo.completeTasks );
	if ( rank != NULL ) {
		if( !type.Icmp( "title" ) ) {
			stack.Push( rank->GetTitle()->GetName() );
			return;
		} else if( !type.Icmp( "material" ) ) {
			stack.Push( rank->GetMaterial() );
			return;
		} else if( !type.Icmpn( "next", 4 ) ) {			
			const idList< sdPersistentRankInfo::sdBadge >& badges = gameLocal.rankInfo.GetBadges();

			const sdPersistentRankInfo::sdBadge* bestBadge = NULL;
			const sdPersistentRankInfo::sdRankInstance::sdBadge* bestDataBadge = NULL;
			int mostSatisfied = -1;
			int mostSatisfiedTotal = -1;
			float bestPercentComplete = -1.0f;

			bool allComplete = true;
			for( int i = 0; i < badges.Num(); i++ ) {
				const sdPersistentRankInfo::sdBadge& badge = badges[ i ];
				const sdPersistentRankInfo::sdRankInstance::sdBadge& dataBadge = gameLocal.rankScratchInfo.badges[ i ];


				int numComplete = 0;
				float percentComplete = 0.0f;
				for( int j = 0; j < badge.tasks.Num(); j++ ) {
					float value = idMath::ClampFloat( 0.0f, dataBadge.taskValues[ j ].max, idMath::Floor( dataBadge.taskValues[ j ].value ) );
					float percent = dataBadge.taskValues[ j ].value / dataBadge.taskValues[ j ].max;
					percentComplete += idMath::ClampFloat( 0.0f, 1.0f, percent );
					if( percent >= 1.0f ) {
						numComplete++;
					}
				}
				if( numComplete == badge.tasks.Num() ) {
					continue;
				}
				allComplete = false;

				if( badge.tasks.Num() > 0 ) {
					percentComplete /= badge.tasks.Num();
				}
				if( percentComplete > bestPercentComplete ) {
					bestBadge = &badge;
					bestDataBadge = &dataBadge;
					mostSatisfied = numComplete;
					mostSatisfiedTotal = badge.tasks.Num();
					bestPercentComplete = percentComplete;
				}
			}

			if( !idStr::Icmp( type.c_str() + 4, "achievementAvailable" )) {
				stack.Push( allComplete ? "0" : "1" );
				return;
			}

			if( bestBadge != NULL ) {
				if( !idStr::Icmp( type.c_str() + 4, "achievementTitle" )) {
					stack.Push( va( "game/achievements/%s_level%i", bestBadge->category.c_str(), bestBadge->level ) );
				} else if( !idStr::Icmp( type.c_str() + 4, "achievementTasks" )) {
					stack.Push( va( "%i", mostSatisfied ) );
				} else if( !idStr::Icmp( type.c_str() + 4, "achievementTasksTotal" )) {
					stack.Push( va( "%i", mostSatisfiedTotal ) );
				} else if( !idStr::Icmp( type.c_str() + 4, "achievementPercent" )) {
					stack.Push( va( "%f", idMath::Ceil( ( bestPercentComplete * 100.0f ) ) / 100.0f ) );
				} else if( !idStr::Icmp( type.c_str() + 4, "achievementMaterial" )) {
					stack.Push( va( "guis/assets/icons/achieve_%s_rank%i", bestBadge->category.c_str(), bestBadge->level ) );
				} else {
					stack.Push( "" );
				}
			} else {
				stack.Push( "" );
			}
			return;
		}
	}

	gameLocal.Warning( "Script_GetPersistentRankInfo: unknown rank info type '%s' for rank index '%i'", type.c_str(), gameLocal.rankScratchInfo.completeTasks );
	stack.Push( "" );	
}

/*
============
sdUserInterfaceLocal::Script_SetSpawnPoint
============
*/
void sdUserInterfaceLocal::Script_SetSpawnPoint( sdUIFunctionStack& stack ) {
	idStr name;
	stack.Pop( name );

	idPlayer* localPlayer = gameLocal.GetLocalPlayer();
	if( localPlayer == NULL ) {
		return;
	}

	// reset to default spawn
	if( name.IsEmpty() ) {
		if( !gameLocal.isClient ) {
			localPlayer->SetSpawnPoint( NULL );
		} else {
			sdReliableClientMessage msg( GAME_RELIABLE_CMESSAGE_SETSPAWNPOINT );
			msg.WriteLong( -1 );
			msg.Send();
		}
		return;
	}

	idEntity* spawnPoint = gameLocal.FindEntity( name.c_str() );
	if( spawnPoint == NULL ) {
		gameLocal.Warning( "Script_SetSpawnPoint: could not find '%s'", name.c_str() );
		return;
	}

	if( !gameLocal.isClient ) {
		localPlayer->SetSpawnPoint( spawnPoint );
	} else {
		sdReliableClientMessage msg( GAME_RELIABLE_CMESSAGE_SETSPAWNPOINT );
		msg.WriteLong( gameLocal.GetSpawnId( spawnPoint ) );
		msg.Send();
	}
}

/*
============
sdUserInterfaceLocal::Script_HighlightSpawnPoint
============
*/
void sdUserInterfaceLocal::Script_HighlightSpawnPoint( sdUIFunctionStack& stack ) {
	idStr name;
	stack.Pop( name );

	idStr teamName;
	stack.Pop( teamName );

	bool setHighlight;
	stack.Pop( setHighlight );

	idPlayer* localPlayer = gameLocal.GetLocalPlayer();
	if( localPlayer == NULL ) {
		return;
	}

	sdTeamInfo* info = sdTeamManager::GetInstance().GetTeamSafe( teamName.c_str() );
	if( info == NULL ) {
		return;
	}

	idEntity* spawnPoint = NULL;
	
	// reset to default spawn
	if( name.IsEmpty() ) {
		spawnPoint = info->GetDefaultSpawn();
		if( spawnPoint == NULL ) {
			return;
		}
	} else {
		spawnPoint = gameLocal.FindEntity( name.c_str() );
		if( spawnPoint == NULL ) {
			gameLocal.Warning( "Script_HighlightSpawnPoint: could not find '%s'", name.c_str() );
			return;
		}
	}

	idScriptObject* obj = spawnPoint->GetScriptObject();
	if ( obj != NULL ) {
		sdScriptHelper h1;
		h1.Push( setHighlight );
		obj->CallNonBlockingScriptEvent( obj->GetFunction( "OnHighlight" ), h1 );
	}
}

/*
============
sdUserInterfaceLocal::Script_MutePlayer
============
*/
void sdUserInterfaceLocal::Script_MutePlayer( sdUIFunctionStack& stack ) {
	idStr playerName;
	bool mute;
	stack.Pop( playerName );
	stack.Pop( mute );

	idPlayer* player = gameLocal.GetClientByName( playerName );
	if( player == NULL ) {
		assert( 0 );
		return;
	}

	if( mute ) {
		gameLocal.MutePlayerLocal( gameLocal.GetLocalPlayer(), player->entityNumber );
	} else {
		gameLocal.UnMutePlayerLocal( gameLocal.GetLocalPlayer(), player->entityNumber );
	}
}

/*
============
sdUserInterfaceLocal::Script_MutePlayerQuickChat
============
*/
void sdUserInterfaceLocal::Script_MutePlayerQuickChat( sdUIFunctionStack& stack ) {
	idStr playerName;
	bool mute;
	stack.Pop( playerName );
	stack.Pop( mute );

	idPlayer* player = gameLocal.GetClientByName( playerName );
	if( player == NULL ) {
		assert( 0 );
		return;
	}

	if( mute ) {
		gameLocal.MutePlayerQuickChatLocal( player->entityNumber );
	} else {
		gameLocal.UnMutePlayerQuickChatLocal( player->entityNumber );
	}
}

/*
============
sdUserInterfaceLocal::Script_SpectateClient
============
*/
void sdUserInterfaceLocal::Script_SpectateClient( sdUIFunctionStack& stack ) {
	int spectateeNum;
	stack.Pop( spectateeNum );

	assert( spectateeNum >= 0 && spectateeNum < MAX_CLIENTS );

	gameLocal.ChangeLocalSpectateClient( spectateeNum );
}

/*
============
sdUserInterfaceLocal::Script_ChatCommand
============
*/
void sdUserInterfaceLocal::Script_ChatCommand( sdUIFunctionStack& stack ) {
	idStr chatCommand;
	idWStr text;

	stack.Pop( chatCommand );
	stack.Pop( text );

	if ( chatCommand.Icmp( "say" ) == 0 ) {
		Cmd_DoSay( text, GAME_RELIABLE_CMESSAGE_CHAT );
	} else if ( chatCommand.Icmp( "sayteam" ) == 0 ) {
		Cmd_DoSay( text, GAME_RELIABLE_CMESSAGE_TEAM_CHAT );
	} else if ( chatCommand.Icmp( "sayfireteam" ) == 0 ) {
		Cmd_DoSay( text, GAME_RELIABLE_CMESSAGE_FIRETEAM_CHAT );
	} else {
		gameLocal.Warning( "sdUserInterfaceLocal::Script_ChatCommand Invalid Chat Command '%s'", chatCommand.c_str() );
	}
}