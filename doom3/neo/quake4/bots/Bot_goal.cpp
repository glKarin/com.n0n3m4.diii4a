// Bot_goal.cpp
//

#include "../../idlib/precompiled.h"
#pragma hdrstop

idCVar bot_droppedweight( "bot_droppedweight", "1000", CVAR_CHEAT | CVAR_INTEGER, "" );
idCVar bot_itemsfile( "bot_itemsfile", "items.c", CVAR_CHEAT, "" );

idBotGoalManager botGoalManager;

/*
==================
idBotGoalManager::idBotGoalManager
==================
*/
idBotGoalManager::idBotGoalManager()
{
	freelevelitems = NULL;
	levelitems = NULL;
}

/*
==================
idBotGoalManager::BotGoalStateFromHandle
==================
*/
bot_goalstate_t* idBotGoalManager::BotGoalStateFromHandle( int handle )
{
	if( handle <= 0 || handle > MAX_CLIENTS )
	{
		gameLocal.Error( "goal state handle %d out of range\n", handle );
		return NULL;
	}

	if( !botgoalstates[handle].InUse() )
	{
		gameLocal.Error( "invalid goal state %d\n", handle );
		return NULL;
	}

	return &botgoalstates[handle];
}

/*
==================
idBotGoalManager::BotInterbreedGoalFuzzyLogic
==================
*/
void idBotGoalManager::BotInterbreedGoalFuzzyLogic( int parent1, int parent2, int child )
{
	bot_goalstate_t* p1, * p2, * c;

	p1 = BotGoalStateFromHandle( parent1 );
	p2 = BotGoalStateFromHandle( parent2 );
	c = BotGoalStateFromHandle( child );

	botFuzzyWeightManager.InterbreedWeightConfigs( p1->itemweightconfig, p2->itemweightconfig, c->itemweightconfig );
}

/*
========================
idBotGoalManager::BotSaveGoalFuzzyLogic
========================
*/
void idBotGoalManager::BotSaveGoalFuzzyLogic( int goalstate, char* filename )
{
	bot_goalstate_t* gs;

	gs = BotGoalStateFromHandle( goalstate );

	//WriteWeightConfig(filename, gs->itemweightconfig);
}

/*
========================
idBotGoalManager::BotMutateGoalFuzzyLogic
========================
*/
void idBotGoalManager::BotMutateGoalFuzzyLogic( int goalstate, float range )
{
	bot_goalstate_t* gs;

	gs = BotGoalStateFromHandle( goalstate );

	botFuzzyWeightManager.EvolveWeightConfig( gs->itemweightconfig );
}

/*
========================
idBotGoalManager::ParseItemInfo
========================
*/
void idBotGoalManager::ParseItemInfo( idParser& parser, iteminfo_t* itemInfo )
{
	idToken token;

	parser.ExpectTokenString( "{" );

	while( true )
	{
		if( !parser.ReadToken( &token ) )
		{
			parser.Error( "Unexpected end of file found while parsing weaponinfo!" );
		}

		if( token == "}" )
		{
			break;
		}

		if( token == "name" )
		{
			idToken valueToken;
			if( !parser.ReadToken( &valueToken ) )
			{
				parser.Error( "Failed to read value token, encountered EOF parsing weapon info!" );
			}
			itemInfo->name = valueToken;
		}
		else if( token == "model" )
		{
			idToken valueToken;
			if( !parser.ReadToken( &valueToken ) )
			{
				parser.Error( "Failed to read value token, encountered EOF parsing weapon info!" );
			}
			itemInfo->model = valueToken;
		}
		else if( token == "modelindex" )
		{
			idToken valueToken;
			if( !parser.ReadToken( &valueToken ) )
			{
				parser.Error( "Failed to read value token, encountered EOF parsing weapon info!" );
			}
			itemInfo->modelindex = valueToken.GetIntValue();
		}
		else if( token == "type" )
		{
			idToken valueToken;
			if( !parser.ReadToken( &valueToken ) )
			{
				parser.Error( "Failed to read value token, encountered EOF parsing weapon info!" );
			}
			itemInfo->type = valueToken.GetIntValue();
		}
		else if( token == "index" )
		{
			idToken valueToken;
			if( !parser.ReadToken( &valueToken ) )
			{
				parser.Error( "Failed to read value token, encountered EOF parsing weapon info!" );
			}
			itemInfo->index = valueToken.GetIntValue();
		}
		else if( token == "respawntime" )
		{
			idToken valueToken;
			if( !parser.ReadToken( &valueToken ) )
			{
				parser.Error( "Failed to read value token, encountered EOF parsing weapon info!" );
			}
			itemInfo->respawntime = valueToken.GetIntValue();
		}
		else if( token == "mins" )
		{
			float value[3];
			if( !parser.Parse1DMatrixLegacy( 3, &value[0] ) )
			{
				parser.Error( "Failed to read value token, encountered EOF parsing weapon info!" );
			}
			itemInfo->mins.Set( value[0], value[1], value[2] );
		}
		else if( token == "maxs" )
		{
			float value[3];
			if( !parser.Parse1DMatrixLegacy( 3, &value[0] ) )
			{
				parser.Error( "Failed to read value token, encountered EOF parsing weapon info!" );
			}
			itemInfo->maxs.Set( value[0], value[1], value[2] );
		}
		else
		{
			parser.Error( "ParseProjectileInfo: Unexpected token %s\n", token.c_str() );
		}
	}
}

/*
========================
idBotGoalManager::LoadItemConfig
========================
*/
itemconfig_t* idBotGoalManager::LoadItemConfig( char* filename )
{
	idToken token;
	idStr path;
	itemconfig_t ic;
	iteminfo_t* ii;
	idParser source;

	path = filename;
	rvmScopedLexerBaseFolder scopedBaseFolder( BOTFILESBASEFOLDER );

	if( !source.LoadFile( path ) )
	{
		gameLocal.Error( "counldn't load %s\n", path.c_str() );
		return NULL;
	}

	ic.Reset();

	//parse the item config file
	while( source.ReadToken( &token ) )
	{
		if( token == "iteminfo" )
		{
			if( ic.numiteminfo >= MAX_BOT_ITEM_INFO )
			{
				source.Error( "more than %d item info defined\n", MAX_BOT_ITEM_INFO );
				return NULL;
			}

			ii = &ic.iteminfo[ic.numiteminfo];
			if( !source.ExpectTokenType( TT_STRING, 0, &token ) )
			{
				return NULL;
			}

			token.StripDoubleQuotes();
			ii->classname = token;

			ParseItemInfo( source, ii );

			ii->number = ic.numiteminfo;
			ic.numiteminfo++;
		}
		else
		{
			gameLocal.Error( "unknown definition %s\n", token.c_str() );
			return NULL;
		}
	}

	if( !ic.numiteminfo )
	{
		common->Warning( "no item info loaded\n" );
	}

#if 0 //k
	common->DPrintf( "loaded %s\n", path );
#else
	common->DPrintf( "loaded %s\n", path.c_str() );
#endif
	itemconfiglocal = ic;
	itemconfig = &itemconfiglocal;

	return itemconfig;
}

