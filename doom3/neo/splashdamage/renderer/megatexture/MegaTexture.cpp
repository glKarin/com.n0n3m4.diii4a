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

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "../renderer/tr_local.h"
#include "MegaTexture.h"
#include "MegaTextureCodec.h"
#include "MegaTextureTileLoader.h"
#include "MegaTextureTileDecompressor.h"
#include "renderer/RenderProgram.h"

#ifdef _USING_STDCXX
#include <chrono>
#include <thread>
#endif

idCVar idMegaTexture::r_showMegaTextureLevels( "r_showMegaTextureLevels", "0", CVAR_RENDERER | CVAR_BOOL, "show alternating MegaTexture levels" );
idCVar idMegaTexture::r_skipMegaTexture( "r_skipMegaTexture", "0", CVAR_RENDERER | CVAR_BOOL, "skip MegaTexture view updates" );
idCVar idMegaTexture::r_skipMegaTextureUpload( "r_skipMegaTextureUpload", "0", CVAR_RENDERER | CVAR_BOOL, "skip MegaTexture tile uploads" );
idCVar idMegaTexture::r_useMegaTextureImageCompression( "r_useMegaTextureImageCompression", "0", CVAR_RENDERER | CVAR_BOOL, "recompress decoded MegaTexture tiles to DXT" );
idCVar idMegaTexture::r_detailTexture( "r_detailTexture", "1", CVAR_RENDERER | CVAR_BOOL, "Detail texture on landscape" );
idCVar idMegaTexture::r_detailRatio( "r_detailRatio", "4", CVAR_RENDERER | CVAR_FLOAT, "Ratio of detail texture to main texture" );
idCVar idMegaTexture::r_detailFade( "r_detailFade", "0.5", CVAR_RENDERER | CVAR_FLOAT, "Distance fading control (reloadImages all needed)", 0.0f, 1.0f );
idCVar idMegaTexture::r_megaStreamBlocks( "r_megaStreamBlocks", "16", CVAR_RENDERER | CVAR_INTEGER, "legacy ETQW 32 KiB stream window size" );
idCVar idMegaTexture::r_megaFadeTime( "r_megaFadeTime", "250", CVAR_RENDERER | CVAR_INTEGER, "MegaTexture level transition time in milliseconds" );
idCVar idMegaTexture::r_megaStreamFromDVD( "r_megaStreamFromDVD", "0", CVAR_RENDERER | CVAR_BOOL, "retain ETQW optical-media streaming policy flag" );
idCVar idMegaTexture::r_megaUpscale( "r_megaUpscale", "0", CVAR_RENDERER | CVAR_BOOL, "enable the optional synthetic finest level" );

static int MegaTextureTileChainBytes( imageCompressionFormat_t format ) {
	int bytes = 0;
	for ( int size = MEGA_TEXTURE_TILE_SIZE; size >= MEGA_TEXTURE_MIN_MIP_SIZE; size >>= 1 ) {
		if ( format == IMAGE_COMPRESSION_DXT1 ) {
			bytes += 8 * ( ( size + 3 ) / 4 ) * ( ( size + 3 ) / 4 );
		} else if ( format == IMAGE_COMPRESSION_DXT5 ) {
			bytes += 16 * ( ( size + 3 ) / 4 ) * ( ( size + 3 ) / 4 );
		} else {
			bytes += size * size * 4;
		}
	}
	return bytes;
}

static bool MegaTextureIsPowerOfTwo( int value ) {
	return value > 0 && ( value & ( value - 1 ) ) == 0;
}

tileData_t::tileData_t() : x( -1 ), y( -1 ), tileBase( -1 ), pic( NULL ) {
	node.SetOwner( this );
}

tileData_t::~tileData_t() {
	node.Remove();
}

void tileData_t::Invalidate() {
	x = y = tileBase = -1;
}

bool tileData_t::IsValid() const {
	return x >= 0 && y >= 0 && tileBase >= 0;
}

idMegaTextureTile::idMegaTextureTile() :
	level( NULL ), localX( 0 ), localY( 0 ), globalX( -99999 ), globalY( -99999 ),
	compressedTileData( NULL ), tileData( NULL ), dirty( false ), loaded( false ) {
	dirtyNode.SetOwner( this );
	memset( childCompressedTileData, 0, sizeof( childCompressedTileData ) );
}

idMegaTextureTile::~idMegaTextureTile() {
	Purge();
}

void idMegaTextureTile::Init( idMegaTextureLevel &owner, int x, int y ) {
	Purge();
	level = &owner;
	localX = x;
	localY = y;
	globalX = globalY = -99999;
	dirty = loaded = false;
	if ( !level->alwaysCached && !level->isInterleaved ) {
		compressedTileData = new byte[level->maxCompressedTileSize + 3];
		level->AddUsedMemory( level->maxCompressedTileSize + 3 );
	}
}

void idMegaTextureTile::PostInit() {
	const int childLevelNumber = level ? level->levelNum - 1 : -1;
	if ( childLevelNumber >= 0 ) {
		idMegaTextureLevel *childLevel = level->megaTexture->GetLevel( childLevelNumber );
		if ( childLevel && childLevel->isInterleaved ) {
			for ( int i = 0; i < 4; ++i ) {
				if ( !childCompressedTileData[i] ) {
					childCompressedTileData[i] = new byte[childLevel->maxCompressedTileSize + 3];
					level->AddUsedMemory( childLevel->maxCompressedTileSize + 3 );
				}
			}
		}
	}
	if ( dirty ) {
		SetCachedTileData( level->megaTexture, level->tileBase, level->tilesPerAxis );
	}
}

void idMegaTextureTile::Purge() {
	ReleaseTileData();
	delete[] compressedTileData;
	compressedTileData = NULL;
	for ( int i = 0; i < 4; ++i ) {
		delete[] childCompressedTileData[i];
		childCompressedTileData[i] = NULL;
	}
	loaded = dirty = false;
}

void idMegaTextureTile::ReleaseTileData() {
	if ( tileData && level ) {
		level->ReleaseTile( tileData );
	}
	tileData = NULL;
	dirtyNode.Remove();
}

bool idMegaTextureTile::Update( int x, int y, bool force ) {
	if ( !force && globalX == x && globalY == y ) {
		return false;
	}
	if ( ( x & ( MEGA_TEXTURE_TILES_PER_LEVEL - 1 ) ) != localX ||
		 ( y & ( MEGA_TEXTURE_TILES_PER_LEVEL - 1 ) ) != localY ) {
		common->Error( "idMegaTextureTile::Update: bad coordinate modulo" );
	}
	ReleaseTileData();
	globalX = x;
	globalY = y;
	dirty = true;
	loaded = false;
	level->dirty = true;
	return true;
}

bool idMegaTextureTile::SetCachedTileData( idMegaTexture *mega, int base, int axis ) {
	if ( !dirty ) {
		return true;
	}
	if ( globalX < 0 || globalY < 0 || globalX >= axis || globalY >= axis ) {
		tileData = NULL;
		return false;
	}
	tileData = level->FindCachedTile( base, globalX, globalY );
	if ( tileData ) {
		loaded = true;
		return true;
	}
	if ( level->isInterleaved && level->levelNum + 1 < mega->numLevels ) {
		idMegaTextureLevel &parentLevel = mega->levels[level->levelNum + 1];
		idMegaTextureTile *parent = parentLevel.GetTileLocal( ( globalX >> 1 ) & 15, ( globalY >> 1 ) & 15 );
		if ( !parent->IsLoaded() && !parent->GetDirtyNode().InList() ) {
			parentLevel.AddDirtyTile( parent );
		}
	}
	level->AddDirtyTile( this );
	return false;
}

