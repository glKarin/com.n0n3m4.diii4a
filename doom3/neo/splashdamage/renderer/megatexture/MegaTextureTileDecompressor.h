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

#ifndef __MEGATEXTURE_TILE_DECOMPRESSOR_H__
#define __MEGATEXTURE_TILE_DECOMPRESSOR_H__

#ifdef _USING_STDCXX
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#endif

class idBareDctDecoder;
class idDxtEncoder;
class idMegaTexture;
class idMegaTextureLevel;
class idMegaTextureTile;

class idMegaTextureTileDecompressor {
public:
	struct compressedTileData_t {
		int			globalX;
		int			globalY;
		byte *			data;
		int			size;
		int			parentLevelNum;
		int			parentGlobalX;
		int			parentGlobalY;
		byte *			parentData;
		int			parentSize;
		int			parentCachedLevelNum;
		int			parentCachedGlobalX;
		int			parentCachedGlobalY;
		byte *			parentCachedData;
	};

					idMegaTextureTileDecompressor();
					~idMegaTextureTileDecompressor();

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

	void			RecompressTile( imageCompressionFormat_t format, byte *source, byte *tileData );
	void			RecompressTile_MMX( imageCompressionFormat_t format, byte *source, byte *tileData );
	void			RecompressTile_SSE2( imageCompressionFormat_t format, byte *source, byte *tileData );
	void			RecompressTile_Xenon( imageCompressionFormat_t format, byte *source, byte *tileData );

	static idCVar	r_megaTilesPerSecond;
	static idCVar	r_megaShowGrid;
	static idCVar	r_megaShowTileSize;

private:
	void			StartThread();
	void			StopThread();
	idMegaTexture *BeginActiveMegaTextureWork();
	void			EndActiveMegaTextureWork( idMegaTexture *megaTexture );
	void			GetCompressedTileData( idMegaTexture *mega, idMegaTextureLevel *level, idMegaTextureTile *tile );
	void			SnapshotCompressedTileData();
	const byte *	SetQuality( const byte *data );
	void			DecompressLuminance( byte *destination );
	void			DecompressTile( megaCompressionFormat_t format, byte *destination );
	void			DeRecompressTile( idMegaTextureLevel *level, byte *destination );

	const byte *	SetQuality_MMX( const byte *data );
	void			DecompressLuminance_MMX( byte *destination );
	void			DecompressTile_MMX( megaCompressionFormat_t format, byte *destination );
	void			DeRecompressTile_MMX( idMegaTextureLevel *level, byte *destination );
	const byte *	SetQuality_SSE2( const byte *data );
	void			DecompressLuminance_SSE2( byte *destination );
	void			DecompressTile_SSE2( megaCompressionFormat_t format, byte *destination );
	void			DeRecompressTile_SSE2( idMegaTextureLevel *level, byte *destination );
	const byte *	SetQuality_Xenon( const byte *data );
	void			DecompressLuminance_Xenon( byte *destination );
	void			DecompressTile_Xenon( megaCompressionFormat_t format, byte *destination );
	void			DeRecompressTile_Xenon( idMegaTextureLevel *level, byte *destination );

	idMegaTextureTile *FindTileToDecode( idMegaTexture *mega, idMegaTextureLevel *&level );

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
	idBareDctDecoder *dctDecoder;
	idDxtEncoder *	dxtEncoder;
	compressedTileData_t compressedData;
	idList<byte>	compressedTileSnapshot;
	idList<byte>	parentCompressedTileSnapshot;
	idMegaTexture *	activeMegaTexture;
	idMegaTexture *	workerMegaTexture;
	int				lastProcessedTime;
	int				numTilesThisMsec;
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

#endif
