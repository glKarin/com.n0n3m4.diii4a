/*
** gamemap_planes.cpp
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

#include "doomerrors.h"
#include "id_ca.h"
#include "g_mapinfo.h"
#include "gamemap.h"
#include "gamemap_common.h"
#include "lnspec.h"
#include "scanner.h"
#include "thingdef/thingdef.h"
#include "tmemory.h"
#include "w_wad.h"
#include "wl_game.h"
#include "wl_shade.h"

static const char* const FeatureFlagNames[] = {
	"globalmeta",
	"lightlevels",
	"planedepth",
	"zheights",
	NULL
};

class Xlat : public TextMapParser
{
public:
	enum
	{
		TF_PATHING = 1,
		TF_HOLOWALL = 2,
		TF_AMBUSH = 4
	};

	enum
	{
		TSF_ISELEVATOR = 1,
		TSF_ISTRIGGER = 2,

		// Only returned by TranslateThing to indicate that the thing object is valid.
		TSF_ISTHING = 0x80000000
	};

	enum EFeatureFlags
	{
		FF_GLOBALMETA = 1,
		FF_LIGHTLEVELS = 2,
		FF_PLANEDEPTH = 4,
		FF_ZHEIGHTS = 8
	};

	struct ThingXlat
	{
	public:
		static int SortCompare(const void *t1, const void *t2)
		{
			unsigned short old1 = static_cast<const ThingXlat *>(t1)->oldnum;
			unsigned short old2 = static_cast<const ThingXlat *>(t2)->oldnum;
			if(old1 < old2)
				return -1;
			else if(old1 > old2)
				return 1;
			return 0;
		}

		FName			type;
		uint32_t		flags;
		unsigned short	oldnum;
		unsigned char	angles;
		unsigned char	minskill;
	};

	struct ThingSpecialXlat
	{
		ThingSpecialXlat() : flags(0) {}

		MapTrigger templateTrigger;
		uint32_t flags;
	};

	struct ModZone
	{
	public:
		enum Type
		{
			AMBUSH,
			CHANGETRIGGER
		};

		Type			type;
		bool			fillZone;

		unsigned int	oldTrigger;
		MapTrigger		triggerTemplate;
	};

	Xlat() : lump(0)
	{
	}

	void LoadXlat(const FString &baseLumpName, const GameInfo::FStringStack *baseStack, bool included=false)
	{
		int lump = Wads.CheckNumForFullName(baseLumpName, true);
		if(lump == -1)
		{
			FString error;
			error.Format("Could not open map translator '%s'.", baseLumpName.GetChars());
			throw CRecoverableError(error);
		}

		if(!included)
		{
			if(this->lump == lump)
				return;
			this->lump = lump;
			FeatureFlags = static_cast<EFeatureFlags>(0);

			ClearTables();
		}

		Scanner sc(lump);

		while(sc.TokensLeft())
		{
			sc.MustGetToken(TK_Identifier);

			if(sc->str.CompareNoCase("tiles") == 0)
				LoadTilesTable(sc);
			else if(sc->str.CompareNoCase("things") == 0)
				LoadThingTable(sc);
			else if(sc->str.CompareNoCase("flats") == 0)
				LoadFlatsTable(sc);
			else if(sc->str.CompareNoCase("include") == 0)
			{
				sc.MustGetToken(TK_StringConst);

				// "$base" is used to include the previous translator in the stack
				FString lumpName = sc->str;
				const GameInfo::FStringStack *baseStackNext = baseStack;
				if(lumpName.CompareNoCase("$base") == 0)
				{
					if(baseStack == NULL)
						sc.ScriptMessage(Scanner::ERROR, "$base is empty.");
					lumpName = baseStack->str;
					baseStackNext = baseStack->Next();
				}

				LoadXlat(lumpName, baseStackNext, true);
			}
			else if(sc->str.CompareNoCase("enable") == 0 || sc->str.CompareNoCase("disable") == 0)
			{
				bool enable = sc->str.CompareNoCase("enable") == 0;
				sc.MustGetToken(TK_Identifier);
				unsigned int i = 0;
				do
				{
					if(sc->str.CompareNoCase(FeatureFlagNames[i]) == 0)
					{
						if(enable)
							FeatureFlags = static_cast<EFeatureFlags>(FeatureFlags|(1<<i));
						else
							FeatureFlags = static_cast<EFeatureFlags>(FeatureFlags|(~(1<<i)));
						break;
					}
				}
				while(FeatureFlagNames[++i]);
				sc.MustGetToken(';');
			}
			else if(sc->str.CompareNoCase("music") == 0)
				LoadMusicTable(sc);
			else
				sc.ScriptMessage(Scanner::ERROR, "Unknown xlat property '%s'.", sc->str.GetChars());
		}
	}

	EFeatureFlags GetFeatureFlags() const { return FeatureFlags; }

	FString GetMusic(unsigned int index) const
	{
		if(const FString *value = musicTable.CheckKey(index))
			return *value;
		return FString();
	}

	WORD GetTilePalette(TArray<MapTile> &tilePalette)
	{
		WORD max = 0;
		WORD min = 0xFFFF;

		TMap<WORD, MapTile>::Iterator iter(this->tilePalette);
		TMap<WORD, MapTile>::Pair *pair;
		while(iter.NextPair(pair))
		{
			if(pair->Key > max)
				max = pair->Key;
			if(pair->Key < min)
				min = pair->Key;
		}
		if(min > max)
			throw CRecoverableError("No tiles found for translation!");

		tilePalette.Resize(max-min+1);

		iter.Reset();
		while(iter.NextPair(pair))
		{
			tilePalette[pair->Key - min] = pair->Value;
		}
		return min;
	}
	void GetZonePalette(TArray<MapZone> &zonePalette)
	{
		TMap<WORD, MapZone>::Iterator iter(this->zonePalette);
		TMap<WORD, MapZone>::Pair *pair;
		while(iter.NextPair(pair))
		{
			pair->Value.index = zonePalette.Size();
			zonePalette.Push(pair->Value);
		}
	}
	bool GetModZone(unsigned short tile, ModZone &modZone)
	{
		ModZone *item = modZones.CheckKey(tile);
		if(item)
		{
			modZone = *item;
			return true;
		}
		return false;
	}
	bool IsValidTile(unsigned short tile)
	{
		MapTile *item = tilePalette.CheckKey(tile);
		if(item)
			return true;
		return false;
	}

	FTextureID TranslateFlat(unsigned int index, bool ceiling, FTextureID def)
	{
		if(flatTable[index][ceiling].isValid())
			return flatTable[index][ceiling];
		return def;
	}

	bool TranslateTileTrigger(unsigned short tile, MapTrigger &trigger)
	{
		MapTrigger *item = tileTriggers.CheckKey(tile);
		if(item)
		{
			trigger = *item;
			return true;
		}
		return false;
	}

	uint32_t TranslateThing(MapThing &thing, MapTrigger &trigger, uint32_t &flags, unsigned short oldnum) const
	{
		uint32_t tsFlags = 0;

		// Add special (i.e. trigger, elevator)
		const ThingSpecialXlat *ts;
		if((ts = thingSpecialTable.CheckKey(oldnum)) != NULL)
		{
			tsFlags = ts->flags;

			if(ts->flags & Xlat::TSF_ISTRIGGER)
				trigger = ts->templateTrigger;
		}

		// Add thing
		unsigned int index = SearchForThing(oldnum, thingTable.Size());
		if(index == UINT_MAX)
			return tsFlags;
		const ThingXlat &type = thingTable[index];

		tsFlags |= TSF_ISTHING;
		flags = type.flags;
		thing.type = type.type;

		// The player has a weird rotation pattern. It's 450-angle.
		bool playerRotation = false;
		if(thing.type == SpecialThingNames[SMT_Player1Start])
			playerRotation = true;

		if(type.angles)
		{
			thing.angle = (oldnum - type.oldnum)*(360/type.angles);
			if(playerRotation)
				thing.angle = (360 + 360/type.angles)-thing.angle;
		}
		else
			thing.angle = 0;

		thing.patrol = type.flags&Xlat::TF_PATHING;
		thing.skill[0] = thing.skill[1] = type.minskill <= 1;
		thing.skill[2] = type.minskill <= 2;
		thing.skill[3] = type.minskill <= 3;
		return tsFlags;
	}

	int TranslateZone(unsigned short tile)
	{
		MapZone *zone = zonePalette.CheckKey(tile);
		if(zone)
			return zone->index;
		return -1;
	}

protected:
	// Find the index in the thing table corresponding to an old thing number
	// taking into account the number of angle variants
	unsigned int SearchForThing(unsigned short oldnum, unsigned int tableSize) const
	{
		unsigned int type = tableSize/2;
		unsigned int max = tableSize-1;
		unsigned int min = 0;
		do
		{
			ThingXlat &check = thingTable[type];
			if(check.oldnum == oldnum ||
				(check.angles && unsigned(oldnum - check.oldnum) < check.angles))
			{
				return type;
			}

			if(check.oldnum > oldnum)
				max = type-1;
			else if(check.oldnum < oldnum)
				min = type+1;

			type = (max+min)/2;
		}
		while(max >= min && max < tableSize);
		return UINT_MAX;
	}

	void LoadFlatsTable(Scanner &sc)
	{
		sc.MustGetToken('{');
		while(!sc.CheckToken('}'))
		{
			sc.MustGetToken(TK_Identifier);
			bool ceiling = sc->str.CompareNoCase("ceiling") == 0;
			if(!ceiling && sc->str.CompareNoCase("floor") != 0)
				sc.ScriptMessage(Scanner::ERROR, "Unknown flat section '%s'.", sc->str.GetChars());

			const TMap<unsigned int, FString> table = LoadStringTable(sc);
			TMap<unsigned int, FString>::ConstPair *pair;
			for(TMap<unsigned int, FString>::ConstIterator iter(table);iter.NextPair(pair);)
			{
				if(pair->Key > 255)
					continue;
				flatTable[pair->Key][ceiling] = TexMan.GetTexture(pair->Value, FTexture::TEX_Flat);
			}
		}
	}

	void LoadMusicTable(Scanner &sc)
	{
		musicTable = LoadStringTable(sc);
	}

	// Parse comma separated list of strings (with optional explicit indexes
	TMap<unsigned int, FString> LoadStringTable(Scanner &sc)
	{
		TMap<unsigned int, FString> table;
		unsigned int index = 0;

		sc.MustGetToken('{');
		do
		{
			sc.MustGetToken(TK_StringConst);
			FString str = sc->str;
			if(sc.CheckToken('='))
			{
				sc.MustGetToken(TK_IntConst);
				index = sc->number;
			}
			table[index++] = str;
		}
		while(sc.CheckToken(','));
		sc.MustGetToken('}');
		return table;
	}

	void LoadTilesTable(Scanner &sc)
	{
		sc.MustGetToken('{');
		while(!sc.CheckToken('}'))
		{
			sc.MustGetToken(TK_Identifier);
			if(sc->str.CompareNoCase("trigger") == 0)
			{
				sc.MustGetToken(TK_IntConst);
				if(sc->number > 0xFFFF)
					sc.ScriptMessage(Scanner::ERROR, "Trigger number out of range.");

				MapTrigger &trigger = tileTriggers[sc->number];
				sc.MustGetToken('{');
				TextMapParser::ParseTrigger(sc, trigger);
			}
			else if(sc->str.CompareNoCase("tile") == 0)
			{
				sc.MustGetToken(TK_IntConst);
				if(sc->number > 0xFFFF)
					sc.ScriptMessage(Scanner::ERROR, "Tile number out of range.");

				MapTile &tile = tilePalette[sc->number];
				sc.MustGetToken('{');
				TextMapParser::ParseTile(sc, tile);
			}
			else if(sc->str.CompareNoCase("modzone") == 0)
			{
				sc.MustGetToken(TK_IntConst);
				if(sc->number > 0xFFFF)
					sc.ScriptMessage(Scanner::ERROR, "Modzone number out of range.");

				unsigned int zoneIndex = sc->number;
				ModZone &zone = modZones[zoneIndex];
				sc.MustGetToken(TK_Identifier);
				if(sc->str.CompareNoCase("fillzone") == 0)
				{
					zone.fillZone = true;
					sc.MustGetToken(TK_Identifier);
				}
				else
				{
					zonePalette[zoneIndex] = MapZone();
					zone.fillZone = false;
				}

				if(sc->str.CompareNoCase("ambush") == 0)
				{
					zone.type = ModZone::AMBUSH;
					sc.MustGetToken(';');
				}
				else if(sc->str.CompareNoCase("changetrigger") == 0)
				{
					zone.type = ModZone::CHANGETRIGGER;

					if(sc.CheckToken(TK_IntConst))
					{
						zone.oldTrigger = sc->number;

						// Warn on first use of deprecated special number.
						static bool deprSpecial = false;
						if(!deprSpecial)
						{
							deprSpecial = true;
							sc.ScriptMessage(Scanner::WARNING, "Use of action special number is deprecated. Use names instead.");
						}
					}
					else
					{
						sc.MustGetToken(TK_StringConst);
						Specials::LineSpecials num = Specials::LookupFunctionNum(sc->str);
						if(num != Specials::NUM_POSSIBLE_SPECIALS)
							zone.oldTrigger = num;
						else
							sc.ScriptMessage(Scanner::ERROR, "Could not resolve action special '%s'.", sc->str.GetChars());
					}

					sc.MustGetToken('{');
					TextMapParser::ParseTrigger(sc, zone.triggerTemplate);
				}
				else
					sc.ScriptMessage(Scanner::ERROR, "Unknown modzone type.");
			}
			else if(sc->str.CompareNoCase("zone") == 0)
			{
				sc.MustGetToken(TK_IntConst);

				MapZone &zone = zonePalette[sc->number];
				sc.MustGetToken('{');
				TextMapParser::ParseZone(sc, zone);
			}
		}
	}

	void LoadThingTable(Scanner &sc)
	{
		// Get the size of existing table so we know what we need to search
		// for replacements.
		unsigned int oldTableSize = thingTable.Size();
		unsigned int replace;

		sc.MustGetToken('{');
		while(!sc.CheckToken('}'))
		{
			// Property
			if(sc.CheckToken(TK_Identifier))
			{
				if(sc->str.CompareNoCase("trigger") == 0)
				{
					sc.MustGetToken(TK_IntConst);
					ThingSpecialXlat &thing = thingSpecialTable[sc->number];
					thing.flags |= Xlat::TSF_ISTRIGGER;

					sc.MustGetToken('{');
					TextMapParser::ParseTrigger(sc, thing.templateTrigger);
				}
				else if(sc->str.CompareNoCase("elevator") == 0)
				{
					sc.MustGetToken(TK_IntConst);
					ThingSpecialXlat &thing = thingSpecialTable[sc->number];
					thing.flags |= Xlat::TSF_ISELEVATOR;

					sc.MustGetToken(';');
				}
				else
					sc.ScriptMessage(Scanner::ERROR, "Unknown thing table block '%s'.", sc->str.GetChars());
			}
			else
			{
				ThingXlat thing;

				// Handle thing translation
				sc.MustGetToken('{');
				sc.MustGetToken(TK_IntConst);
				thing.oldnum = sc->number;
				sc.MustGetToken(',');
				if(sc.CheckToken(TK_IntConst))
				{
					// Deprecated use of Doom Editor Number
					static bool deprEdNum = false;
					if(!deprEdNum)
					{
						deprEdNum = true;
						sc.ScriptMessage(Scanner::WARNING, "Deprecated use of editor number. Use class name instead.");
					}
					if(const ClassDef *cls = ClassDef::FindClass(sc->number))
						thing.type = cls->GetName();
					else if(sc->number >= 1 && sc->number <= SMT_NumThings)
						thing.type = SpecialThingNames[sc->number-1];
				}
				else
				{
					bool sigil = sc.CheckToken('$');
					sc.MustGetToken(TK_Identifier);
					thing.type = FName(sigil ? FString("$") + sc->str : sc->str, true);
					if(!sigil && ClassDef::FindClass(thing.type) == NULL)
						sc.ScriptMessage(Scanner::ERROR, "Could not find class '%s'.", sc->str.GetChars());
				}
				sc.MustGetToken(',');
				sc.MustGetToken(TK_IntConst);
				thing.angles = sc->number;
				sc.MustGetToken(',');
				if(sc.CheckToken(TK_IntConst))
					thing.flags = sc->number;
				else
				{
					thing.flags = 0;
					do
					{
						sc.MustGetToken(TK_Identifier);
						if(sc->str.CompareNoCase("PATHING") == 0)
							thing.flags |= TF_PATHING;
						else if(sc->str.CompareNoCase("HOLOWALL") == 0)
							thing.flags |= TF_HOLOWALL;
						else if(sc->str.CompareNoCase("AMBUSH") == 0)
							thing.flags |= TF_AMBUSH;
						else
							sc.ScriptMessage(Scanner::ERROR, "Unknown flag '%s'.", sc->str.GetChars());
					}
					while(sc.CheckToken('|'));
				}
				sc.MustGetToken(',');
				sc.MustGetToken(TK_IntConst);
				thing.minskill = sc->number;
				sc.MustGetToken('}');

				if(oldTableSize &&
				(replace = SearchForThing(thing.oldnum, oldTableSize)) != UINT_MAX)
				{
					ThingXlat &replaced = thingTable[replace];
					if(replaced.oldnum != thing.oldnum) // Quick check for potential unwanted behavior.
						sc.ScriptMessage(Scanner::ERROR, "Thing %d partially replaces %d.\n", thing.oldnum, replaced.oldnum);
					replaced = thing;
				}
				else
					thingTable.Push(thing);
			}
		}

		qsort(&thingTable[0], thingTable.Size(), sizeof(thingTable[0]), ThingXlat::SortCompare);

		// Sanity check for mod authors: Check for duplicate/overlapping oldnums
		unsigned int i = thingTable.Size();
		if(i >= 2)
		{
			const ThingXlat *current = &thingTable[--i];
			unsigned short oldmin = current->oldnum;
			unsigned short oldmax = oldmin + (current->angles ? current->angles - 1 : 0);
			do
			{
				--current;
				if(current->oldnum <= oldmax && current->oldnum + current->angles - 1 >= oldmin)
					sc.ScriptMessage(Scanner::ERROR, "Thing table contains ambiguous overlap for old num %d (%s).\n", oldmin, current->type.GetChars());
				oldmin = current->oldnum;
				oldmax = oldmin + (current->angles ? current->angles - 1 : 0);
			}
			while(--i);
		}
	}

private:
	void ClearTables()
	{
		// Clear out old data (to be called before initial load)
		for(unsigned int i = 0;i < 256;++i)
		{
			flatTable[i][0].SetInvalid();
			flatTable[i][1].SetInvalid();
		}
		thingTable.Clear();
		thingSpecialTable.Clear();
		tilePalette.Clear();
		tileTriggers.Clear();
		modZones.Clear();
		zonePalette.Clear();
		musicTable.Clear();
		FeatureFlags = static_cast<EFeatureFlags>(0);
	}

	int lump;

	TArray<ThingXlat> thingTable;
	TMap<WORD, ThingSpecialXlat> thingSpecialTable;
	TMap<WORD, MapTile> tilePalette;
	TMap<WORD, MapTrigger> tileTriggers;
	TMap<WORD, ModZone> modZones;
	TMap<WORD, MapZone> zonePalette;
	TMap<unsigned, FString> musicTable;
	FTextureID flatTable[256][2]; // Floor/ceiling textures
	EFeatureFlags FeatureFlags;
};
static Xlat xlat;

static int FindAdjacentDoor(MapSpot spot, MapTrigger *&trigger);

struct HolowallProducer
{
	MapSpot spot;
	MapTile::Side facing;
	unsigned int thingnum;
	uint32_t flags;

	HolowallProducer() : spot(NULL), facing(MapTile::East), flags(0) {}
	HolowallProducer(const HolowallProducer &other) : spot(other.spot), facing(other.facing), thingnum(other.thingnum), flags(other.flags) {}

	HolowallProducer(MapSpot spot, const MapThing &thing, unsigned int thingnum, uint32_t flags)
	: spot(spot), facing(MapTile::Side(thing.angle/90)), thingnum(thingnum), flags(flags)
	{
	}

	void Produce(TArray<MapThing> &things) const
	{
		MapSpot nextSpot = spot->GetAdjacent(facing);
		if(spot->tile)
		{
			spot->sideSolid[0] = spot->sideSolid[1] =
				spot->sideSolid[2] = spot->sideSolid[3] = false;
		}

		// Pathing objects can make statics and walls in front of them non-solid
		if((flags & Xlat::TF_PATHING) && nextSpot)
		{
			if(nextSpot->tile)
			{
				nextSpot->sideSolid[0] = nextSpot->sideSolid[1] =
					nextSpot->sideSolid[2] = nextSpot->sideSolid[3] = false;
			}

			fixed x = nextSpot->GetX(), y = nextSpot->GetY();
			for(unsigned int i = 0; i < things.Size(); ++i)
			{
				MapThing &thing = things[i];
				if((thing.x>>FRACBITS) == x && (thing.y>>FRACBITS) == y)
				{
					// Technically this should be conditional on if the target thing is a holowall producer (non-static)
					// but we don't have that information easily accessible so we let the caller clean this up.
					thing.holo = true;
					break; // Binary map, so we can assume there's only one thing in the given spot
				}
			}
		}
	}

	static void Produce(TArray<HolowallProducer> &producers, TArray<MapThing> &things)
	{
		for(unsigned int i = 0; i < producers.Size(); ++i)
			producers[i].Produce(things);

		// Holowall producers should not affect other holowall producers, so fixup flag
		for(unsigned int i = 0; i < producers.Size(); ++i)
			things[producers[i].thingnum].holo = false;
	}
};

struct MacTile
{
	unsigned int tilenum;
	WORD wallnum1, wallnum2;
	bool used;
};
void GameMap::ReadMacData()
{
	static const BYTE TEX_MASK = 0x1F;
	static const BYTE NUM_MASK = 0x3F;
	static const BYTE DOOR_TEX = 31;
	static const BYTE BLOCKING = 0x80;

	FileReader &lump = *lumps[0];
	lump.Seek(0, SEEK_SET);

	// Setup header
	header.width = 64;
	header.height = 64;

	Plane &mapPlane = NewPlane();
	mapPlane.depth = 64;

	BYTE tilemap[64*64];
	BYTE areamap[64];
	lump.Read(tilemap, 64*64);
	lump.Read(areamap, 64);

	// Areamap maps area number into sound zones. The area number was
	// apparently used to speed up collision checks.
	for(unsigned int i = 0;i < 64;++i)
	{
		if(i+1 >= zonePalette.Size())
		{
			unsigned int start = zonePalette.Size();
			zonePalette.Resize(i+1);
			for(unsigned int j = start;j < zonePalette.Size();++j)
				zonePalette[j].index = j;
		}
	}

	WORD numSpawn, spawnListOfs;
	lump >> numSpawn >> spawnListOfs;
	numSpawn = LittleShort(numSpawn);
	spawnListOfs = LittleShort(spawnListOfs);
	// Followed by 2 more WORDs for numNodes and nodeListOfs. If we had a BSP
	// renderer, we'd care about those.

	// Load in the wall list so we can build a tile set.
	FWadLump wallListLump = Wads.OpenLumpName("WALLLIST");
	TArray<WORD> walltexs;
	walltexs.Resize(wallListLump.GetLength()/2 - 1);
	wallListLump.Seek(2, SEEK_SET);
	wallListLump.Read(&walltexs[0], walltexs.Size()*2);
	for(unsigned int i = 0;i < walltexs.Size();++i)
		walltexs[i] = BigShort(walltexs[i]);
		

	// Find used tiles
	MacTile wallsused[TEX_MASK+1+5];
	for(unsigned int i = 0;i < 64*64;++i)
		wallsused[tilemap[i]&TEX_MASK].used = true;

	// Load wall textures based on our wall list.
	// TODO: These have a flag to have the engine darken the texture with a
	// colormap.
	for(unsigned int i = 1;i < TEX_MASK+1;++i)
	{
		if(!wallsused[i].used)
			continue;

		wallsused[i].tilenum = tilePalette.Size();
		wallsused[i].wallnum1 = walltexs[(i-1)*2]&0x3FFF;
		wallsused[i].wallnum2 = walltexs[(i-1)*2+1]&0x3FFF;

		char name[2][9];
		sprintf(name[0], "WALL%04X", wallsused[i].wallnum1);
		sprintf(name[1], "WALL%04X", wallsused[i].wallnum2);

		Tile tile;
		tile.texture[1] = tile.texture[3] =
			TexMan.CheckForTexture(name[0], FTexture::TEX_Wall);
		tile.texture[0] = tile.texture[2] =
			TexMan.CheckForTexture(name[1], FTexture::TEX_Wall);

		tilePalette.Push(tile);
	}

	// Load in the door textures which are a constant.
	const unsigned int firstdoor = tilePalette.Size();
	for(unsigned int i = 0;i < 4;++i)
	{
		char name[2][9];
		sprintf(name[0], "WALL%04X", walltexs[59+i]&0x3FFF);
		sprintf(name[1], "WALL%04X", walltexs[63]&0x3FFF);

		Tile tile1, tile2;
		tile1.texture[1] = tile1.texture[3] =
		tile2.texture[0] = tile2.texture[2] =
			TexMan.CheckForTexture(name[0], FTexture::TEX_Wall);
		tile1.texture[0] = tile1.texture[2] =
		tile2.texture[1] = tile2.texture[3] =
			TexMan.CheckForTexture(name[1], FTexture::TEX_Wall);

		tile1.offsetHorizontal = true;
		tile2.offsetVertical = true;

		tilePalette.Push(tile1);
		tilePalette.Push(tile2);
	}

	sectorPalette.Resize(1);
	sectorPalette[0].texture[Sector::Floor] = levelInfo->DefaultTexture[Sector::Floor];
	sectorPalette[0].texture[Sector::Ceiling] = levelInfo->DefaultTexture[Sector::Ceiling];
	for(unsigned int i = 0;i < 64*64;++i)
	{
		if(tilemap[i]&BLOCKING)
		{
			if((tilemap[i]&TEX_MASK) > 0)
				mapPlane.map[i].SetTile(&tilePalette[wallsused[tilemap[i]&TEX_MASK].tilenum]);
		}
		else
			mapPlane.map[i].zone = &zonePalette[areamap[tilemap[i]&NUM_MASK]];

		mapPlane.map[i].sector = &sectorPalette[0];
	}

	TArray<HolowallProducer> holowallThings;
	lump.Seek(spawnListOfs, SEEK_SET);
	for(unsigned int i = 0;i < numSpawn;++i)
	{
		BYTE x, y, type, pwallTile;
		lump >> x >> y >> type;
		if(type == 98)
		{
			lump >> pwallTile;
			assert(wallsused[pwallTile].used);
			mapPlane.map[y*64+x].SetTile(&tilePalette[wallsused[pwallTile].tilenum]);
		}

		Thing thing;
		Trigger trigger;
		uint32_t flags = 0;
		uint32_t tsFlags = 0;

		if((tsFlags = xlat.TranslateThing(thing, trigger, flags, type)) == 0)
			printf("Unknown old type %d @ (%d,%d)\n", type, x, y);
		else
		{
			if(tsFlags & Xlat::TSF_ISTRIGGER)
			{
				if(trigger.isSecret)
					++gamestate.secrettotal;

				MapSpot spot = &mapPlane.map[y*64+x];
				if(trigger.action == Specials::Door_Open || trigger.action == Specials::Door_Elevator)
				{
					const unsigned int tex = type/2 != 96/2 ? firstdoor+MIN<int32_t>(trigger.arg[3], 3)*2 : firstdoor+6;
					spot->SetTile(&tilePalette[tex+!(trigger.arg[4]&1)]);
				}

				trigger.x = x;
				trigger.y = y;
				trigger.z = 0;

				Trigger &trig = NewTrigger(x, y, 0);
				trig = trigger;
			}
			if(tsFlags & Xlat::TSF_ISTHING)
			{
				thing.x = (fixed(x)<<FRACBITS)+(FRACUNIT/2);
				thing.y = (fixed(y)<<FRACBITS)+(FRACUNIT/2);
				thing.z = 0;
				thing.ambush = !!(flags & Xlat::TF_AMBUSH);
				int thingnum = things.Push(thing);
				if(flags & Xlat::TF_HOLOWALL)
					holowallThings.Push(HolowallProducer(&mapPlane.map[y*64+x], thing, thingnum, flags));
			}
		}
	}
	HolowallProducer::Produce(holowallThings, things);

	SetupLinks();
}

/* Reads old format maps... well technically WDC format maps.
 * char[6] - Magic "WDC3.1"
 * int32 - Number of maps (Should be 1 in our case)
 * int16 - Number of planes
 * int16 - (Max) name length
 * --- The following would be repeated per map ---
 * char[max] - Name
 * int16 - Width
 * int16 - Hieght
 * ... raw plane data ...
 */
