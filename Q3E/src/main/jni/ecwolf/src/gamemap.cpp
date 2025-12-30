/*
** gamemap.cpp
**
**---------------------------------------------------------------------------
** Copyright 2011 Braden Obrzut
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
**
*/

#include <climits>
#include "id_ca.h"
#include "farchive.h"
#include "gamemap.h"
#include "tarray.h"
#include "w_wad.h"
#include "wl_def.h"
#include "lnspec.h"
#include "actor.h"
#include "thingdef/thingdef.h"
#include "wl_agent.h"
#include "wl_game.h"
#include "wl_play.h"
#include "r_sprites.h"
#include "resourcefiles/resourcefile.h"
#include "wl_loadsave.h"
#include "doomerrors.h"
#include "m_random.h"
#include "g_mapinfo.h"

const FName SpecialThingNames[SMT_NumThings] = {
	"$Player1Start"
};

GameMap::GameMap(const FString &map) : map(map), valid(false), isUWMF(false),
	file(NULL), zoneTraversed(NULL), zoneLinks(NULL)
{
	lumps[0] = NULL;

	// Find the map
	markerLump = Wads.CheckNumForName(map);

	// PK3 format maps
	FString mapWad;
	mapWad.Format("maps/%s.wad", map.GetChars());

	int wadLump = Wads.CheckNumForFullName(mapWad);
	if(wadLump > markerLump)
	{
		isWad = true;
		markerLump = wadLump;
	}
	else
		isWad = false;

	if(markerLump == -1)
	{
		FString error;
		error.Format("Could not find map %s!", map.GetChars());
		throw CRecoverableError(error);
	}

	// Hmm... What follows is some massive copy and paste, but I can't really
	// think of a cleaner way to do this.
	// Anyways, if we have a wad we need to open a resource file for it.
	// Otherwise we open the relevent lumps.
	if(isWad)
	{
		file = FResourceFile::OpenResourceFile(mapWad.GetChars(), Wads.ReopenLumpNum(markerLump), true);
		if(!file || file->LumpCount() < 2) // Maps must be 2 lumps in size
		{
			FString error;
			error.Format("Map %s is in an unknown format.", map.GetChars());
			throw CRecoverableError(error);
		}

		// First lump is assumed marker
		FResourceLump *lump = file->GetLump(1);
		if(stricmp(lump->Name, "PLANES") == 0)
		{
			numLumps = 1;
			isUWMF = false;
			valid = true;
			lumps[0] = lump->NewReader();
		}
		else
		{
			if(stricmp(lump->Name, "TEXTMAP") != 0)
			{
				FString error;
				error.Format("Invalid map format for %s!", map.GetChars());
				throw CRecoverableError(error);
			}

			isUWMF = true;
			lumps[0] = lump->NewReader();

			for(unsigned int i = 2;i < file->LumpCount();++i)
			{
				lump = file->GetLump(i);
				if(stricmp(lump->Name, "ENDMAP") == 0)
				{
					valid = true;
					break;
				}
				numLumps++;
			}
			if(!valid)
			{
				FString error;
				error.Format("ENDMAP not found for map %s!", map.GetChars());
				throw CRecoverableError(error);
			}
		}
	}
	else
	{
		const char* nextLumpName = Wads.GetLumpFullName(markerLump+1);
		if(nextLumpName == NULL || strcmp(nextLumpName, "TEXTMAP") != 0)
		{
			numLumps = 1;
			isUWMF = false;
			valid = true;
			if(nextLumpName != NULL && strcmp(nextLumpName, "PLANES") == 0)
			{
				// DOS (WDC) format binary map
				lumps[0] = Wads.ReopenLumpNum(markerLump+1);
			}
			else
			{
				// Must be a Mac format map
				lumps[0] = Wads.ReopenLumpNum(markerLump);
				if(lumps[0]->GetLength() <= 0)
				{
					FString error;
					error.Format("Invalid map format for %s!", map.GetChars());
					throw CRecoverableError(error);
				}
			}
			
		}
		else
		{
			// Expect UWMF formatted map.
			isUWMF = true;
			lumps[0] = Wads.ReopenLumpNum(markerLump+1);

			for(int i = 2;i < Wads.GetNumLumps();i++)
			{
				if(strcmp(Wads.GetLumpFullName(markerLump+i), "ENDMAP") == 0)
				{
					valid = true;
					break;
				}
				numLumps++;
			}
			if(!valid)
			{
				FString error;
				error.Format("ENDMAP not found for map %s!", map.GetChars());
				throw CRecoverableError(error);
			}
		}
	}
}

