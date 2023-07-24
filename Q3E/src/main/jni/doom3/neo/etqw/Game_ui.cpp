// Copyright (C) 2007 Id Software, Inc.
//

#include "precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "../framework/Licensee.h"

#include "roles/Inventory.h"
#include "roles/FireTeams.h"
#include "roles/ObjectiveManager.h"
#include "roles/Tasks.h"
#include "structures/TeamManager.h"
#include "rules/GameRules.h"
#include "Player.h"
#include "../framework/BuildVersion.h"
#include "../framework/KeyInput.h"
#include "guis/UserInterfaceLocal.h"
#include "guis/UserInterfaceManagerLocal.h"
#include "guis/UIList.h"
#include "guis/UIRadialMenu.h"
#include "CommandMapInfo.h"
#include "demos/DemoManager.h"
#include "rules/UserGroup.h"
#include "rules/AdminSystem.h"
#include "rules/VoteManager.h"
#include "Atmosphere.h"
#include "misc/Door.h"
#include "vehicles/Transport.h"
#include "vehicles/VehicleWeapon.h"
#include "ScriptEntity.h"
#include "Waypoints/LocationMarker.h"
#include "proficiency/StatsTracker.h"
#include "../framework/async/Demo.h"
#include "../sys/sys_local.h"

#ifndef _XENON
#include "../sdnet/SDNet.h"
#endif

#define ON_UIVALID( handle, thename ) sdUserInterfaceLocal* thename = GetUserInterface( handle ); if ( thename )
#define ON_UIINVALID( handle, thename ) sdUserInterfaceLocal* thename = GetUserInterface( handle ); if ( !thename )

idList< qhandle_t > idGameLocal::taskListHandles;

/*
================
idGameLocal::HandleGuiEvent
================
*/
bool idGameLocal::HandleGuiEvent( const sdSysEvent* event ) {
	if ( networkSystem->IsDedicated() ) {
		return false;
	}

	ON_UIVALID( uiMainMenuHandle, ui ) {
		if ( ui->IsActive() ) {
			return ui->PostEvent( event );
		}
	}

	sdHudModule* module = localPlayerProperties.GetActiveHudModule();
	if ( module && module->HandleGuiEvent( event ) ) {
		return true;
	}

	idPlayer* localPlayer = gameLocal.GetLocalPlayer();
	if ( localPlayer ) {
		if( isServer && event->GetKey() == K_F1 && event->IsKeyDown() && isPaused && localPlayer->IsSinglePlayerToolTipPlaying() ) { 
			cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "unPauseGame\n" ); 
			return true; 
		}

		if ( localPlayer->HandleGuiEvent( event ) ) {
			return true;
		}
	}

	if ( rules ) {
		if ( rules->HandleGuiEvent( event ) ) {
			return true;
		}
	}

	return false;
}

/*
================
idGameLocal::TranslateGuiBind
================
*/
bool idGameLocal::TranslateGuiBind( const idKey& key, sdKeyCommand** cmd ) {
	ON_UIVALID( uiMainMenuHandle, ui ) {
		if ( ui->IsActive() ) {
			if ( ui->Translate( key, cmd ) ) {
				return true;
			}
		}
	}

	sdHudModule* module = gameLocal.localPlayerProperties.GetActiveHudModule();
	if ( module != NULL ) {
		sdUserInterfaceLocal* ui = module->GetGui();
		if ( ui != NULL ) {
			if ( ui->Translate( key, cmd ) ) {
				return true;
			}
		}
	}

	idPlayer* localPlayer = gameLocal.GetLocalPlayer();
	if ( localPlayer ) {
		if ( localPlayer->TranslateGuiBind( key, cmd ) ) {
			return true;
		}
	}

	if ( rules != NULL ) {
		if ( rules->TranslateGuiBind( key, cmd ) ) {
			return true;
		}
	}

	return false;
}

/*
============
idGameLocal::ShowMainMenu
============
*/
void idGameLocal::ShowMainMenu() {
	ON_UIVALID( uiMainMenuHandle, ui ) {
		ui->Activate();
	}
}

/*
============
idGameLocal::HideMainMenu
============
*/
void idGameLocal::HideMainMenu() {
	// ao: never called

	ON_UIVALID( uiMainMenuHandle, ui ) {
		ui->Deactivate( true );
	}
}

/*
============
idGameLocal::IsMainMenuActive
============
*/
bool idGameLocal::IsMainMenuActive() {
	ON_UIVALID( uiMainMenuHandle, ui ) {
		return ui->IsActive();
	}
	return false;
}

/*
============
idGameLocal::DrawMainMenu
============
*/
void idGameLocal::DrawMainMenu() {
	ON_UIINVALID( uiMainMenuHandle, ui ) {
		return;
	}

	if ( gamestate == GAMESTATE_NOMAP ) {
		sdGlobalStatsTracker::GetInstance().UpdateStatsRequests();
	}

	ui->Draw();
}

/*
============
idGameLocal::DrawSystemUI
============
*/
void idGameLocal::DrawSystemUI() {
	ON_UIINVALID( uiSystemUIHandle, ui ) {
		return;
	}

	ui->Draw();
}

/*
============
idGameLocal::DrawLoadScreen
============
*/
void idGameLocal::DrawLoadScreen() {
	ON_UIINVALID( uiLevelLoadHandle, ui ) {
		return;
	}

	ui->Activate();
	ui->Update();
	ui->Draw();
}

/*
============
idGameLocal::DrawPureWaitScreen
============
*/
void idGameLocal::DrawPureWaitScreen() {
	ON_UIINVALID( pureWaitHandle, ui ) {
		return;
	}

	ui->Activate();
	ui->Update();
	ui->Draw();
}

/*
============
idGameLocal::CreateKeyBindingList
============
*/
void idGameLocal::CreateKeyBindingList( sdUIList* list ) {
	assert( list );

	using namespace sdProperties;
	idStr category;
	if( sdProperty* property = list->GetScope().GetProperties().GetProperty( "bindCategory", PT_STRING )) {
		category = *property->value.stringValue;
	}

	sdUIList::ClearItems( list );
	const sdDeclKeyBinding* bind = gameLocal.declKeyBindingType.LocalFind( category, false );
	if( bind == NULL ) {
		return;
	}

	idWStr text;
	for( int i = 0; i < bind->GetKeys().GetNumKeyVals(); i++ ) {
		const idKeyValue* kv = bind->GetKeys().GetKeyVal( i );
		int item = sdUIList::InsertItem( list, va( L"<loc = '%hs'>", kv->GetKey().c_str() ), -1, 0 );

		int numKeys = 0;
		keyInputManager->KeysFromBinding( gameLocal.GetDefaultBindContext(), kv->GetValue(), numKeys, NULL );

		idKey** keys = NULL;
		
		if( numKeys > 0 ) {
			keys = static_cast< idKey** >( _alloca( numKeys * sizeof( idKey* ) ) );
			keyInputManager->KeysFromBinding( gameLocal.GetDefaultBindContext(), kv->GetValue(), numKeys, keys );
		}

		if( numKeys > 0 ) {
			assert( keys != NULL );
			keys[ 0 ]->GetLocalizedText( text );
			sdUIList::CleanUserInput( text );
			sdUIList::SetItemText( list, text.c_str(), item, 1 );
		} else {
			sdUIList::SetItemText( list, L"<loc = 'guis/mainmenu/bindings/unbound'>", item, 1 );
		}
		if( numKeys > 1 ) {
			assert( keys != NULL );
			keys[ 1 ]->GetLocalizedText( text );
			sdUIList::CleanUserInput( text );
			sdUIList::SetItemText( list, text.c_str(), item, 2 );
		} else {
			sdUIList::SetItemText( list, L"<loc = 'guis/mainmenu/bindings/unbound'>", item, 2 );
		}
		sdUIList::SetItemText( list, va( L"%hs", kv->GetValue().c_str() ), item, 3 );
	}
}

/*
============
idGameLocal::CreateCrosshairList
============
*/
void idGameLocal::CreateCrosshairList( sdUIList* list ) {
	assert( list != NULL );

	sdUIList::ClearItems( list );
	
	int num = gameLocal.declStringMapType.Num();
	for( int i = 0; i < num; i++ ) {
		const sdDeclStringMap* decl = gameLocal.declStringMapType.LocalFindByIndex( i, false );
		if( idStr::Icmpn( decl->GetName(), "crosshairs", 10 ) != 0 ) {
			continue;
		}
		// ensure we're parsed
		gameLocal.declStringMapType.LocalFindByIndex( i, true );

		const idDict& dict = decl->GetDict();
		for( int j = 0; j < dict.GetNumKeyVals(); j++ ) {
			const idKeyValue* kv = dict.GetKeyVal( j );
			sdUIList::InsertItem( list, va( L"<material = 'literal: %hs'>\t%hs\t%hs", kv->GetValue().c_str(), decl->GetName(), kv->GetKey().c_str() ), -1, 0 );
		}
	}

}

/*
============
idGameLocal::CreateDemoList
============
*/
void idGameLocal::CreateDemoList( sdUIList* list ) {

	sdUIList::ClearItems( list );
    idFileList* files = fileSystem->ListFiles( "demos", ".ndm" );
	if( files == NULL ) {
		return;
	}

	idStr file;
	idWStr date;
	idStr fullPath;
	sysTime_t time;

	if( files->GetNumFiles() > 0 ) {
		list->BeginBatch();
		list->SetItemGranularity( files->GetNumFiles() );
		for( int i = 0; i < files->GetNumFiles(); i++ ) {
			file = files->GetFile( i );
			if( networkSystem->CanPlayDemo( file.c_str() ) ) {
				unsigned int timeStamp;

				fullPath = files->GetBasePath();
				fullPath += "/" + file;
				fileSystem->ReadFile( fullPath, NULL, &timeStamp );

				sys->SecondsToTime( timeStamp, time );
				sdNetProperties::FormatTimeStamp( time, date );

				file.StripFileExtension();
				int index = sdUIList::InsertItem( list, va( L"%hs\t%ls\t%hs", file.c_str(), date.c_str(), fullPath.c_str() ), -1, 0 );
				list->SetItemDataInt( timeStamp, index, 1, true );
			}
		}
		list->EndBatch();
	}

	fileSystem->FreeFileList( files );
}