int idMegaTextureTile::GetTileNum() const {
	if ( !level || globalX < 0 || globalY < 0 || globalX >= level->tilesPerAxis || globalY >= level->tilesPerAxis ) {
		return -1;
	}
	return level->tileBase + globalY * level->tilesPerAxis + globalX;
}

bool idMegaTextureTile::IsLoaded() const {
	if ( !level ) return false;
	if ( level->alwaysCached ) return true;
	if ( level->isInterleaved && level->levelNum + 1 < level->megaTexture->numLevels ) {
		idMegaTextureLevel &parentLevel = level->megaTexture->levels[level->levelNum + 1];
		const idMegaTextureTile *parent = parentLevel.GetTileLocal( ( globalX >> 1 ) & 15, ( globalY >> 1 ) & 15 );
		return parent->IsLoaded();
	}
	return loaded;
}

byte *idMegaTextureTile::GetCompressedTileData() const {
	if ( !level ) {
		return NULL;
	}
	if ( level->isInterleaved && level->levelNum + 1 < level->megaTexture->numLevels ) {
		idMegaTextureLevel &parentLevel = level->megaTexture->levels[level->levelNum + 1];
		idMegaTextureTile *parent = parentLevel.GetTileLocal( ( globalX >> 1 ) & 15, ( globalY >> 1 ) & 15 );
		return parent->childCompressedTileData[( globalX & 1 ) + 2 * ( globalY & 1 )];
	}
	if ( compressedTileData ) {
		return compressedTileData;
	}
	return level->GetCompressedTileData( globalX, globalY );
}

byte *idMegaTextureTile::GetChildCompressedTileData( int index ) const {
	return index >= 0 && index < 4 ? childCompressedTileData[index] : NULL;
}

void idMegaTextureTile::Upload( idMegaTexture *mega ) {
	if ( !dirty ) {
		return;
	}
	if ( idMegaTexture::r_skipMegaTextureUpload.GetBool() ) {
		dirty = false;
		return;
	}
	const byte *data = tileData ? tileData->pic : mega->GetNullTileData();
	int offset = 0;
	int mip = 0;
	for ( int size = MEGA_TEXTURE_TILE_SIZE; size >= MEGA_TEXTURE_MIN_MIP_SIZE; size >>= 1, ++mip ) {
		if ( mega->GetImageCompressionFormat() == IMAGE_COMPRESSION_DXT1 ) {
			const int bytes = 8 * ( ( size + 3 ) / 4 ) * ( ( size + 3 ) / 4 );
			qglCompressedTexSubImage2DARB( GL_TEXTURE_2D, mip, localX * size, localY * size,
				size, size, GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, bytes, data + offset );
			offset += bytes;
		} else if ( mega->GetImageCompressionFormat() == IMAGE_COMPRESSION_DXT5 ) {
			const int bytes = 16 * ( ( size + 3 ) / 4 ) * ( ( size + 3 ) / 4 );
			qglCompressedTexSubImage2DARB( GL_TEXTURE_2D, mip, localX * size, localY * size,
				size, size, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, bytes, data + offset );
			offset += bytes;
		} else {
			qglTexSubImage2D( GL_TEXTURE_2D, mip, localX * size, localY * size,
				size, size, GL_RGBA, GL_UNSIGNED_BYTE, data + offset );
			offset += size * size * 4;
		}
	}
	dirty = false;
}

idMegaTextureLevel::idMegaTextureLevel() :
	megaTexture( NULL ), levelNum( -1 ), usedMemory( 0 ), image( NULL ), imageValid( false ),
	tileBase( 0 ), tilesPerAxis( 0 ), megaCompressionFormat( MEGA_COMPRESSION_NONE ),
	isInterleaved( false ), maxCompressedTileSize( 0 ), fadeTime( 0 ), alwaysCached( false ),
	compressedData( NULL ), compressedTiles( NULL ), compressedTilesPerAxis( 0 ),
	tileCache( NULL ), tileCacheSize( 0 ), dirty( false ) {
	parms[0] = -1.0f; parms[1] = parms[2] = 0.0f; parms[3] = 1.0f;
	newParms[0] = newParms[1] = 0.0f;
#ifdef MEGATEXTURE_IMAGE_USING_GENERATOR //karin: using image generator callback
	imageFunctor.Init(this, &idMegaTextureLevel::EmptyLevelImage);
#endif
}

idMegaTextureLevel::~idMegaTextureLevel() {
	Purge();
}

void idMegaTextureLevel::Purge() {
	for ( int y = 0; y < 16; ++y ) {
		for ( int x = 0; x < 16; ++x ) {
			tiles[y][x].Purge();
		}
	}
	dirtyTiles.Clear();
	ShutdownTileCache();
	delete[] compressedData;
	delete[] compressedTiles;
	compressedData = NULL;
	compressedTiles = NULL;
	if ( image ) {
		image->PurgeImage();
	}
	imageValid = false;
	dirty = false;
}

void idMegaTextureLevel::Init( idMegaTexture &mega, int number, int base, int axis,
		bool activateImage, megaCompressionFormat_t format, int maxSize ) {
	Purge();
	megaTexture = &mega;
	levelNum = number;
	tileBase = base;
	tilesPerAxis = axis;
	megaCompressionFormat = format;
	maxCompressedTileSize = maxSize;
	isInterleaved = number == 0 && mega.numLevels > 4;
	alwaysCached = number >= 2;
	compressedTilesPerAxis = axis;
	usedMemory = sizeof( *this );
	parms[0] = -1.0f;
	parms[1] = parms[2] = 0.0f;
	parms[3] = axis / 16.0f;
	newParms[0] = newParms[1] = 0.0f;

	idStr imageName = va( "_mega_%s_%d", mega.GetName(), number );
	image = globalImages->GetImage( imageName );
	if ( !image ) {
		image = globalImages->AllocImage( imageName );
#ifdef MEGATEXTURE_IMAGE_USING_GENERATOR //karin: using image generator callback
		image->generatorFunctor = &imageFunctor;
#endif
	}
	// These atlases are populated by the MegaTexture streamer, not loaded from
	// files.  Keep the image manager from purging them at EndLevelLoad and then
	// trying to resolve their synthetic names as ordinary image programs.
	if ( image ) {
		image->referencedOutsideLevelLoad = true;
		image->levelLoadReferenced = true;
	}
	if ( activateImage && image ) {
#ifdef MEGATEXTURE_IMAGE_USING_GENERATOR
#ifdef _MULTITHREAD //karin: add image allocate queue in non-OpenGL thread if multithreading-rendering, else call OpenGL directly
		/*if (multithreadActive) {
			renderThread->AddAllocList( image, false, false );
		}
		else*/
		image->ActuallyLoadImage(false, false, renderThread->IsActive());
#else
		target->ActuallyLoadImage(false, false);
#endif
#else
		EmptyLevelImage( image ); // not support multithreading rendering
#endif
	}

	const int tileCount = axis * axis;
	compressedTiles = new byte *[tileCount];
	memset( compressedTiles, 0, tileCount * sizeof( compressedTiles[0] ) );
	usedMemory += tileCount * sizeof( compressedTiles[0] );
	if ( alwaysCached && tileCount > 0 ) {
		int totalBytes = 0;
		for ( int i = 0; i < tileCount; ++i ) {
			totalBytes += mega.GetTileDataSize( base + i ) + 3;
		}
		compressedData = new byte[totalBytes];
		usedMemory += totalBytes;
		byte *cursor = compressedData;
		for ( int i = 0; i < tileCount; ++i ) {
			const int bytes = mega.GetTileDataSize( base + i ) + 3;
			if ( !mega.ReadTileData( base + i, cursor, bytes ) ) {
				memset( cursor, 0, bytes );
			}
			compressedTiles[i] = cursor;
			cursor += bytes;
		}
	}
	InitTileCache();
	for ( int y = 0; y < 16; ++y ) {
		for ( int x = 0; x < 16; ++x ) {
			tiles[y][x].Init( *this, x, y );
		}
	}
}