GameMap::~GameMap()
{
	delete lumps[0];
	if(isWad)
		delete file;

	for(unsigned int i = 0;i < planes.Size();++i)
		delete[] planes[i].map;
	UnloadLinks();
}

bool GameMap::ActivateTrigger(Trigger &trig, Trigger::Side direction, AActor *activator)
{
	if(!trig.repeatable && !trig.active)
		return false;

	MapSpot spot = GetSpot(trig.x, trig.y, trig.z);

	Specials::LineSpecialFunction func = Specials::LookupFunction(Specials::LineSpecials(trig.action));
	bool ret = func(spot, trig.arg, direction, activator) != 0;
	if(ret)
	{
		if(trig.active && trig.isSecret)
			++gamestate.secretcount;
		trig.active = false;
	}
	return ret;
}

void GameMap::ClearVisibility()
{
	for(unsigned int i = 0;i < header.width*header.height;++i)
	{
		for(unsigned int p = 0;p < planes.Size();++p)
			planes[p].map[i].visible = false;
	}
	if(players[ConsolePlayer].camera)
		GetSpot(players[ConsolePlayer].camera->tilex, players[ConsolePlayer].camera->tiley, 0)->visible = true;
}

bool GameMap::CheckMapExists(const FString &map)
{
	if (Wads.CheckNumForName(map) < 0)
		return false;
	try
	{
		GameMap gm(map);
		return true;
	}
	catch(CRecoverableError &)
	{
		return false;
	}
}

bool GameMap::CheckLink(const Zone *zone1, const Zone *zone2, bool recurse)
{
	if(zone1 == NULL || zone2 == NULL)
		return false;

	// We only have the top half of the table.
	if(zone2->index < zone1->index)
	{
		const Zone *tmp = zone1;
		zone1 = zone2;
		zone2 = tmp;
	}

	// If we're doing a recursive check and the straight check passes use that
	bool straightCheck = zoneLinks[zone1->index][zone2->index - zone1->index] > 0;
	if(!recurse || straightCheck)
		return straightCheck;

	memset(zoneTraversed, 0, sizeof(bool)*zonePalette.Size());
	return TraverseLink(zone1, zone2);
}
bool GameMap::TraverseLink(const Zone* src, const Zone* dest)
{
	// Mark this node as checked
	zoneTraversed[src->index] = true;

	// Check upper zones (right side of table)
	unsigned int ofs = src->index;
	unsigned int i = zonePalette.Size() - src->index;
	while(--i > 0)
	{
		if(!zoneTraversed[i + ofs] && zoneLinks[ofs][i] > 0)
		{
			if(i + ofs == dest->index || TraverseLink(&zonePalette[i + ofs], dest))
				return true;
		}
	}

	// Check lower zones (top side of table)
	while(i < src->index)
	{
		if(!zoneTraversed[i] && zoneLinks[i][ofs] > 0)
		{
			if(i == dest->index || TraverseLink(&zonePalette[i], dest))
				return true;
		}
		--ofs;
		++i;
	}
	return false;
}