/*
============
idGameLocal::CreateVehiclePlayerList
============
*/
void idGameLocal::CreateVehiclePlayerList( sdUIList* list ) {
	idPlayer* localPlayer = gameLocal.GetLocalViewPlayer();
	if ( localPlayer == NULL ) {
		if( g_debugPlayerList.GetInteger() ) {
			sdUIList::ClearItems( list );
			idWStr formatString;
			for( int i = 0; i < g_debugPlayerList.GetInteger(); i++ ) {
				formatString = va( L"<material = \"soldier\" >\tPlayer%i", i );
				sdUIList::InsertItem( list, formatString.c_str(), -1, 0 );
			}
		}
		return;
	}

	sdUIList::ClearItems( list );

	idEntity* proxy = localPlayer->GetProxyEntity();
	if( proxy == NULL ) {
		return;
	}

	if ( sdUsableInterface* iface = proxy->GetUsableInterface() ) {
		const wchar_t* formatString;

		int selIndex = -1;
		idWStr cleanPlayerName;

		for( int i = 0; i < iface->GetNumPositions(); i++ ) {
			const idPlayer* player = iface->GetPlayerAtPosition( i );
			if( player == NULL ) {
				if( g_debugPlayerList.GetInteger() ) {
					formatString = va( L"<material = \"soldier\" >\tPlayer%i", i );
					sdUIList::InsertItem( list, formatString, -1, 0 );
				}
				continue;
			}
			if( const sdDeclPlayerClass* pc = player->GetInventory().GetClass() ) {
				cleanPlayerName = va( L"%hs", player->userInfo.name.c_str() );
				sdUIList::CleanUserInput( cleanPlayerName );
				formatString = va( L"<material = \"%hs\" >\t%ls", ( pc == NULL ) ? "" : pc->GetName(), cleanPlayerName.c_str() );
				int index = sdUIList::InsertItem( list, formatString, -1, 0 );
				if( player == localPlayer ) {
					selIndex = index;
				}
			}
		}
		list->SelectItem( selIndex );
	} 
}

/*
============
idGameLocal::CreateScoreboardList
============
*/
void idGameLocal::CreateScoreboardList( sdUIList* list ) {
	if( gameLocal.rules ) {
		idStr teamName;
		if( sdUserInterfaceLocal* ui = list->GetUI() ) {
			ui->PopScriptVar( teamName );
		}
		gameLocal.rules->UpdateScoreboard( list, teamName );
	} else {
		if( sdUserInterfaceLocal* ui = list->GetUI() ) {
			idStr name;
			ui->PopScriptVar( name );
			ui->PushScriptVar( 0.0f );
			ui->PushScriptVar( 0.0f );
			ui->PushScriptVar( 0.0f );
		}
	}
}

/*
============
idGameLocal::CreatePlayerAdminList
============
*/
void idGameLocal::CreatePlayerAdminList( sdUIList* list ) {
	sdAdminSystem::GetInstance().CreatePlayerAdminList( list );
}

/*
============
idGameLocal::CreateUserGroupList
============
*/
void idGameLocal::CreateUserGroupList( sdUIList* list ) {
	sdUserGroupManager::GetInstance().CreateUserGroupList( list );
}

/*
============
idGameLocal::CreateUserGroupList
============
*/
void idGameLocal::CreateServerConfigList( sdUIList* list ) {
	sdUserGroupManager::GetInstance().CreateServerConfigList( list );
}

/*
============
idGameLocal::CreateFireTeamListEntry
============
*/
void idGameLocal::CreateFireTeamListEntry( sdUIList* list, int index, idPlayer* player ) {
	static const idVec4 noDraw( 0.0f, 0.0f, 0.0f, 0.0f );

	sdUIList::InsertItem( list, L"", index, 0 );

	const sdDeclPlayerClass* cls = player->GetInventory().GetClass();
	if ( cls != NULL ) {
		// class
		sdUIList::SetItemIcon( list, cls->GetName(), index, 0 );
	}

	// rank/voip
	bool sendingVoice = ( sys->Milliseconds() - networkSystem->GetLastVoiceReceivedTime( player->entityNumber ) ) < SEC2MS( 0.5f );
	if ( sendingVoice ) {
		sdUIList::SetItemIcon( list, "voip", index, 1 );
		sdUIList::SetItemForeColor( list, colorWhite, index, 1 );
	}  else if ( const sdDeclRank* rank = player->GetProficiencyTable().GetRank() ) {
		sdUIList::SetItemIcon( list, rank->GetMaterial(), index, 1 );
		sdUIList::SetItemForeColor( list, colorWhite, index, 1 );
	} else {
		sdUIList::SetItemForeColor( list, noDraw, index, 1 );
	}

	// name

	idWStr cleanPlayerName = va( L"%hs", player->userInfo.cleanName.c_str() );
	sdUIList::CleanUserInput( cleanPlayerName );

	sdUIList::SetItemText( list, cleanPlayerName.c_str(), index, 2 );

	if( player->IsInLimbo() ) {
		sdUIList::SetItemForeColor( list, colorBlack, index, 2 );
	} else {
		float healthFrac = player->GetHealth() / ( float )player->GetMaxHealth();
		healthFrac = idMath::ClampFloat( 0.0f, 1.0f, healthFrac );
		idVec4 healthColor( 1.0f - healthFrac, healthFrac, 0.0f, 1.0f );
		sdUIList::SetItemForeColor( list, healthColor, index, 2 );
	}	
}

/*
============
idGameLocal::CreateFireTeamList
============
*/
void idGameLocal::CreateFireTeamList( sdUIList* list ) {
	sdUIList::ClearItems( list );

	int listType;
	if( sdUserInterfaceLocal* ui = list->GetUI() ) {
		ui->PopScriptVar( listType );
	}

	if ( gameLocal.rules == NULL ) {
		return;
	}

	if ( gameLocal.GetLocalPlayer() == NULL ) {
		return;
	}

	if ( listType == FIRETEAMLIST_MYFIRETEAM ) {
		CreateFireTeamList_MyFireTeam( list );
	} else if ( listType == FIRETEAMLIST_MAIN ) {
		CreateFireTeamList_Main( list );
	} else if ( listType == FIRETEAMLIST_JOIN ) {
		CreateFireTeamList_Join( list );
	} else if ( listType == FIRETEAMLIST_INVITE ) {
		CreateFireTeamList_Invite( list );
	} else if ( listType == FIRETEAMLIST_KICK ) {
		CreateFireTeamList_Kick( list );
	} else if ( listType == FIRETEAMLIST_PROMOTE ) {
		CreateFireTeamList_Promote( list );
	} else if ( listType == FIRETEAMLIST_MANAGE ) {
		CreateFireTeamList_Manage( list );
	} else {
		assert( 0 );
	}
}

/*
============
idGameLocal::CreateFireTeamList_MyFireTeam
============
*/
void idGameLocal::CreateFireTeamList_MyFireTeam( sdUIList* list ) {
	idPlayer* localPlayer = gameLocal.GetLocalPlayer();
	sdFireTeam* fireTeam = gameLocal.rules->GetPlayerFireTeam( localPlayer->entityNumber );
	if ( fireTeam != NULL ) {
		int index = 0;
		for ( int i = 0; i < fireTeam->GetNumMembers(); i++, index++ ) {
			idPlayer* player = fireTeam->GetMember( i );
			CreateFireTeamListEntry( list, index, player );
		}
		if ( g_debugPlayerList.GetInteger() ) {
			int i = 0;

			int num = 8;
			while ( list->GetNumItems() < num ) {
				idWStr formatString = va( L"<material = \"soldier\" >\t<material = \"guis/assets/icons/rank01\" >\tPlayer%i\t<fore = 1,1,1,1>100.00XP\t100%%", i );
				sdUIList::InsertItem( list, formatString.c_str(), -1, 0 );
				i++;
			}
		}
	} else {
		sdTeamInfo* localTeam = localPlayer->GetGameTeam();
		if ( localTeam == NULL ) {
			return;
		}

		idStaticList< idPlayer*, MAX_CLIENTS > playerList;

		// Mission teams
		for ( int i = 0; i < MAX_CLIENTS; i++ ) {
			idPlayer* player = gameLocal.GetClient( i );
			if ( player == NULL ) {
				continue;
			}

			if ( player->GetGameTeam() != localTeam ) {
				continue;
			}

			if ( gameLocal.rules->GetPlayerFireTeam( i ) != NULL ) {
				continue;
			}

			if( player->GetActiveTaskHandle() != localPlayer->GetActiveTaskHandle() ) {
				continue;
			}

			playerList.Append( player );
		}

		for ( int i = 0; i < playerList.Num() && i < MAX_MISSION_TEAM_PLAYERS; i++ ) {
			idPlayer* player = playerList[ i ];
			CreateFireTeamListEntry( list, i, player );
		}
	}
}

/*
============
idGameLocal::CreateFireTeamList_Main
============
*/
void idGameLocal::CreateFireTeamList_Main( sdUIList* list ) {
	idPlayer* localPlayer = gameLocal.GetLocalPlayer();

	bool canJoin = false;
	bool canCreate = false;
	for ( int i = 0; i < sdTeamInfo::MAX_FIRETEAMS; i++ ) {
		sdFireTeam& fireTeam = localPlayer->GetTeam()->GetFireTeam( i );
		if ( fireTeam.GetNumMembers() > 0 ) {
			if ( !fireTeam.IsPrivate() ) {
				if ( fireTeam.MaxMembers() - fireTeam.GetNumMembers() > 0 ) {
					canJoin = true;
				}
			}
		} else {
			canCreate = true;
		}
	}

	if ( canJoin ) {
		sdUIList::InsertItem( list, va( L" 1. %ls\tcanjoin", common->LocalizeText( "guis/hud/fireteam/menu/join" ).c_str() ), 0, 0 );
	} else {
		sdUIList::InsertItem( list, va( L" 1. ^1%ls\tcannot", common->LocalizeText( "guis/hud/fireteam/menu/join" ).c_str() ), 0, 0 );
	}

	if ( canCreate ) {
		sdUIList::InsertItem( list, va( L" 2. %ls\tcancreate", common->LocalizeText( "guis/hud/fireteam/menu/create" ).c_str() ), 1, 0 );
	} else {
		sdUIList::InsertItem( list, va( L" 2. ^1%ls\tcannot", common->LocalizeText( "guis/hud/fireteam/menu/create" ).c_str() ), 1, 0 );
	}
}

