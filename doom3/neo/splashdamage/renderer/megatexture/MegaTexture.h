/*
===========================================================================

DarklightNG Source Code
Copyright (C) 2026 - Justin Marshall(aka IceColdDuke).

This file is part of the DarklightNG GPL source code.
This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

DarklightNG is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

DarklightNG is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

===========================================================================
*/

#ifndef __MEGATEXTURE_H__
#define __MEGATEXTURE_H__

//#define _USING_STDCXX 1 // if using C++11 std library instead of idlib
#if 1 // defined(_MULTITHREAD)
#define MEGATEXTURE_IMAGE_USING_GENERATOR 1 //karin: using image generator callback
#endif

#ifdef _USING_STDCXX
#include "idlib/remove_idlib_macros.h"

#include <mutex>
#endif

#include "MegaTexture_Compat.h"

class idFile;
class idImage;
class idRenderWorldLocal;
class idMegaTexture;
class idMegaTextureLevel;
class idMegaTextureTile;
class idMegaTextureTileLoader;
class idMegaTextureTileDecompressor;

static const int MEGA_TEXTURE_TILE_SIZE = 128;
static const int MEGA_TEXTURE_TILES_PER_LEVEL = 16;
static const int MEGA_TEXTURE_LEVEL_SIZE = MEGA_TEXTURE_TILE_SIZE * MEGA_TEXTURE_TILES_PER_LEVEL;
static const int MEGA_TEXTURE_TILE_CACHE_SIZE = 288;
static const int MEGA_TEXTURE_MIN_MIP_SIZE = 4;
static const int MEGA_TEXTURE_FILE_MAGIC = 1095189837; // little-endian "MEGA"
static const int MEGA_TEXTURE_VERSION = 9;

enum megaCompressionFormat_t {
	MEGA_COMPRESSION_NONE = 0,
	MEGA_COMPRESSION_RGB = 1,
	MEGA_COMPRESSION_RGBA = 2,
	MEGA_COMPRESSION_LUM = 3
};

enum imageCompressionFormat_t {
	IMAGE_COMPRESSION_NONE = 0x8058, // GL_RGBA8
	IMAGE_COMPRESSION_DXT1 = 0x83F0,
	IMAGE_COMPRESSION_DXT5 = 0x83F3
};

struct tileData_t {
	tileData_t();
	~tileData_t();

	void			Invalidate();
	bool			IsValid() const;

	int				x;
	int				y;
	int				tileBase;
	byte *				pic;
	idLinkList<tileData_t>	node;
};

class idMegaTextureTile {
public:
					idMegaTextureTile();
					~idMegaTextureTile();

	void			Init( idMegaTextureLevel &level, int localX, int localY );
	void			PostInit();
	void			ReleaseTileData();
	bool			Update( int globalX, int globalY, bool force );
	bool			SetCachedTileData( idMegaTexture *megaTexture, int tileBase, int tilesPerAxis );
	void			Upload( idMegaTexture *megaTexture );

	void			SetDirty() { dirty = true; }
	bool			IsDirty() const { return dirty; }
	idLinkList<idMegaTextureTile> &GetDirtyNode() { return dirtyNode; }
	int				GetGlobalX() const { return globalX; }
	int				GetGlobalY() const { return globalY; }
	int				GetTileNum() const;
	tileData_t *	GetTileData() const { return tileData; }
	void			SetTileData( tileData_t *data ) { tileData = data; }
	void			SetLoaded( bool value ) { loaded = value; }
	bool			IsLoaded() const;
	byte *			GetCompressedTileData() const;
	byte *			GetChildCompressedTileData( int index ) const;

private:
	friend class idMegaTextureLevel;
	friend class idMegaTextureTileLoader;
	friend class idMegaTextureTileDecompressor;

	void			Purge();

	idMegaTextureLevel *level;
	idLinkList<idMegaTextureTile> dirtyNode;
	int				localX;
	int				localY;
	int				globalX;
	int				globalY;
	byte *				compressedTileData;
	byte *				childCompressedTileData[4];
	tileData_t *	tileData;
	bool			dirty;
	bool			loaded;
};

class idMegaTextureLevel {
public:
					idMegaTextureLevel();
					~idMegaTextureLevel();

	void			Init( idMegaTexture &megaTexture, int levelNum, int tileBase,
						int tilesPerAxis, bool activateImage,
						megaCompressionFormat_t compressionFormat,
						int maxCompressedTileSize );
	void			PostInit();
	void			Reset();
	int				GetUsedMemory() const { return usedMemory; }
	void			AddUsedMemory( int bytes ) { usedMemory += bytes; }
	bool			UpdateForCenter( const idVec2 &center, bool force );
	bool			UploadTiles( int time );
	void			AddDirtyTile( idMegaTextureTile *tile );
	void			RemoveDirtyTile( idMegaTextureTile *tile );
	idMegaTextureTile *GetDirtyTile();