/*
========================
idBotGoalManager::ItemWeightIndex
========================
*/
int* idBotGoalManager::ItemWeightIndex( weightconfig_t* iwc, itemconfig_t* ic )
{
	int* index, i;

	//initialize item weight index
	index = new int[( sizeof( int ) * ic->numiteminfo )]; // jmarshall: Get this off of the heap!

	for( i = 0; i < ic->numiteminfo; i++ )
	{
		index[i] = botFuzzyWeightManager.FindFuzzyWeight( iwc, ( char* )ic->iteminfo[i].classname.c_str() );
		if( index[i] < 0 )
		{
			common->Warning( "item info %d \"%s\" has no fuzzy weight\r\n", i, ic->iteminfo[i].classname.c_str() );
		}
	}
	return index;
}

/*
========================
idBotGoalManager::InitLevelItemHeap
========================
*/
void idBotGoalManager::InitLevelItemHeap( void )
{
	for( int i = 0; i < MAX_BOT_LEVEL_ITEMS - 1; i++ )
	{
		levelitemheap[i].next = &levelitemheap[i + 1];
	}

	levelitemheap[MAX_BOT_LEVEL_ITEMS - 1].next = NULL;

	freelevelitems = levelitemheap;
}

/*
========================
idBotGoalManager::AllocLevelItem
========================
*/
levelitem_t* idBotGoalManager::AllocLevelItem( void )
{
	levelitem_t* li;

	li = freelevelitems;
	if( !li )
	{
		gameLocal.Error( "out of level items\n" );
		return NULL;
	}

	freelevelitems = freelevelitems->next;
	//memset(li, 0, sizeof(levelitem_t));
	li->Reset();
	return li;
}

/*
========================
idBotGoalManager::FreeLevelItem
========================
*/
void idBotGoalManager::FreeLevelItem( levelitem_t* li )
{
	li->next = freelevelitems;
	freelevelitems = li;
}

/*
========================
idBotGoalManager::AddLevelItemToList
========================
*/
void idBotGoalManager::AddLevelItemToList( levelitem_t* li )
{
	if( levelitems )
	{
		levelitems->prev = li;
	}

	li->prev = NULL;
	li->next = levelitems;
	levelitems = li;
}

/*
========================
idBotGoalManager::RemoveLevelItemFromList
========================
*/
void idBotGoalManager::RemoveLevelItemFromList( levelitem_t* li )
{
	if( li->prev )
	{
		li->prev->next = li->next;
	}
	else
	{
		levelitems = li->next;
	}
	if( li->next )
	{
		li->next->prev = li->prev;
	}
}

/*
========================
idBotGoalManager::BotFreeInfoEntities
========================
*/
void idBotGoalManager::BotFreeInfoEntities( void )
{
	maplocation_t* ml, * nextml;
	campspot_t* cs, * nextcs;

//	for (ml = maplocations; ml; ml = nextml)
//	{
//		nextml = ml->next;
//		// jmarshall
//				//FreeMemory(ml);
//		// jmarshall end
//	}
	maplocations.Clear();
	//for (cs = campspots; cs; cs = nextcs)
	//{
	//	nextcs = cs->next;
	//	// jmarshall
	//			//FreeMemory(cs);
	//	// jmarshall end
	//}
	campspots.Clear();
}

/*
========================
idBotGoalManager::BotInitInfoEntities
========================
*/
void idBotGoalManager::BotInitInfoEntities( void )
{
	//char classname[MAX_EPAIRKEY];
	idStr classname;
	maplocation_t* ml;
	campspot_t* cs;

	BotFreeInfoEntities();

	maplocations.Clear();
	campspots.Clear();

	for( int i = 0; i < gameLocal.num_entities; i++ )
	{
		idEntity* ent = gameLocal.entities[i];

		if( ent == NULL )
		{
			continue;
		}

		classname = ent->spawnArgs.GetString( "classname" );

		//map locations
		if( classname == "target_location" )
		{
			maplocation_t ml;
			ml.origin = ent->GetPhysics()->GetOrigin();
			ml.name = ent->GetKey( "message" );
			maplocations.Append( ml );
		}
		//camp spots
		else if( classname == "info_camp" )
		{
			//cs = (campspot_t*)G_AllocClearedMemory(sizeof(campspot_t));
			campspot_t cs;
			cs.origin = ent->GetPhysics()->GetOrigin();
			cs.name = ent->GetKey( "message" );
			cs.range = ent->GetFloat( "range" );
			cs.weight = ent->GetFloat( "weight" );
			cs.wait = ent->GetFloat( "wait" );
			cs.random = ent->GetFloat( "random" );
			campspots.Append( cs );
		}
	}
	common->Printf( "%d map locations\n", maplocations.Num() );
	common->Printf( "%d camp spots\n", campspots.Num() );
}