/*
============
idGameLocal::CreateFireTeamList_Join
============
*/
void idGameLocal::CreateFireTeamList_Join( sdUIList* list ) {
	idPlayer* localPlayer = gameLocal.GetLocalPlayer();

	int num = 0;
	for ( int i = 0; i < sdTeamInfo::MAX_FIRETEAMS; i++ ) {
		sdFireTeam& fireTeam = localPlayer->GetTeam()->GetFireTeam( i );
		if ( !fireTeam.IsPrivate() ) {
			if ( fireTeam.GetNumMembers() > 0 && fireTeam.MaxMembers() - fireTeam.GetNumMembers() > 0 ) {
				sdUIList::InsertItem( list, va( L" %i. %hs\t%i", num+1, fireTeam.GetName(), i ), i, 0 );
				num++;
			}
		}
	}
}

/*
============
idGameLocal::CreateFireTeamList_Invite
============
*/
void idGameLocal::CreateFireTeamList_Invite( sdUIList* list ) {
	idPlayer* localPlayer = gameLocal.GetLocalPlayer();
	sdFireTeam* fireTeam = gameLocal.rules->GetPlayerFireTeam( localPlayer->entityNumber );

	if ( fireTeam == NULL ) {
		return;
	}

	int page;
	if( sdUserInterfaceLocal* ui = list->GetUI() ) {
		ui->PopScriptVar( page );
	}

	assert( page >= 0 && page <= 1 );

	int numAdded = 0;
	int numFound = 0;
	if ( fireTeam->MaxMembers() - fireTeam->GetNumMembers() >= 1 ) {
		for ( int i = 0; i < MAX_CLIENTS; i++ ) {
			idPlayer *p = gameLocal.GetClient( i );
			if ( p != NULL && p->GetTeam() == localPlayer->GetTeam() ) {
				if ( gameLocal.rules->GetPlayerFireTeam( i ) == NULL ) {
					numFound++;
					if ( numFound > FIRETEAMLIST_PAGE_NUM_PLAYERS * page && numAdded < FIRETEAMLIST_PAGE_NUM_PLAYERS ) {
						sdUIList::InsertItem( list, va( L" %i. %hs\t%hs", numAdded+1, p->userInfo.cleanName.c_str(), p->userInfo.cleanName.c_str() ), numAdded, 0 );
						numAdded++;
					}
				}
			}
		}
	}

	if ( numFound != numAdded ) {
		if ( page == 1 ) {
			sdUIList::InsertItem( list, va( L" %i. %ls\tprevpage", numAdded+1, common->LocalizeText( "guis/hud/fireteam/menu/prevpage" ).c_str() ), numAdded, 0 );
		} else {
			sdUIList::InsertItem( list, va( L" %i. %ls\tnextpage", numAdded+1, common->LocalizeText( "guis/hud/fireteam/menu/nextpage" ).c_str() ), numAdded, 0 );
		}
		numAdded++;
	}
}

/*
============
idGameLocal::CreateFireTeamList_Kick
============
*/
void idGameLocal::CreateFireTeamList_Kick( sdUIList* list ) {
	idPlayer* localPlayer = gameLocal.GetLocalPlayer();
	sdFireTeam* fireTeam = gameLocal.rules->GetPlayerFireTeam( localPlayer->entityNumber );

	if ( fireTeam == NULL ) {
		return;
	}

	int numAdded = 0;
	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		idPlayer* p = gameLocal.GetClient( i );
		if ( p != NULL && p != localPlayer && fireTeam->IsMember( p ) ) {
			sdUIList::InsertItem( list, va( L" %i. %hs\t%hs", numAdded+1, p->userInfo.cleanName.c_str(), p->userInfo.cleanName.c_str() ), numAdded, 0 );
			numAdded++;
		}
	}
}

/*
============
idGameLocal::CreateFireTeamList_Promote
============
*/
void idGameLocal::CreateFireTeamList_Promote( sdUIList* list ) {
	idPlayer* localPlayer = gameLocal.GetLocalPlayer();
	sdFireTeam* fireTeam = gameLocal.rules->GetPlayerFireTeam( localPlayer->entityNumber );

	if ( fireTeam == NULL ) {
		return;
	}

	int numAdded = 0;
	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		idPlayer* p = gameLocal.GetClient( i );
		if ( p != NULL && p != localPlayer && fireTeam->IsMember( p ) ) {
			if ( !p->userInfo.isBot ) {
				sdUIList::InsertItem( list, va( L" %i. %hs\t%hs", numAdded+1, p->userInfo.cleanName.c_str(), p->userInfo.cleanName.c_str() ), numAdded, 0 );
				numAdded++;
			}
		}
	}
}

/*
============
idGameLocal::CreateFireTeamList_Manage
============
*/
void idGameLocal::CreateFireTeamList_Manage( sdUIList* list ) {
	idPlayer* localPlayer = gameLocal.GetLocalPlayer();
	sdFireTeam* fireTeam = gameLocal.rules->GetPlayerFireTeam( localPlayer->entityNumber );

	if ( fireTeam == NULL ) {
		return;
	}

	int index = 0;
	
	if ( fireTeam->IsCommander( localPlayer ) ) {
		bool canInvite = false;
		if ( fireTeam->MaxMembers() - fireTeam->GetNumMembers() > 0 ) {
			for ( int i = 0; i < MAX_CLIENTS; i++ ) {
				idPlayer *p = gameLocal.GetClient( i );
				if ( p != NULL && p->GetTeam() == localPlayer->GetTeam() ) {
					if ( gameLocal.rules->GetPlayerFireTeam( i ) == NULL ) {
						canInvite = true;
						break;
					}
				}
			}
		}

		if ( canInvite ) {
			sdUIList::InsertItem( list, va( L" %i. %ls\tcaninvite", index+1, common->LocalizeText( "guis/hud/fireteam/menu/invite" ).c_str() ), index, 0 );
		} else {
			sdUIList::InsertItem( list, va( L" %i. ^1%ls\tcannot", index+1, common->LocalizeText( "guis/hud/fireteam/menu/invite" ).c_str() ), index, 0 );
		}
		index++;

		if ( fireTeam->GetNumMembers() > 1 ) {
			sdUIList::InsertItem( list, va( L" %i. %ls\tcankick", index+1, common->LocalizeText( "guis/hud/fireteam/menu/kick" ).c_str() ), index, 0 );
		} else {
			sdUIList::InsertItem( list, va( L" %i. ^1%ls\tcannot", index+1, common->LocalizeText( "guis/hud/fireteam/menu/kick" ).c_str() ), index, 0 );
		}
		index++;

		sdUIList::InsertItem( list, va( L" %i. %ls\tdisband", index+1, common->LocalizeText( "guis/hud/fireteam/menu/disband" ).c_str() ), index, 0 );
		index++;

		sdUIList::InsertItem( list, va( L" %i. %ls\tleave", index+1, common->LocalizeText( "guis/hud/fireteam/menu/leave" ).c_str() ), index, 0 );
		index++;


		int numPromote = 0;
		for ( int i = 0; i < MAX_CLIENTS; i++ ) {
			idPlayer* p = gameLocal.GetClient( i );
			if ( p != NULL && p != localPlayer && fireTeam->IsMember( p ) ) {
				if ( !p->userInfo.isBot ) {
					numPromote++;
				}
			}
		}
		if ( numPromote > 0 ) {
			sdUIList::InsertItem( list, va( L" %i. %ls\tcanpromote", index+1, common->LocalizeText( "guis/hud/fireteam/menu/promote" ).c_str() ), index, 0 );
		} else {
			sdUIList::InsertItem( list, va( L" %i. ^1%ls\tcannot", index+1, common->LocalizeText( "guis/hud/fireteam/menu/promote" ).c_str() ), index, 0 );
		}
		index++;

		if ( fireTeam->IsPrivate() ) {
			sdUIList::InsertItem( list, va( L" %i. %ls\tpublic", index+1, common->LocalizeText( "guis/hud/fireteam/menu/makepublic" ).c_str() ), index, 0 );
		} else {
			sdUIList::InsertItem( list, va( L" %i. %ls\tprivate", index+1, common->LocalizeText( "guis/hud/fireteam/menu/makeprivate" ).c_str() ), index, 0 );
		}
		index++;

		sdUIList::InsertItem( list, va( L" %i. %ls\trename", index+1, common->LocalizeText( "guis/hud/fireteam/menu/rename" ).c_str() ), index, 0 );
		index++;
	} else {
		bool canPropose = false;
		if ( fireTeam->MaxMembers() - fireTeam->GetNumMembers() >= 1 ) {
			for ( int i = 0; i < MAX_CLIENTS; i++ ) {
				idPlayer *p = gameLocal.GetClient( i );
				if ( p != NULL && p->GetTeam() == localPlayer->GetTeam() ) {
					if ( gameLocal.rules->GetPlayerFireTeam( i ) == NULL ) {
						canPropose = true;
					}
				}
			}
		}

		if ( canPropose ) {
			sdUIList::InsertItem( list, va( L" %i. %ls\tcanpropose", index+1, common->LocalizeText( "guis/hud/fireteam/menu/propose" ).c_str() ), index, 0 );
		} else {
			sdUIList::InsertItem( list, va( L" %i. ^1%ls\tcannot", index+1, common->LocalizeText( "guis/hud/fireteam/menu/propose" ).c_str() ), index, 0 );
		}
		index++;

		sdUIList::InsertItem( list, va( L" %i. %ls\tleave", index+1, common->LocalizeText( "guis/hud/fireteam/menu/leave" ).c_str() ), index, 0 );
		index++;
	}
}

/*
============
idGameLocal::GeneratePlayerListForTask
============
*/
void idGameLocal::GeneratePlayerListForTask( idStr& playerList, const sdPlayerTask* task ) {
	sdStringBuilder_Heap builder;
	playerList.Clear();

	sdTaskManagerLocal& manager = sdTaskManager::GetInstance();

	for( int playerIndex = 0; playerIndex < gameLocal.numClients; playerIndex++ ) {
		idPlayer* player = gameLocal.GetClient( playerIndex );
		if ( player == NULL ) {
			continue;
		}
		if ( player->GetActiveTaskHandle() != task->GetHandle() ) {
			continue;
		}

		builder += " " + player->userInfo.name;
	}
	builder.ToString( playerList );
}