void idMegaTextureLevel::PostInit() {
	for ( int y = 0; y < 16; ++y ) {
		for ( int x = 0; x < 16; ++x ) {
			tiles[y][x].PostInit();
		}
	}
}

void idMegaTextureLevel::Reset() {
	for ( int y = 0; y < 16; ++y ) {
		for ( int x = 0; x < 16; ++x ) {
			tiles[y][x].ReleaseTileData();
			tiles[y][x].globalX = tiles[y][x].globalY = -99999;
			tiles[y][x].dirty = tiles[y][x].loaded = false;
		}
	}
	dirtyTiles.Clear();
	dirty = false;
	imageValid = false;
	parms[0] = -1.0f;
}

void idMegaTextureLevel::InitTileCache() {
	ShutdownTileCache();
	tileCacheSize = MEGA_TEXTURE_TILE_CACHE_SIZE;
	tileCache = new tileData_t[tileCacheSize];
	const int dataBytes = MegaTextureTileChainBytes( megaTexture->GetImageCompressionFormat() );
	usedMemory += tileCacheSize * sizeof( tileData_t );
	for ( int i = 0; i < tileCacheSize; ++i ) {
		tileCache[i].pic = new byte[dataBytes];
		tileCache[i].Invalidate();
		tileCache[i].node.AddToEnd( availableTiles );
		usedMemory += dataBytes;
	}
}

void idMegaTextureLevel::ShutdownTileCache() {
	availableTiles.Clear();
	activeTiles.Clear();
	if ( tileCache ) {
		for ( int i = 0; i < tileCacheSize; ++i ) {
			delete[] tileCache[i].pic;
			tileCache[i].pic = NULL;
		}
		delete[] tileCache;
	}
	tileCache = NULL;
	tileCacheSize = 0;
}

void idMegaTextureLevel::EmptyLevelImage( idImage *target ) {
	const int pixels = MEGA_TEXTURE_LEVEL_SIZE * MEGA_TEXTURE_LEVEL_SIZE;
	byte *data = new byte[pixels * 4];
	for ( int i = 0; i < pixels; ++i ) {
		data[i * 4 + 0] = 0;
		data[i * 4 + 1] = 0;
		data[i * 4 + 2] = 0;
		data[i * 4 + 3] = 0;
	}
	target->GenerateImage( data, MEGA_TEXTURE_LEVEL_SIZE, MEGA_TEXTURE_LEVEL_SIZE,
		TF_DEFAULT, false, TR_REPEAT, TD_HIGH_QUALITY );
	delete[] data;
	imageValid = false;
}

#ifdef _MULTITHREAD //karin: add image allocate queue in non-OpenGL thread if multithreading-rendering, else call OpenGL directly
void idMegaTextureLevel::LoadEmptyLevelImage( idImage *target ) {
#ifdef MEGATEXTURE_IMAGE_USING_GENERATOR
#ifdef _MULTITHREAD
		target->ActuallyLoadImage(false, false, renderThread->IsActive());
#else
		target->ActuallyLoadImage(false, false);
#endif
#else
		EmptyLevelImage(target); // not support multithreading rendering
#endif
}
#endif

bool idMegaTextureLevel::UpdateForCenter( const idVec2 &center, bool force ) {
	int corner[2];
	int localOffset[2];
	if ( tilesPerAxis > 16 ) {
		for ( int i = 0; i < 2; ++i ) {
			corner[i] = idMath::Ftoi( ( center[i] * parms[3] - 0.5f ) * 16.0f + 0.5f );
			localOffset[i] = corner[i] & 15;
			newParms[i] = -corner[i] / 16.0f;
		}
	} else {
		corner[0] = corner[1] = 0;
		localOffset[0] = localOffset[1] = 0;
		newParms[0] = newParms[1] = 0.25f;
		parms[3] = 0.25f;
	}
	bool needsStreaming = false;
	for ( int y = 0; y < 16; ++y ) {
		const int globalY = corner[1] + ( ( y - localOffset[1] ) & 15 );
		for ( int x = 0; x < 16; ++x ) {
			const int globalX = corner[0] + ( ( x - localOffset[0] ) & 15 );
			idMegaTextureTile &tile = tiles[globalY & 15][globalX & 15];
			if ( tile.Update( globalX, globalY, force ) ) {
				if ( levelNum >= 0 && !tile.SetCachedTileData( megaTexture, tileBase, tilesPerAxis ) ) {
					needsStreaming = true;
				}
			}
		}
	}
	return needsStreaming;
}

bool idMegaTextureLevel::UploadTiles( int time ) {
	if ( !dirty ) {
		return true;
	}
	if ( !dirtyTiles.IsListEmpty() ) {
		return false;
	}
	int dirtyCount = 0;
	for ( int y = 0; y < 16; ++y ) {
		for ( int x = 0; x < 16; ++x ) {
			dirtyCount += tiles[y][x].dirty ? 1 : 0;
		}
	}
	if ( dirtyCount > 128 ) {
		fadeTime = time;
	}
	parms[0] = newParms[0];
	parms[1] = newParms[1];
	if ( image ) {
		image->BindFragment();
	}
	for ( int y = 0; y < 16; ++y ) {
		for ( int x = 0; x < 16; ++x ) {
			tiles[y][x].Upload( megaTexture );
		}
	}
	imageValid = true;
	dirty = false;
	return true;
}

void idMegaTextureLevel::AddDirtyTile( idMegaTextureTile *tile ) {
	if ( !tile || tile->dirtyNode.InList() ) {
		return;
	}
	tile->dirtyNode.AddToEnd( dirtyTiles );
	if ( megaTextureTileLoader ) {
		megaTextureTileLoader->SignalThread();
	}
}

void idMegaTextureLevel::RemoveDirtyTile( idMegaTextureTile *tile ) {
	if ( tile ) {
		tile->dirtyNode.Remove();
	}
}

idMegaTextureTile *idMegaTextureLevel::GetDirtyTile() {
	return dirtyTiles.Next();
}

byte *idMegaTextureLevel::GetCompressedTileData( int x, int y ) const {
	if ( !compressedTiles || compressedTilesPerAxis <= 0 || x < 0 || y < 0 ) {
		return NULL;
	}
	byte *data = compressedTiles[( y % compressedTilesPerAxis ) * compressedTilesPerAxis + ( x % compressedTilesPerAxis )];
	if ( data ) return data;
	const idMegaTextureTile *tile = &tiles[y & 15][x & 15];
	if ( tile->globalX == x && tile->globalY == y && tile->loaded ) return tile->compressedTileData;
	return NULL;
}