/*
=======================
idBotGoalManager::InitLevelItems
=======================
*/
void idBotGoalManager::InitLevelItems( void )
{
	int i, spawnflags, value;
	idStr classname;
	idVec3 origin, end;
	int ent, goalareanum;
	itemconfig_t* ic;
	levelitem_t* li;
	//bsp_trace_t trace;

	//initialize the map locations and camp spots
	BotInitInfoEntities();

	//initialize the level item heap
	InitLevelItemHeap();
	levelitems = NULL;
	numlevelitems = 0;

	ic = itemconfig;
	if( !ic )
	{
		return;
	}

	//update the modelindexes of the item info
	//for (i = 0; i < ic->numiteminfo; i++)
	//{
	//	if (!ic->iteminfo[i].modelindex)
	//	{
	//		Log_Write("item %s has modelindex 0", ic->iteminfo[i].classname);
	//	}
	//}

	//for (ent = AAS_NextBSPEntity(0); ent; ent = AAS_NextBSPEntity(ent))
	for( int idx = 0; idx < gameLocal.num_entities; idx++ )
	{
		idItem* ent = gameLocal.entities[idx]->Cast<idItem>();

		if( ent == 0 )
		{
			continue;
		}

		spawnflags = ent->GetInt( "spawnflags" );
		classname = ent->spawnArgs.GetString( "classname" );

		for( i = 0; i < ic->numiteminfo; i++ )
		{
			if( !strcmp( classname, ic->iteminfo[i].classname ) )
			{
				break;
			}
		}
		if( i >= ic->numiteminfo )
		{
			gameLocal.Warning( "entity %s unknown item\r\n", classname.c_str() );
			continue;
		}

		//get the origin of the item.
		origin = ent->GetPhysics()->GetOrigin();
		goalareanum = 0;
		// jmarshall - fix floating items
		//if it is a floating item
		//if (spawnflags & 1)
		//{
		//	//if the item is not floating in water
		//	if (!(AAS_PointContents(origin) & CONTENTS_WATER))
		//	{
		//		VectorCopy(origin, end);
		//		end[2] -= 32;
		//		trace = AAS_Trace(origin, ic->iteminfo[i].mins, ic->iteminfo[i].maxs, end, -1, CONTENTS_SOLID | CONTENTS_PLAYERCLIP);
		//		//if the item not near the ground
		//		if (trace.fraction >= 1)
		//		{
		//			//if the item is not reachable from a jumppad
		//			goalareanum = AAS_BestReachableFromJumpPadArea(origin, ic->iteminfo[i].mins, ic->iteminfo[i].maxs);
		//			Log_Write("item %s reachable from jumppad area %d\r\n", ic->iteminfo[i].classname, goalareanum);
		//			//G_Printf( "item %s reachable from jumppad area %d\r\n", ic->iteminfo[i].classname, goalareanum);
		//			if (!goalareanum) continue;
		//		} //end if
		//	} //end if
		//} //end if
		// jmarshall end

		li = AllocLevelItem();
		if( !li )
		{
			return;
		}

		li->number = ++numlevelitems;
		li->timeout = 0;
		li->item = ent;
		li->name = classname;

		li->flags = 0;
		if( ent->GetBool( "notfree" ) )
		{
			li->flags |= IFL_NOTFREE;
		}
		if( ent->GetBool( "notteam" ) )
		{
			li->flags |= IFL_NOTTEAM;
		}
		if( ent->GetBool( "notsingle" ) )
		{
			li->flags |= IFL_NOTSINGLE;
		}
		if( ent->GetBool( "notbot" ) )
		{
			li->flags |= IFL_NOTBOT;
		}

		if( classname == "item_botroam" )
		{
			li->flags |= IFL_ROAM;
			//AAS_FloatForBSPEpairKey(ent, "weight", &li->weight);
			li->weight = ent->GetFloat( "weight" );
		}

		////if not a stationary item
		//if (!(spawnflags & 1))
		//{
		//	if (!AAS_DropToFloor(origin, ic->iteminfo[i].mins, ic->iteminfo[i].maxs))
		//	{
		//		G_Printf( "%s in solid at (%1.1f %1.1f %1.1f)\n",
		//			classname, origin[0], origin[1], origin[2]);
		//	} //end if
		//} //end if

		//item info of the level item
		li->iteminfo = i;

		//origin of the item
		VectorCopy( origin, li->origin );
		VectorCopy( origin, li->goalorigin );

		AddLevelItemToList( li );
	}
	gameLocal.Printf( "found %d level items\n", numlevelitems );
}

/*
=======================
idBotGoalManager::BotGoalName
=======================
*/
void idBotGoalManager::BotGoalName( int number, char* name, int size )
{
	levelitem_t* li;

	if( !itemconfig )
	{
		return;
	}

	for( li = levelitems; li; li = li->next )
	{
		if( li->number == number )
		{
			name[size - 1] = '\0';
			strcpy( name, itemconfig->iteminfo[li->iteminfo].name );
			return;
		}
	}
}

/*
=======================
idBotGoalManager::BotResetAvoidGoals
=======================
*/
void idBotGoalManager::BotResetAvoidGoals( int goalstate )
{
	bot_goalstate_t* gs;

	gs = BotGoalStateFromHandle( goalstate );
	if( !gs )
	{
		return;
	}
	memset( gs->avoidgoals, 0, MAX_AVOIDGOALS * sizeof( int ) );
	memset( gs->avoidgoaltimes, 0, MAX_AVOIDGOALS * sizeof( float ) );
}

/*
=======================
idBotGoalManager::BotResetAvoidGoals
=======================
*/
void idBotGoalManager::BotDumpAvoidGoals( int goalstate )
{
	int i;
	bot_goalstate_t* gs;
	char name[32];

	gs = BotGoalStateFromHandle( goalstate );
	if( !gs )
	{
		return;
	}
	for( i = 0; i < MAX_AVOIDGOALS; i++ )
	{
		if( gs->avoidgoaltimes[i] >= Bot_Time() )
		{
			BotGoalName( gs->avoidgoals[i], name, 32 );
			//Log_Write("avoid goal %s, number %d for %f seconds", name,
			//	gs->avoidgoals[i], gs->avoidgoaltimes[i] - AAS_Time());
		}
	}
}

/*
=======================
idBotGoalManager::BotAddToAvoidGoals
=======================
*/
void idBotGoalManager::BotAddToAvoidGoals( bot_goalstate_t* gs, int number, float avoidtime )
{
	int i;

	for( i = 0; i < MAX_AVOIDGOALS; i++ )
	{
		//if the avoid goal is already stored
		if( gs->avoidgoals[i] == number )
		{
			gs->avoidgoals[i] = number;
			gs->avoidgoaltimes[i] = Bot_Time() + avoidtime;
			return;
		}
	}

	for( i = 0; i < MAX_AVOIDGOALS; i++ )
	{
		//if this avoid goal has expired
		if( gs->avoidgoaltimes[i] < Bot_Time() )
		{
			gs->avoidgoals[i] = number;
			gs->avoidgoaltimes[i] = Bot_Time() + avoidtime;
			return;
		}
	}
}

/*
=======================
idBotGoalManager::BotAddToAvoidGoals
=======================
*/
void idBotGoalManager::BotRemoveFromAvoidGoals( int goalstate, int number )
{
	int i;
	bot_goalstate_t* gs;

	gs = BotGoalStateFromHandle( goalstate );
	if( !gs )
	{
		return;
	}

	//don't use the goals the bot wants to avoid
	for( i = 0; i < MAX_AVOIDGOALS; i++ )
	{
		if( gs->avoidgoals[i] == number && gs->avoidgoaltimes[i] >= Bot_Time() )
		{
			gs->avoidgoaltimes[i] = 0;
			return;
		}
	}
}

/*
=======================
idBotGoalManager::BotAvoidGoalTime
=======================
*/
float idBotGoalManager::BotAvoidGoalTime( int goalstate, int number )
{
	int i;
	bot_goalstate_t* gs;

	gs = BotGoalStateFromHandle( goalstate );
	if( !gs )
	{
		return 0;
	}

	//don't use the goals the bot wants to avoid
	for( i = 0; i < MAX_AVOIDGOALS; i++ )
	{
		if( gs->avoidgoals[i] == number && gs->avoidgoaltimes[i] >= Bot_Time() )
		{
			return gs->avoidgoaltimes[i] - Bot_Time();
		}
	}
	return 0;
}