/*
============
idGameLocal::InsertTask
============
*/
int idGameLocal::InsertTask( sdUIList* list, const sdPlayerTask* task, bool highlightActive ) {	
	const char* iconMat = "guis/nodraw";
	if ( task->GetInfo()->GetNumWayPoints() > 0 ) {
		const idMaterial* mat = task->GetInfo()->GetWaypointIcon( 0 );
		if( mat != NULL ) {
			iconMat = mat->GetName();
		}
	}

	idWStr xpString = task->GetInfo()->GetXPString();
	sdUIList::CleanUserInput( xpString );

	int index = sdUIList::InsertItem( list, va( L"<material = \"::%hs\">\t%ls\t<align = right>%ls", 
										iconMat, task->GetTitle(), xpString.c_str() ), -1, 0 );
	
	return index;
}

/*
============
idGameLocal::CreateActiveTaskList
============
*/
void idGameLocal::CreateActiveTaskList( sdUIList* list ) {
	sdTaskManagerLocal& manager = sdTaskManager::GetInstance();

	sdUIList::ClearItems( list );
	bool showAll;
	list->GetUI()->PopScriptVar( showAll );

	idPlayer* localPlayer = gameLocal.GetLocalPlayer();
	idPlayer* localPlayerView = gameLocal.GetLocalViewPlayer();
	if ( localPlayerView == NULL ) {
		return;
	}

	if ( g_debugPlayerList.GetInteger() ) {
		sdUIList::InsertItem( list, L"Task 1", 0, 1 );
		idWStrList parms( 1 );
		keyInputManager->KeysFromBinding( gameLocal.GetDefaultBindContext(), "_taskmenu", true, parms.Alloc() );
		sdUIList::InsertItem( list, va( L"<fore = 0 1 0 1>\t%ls", common->LocalizeText( "guis/hud/tasks_are_available", parms ).c_str() ), -1, 1 );
		return;
	}

	sdTeamInfo* team = localPlayerView->GetGameTeam();
	if ( team == NULL ) {
		return;
	}

	bool canSelect = true;
	sdPlayerTask* activeTask = NULL;

	sdFireTeam* fireTeam = gameLocal.rules->GetPlayerFireTeam( localPlayerView->entityNumber );
	if ( fireTeam != NULL ) {
		activeTask = fireTeam->GetCommander()->GetActiveTask();
		canSelect = fireTeam->GetCommander() == localPlayer;
	} else {
		activeTask = localPlayerView->GetActiveTask();
	}	

	const sdPlayerTask::nodeType_t& objectiveTasks = sdTaskManager::GetInstance().GetObjectiveTasks( team );
	sdPlayerTask* objectiveTask = objectiveTasks.Next();
	if ( ( showAll || activeTask == NULL ) && objectiveTask != NULL ) {
		int index = InsertTask( list, objectiveTask, false );
		if( activeTask == NULL ) {
			list->SelectItem( index );
		}
	}

	gameLocal.localPlayerProperties.SetHasTask( !showAll && canSelect && ( activeTask != NULL || ( activeTask == NULL && objectiveTask != NULL ) ) );

	if( showAll && canSelect && localPlayerView == localPlayer ) {
		playerTaskList_t miniList;
		manager.BuildTaskList( localPlayer, miniList );
		int num = Min( sdPlayerTask::MAX_CHOOSABLE_TASKS, miniList.Num() );

		for( int i = 0; i < num; i++ ) {			
			int index = InsertTask( list, miniList[ i ], false );
			if( activeTask == miniList[ i ] ){
				list->SelectItem( index );
			}
		}
		/*
		int index = sdUIList::InsertItem( list, common->LocalizeText( "guis/game/scoreboard/nomission" ).c_str(), -1, 1 );
		if( activeTask == NULL ) {
			list->SelectItem( index );
		}
		*/
		idWStrList args( 1 );
		keyInputManager->KeysFromBinding( gameLocal.GetDefaultBindContext(), "_taskmenu", true, args.Alloc() );
		
		const idWStr localized = common->LocalizeText( "guis/hud/tasks_are_available", args );
		sdUIList::InsertItem( list, localized.c_str(), -1, 1 );

		return;
	}

	if ( activeTask != NULL ) {
		int index = InsertTask( list, activeTask, false );
		list->SelectItem( index );
	}

	if ( localPlayerView != localPlayer ) {
		return;
	}

	playerTaskList_t miniList;
	manager.BuildTaskList( localPlayer, miniList );

	int numAvailableTasks = miniList.Num();

	if( objectiveTask != NULL ) {
		numAvailableTasks++;
	}

	if ( numAvailableTasks > 1 && canSelect ) {
		idWStrList args( 1 );
		keyInputManager->KeysFromBinding( gameLocal.GetDefaultBindContext(), "_taskmenu", true, args.Alloc() );

		const idWStr localized = common->LocalizeText( "guis/hud/tasks_are_available", args );
		sdUIList::InsertItem( list, localized.c_str(), -1, 1 );
	}

	if ( list->GetNumItems() == 0 ) {
		const idWStr localized = common->LocalizeText( "guis/hud/no_tasks" );
		sdUIList::InsertItem( list, localized.c_str(), -1, 1 );
	}
}

/*
============
idGameLocal::GetTaskListHandle
============
*/
qhandle_t idGameLocal::GetTaskListHandle( int index ) {
	if ( index < 0 || index >= taskListHandles.Num() ) {
		return -1;
	}

	return taskListHandles[ index ];
}

/*
============
idGameLocal::TestGUI_f
============
*/
void idGameLocal::TestGUI_f( const idCmdArgs& args ) {
	if( args.Argc() < 2 || idStr::Length( args.Argv( 1 )) == 0 ) {
		gameLocal.uiMainMenuHandle	= gameLocal.LoadUserInterface( "mainmenu",	false, true );
	} else {
		gameLocal.uiMainMenuHandle	= gameLocal.LoadUserInterface( args.Argv( 1 ),	false, true );			
		// jrad - something of a hack:
		// ensure that we're pushing the proper times through to the gui
		sdUserInterfaceLocal* ui =gameLocal.GetUserInterface( gameLocal.uiMainMenuHandle );
		if( ui != NULL ) {
			ui->SetGUIFlag( sdUserInterfaceLocal::GUI_FRONTEND );
		}
	}
	gameLocal.ShowMainMenu();
}

/*
================
idGameLocal::LoadUserInterface
================
*/
guiHandle_t idGameLocal::LoadUserInterface( const char* name, bool requireUnique, bool permanent, const char* theme, sdHudModule* module ) {
	return uiManager->AllocUI( name, requireUnique, permanent, theme, module );
}

/*
================
idGameLocal::FreeUserInterface
================
*/
void idGameLocal::FreeUserInterface( guiHandle_t handle ) {
	uiManager->FreeUserInterface( handle );
}

/*
================
idGameLocal::GetUserInterface
================
*/
sdUserInterfaceLocal* idGameLocal::GetUserInterface( const guiHandle_t handle ) {
	return static_cast< sdUserInterfaceLocal* >( uiManager->GetUserInterface( handle ) );
}


/*
============
idGameLocal::GetUserInterfaceWindow
============
*/
sdUIWindow* idGameLocal::GetUserInterfaceWindow( const guiHandle_t handle, const char* windowName ) {
	if( handle.IsValid() ) {
		sdUserInterfaceLocal* ui = GetUserInterface( handle );
		if( ui ) {
			return ui->GetWindow( windowName )->Cast< sdUIWindow >();
		}
	}
	return NULL;
}

/*
============
idGameLocal::GetUserInterfaceProperty
============
*/
sdProperties::sdProperty*	idGameLocal::GetUserInterfaceProperty( guiHandle_t handle, const char* propertyName, sdProperties::ePropertyType expectedType ) {
	if( sdUserInterfaceLocal* ui = GetUserInterface( handle ) ) {
		return ui->GetState().GetProperty( propertyName, expectedType );
	}
	return NULL;
}

/*
============
idGameLocal::GetUserInterfaceProperty_r
============
*/
sdProperties::sdProperty*	idGameLocal::GetUserInterfaceProperty_r( guiHandle_t handle, const char* propertyName, sdProperties::ePropertyType expectedType ) {
	sdProperties::sdProperty* property = GetUserInterfaceProperty( handle, propertyName, expectedType );
	if( property ) {
		return property;
	}

	if( sdUserInterfaceLocal* ui = GetUserInterface( handle ) ) {
		idLexer parser( propertyName, idStr::Length( propertyName ), "GetUserInterfaceProperty_r", sdDeclGUI::LEXER_FLAGS | LEXFL_NOERRORS );
				
		sdUserInterfaceScope* scope = GetUserInterfaceScope( ui->GetState(), &parser );
		if( scope ) {
			idToken token;
			if( parser.ReadToken( &token )) {
				return scope->GetProperty( token, expectedType );
			}			
		}
	}
	return NULL;
}

/*
================
idGameLocal::GetUserInterfaceScope
================
*/
sdUserInterfaceScope* idGameLocal::GetGlobalUserInterfaceScope( sdUserInterfaceScope& scope, const char* name ) {
	for( int i = 0; i < uiScopes.Num(); i++ ) {
		guiScope_t& scope = uiScopes[ i ];
		if( idStr::Icmp( name, scope.name ) == 0 ) {
			return scope.scope;
		}
	}

	return NULL;
}

/*
================
idGameLocal::GetUserInterfaceScope
================
*/
sdUserInterfaceScope* idGameLocal::GetUserInterfaceScope( sdUserInterfaceScope& scope, idLexer* src ) {
	idToken dot;
	idToken token;

	if ( !src->ReadToken( &token ) ) {
		return &scope;
	}

	if ( token.type != TT_NAME ) {
		src->UnreadToken( &token );
		return &scope;
	}

	sdUserInterfaceScope* altScope = &scope;

	sdUserInterfaceScope* globalScope = GetGlobalUserInterfaceScope( scope, token );
	if ( globalScope ) {
		if ( !src->ReadToken( &dot ) || dot != "." ) {
			src->UnreadToken( &dot );
			src->UnreadToken( &token );
			return &scope;
		}
		src->ReadToken( &token );
		altScope = globalScope;
	}

	while( true ) {
		sdUserInterfaceScope* temp;
		if ( !token.Cmp( "self" ) ) {
			temp = altScope;
		} else {
			temp = altScope->GetSubScope( token );
		}

		if ( !temp || !src->ReadToken( &dot ) ) {
			src->UnreadToken( &token );
			return altScope;
		}

		if ( dot != "." ) {
			src->UnreadToken( &dot );
			src->UnreadToken( &token );
			return altScope;
		}

		src->ReadToken( &token );
		altScope = temp;

		if ( token.type != TT_NAME ) {
			return altScope;
		}
	}
}

