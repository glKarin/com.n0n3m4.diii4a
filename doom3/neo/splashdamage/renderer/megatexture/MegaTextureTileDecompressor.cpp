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
#include "MegaTextureTileDecompressor.h"
#include "MegaTextureTileLoader.h"

#ifdef _USING_STDCXX
#include <chrono>
#endif
#include <math.h>

idMegaTextureTileDecompressor *megaTextureTileDecompressor = NULL;

idCVar idMegaTextureTileDecompressor::r_megaTilesPerSecond( "r_megaTilesPerSecond", "0", CVAR_RENDERER | CVAR_INTEGER, "maximum MegaTexture tile decodes per second; zero is unlimited" );
idCVar idMegaTextureTileDecompressor::r_megaShowGrid( "r_megaShowGrid", "0", CVAR_RENDERER | CVAR_BOOL, "replace decoded MegaTexture tiles with a diagnostic grid" );
idCVar idMegaTextureTileDecompressor::r_megaShowTileSize( "r_megaShowTileSize", "0", CVAR_RENDERER | CVAR_BOOL, "color MegaTexture tiles by compressed byte size" );

static int RuntimeTileBytes( imageCompressionFormat_t format ) {
	int bytes = 0;
	for ( int size = 128; size >= 4; size >>= 1 ) {
		if ( format == IMAGE_COMPRESSION_DXT1 ) bytes += 8 * ( ( size + 3 ) / 4 ) * ( ( size + 3 ) / 4 );
		else if ( format == IMAGE_COMPRESSION_DXT5 ) bytes += 16 * ( ( size + 3 ) / 4 ) * ( ( size + 3 ) / 4 );
		else bytes += size * size * 4;
	}
	return bytes;
}

idMegaTextureTileDecompressor::idMegaTextureTileDecompressor() :
	thread( NULL ), dctDecoder( NULL ), dxtEncoder( NULL ), activeMegaTexture( NULL ), workerMegaTexture( NULL ),
	lastProcessedTime( 0 ), numTilesThisMsec( 0 ), numProcessedTiles( 0 ), terminate( false ), forceUpdate( false ) {
	memset( &compressedData, 0, sizeof( compressedData ) );
	compressedData.parentCachedLevelNum = -1;
	compressedData.parentCachedGlobalX = -1;
	compressedData.parentCachedGlobalY = -1;
}

idMegaTextureTileDecompressor::~idMegaTextureTileDecompressor() {
	Shutdown();
}

void idMegaTextureTileDecompressor::Init() {
	if ( !dctDecoder ) dctDecoder = new idBareDctDecoder;
	if ( !dxtEncoder ) dxtEncoder = new idDxtEncoder;
	if ( !compressedData.parentCachedData ) compressedData.parentCachedData = new byte[128 * 128 * 4];
	StartThread();
}

void idMegaTextureTileDecompressor::Shutdown() {
	StopThread();
	SetActiveMegaTexture( NULL );
	delete dctDecoder;
	delete dxtEncoder;
	delete[] compressedData.parentCachedData;
	dctDecoder = NULL;
	dxtEncoder = NULL;
	compressedData.parentCachedData = NULL;
}

void idMegaTextureTileDecompressor::StartThread() {
	if ( thread ) return;
	terminate = false;
#ifdef _USING_STDCXX
	std::lock_guard<std::mutex> guard( stateMutex );
	thread = new std::thread( [this]() { Run( NULL ); } );
#else
	sdLockGuard<sdLock> guard( stateMutex );
	sdThreadProcessFunctor<idMegaTextureTileDecompressor> *runner = new sdThreadProcessFunctor<idMegaTextureTileDecompressor>();
	runner->Init(this, &idMegaTextureTileDecompressor::ThreadPlaceholder, &idMegaTextureTileDecompressor::Run, &idMegaTextureTileDecompressor::ThreadPlaceholder );
	thread = new sdThread( runner );
	thread->SetName( "idMegaTextureTileDecompressor" );
	thread->Start();
#endif
}

void idMegaTextureTileDecompressor::StopThread() {
	if ( !thread ) return;
	Stop();
#ifdef _USING_STDCXX
	thread->join();
	delete thread;
#else
	thread->Stop();
	thread->Join();
	thread->Destroy();
#endif
	thread = NULL;
}