/*
=======================
idBotGoalManager::BotSetAvoidGoalTime
=======================
*/
void idBotGoalManager::BotSetAvoidGoalTime( int goalstate, int number, float avoidtime )
{
	bot_goalstate_t* gs;
	levelitem_t* li;

	gs = BotGoalStateFromHandle( goalstate );
	if( !gs )
	{
		return;
	}
	if( avoidtime < 0 )
	{
		if( !itemconfig )
		{
			return;
		}

		for( li = levelitems; li; li = li->next )
		{
			if( li->number == number )
			{
				avoidtime = itemconfig->iteminfo[li->iteminfo].respawntime;
				if( !avoidtime )
				{
					avoidtime = AVOID_DEFAULT_TIME;
				}
				if( avoidtime < AVOID_MINIMUM_TIME )
				{
					avoidtime = AVOID_MINIMUM_TIME;
				}
				BotAddToAvoidGoals( gs, number, avoidtime );
				return;
			}
		}
		return;
	}
	else
	{
		BotAddToAvoidGoals( gs, number, avoidtime );
	}
}

/*
=======================
idBotGoalManager::BotGetLevelItemGoal
=======================
*/
int idBotGoalManager::BotGetLevelItemGoal( int index, char* name, bot_goal_t* goal )
{
	levelitem_t* li;

	if( !itemconfig )
	{
		return -1;
	}
	li = levelitems;
	if( index >= 0 )
	{
		for( ; li; li = li->next )
		{
			if( li->number == index )
			{
				li = li->next;
				break;
			}
		}
	}

	for( ; li; li = li->next )
	{
		if( gameLocal.gameType == GAME_SP )
		{
			if( li->flags & IFL_NOTSINGLE )
			{
				continue;
			}
		}
		else if( gameLocal.gameType >= GAME_TDM )
		{
			if( li->flags & IFL_NOTTEAM )
			{
				continue;
			}
		}
		else
		{
			if( li->flags & IFL_NOTFREE )
			{
				continue;
			}
		}
		if( li->flags & IFL_NOTBOT )
		{
			continue;
		}

		if( itemconfig->iteminfo[li->iteminfo].name == name )
		{
//			goal->areanum = li->goalareanum;
			goal->origin = li->goalorigin;
			goal->entitynum = li->item->entityNumber;
			goal->mins = itemconfig->iteminfo[li->iteminfo].mins;
			goal->maxs = itemconfig->iteminfo[li->iteminfo].maxs;
			goal->number = li->number;
			goal->flags = GFL_ITEM;
			if( li->timeout )
			{
				goal->flags |= GFL_DROPPED;
			}
			//G_Printf( "found li %s\n", itemconfig->iteminfo[li->iteminfo].name);
			return li->number;
		}
	}
	return -1;
}

/*
=========================
idBotGoalManager::BotGetMapLocationGoal
=========================
*/
int idBotGoalManager::BotGetMapLocationGoal( char* name, bot_goal_t* goal )
{
	maplocation_t* ml;
	idVec3 mins( -8, -8, -8 );
	idVec3 maxs( 8, 8, 8 );

	for( int i = 0; i < maplocations.Num(); i++ )
	{
		ml = &maplocations[i];
		if( ml->name == name )
		{
			goal->areanum = ml->areanum;
			VectorCopy( ml->origin, goal->origin );
			goal->entitynum = 0;
			VectorCopy( mins, goal->mins );
			VectorCopy( maxs, goal->maxs );
			return true;
		}
	}
	return false;
}

/*
=========================
idBotGoalManager::BotGetNextCampSpotGoal
=========================
*/
int idBotGoalManager::BotGetNextCampSpotGoal( int num, bot_goal_t* goal )
{
	int i;
	campspot_t* cs;
	idVec3 mins( -8, -8, -8 );
	idVec3 maxs( 8, 8, 8 );

	if( num < 0 )
	{
		num = 0;
	}
	i = num;
	for( int d = 0; d < campspots.Num(); d++ )
	{
		cs = &campspots[d];
		if( --i < 0 )
		{
			goal->areanum = cs->areanum;
			VectorCopy( cs->origin, goal->origin );
			goal->entitynum = 0;
			VectorCopy( mins, goal->mins );
			VectorCopy( maxs, goal->maxs );
			return num + 1;
		}
	}
	return 0;
}

/*
============================
idBotGoalManager::BotFindEntityForLevelItem
============================
*/
void idBotGoalManager::BotFindEntityForLevelItem( levelitem_t* li )
{
	int ent, modelindex;
	itemconfig_t* ic;
	//aas_entityinfo_t entinfo;
	idVec3 dir;

	ic = itemconfig;
	if( !itemconfig )
	{
		return;
	}
	for( int idx = 0; idx < gameLocal.num_entities; idx++ )
	{
		idItem* ent = gameLocal.entities[idx]->Cast<idItem>();

		if( ent == NULL )
		{
			continue;
		}

		//get the model index of the entity
		modelindex = ent->GetModelIndex();

		if( !modelindex )
		{
			continue;
		}

		// jmarshall - eval moving objects
		//get info about the entity
		//AAS_EntityInfo(ent, &entinfo);
		//if the entity is still moving
		//if (entinfo.origin[0] != entinfo.lastvisorigin[0] ||
		//	entinfo.origin[1] != entinfo.lastvisorigin[1] ||
		//	entinfo.origin[2] != entinfo.lastvisorigin[2]) continue;
		//
		// jmarshall end
		if( ic->iteminfo[li->iteminfo].modelindex == modelindex )
		{
			//check if the entity is very close
			idVec3 dir = li->origin - ent->GetPhysics()->GetOrigin();

			if( dir.Length() < 30 )
			{
				//found an entity for this level item
				li->item = ent;
			}
		}
	}
}

/*
========================
UpdateEntityItems
========================
*/

//NOTE: enum entityType_t in bg_public.h
#define ET_ITEM			2