/*
================
sdUIScopeParser::sdUIScopeParser
================
*/
sdUIScopeParser::sdUIScopeParser( const char* text ) {
	idStr::Copynz( entryText, text, sizeof( entryText ) );

	int len = idStr::Length( entryText );
	( *entries.Alloc() ) = entryText;
	int index = 0;
	while ( ( index = idStr::FindChar( entryText, '.', index, len - 1 ) ) != idStr::INVALID_POSITION ) {
		entryText[ index ] = '\0';
		index++;
		const char** entry = entries.Alloc();
		if ( entry == NULL ) {
			gameLocal.Error( "sdUIScopeParser: Max Entries Hit" );
		}
		*entry = &entryText[ index ];
	}

	entryIndex = 0;
}

/*
================
idGameLocal::GetUserInterfaceScope
================
*/
sdUserInterfaceScope* idGameLocal::GetUserInterfaceScope( sdUserInterfaceScope& scope, sdUIScopeParser& src ) {
	sdUserInterfaceScope* altScope = &scope;

	const char* token = src.GetNextEntry();
	if ( src.IsLastEntry() ) {
		src.Revert();
		return altScope;
	}

	sdUserInterfaceScope* globalScope = GetGlobalUserInterfaceScope( scope, token );
	if ( globalScope != NULL ) {
		altScope = globalScope;
		token = src.GetNextEntry();
	}

	while ( true ) {
		if ( src.IsLastEntry() ) {
			src.Revert();
			return altScope;
		}

		sdUserInterfaceScope* temp = altScope->GetSubScope( token );
		if ( temp == NULL ) {
			gameLocal.Warning( "idGameLocal::GetUserInterfaceScope Failed To Find Scope: '%s'", token );
			return altScope;
		}

		altScope = temp;
		token = src.GetNextEntry();
	}
}

/*
============
idGameLocal::GuiFrameEvents
============
*/
void idGameLocal::GuiFrameEvents( bool outOfSequence ) {
	uiManager->Update( outOfSequence );
}

/*
============
idGameLocal::CreateInventoryList
============
*/
void idGameLocal::CreateInventoryList( sdUIList* list ) {
	sdUserInterfaceLocal* ui = list->GetUI();
		
	assert( ui );

	idStr classname;
	int bank;
	int package;

	ui->PopScriptVar( classname );
	ui->PopScriptVar( bank );
	ui->PopScriptVar( package );
	

	idPlayer* player = gameLocal.GetLocalPlayer();
	idList< itemBankPair_t > items;
	ui->EnumerateItemsForBank( items, classname, bank, package );

	for( int i = 0; i < items.Num(); i++ ) {
		const wchar_t* formatString = va( L"<material = '::%hs'>\t<align = top><loc = '%hs'>", items[ i ].first->GetData().GetString( "mtr_weaponmenu", "default" ), items[ i ].first->GetItemName()->GetName() );
		int index = sdUIList::InsertItem( list, formatString, -1, 0 );
		list->SetItemDataInt( items[ i ].second, index, 0 );
	}
}

typedef sdPair< int, const vidmode_t* > modePair_t;
struct sortVidMode_t {
	int operator()( const modePair_t& a, const modePair_t& b ) {
		if( a.second->width == b.second->width ) {
			return a.second->height - b.second->height;
		}
		return a.second->width - b.second->width;
		return 0;
	}
};

/*
============
idGameLocal::CreateVideoModeList
============
*/
void idGameLocal::CreateVideoModeList( sdUIList* list ) {
	sdUIList::ClearItems( list );

	bool fullScreen;
	list->GetUI()->PopScriptVar( fullScreen );

	int aspect;
	list->GetUI()->PopScriptVar( aspect );

	int currentMode = cvarSystem->GetCVarInteger( "r_mode" );
	int startMode = 3;
	int selIndex = -1;

	static const char* ratios[ 4 ] = {
		"(4:3)",
		"(16:9)",
		"(16:10)",
		"(5:4)"
	};
	
	idList< modePair_t > modes;

	for( int i = startMode; i < common->GetNumVideoModes(); i++ ) {
		const vidmode_t& mode = common->GetVideoMode( i );
		// don't show modes that the monitor says it doesn't support
		if ( fullScreen && !mode.available ) {
			continue;
		}
		if ( aspect != -1 && aspect != mode.aspectRatio ) {
			continue;
		}
		modes.Append( modePair_t( i, &mode ) );
	}

	sdQuickSort( modes.Begin(), modes.End(), sortVidMode_t() );


	int nextBestModeWidth = -1;
	int nextBestMode = -1;

	const int currentWidth = currentMode == -1 ? cvarSystem->GetCVarInteger( "r_customWidth" ) : common->GetVideoMode( currentMode ).width;

	for( int i = 0; i < modes.Num(); i++ ) {
		const vidmode_t& mode = *modes[ i ].second;
		int index = sdUIList::InsertItem( list, va( L"%ix%i %hs\t%i", mode.width, mode.height, ratios[ mode.aspectRatio ], modes[ i ].first ), -1, 0 );
		if( modes[ i ].first == currentMode ) {
			selIndex = index;
		}
		if( modes[ i ].second->width < currentWidth && modes[ i ].second->width > nextBestModeWidth ) {
			nextBestMode = index;
			nextBestModeWidth = modes[ i ].second->width;
		}
	}

	if( selIndex != -1 ) {
		list->SelectItem( selIndex );
	} else if( nextBestMode != -1 ) {
		list->SelectItem( nextBestMode );
	}
}

/*
============
idGameLocal::CreateCampaignList
============
*/
void idGameLocal::CreateCampaignList( sdUIList* list ) {
	sdUIList::ClearItems( list );
	for( int i = 0; i < gameLocal.campaignMetaDataList->GetNumMetaData(); i++ ) {
		const metaDataContext_t& metaDataContext = gameLocal.campaignMetaDataList->GetMetaDataContext( i );
		if ( !gameLocal.IsMetaDataValidForPlay( metaDataContext, true ) ) {
			continue;
		}
		const idDict& dict = *metaDataContext.meta;

		const char* materialName = dict.GetString( "server_shot_thumb" );
		const char* prettyName = dict.GetString( "pretty_name" );
		const char* defName = dict.GetString( "metadata_name" );
		int index = sdUIList::InsertItem( list, va( L"%hs\t%hs\t%hs", prettyName, defName, materialName ), -1, 0 );
	}
}

/*
============
idGameLocal::CreateMapList
============
*/
void idGameLocal::CreateMapList( sdUIList* list ) {
	sdUIList::ClearItems( list );

	sdAddonMetaDataList* metaData = fileSystem->ListAddonMetaData( "mapMetaData" );
	for( int i = 0; i < metaData->GetNumMetaData(); i++ ) {
		const metaDataContext_t& metaDataContext = metaData->GetMetaDataContext( i );
		if ( !gameLocal.IsMetaDataValidForPlay( metaDataContext, true ) ) {
			continue;
		}
		const idDict& data = *metaDataContext.meta;

		const char* materialName = data.GetString( "server_shot_thumb", "levelshots/thumbs/generic.tga" );
		const char* defName = data.GetString( "metadata_name" );
		const char* prettyName = data.GetString( "pretty_name" );
		int index = sdUIList::InsertItem( list, va( L"%hs\t%hs\t%hs", prettyName, defName, materialName ), -1, 0 );
	}

	fileSystem->FreeAddonMetaDataList( metaData );
}

/*
============
idGameLocal::PacifierUpdate
============
*/
void idGameLocal::PacifierUpdate() {
	gameLocal.localPlayerProperties.PacifierUpdate();

	deviceContext->BeginEmitFullScreen();

	if ( sdUserInterfaceLocal* ui = GetUserInterface( uiLevelLoadHandle ) ) {
		ui->SetCurrentTime( sys->Milliseconds() );
		ui->Update();
		ui->Draw();
	}

	deviceContext->End();
}

/*
============
idGameLocal::ShowLevelLoadScreen
============
*/
void idGameLocal::ShowLevelLoadScreen( const char* mapName ) {
	if ( networkSystem->IsDedicated() ) {
		return;
	}
/*
	if ( !reloadingSameMap ) {
		bytesNeededForMapLoad = GetBytesNeededForMapLoad( mapName );
	} else {
		bytesNeededForMapLoad = 30 * 1024 * 1024;
	}
*/
	idStr stripped = mapName;
	stripped.StripFileExtension();

	mapMetaData = mapMetaDataList->FindMetaData( stripped, &defaultMetaData );
	mapInfo = declMapInfoType.LocalFind( mapMetaData->GetString( "mapinfo", "default" ) );

	using namespace sdProperties;	
	if ( sdUserInterfaceScope* scope = globalProperties.GetSubScope( "mapinfo" ) ) {
		if ( sdProperty* property = scope->GetProperty( "numObjectives", PT_FLOAT ) ) {
			*property->value.floatValue = mapInfo->GetData().GetFloat( "numObjectives", "0" );
		}
	}


	sdUserInterfaceLocal* ui;

	ui = GetUserInterface( pureWaitHandle );
	if ( ui != NULL ) {
		ui->Deactivate( false );
	}

	uiLevelLoadHandle = LoadUserInterface( "loadscreen", false, true );

	UpdateCampaignStats( true );

	ui = GetUserInterface( uiLevelLoadHandle );
	if ( ui != NULL ) {
		ui->SetCurrentTime( sys->Milliseconds() );
		ui->Activate();
		PacifierUpdate();
	}
}


/*
============
idGameLocal::UpdateLevelLoadScreen
============
*/
void idGameLocal::UpdateLevelLoadScreen( const wchar_t* status ) {
	if ( networkSystem->IsDedicated() ) {
		return;
	}

	using namespace sdProperties;
	sdUserInterfaceScope* scope = globalProperties.GetSubScope( "mapinfo" );	
	if( scope ) {
		if( sdProperty* property = scope->GetProperty( "mapStatus", PT_WSTRING )) {
			*property->value.wstringValue = status;			
		}
	}
	common->PacifierUpdate();
}


/*
============
idGameLocal::HideLevelLoadScreen
============
*/
void idGameLocal::HideLevelLoadScreen() {
	if ( networkSystem->IsDedicated() ) {
		return;
	}

	if ( sdUserInterfaceLocal* ui = GetUserInterface( uiLevelLoadHandle ) ) {		
		ui->Deactivate();
	}
}