// Get a list of textures to precache
void GameMap::GetHitlist(BYTE* hitlist) const
{
	R_GetSpriteHitlist(hitlist);

	for(unsigned int i = planes.Size();i-- > 0;)
	{
		Plane &plane = planes[i];
		for(unsigned int j = GetHeader().width*GetHeader().height;j-- > 0;)
		{
			Plane::Map &spot = plane.map[j];

			if(spot.tile)
			{
				hitlist[spot.tile->texture[Tile::East].GetIndex()] =
					hitlist[spot.tile->texture[Tile::North].GetIndex()] =
					hitlist[spot.tile->texture[Tile::West].GetIndex()] =
					hitlist[spot.tile->texture[Tile::South].GetIndex()] |= 1;
			}

			if(spot.sector)
			{
				hitlist[spot.sector->texture[Sector::Floor].GetIndex()] =
					hitlist[spot.sector->texture[Sector::Ceiling].GetIndex()] |= 2;
			}
		}
	}
}

// Looks up the MapSpot by tag number.  If spot is NULL then the first spot
// in the chain is returned.
// Technically at the moment further spots could be found by just using nexttag
// but the implementation details could change.
MapSpot GameMap::GetSpotByTag(unsigned int tag, MapSpot spot) const
{
	if(!spot)
	{
		const MapSpot *starttag = tagMap.CheckKey(tag);
		if(!starttag)
			return NULL;
		spot = *starttag;
	}
	else
		spot = spot->nexttag;

	return spot;
}

const GameMap::Tile *GameMap::GetTile(unsigned int index) const
{
	if(index > tilePalette.Size())
		return NULL;
	return &tilePalette[index];
}

unsigned int GameMap::GetTileIndex(const GameMap::Tile *tile) const
{
	if(!tile)
		return INT_MAX;

	return static_cast<unsigned int>(tile - &tilePalette[0]);
}

const GameMap::Sector *GameMap::GetSector(unsigned int index) const
{
	if(index > sectorPalette.Size())
		return NULL;
	return &sectorPalette[index];
}

unsigned int GameMap::GetSectorIndex(const GameMap::Sector *sector) const
{
	if(!sector)
		return INT_MAX;

	return static_cast<unsigned int>(sector - &sectorPalette[0]);
}

void GameMap::LinkZones(const Zone *zone1, const Zone *zone2, bool open)
{
	if(zone1 == zone2 || zone1 == NULL || zone2 == NULL)
		return;

	unsigned short &value = zone1->index < zone2->index ?
		zoneLinks[zone1->index][zone2->index - zone1->index] :
		zoneLinks[zone2->index][zone1->index - zone2->index];
	if(!open)
	{
		if(value > 0)
			--value;
	}
	else
		++value;
}

void GameMap::LoadMap(bool loadingSave)
{
	if(!valid)
		throw CRecoverableError("Tried to load invalid map!");

	if(isUWMF)
		ReadUWMFData();
	else
		ReadPlanesData();

	if(!loadingSave)
		ScanTiles();
}

GameMap::Plane &GameMap::NewPlane()
{
	planes.Reserve(1);
	Plane &newPlane = planes[planes.Size()-1];
	newPlane.gm = this;
	newPlane.map = new Plane::Map[header.width*header.height];
	for(unsigned int i = 0;i < header.width*header.height;++i)
		newPlane.map[i].plane = &newPlane;
	return newPlane;
}

GameMap::Trigger &GameMap::NewTrigger(unsigned int x, unsigned int y, unsigned int z)
{
	if(z >= planes.Size())
		throw CRecoverableError("Trigger assigned to non-existant plane!");

	MapSpot spot = GetSpot(x, y, z);
	Trigger newTrig;
	newTrig.x = x;
	newTrig.y = y;
	newTrig.z = z;
	spot->triggers.Push(newTrig);
	return spot->triggers[spot->triggers.Size()-1];
}

void GameMap::PropagateMark()
{
	for(unsigned int p = 0;p < NumPlanes();++p)
	{
		MapPlane &plane = planes[p];

		for(unsigned int i = 0;i < GetHeader().width*GetHeader().height;++i)
			GC::Mark(plane.map[i].thinker);
	}
}