tileData_t *idMegaTextureLevel::FindCachedTile( int base, int x, int y ) {
	for ( tileData_t *tile = activeTiles.Next(); tile; tile = tile->node.Next() ) {
		if ( tile->tileBase == base && tile->x == x && tile->y == y ) {
			tile->node.AddToEnd( activeTiles );
			return tile;
		}
	}
	return NULL;
}

void idMegaTextureLevel::RemoveCachedTile( int base, int x, int y ) {
	tileData_t *tile = FindCachedTile( base, x, y );
	if ( tile ) {
		ReleaseTile( tile );
	}
}

tileData_t *idMegaTextureLevel::GetAvailableTile() {
	tileData_t *tile = availableTiles.Next();
	if ( !tile ) {
		tile = activeTiles.Next();
		if ( tile ) {
			for ( int y = 0; y < 16; ++y ) {
				for ( int x = 0; x < 16; ++x ) {
					if ( tiles[y][x].tileData == tile ) {
						tiles[y][x].tileData = NULL;
					}
				}
			}
		}
	}
	if ( tile ) {
		tile->node.AddToEnd( activeTiles );
		tile->Invalidate();
	}
	return tile;
}

void idMegaTextureLevel::ReleaseTile( tileData_t *tile ) {
	if ( !tile ) {
		return;
	}
	tile->Invalidate();
	tile->node.AddToEnd( availableTiles );
}

idMegaTexture::idMegaTexture() :
	version( 0 ), resolution( 0 ), levelLoadReferenced( false ), referencedOutsideLevelLoad( false ),
	purged( true ), file( NULL ), lastTileOffset( 0 ), imageCompressionFormat( IMAGE_COMPRESSION_NONE ),
	useImageCompression( false ), forcedUpdate( false ), detailTexture( NULL ), detailTextureMask( NULL ),
	lastUsedFrame( -1 ), currentWorld( NULL ), tilesPerAxis( 0 ), numLevels( 0 ), levels( NULL ),
	upscaleLevel( NULL ), stGridWidth( 0 ), stGridHeight( 0 ), stGrid( NULL ), tileIndexMap( NULL ),
	tileIndexedDataSizes( NULL ), nullTileData( NULL ), gridTileData( NULL ),
	tileRecompressionScratch( NULL ), lastShaderQuality( 0 ), currentTriMapping( NULL ) {
	lastShaderQuality = r_shaderQuality.GetInteger();
	currentViewOrigin.Set( 262144.0f, 262144.0f, 262144.0f );
	memset( localViewToTextureCenter, 0, sizeof( localViewToTextureCenter ) );
	for ( int i = 0; i < 7; ++i ) shaderLevelOpacity[i] = 1.0f;
}

idMegaTexture::~idMegaTexture() {
	Purge();
}

bool idMegaTexture::InitFromMegaFile( const char *fileBase ) {
	if ( !fileBase || !fileBase[0] ) {
		return false;
	}
	name = fileBase;
	/*name.BackSlashesToSlashes();
	name.StripPath();*/ //k
	name.StripFileExtension();
	Load();
	return IsLoaded();
}

bool idMegaTexture::OpenFile() {
	CloseFile();
	fileName = "megatextures/";
	fileName += name;
	fileName.StripFileExtension();
	fileName += ".mega";
	file = fileSystem->OpenFileRead( fileName );
	if ( !file ) {
		common->Warning( "idMegaTexture::OpenFile: failed to open '%s'", fileName.c_str() );
		return false;
	}
	int magic = 0;
	if ( file->ReadInt( magic ) != sizeof( magic ) || file->ReadInt( version ) != sizeof( version ) ) {
		common->Warning( "idMegaTexture::OpenFile: truncated header in '%s'", fileName.c_str() );
		CloseFile();
		return false;
	}
	if ( magic != MEGA_TEXTURE_FILE_MAGIC || ( version != 8 && version != MEGA_TEXTURE_VERSION ) ) {
		common->Warning( "idMegaTexture::OpenFile: unsupported header in '%s' (magic %d, version %d)", fileName.c_str(), magic, version );
		CloseFile();
		return false;
	}
	return true;
}

bool idMegaTexture::CloseFile() {
	if ( file ) {
		fileSystem->CloseFile( file );
		file = NULL;
	}
	return true;
}

void idMegaTexture::Load() {
	Purge();
	if ( !OpenFile() ) {
		return;
	}
	int fileCompression = 0;
	if ( file->ReadInt( resolution ) != 4 || !MegaTextureIsPowerOfTwo( resolution ) || resolution < MEGA_TEXTURE_LEVEL_SIZE || resolution % 128 != 0 ) {
		common->Warning( "idMegaTexture::Load: invalid resolution in '%s'", fileName.c_str() );
		CloseFile();
		return;
	}
	tilesPerAxis = resolution / MEGA_TEXTURE_TILE_SIZE;
	numLevels = 1;
	for ( int ratio = resolution / MEGA_TEXTURE_LEVEL_SIZE; ratio > 1; ratio >>= 1 ) {
		++numLevels;
	}
	if ( numLevels <= 0 || numLevels > 16 || file->ReadInt( fileCompression ) != 4 ) {
		common->Warning( "idMegaTexture::Load: invalid level header in '%s'", fileName.c_str() );
		CloseFile();
		return;
	}
	megaCompressionFormat_t *formats = new megaCompressionFormat_t[numLevels];
	int *maxSizes = new int[numLevels];
	bool headerOkay = true;
	for ( int i = 0; i < numLevels; ++i ) {
		int value = 0;
		headerOkay &= file->ReadInt( value ) == 4 && value >= MEGA_COMPRESSION_NONE && value <= MEGA_COMPRESSION_LUM;
		formats[i] = (megaCompressionFormat_t)value;
	}
	if ( version == 8 ) {
		int value = 0;
		headerOkay &= file->ReadInt( value ) == 4 && value >= 0;
		for ( int i = 0; i < numLevels; ++i ) maxSizes[i] = value;
	} else {
		for ( int i = 0; i < numLevels; ++i ) {
			headerOkay &= file->ReadInt( maxSizes[i] ) == 4 && maxSizes[i] >= 0;
		}
	}
	const int totalTiles = TotalStoredTileCount( resolution );
	tileIndexMap = new int[totalTiles];
	tileIndexedDataSizes = new int[totalTiles];
	for ( int i = 0; i < totalTiles; ++i ) {
		headerOkay &= file->ReadInt( tileIndexMap[i] ) == 4;
		headerOkay &= file->ReadInt( tileIndexedDataSizes[i] ) == 4;
		const long long end = (long long)tileIndexMap[i] + tileIndexedDataSizes[i] + 3;
		headerOkay &= tileIndexMap[i] >= 0 && tileIndexedDataSizes[i] >= 0 && end <= file->Length();
	}
	if ( !headerOkay ) {
		common->Warning( "idMegaTexture::Load: corrupt index in '%s'", fileName.c_str() );
		delete[] formats;
		delete[] maxSizes;
		Purge();
		return;
	}
	useImageCompression = r_useMegaTextureImageCompression.GetBool();
	imageCompressionFormat = useImageCompression ?
		( fileCompression == MEGA_COMPRESSION_RGB ? IMAGE_COMPRESSION_DXT1 : IMAGE_COMPRESSION_DXT5 ) : IMAGE_COMPRESSION_NONE;
	AllocRecompressionScratch();
	GenerateNullTileData();
	GenerateGridTileData();
	levels = new idMegaTextureLevel[numLevels];
	int axis = tilesPerAxis;
	int base = 0;
	for ( int i = 0; i < numLevels; ++i ) {
		levels[i].Init( *this, i, base, axis, true, formats[i], maxSizes[i] );
		base += axis * axis;
		axis >>= 1;
	}
	for ( int i = 0; i < numLevels; ++i ) {
		levels[i].PostInit();
	}
	delete[] formats;
	delete[] maxSizes;
	LoadDetailTexture();
	purged = false;
	if ( megaTextureTileLoader ) megaTextureTileLoader->SetActiveMegaTexture( this );
	if ( megaTextureTileDecompressor ) megaTextureTileDecompressor->SetActiveMegaTexture( this );
	{
#ifdef _USING_STDCXX
		std::lock_guard<std::recursive_mutex> guard( lock );
#else
		sdLockGuard<sdRecursiveLock> guard( lock );
#endif
		idVec2 center( 0.5f, 0.5f );
		levels[numLevels - 1].UpdateForCenter( center, false );
	}
	if ( megaTextureTileLoader ) megaTextureTileLoader->SignalThread();
	if ( megaTextureTileDecompressor ) megaTextureTileDecompressor->SignalThread();

	ForceUpdate();
}