/*
============
idGameLocal::GetUIManager
============
*/
sdUserInterfaceManager* idGameLocal::GetUIManager() {
	return uiManager;
}

/*
============
idGameLocal::SetGUIFloat
============
*/
void idGameLocal::SetGUIFloat( int handle, const char* name, float value ) {
	if ( networkSystem->IsDedicated() ) {	
		gameLocal.Warning( "Tried to run a GUI command on the server" );
	}

	sdUserInterfaceScope* scope;
	if ( handle == GUI_GLOBALS_HANDLE ) {
		scope = &gameLocal.globalProperties;
	} else {
		sdUserInterfaceLocal* ui = gameLocal.GetUserInterface( handle );
		if ( !ui ) {
			gameLocal.Warning( "idGameLocal::SetGUIFloat: could not find GUI" );
			return;
		}
		scope = &ui->GetState();
	}

	sdUIScopeParser parser( name );
	scope = gameLocal.GetUserInterfaceScope( *scope, parser );

	sdProperties::sdProperty* prop = NULL;

	const char* token = parser.GetNextEntry();
	if ( token == NULL ) {
		assert( false );
	} else {
		prop = scope->GetProperty( token, sdProperties::PT_FLOAT );
	}

	if ( prop != NULL ) {
		*( prop->value.floatValue ) = value;
	} else {
		gameLocal.Warning( "idGameLocal::SetGUIFloat: could not find float property '%s'", name );
	}
}

/*
============
idGameLocal::GetGUIFloat
============
*/
float idGameLocal::GetGUIFloat( int handle, const char* name ) {
	if( networkSystem->IsDedicated() ) {	
		gameLocal.Warning( "Tried to run a GUI command on the server" );
	}

	sdUserInterfaceScope* scope;

	if ( handle == GUI_GLOBALS_HANDLE ) {
		scope = &gameLocal.globalProperties;
	} else {
		sdUserInterfaceLocal* ui = gameLocal.GetUserInterface( handle );
		if( !ui ) {
			gameLocal.Warning( "idGameLocal::GetGUIFloat: could not find GUI" );
			return 0.0f;
		}
		scope = &ui->GetState();
	}

	sdUIScopeParser parser( name );
	scope = gameLocal.GetUserInterfaceScope( *scope, parser );

	sdProperties::sdProperty* prop = NULL;

	const char* token = parser.GetNextEntry();
	if ( token == NULL ) {
		assert( false );
	} else {
		prop = scope->GetProperty( token, sdProperties::PT_FLOAT );
	}

	if ( prop == NULL ) {
		 gameLocal.Warning( "idGameLocal::GetGUIFloat: could not find float property '%s'", name );
		 return 0.f;
	 }

	return prop->value.floatValue->GetValue();
}

/*
============
idGameLocal::SetGUIInt
============
*/
void idGameLocal::SetGUIInt( int handle, const char* name, int value ) {
	if ( networkSystem->IsDedicated() ) {	
		gameLocal.Warning( "Tried to run a GUI command on the server" );
	}

	sdUserInterfaceScope* scope;
	if ( handle == GUI_GLOBALS_HANDLE ) {
		scope = &gameLocal.globalProperties;
	} else {
		sdUserInterfaceLocal* ui = gameLocal.GetUserInterface( handle );
		if ( !ui ) {
			gameLocal.Warning( "idGameLocal::SetGUIInt: could not find GUI" );
			return;
		}
		scope = &ui->GetState();
	}

	sdUIScopeParser parser( name );
	scope = gameLocal.GetUserInterfaceScope( *scope, parser );

	sdProperties::sdProperty* prop = NULL;

	const char* token = parser.GetNextEntry();
	if ( token == NULL ) {
		assert( false );
	} else {
		prop = scope->GetProperty( token, sdProperties::PT_INT );
	}

	if ( prop != NULL ) {
		*( prop->value.intValue ) = value;
	} else {
		gameLocal.Warning( "idGameLocal::SetGUIInt: could not find int property '%s'", name );
	}
}

/*
============
idGameLocal::SetGUIVec2
============
*/
void idGameLocal::SetGUIVec2( int handle, const char* name, const idVec2& value ) {
	if ( networkSystem->IsDedicated() ) {	
		gameLocal.Warning( "Tried to run a GUI command on the server" );
	}

	sdUserInterfaceScope* scope;
	if ( handle == GUI_GLOBALS_HANDLE ) {
		scope = &gameLocal.globalProperties;
	} else {
		sdUserInterfaceLocal* ui = gameLocal.GetUserInterface( handle );
		if( !ui ) {
			gameLocal.Warning( "idGameLocal::SetGUIVec2: could not find GUI" );
			return;
		}
		scope = &ui->GetState();
	}

	sdUIScopeParser parser( name );
	scope = gameLocal.GetUserInterfaceScope( *scope, parser );

	sdProperties::sdProperty* prop = NULL;

	const char* token = parser.GetNextEntry();
	if ( token == NULL ) {
		assert( false );
	} else {
		prop = scope->GetProperty( token, sdProperties::PT_VEC2 );
	}

	if ( prop != NULL ) {
		*( prop->value.vec2Value ) = value;
	} else {
		gameLocal.Warning( "idGameLocal::SetGUIVec2: could not find vec2 property '%s'", name );
	}
}

/*
============
idGameLocal::SetGUIVec3
============
*/
void idGameLocal::SetGUIVec3( int handle, const char* name, const idVec3& value ) {
	if ( networkSystem->IsDedicated() ) {	
		gameLocal.Warning( "Tried to run a GUI command on the server" );
	}

	sdUserInterfaceScope* scope;
	if ( handle == GUI_GLOBALS_HANDLE ) {
		scope = &gameLocal.globalProperties;
	} else {
		sdUserInterfaceLocal* ui = gameLocal.GetUserInterface( handle );
		if( !ui ) {
			gameLocal.Warning( "idGameLocal::SetGUIVec3: could not find GUI" );
			return;
		}
		scope = &ui->GetState();
	}

	sdUIScopeParser parser( name );
	scope = gameLocal.GetUserInterfaceScope( *scope, parser );

	sdProperties::sdProperty* prop = NULL;

	const char* token = parser.GetNextEntry();
	if ( token == NULL ) {
		assert( false );
	} else {
		prop = scope->GetProperty( token, sdProperties::PT_VEC3 );
	}

	if ( prop != NULL ) {
		*( prop->value.vec3Value ) = value;
	} else {
		gameLocal.Warning( "idGameLocal::SetGUIVec3: could not find vec3 property '%s'", name );
	}
}

/*
============
idGameLocal::SetGUIVec4
============
*/
void idGameLocal::SetGUIVec4( int handle, const char* name, const idVec4& value ) {
	if ( networkSystem->IsDedicated() ) {	
		gameLocal.Warning( "Tried to run a GUI command on the server" );
	}

	sdUserInterfaceScope* scope;
	if ( handle == GUI_GLOBALS_HANDLE ) {
		scope = &gameLocal.globalProperties;
	} else {
		sdUserInterfaceLocal* ui = gameLocal.GetUserInterface( handle );
		if( !ui ) {
			gameLocal.Warning( "idGameLocal::SetGUIVec4: could not find GUI" );
			return;
		}
		scope = &ui->GetState();
	}

	sdUIScopeParser parser( name );
	scope = gameLocal.GetUserInterfaceScope( *scope, parser );

	sdProperties::sdProperty* prop = NULL;

	const char* token = parser.GetNextEntry();
	if ( token == NULL ) {
		assert( false );
	} else {
		prop = scope->GetProperty( token, sdProperties::PT_VEC4 );
	}

	if ( prop != NULL ) {
		*( prop->value.vec4Value ) = value;
	} else {
		gameLocal.Warning( "idGameLocal::SetGUIVec4: could not find vec4 property '%s'", name );
	}
}

/*
============
idGameLocal::SetGUIString
============
*/
void idGameLocal::SetGUIString( int handle, const char* name, const char* value ) {
	if ( networkSystem->IsDedicated() ) {	
		gameLocal.Warning( "Tried to run a GUI command on the server" );
	}

	sdUserInterfaceScope* scope;
	if ( handle == GUI_GLOBALS_HANDLE ) {
		scope = &gameLocal.globalProperties;
	} else {
		sdUserInterfaceLocal* ui = gameLocal.GetUserInterface( handle );
		if( !ui ) {
			gameLocal.Warning( "idGameLocal::SetGUIString: could not find GUI" );
			return;
		}
		scope = &ui->GetState();
	}

	sdUIScopeParser parser( name );
	scope = gameLocal.GetUserInterfaceScope( *scope, parser );

	sdProperties::sdProperty* prop = NULL;

	const char* token = parser.GetNextEntry();
	if ( token == NULL ) {
		assert( false );
	} else {
		prop = scope->GetProperty( token, sdProperties::PT_STRING );
	}

	if ( prop != NULL ) {
		*( prop->value.stringValue ) = value;
	} else {
		gameLocal.Warning( "idGameLocal::SetGUIString: could not find string property '%s'", name );
	}
}

/*
============
idGameLocal::SetGUIWString
============
*/
void idGameLocal::SetGUIWString( int handle, const char* name, const wchar_t* value ) {
	if( networkSystem->IsDedicated() ) {	
		gameLocal.Warning( "Tried to run a GUI command on the server" );
	}

	sdUserInterfaceScope* scope;

	if ( handle == GUI_GLOBALS_HANDLE ) {
		scope = &gameLocal.globalProperties;
	} else {
		sdUserInterfaceLocal* ui = gameLocal.GetUserInterface( handle );
		if( !ui ) {
			gameLocal.Warning( "idGameLocal::SetGUIWString: could not find GUI" );
			return;
		}
		scope = &ui->GetState();
	}

	sdUIScopeParser parser( name );
	scope = gameLocal.GetUserInterfaceScope( *scope, parser );

	sdProperties::sdProperty* prop = NULL;

	const char* token = parser.GetNextEntry();
	if ( token == NULL ) {
		assert( false );
	} else {
		prop = scope->GetProperty( token, sdProperties::PT_WSTRING );
	}

	if ( prop != NULL ) {
		*( prop->value.wstringValue ) = value;
	} else {
		gameLocal.Warning( "idGameLocal::SetGUIWString: could not find string property '%s'", name );
	}
}