void idMegaTextureTileDecompressor::SetActiveMegaTexture( idMegaTexture *mega ) {
#ifdef _USING_STDCXX
	std::unique_lock<std::mutex> stateLock( stateMutex );
	idMegaTexture *old = activeMegaTexture;
	if ( old == mega ) return;
	activeMegaTexture = mega;
	// Decoding uses private snapshots without the MegaTexture lock. Keep the
	// previous resource alive until its in-flight decode has finished.
	while ( old && workerMegaTexture == old ) {
		workerIdleSignal.wait( stateLock );
	}
	stateLock.unlock();
	SignalThread();
#else
	sdUniqueLock<sdLock> stateLock( stateMutex );
	idMegaTexture *old = activeMegaTexture;
	if ( old == mega ) return;
	activeMegaTexture = mega;
	// Decoding uses private snapshots without the MegaTexture lock. Keep the
	// previous resource alive until its in-flight decode has finished.
	while ( old && workerMegaTexture == old ) {
		stateLock.Unlock();
		workerIdleSignal.Wait();
		stateLock.Lock();
	}
	stateLock.Unlock();
	SignalThread();
#endif
}

idMegaTexture *idMegaTextureTileDecompressor::GetActiveMegaTexture() const {
#ifdef _USING_STDCXX
	std::lock_guard<std::mutex> guard( stateMutex );
#else
	sdLockGuard<sdLock> guard( stateMutex );
#endif
	return activeMegaTexture;
}

idMegaTexture *idMegaTextureTileDecompressor::BeginActiveMegaTextureWork() {
#ifdef _USING_STDCXX
	std::lock_guard<std::mutex> guard( stateMutex );
#else
	sdLockGuard<sdLock> guard( stateMutex );
#endif
	if ( terminate ) {
		return NULL;
	}
	workerMegaTexture = activeMegaTexture;
	return workerMegaTexture;
}

void idMegaTextureTileDecompressor::EndActiveMegaTextureWork( idMegaTexture *mega ) {
#ifdef _USING_STDCXX
	std::lock_guard<std::mutex> guard( stateMutex );
#else
	sdLockGuard<sdLock> guard( stateMutex );
#endif
	if ( workerMegaTexture == mega ) {
		workerMegaTexture = NULL;
	}
#ifdef _USING_STDCXX
	workerIdleSignal.notify_all();
#else
	workerIdleSignal.Set();
#endif
}

void idMegaTextureTileDecompressor::ForceUpdate() {
	forceUpdate = true;
	SignalThread();
}

void idMegaTextureTileDecompressor::ResetNumProcessedTiles() { numProcessedTiles = 0; }
#ifdef _USING_STDCXX
int idMegaTextureTileDecompressor::GetNumProcessedTiles() const { return numProcessedTiles.load(); }
void idMegaTextureTileDecompressor::SignalThread() { signal.notify_one(); }
#else
int idMegaTextureTileDecompressor::GetNumProcessedTiles() const { return numProcessedTiles.Load(); }
void idMegaTextureTileDecompressor::SignalThread() { signal.Set(); }
#endif

void idMegaTextureTileDecompressor::Stop() {
	terminate = true;
#ifdef _USING_STDCXX
	signal.notify_all();
	throttleSignal.notify_all();
#else
	signal.Set();
	throttleSignal.Set();
#endif
}

idMegaTextureTile *idMegaTextureTileDecompressor::FindTileToDecode( idMegaTexture *mega, idMegaTextureLevel *&outLevel ) {
	outLevel = NULL;
	for ( int levelNumber = mega->GetNumLevels() - 1; levelNumber >= 0; --levelNumber ) {
		idMegaTextureLevel *level = mega->GetLevel( levelNumber );
		for ( idMegaTextureTile *tile = level->dirtyTiles.Next(); tile; tile = tile->dirtyNode.Next() ) {
			if ( !tile->IsLoaded() || tile->tileData ) continue;
			if ( level->megaCompressionFormat == MEGA_COMPRESSION_LUM ) {
				idMegaTextureLevel *parentLevel = mega->GetLevel( levelNumber + 1 );
				if ( !parentLevel || !parentLevel->GetCompressedTileData( tile->globalX >> 1, tile->globalY >> 1 ) ) continue;
			}
			outLevel = level;
			return tile;
		}
	}
	return NULL;
}