	int				GetLevelNum() const { return levelNum; }
	int				GetTileBase() const { return tileBase; }
	int				GetTilesPerAxis() const { return tilesPerAxis; }
	megaCompressionFormat_t GetMegaCompressionFormat() const { return megaCompressionFormat; }
	bool			IsInterleaved() const { return isInterleaved; }
	const float *	GetParms() const { return parms; }
	idImage *		GetImage() const { return image; }
	idMegaTextureTile *GetTileLocal( int x, int y ) { return &tiles[y][x]; }
	idMegaTexture *	GetMegaTexture() const { return megaTexture; }
	int				GetFadeTime() const { return fadeTime; }
	bool			ImageIsValid() const { return imageValid; }
	bool			IsAlwaysCached() const { return alwaysCached; }
	byte *			GetCompressedTileData( int globalX, int globalY ) const;
	tileData_t *	FindCachedTile( int tileBase, int x, int y );
	void			RemoveCachedTile( int tileBase, int x, int y );
	tileData_t *	GetAvailableTile();
	void			ReleaseTile( tileData_t *tile );
	int				GetMaxCompressedTileSize() const { return maxCompressedTileSize; }
#ifdef _MULTITHREAD //karin: add image allocate queue in non-OpenGL thread if multithreading-rendering, else call OpenGL directly
	void			LoadEmptyLevelImage( idImage *target );
#endif

private:
	friend class idMegaTexture;
	friend class idMegaTextureTile;
	friend class idMegaTextureTileLoader;
	friend class idMegaTextureTileDecompressor;

	void			Purge();
	void			InitTileCache();
	void			ShutdownTileCache();
	void			EmptyLevelImage( idImage *image );

	idMegaTexture *	megaTexture;
	int				levelNum;
	int				usedMemory;
	idImage *		image;
	bool			imageValid;
	int				tileBase;
	int				tilesPerAxis;
	megaCompressionFormat_t megaCompressionFormat;
	bool			isInterleaved;
	int				maxCompressedTileSize;
	float			parms[4];
	float			newParms[2];
	int				fadeTime;
	idMegaTextureTile tiles[MEGA_TEXTURE_TILES_PER_LEVEL][MEGA_TEXTURE_TILES_PER_LEVEL];
	bool			alwaysCached;
	byte *			compressedData;
	byte **			compressedTiles;
	int				compressedTilesPerAxis;
	tileData_t *	tileCache;
	int				tileCacheSize;
	idLinkList<tileData_t> availableTiles;
	idLinkList<tileData_t> activeTiles;
	bool			dirty;
	idLinkList<idMegaTextureTile> dirtyTiles;
#ifdef MEGATEXTURE_IMAGE_USING_GENERATOR //karin: using image generator callback
	idImageGeneratorFunctor<idMegaTextureLevel> imageFunctor;
#endif
};

class idMegaTexture {
public:
					idMegaTexture();
					~idMegaTexture();

	bool			InitFromMegaFile( const char *name );
	int				GetVersion() const { return version; }
	void			Load();
	void			Touch();
	void			Purge();
	void			Reset();
	bool			IsLoaded() const { return !purged; }
	const char *	GetName() const { return name.c_str(); }
	unsigned int	GetPureServerChecksum( unsigned int offset );
	void			UpdateMapping( const idRenderWorldLocal *world );
	void			UpdateForViewOrigin( const idVec3 &viewOrigin, int time );

	const byte *	GetNullTileData() const { return nullTileData; }
	const byte *	GetGridTileData() const { return gridTileData; }
	int				GetNumLevels() const { return numLevels; }
	idMegaTextureLevel *GetLevel( int index ) const;
	int				SeekToTile( int tileNum );
	idFile *		GetFile() const { return file; }
	bool			UseImageCompression() const { return useImageCompression; }
	imageCompressionFormat_t GetImageCompressionFormat() const { return imageCompressionFormat; }
	int				GetTileOffset( int tileNum ) const;
	int				GetTileDataSize( int tileNum ) const;
	byte *			GetTileRecompressionScratch() const { return tileRecompressionScratch; }
	void			SetLevelLoadReferenced( bool value ) { levelLoadReferenced = value; }
	bool			IsLevelLoadReferenced() const { return levelLoadReferenced; }
	void			SetReferencedOutsideLevelLoad( bool value ) { referencedOutsideLevelLoad = value; }
	bool			IsReferencedOutsideLevelLoad() const { return referencedOutsideLevelLoad; }
#ifdef _USING_STDCXX
	std::recursive_mutex &GetLock() { return lock; }
#else
	sdRecursiveLock &GetLock() { return lock; }
#endif
	void			ForceUpdate();
	bool			IsForcedUpdate() const { return forcedUpdate; }
	void			OnUseMegaTextureCompressionChange();
	static int		TotalStoredTileCount( int resolution );