void idMegaTexture::Touch() {
	lastUsedFrame = tr.frameCount;
	levelLoadReferenced = true;
	if ( purged ) {
		Load();
	}
}

void idMegaTexture::Purge() {
	if ( megaTextureTileLoader && megaTextureTileLoader->GetActiveMegaTexture() == this ) {
		megaTextureTileLoader->SetActiveMegaTexture( NULL );
	}
	if ( megaTextureTileDecompressor && megaTextureTileDecompressor->GetActiveMegaTexture() == this ) {
		megaTextureTileDecompressor->SetActiveMegaTexture( NULL );
	}
#ifdef _USING_STDCXX
	std::lock_guard<std::recursive_mutex> guard( lock );
#else
	sdLockGuard<sdRecursiveLock> guard( lock );
#endif
	delete[] levels;
	delete upscaleLevel;
	delete[] tileIndexMap;
	delete[] tileIndexedDataSizes;
	delete[] nullTileData;
	delete[] gridTileData;
	delete[] tileRecompressionScratch;
	levels = NULL;
	upscaleLevel = NULL;
	tileIndexMap = tileIndexedDataSizes = NULL;
	nullTileData = gridTileData = tileRecompressionScratch = NULL;
	numLevels = tilesPerAxis = resolution = 0;
	currentWorld = NULL;
	stGrid = NULL;
	stGridWidth = stGridHeight = 0;
	currentTriMapping = NULL;
	detailTexture = detailTextureMask = NULL;
	purged = true;
	CloseFile();
}

void idMegaTexture::Reset() {
#ifdef _USING_STDCXX
	std::lock_guard<std::recursive_mutex> guard( lock );
#else
	sdLockGuard<sdRecursiveLock> guard( lock );
#endif
	for ( int i = 0; i < numLevels; ++i ) levels[i].Reset();
	currentViewOrigin.Set( 262144.0f, 262144.0f, 262144.0f );
}

unsigned int idMegaTexture::GetPureServerChecksum( unsigned int offset ) {
	unsigned int hash = 2166136261u ^ offset;
	for ( const char *p = fileName.c_str(); *p; ++p ) hash = ( hash ^ (byte)*p ) * 16777619u;
	for ( int i = 0, count = TotalStoredTileCount( resolution ); i < count; ++i ) {
		hash = ( hash ^ (unsigned int)tileIndexMap[i] ) * 16777619u;
		hash = ( hash ^ (unsigned int)tileIndexedDataSizes[i] ) * 16777619u;
	}
	return hash;
}

void idMegaTexture::UpdateMapping( const idRenderWorldLocal *world ) {
	currentWorld = world;
	if ( world && world->megaTextureSTGridWidth > 1 && world->megaTextureSTGridHeight > 1 && world->megaTextureSTGrid.Num() > 0 ) {
		stGridBounds = world->megaTextureBounds;
		stGridWidth = world->megaTextureSTGridWidth;
		stGridHeight = world->megaTextureSTGridHeight;
		stGrid = world->megaTextureSTGrid.Ptr();
	} else {
		stGrid = NULL;
		stGridWidth = stGridHeight = 0;
	}
}

void idMegaTexture::SetMappingForSurface( const srfTriangles_t *tri ) {
	if ( tri == currentTriMapping || !tri || !tri->verts || tri->numVerts <= 0 ) return;
	currentTriMapping = tri;
	idDrawVert origin = tri->verts[0];
	idDrawVert axis[2] = { tri->verts[0], tri->verts[0] };
	for ( int i = 0; i < tri->numVerts; ++i ) {
		const idDrawVert &v = tri->verts[i];
		if ( v.st[0] <= origin.st[0] && v.st[1] <= origin.st[1] ) origin = v;
		if ( v.st[0] >= axis[0].st[0] && v.st[1] <= axis[0].st[1] ) axis[0] = v;
		if ( v.st[0] <= axis[1].st[0] && v.st[1] >= axis[1].st[1] ) axis[1] = v;
	}
	for ( int i = 0; i < 2; ++i ) {
		idVec3 direction = axis[i].xyz - origin.xyz;
		const float lengthSqr = direction.LengthSqr();
		if ( lengthSqr <= 0.0f ) continue;
		direction *= ( axis[i].st[i] - origin.st[i] ) / lengthSqr;
		localViewToTextureCenter[i][0] = direction[0];
		localViewToTextureCenter[i][1] = direction[1];
		localViewToTextureCenter[i][2] = direction[2];
		localViewToTextureCenter[i][3] = origin.st[i] - origin.xyz * direction;
	}
}

void idMegaTexture::SetViewOrigin( const idVec3 &origin ) {
	if ( purged || r_skipMegaTexture.GetBool() || origin == currentViewOrigin ) return;
	currentViewOrigin = origin;
	idVec2 center;
	if ( stGrid && stGridWidth > 1 && stGridHeight > 1 &&
		 stGridBounds[1][0] > stGridBounds[0][0] && stGridBounds[1][1] > stGridBounds[0][1] ) {
		const float boundsWidth = stGridBounds[1][0] - stGridBounds[0][0];
		const float boundsHeight = stGridBounds[1][1] - stGridBounds[0][1];
		const float gridX = idMath::ClampFloat( 0.0f, (float)( stGridWidth - 1 ),
			( origin[0] - stGridBounds[0][0] ) * ( stGridWidth - 1 ) / boundsWidth );
		const float gridY = idMath::ClampFloat( 0.0f, (float)( stGridHeight - 1 ),
			( origin[1] - stGridBounds[0][1] ) * ( stGridHeight - 1 ) / boundsHeight );
		const int x0 = idMath::Ftoi( gridX );
		const int y0 = idMath::Ftoi( gridY );
		const int x1 = x0 + 1 < stGridWidth ? x0 + 1 : x0;
		const int y1 = y0 + 1 < stGridHeight ? y0 + 1 : y0;
		const float fx = gridX - x0;
		const float fy = gridY - y0;
		const idVec2 top = stGrid[y0 * stGridWidth + x0] * ( 1.0f - fx ) + stGrid[y0 * stGridWidth + x1] * fx;
		const idVec2 bottom = stGrid[y1 * stGridWidth + x0] * ( 1.0f - fx ) + stGrid[y1 * stGridWidth + x1] * fx;
		center = top * ( 1.0f - fy ) + bottom * fy;
	} else {
		for ( int i = 0; i < 2; ++i ) {
			center[i] = origin[0] * localViewToTextureCenter[i][0] + origin[1] * localViewToTextureCenter[i][1] +
				origin[2] * localViewToTextureCenter[i][2] + localViewToTextureCenter[i][3];
		}
	}
	for ( int i = numLevels - 1; i >= 0; --i ) {
		levels[i].UpdateForCenter( center, false );
	}
}

