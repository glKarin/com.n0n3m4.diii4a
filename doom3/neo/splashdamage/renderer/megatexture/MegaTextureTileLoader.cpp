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
#include "MegaTextureTileLoader.h"
#include "MegaTextureTileDecompressor.h"

#ifdef _USING_STDCXX
#include <chrono>
#include <vector>
#endif

idMegaTextureTileLoader *megaTextureTileLoader = NULL;

static const int MEGA_LOAD_HISTORY = 2048;
#ifdef _USING_STDCXX
static std::atomic<int> megaLoadHistoryIndex( 0 );
#else
static sdAtomic<int> megaLoadHistoryIndex( 0 );
#endif
static int megaLoadTimes[MEGA_LOAD_HISTORY];
static int megaLoadBytes[MEGA_LOAD_HISTORY];
static int megaLoadSeekBytes[MEGA_LOAD_HISTORY];

static void RecordMegaTileLoad( int bytes, int seekBytes ) {
#ifdef _USING_STDCXX
	const int index = megaLoadHistoryIndex.fetch_add( 1 ) & ( MEGA_LOAD_HISTORY - 1 );
#else
	const int index = megaLoadHistoryIndex.Add( 1 ) & ( MEGA_LOAD_HISTORY - 1 );
#endif
	megaLoadBytes[index] = bytes;
	megaLoadSeekBytes[index] = seekBytes;
	megaLoadTimes[index] = Sys_Milliseconds();
}

idMegaTextureTileLoader::idMegaTextureTileLoader() :
	thread( NULL ), activeMegaTexture( NULL ), workerMegaTexture( NULL ), numProcessedTiles( 0 ), terminate( false ), forceUpdate( false ) {
	memset( megaLoadTimes, 0, sizeof( megaLoadTimes ) );
	memset( megaLoadBytes, 0, sizeof( megaLoadBytes ) );
	memset( megaLoadSeekBytes, 0, sizeof( megaLoadSeekBytes ) );
}

idMegaTextureTileLoader::~idMegaTextureTileLoader() {
	Shutdown();
}

void idMegaTextureTileLoader::Init() {
	StartThread();
}

void idMegaTextureTileLoader::Shutdown() {
	StopThread();
	SetActiveMegaTexture( NULL );
}

void idMegaTextureTileLoader::StartThread() {
	if ( thread ) return;
	terminate = false;
#ifdef _USING_STDCXX
	thread = new std::thread( [this]() { Run( NULL ); } );
#else
	sdThreadProcessFunctor<idMegaTextureTileLoader> *runner = new sdThreadProcessFunctor<idMegaTextureTileLoader>();
	runner->Init(this, &idMegaTextureTileLoader::ThreadPlaceholder, &idMegaTextureTileLoader::Run, &idMegaTextureTileLoader::ThreadPlaceholder );
	thread = new sdThread( runner );
	thread->SetName( "idMegaTextureTileLoader" );
	thread->Start();
#endif
}