void GameMap::ReadPlanesData()
{
	static const unsigned short UNIT = 64;
	enum OldPlanes { Plane_Tiles, Plane_Object, Plane_Flats, NUM_USABLE_PLANES };

	if(levelInfo->Translator.IsEmpty())
		xlat.LoadXlat(gameinfo.Translator.str, gameinfo.Translator.Next());
	else
		xlat.LoadXlat(levelInfo->Translator, &gameinfo.Translator);

	Xlat::EFeatureFlags FeatureFlags = xlat.GetFeatureFlags();
	sectorPalette.Clear();

	// Old format maps always have a tile size of 64
	header.tileSize = UNIT;

	// Xlat loaded, see if we have a Mac format map.
	char magic[6];
	if(lumps[0]->Read(magic, 6) != 6 || strncmp(magic, "WDC3.1", 6) != 0)
	{
		ReadMacData();
		return;
	}

	FileReader *lump = lumps[0];

	// Read plane count
	lump->Seek(10, SEEK_SET);
	WORD numPlanes, nameLength;
	lump->Read(&numPlanes, 2);
	lump->Read(&nameLength, 2);
	numPlanes = LittleShort(numPlanes);
	nameLength = LittleShort(nameLength);

	TUniquePtr<char[]> name(new char[nameLength+1]);
	lump->Read(name.Get(), nameLength);
	name[nameLength] = 0;
	header.name = name;

	WORD dimensions[2];
	lump->Read(dimensions, 4);
	dimensions[0] = LittleShort(dimensions[0]);
	dimensions[1] = LittleShort(dimensions[1]);
	DWORD size = dimensions[0]*dimensions[1];
	header.width = dimensions[0];
	header.height = dimensions[1];

	Plane &mapPlane = NewPlane();
	mapPlane.depth = UNIT;

	// We need to store the spots marked for ambush since it's stored in the
	// tiles plane instead of the objects plane.
	TArray<WORD> ambushSpots;
	TArray<MapTrigger> triggers;
	TMap<WORD, TArray<MapSpot> > elevatorSpots;

	// Read and store the info plane so we can reference it
	TUniquePtr<WORD[]> infoplane(new WORD[size]);
	if(numPlanes > 3)
	{
		lump->Seek(size*2*3, SEEK_CUR);
		lump->Read(infoplane.Get(), size*2);
		lump->Seek(18+nameLength, SEEK_SET);
	}
	else
		memset(infoplane.Get(), 0, size*2);

	FTextureID defaultCeiling = levelInfo->DefaultTexture[Sector::Ceiling];
	FTextureID defaultFloor = levelInfo->DefaultTexture[Sector::Floor];

	for(int plane = 0;plane < numPlanes && plane < NUM_USABLE_PLANES;++plane)
	{
		if(plane == 3) // Info plane is already read
			continue;

		TUniquePtr<WORD[]> oldplane(new WORD[size]);
		lump->Read(oldplane.Get(), size*2);

		switch(plane)
		{
			default:
				break;

			case Plane_Tiles:
			{
				WORD tileStart = xlat.GetTilePalette(tilePalette);
				xlat.GetZonePalette(zonePalette);

				TArray<WORD> fillSpots;
				TMap<WORD, Xlat::ModZone> changeTriggerSpots;
				

				for(unsigned int i = 0;i < size;++i)
				{
					oldplane[i] = LittleShort(oldplane[i]);

					if(xlat.IsValidTile(oldplane[i]))
						mapPlane.map[i].SetTile(&tilePalette[oldplane[i]-tileStart]);
					else
						mapPlane.map[i].SetTile(NULL);

					Xlat::ModZone zone;
					if(xlat.GetModZone(oldplane[i], zone))
					{
						if(zone.fillZone)
							fillSpots.Push(i);

						switch(zone.type)
						{
							case Xlat::ModZone::AMBUSH:
								ambushSpots.Push(i);
								break;
							case Xlat::ModZone::CHANGETRIGGER:
								changeTriggerSpots[i] = zone;
								break;
						}
					}

					MapTrigger templateTrigger;
					if(xlat.TranslateTileTrigger(oldplane[i], templateTrigger))
					{
						templateTrigger.x = i%header.width;
						templateTrigger.y = i/header.width;
						templateTrigger.z = 0;

						triggers.Push(templateTrigger);
					}

					int zoneIndex;
					if((zoneIndex = xlat.TranslateZone(oldplane[i])) != -1)
						mapPlane.map[i].zone = &zonePalette[zoneIndex];
					else
						mapPlane.map[i].zone = NULL;
				}

				// Get a sound zone for modzones that aren't valid sound zones.
				for(unsigned int i = 0;i < fillSpots.Size();++i)
				{
					const int candidates[4] = {
						fillSpots[i] + 1,
						fillSpots[i] - (int)header.width,
						fillSpots[i] - 1,
						fillSpots[i] + (int)header.width
					};
					for(unsigned int j = 0;j < 4;++j)
					{
						// Ensure that all candidates are valid locations.
						// In addition for moving left/right check to see that
						// the new location is indeed in the same row.
						if((candidates[j] < 0 || (unsigned)candidates[j] > size) ||
							((j == Tile::East || j == Tile::West) &&
							(candidates[j]/header.width != fillSpots[i]/header.width)))
							continue;

						// First adjacent zone wins
						if(mapPlane.map[candidates[j]].zone != NULL)
						{
							mapPlane.map[fillSpots[i]].zone = mapPlane.map[candidates[j]].zone;
							break;
						}
					}
				}

				TMap<WORD, Xlat::ModZone>::Iterator iter(changeTriggerSpots);
				TMap<WORD, Xlat::ModZone>::Pair *pair;
				while(iter.NextPair(pair))
				{
					// Look for and switch exit triggers.
					// NOTE: We're modifying the adjacent tile so the directions here are reversed.
					//       That is to say the tile to the east modifies the west wall.
					const int candidates[4] = {
						pair->Key - 1,
						pair->Key + (int)header.width,
						pair->Key + 1,
						pair->Key - (int)header.width
					};
					for(unsigned int j = 0;j < 4;++j)
					{
						// Same as before, only this time we check if our
						// replacement trigger activates at the line between
						// this tile and the cadidate.
						if((candidates[j] < 0 || (unsigned)candidates[j] > size) ||
							((j == Trigger::East || j == Trigger::West) &&
							(candidates[j]/header.width != pair->Key/header.width)) ||
							!pair->Value.triggerTemplate.activate[j])
							continue;

						// Look for any triggers matching the candidate
						for(int k = triggers.Size()-1;k >= 0;--k)
						{
							if(triggers[k].action == pair->Value.oldTrigger &&
								triggers[k].x == (unsigned)candidates[j]%header.width &&
								triggers[k].y == (unsigned)candidates[j]/header.width)
							{
								// Disable
								triggers[k].activate[j] = false;

								Trigger &triggerTemplate = pair->Value.triggerTemplate;
								triggerTemplate.x = triggers[k].x;
								triggerTemplate.y = triggers[k].y;
								triggerTemplate.z = triggers[k].z;

								// Only enable the activation in the direction we're changing.
								triggerTemplate.activate[0] = triggerTemplate.activate[1] = triggerTemplate.activate[2] = triggerTemplate.activate[3] = false;
								triggerTemplate.activate[j] = true;

								triggers.Push(triggerTemplate);
							}
						}
					}
				}

				if(FeatureFlags & Xlat::FF_LIGHTLEVELS)
				{
					// Visibility is roughly exponential
					static const fixed visTable[16] = {
						0x8888, 0xDDDD, 2<<FRACBITS,
						3<<FRACBITS, 8<<FRACBITS, 15<<FRACBITS,
						29<<FRACBITS, 56<<FRACBITS, 108<<FRACBITS,
						// After this point we basically max out the depth fog any way
						200<<FRACBITS, 200<<FRACBITS, 200<<FRACBITS,
						200<<FRACBITS, 200<<FRACBITS, 200<<FRACBITS,
						200<<FRACBITS
					};

					gLevelVisibility = visTable[clamp(oldplane[3] - 0xFC, 0, 15)]*LIGHTVISIBILITY_FACTOR;
					gLevelLight = clamp(oldplane[2] - 0xD8, 0, 7)*8 + 130; // Seems to be approx accurate for every even number lighting (0, 2, 4, 6)
				}
				else
				{
					gLevelVisibility = levelInfo->DefaultVisibility;
					gLevelLight = levelInfo->DefaultLighting;
				}
				gLevelMaxLightVis = levelInfo->DefaultMaxLightVis;
				break;
			}

			case Plane_Object:
			{
				bool canUseFlatColor = true;
				bool gotFlatTextures = false;
				unsigned int ambushSpot = 0;
				ambushSpots.Push(0xFFFF); // Prevent uninitialized value errors.

				TArray<HolowallProducer> holowallThings;

				unsigned int i = 0;
				for(;i < size;++i)
				{
					oldplane[i] = LittleShort(oldplane[i]);

					if(oldplane[i] == 0)
					{
						// In case of malformed maps we need to always check this.
						if(ambushSpots[ambushSpot] == i)
							++ambushSpot;
						continue;
					}

					if(i == 0 && (FeatureFlags & Xlat::FF_PLANEDEPTH))
					{
						if(oldplane[0] >= 0x5A && oldplane[0] <= 0x61)
							mapPlane.depth = UNIT*(oldplane[i]-0x5A+1);
						else if(oldplane[0] >= 0x1C2 && oldplane[0] <= 0x1C9)
							mapPlane.depth = UNIT*(oldplane[i]-0x1C2+9);
						else // ROTT would error if this is invalid.
							printf("Error: Map height specifier %X not in range!\n", oldplane[i]);
						continue;
					}

					if(FeatureFlags & Xlat::FF_GLOBALMETA)
					{
						switch(oldplane[i]>>8)
						{
							default: break;
							case 0xF1: // Informant messages
							case 0xF2: // Scientist messages
							case 0xF3: // Men scientist messages
							case 0xF5: // Intralevel warp coordinate
								continue;
							case 0xFB:
								// Floor/ceiling texture
								// We only read the first instance
								if(canUseFlatColor || !gotFlatTextures)
								{
									canUseFlatColor = false;
									gotFlatTextures = true;
									defaultCeiling = xlat.TranslateFlat(oldplane[++i]>>8, Sector::Ceiling, levelInfo->DefaultTexture[Sector::Ceiling]);
									defaultFloor = xlat.TranslateFlat(oldplane[i]&0xFF, Sector::Floor, levelInfo->DefaultTexture[Sector::Floor]);

									sectorPalette.Resize(1);
									continue;
								}
								break;
							case 0xFE:
								// This would be pointless since ECWolf is
								// always texture mapped, but it seems to be
								// legal for a map to not include a texture tag.
								if(canUseFlatColor && !gotFlatTextures)
								{
									gotFlatTextures = true;

									++i;

									const PalEntry c = GPalette.BaseColors[oldplane[i]>>8], f = GPalette.BaseColors[oldplane[i]&0xFF];
									FString ceilingColor, floorColor;
									ceilingColor.Format("#%02X%02X%02X", c.r, c.g, c.b);
									floorColor.Format("#%02X%02X%02X", f.r, f.g, f.b);

									defaultCeiling = TexMan.GetTexture(ceilingColor, FTexture::TEX_Flat);
									defaultFloor = TexMan.GetTexture(floorColor, FTexture::TEX_Flat);
								}
								continue;
						}
					}

					Thing thing;
					Trigger trigger;
					uint32_t flags = 0;
					uint32_t tsFlags = 0;

					if((tsFlags = xlat.TranslateThing(thing, trigger, flags, oldplane[i])) == 0)
						printf("Unknown old type %d @ (%d,%d)\n", oldplane[i], i%header.width, i/header.width);
					else
					{
						if(tsFlags & Xlat::TSF_ISTRIGGER)
						{
							trigger.x = i%header.width;
							trigger.y = i/header.width;
							trigger.z = 0;

							triggers.Push(trigger);
						}
						if(tsFlags & Xlat::TSF_ISELEVATOR)
						{
							elevatorSpots[oldplane[i]].Push(&mapPlane.map[i]);
						}
						if(tsFlags & Xlat::TSF_ISTHING)
						{
							thing.x = ((i%header.width)<<FRACBITS)+(FRACUNIT/2);
							thing.y = ((i/header.width)<<FRACBITS)+(FRACUNIT/2);
							if((FeatureFlags & Xlat::FF_ZHEIGHTS) && (infoplane[i]&0xFF00) == 0xB000)
							{
								// ROTT uses the signed lower byte of info plane to assign height in 1/16 tile (4 unit) increments
								thing.z = int8_t(infoplane[i]&0xFF)*0x1000;
								// Unset value so it can't also be interpreted as a remote trigger (should only affect oversized maps)
								infoplane[i] = 0;
							}
							else
								thing.z = 0;
							thing.ambush = (flags & Xlat::TF_AMBUSH) || ambushSpots[ambushSpot] == i;

							int thingnum = things.Push(thing);
							if(flags & Xlat::TF_HOLOWALL)
								holowallThings.Push(HolowallProducer(&mapPlane.map[i], thing, thingnum, flags));
						}
					}

					if(ambushSpots[ambushSpot] == i)
						++ambushSpot;
				}

				HolowallProducer::Produce(holowallThings, things);
				break;
			}

			case Plane_Flats:
			{
				// Look for all unique floor/ceiling texture combinations.
				WORD type = sectorPalette.Size();
				TMap<WORD, WORD> flatMap;
				for(unsigned int i = 0;i < size;++i)
				{
					oldplane[i] = LittleShort(oldplane[i]);

					if(!flatMap.CheckKey(oldplane[i]))
						flatMap[oldplane[i]] = type++;
				}

				// Build the palette.
				sectorPalette.Resize(type);
				TMap<WORD, WORD>::ConstIterator iter(flatMap);
				TMap<WORD, WORD>::ConstPair *pair;
				while(iter.NextPair(pair))
				{
					Sector &sect = sectorPalette[pair->Value];
					sect.texture[Sector::Floor] = xlat.TranslateFlat(pair->Key&0xFF, Sector::Floor, defaultFloor);
					sect.texture[Sector::Ceiling] = xlat.TranslateFlat(pair->Key>>8, Sector::Ceiling, defaultCeiling);
				}

				// Now link the sector data to map points!
				for(unsigned int i = 0;i < size;++i)
					mapPlane.map[i].sector = &sectorPalette[flatMap[oldplane[i]]];
				break;
			}
		}
	}

	SetupLinks();

	// Scan for music selection in info plane in y = 0
	// No need for a feature flag here since we only use it if we can find a
	// valid entry, so we presence of the music table in xlat should be enough.
	for(unsigned int i = 0;i < header.width;++i)
	{
		if((infoplane[i]&0xFF00) == 0xBA00)
		{
			FString music = xlat.GetMusic(infoplane[i]&0xFF);
			if(music.IsNotEmpty())
			{
				header.music = music;
				infoplane[i] = 0;
				break;
			}
		}
	}

	// Install triggers
	for(unsigned int i = 0;i < triggers.Size();++i)
	{
		Trigger &templateTrigger = triggers[i];

		// Check the info plane and if set move the activation point to a switch ot touch plate
		const WORD info = infoplane[templateTrigger.y*header.width + templateTrigger.x];
		if(info)
		{
			MapSpot target = GetSpot(templateTrigger.x, templateTrigger.y, 0);
			unsigned int tag = (templateTrigger.x<<8)|templateTrigger.y;
			SetSpotTag(target, tag);

			// Activated by touch plate or switch
			templateTrigger.arg[0] = tag;
			templateTrigger.x = (info>>8)&0xFF;
			templateTrigger.y = info&0xFF;

			MapSpot spot = GetSpot(templateTrigger.x, templateTrigger.y, 0);
			if(spot->tile) // Switch
			{
				templateTrigger.playerCross = false;
				templateTrigger.playerUse = true;
			}
			else // Touch plate
			{
				templateTrigger.playerCross = true;
				templateTrigger.playerUse = false;
			}
			templateTrigger.activate[0] = templateTrigger.activate[1] = templateTrigger.activate[2] = templateTrigger.activate[3] = true;
		}

		Trigger &trig = NewTrigger(templateTrigger.x, templateTrigger.y, templateTrigger.z);
		trig = templateTrigger;

		if(trig.isSecret)
			++gamestate.secrettotal;
	}

	// Install elevators
	TMap<WORD, TArray<MapSpot> >::ConstIterator iter(elevatorSpots);
	TMap<WORD, TArray<MapSpot> >::ConstPair *pair;
	while(iter.NextPair(pair))
	{
		const TArray<MapSpot> &locations = pair->Value;

		// Elevators in the same sound zone move faster
		bool samezone = true;
		for(unsigned int i = locations.Size();i-- > 1;)
		{
			if(locations[i]->zone != locations[0]->zone)
			{
				samezone = false;
				break;
			}
		}

		assert(locations.Size() > 0);
		{
			unsigned int elevTag = 0;
			unsigned int swtchTag = 0;
			int *lastNext = NULL;
			for(unsigned int i = 0;i < locations.Size();++i)
			{
				MapSpot spot = locations[i];

				// Search for door in adjacent tile
				int doorside;
				MapTrigger *trigger = NULL;
				if((doorside = FindAdjacentDoor(spot, trigger)) != -1)
				{
					MapSpot door = spot->GetAdjacent(static_cast<MapTile::Side>(doorside));
					MapSpot swtch = spot->GetAdjacent(static_cast<MapTile::Side>(doorside), true);

					trigger->action = Specials::Door_Elevator;
					trigger->arg[0] = (swtch->GetX()<<8)|swtch->GetY();
					SetSpotTag(swtch, trigger->arg[0]);
					if(i == 0)
					{
						elevTag = trigger->arg[0];
						elevatorPosition[elevTag] = swtch;
					}

					Trigger &elevTrigger = NewTrigger(swtch->GetX(), swtch->GetY(), 0);
					elevTrigger.action = Specials::Elevator_SwitchFloor;
					elevTrigger.playerUse = true;
					elevTrigger.repeatable = true;
					elevTrigger.arg[0] = elevTag;
					elevTrigger.arg[1] = (door->GetX()<<8)|door->GetY();
					elevTrigger.arg[2] = samezone ? 140 : 280;
					if(i == 0)
						lastNext = &elevTrigger.arg[3];
					else
						elevTrigger.arg[3] = swtchTag;
					SetSpotTag(door, elevTrigger.arg[1]);

					swtchTag = trigger->arg[0];
				}
			}
			if(lastNext)
				*lastNext = swtchTag;
		}
	}
}

static int FindAdjacentDoor(MapSpot spot, MapTrigger *&trigger)
{
	const MapSpot adjacent[4] = {
		spot->GetAdjacent(MapTile::East),
		spot->GetAdjacent(MapTile::North),
		spot->GetAdjacent(MapTile::West),
		spot->GetAdjacent(MapTile::South)
	};

	for(unsigned int i = 0;i < 4;++i)
	{
		if(!adjacent[i])
			continue;

		for(unsigned int t = adjacent[i]->triggers.Size();t-- > 0;)
		{
			if(adjacent[i]->triggers[t].action == Specials::Door_Open)
			{
				trigger = &adjacent[i]->triggers[t];
				return i;
			}
		}
	}
	return -1;
}