void idBotGoalManager::UpdateEntityItems( void )
{
	int i, modelindex;
	idVec3 dir;
	levelitem_t* li, * nextli;
	itemconfig_t* ic;

	//timeout current entity items if necessary
	for( li = levelitems; li; li = nextli )
	{
		nextli = li->next;
		//if it is a item that will time out
		if( li->timeout )
		{
			//timeout the item
			if( li->timeout < Bot_Time() )
			{
				RemoveLevelItemFromList( li );
				FreeLevelItem( li );
			}
		}
	}

	//find new entity items
	ic = itemconfig;
	if( !itemconfig )
	{
		return;
	}
	//
	for( int idx = 0; idx < gameLocal.num_entities; idx++ )
	{
		idItem* ent = gameLocal.entities[idx]->Cast<idItem>();

		if( ent == 0 )
		{
			continue;
		}

		//get the model index of the entity
		modelindex = ent->GetModelIndex();

		if( !modelindex )
		{
			continue;
		}
		// jmarshall - eval floating items.
		////get info about the entity
		//AAS_EntityInfo(ent, &entinfo);
		////FIXME: don't do this
		////skip all floating items for now
		////if (entinfo.groundent != ENTITYNUM_WORLD) continue;
		////if the entity is still moving
		//if (entinfo.origin[0] != entinfo.lastvisorigin[0] ||
		//	entinfo.origin[1] != entinfo.lastvisorigin[1] ||
		//	entinfo.origin[2] != entinfo.lastvisorigin[2]) continue;
		// jmarshall end

		//check if the entity is already stored as a level item
		for( li = levelitems; li; li = li->next )
		{
			//if the level item is linked to an entity
			if( li->item && li->item == ent )
			{
				//the entity is re-used if the models are different
				if( ic->iteminfo[li->iteminfo].modelindex != modelindex )
				{
					//remove this level item
					RemoveLevelItemFromList( li );
					FreeLevelItem( li );
					li = NULL;
					break;
				}
				else
				{
					//if (entinfo.origin[0] != li->origin[0] ||
					//	entinfo.origin[1] != li->origin[1] ||
					//	entinfo.origin[2] != li->origin[2])
					//{
					//	VectorCopy(entinfo.origin, li->origin);
					//	//also update the goal area number
					//	li->goalareanum = AAS_BestReachableArea(li->origin,
					//		ic->iteminfo[li->iteminfo].mins, ic->iteminfo[li->iteminfo].maxs,
					//		li->goalorigin);
					//} //end if
					li->origin = ent->GetPhysics()->GetOrigin();
					break;
				}
			}
		}
		if( li )
		{
			continue;
		}

		//try to link the entity to a level item
		for( li = levelitems; li; li = li->next )
		{
			//if this level item is already linked
			if( li->item )
			{
				continue;
			}
			//
			if( gameLocal.gameType == GAME_SP )
			{
				if( li->flags & IFL_NOTSINGLE )
				{
					continue;
				}
			}
			else if( gameLocal.gameType >= GAME_TDM )
			{
				if( li->flags & IFL_NOTTEAM )
				{
					continue;
				}
			}
			else
			{
				if( li->flags & IFL_NOTFREE )
				{
					continue;
				}
			}

			//if the model of the level item and the entity are the same
			if( ic->iteminfo[li->iteminfo].modelindex == modelindex )
			{
				//check if the entity is very close
				dir = li->origin - ent->GetPhysics()->GetOrigin();
				if( dir.Length() < 30 )
				{
					//found an entity for this level item
					li->item = ent;
					//if the origin is different
					//if (entinfo.origin[0] != li->origin[0] ||
					//	entinfo.origin[1] != li->origin[1] ||
					//	entinfo.origin[2] != li->origin[2])
					//{
					//	//update the level item origin
					//	VectorCopy(ent->r.currentOrigin, li->origin);
					//	//also update the goal area number
					//	li->goalareanum = AAS_BestReachableArea(li->origin,
					//		ic->iteminfo[li->iteminfo].mins, ic->iteminfo[li->iteminfo].maxs,
					//		li->goalorigin);
					//} //end if

					//update the level item origin
					//VectorCopy(ent->r.currentOrigin, li->origin);
					li->origin = ent->GetPhysics()->GetOrigin();
					break;
				}
			}
		}

		if( li )
		{
			continue;
		}
		//check if the model is from a known item
		for( i = 0; i < ic->numiteminfo; i++ )
		{
			if( ic->iteminfo[i].modelindex == modelindex )
			{
				break;
			}
		}

		//if the model is not from a known item
		if( i >= ic->numiteminfo )
		{
			continue;
		}
		//allocate a new level item
		li = AllocLevelItem();

		if( !li )
		{
			continue;
		}

		//entity number of the level item
		li->item = ent;

		//number for the level item
		li->number = ent->entityNumber;

		//set the item info index for the level item
		li->iteminfo = i;

		//origin of the item
		//VectorCopy(ent->r.currentOrigin, li->origin);
		li->origin = ent->GetPhysics()->GetOrigin();

		// jmarshall - fix this, bots, jump pads, and droppable items, bad combo.
		//get the item goal area and goal origin
		//li->goalareanum = AAS_BestReachableArea(li->origin,
		//	ic->iteminfo[i].mins, ic->iteminfo[i].maxs,
		//	li->goalorigin);
		////never go for items dropped into jumppads
		//if (AAS_AreaJumpPad(li->goalareanum))
		//{
		//	FreeLevelItem(li);
		//	continue;
		//} //end if
		// jmarshall end
		//time this item out after 30 seconds
		//dropped items disappear after 30 seconds
		li->timeout = Bot_Time() + 30;
		//add the level item to the list
		AddLevelItemToList( li );
		//G_Printf( "found new level item %s\n", ic->iteminfo[i].classname);
	}
}

/*
====================
idBotGoalManager::BotDumpGoalStack
====================
*/
void idBotGoalManager::BotDumpGoalStack( int goalstate )
{
	int i;
	bot_goalstate_t* gs;
	char name[32];

	gs = BotGoalStateFromHandle( goalstate );
	if( !gs )
	{
		return;
	}
	for( i = 1; i <= gs->goalstacktop; i++ )
	{
		BotGoalName( gs->goalstack[i].number, name, 32 );
		common->Printf( "%d: %s", i, name );
	}
}

/*
====================
BotPushGoal
====================
*/
void idBotGoalManager::BotPushGoal( int goalstate, bot_goal_t* goal )
{
	bot_goalstate_t* gs;

	gs = BotGoalStateFromHandle( goalstate );
	if( !gs )
	{
		return;
	}
	if( gs->goalstacktop >= MAX_GOALSTACK - 1 )
	{
		gameLocal.Error( "goal heap overflow\n" );
		BotDumpGoalStack( goalstate );
		return;
	}
	gs->goalstacktop++;
	gs->goalstack[gs->goalstacktop] = *goal;
}

/*
====================
idBotGoalManager::BotPopGoal
====================
*/
void idBotGoalManager::BotPopGoal( int goalstate )
{
	bot_goalstate_t* gs;

	gs = BotGoalStateFromHandle( goalstate );
	if( !gs )
	{
		return;
	}
	if( gs->goalstacktop > 0 )
	{
		gs->goalstacktop--;
	}
}

/*
====================
idBotGoalManager::BotEmptyGoalStack
====================
*/
void idBotGoalManager::BotEmptyGoalStack( int goalstate )
{
	bot_goalstate_t* gs;

	gs = BotGoalStateFromHandle( goalstate );
	if( !gs )
	{
		return;
	}
	gs->goalstacktop = 0;
}

/*
====================
idBotGoalManager::BotGetTopGoal
====================
*/
int idBotGoalManager::BotGetTopGoal( int goalstate, bot_goal_t* goal )
{
	bot_goalstate_t* gs;

	gs = BotGoalStateFromHandle( goalstate );
	if( !gs )
	{
		return false;
	}
	if( !gs->goalstacktop )
	{
		return false;
	}
	memcpy( goal, &gs->goalstack[gs->goalstacktop], sizeof( bot_goal_t ) );
	return true;
}