/*
============
idGameLocal::SetGUITheme
============
*/
void idGameLocal::SetGUITheme( guiHandle_t handle, const char* theme ) {
	if( !handle ) {
		gameLocal.Error( "SetGUITheme: Invalid GUI handle." );
		return;
	}

	sdUserInterfaceLocal* ui = static_cast< sdUserInterfaceLocal* >( uiManager->GetUserInterface( handle ) );
	if( !ui ) {
		gameLocal.Warning( "idGameLocal::SetGUITheme : Could not find GUI." );
		return;
	}
	ui->SetTheme( theme );
}

/*
============
idGameLocal::GUIPostNamedEvent
============
*/
void idGameLocal::GUIPostNamedEvent( int handle, const char* window, const char* name ) {
	if( networkSystem->IsDedicated() ) {	
		gameLocal.Warning( "Tried to run a GUI command on the server" );
	}

	sdUserInterfaceLocal* ui = gameLocal.GetUserInterface( handle );
	if( !ui ) {
		gameLocal.Warning( "idGameLocal::GUIPostNamedEvent: could not find GUI" );
		return;
	}
	sdUIObject* object = NULL;
	if( idStr::Length( window )) {
		object = ui->GetWindow( window );
	}

	if( !object && idStr::Length( window )) {
		gameLocal.Warning( "idGameLocal::GUIPostNamedEvent: could not find window '%s'", window );
		return;
	} else if( object ) {
		object->PostNamedEvent( name, false );
	} else {	
		ui->PostNamedEvent( name );
	}	
}



/*
============
idGameLocal::UpdateCampaignStats
============
*/
void idGameLocal::UpdateCampaignStats( bool allowMedia ) {
	if( networkSystem->IsDedicated() ) {
		return;
	}

	sdGameRules* rulesInstance = gameLocal.GetRulesInstance( serverInfo.GetString( "si_rules" ) );

	if( rulesInstance != NULL ) {
		rulesInstance->UpdateClientFromServerInfo( serverInfo, allowMedia );
	}
}


/*
============
idGameLocal::CreateWeaponSwitchList
============
*/
void idGameLocal::CreateWeaponSwitchList( sdUIList* list ) {
	sdUIList::ClearItems( list );
	idPlayer* localPlayer = gameLocal.GetLocalPlayer();
	if( localPlayer == NULL ) {
		return;
	}

	using namespace sdProperties;

	const sdInventory& inv = localPlayer->GetInventory();
	const int currentWeapon = inv.GetCurrentWeapon();
	const int switchingWeapon = inv.GetSwitchingWeapon();

	if( currentWeapon == -1 || switchingWeapon == -1 ) {
		return;
	}

	const sdDeclInvItem* item = inv.GetItem( currentWeapon );
	const sdDeclInvItem* switchItem = inv.GetItem( switchingWeapon );

	typedef sdPair< const sdDeclInvItem*, int > invElement_t;
	idStaticList< invElement_t, 10 > items;

	for( int i = 0; i < inv.GetNumItems(); i++ ) {		
		if ( !inv.CanAutoEquip( i, false ) ) {
			continue;
		}

		const sdDeclInvItem* item = inv.GetItem( i );
		if ( inv.GetSlotForWeapon( i ) != -1 ) {
			int j = 0;
			const sdDeclInvSlot* slot = gameLocal.declInvSlotType.LocalFindByIndex( item->GetSlot() );

			while( j < items.Num() ) {
				if( item->GetSlot() < items[ j ].first->GetSlot() ) {
					break;
				}

				const sdDeclInvSlot* otherSlot = gameLocal.declInvSlotType.LocalFindByIndex( items[ j ].first->GetSlot() );
				if( slot->GetBank() < otherSlot->GetBank() ) {
					break;
				}

				j++;
			}
			items.Insert( invElement_t( item, i ), j );
		}
	}

	int offset = 0;
	for( int i = 0; i < items.Num(); i++ ) {
		int bankNumber = i;
		const sdDeclInvItem* invItem = inv.GetItem( items[ i ].second );
		const bool available = inv.CheckWeaponHasAmmo( invItem ) || invItem->GetData().GetBool( "select_when_empty", "0" );
		const char* material = invItem->GetData().GetString( "mtr_weaponmenu", "_default" );
		const int title = invItem->GetItemName()->Index();
 		float weaponAmmo = -1.0f;
	
		const idList< itemClip_t >& itemClips = invItem->GetClips();

		if ( itemClips.Num() && itemClips[ 0 ].ammoPerShot > 0 ) {
			float ammo;
			float totalAmmo;
			if( invItem->GetData().GetBool( "show_all_ammo", "0" )) {
				ammo = inv.GetAmmo( itemClips[ 0 ].ammoType ) / itemClips[ 0 ].ammoPerShot;
				totalAmmo = inv.GetMaxAmmo( itemClips[ 0 ].ammoType ) / itemClips[ 0 ].ammoPerShot;				
			} else {
				ammo = inv.GetClip( items[ i ].second, 0 );
				totalAmmo = inv.GetClipSize( items[ i ].second, 0 );				
			}
			weaponAmmo = idMath::ClampFloat( 0.0f, 1.0f, ammo / totalAmmo );
		}

		int index = sdUIList::InsertItem( list, va( L"%i\t<material = '::%hs'>\t%f\t%i\t%i", invItem->GetSlot(), material, weaponAmmo, title, available ), -1, 0 );

		if( invItem == switchItem ) {
			list->SelectItem( index );
			localPlayer->SetWeaponSelectionItemIndex( bankNumber );
		}
	}
}

/*
============
idGameLocal::CreateColorList
============
*/
void idGameLocal::CreateColorList( sdUIList* list ) {
	idVec4 customColor;
	list->GetUI()->PopScriptVar( customColor );

	idStr currentColorStr;
	list->GetUI()->PopScriptVar( currentColorStr );

	idVec4 currentColor = colorWhite;
	if( !currentColorStr.IsEmpty() ) {
		currentColor = sdTypeFromString< idVec4 >( currentColorStr.c_str() );
	}

	// if refreshing, only add new custom colors
	bool refreshOnly;
	list->GetUI()->PopScriptVar( refreshOnly );

	if( refreshOnly == false ) {
		sdUIList::ClearItems( list );
	}

	idWStr str;
	idWStr colorSymbol;
	idStr color;

	bool customMatchesDefault = false;
	int toSelect = -1;
	static const float COLOR_EPSILON = 0.001f;

	// check against previous custom colors
	if( refreshOnly == false || list->GetNumItems() == 0 ) {
		// skip default color
		for( int i = 1; i < COLOR_BITS + 1; i++ ) {
			const idVec4& c = idStr::ColorForIndex( i );
			if( c.ToVec3().Compare( customColor.ToVec3(), COLOR_EPSILON ) ) {
				customMatchesDefault = true;
			}
			colorSymbol = va( L"%hs", idStr::StrForColorIndex( i ) );
			sdUIList::CleanUserInput( colorSymbol );
			color = c.ToString( 2 );
			str = va( L"<fore = %hs><material = '::guis/assets/white'>\t%ls\t%hs", color.c_str(), colorSymbol.c_str(), color.c_str() );
			int index = sdUIList::InsertItem( list, str.c_str(), -1, 0 );

			if( currentColor.ToVec3().Compare( c.ToVec3(), COLOR_EPSILON ) ) {
				toSelect = index;
			}
		}
	} else {
		for( int i = 0; i < list->GetNumItems(); i++ ) {
			idVec4 c = sdTypeFromString< idVec4 >( va( "%ls", list->GetItemText( i, 2, true ) ) );
			if( c.ToVec3().Compare( customColor.ToVec3(), COLOR_EPSILON ) ) {
				customMatchesDefault = true;
			}
			if( currentColor.ToVec3().Compare( c.ToVec3(), COLOR_EPSILON ) ) {
				toSelect = i;
			}
		}
	}



	if( !customMatchesDefault ) {
		str = va( L"<fore = %hs><material = '::guis/assets/white'>\t%\t%hs", customColor.ToString( 2 ), customColor.ToString( 2 ) );
		int index = sdUIList::InsertItem( list, str.c_str(), -1, 0 );
		if( currentColor.ToVec3().Compare( customColor.ToVec3(), COLOR_EPSILON ) ) {
			toSelect = index;
		}
	}
	list->SelectItem( toSelect );
}

/*
============
idGameLocal::CreateSpawnLocationList
============
*/
void idGameLocal::CreateSpawnLocationList( sdUIList* list ) {
	sdUIList::ClearItems( list );
	idStr team;
	list->GetUI()->PopScriptVar( team );

	const sdTeamInfo* info = sdTeamManager::GetInstance().GetTeamSafe( team.c_str() );
	if( info == NULL ) {
		return;
	}

	int currentSpawn = -1;
	int defaultSpawn = -1;

	int numLocs = info->GetNumSpawnLocations();
	int* spawnCounts = static_cast< int* >( _alloca( numLocs * sizeof( int ) ) );
	memset( &spawnCounts[ 0 ], 0, numLocs * sizeof( int ) );
	
	const idPlayer* localPlayer = gameLocal.GetLocalPlayer();
	for( int i = 0; i < MAX_CLIENTS; i++ ) {
		idPlayer* player = gameLocal.GetClient( i );
		if( player == NULL || player->GetGameTeam() != info ) {
			continue;
		}
		const idEntity* playerSpawnLoc = player->GetSpawnPoint();
		for( int si = 0; si < numLocs; si++ ) {
			const idEntity* spawn = info->GetSpawnLocation( si );
			if( playerSpawnLoc == NULL && spawn == info->GetDefaultSpawn() ) {
				defaultSpawn = si;
				if( playerSpawnLoc == NULL ) {
					spawnCounts[ defaultSpawn ]++;
					break;
				}
			}
			if( spawn == playerSpawnLoc ) {
				spawnCounts[ si ]++;

				if( player == localPlayer ) {
					currentSpawn = si;
				}
				break;
			}			
		}
	}

	idWStr loc;	
	for( int i = 0; i < numLocs; i++ ) {
		const idEntity* spawn = info->GetSpawnLocation( i );

		sdLocationMarker::GetLocationText( spawn->GetPhysics()->GetOrigin(), loc );
		int index = sdUIList::InsertItem( list, va( L"%ls\t%i\t%hs", loc.c_str(), spawnCounts[ i ], spawn->GetName() ), -1, 0 );
		if( i == currentSpawn ) {
			list->SelectItem( index );
		}
	}
	int index = sdUIList::InsertItem( list, L"<loc = 'guis/mainmenu/defaultspawn'>\t\t", -1, 0 );
	if( currentSpawn == -1 ) {
		list->SelectItem( index );
	}
}