void idMegaTexture::UpdateForViewOrigin( const idVec3 &origin, int time ) {
#ifdef _USING_STDCXX
	std::lock_guard<std::recursive_mutex> guard( lock );
#else
	sdLockGuard<sdRecursiveLock> guard( lock );
#endif
	if ( lastUsedFrame < tr.frameCount ) {
		// Upload work completed for the previous center before requesting the next
		// center.  This is the ordering used by the ETQW renderer and prevents a
		// newly uploaded atlas offset from lagging one draw behind its image.
		UploadTiles( time );
		SetViewOrigin( origin );
		lastUsedFrame = tr.frameCount;
	}
	for ( int shaderLevel = 0; shaderLevel < numLevels && shaderLevel < 7; ++shaderLevel ) {
		UpdateLevelForViewOrigin( &levels[numLevels - shaderLevel - 1], shaderLevel, time );
	}
}

void idMegaTexture::UpdateLevelForViewOrigin( idMegaTextureLevel *level, int index, int time ) {
	if ( !level || index < 0 || index >= 7 ) return;
	const float hidden[4] = { -2.0f, -2.0f, 0.0f, 1.0f };
	R_SetGLSLProgramEnvParameter( GL_VERTEX_SHADER, index,
		level->ImageIsValid() ? level->GetParms() : hidden );
	const int fadeMilliseconds = r_megaFadeTime.GetInteger();
	shaderLevelOpacity[index] = fadeMilliseconds > 0 ?
		idMath::ClampFloat( 0.0f, 1.0f, ( time - level->GetFadeTime() ) / (float)fadeMilliseconds ) : 1.0f;
}

void idMegaTexture::BindForViewOrigin( const idVec3 origin ) {
	if ( megaTextureTileLoader ) megaTextureTileLoader->SetActiveMegaTexture( this );
	if ( megaTextureTileDecompressor ) megaTextureTileDecompressor->SetActiveMegaTexture( this );
	const idVec3 &streamOrigin = stGrid && backEnd.viewDef ? backEnd.viewDef->renderView.vieworg : origin;
	UpdateForViewOrigin( streamOrigin, Sys_Milliseconds() );
	GL_SelectTexture( 0 );
	globalImages->borderClampImage->Bind();
	// ETQW's 32768 terrain layout uses five moving atlases, followed by the
	// detail texture and detail mask.  All eight available Doom 3 units are used.
	for ( int i = 0; i < 5; ++i ) {
		GL_SelectTexture( 1 + i );
		if ( i < numLevels ) {
			idMegaTextureLevel &level = levels[numLevels - 1 - i];
			if ( r_showMegaTextureLevels.GetBool() ) ( i & 1 ? globalImages->blackImage : globalImages->whiteImage )->Bind();
			else level.GetImage()->Bind();
		} else {
			globalImages->whiteImage->Bind();
		}
	}
	idImage *activeDetail = detailTexture && !detailTexture->defaulted ? detailTexture : globalImages->whiteImage;
	idImage *activeDetailMask = r_detailTexture.GetBool() && detailTextureMask && !detailTextureMask->defaulted ?
		detailTextureMask : globalImages->blackImage;
	GL_SelectTexture( 6 );
	activeDetail->Bind();
	GL_SelectTexture( 7 );
	activeDetailMask->Bind();
	const float detailWidth = activeDetail->uploadWidth > 0 ? (float)activeDetail->uploadWidth : 1.0f;
	const float detailParms[4] = {
		( tilesPerAxis * MEGA_TEXTURE_TILE_SIZE / detailWidth ) * r_detailRatio.GetFloat(),
		r_detailTexture.GetBool() ? 1.0f : 0.0f,
		r_detailFade.GetFloat(),
		0.0f
	};
	R_SetGLSLProgramEnvParameter( GL_VERTEX_SHADER, 7, shaderLevelOpacity + 1 );
	R_SetGLSLProgramEnvParameter( GL_VERTEX_SHADER, 8, detailParms );
}

void idMegaTexture::Unbind() {
	for ( int i = 1; i <= 7; ++i ) {
		GL_SelectTexture( i );
		globalImages->BindNull();
	}
	GL_SelectTexture( 0 );
}

bool idMegaTexture::UploadTiles( int time ) {
	bool complete = true;
	for ( int i = numLevels - 1; i >= 0; --i ) complete &= levels[i].UploadTiles( time );
	if ( upscaleLevel ) complete &= upscaleLevel->UploadTiles( time );
	return complete;
}

idMegaTextureLevel *idMegaTexture::GetLevel( int index ) const {
	return index >= 0 && index < numLevels ? &levels[index] : NULL;
}

int idMegaTexture::SeekToTile( int tileNum ) {
	if ( !file || tileNum < 0 || tileNum >= TotalStoredTileCount( resolution ) ) return -1;
	const int offset = tileIndexMap[tileNum];
	const int distance = idMath::Abs( offset - lastTileOffset );
	file->Seek( offset, FS_SEEK_SET );
	lastTileOffset = offset;
	return distance;
}

int idMegaTexture::GetTileOffset( int tileNum ) const {
	return tileIndexMap && tileNum >= 0 && tileNum < TotalStoredTileCount( resolution ) ? tileIndexMap[tileNum] : -1;
}

int idMegaTexture::GetTileDataSize( int tileNum ) const {
	return tileIndexedDataSizes && tileNum >= 0 && tileNum < TotalStoredTileCount( resolution ) ? tileIndexedDataSizes[tileNum] : 0;
}

bool idMegaTexture::ReadTileData( int tileNum, byte *destination, int destinationBytes ) {
#ifdef _USING_STDCXX
	std::lock_guard<std::recursive_mutex> guard( lock );
#else
	sdLockGuard<sdRecursiveLock> guard( lock );
#endif
	const int expected = GetTileDataSize( tileNum ) + 3;
	if ( !destination || expected <= 3 || destinationBytes < expected || SeekToTile( tileNum ) < 0 ) return false;
	return file->Read( destination, expected ) == expected;
}

void idMegaTexture::AllocRecompressionScratch() {
	delete[] tileRecompressionScratch;
	tileRecompressionScratch = new byte[MegaTextureMipChainBytes()];
}

void idMegaTexture::GenerateNullTileData() {
	delete[] nullTileData;
	nullTileData = new byte[MegaTextureTileChainBytes( imageCompressionFormat )];
	memset( tileRecompressionScratch, 0, MegaTextureMipChainBytes() );
	if ( megaTextureTileDecompressor ) megaTextureTileDecompressor->RecompressTile( imageCompressionFormat, tileRecompressionScratch, nullTileData );
	else memcpy( nullTileData, tileRecompressionScratch, MegaTextureMipChainBytes() );
}