// Look at data and determine if we need to set up any flags.
void GameMap::ScanTiles()
{
	for(unsigned int p = 0;p < planes.Size();++p)
	{
		MapSpot spot = planes[p].map;
		MapSpot endSpot = spot + header.width*header.height; 
		while(spot < endSpot)
		{
			if(spot->tile)
			{
				if(spot->tile->mapped > gamestate.difficulty->MapFilter)
					spot->amFlags |= AM_Visible;
				if(spot->tile->dontOverlay)
					spot->amFlags |= AM_DontOverlay;
			}

			++spot;
		}
	}
}

// Adds the spot to the tag list. The linked chain is stored in the tile itself.
void GameMap::SetSpotTag(MapSpot spot, unsigned int tag)
{
	spot->tag = tag;

	MapSpot *chainPtr = tagMap.CheckKey(tag);
	if(chainPtr)
	{
		MapSpot chain = *chainPtr;
		while(chain->nexttag)
			chain = chain->nexttag;
		chain->nexttag = spot;
	}
	else
		tagMap.Insert(tag, spot);
}

void GameMap::SetupLinks()
{
	const unsigned int zdSize = ((zonePalette.Size()*(zonePalette.Size()+1))>>1);
	zoneTraversed = new bool[zonePalette.Size()];
	memset(zoneTraversed, 0, zonePalette.Size() * sizeof (zoneTraversed[0]));
	zptrBack = new unsigned short[zdSize];
	memset(zptrBack, 0, sizeof(zptrBack[0]) * zdSize);	

	// Set up the table
	unsigned short* ptr = zptrBack;
	zoneLinks = new unsigned short* [zonePalette.Size()];
	for(unsigned int i = 0;i < zonePalette.Size();++i)
	{
		zoneLinks[i] = ptr;
		ptr += zonePalette.Size()-i;
		zoneLinks[i][0] = 1;
	}
}

extern FRandom pr_spawnmobj;
void GameMap::SpawnThings() const
{
#if 0
	// Debug code - Show the number of things spawned at map start.
	printf("Spawning %d things\n", things.Size());
#endif
	for(unsigned int i = 0;i < things.Size();++i)
	{
		Thing &thing = things[i];
		if(!thing.skill[gamestate.difficulty->SpawnFilter])
			continue;

		if(thing.type == SpecialThingNames[SMT_Player1Start])
			SpawnPlayer(thing.x>>FRACBITS, thing.y>>FRACBITS, thing.angle);
		else
		{
			static const ClassDef *unknownClass = ClassDef::FindClass("Unknown");
			// Spawn object
			const ClassDef *cls = ClassDef::FindClass(thing.type);
			if(cls == NULL)
			{
				cls = unknownClass;
				printf("Unknown thing %s @ (%d, %d)\n", thing.type.GetChars(), thing.x>>FRACBITS, thing.y>>FRACBITS);
			}

			AActor *actor = AActor::Spawn(cls, thing.x, thing.y, thing.z, SPAWN_AllowReplacement|(thing.patrol ? SPAWN_Patrol : 0));
			// This forumla helps us to avoid errors in roundoffs.
			actor->angle = (thing.angle/45)*ANGLE_45 + (thing.angle%45)*ANGLE_1;
			actor->dir = nodir;
			if(thing.ambush)
				actor->flags |= FL_AMBUSH;
			if(thing.patrol)
				actor->dir = dirtype(actor->angle/ANGLE_45);
			if(thing.holo)
				actor->flags &= ~(FL_SOLID);

			// Check for valid frames
			if(!actor->state || !R_CheckSpriteValid(actor->sprite))
			{
				actor->Destroy();
				actor = AActor::Spawn(unknownClass, thing.x, thing.y, thing.z, SPAWN_AllowReplacement);

				printf("%s at (%d, %d) has no frames\n", cls->GetName().GetChars(), thing.x>>FRACBITS, thing.y>>FRACBITS);
			}
		}
	}
}

