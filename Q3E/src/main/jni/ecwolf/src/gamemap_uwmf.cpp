/*
** gamemap_uwmf.cpp
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

#include "gamemap.h"
#include "gamemap_common.h"
#include "g_mapinfo.h"
#include "id_ca.h"
#include "lnspec.h"
#include "scanner.h"
#include "thingdef/thingdef.h"
#include "w_wad.h"
#include "wl_game.h"
#include "wl_shade.h"

#define CheckKey(x) if(key.CompareNoCase(x) == 0)

#define StartParseBlock \
	while(!sc.CheckToken('}')) { \
		sc.MustGetToken(TK_Identifier); \
		FString key(sc->str); \
		if(sc.CheckToken('=')) {
#define EndParseBlock \
			else \
				sc.GetNextToken(); \
			sc.MustGetToken(';'); \
		} else \
			sc.ScriptMessage(Scanner::ERROR, "Invalid syntax.\n"); \
	}

static inline int MustGetSignedInteger(Scanner &sc)
{
	bool neg = sc.CheckToken('-');
	sc.MustGetToken(TK_IntConst);
	return neg ? -sc->number : sc->number;
}

void TextMapParser::ParseTile(Scanner &sc, MapTile &tile)
{
	StartParseBlock

	CheckKey("blockingnorth")
	{
		sc.MustGetToken(TK_BoolConst);
		tile.sideSolid[MapTile::North] = sc->boolean;
	}
	else CheckKey("blockingsouth")
	{
		sc.MustGetToken(TK_BoolConst);
		tile.sideSolid[MapTile::South] = sc->boolean;
	}
	else CheckKey("blockingeast")
	{
		sc.MustGetToken(TK_BoolConst);
		tile.sideSolid[MapTile::East] = sc->boolean;
	}
	else CheckKey("blockingwest")
	{
		sc.MustGetToken(TK_BoolConst);
		tile.sideSolid[MapTile::West] = sc->boolean;
	}
	else CheckKey("dontoverlay")
	{
		sc.MustGetToken(TK_BoolConst);
		tile.dontOverlay = sc->boolean;
	}
	else CheckKey("soundsequence")
	{
		sc.MustGetToken(TK_StringConst);
		tile.soundSequence = sc->str;
	}
	else CheckKey("texturenorth")
	{
		sc.MustGetToken(TK_StringConst);
		tile.texture[MapTile::North] = TexMan.CheckForTexture(sc->str, FTexture::TEX_Wall);
	}
	else CheckKey("texturesouth")
	{
		sc.MustGetToken(TK_StringConst);
		tile.texture[MapTile::South] = TexMan.CheckForTexture(sc->str, FTexture::TEX_Wall);
	}
	else CheckKey("texturewest")
	{
		sc.MustGetToken(TK_StringConst);
		tile.texture[MapTile::West] = TexMan.CheckForTexture(sc->str, FTexture::TEX_Wall);
	}
	else CheckKey("textureeast")
	{
		sc.MustGetToken(TK_StringConst);
		tile.texture[MapTile::East] = TexMan.CheckForTexture(sc->str, FTexture::TEX_Wall);
	}
	else CheckKey("textureoverhead")
	{
		sc.MustGetToken(TK_StringConst);
		tile.overhead = TexMan.CheckForTexture(sc->str, FTexture::TEX_Wall);
	}
	else CheckKey("mapped")
	{
		sc.MustGetToken(TK_IntConst);
		tile.mapped = sc->number;
	}
	else CheckKey("offsetvertical")
	{
		sc.MustGetToken(TK_BoolConst);
		tile.offsetVertical = sc->boolean;
	}
	else CheckKey("offsethorizontal")
	{
		sc.MustGetToken(TK_BoolConst);
		tile.offsetHorizontal = sc->boolean;
	}

	EndParseBlock
}

void TextMapParser::ParseTrigger(Scanner &sc, MapTrigger &trigger)
{
	StartParseBlock

	CheckKey("x")
	{
		sc.MustGetToken(TK_IntConst);
		trigger.x = sc->number;
	}
	else CheckKey("y")
	{
		sc.MustGetToken(TK_IntConst);
		trigger.y = sc->number;
	}
	else CheckKey("z")
	{
		sc.MustGetToken(TK_IntConst);
		trigger.z = sc->number;
	}
	else CheckKey("activatenorth")
	{
		sc.MustGetToken(TK_BoolConst);
		trigger.activate[MapTrigger::North] = sc->boolean;
	}
	else CheckKey("activatesouth")
	{
		sc.MustGetToken(TK_BoolConst);
		trigger.activate[MapTrigger::South] = sc->boolean;
	}
	else CheckKey("activateeast")
	{
		sc.MustGetToken(TK_BoolConst);
		trigger.activate[MapTrigger::East] = sc->boolean;
	}
	else CheckKey("activatewest")
	{
		sc.MustGetToken(TK_BoolConst);
		trigger.activate[MapTrigger::West] = sc->boolean;
	}
	else CheckKey("action")
	{
		if(sc.CheckToken(TK_IntConst))
		{
			trigger.action = sc->number;

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
				trigger.action = num;
			else
			{
				if(!sc->str.IsEmpty())
					sc.ScriptMessage(Scanner::WARNING, "Could not resolve action special '%s'.", sc->str.GetChars());
				trigger.action = 0;
			}
		}
	}
	else CheckKey("arg0")
	{
		trigger.arg[0] = MustGetSignedInteger(sc);
	}
	else CheckKey("arg1")
	{
		trigger.arg[1] = MustGetSignedInteger(sc);
	}
	else CheckKey("arg2")
	{
		trigger.arg[2] = MustGetSignedInteger(sc);
	}
	else CheckKey("arg3")
	{
		trigger.arg[3] = MustGetSignedInteger(sc);
	}
	else CheckKey("arg4")
	{
		trigger.arg[4] = MustGetSignedInteger(sc);
	}
	else CheckKey("playeruse")
	{
		sc.MustGetToken(TK_BoolConst);
		trigger.playerUse = sc->boolean;
	}
	else CheckKey("playercross")
	{
		sc.MustGetToken(TK_BoolConst);
		trigger.playerCross = sc->boolean;
	}
	else CheckKey("monsteruse")
	{
		sc.MustGetToken(TK_BoolConst);
		trigger.monsterUse = sc->boolean;
	}
	else CheckKey("repeatable")
	{
		sc.MustGetToken(TK_BoolConst);
		trigger.repeatable = sc->boolean;
	}
	else CheckKey("secret")
	{
		sc.MustGetToken(TK_BoolConst);
		trigger.isSecret = sc->boolean;
	}

	EndParseBlock
}

void TextMapParser::ParseZone(Scanner &sc, MapZone &zone)
{
	StartParseBlock
	if(false);
	EndParseBlock
}

class UWMFParser : public TextMapParser
{
	public:
		UWMFParser(GameMap *gm, Scanner &sc) : gm(gm), sc(sc)
		{
		}
		~UWMFParser()
		{
			for(unsigned int i = 0;i < data.Size();++i)
				delete[] data[i];
		}

		void Parse()
		{
			bool ecwolf12Namespace = false;
			bool canChangeHeader = true;
			gm->header.width = 64;
			gm->header.height = 64;
			gm->header.tileSize = 64;

			while(sc.TokensLeft())
			{
				sc.MustGetToken(TK_Identifier);
				FString key(sc->str);
				if(sc.CheckToken('='))
				{
					CheckKey("namespace")
					{
						sc.MustGetToken(TK_StringConst);
						// Experimental namespace
						ecwolf12Namespace = sc->str.Compare("ECWolf-v12") == 0;
						if(sc->str.Compare("Wolf3D") != 0 && !ecwolf12Namespace)
							sc.ScriptMessage(Scanner::WARNING, "Wolf3D and ECWolf-v12 are the only supported namespaces.\n");
					}
					else CheckKey("tilesize")
					{
						sc.MustGetToken(TK_IntConst);
						gm->header.tileSize = sc->number;
					}
					else CheckKey("name")
					{
						sc.MustGetToken(TK_StringConst);
						gm->header.name = sc->str;
					}
					else CheckKey("width")
					{
						if(!canChangeHeader)
							sc.ScriptMessage(Scanner::ERROR, "Changing dimensions after dependent data.\n");
						sc.MustGetToken(TK_IntConst);
						gm->header.width = sc->number;
					}
					else CheckKey("height")
					{
						if(!canChangeHeader)
							sc.ScriptMessage(Scanner::ERROR, "Changing dimensions after dependent data.\n");
						sc.MustGetToken(TK_IntConst);
						gm->header.height = sc->number;
					}
					// Defaultlightlevel and defaultvisibility may be merged
					// into the UWMF spec once the values from ROTT are set in
					// stone.
					else CheckKey("defaultlightlevel")
					{
						if(!ecwolf12Namespace)
							sc.ScriptMessage(Scanner::WARNING, "Setting defaultlightlevel on Wolf3D namespace not standard, use ECWolf-v12\n");
						sc.MustGetToken(TK_IntConst);
						gLevelLight = sc->number;
					}
					else CheckKey("defaultvisibility")
					{
						if(!ecwolf12Namespace)
							sc.ScriptMessage(Scanner::WARNING, "Setting defaultvisibility on Wolf3D namespace not standard, use ECWolf-v12\n");
						sc.MustGetToken(TK_FloatConst);
						gLevelVisibility = static_cast<fixed>(sc->decimal*LIGHTVISIBILITY_FACTOR*65536.);
					}
					else
						sc.GetNextToken();
					sc.MustGetToken(';');
				}
				else if(sc.CheckToken('{'))
				{
					CheckKey("planemap")
					{
						canChangeHeader = false;
						ParsePlaneMap();
					}
					else CheckKey("tile")
						ParseTile();
					else CheckKey("sector")
						ParseSector();
					else CheckKey("zone")
						ParseZone();
					else CheckKey("plane")
						ParsePlane();
					else CheckKey("thing")
						ParseThing();
					else CheckKey("trigger")
						ParseTrigger();
					else
					{
						while(!sc.CheckToken('}'))
							sc.GetNextToken();
					}
				}
				else
					sc.ScriptMessage(Scanner::ERROR, "Unable to parse TEXTMAP, invalid syntax.\n");
			}

			InstallPlanes();
		}

	protected:
		void InstallPlanes()
		{
			// Transfer data into actual structure with pointers.
			const unsigned int size = gm->GetHeader().width*gm->GetHeader().height;

			for(unsigned int i = 0;i < data.Size();++i)
			{
				MapPlane &plane = gm->planes[i];
				PMData* pdata = data[i];
				for(unsigned int j = 0;j < size;++j)
				{
					plane.map[j].SetTile(
						pdata[j].tile < 0 || (unsigned)pdata[j].tile >= gm->tilePalette.Size()
						? NULL : &gm->tilePalette[pdata[j].tile]
					);
					plane.map[j].sector =
						pdata[j].sector < 0 || (unsigned)pdata[j].sector >= gm->sectorPalette.Size()
						? NULL : &gm->sectorPalette[pdata[j].sector];
					plane.map[j].zone =
						pdata[j].zone < 0 || (unsigned)pdata[j].zone >= gm->zonePalette.Size()
						? NULL : &gm->zonePalette[pdata[j].zone];

					if(pdata[j].tag)
						gm->SetSpotTag(&plane.map[j], pdata[j].tag);
				}
			}

			// Load in the triggers since they can depend on plane data
			for(unsigned int i = 0;i < triggers.Size();++i)
			{
				MapTrigger &src = triggers[i];
				MapTrigger &trig = gm->NewTrigger(src.x, src.y, src.z);
				trig = src;

				if(trig.isSecret)
					++gamestate.secrettotal;
			}
		}

		void ParsePlaneMap()
		{
			const unsigned int size = gm->GetHeader().width*gm->GetHeader().height;

			PMData* pdata = new PMData[size];
			data.Push(pdata);
			unsigned int i = 0;
			// Different syntax
			while(!sc.CheckToken('}'))
			{
				sc.MustGetToken('{');
				pdata[i].tile = MustGetSignedInteger(sc);
				sc.MustGetToken(',');
				pdata[i].sector = MustGetSignedInteger(sc);
				sc.MustGetToken(',');
				pdata[i].zone = MustGetSignedInteger(sc);
				if(sc.CheckToken(','))
					pdata[i].tag = MustGetSignedInteger(sc);
				else
					pdata[i].tag = 0;
				sc.MustGetToken('}');
				if(++i != size)
					sc.MustGetToken(',');
				else
				{
					sc.MustGetToken('}');
					break;
				}
			}
			if(i != size)
				sc.ScriptMessage(Scanner::ERROR, "Not enough data in planemap.\n");
		}

		void ParsePlane()
		{
			MapPlane &plane = gm->NewPlane();
			StartParseBlock

			CheckKey("depth")
			{
				sc.MustGetToken(TK_IntConst);
				plane.depth = sc->number;
			}

			EndParseBlock
		}

		void ParseSector()
		{
			MapSector sector;
			StartParseBlock

			CheckKey("texturefloor")
			{
				sc.MustGetToken(TK_StringConst);
				sector.texture[MapSector::Floor] = TexMan.GetTexture(sc->str, FTexture::TEX_Flat);
			}
			else CheckKey("textureceiling")
			{
				sc.MustGetToken(TK_StringConst);
				sector.texture[MapSector::Ceiling] = TexMan.GetTexture(sc->str, FTexture::TEX_Flat);
			}

			EndParseBlock
			gm->sectorPalette.Push(sector);
		}

		void ParseThing()
		{
			MapThing thing;
			StartParseBlock

			CheckKey("x")
			{
				sc.MustGetToken(TK_FloatConst);
				thing.x = static_cast<fixed>(sc->decimal*FRACUNIT);
			}
			else CheckKey("y")
			{
				sc.MustGetToken(TK_FloatConst);
				thing.y = static_cast<fixed>(sc->decimal*FRACUNIT);
			}
			else CheckKey("z")
			{
				sc.MustGetToken(TK_FloatConst);
				thing.z = static_cast<fixed>(sc->decimal*FRACUNIT);
			}
			else CheckKey("angle")
			{
				sc.MustGetToken(TK_IntConst);
				thing.angle = sc->number;
			}
			else CheckKey("type")
			{
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
					sc.MustGetToken(TK_StringConst);
					thing.type = FName(sc->str, true);
				}
			}
			else CheckKey("ambush")
			{
				sc.MustGetToken(TK_BoolConst);
				thing.ambush = sc->boolean;
			}
			else CheckKey("patrol")
			{
				sc.MustGetToken(TK_BoolConst);
				thing.patrol = sc->boolean;
			}
			else CheckKey("skill1")
			{
				sc.MustGetToken(TK_BoolConst);
				thing.skill[0] = sc->boolean;
			}
			else CheckKey("skill2")
			{
				sc.MustGetToken(TK_BoolConst);
				thing.skill[1] = sc->boolean;
			}
			else CheckKey("skill3")
			{
				sc.MustGetToken(TK_BoolConst);
				thing.skill[2] = sc->boolean;
			}
			else CheckKey("skill4")
			{
				sc.MustGetToken(TK_BoolConst);
				thing.skill[3] = sc->boolean;
			}

			EndParseBlock
			gm->things.Push(thing);
		}

		void ParseTile()
		{
			MapTile tile;
			TextMapParser::ParseTile(sc, tile);

			gm->tilePalette.Push(tile);
		}

		void ParseTrigger()
		{
			// Since we can't know the x,y,z yet we need to create a temp and
			// copy it later.
			MapTrigger trigger;
			TextMapParser::ParseTrigger(sc, trigger);

			triggers.Push(trigger);
		}

		void ParseZone()
		{
			MapZone zone;
			zone.index = gm->zonePalette.Size();

			TextMapParser::ParseZone(sc, zone);
			gm->zonePalette.Push(zone);
		}

	private:
		struct PMData
		{
			int tile;
			int sector;
			int zone;
			int tag;
		};

		GameMap * const gm;
		Scanner &sc;
		TArray<PMData*> data;
		TArray<MapTrigger> triggers;
};

void GameMap::ReadUWMFData()
{
	gLevelVisibility = levelInfo->DefaultVisibility;
	gLevelLight = levelInfo->DefaultLighting;
	gLevelMaxLightVis = levelInfo->DefaultMaxLightVis;

	long size = lumps[0]->GetLength();
	char *data = new char[size];
	lumps[0]->Read(data, size);
	Scanner sc(data, size);
	delete[] data;

	// Read TEXTMAP
	UWMFParser parser(this, sc);
	parser.Parse();

	SetupLinks();
}
