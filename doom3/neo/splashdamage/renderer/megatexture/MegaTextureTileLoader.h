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

#ifndef __MEGATEXTURE_TILE_LOADER_H__
#define __MEGATEXTURE_TILE_LOADER_H__

#ifdef _USING_STDCXX
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#endif

class idMegaTexture;
class idMegaTextureTile;

class idMegaTextureTileLoader {
public:
					idMegaTextureTileLoader();
					~idMegaTextureTileLoader();

	void			Init();
	void			Shutdown();
	void			SetActiveMegaTexture( idMegaTexture *megaTexture );
	idMegaTexture *	GetActiveMegaTexture() const;
	void			ForceUpdate();
	void			ResetNumProcessedTiles();
	int				GetNumProcessedTiles() const;
	void			SignalThread();
	unsigned int	Run( void *parameter );
	void			Stop();

private:
	void			StartThread();
	void			StopThread();
	idMegaTexture *BeginActiveMegaTextureWork();
	void			EndActiveMegaTextureWork( idMegaTexture *megaTexture );
	int				LoadTile( byte *destination, idMegaTexture *megaTexture, int tileNum );
	idMegaTextureTile *FindTileToLoad( idMegaTexture *megaTexture );
	void			LoadInterleavedChildren( idMegaTexture *megaTexture, idMegaTextureTile *parentTile );

#ifdef _USING_STDCXX
	std::thread *	thread;
	mutable std::mutex stateMutex;
	std::condition_variable signal;
	std::condition_variable throttleSignal;
	std::condition_variable workerIdleSignal;
#else
	sdThread *	thread;
	mutable sdLock stateMutex;
	void ThreadPlaceholder() {}
	sdSignal signal;
	sdSignal throttleSignal;
	sdSignal workerIdleSignal;
#endif
	idMegaTexture *	activeMegaTexture;
	idMegaTexture *	workerMegaTexture;
#ifdef _USING_STDCXX
	std::atomic<int> numProcessedTiles;
	std::atomic<bool> terminate;
	std::atomic<bool> forceUpdate;
#else
	sdAtomic<int> numProcessedTiles;
	sdAtomic<bool> terminate;
	sdAtomic<bool> forceUpdate;
#endif
};

int GetMegaTilesPerSecond();
int GetCompressedUsefulKiloBytesReadPerSecond();
int GetCompressedSeekMBPerSecond();
int GetCompressedSeeksPerSecond();
float GetTilesPerSeek();

#endif