void idMegaTextureTileDecompressor::GetCompressedTileData( idMegaTexture *mega, idMegaTextureLevel *level, idMegaTextureTile *tile ) {
	compressedData.globalX = tile->globalX;
	compressedData.globalY = tile->globalY;
	compressedData.data = tile->GetCompressedTileData();
	compressedData.size = mega->GetTileDataSize( tile->GetTileNum() );
	compressedData.parentLevelNum = -1;
	compressedData.parentData = NULL;
	compressedData.parentSize = 0;
	if ( level->megaCompressionFormat == MEGA_COMPRESSION_LUM ) {
		compressedData.parentLevelNum = level->levelNum + 1;
		compressedData.parentGlobalX = tile->globalX >> 1;
		compressedData.parentGlobalY = tile->globalY >> 1;
		idMegaTextureLevel *parentLevel = mega->GetLevel( compressedData.parentLevelNum );
		if ( parentLevel ) {
			compressedData.parentData = parentLevel->GetCompressedTileData( compressedData.parentGlobalX, compressedData.parentGlobalY );
			const int parentTile = parentLevel->tileBase + compressedData.parentGlobalY * parentLevel->tilesPerAxis + compressedData.parentGlobalX;
			compressedData.parentSize = mega->GetTileDataSize( parentTile );
		}
	}
}

void idMegaTextureTileDecompressor::SnapshotCompressedTileData() {
	compressedTileSnapshot.Clear();
	parentCompressedTileSnapshot.Clear();
	if ( compressedData.data && compressedData.size > 0 ) {
		compressedTileSnapshot.SetNum( compressedData.size + 3, false );
		memcpy( compressedTileSnapshot.Ptr(), compressedData.data, compressedData.size + 3 );
		compressedData.data = compressedTileSnapshot.Ptr();
	}
	if ( compressedData.parentData && compressedData.parentSize > 0 ) {
		parentCompressedTileSnapshot.SetNum( compressedData.parentSize + 3, false );
		memcpy( parentCompressedTileSnapshot.Ptr(), compressedData.parentData, compressedData.parentSize + 3 );
		compressedData.parentData = parentCompressedTileSnapshot.Ptr();
	}
}

const byte *idMegaTextureTileDecompressor::SetQuality( const byte *data ) {
	if ( !data ) return NULL;
	dctDecoder->SetQuality_Generic( data[0], data[1], data[2] );
	return data + 3;
}

void idMegaTextureTileDecompressor::DecompressLuminance( byte *destination ) {
	if ( !compressedData.parentData || !compressedData.data ) {
		memset( destination, 0, 128 * 128 * 4 );
		return;
	}
	if ( compressedData.parentCachedLevelNum != compressedData.parentLevelNum ||
		 compressedData.parentCachedGlobalX != compressedData.parentGlobalX ||
		 compressedData.parentCachedGlobalY != compressedData.parentGlobalY ) {
		const byte *parent = SetQuality( compressedData.parentData );
		if ( !dctDecoder->DecompressImageYCoCg_Generic( parent, compressedData.parentCachedData, 128, 128, compressedData.parentSize ) ) {
			memset( compressedData.parentCachedData, 128, 128 * 128 * 4 );
		}
		compressedData.parentCachedLevelNum = compressedData.parentLevelNum;
		compressedData.parentCachedGlobalX = compressedData.parentGlobalX;
		compressedData.parentCachedGlobalY = compressedData.parentGlobalY;
	}
	const int quadrantX = ( compressedData.globalX & 1 ) * 64;
	const int quadrantY = ( compressedData.globalY & 1 ) * 64;
	const byte *quadrant = compressedData.parentCachedData + ( quadrantY * 128 + quadrantX ) * 4;
	MegaTextureUpscale2xBicubic( quadrant, 64, 64, 128 * 4, destination );
	const byte *enhancement = SetQuality( compressedData.data );
	dctDecoder->DecompressLuminanceEnhancement_Generic( enhancement, destination, 128, 128, compressedData.size );
	MegaTextureConvertYCoCgToRGB( destination, 128, 128 );
}