void GameMap::UnloadLinks()
{
	// Make sure there's stuff to unload.
	if(!zoneLinks)
		return;

	// zoneTraversed holds the base address for our single allocation.
	delete[] zoneTraversed;
	delete[] zptrBack;
	delete[] zoneLinks;
	zoneTraversed = NULL;
	zoneLinks = NULL;
	zptrBack = NULL;
}

////////////////////////////////////////////////////////////////////////////////

unsigned int GameMap::Plane::Map::GetX() const
{
	return static_cast<unsigned int>(this - plane->map)%plane->gm->GetHeader().width;
}

unsigned int GameMap::Plane::Map::GetY() const
{
	return static_cast<unsigned int>(this - plane->map)/plane->gm->GetHeader().width;
}

MapSpot GameMap::Plane::Map::GetAdjacent(MapTile::Side dir, bool opposite) const
{
	if(opposite) // Rotate the dir 180 degrees.
		dir = MapTile::Side((dir+2)%4);

	unsigned int x = GetX();
	unsigned int y = GetY();
	switch(dir)
	{
		case MapTile::South:
			++y;
			break;
		case MapTile::North:
			--y;
			break;
		case MapTile::West:
			--x;
			break;
		case MapTile::East:
			++x;
			break;
	}
	if(y >= plane->gm->GetHeader().height || x >= plane->gm->GetHeader().width)
		return NULL;
	return &plane->map[y*plane->gm->GetHeader().width+x];
}

void GameMap::Plane::Map::SetTile(const MapTile *tile)
{
	this->tile = tile;
	for(unsigned int i = 0;i < 4;++i)
	{
		if(tile)
		{
			sideSolid[i] = tile->sideSolid[i];
			texture[i] = tile->texture[i];
		}
		else
		{
			sideSolid[i] = false;
			texture[i].SetInvalid();
		}
	}
}

FArchive &operator<< (FArchive &arc, GameMap *&gm)
{
	arc << gm->header.name
		<< gm->header.width
		<< gm->header.height
		<< gm->header.tileSize;

	// zoneLinks
	if(GameSave::SaveVersion >= 1383348286)
	{
		unsigned int zone = gm->zonePalette.Size();
		while(--zone > 0) // We don't care about == 0 since it's always 1
		{
			unsigned int i = gm->zonePalette.Size() - zone;
			while(--i > 0)
				arc << gm->zoneLinks[zone][i];
		}
	}
	else
	{
		// Old zoneLinks
		// It would probably be too much work to try to convert this, so we'll
		// just read past it and let the game be a little inconsistent.  Most
		// people won't notice and in most cases the level will fix itself after
		// some time elapses.
		uint32_t packing = 0;
		unsigned short shift = 0;
		unsigned int x = 0;
		unsigned int y = 1;
		unsigned int max = 1;

		arc << packing;

		do
		{
			//gm->zoneLinks[x][y] = (packing>>(shift++))&1;
			++shift;

			if(++x >= max)
			{
				x = 0;
				++y;
				++max;
			}

			if(shift == sizeof(packing)*8)
			{
				arc << packing;
				shift = 0;
			}
		}
		while(y < gm->zonePalette.Size());
	}

	// Serialize any map information that may change
	for(unsigned int p = 0;p < gm->NumPlanes();++p)
	{
		MapPlane &plane = gm->planes[p];

		arc << plane.depth;
		assert(plane.depth == 64);
		if(!arc.IsStoring())
			plane.gm = gm;

		for(unsigned int i = 0;i < gm->GetHeader().width*gm->GetHeader().height;++i)
		{
			BYTE pushdir = plane.map[i].pushDirection;
			arc << pushdir;
			plane.map[i].pushDirection = static_cast<MapTile::Side>(pushdir);

			arc << plane.map[i].texture[0] << plane.map[i].texture[1] << plane.map[i].texture[2] << plane.map[i].texture[3]
				<< plane.map[i].visible;
			if(GameSave::SaveVersion >= 1393719642)
				arc << plane.map[i].amFlags;
			arc << plane.map[i].thinker
				<< plane.map[i].slideAmount[0] << plane.map[i].slideAmount[1] << plane.map[i].slideAmount[2] << plane.map[i].slideAmount[3]
				<< plane.map[i].sideSolid[0] << plane.map[i].sideSolid[1] << plane.map[i].sideSolid[2] << plane.map[i].sideSolid[3]
				<< plane.map[i].triggers
				<< plane.map[i].pushAmount
				<< plane.map[i].tile
				<< plane.map[i].sector
				<< plane.map[i].zone
				<< plane.map[i].pushReceptor;

			if(GameSave::SaveProdVersion >= 0x001002FF && GameSave::SaveVersion >= 1375246092)
				arc << plane.map[i].slideStyle;

			if(!arc.IsStoring())
				plane.map[i].plane = &plane;
		}
	}

	// Current elevator positions.
	if(GameSave::SaveVersion > 1438232816)
	{
		if(arc.IsStoring())
		{
			unsigned int count = gm->elevatorPosition.CountUsed();
			arc << count;

			TMap<unsigned int, MapSpot>::Iterator iter(gm->elevatorPosition);
			TMap<unsigned int, MapSpot>::Pair *pair;
			while(iter.NextPair(pair))
			{
				DWORD key = pair->Key;
				arc << key << pair->Value;
			}
		}
		else
		{
			unsigned int count;
			arc << count;

			gm->elevatorPosition.Clear();
			while(count-- > 0)
			{
				DWORD key;
				MapSpot value;
				arc << key << value;

				gm->elevatorPosition[key] = value;
			}
		}
	}

	return arc;
}