/*
====================
idBotGoalManager::BotGetSecondGoal
====================
*/
int idBotGoalManager::BotGetSecondGoal( int goalstate, bot_goal_t* goal )
{
	bot_goalstate_t* gs;

	gs = BotGoalStateFromHandle( goalstate );
	if( !gs )
	{
		return false;
	}
	if( gs->goalstacktop <= 1 )
	{
		return false;
	}
	memcpy( goal, &gs->goalstack[gs->goalstacktop - 1], sizeof( bot_goal_t ) );
	return true;
}

/*
====================
idBotGoalManager::BotChooseLTGItem
====================
*/
int idBotGoalManager::BotChooseLTGItem( int goalstate, idVec3 origin, int* inventory, int travelflags )
{
	int t, weightnum;
	float weight, bestweight, avoidtime;
	iteminfo_t* iteminfo;
	itemconfig_t* ic;
	levelitem_t* li, * bestitem;
	bot_goal_t goal;
	bot_goalstate_t* gs;

	gs = BotGoalStateFromHandle( goalstate );
	if( !gs )
	{
		return false;
	}
	if( !gs->itemweightconfig )
	{
		return false;
	}

	//get the area the bot is in
	//areanum = BotReachabilityArea(origin, gs->client);
	//if the bot is in solid or if the area the bot is in has no reachability links
	//if (!areanum || !AAS_AreaReachability(areanum))
	//{
	//	//use the last valid area the bot was in
	//	areanum = gs->lastreachabilityarea;
	//} //end if
	//remember the last area with reachabilities the bot was in
	//gs->lastreachabilityarea = areanum;
	//if still in solid
	//if (!areanum)
	//	return qfalse;
	//the item configuration
	ic = itemconfig;
	if( !itemconfig )
	{
		return false;
	}
	//best weight and item so far
	bestweight = 0;
	bestitem = NULL;
	memset( &goal, 0, sizeof( bot_goal_t ) );
	//go through the items in the level
	for( li = levelitems; li; li = li->next )
	{
		if( gameLocal.gameType == GAME_SP )
		{
			if( li->flags & IFL_NOTSINGLE )
			{
				continue;
			}
		}
		else if( gameLocal.gameType >= GAME_TDM )
		{
			if( li->flags & IFL_NOTTEAM )
			{
				continue;
			}
		}
		else
		{
			if( li->flags & IFL_NOTFREE )
			{
				continue;
			}
		}
		if( li->flags & IFL_NOTBOT )
		{
			continue;
		}
		//if the item is not in a possible goal area
		//if (!li->goalareanum)
		//	continue;
		//FIXME: is this a good thing? added this for items that never spawned into the game (f.i. CTF flags in obelisk)
		if( !li->item && !( li->flags & IFL_ROAM ) )
		{
			continue;
		}
		//get the fuzzy weight function for this item
		iteminfo = &ic->iteminfo[li->iteminfo];
		weightnum = gs->itemweightindex[iteminfo->number];
		if( weightnum < 0 )
		{
			continue;
		}

#ifdef UNDECIDEDFUZZY
		weight = FuzzyWeightUndecided( inventory, gs->itemweightconfig, weightnum );
#else
		weight = botFuzzyWeightManager.FuzzyWeight( inventory, gs->itemweightconfig, weightnum );
#endif //UNDECIDEDFUZZY
#ifdef DROPPEDWEIGHT
		//HACK: to make dropped items more attractive
		if( li->timeout )
		{
			weight += bot_droppedweight.GetFloat();    //GetValueFromLibVar(droppedweight);
		}
#endif //DROPPEDWEIGHT
		//use weight scale for item_botroam
		if( li->flags & IFL_ROAM )
		{
			weight *= li->weight;
		}
		//
		if( weight > 0 )
		{
			//get the travel time towards the goal area
// jmarshall
			//t = AAS_AreaTravelTimeToGoalArea(areanum, origin, li->goalareanum, travelflags);
			t = gameLocal.TravelTimeToGoal( origin, li->goalorigin );
			// jmarshall end

			//if the goal is reachable
			if( t > 0 )
			{
				//if this item won't respawn before we get there
				avoidtime = BotAvoidGoalTime( goalstate, li->number );
				if( avoidtime - t * 0.009 > 0 )
				{
					continue;
				}

				weight /= ( float )t * TRAVELTIME_SCALE;

				if( weight > bestweight )
				{
					bestweight = weight;
					bestitem = li;
				}
			}
		}
	}

	//if no goal item found
	if( !bestitem )
	{
		/*
		//if not in lava or slime
		if (!AAS_AreaLava(areanum) && !AAS_AreaSlime(areanum))
		{
			if (AAS_RandomGoalArea(areanum, travelflags, &goal.areanum, goal.origin))
			{
				VectorSet(goal.mins, -15, -15, -15);
				VectorSet(goal.maxs, 15, 15, 15);
				goal.entitynum = 0;
				goal.number = 0;
				goal.flags = GFL_ROAM;
				goal.iteminfo = 0;
				//push the goal on the stack
				BotPushGoal(goalstate, &goal);
				//
		#ifdef DEBUG
				G_Printf( "chosen roam goal area %d\n", goal.areanum);
		#endif //DEBUG
				return qtrue;
			} //end if
		} //end if
		*/
		return false;
	}

	//create a bot goal for this item
	iteminfo = &ic->iteminfo[bestitem->iteminfo];
	VectorCopy( bestitem->goalorigin, goal.origin );
	VectorCopy( iteminfo->mins, goal.mins );
	VectorCopy( iteminfo->maxs, goal.maxs );
//	goal.areanum = bestitem->goalareanum;
	goal.entitynum = bestitem->item->entityNumber;
	goal.number = bestitem->number;
	goal.flags = GFL_ITEM;
	if( bestitem->timeout )
	{
		goal.flags |= GFL_DROPPED;
	}
	if( bestitem->flags & IFL_ROAM )
	{
		goal.flags |= GFL_ROAM;
	}
	goal.iteminfo = bestitem->iteminfo;
	//if it's a dropped item
	if( bestitem->timeout )
	{
		avoidtime = AVOID_DROPPED_TIME;
	}
	else
	{
		avoidtime = iteminfo->respawntime;
		if( !avoidtime )
		{
			avoidtime = AVOID_DEFAULT_TIME;
		}
		if( avoidtime < AVOID_MINIMUM_TIME )
		{
			avoidtime = AVOID_MINIMUM_TIME;
		}
	}

	//add the chosen goal to the goals to avoid for a while
	BotAddToAvoidGoals( gs, bestitem->number, avoidtime );

	//push the goal on the stack
	BotPushGoal( goalstate, &goal );

	return true;
}