	// Darklight renderer adapter. Shader translation is intentionally separate
	// from the runtime replacement.
	void			SetMappingForSurface( const srfTriangles_t *tri );
	void			BindForViewOrigin( const idVec3 origin );
	void			Unbind();
	void			ReloadImages();
	void			PrintInfo() const;
	bool			DebugDecodeTile( int level, int x, int y, const char *outputName );
	void			BindRenderProgram(const sdRenderProgram *program) {
		renderProgram = program;
	}

	static void		MegaTestStreamingPerformance_f( const idCmdArgs &args );
	static void		MegaShowMemoryUsage_f( const idCmdArgs &args );
	static void		MegaTextureInfo_f( const idCmdArgs &args );
	static void		MegaTextureLoad_f( const idCmdArgs &args );
	static void		MegaTextureDecodeTile_f( const idCmdArgs &args );

	static idCVar	r_showMegaTextureLevels;
	static idCVar	r_skipMegaTexture;
	static idCVar	r_skipMegaTextureUpload;
	static idCVar	r_useMegaTextureImageCompression;
	static idCVar	r_detailTexture;
	static idCVar	r_detailRatio;
	static idCVar	r_detailFade;
	static idCVar	r_megaStreamBlocks;
	static idCVar	r_megaFadeTime;
	static idCVar	r_megaStreamFromDVD;
	static idCVar	r_megaUpscale;

private:
	friend class idMegaTextureLevel;
	friend class idMegaTextureTile;
	friend class idMegaTextureTileLoader;
	friend class idMegaTextureTileDecompressor;

	bool			OpenFile();
	bool			CloseFile();
	void			GenerateNullTileData();
	void			GenerateGridTileData();
	void			AllocRecompressionScratch();
	void			LoadDetailTexture();
	void			UpdateLevelForViewOrigin( idMegaTextureLevel *level, int index, int time );
	void			SetViewOrigin( const idVec3 &viewOrigin );
	bool			UploadTiles( int time );
	void			TestStreamingPerformance( const idCmdArgs &args );
	void			ShowMemoryUsage( const idCmdArgs &args );
	bool			ReadTileData( int tileNum, byte *destination, int destinationBytes );

	idStr			name;
	idStr			fileName;
	int				version;
	int				resolution;
	bool			levelLoadReferenced;
	bool			referencedOutsideLevelLoad;
	bool			purged;
	idFile *		file;
	int				lastTileOffset;
	imageCompressionFormat_t imageCompressionFormat;
	bool			useImageCompression;
	bool			forcedUpdate;
	idImage *		detailTexture;
	idImage *		detailTextureMask;
	int				lastUsedFrame;
	const idRenderWorldLocal *currentWorld;
	idVec3			currentViewOrigin;
	int				tilesPerAxis;
	int				numLevels;
	idMegaTextureLevel *levels;
	idMegaTextureLevel *upscaleLevel;
	idBounds		stGridBounds;
	int				stGridWidth;
	int				stGridHeight;
	const idVec2 *	stGrid;
	int *			tileIndexMap;
	int *			tileIndexedDataSizes;
	byte *			nullTileData;
	byte *			gridTileData;
	byte *			tileRecompressionScratch;
	int				lastShaderQuality;
#ifdef _USING_STDCXX
	mutable std::recursive_mutex lock;
#else
	mutable sdRecursiveLock lock;
#endif

	const srfTriangles_t *currentTriMapping;
	float			localViewToTextureCenter[2][4];
	float			shaderLevelOpacity[7];

	const sdRenderProgram *renderProgram;
};

extern idMegaTextureTileLoader *megaTextureTileLoader;
extern idMegaTextureTileDecompressor *megaTextureTileDecompressor;

int GetCompressedTotalKiloBytesReadPerSecond();
int GetPercentageTilesReady( int levelNum );

void R_InitMegaTextureSystem(void);
void R_ShutdownMegaTextureSystem(void);

#ifdef _USING_STDCXX
#include "idlib/remove_idlib_macros.h"
#endif

#endif
