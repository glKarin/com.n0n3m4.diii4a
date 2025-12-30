/*
** gamemap.h
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

#ifndef __GAMEMAP_H__
#define __GAMEMAP_H__

#include "tarray.h"
#include "zstring.h"
#include "textures/textures.h"
#include "dobject.h"

class Thinker;
class UWMFParser;

enum
{
	SLIDE_Normal,
	SLIDE_Split,
	SLIDE_Invert
};

enum
{
	AM_Visible = 0x1,
	AM_DontOverlay = 0x2
};

class GameMap
{
	public:
		struct Header
		{
			FString name;
			FString music;
			unsigned int width;
			unsigned int height;
			unsigned int tileSize;
		};
		struct Thing
		{
			Thing() : x(0), y(0), z(0), type(NAME_None), angle(0),
				ambush(false), patrol(false), holo(false)
			{
				skill[0] = skill[1] = skill[2] = skill[3] = false;
			}

			fixed			x, y, z;
			FName			type;
			unsigned short	angle;
			bool			ambush;
			bool			patrol;
			bool			holo;
			bool			skill[4];
		};
		struct Trigger
		{
			Trigger() : x(0), y(0), z(0), active(true), action(0),
				playerUse(false), playerCross(false), monsterUse(false),
				isSecret(false), repeatable(false)
			{
				activate[0] = activate[1] = activate[2] = activate[3] = true;
				arg[0] = arg[1] = arg[2] = arg[3] = arg[4] = 0;
			}

			unsigned int	x, y, z;
			// Stores if the trigger hasn't been used yet.
			// Note that this is set to false on for repeatable actions as well so that secrets are only accounted for once.
			bool			active;

			enum Side { East, North, West, South };
			unsigned int	action;
			bool			activate[4];
			int				arg[5];

			bool			playerUse;
			bool			playerCross;
			bool			monsterUse;
			bool			isSecret;
			bool			repeatable;
		};
		struct Tile
		{
			Tile() : offsetVertical(false), offsetHorizontal(false),
				mapped(0), dontOverlay(false)
			{
				overhead.SetInvalid();
				sideSolid[0] = sideSolid[1] = sideSolid[2] = sideSolid[3] = true;
			}

			enum Side { East, North, West, South };
			FTextureID		texture[4];
			FTextureID		overhead;
			bool			sideSolid[4];
			bool			offsetVertical;
			bool			offsetHorizontal;
			FName			soundSequence;

			unsigned int	mapped; // filter level for always visible
			bool			dontOverlay;
		};
		struct Sector
		{
			enum Flat { Floor, Ceiling };
			FTextureID	texture[2];
		};
		struct Zone
		{
			unsigned short	index;
		};
		struct Plane
		{
			const GameMap	*gm;

			unsigned int	depth;
			struct Map
			{
				Map() : tile(NULL), sector(NULL), zone(NULL), visible(false),
					amFlags(0), thinker(NULL), slideStyle(0),
					pushDirection(Tile::East), pushAmount(0),
					pushReceptor(NULL), tag(0), nexttag(NULL)
				{
					slideAmount[0] = slideAmount[1] = slideAmount[2] = slideAmount[3] = 0;
					sideSolid[0] = sideSolid[1] = sideSolid[2] = sideSolid[3] = true;
				}

				unsigned int	GetX() const;
				unsigned int	GetY() const;
				Map				*GetAdjacent(Tile::Side dir, bool opposite=false) const;
				void			SetTile(const Tile *tile);

				const Plane		*plane;

				const Tile		*tile;
				const Sector	*sector;
				const Zone		*zone;

				// So that the textures can change.
				FTextureID		texture[4];

				bool			visible;
				unsigned int	amFlags;
				TObjPtr<Thinker> thinker;
				unsigned int	slideAmount[4];
				unsigned int	slideStyle;
				bool			sideSolid[4];
				TArray<Trigger>	triggers;
				Tile::Side		pushDirection;
				unsigned int	pushAmount;
				Map				*pushReceptor;

				unsigned int	tag;
				Plane::Map		*nexttag;
			}*	map;
		};

		GameMap(const FString &map);
		~GameMap();

		bool			ActivateTrigger(Trigger &trig, Trigger::Side direction, AActor *activator);
		void			ClearVisibility();
		const Header	&GetHeader() const { return header; }
		void			GetHitlist(BYTE* hitlist) const;
		int				GetMarketLumpNum() const { return markerLump; }
		Plane::Map		*GetSpot(unsigned int x, unsigned int y, unsigned int z) const { return &GetPlane(z).map[y*header.width+x]; }
		Plane::Map		*GetSpotByTag(unsigned int tag, Plane::Map *start) const;
		const Zone		&GetZone(unsigned int index) { return zonePalette[index]; }
		bool			IsValid() const { return valid; }
		bool			IsValidTileCoordinate(unsigned int x, unsigned int y, unsigned int z) const { return x < header.width && y < header.height && z < NumPlanes(); }
		void			LoadMap(bool loadingSave);
		unsigned int	NumPlanes() const { return planes.Size(); }
		const Plane		&GetPlane(unsigned int index) const { return planes[index]; }
		void			SpawnThings() const;

		// Sound functions
		bool			CheckLink(const Zone *zone1, const Zone *zone2, bool recurse);
		void			LinkZones(const Zone *zone1, const Zone *zone2, bool open);

		// Save lookups
		const Tile		*GetTile(unsigned int index) const;
		unsigned int	GetTileIndex(const Tile *tile) const;
		const Sector	*GetSector(unsigned int index) const;
		unsigned int	GetSectorIndex(const Sector *sector) const;

		static bool		CheckMapExists(const FString &map);

		void PropagateMark();

		TMap<unsigned int, Plane::Map *> elevatorPosition;
	private:
		friend class UWMFParser;
		friend FArchive &operator<< (FArchive &, GameMap *&);

		Plane	&NewPlane();
		Trigger	&NewTrigger(unsigned int x, unsigned int y, unsigned int z);
		void	ReadMacData();
		void	ReadPlanesData();
		void	ReadUWMFData();
		void	SetSpotTag(Plane::Map *spot, unsigned int tag);
		void	SetupLinks();
		void	ScanTiles();
		bool	TraverseLink(const Zone *src, const Zone *dest);
		void	UnloadLinks();

		FString	map;

		bool	valid;
		bool	isWad;
		bool	isUWMF;
		int		markerLump;
		int		numLumps;

		class FResourceFile	*file;
		class FileReader	*lumps[1];

		// Actual map data
		Header			header;
		TArray<Tile>	tilePalette;
		TArray<Sector>	sectorPalette;
		TArray<Zone>	zonePalette;
		TArray<Thing>	things;
		TArray<Plane>	planes;
		TMap<unsigned int, Plane::Map *> tagMap;

		// Sound travel links.  zoneTraversed is temporary array for recursive
		// traversals.  zoneLinks is the table of links (counts the number of
		// links that are opened).
		bool*				zoneTraversed;
		unsigned short *zptrBack;
		unsigned short**	zoneLinks;
};

enum ESpecialThings
{
	SMT_Player1Start,

	SMT_NumThings
};
extern const FName SpecialThingNames[SMT_NumThings];

typedef GameMap::Plane::Map *	MapSpot;

// The following are mainly for easy access to enums
typedef GameMap::Plane			MapPlane;
typedef GameMap::Sector			MapSector;
typedef GameMap::Thing			MapThing;
typedef GameMap::Tile			MapTile;
typedef GameMap::Trigger		MapTrigger;
typedef GameMap::Zone			MapZone;

#include "farchive.h"
FArchive &operator<< (FArchive &arc, GameMap *&gm);
FArchive &operator<< (FArchive &arc, MapSpot &spot);
FArchive &operator<< (FArchive &arc, const MapSector *&tile);
FArchive &operator<< (FArchive &arc, const MapTile *&tile);
FArchive &operator<< (FArchive &arc, const MapZone *&zone);
FArchive &operator<< (FArchive &arc, MapTrigger &trigger);

#endif