/*
====================
idBotGoalManager::BotChooseNBGItem
====================
*/
int idBotGoalManager::BotChooseNBGItem( int goalstate, idVec3 origin, int* inventory, int travelflags, bot_goal_t* ltg, float maxtime )
{
	int areanum, t, weightnum, ltg_time;
	float weight, bestweight, avoidtime;
	iteminfo_t* iteminfo;
	itemconfig_t* ic;
	levelitem_t* li, * bestitem;
	bot_goal_t goal;
	bot_goalstate_t* gs;

	gs = BotGoalStateFromHandle( goalstate );
	if( !gs )
	{
		return false;
	}
	if( !gs->itemweightconfig )
	{
		return false;
	}
	//get the area the bot is in
// jmarshall
	areanum = 1; // BotReachabilityArea(origin, gs->client);
// jmarshall end
	//if the bot is in solid or if the area the bot is in has no reachability links
	//if (!areanum || !AAS_AreaReachability(areanum))
	//{
	//	//use the last valid area the bot was in
	//	areanum = gs->lastreachabilityarea;
	//} //end if
	//remember the last area with reachabilities the bot was in
	gs->lastreachabilityarea = areanum;
	//if still in solid
	if( !areanum )
	{
		return false;
	}
	// jmarshall
	if( ltg )
	{
		//ltg_time = AAS_AreaTravelTimeToGoalArea(areanum, origin, ltg->areanum, travelflags);
		ltg_time = gameLocal.TravelTimeToGoal( origin, ltg->origin );
	}
	else
	{
		ltg_time = 99999;
	}
	// jmarshall end
	//the item configuration
	ic = itemconfig;
	if( !itemconfig )
	{
		return false;
	}
	//best weight and item so far
	bestweight = 0;
	bestitem = NULL;
	memset( &goal, 0, sizeof( bot_goal_t ) );
	//go through the items in the level
	for( li = levelitems; li; li = li->next )
	{
		if( gameLocal.gameType == GAME_SP )
		{
			if( li->flags & IFL_NOTSINGLE )
			{
				continue;
			}
		}
		else if( gameLocal.gameType >= GAME_TDM )
		{
			if( li->flags & IFL_NOTTEAM )
			{
				continue;
			}
		}
		else
		{
			if( li->flags & IFL_NOTFREE )
			{
				continue;
			}
		}
		if( li->flags & IFL_NOTBOT )
		{
			continue;
		}
		//if the item is in a possible goal area
//		if (!li->goalareanum)
//			continue;
		//FIXME: is this a good thing? added this for items that never spawned into the game (f.i. CTF flags in obelisk)
		if( !li->item && !( li->flags & IFL_ROAM ) )
		{
			continue;
		}
		//get the fuzzy weight function for this item
		iteminfo = &ic->iteminfo[li->iteminfo];
		weightnum = gs->itemweightindex[iteminfo->number];
		if( weightnum < 0 )
		{
			continue;
		}
		//
#ifdef UNDECIDEDFUZZY
		weight = FuzzyWeightUndecided( inventory, gs->itemweightconfig, weightnum );
#else
		weight = botFuzzyWeightManager.FuzzyWeight( inventory, gs->itemweightconfig, weightnum );
#endif //UNDECIDEDFUZZY
#ifdef DROPPEDWEIGHT
		//HACK: to make dropped items more attractive
		if( li->timeout )
		{
			weight += bot_droppedweight.GetFloat();
		}
#endif //DROPPEDWEIGHT
		//use weight scale for item_botroam
		if( li->flags & IFL_ROAM )
		{
			weight *= li->weight;
		}
		//
		if( weight > 0 )
		{
			//get the travel time towards the goal area
			//t = AAS_AreaTravelTimeToGoalArea(areanum, origin, li->goalareanum, travelflags);
			t = gameLocal.TravelTimeToGoal( origin, li->goalorigin );

			//if the goal is reachable
			if( t > 0 && t < maxtime )
			{
				//if this item won't respawn before we get there
				avoidtime = BotAvoidGoalTime( goalstate, li->number );
				if( avoidtime - t * 0.009 > 0 )
				{
					continue;
				}
				//
				weight /= ( float )t * TRAVELTIME_SCALE;
				//
				if( weight > bestweight )
				{
					t = 0;
					if( ltg && !li->timeout )
					{
						//get the travel time from the goal to the long term goal
// jmarshall
						//t = AAS_AreaTravelTimeToGoalArea(li->goalareanum, li->goalorigin, ltg->areanum, travelflags);
						t = gameLocal.TravelTimeToGoal( li->goalorigin, ltg->origin );
						// jmarshall end
					} //end if
					//if the travel back is possible and doesn't take too long
					if( t <= ltg_time )
					{
						bestweight = weight;
						bestitem = li;
					} //end if
				} //end if
			} //end if
		} //end if
	} //end for
	//if no goal item found
	if( !bestitem )
	{
		return false;
	}
	//create a bot goal for this item
	iteminfo = &ic->iteminfo[bestitem->iteminfo];
	VectorCopy( bestitem->goalorigin, goal.origin );
	VectorCopy( iteminfo->mins, goal.mins );
	VectorCopy( iteminfo->maxs, goal.maxs );
//	goal.areanum = bestitem->goalareanum;
	goal.entitynum = bestitem->item->entityNumber;
	goal.number = bestitem->number;
	goal.flags = GFL_ITEM;
	if( bestitem->timeout )
	{
		goal.flags |= GFL_DROPPED;
	}
	if( bestitem->flags & IFL_ROAM )
	{
		goal.flags |= GFL_ROAM;
	}
	goal.iteminfo = bestitem->iteminfo;
	//if it's a dropped item
	if( bestitem->timeout )
	{
		avoidtime = AVOID_DROPPED_TIME;
	} //end if
	else
	{
		avoidtime = iteminfo->respawntime;
		if( !avoidtime )
		{
			avoidtime = AVOID_DEFAULT_TIME;
		}
		if( avoidtime < AVOID_MINIMUM_TIME )
		{
			avoidtime = AVOID_MINIMUM_TIME;
		}
	} //end else
	//add the chosen goal to the goals to avoid for a while
	BotAddToAvoidGoals( gs, bestitem->number, avoidtime );
	//push the goal on the stack
	BotPushGoal( goalstate, &goal );
	//
	return true;
} //end of the function BotChooseNBGItem



/*
====================
idBotGoalManager::BotTouchingGoal
====================
*/
int idBotGoalManager::BotTouchingGoal( idVec3 origin, bot_goal_t* goal )
{
	int i;
	idVec3 boxmins, boxmaxs;
	idVec3 absmins, absmaxs;
	idVec3 safety_maxs( 0, 0, 0 ); //{4, 4, 10};
	idVec3 safety_mins( 0, 0, 0 ); //{-4, -4, 0};

	rvmBot::PresenceTypeBoundingBox( PRESENCE_NORMAL, boxmins, boxmaxs );
	VectorSubtract( goal->mins, boxmaxs, absmins );
	VectorSubtract( goal->maxs, boxmins, absmaxs );
	VectorAdd( absmins, goal->origin, absmins );
	VectorAdd( absmaxs, goal->origin, absmaxs );

	//make the box a little smaller for safety
	VectorSubtract( absmaxs, safety_maxs, absmaxs );
	VectorSubtract( absmins, safety_mins, absmins );

	for( i = 0; i < 3; i++ )
	{
		if( origin[i] < absmins[i] || origin[i] > absmaxs[i] )
		{
			return false;
		}
	}
	return true;
}