void idMegaTextureTileDecompressor::DecompressTile( megaCompressionFormat_t format, byte *destination ) {
	if ( !compressedData.data ) {
		memset( destination, 0, 128 * 128 * 4 );
		return;
	}
	const byte *data = SetQuality( compressedData.data );
	bool okay = false;
	switch ( format ) {
		case MEGA_COMPRESSION_RGB:
			okay = dctDecoder->DecompressImageRGB_Generic( data, destination, 128, 128, compressedData.size );
			break;
		case MEGA_COMPRESSION_RGBA:
			okay = dctDecoder->DecompressImageRGBA_Generic( data, destination, 128, 128, compressedData.size );
			break;
		case MEGA_COMPRESSION_LUM:
			DecompressLuminance( destination );
			return;
		case MEGA_COMPRESSION_NONE:
			if ( compressedData.size >= 128 * 128 * 4 ) {
				memcpy( destination, data, 128 * 128 * 4 );
				okay = true;
			}
			break;
	}
	if ( !okay ) {
		for ( int i = 0; i < 128 * 128; ++i ) {
			destination[i * 4 + 0] = 255; destination[i * 4 + 1] = 0; destination[i * 4 + 2] = 255; destination[i * 4 + 3] = 255;
		}
	}
}

void idMegaTextureTileDecompressor::DeRecompressTile( idMegaTextureLevel *level, byte *destination ) {
	byte *decodeTarget = level->megaTexture->UseImageCompression() ? level->megaTexture->GetTileRecompressionScratch() : destination;
	DecompressTile( level->megaCompressionFormat, decodeTarget );
	RecompressTile( level->megaTexture->GetImageCompressionFormat(), decodeTarget, destination );
}

void idMegaTextureTileDecompressor::RecompressTile( imageCompressionFormat_t format, byte *source, byte *tileData ) {
	if ( !source || !tileData ) return;
	idMipMap::CreateMips( source, 6 );
	if ( format == IMAGE_COMPRESSION_NONE ) {
		if ( source != tileData ) memcpy( tileData, source, RuntimeTileBytes( format ) );
		return;
	}
	byte *in = source;
	byte *out = tileData;
	for ( int size = 128; size >= 4; size >>= 1 ) {
		int bytes = 0;
		if ( format == IMAGE_COMPRESSION_DXT1 ) dxtEncoder->CompressImageDXT1Fast_Generic( in, out, size, size, bytes );
		else dxtEncoder->CompressImageDXT5Fast_Generic( in, out, size, size, bytes );
		out += bytes;
		in += size * size * 4;
	}
}

void idMegaTextureTileDecompressor::RecompressTile_MMX( imageCompressionFormat_t f, byte *s, byte *d ) { RecompressTile( f, s, d ); }
void idMegaTextureTileDecompressor::RecompressTile_SSE2( imageCompressionFormat_t f, byte *s, byte *d ) { RecompressTile( f, s, d ); }
void idMegaTextureTileDecompressor::RecompressTile_Xenon( imageCompressionFormat_t f, byte *s, byte *d ) { RecompressTile( f, s, d ); }
const byte *idMegaTextureTileDecompressor::SetQuality_MMX( const byte *d ) { return SetQuality( d ); }
const byte *idMegaTextureTileDecompressor::SetQuality_SSE2( const byte *d ) { return SetQuality( d ); }
const byte *idMegaTextureTileDecompressor::SetQuality_Xenon( const byte *d ) { return SetQuality( d ); }
void idMegaTextureTileDecompressor::DecompressLuminance_MMX( byte *d ) { DecompressLuminance( d ); }
void idMegaTextureTileDecompressor::DecompressLuminance_SSE2( byte *d ) { DecompressLuminance( d ); }
void idMegaTextureTileDecompressor::DecompressLuminance_Xenon( byte *d ) { DecompressLuminance( d ); }
void idMegaTextureTileDecompressor::DecompressTile_MMX( megaCompressionFormat_t f, byte *d ) { DecompressTile( f, d ); }
void idMegaTextureTileDecompressor::DecompressTile_SSE2( megaCompressionFormat_t f, byte *d ) { DecompressTile( f, d ); }
void idMegaTextureTileDecompressor::DecompressTile_Xenon( megaCompressionFormat_t f, byte *d ) { DecompressTile( f, d ); }
void idMegaTextureTileDecompressor::DeRecompressTile_MMX( idMegaTextureLevel *l, byte *d ) { DeRecompressTile( l, d ); }
void idMegaTextureTileDecompressor::DeRecompressTile_SSE2( idMegaTextureLevel *l, byte *d ) { DeRecompressTile( l, d ); }
void idMegaTextureTileDecompressor::DeRecompressTile_Xenon( idMegaTextureLevel *l, byte *d ) { DeRecompressTile( l, d ); }