void idMegaTexture::GenerateGridTileData() {
	delete[] gridTileData;
	gridTileData = new byte[MegaTextureTileChainBytes( imageCompressionFormat )];
	for ( int y = 0; y < 128; ++y ) {
		for ( int x = 0; x < 128; ++x ) {
			byte *pixel = tileRecompressionScratch + ( y * 128 + x ) * 4;
			const bool line = ( x & 15 ) == 0 || ( y & 15 ) == 0;
			pixel[0] = line ? 255 : 32; pixel[1] = line ? 255 : 32; pixel[2] = line ? 0 : 32; pixel[3] = 255;
		}
	}
	if ( megaTextureTileDecompressor ) megaTextureTileDecompressor->RecompressTile( imageCompressionFormat, tileRecompressionScratch, gridTileData );
	else memcpy( gridTileData, tileRecompressionScratch, MegaTextureMipChainBytes() );
}

void idMegaTexture::LoadDetailTexture() {
	idStr imageBase = "megatextures/";
	imageBase += name;
	imageBase.StripFileExtension();
	idStr detailName = imageBase;
	detailName += "_detail.tga";
	idStr detailMaskName = imageBase;
	detailMaskName += "_detailmask.tga";
	mipmapState_t detailMipmapState;
	detailMipmapState.colorType = mipmapState_t::MT_BLEND;
	for ( int channel = 0; channel < 4; ++channel ) {
		detailMipmapState.color[channel] = 0.5f;
		detailMipmapState.blend[channel] = r_detailFade.GetFloat();
	}
	detailTexture = globalImages->ImageFromFile( detailName, TF_DEFAULT, false, TR_REPEAT, TD_DEFAULT, CF_2D, /*//k false, &*/detailMipmapState );
	detailTextureMask = globalImages->ImageFromFile( detailMaskName, TF_DEFAULT, false, TR_REPEAT, TD_DEFAULT );
	// These are owned by the MegaTexture resource and must survive the ordinary
	// per-map image purge just like the generated moving atlases.
	if ( detailTexture ) {
		detailTexture->referencedOutsideLevelLoad = true;
		detailTexture->levelLoadReferenced = true;
	}
	if ( detailTextureMask ) {
		detailTextureMask->referencedOutsideLevelLoad = true;
		detailTextureMask->levelLoadReferenced = true;
	}
}

void idMegaTexture::ForceUpdate() {
	if ( purged ) return;
	forcedUpdate = true;
	while ( !UploadTiles( 0 ) ) {
		if ( megaTextureTileLoader ) megaTextureTileLoader->SignalThread();
		if ( megaTextureTileDecompressor ) megaTextureTileDecompressor->SignalThread();
#ifdef _USING_STDCXX
		std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );
#else
		Sys_Sleep(10);
#endif
	}
	forcedUpdate = false;
}

void idMegaTexture::OnUseMegaTextureCompressionChange() {
	if ( purged ) return;
	Load();
}

int idMegaTexture::TotalStoredTileCount( int imageResolution ) {
	int count = 0;
	for ( int axis = imageResolution / 128; axis >= 16; axis >>= 1 ) count += axis * axis;
	return count;
}

int GetCompressedTotalKiloBytesReadPerSecond() {
	return GetCompressedUsefulKiloBytesReadPerSecond();
}

int GetPercentageTilesReady( int levelNum ) {
	idMegaTexture *mega = megaTextureTileLoader ? megaTextureTileLoader->GetActiveMegaTexture() : NULL;
	idMegaTextureLevel *level = mega ? mega->GetLevel( levelNum ) : NULL;
	if ( !level ) return 0;
	int ready = 0;
	for ( int y = 0; y < 16; ++y ) {
		for ( int x = 0; x < 16; ++x ) ready += !level->GetTileLocal( x, y )->IsDirty();
	}
	return ready * 100 / 256;
}

void idMegaTexture::ReloadImages() {
#ifdef _USING_STDCXX
	std::lock_guard<std::recursive_mutex> guard( lock );
#else
	sdLockGuard<sdRecursiveLock> guard( lock );
#endif
	for ( int i = 0; i < numLevels; ++i )
#ifdef MEGATEXTURE_IMAGE_USING_GENERATOR
#ifdef _MULTITHREAD //karin: add image allocate queue in non-OpenGL thread if multithreading-rendering, else call OpenGL directly
		/*if (multithreadActive)
			levels[i].LoadEmptyLevelImage( levels[i].image );
		else*/
		levels[i].image->ActuallyLoadImage(false, false, renderThread->IsActive());
#else
		levels[i].image->ActuallyLoadImage(false, false);
#endif
#else
		levels[i].EmptyLevelImage( levels[i].image ); // not support multithreading rendering
#endif
	ForceUpdate();
}

void idMegaTexture::PrintInfo() const {
	common->Printf( "%s: version %d, %dx%d, %d levels, %d stored tiles%s\n", fileName.c_str(), version,
		resolution, resolution, numLevels, TotalStoredTileCount( resolution ), useImageCompression ? ", DXT runtime cache" : "" );
	for ( int i = 0; i < numLevels; ++i ) {
		common->Printf( "  level %d: %d tiles/axis, format %d, max compressed %d, %.2f MB runtime\n", i,
			levels[i].tilesPerAxis, (int)levels[i].megaCompressionFormat, levels[i].maxCompressedTileSize,
			levels[i].usedMemory / ( 1024.0f * 1024.0f ) );
	}
}

bool idMegaTexture::DebugDecodeTile( int levelNumber, int x, int y, const char *outputName ) {
	if ( levelNumber < 0 || levelNumber >= numLevels || x < 0 || y < 0 ||
		x >= levels[levelNumber].tilesPerAxis || y >= levels[levelNumber].tilesPerAxis ) return false;
	idMegaTextureLevel &level = levels[levelNumber];
	idMegaTextureTile *tile = level.GetTileLocal( x & 15, y & 15 );
	{
#ifdef _USING_STDCXX
		std::lock_guard<std::recursive_mutex> guard( lock );
#else
		sdLockGuard<sdRecursiveLock> guard( lock );
#endif
		tile->Update( x, y, true );
		tile->SetCachedTileData( this, level.tileBase, level.tilesPerAxis );
	}
	ForceUpdate();
	if ( !tile->GetTileData() ) return false;
	R_WriteTGA( outputName, tile->GetTileData()->pic, 128, 128, false );
	return true;
}

void idMegaTexture::TestStreamingPerformance( const idCmdArgs &args ) {
	(void)args;
	common->Printf( "MegaTexture: %d tiles/s, %d useful KiB/s, %d seeks/s, %.2f tiles/seek\n",
		GetMegaTilesPerSecond(), GetCompressedUsefulKiloBytesReadPerSecond(), GetCompressedSeeksPerSecond(), GetTilesPerSeek() );
}

void idMegaTexture::ShowMemoryUsage( const idCmdArgs &args ) {
	(void)args;
	float total = 0.0f;
	for ( int i = 0; i < numLevels; ++i ) {
		const float megabytes = levels[i].GetUsedMemory() / ( 1024.0f * 1024.0f );
		common->Printf( "level %d: %.2f MB\n", i, megabytes );
		total += megabytes;
	}
	common->Printf( "total: %.2f MB\n", total );
}