/*
========================
idBotGoalManager::BotItemGoalInVisButNotVisible
========================
*/
int idBotGoalManager::BotItemGoalInVisButNotVisible( int viewer, idVec3 eye, idAngles viewangles, bot_goal_t* goal )
{
	//aas_entityinfo_t entinfo;
	trace_t trace;
	idVec3 middle;

	if( !( goal->flags & GFL_ITEM ) )
	{
		return false;
	}

	VectorAdd( goal->mins, goal->mins, middle );
	VectorScale( middle, 0.5, middle );
	VectorAdd( goal->origin, middle, middle );

	//trap_Trace(&trace, eye, NULL, NULL, middle, viewer, CONTENTS_SOLID);
	idMat3 axis;
	axis.Identity();
	gameLocal.clip[0]->Translation( trace, eye, middle, NULL, axis, CONTENTS_SOLID, NULL );

	//if the goal middle point is visible
	if( trace.fraction >= 1 )
	{
		//the goal entity number doesn't have to be valid
		//just assume it's valid
		if( goal->entitynum <= 0 )
		{
			return false;
		}
		// jmarshall - time delay thing here?
		//
		//if the entity data isn't valid
		//AAS_EntityInfo(goal->entitynum, &entinfo);
		//NOTE: for some wacko reason entities are sometimes
		// not updated
		//if (!entinfo.valid) return qtrue;
		//if (entinfo.ltime < AAS_Time() - 0.5)
		//	return qtrue;
		// jmarshall end
	}
	return false;
}

/*
========================
BotResetGoalState
========================
*/
void idBotGoalManager::BotResetGoalState( int goalstate )
{
	bot_goalstate_t* gs;

	gs = BotGoalStateFromHandle( goalstate );
	if( !gs )
	{
		return;
	}
	//memset(gs->goalstack, 0, MAX_GOALSTACK * sizeof(bot_goal_t));
	for( int i = 0; i < MAX_GOALSTACK; i++ )
	{
		gs->goalstack[i].Reset();
	}
	gs->goalstacktop = 0;
	BotResetAvoidGoals( goalstate );
}

/*
========================
idBotGoalManager::BotResetGoalState
========================
*/
int idBotGoalManager::BotLoadItemWeights( int goalstate, char* filename )
{
	bot_goalstate_t* gs;

	gs = BotGoalStateFromHandle( goalstate );
	if( !gs )
	{
		return BLERR_CANNOTLOADITEMWEIGHTS;
	}

	//load the weight configuration
	gs->itemweightconfig = botFuzzyWeightManager.ReadWeightConfig( filename );
	if( !gs->itemweightconfig )
	{
		gameLocal.Error( "couldn't load weights\n" );
		return BLERR_CANNOTLOADITEMWEIGHTS;
	}

	//if there's no item configuration
	if( !itemconfig )
	{
		return BLERR_CANNOTLOADITEMWEIGHTS;
	}

	//create the item weight index
	gs->itemweightindex = ItemWeightIndex( gs->itemweightconfig, itemconfig );

	//everything went ok
	return BLERR_NOERROR;
}

/*
========================
idBotGoalManager::BotResetGoalState
========================
*/
void idBotGoalManager::BotFreeItemWeights( int goalstate )
{
	bot_goalstate_t* gs;

	gs = BotGoalStateFromHandle( goalstate );
	if( !gs )
	{
		return;
	}
	if( gs->itemweightconfig )
	{
		botFuzzyWeightManager.FreeWeightConfig( gs->itemweightconfig );
	}
	// jmarshall - eval
	//if (gs->itemweightindex)
	//	FreeMemory(gs->itemweightindex);
	// jmarshall end
}

/*
========================
idBotGoalManager::BotAllocGoalState
========================
*/
int idBotGoalManager::BotAllocGoalState( int client )
{
	int i;

	for( i = 1; i <= MAX_CLIENTS; i++ )
	{
		if( !botgoalstates[i].InUse() )
		{
			botgoalstates[i].Reset();
			botgoalstates[i].client = client;
			return i;
		}
	}
	return 0;
}

/*
========================
idBotGoalManager::BotFreeGoalState
========================
*/
void idBotGoalManager::BotFreeGoalState( int handle )
{
	if( handle <= 0 || handle > MAX_CLIENTS )
	{
		gameLocal.Error( "goal state handle %d out of range\n", handle );
		return;
	}

	if( !botgoalstates[handle].InUse() )
	{
		gameLocal.Error( "invalid goal state handle %d\n", handle );
		return;
	}

	BotFreeItemWeights( handle );
	// jmarshall
	//FreeMemory(botgoalstates[handle]);
	// jmarshall end
	botgoalstates[handle].Reset();
}

/*
========================
idBotGoalManager::BotSetupGoalAI
========================
*/
int idBotGoalManager::BotSetupGoalAI( void )
{
	//load the item configuration
	itemconfig = LoadItemConfig( ( char* )bot_itemsfile.GetString() );
	if( !itemconfig )
	{
		gameLocal.Error( "couldn't load item config\n" );
		return BLERR_CANNOTLOADITEMCONFIG;
	}

	//everything went ok
	return BLERR_NOERROR;
}

/*
========================
idBotGoalManager::BotSetupGoalAI
========================
*/
void idBotGoalManager::BotShutdownGoalAI( void )
{
	int i;
	// jmarshall
	//if (itemconfig)
	//	FreeMemory(itemconfig);
	// jmarshall end
	itemconfig = NULL;
	// jmarshall
	//if (levelitemheap)
	//	FreeMemory(levelitemheap);
	// jmarshall end
	//levelitemheap = NULL;
	for( int i = 0; i < MAX_BOT_LEVEL_ITEMS; i++ )
	{
		levelitemheap[i].Reset();
	}

	freelevelitems = NULL;
	levelitems = NULL;
	numlevelitems = 0;

	BotFreeInfoEntities();

	for( i = 1; i <= MAX_CLIENTS; i++ )
	{
		if( botgoalstates[i].InUse() )
		{
			BotFreeGoalState( i );
		}
	}
}


/*
=======================
idBotGoalManager::BotNearGoal(
=======================
*/
bool idBotGoalManager::BotNearGoal( idVec3 p1, idVec3 p2 )
{
	idVec3 p1_z, p2_z;

	p1_z[0] = p1[0];
	p1_z[1] = p1[1];
	p1_z[2] = 0;

	p2_z[0] = p2[0];
	p2_z[1] = p2[1];
	p2_z[2] = 0;

	float distToGoal = idMath::Distance( p1_z, p2_z );
	if( distToGoal <= 50 )
	{
		return true;
	}

	return false;
}