unsigned int idMegaTextureTileDecompressor::Run( void *parameter ) {
	(void)parameter;
	while ( !terminate ) {
		idMegaTexture *mega = BeginActiveMegaTextureWork();
		bool didWork = false;
		if ( mega ) {
			idMegaTextureLevel *level = NULL;
			idMegaTextureTile *tile = NULL;
			tileData_t *decoded = NULL;
			int tileX = 0;
			int tileY = 0;
			int tileBase = 0;
			bool decodeTile = false;
			{
#ifdef _USING_STDCXX
				std::lock_guard<std::recursive_mutex> guard( mega->GetLock() );
#else
				sdLockGuard<sdRecursiveLock> guard( mega->GetLock() );
#endif
				tile = FindTileToDecode( mega, level );
				if ( tile && level ) {
					const int limit = r_megaTilesPerSecond.GetInteger();
					const int now = Sys_Milliseconds();
					const int interval = limit > 0 && 1000 / limit > 1 ? 1000 / limit : 1;
					if ( mega->IsForcedUpdate() || limit <= 0 || now - lastProcessedTime >= interval ) {
						lastProcessedTime = now;
						decoded = level->GetAvailableTile();
						if ( decoded ) {
							tileX = tile->globalX;
							tileY = tile->globalY;
							tileBase = level->tileBase;
							GetCompressedTileData( mega, level, tile );
							SnapshotCompressedTileData();
							decodeTile = true;
						}
					}
				}
			}

			// DCT decode, mip generation, and optional DXT recompression are the
			// expensive portion. They must not hold the per-frame renderer lock.
			if ( decodeTile ) {
				if ( r_megaShowGrid.GetBool() ) {
					memcpy( decoded->pic, mega->GetGridTileData(), RuntimeTileBytes( mega->GetImageCompressionFormat() ) );
				} else if ( r_megaShowTileSize.GetBool() ) {
					byte *scratch = mega->GetTileRecompressionScratch();
					const byte color[4] = {
						(byte)( compressedData.size >= 12288 ? 255 : 0 ),
						(byte)( compressedData.size >= 5120 && compressedData.size < 12288 ? 255 : 0 ),
						(byte)( compressedData.size < 5120 ? 255 : 0 ), 255 };
					for ( int i = 0; i < 128 * 128; ++i ) memcpy( scratch + i * 4, color, 4 );
					RecompressTile( mega->GetImageCompressionFormat(), scratch, decoded->pic );
				} else {
					DeRecompressTile( level, decoded->pic );
				}

				{
#ifdef _USING_STDCXX
					std::lock_guard<std::recursive_mutex> guard( mega->GetLock() );
#else
					sdLockGuard<sdRecursiveLock> guard( mega->GetLock() );
#endif
					if ( tile->globalX == tileX && tile->globalY == tileY ) {
						decoded->x = tileX;
						decoded->y = tileY;
						decoded->tileBase = tileBase;
						tile->tileData = decoded;
						level->RemoveDirtyTile( tile );
					} else {
						level->ReleaseTile( decoded );
					}
				}
				++numProcessedTiles;
				didWork = true;
			}
			EndActiveMegaTextureWork( mega );
		}
		if ( didWork ) continue;
#ifdef _USING_STDCXX
		std::unique_lock<std::mutex> waitLock( stateMutex );
		signal.wait_for( waitLock, std::chrono::milliseconds( 5 ) );
#else
		sdUniqueLock<sdLock> waitLock( stateMutex );
#if 1
		signal.WaitForLock( stateMutex, 5 );
#else
		waitLock.Unlock();
		signal.Wait( 5 );
		waitLock.Lock();
#endif
#endif
	}
	return 0;
}