////////////////////////////////////////////////////////////////////////////////

FArchive &operator<< (FArchive &arc, MapSpot &spot)
{
	if(arc.IsStoring())
	{
		unsigned int x = INT_MAX;
		unsigned int y = INT_MAX;
		if(spot)
		{
			x = spot->GetX();
			y = spot->GetY();
		}

		arc << x << y;
	}
	else
	{
		unsigned int x, y;
		arc << x << y;

		if(x == INT_MAX || y == INT_MAX)
			spot = NULL;
		else
			spot = map->GetSpot(x, y, 0);
	}

	return arc;
}

FArchive &operator<< (FArchive &arc, const MapSector *&sector)
{
	if(arc.IsStoring())
	{
		unsigned int index = map->GetSectorIndex(sector);
		arc << index;
	}
	else
	{
		unsigned int index;
		arc << index;

		sector = map->GetSector(index);
	}
	return arc;
}

FArchive &operator<< (FArchive &arc, const MapTile *&tile)
{
	if(arc.IsStoring())
	{
		unsigned int index = map->GetTileIndex(tile);
		arc << index;
	}
	else
	{
		unsigned int index;
		arc << index;

		tile = map->GetTile(index);
	}
	return arc;
}

FArchive &operator<< (FArchive &arc, const MapZone *&zone)
{
	if(arc.IsStoring())
	{
		unsigned int index;
		if(zone)
			index = zone->index;
		else
			index = INT_MAX;

		arc << index;
	}
	else
	{
		unsigned int index;
		arc << index;

		if(index != INT_MAX)
			zone = &map->GetZone(index);
		else
			zone = NULL;
	}

	return arc;
}

FArchive &operator<< (FArchive &arc, MapTrigger &trigger)
{
	arc << trigger.x
		<< trigger.y
		<< trigger.z
		<< trigger.active
		<< trigger.action
		<< trigger.activate[0] << trigger.activate[1] << trigger.activate[2] << trigger.activate[3]
		<< trigger.arg[0] << trigger.arg[1] << trigger.arg[2] << trigger.arg[3] << trigger.arg[4]
		<< trigger.playerUse
		<< trigger.playerCross
		<< trigger.monsterUse
		<< trigger.isSecret
		<< trigger.repeatable;

	return arc;
}