void idMegaTexture::MegaTestStreamingPerformance_f( const idCmdArgs &args ) {
	if ( megaTextureTileLoader && megaTextureTileLoader->GetActiveMegaTexture() ) megaTextureTileLoader->GetActiveMegaTexture()->TestStreamingPerformance( args );
}

void idMegaTexture::MegaShowMemoryUsage_f( const idCmdArgs &args ) {
	if ( megaTextureTileLoader && megaTextureTileLoader->GetActiveMegaTexture() ) megaTextureTileLoader->GetActiveMegaTexture()->ShowMemoryUsage( args );
}

void idMegaTexture::MegaTextureInfo_f( const idCmdArgs &args ) {
	(void)args;
	if ( megaTextureTileLoader && megaTextureTileLoader->GetActiveMegaTexture() ) megaTextureTileLoader->GetActiveMegaTexture()->PrintInfo();
	else common->Printf( "No active MegaTexture\n" );
}

void idMegaTexture::MegaTextureLoad_f( const idCmdArgs &args ) {
	if ( args.Argc() != 2 ) {
		common->Printf( "usage: megaTextureLoad <name>\n" );
		return;
	}
	idMegaTexture *mega = globalImages->MegaTextureFromFile( args.Argv( 1 ) );
	if ( mega ) mega->PrintInfo();
	else common->Warning( "Could not load MegaTexture '%s'", args.Argv( 1 ) );
}

void idMegaTexture::MegaTextureDecodeTile_f( const idCmdArgs &args ) {
	if ( args.Argc() < 5 ) {
		common->Printf( "usage: megaTextureDecodeTile <level> <x> <y> <output.tga>\n" );
		return;
	}
	idMegaTexture *mega = megaTextureTileLoader ? megaTextureTileLoader->GetActiveMegaTexture() : NULL;
	if ( !mega || !mega->DebugDecodeTile( atoi( args.Argv( 1 ) ), atoi( args.Argv( 2 ) ), atoi( args.Argv( 3 ) ), args.Argv( 4 ) ) ) {
		common->Warning( "MegaTexture tile decode failed" );
	}
}

void idMegaTexture::UpdateForViewOrigin( const idVec3 &origin, int time, const sdRenderProgram *renderProgram ) {
#ifdef _USING_STDCXX
	std::lock_guard<std::recursive_mutex> guard( lock );
#else
	sdLockGuard<sdRecursiveLock> guard( lock );
#endif
	if ( lastUsedFrame != tr.frameCount ) {
		// Upload work completed for the previous center before requesting the next
		// center.  This is the ordering used by the ETQW renderer and prevents a
		// newly uploaded atlas offset from lagging one draw behind its image.
		UploadTiles( time );
		SetViewOrigin( origin );
		lastUsedFrame = tr.frameCount;
	}
	for ( int shaderLevel = 0; shaderLevel < numLevels; ++shaderLevel ) {
		UpdateLevelForViewOrigin( &levels[numLevels - shaderLevel - 1], shaderLevel, time, renderProgram );
	}
}

void idMegaTexture::UpdateLevelForViewOrigin( idMegaTextureLevel *level, int index, int time, const sdRenderProgram *renderProgram ) {
	if ( !level || index < 0 ) return;
	const float hidden[4] = { -2.0f, -2.0f, 0.0f, 1.0f };
	char parmName[32];

	idStr::snPrintf(parmName, sizeof(parmName), "megaMaskParams_%d", index);
	renderProgram->BindVector( parmName, level->ImageIsValid() ? level->GetParms() : hidden );

	idStr::snPrintf(parmName, sizeof(parmName), "megaTextureParams_%d", index);
    float opacitya = (float)(1 << (index + 1)) * 0.5f;
	renderProgram->BindVector( parmName , opacitya );

	const int fadeMilliseconds = r_megaFadeTime.GetInteger();
	shaderLevelOpacity[index] = fadeMilliseconds > 0 ?
		idMath::ClampFloat( 0.0f, 1.0f, ( time - level->GetFadeTime() ) / (float)fadeMilliseconds ) : 1.0f;
}

void idMegaTexture::BindForViewOrigin( const idVec3 origin, const sdRenderProgram *renderProgram ) {
	if ( megaTextureTileLoader ) megaTextureTileLoader->SetActiveMegaTexture( this );
	if ( megaTextureTileDecompressor ) megaTextureTileDecompressor->SetActiveMegaTexture( this );
	const idVec3 &streamOrigin = stGrid && backEnd.viewDef ? backEnd.viewDef->renderView.vieworg : origin;
	UpdateForViewOrigin( streamOrigin, Sys_Milliseconds(), renderProgram );

	// ETQW's 32768 terrain layout uses five moving atlases, followed by the
	// detail texture and detail mask.  All eight available Doom 3 units are used.
	char texName[32];
	for ( int i = 0; i < 5; ++i ) {
		idStr::snPrintf(texName, sizeof(texName), "megaTextureLevel_%d", i);
		if ( i < numLevels ) {
			idMegaTextureLevel &level = levels[numLevels - 1 - i];
			if ( r_showMegaTextureLevels.GetBool() )
				renderProgram->BindImage(texName, ( i & 1 ? globalImages->blackImage : globalImages->whiteImage ));
			else
				renderProgram->BindImage(texName, level.GetImage());
		} else {
			renderProgram->BindImage(texName, globalImages->whiteImage);
		}
	}
	renderProgram->BindImage("megaTextureLevel_5", globalImages->whiteImage); // unused

	idImage *activeDetail = detailTexture && !detailTexture->defaulted ? detailTexture : globalImages->whiteImage;
	idImage *activeDetailMask = r_detailTexture.GetBool() && detailTextureMask && !detailTextureMask->defaulted ?
		detailTextureMask : globalImages->blackImage;
	renderProgram->BindImage("megaDetailTexture", activeDetail);
	renderProgram->BindImage("megaDetailTextureMask", activeDetailMask);

	const float detailWidth = activeDetail->uploadWidth > 0 ? (float)activeDetail->uploadWidth : 1.0f;
	const float detailParms[4] = {
		( tilesPerAxis * MEGA_TEXTURE_TILE_SIZE / detailWidth ) * r_detailRatio.GetFloat(),
		r_detailTexture.GetBool() ? 1.0f : 0.0f,
		r_detailFade.GetFloat(),
		0.0f
	};
	//R_SetGLSLProgramEnvParameter( GL_VERTEX_SHADER, 7, shaderLevelOpacity + 1 );
	renderProgram->BindVector("megaDetailTextureParams", detailParms);
}

void R_InitMegaTextureSystem(void) {
	megaTextureTileDecompressor = new idMegaTextureTileDecompressor;
	megaTextureTileDecompressor->Init();
	megaTextureTileLoader = new idMegaTextureTileLoader;
	megaTextureTileLoader->Init();
}

void R_ShutdownMegaTextureSystem(void) {
	if ( megaTextureTileLoader ) {
		megaTextureTileLoader->Shutdown();
		delete megaTextureTileLoader;
		megaTextureTileLoader = NULL;
	}
	if ( megaTextureTileDecompressor ) {
		megaTextureTileDecompressor->Shutdown();
		delete megaTextureTileDecompressor;
		megaTextureTileDecompressor = NULL;
	}
}