/*
============
idGameLocal::CreateMSAAList
============
*/
void idGameLocal::CreateMSAAList( sdUIList* list ) {
	int num = renderSystem->GetNumMSAAModes();
	for (int i=0; i<num; i++) {
		int val;
		const char *msaaName = renderSystem->GetMSAAMode( i, val );
		int index = sdUIList::InsertItem( list, va( L"%hs", msaaName ), -1, 0 );
		list->SetItemDataInt( val, index, 0, true );
	}
}


/*
============
idGameLocal::CreateSoundPlaybackList
============
*/
void idGameLocal::CreateSoundPlaybackList( sdUIList* list ) {
	sdUIList::ClearItems( list );
	const idWStrList* devices = soundSystem->ListSoundPlaybackDevices();
	
	for( int i = 0; devices != NULL && i < devices->Num(); i++ ) {
		const idWStr& str = (*devices)[ i ];
		int hash = soundSystem->GetAudioDeviceHash( str.c_str() );
		sdUIList::InsertItem( list, va( L"%ls\t%d", str.c_str(), hash ), -1, 0 );
	}

	soundSystem->FreeDeviceList( devices );
}


/*
============
idGameLocal::CreateSoundCaptureList
============
*/
void idGameLocal::CreateSoundCaptureList( sdUIList* list ) {
	sdUIList::ClearItems( list );

	const idWStrList* devices = soundSystem->ListSoundCaptureDevices();

	for( int i = 0; devices != NULL && i < devices->Num(); i++ ) {
		const idWStr& str = (*devices)[ i ];
		int hash = soundSystem->GetAudioDeviceHash( str.c_str() );
		sdUIList::InsertItem( list, va( L"%ls\t%d", str.c_str(), hash ), -1, 0 );
	}

	soundSystem->FreeDeviceList( devices );
}

/*
============
idGameLocal::CreateModList
============
*/
void idGameLocal::CreateModList( sdUIList* list ) {
	sdUIList::ClearItems( list );

	idModList* mods = fileSystem->ListMods();

	if( mods != NULL ) {
		for( int i = 0; i < mods->GetNumMods(); i++ ) {
			sdUIList::InsertItem( list, va( L"%hs\t%hs", mods->GetDescription( i ), mods->GetMod( i ) ), -1, 0 );
		}
		fileSystem->FreeModList( mods );
	}
}

static void AddLifeStatsItems( sdUIList* list, const char* material, const idList< const sdStatsTracker::lifeStatsData_t* >& data ) {
	for( int i = 0; i < data.Num(); i++ ) {
		const sdStatsTracker::lifeStatsData_t* item = data[ i ];
		const lifeStat_t& stat = gameLocal.lifeStats[ item->index ];

		
		switch( item->newValue.GetType() ) {
			case sdNetStatKeyValue::SVT_INT:
			case sdNetStatKeyValue::SVT_INT_MAX:
				if( stat.isTimeBased ) {
					idStr::hmsFormat_t format;
					format.showZeroMinutes = true;

					sdUIList::InsertItem( list, va( L"<material = '%hs'>\t<loc = '%hs'>\t%hs (%hs)", material, stat.text->GetName(), idStr::MS2HMS( item->newValue.GetInt() * 1000, format ), idStr::MS2HMS( item->oldValue.GetInt() * 1000, format ) ), -1, 0 );
				} else {
					sdUIList::InsertItem( list, va( L"<material = '%hs'>\t<loc = '%hs'>\t%i (%i)", material, stat.text->GetName(), item->newValue.GetInt(), item->oldValue.GetInt() ), -1, 0 );
				}
				break;
			case sdNetStatKeyValue::SVT_FLOAT:
			case sdNetStatKeyValue::SVT_FLOAT_MAX:
				sdUIList::InsertItem( list, va( L"<material = '%hs'>\t<loc = '%hs'>\t%i (%i)", material, stat.text->GetName(), idMath::Ftoi( idMath::Ceil( item->newValue.GetFloat() ) ), idMath::Ftoi( idMath::Ceil( item->oldValue.GetFloat() ) ) ), -1, 0 );
				break;
		}
	}
}

/*
============
idGameLocal::CreateLifeStatsList
============
*/
void idGameLocal::CreateLifeStatsList( sdUIList* list ) {
	sdUIList::ClearItems( list );

	idList< const sdStatsTracker::lifeStatsData_t* > improved;
	idList< const sdStatsTracker::lifeStatsData_t* > unchanged;
	idList< const sdStatsTracker::lifeStatsData_t* > worse;

	sdGlobalStatsTracker::GetInstance().GetTopLifeStats( improved, unchanged, worse );


	AddLifeStatsItems( list, "improved", improved );
	AddLifeStatsItems( list, "unchanged", unchanged );
	AddLifeStatsItems( list, "nodraw", worse );
}


class sdSortCategories {
public:
	sdSortCategories( const sdProficiencyTable& table ) : table( table ) {}
	int operator()( const sdDeclPlayerClass::proficiencyCategory_t* lhs, const sdDeclPlayerClass::proficiencyCategory_t* rhs ) {
		float lhsPercent = table.GetPercent( lhs->index );
		float rhsPercent = table.GetPercent( rhs->index );

		return ( idMath::Ftoi( rhsPercent * 100 ) ) - ( idMath::Ftoi( lhsPercent * 100 ) );
	}

	const sdProficiencyTable& table;
};

/*
============
idGameLocal::CreatePredictedUpgradesList
============
*/
void idGameLocal::CreatePredictedUpgradesList( sdUIList* list ) {
	sdUIList::ClearItems( list );

	idPlayer* localPlayer = gameLocal.GetLocalPlayer();
	if( localPlayer == NULL ) {	
		return;
	}

	const sdTeamInfo* ti = localPlayer->GetGameTeam();
	idList< const sdDeclPlayerClass::proficiencyCategory_t* > bestCategories;

	const sdProficiencyTable& table = localPlayer->GetProficiencyTable();

	for( int i = 0; i < gameLocal.declPlayerClassType.Num(); i++ ) {
		const sdDeclPlayerClass* pc = gameLocal.declPlayerClassType.LocalFindByIndex( i );
		if( pc->GetTeam() != ti && ti != NULL ) {
			continue;
		}

		for ( int j = 0; j < pc->GetNumProficiencies(); j++ ) {
			const sdDeclPlayerClass::proficiencyCategory_t& category = pc->GetProficiency( j );

			float percent = table.GetPercent( category.index );
			if( percent >= 1.0f ) {
				continue;
			}

			int k;
			for( k = 0; k < bestCategories.Num(); k++ ) {
				if( bestCategories[ k ]->index == category.index ) {
					break;
				}
			}
			if( k >= bestCategories.Num() ) {
				bestCategories.Append( &category );
			}
		}
	}

	sdQuickSort( bestCategories.Begin(), bestCategories.End(), sdSortCategories( table ) );

	int numAdded = 0;
	for( int i = 0; i < bestCategories.Num() && numAdded < 2; i++ ) {
		int currentLevel = table.GetLevel( bestCategories[ i ]->index );
		if( currentLevel < bestCategories[ i ]->upgrades.Num() ) {
			const sdDeclProficiencyType* type = gameLocal.declProficiencyTypeType.LocalFindByIndex( bestCategories[ i ]->index );

			const sdDeclPlayerClass::proficiencyUpgrade_t& upgrade = bestCategories[ i ]->upgrades[ currentLevel ];
			int index = sdUIList::InsertItem( list, va( L"<material = '%hs'>\t<loc = '%hs'>\t<flags customdraw>%hs", 
				upgrade.materialInfo.c_str(), 
				upgrade.title->GetName(),
				type->GetName()	),
				-1, 0 );

			float percent = table.GetPercent( bestCategories[ i ]->index );

			// push some extra data along for the GUI
			list->SetItemDataInt( percent * 100 , index, 0, true );
			list->SetItemDataInt( currentLevel, index, 1, true );
			numAdded++;
		}
	}
}


/*
============
idGameLocal::CreateUpgradesReviewList
============
*/
void idGameLocal::CreateUpgradesReviewList( sdUIList* list ) {
	sdUIList::ClearItems( list );

	idPlayer* localPlayer = gameLocal.GetLocalPlayer();
	if( localPlayer == NULL ) {	
		return;
	}

	const sdTeamInfo* ti = localPlayer->GetGameTeam();
	const sdProficiencyTable& table = localPlayer->GetProficiencyTable();

	idList< const sdDeclPlayerClass::proficiencyCategory_t* > bestCategories;

	for( int i = 0; i < gameLocal.declPlayerClassType.Num(); i++ ) {
		const sdDeclPlayerClass* pc = gameLocal.declPlayerClassType.LocalFindByIndex( i );
		if( pc->GetTeam() != ti && ti != NULL ) {
			continue;
		}

		for ( int j = 0; j < pc->GetNumProficiencies(); j++ ) {
			const sdDeclPlayerClass::proficiencyCategory_t& category = pc->GetProficiency( j );

			int k;
			for( k = 0; k < bestCategories.Num(); k++ ) {
				if( bestCategories[ k ]->index == category.index ) {
					break;
				}
			}
			if( k < bestCategories.Num() ) {
				continue;				
			}

			bestCategories.Append( &category );

			int currentLevel = table.GetLevel(category.index );

			for ( int level = 0; level < currentLevel && level < category.upgrades.Num(); level++ ) {
				const sdDeclPlayerClass::proficiencyUpgrade_t& upgrade = category.upgrades[ level ];

				int index = sdUIList::InsertItem( list, va( L"<material = '%hs'>\t<loc = '%hs'>", 
					upgrade.materialInfo.c_str(), 
					upgrade.title->GetName()),
					-1, 0 );
			}
		}
	}
}

/*
===============
idGameLocal::UpdateHudStats
===============
*/
void idGameLocal::UpdateHudStats( idPlayer* player ) {
	localPlayerProperties.Update( player );
	limboProperties.Update();
	sdAdminSystem::GetInstance().UpdateProperties();
}

#undef ON_UIVALID
#undef ON_UIINVALID