void idMegaTextureTileLoader::StopThread() {
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

void idMegaTextureTileLoader::SetActiveMegaTexture( idMegaTexture *mega ) {
#ifdef _USING_STDCXX
	std::unique_lock<std::mutex> stateLock( stateMutex );
	idMegaTexture *old = activeMegaTexture;
	if ( old == mega ) return;
	activeMegaTexture = mega;
	// Reads deliberately run without the MegaTexture lock. Keep the previous
	// resource alive until its in-flight read has completed.
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
	// Reads deliberately run without the MegaTexture lock. Keep the previous
	// resource alive until its in-flight read has completed.
	while ( old && workerMegaTexture == old ) {
		stateLock.Unlock();
		workerIdleSignal.Wait();
		stateLock.Lock();
	}
	stateLock.Unlock();
	SignalThread();
#endif
}

idMegaTexture *idMegaTextureTileLoader::GetActiveMegaTexture() const {
#ifdef _USING_STDCXX
	std::lock_guard<std::mutex> guard( stateMutex );
#else
	sdLockGuard<sdLock> guard( stateMutex );
#endif
	return activeMegaTexture;
}

idMegaTexture *idMegaTextureTileLoader::BeginActiveMegaTextureWork() {
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

void idMegaTextureTileLoader::EndActiveMegaTextureWork( idMegaTexture *mega ) {
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

void idMegaTextureTileLoader::ForceUpdate() {
	forceUpdate = true;
	SignalThread();
}

void idMegaTextureTileLoader::ResetNumProcessedTiles() {
	numProcessedTiles = 0;
}

int idMegaTextureTileLoader::GetNumProcessedTiles() const {
#ifdef _USING_STDCXX
	return numProcessedTiles.load();
#else
	return numProcessedTiles.Load();
#endif
}

void idMegaTextureTileLoader::SignalThread() {
#ifdef _USING_STDCXX
	signal.notify_one();
#else
	signal.Set();
#endif
}

void idMegaTextureTileLoader::Stop() {
	terminate = true;
#ifdef _USING_STDCXX
	signal.notify_all();
	throttleSignal.notify_all();
#else
	signal.Set();
	throttleSignal.Set();
#endif
}

idMegaTextureTile *idMegaTextureTileLoader::FindTileToLoad( idMegaTexture *mega ) {
	for ( int levelNumber = mega->GetNumLevels() - 1; levelNumber >= 0; --levelNumber ) {
		idMegaTextureLevel *level = mega->GetLevel( levelNumber );
		for ( idMegaTextureTile *tile = level->dirtyTiles.Next(); tile; tile = tile->dirtyNode.Next() ) {
			if ( tile->globalX < 0 || tile->globalY < 0 || tile->globalX >= level->tilesPerAxis || tile->globalY >= level->tilesPerAxis ) {
				tile->dirtyNode.Remove();
				return NULL;
			}
			if ( tile->IsLoaded() ) continue;
			if ( level->isInterleaved ) {
				idMegaTextureLevel *parentLevel = mega->GetLevel( levelNumber + 1 );
				idMegaTextureTile *parent = parentLevel ? parentLevel->GetTileLocal( ( tile->globalX >> 1 ) & 15, ( tile->globalY >> 1 ) & 15 ) : NULL;
				const int child = ( tile->globalX & 1 ) + 2 * ( tile->globalY & 1 );
				if ( parent && parent->GetChildCompressedTileData( child ) ) {
					tile->SetLoaded( true );
					continue;
				}
				if ( parent && !parent->dirtyNode.InList() ) parentLevel->AddDirtyTile( parent );
				continue;
			}
			return tile;
		}
	}
	return NULL;
}

int idMegaTextureTileLoader::LoadTile( byte *destination, idMegaTexture *mega, int tileNum ) {
	if ( !destination || tileNum < 0 ) return 0;
	const int bytes = mega->GetTileDataSize( tileNum ) + 3;
	const int seek = mega->SeekToTile( tileNum );
	if ( seek < 0 || mega->GetFile()->Read( destination, bytes ) != bytes ) return 0;
	RecordMegaTileLoad( bytes, seek );
	return bytes;
}

void idMegaTextureTileLoader::LoadInterleavedChildren( idMegaTexture *mega, idMegaTextureTile *parentTile ) {
	idMegaTextureLevel *parentLevel = parentTile->level;
	if ( !parentLevel || parentLevel->levelNum != 1 ) return;
	idMegaTextureLevel *childLevel = mega->GetLevel( 0 );
	if ( !childLevel || !childLevel->isInterleaved ) return;
	for ( int child = 0; child < 4; ++child ) {
		const int childX = parentTile->globalX * 2 + ( child & 1 );
		const int childY = parentTile->globalY * 2 + ( child >> 1 );
		if ( childX < 0 || childY < 0 || childX >= childLevel->tilesPerAxis || childY >= childLevel->tilesPerAxis ) continue;
		const int tileNum = childLevel->tileBase + childY * childLevel->tilesPerAxis + childX;
		if ( !parentTile->childCompressedTileData[child] ) {
			parentTile->childCompressedTileData[child] = new byte[childLevel->maxCompressedTileSize + 3];
			parentLevel->AddUsedMemory( childLevel->maxCompressedTileSize + 3 );
		}
		LoadTile( parentTile->childCompressedTileData[child], mega, tileNum );
		idMegaTextureTile *childTile = childLevel->GetTileLocal( childX & 15, childY & 15 );
		if ( childTile->globalX == childX && childTile->globalY == childY ) childTile->SetLoaded( true );
	}
}

unsigned int idMegaTextureTileLoader::Run( void *parameter ) {
	(void)parameter;
#ifdef _USING_STDCXX
	std::vector<byte> loadData[5];
#else
	idList<byte> loadData[5];
#endif
	while ( !terminate ) {
		idMegaTexture *mega = BeginActiveMegaTextureWork();
		bool didWork = false;
		if ( mega ) {
			idMegaTextureTile *tile = NULL;
			idMegaTextureLevel *level = NULL;
			int tileX = 0;
			int tileY = 0;
			int tileNums[5] = { -1, -1, -1, -1, -1 };
			int tileBytes[5] = { 0, 0, 0, 0, 0 };
			int childX[4] = { 0, 0, 0, 0 };
			int childY[4] = { 0, 0, 0, 0 };
			int loadCount = 0;
			{
#ifdef _USING_STDCXX
				std::lock_guard<std::recursive_mutex> guard( mega->GetLock() );
#else
				sdLockGuard<sdRecursiveLock> guard( mega->GetLock() );
#endif
				tile = FindTileToLoad( mega );
				if ( tile ) {
					level = tile->level;
					tileX = tile->globalX;
					tileY = tile->globalY;
					tileNums[0] = tile->GetTileNum();
					tileBytes[0] = mega->GetTileDataSize( tileNums[0] ) + 3;
					loadCount = 1;

					// Level zero is stored interleaved with each level-one parent.
					// Stage all five reads away from live tile buffers so a view
					// update can invalidate this job safely while I/O is in flight.
					if ( level && level->levelNum == 1 ) {
						idMegaTextureLevel *childLevel = mega->GetLevel( 0 );
						if ( childLevel && childLevel->isInterleaved ) {
							for ( int child = 0; child < 4; ++child ) {
								childX[child] = tileX * 2 + ( child & 1 );
								childY[child] = tileY * 2 + ( child >> 1 );
								if ( childX[child] < 0 || childY[child] < 0 ||
									 childX[child] >= childLevel->tilesPerAxis || childY[child] >= childLevel->tilesPerAxis ) {
									continue;
								}
								if ( !tile->childCompressedTileData[child] ) {
									tile->childCompressedTileData[child] = new byte[childLevel->maxCompressedTileSize + 3];
									level->AddUsedMemory( childLevel->maxCompressedTileSize + 3 );
								}
								tileNums[loadCount] = childLevel->tileBase + childY[child] * childLevel->tilesPerAxis + childX[child];
								tileBytes[loadCount] = mega->GetTileDataSize( tileNums[loadCount] ) + 3;
								++loadCount;
							}
						}
					}
				}
			}

			bool loaded = tile != NULL && loadCount > 0;
			for ( int i = 0; loaded && i < loadCount; ++i ) {
#ifdef _USING_STDCXX
				loadData[i].resize( tileBytes[i] );
				loaded = tileBytes[i] > 3 && LoadTile( loadData[i].data(), mega, tileNums[i] ) == tileBytes[i];
#else
				loadData[i].Resize( tileBytes[i] );
				loaded = tileBytes[i] > 3 && LoadTile( loadData[i].Ptr(), mega, tileNums[i] ) == tileBytes[i];
#endif
			}

			if ( loaded ) {
#ifdef _USING_STDCXX
				std::lock_guard<std::recursive_mutex> guard( mega->GetLock() );
#else
				sdLockGuard<sdRecursiveLock> guard( mega->GetLock() );
#endif
				if ( tile->globalX == tileX && tile->globalY == tileY ) {
					byte *destination = tile->GetCompressedTileData();
					if ( destination ) {
#ifdef _USING_STDCXX
						memcpy( destination, loadData[0].data(), tileBytes[0] );
#else
						memcpy( destination, loadData[0].Ptr(), tileBytes[0] );
#endif
						int loadIndex = 1;
						idMegaTextureLevel *childLevel = mega->GetLevel( 0 );
						if ( level && level->levelNum == 1 && childLevel && childLevel->isInterleaved ) {
							for ( int child = 0; child < 4 && loadIndex < loadCount; ++child ) {
								if ( childX[child] < 0 || childY[child] < 0 ||
									 childX[child] >= childLevel->tilesPerAxis || childY[child] >= childLevel->tilesPerAxis ) {
									continue;
								}
#ifdef _USING_STDCXX
								memcpy( tile->childCompressedTileData[child], loadData[loadIndex].data(), tileBytes[loadIndex] );
#else
								memcpy( tile->childCompressedTileData[child], loadData[loadIndex].Ptr(), tileBytes[loadIndex] );
#endif
								idMegaTextureTile *childTile = childLevel->GetTileLocal( childX[child] & 15, childY[child] & 15 );
								if ( childTile->globalX == childX[child] && childTile->globalY == childY[child] ) {
									childTile->SetLoaded( true );
								}
								++loadIndex;
							}
						}
						tile->SetLoaded( true );
						didWork = true;
						++numProcessedTiles;
					}
				}
			}
			EndActiveMegaTextureWork( mega );
		}
		if ( didWork ) {
			if ( megaTextureTileDecompressor ) megaTextureTileDecompressor->SignalThread();
			continue;
		}
#ifdef _USING_STDCXX
		std::unique_lock<std::mutex> waitLock( stateMutex );
		signal.wait_for( waitLock, std::chrono::milliseconds( 20 ) );
#else
		sdUniqueLock<sdLock> waitLock( stateMutex );
		waitLock.Unlock();
		signal.Wait( 20 );
		waitLock.Lock();
#endif
	}
	return 0;
}

static int MegaMetricSum( const int *values ) {
	const int threshold = Sys_Milliseconds() - 1000;
	int total = 0;
	for ( int i = 0; i < MEGA_LOAD_HISTORY; ++i ) if ( megaLoadTimes[i] >= threshold ) total += values[i];
	return total;
}

int GetMegaTilesPerSecond() {
	const int threshold = Sys_Milliseconds() - 1000;
	int count = 0;
	for ( int i = 0; i < MEGA_LOAD_HISTORY; ++i ) count += megaLoadTimes[i] >= threshold;
	return count;
}

int GetCompressedUsefulKiloBytesReadPerSecond() { return MegaMetricSum( megaLoadBytes ) / 1024; }
int GetCompressedSeekMBPerSecond() { return MegaMetricSum( megaLoadSeekBytes ) / ( 1024 * 1024 ); }

int GetCompressedSeeksPerSecond() {
	const int threshold = Sys_Milliseconds() - 1000;
	int count = 0;
	for ( int i = 0; i < MEGA_LOAD_HISTORY; ++i ) count += megaLoadTimes[i] >= threshold && megaLoadSeekBytes[i] > 0;
	return count;
}

float GetTilesPerSeek() {
	int tiles = 0;
	int seeks = 0;
	for ( int i = 0; i < MEGA_LOAD_HISTORY; ++i ) {
		if ( megaLoadTimes[i] > 0 ) {
			++tiles;
			seeks += megaLoadSeekBytes[i] > 0;
		}
	}
	return seeks ? (float)tiles / seeks : 0.0f;
}
