/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/
#include "precompiled.h"
#pragma hdrstop



#include "snd_local.h"
#include <limits.h>
#include "framework/LoadStack.h"
#include "framework/DeclSubtitles.h"
#include "containers/ProducerConsumerQueue.h"

#define USE_SOUND_CACHE_ALLOCATOR

#ifdef USE_SOUND_CACHE_ALLOCATOR
static idDynamicBlockAlloc<byte, 1<<20, 1<<10>	soundCacheAllocator;
#else
static idDynamicAlloc<byte, 1<<20, 1<<10>		soundCacheAllocator;
#endif
static idSysMutex soundCacheAllocatorMutex;	// only needed when loading from non-main thread


/*
===================
LoadSrtFile
===================
*/
static bool LoadSrtFile( const char *filename, idList<Subtitle> &subtitles ) {
	subtitles.Clear();
	TRACE_CPU_SCOPE_TEXT( "LoadSrtFile", filename );

	char *buffer = nullptr;
	int len = fileSystem->ReadFile( filename, (void**)&buffer );
	if ( len < 0 ) {
		return false;
	}
	idStr text( buffer, 0, len );
	fileSystem->FreeFile( buffer );

	auto ParseTimeOffset = []( const char *text, int &offset ) -> bool {
		static const char FORMAT[13] = "dd:dd:dd,ddd";
		static const int MULTIPLIER[12] = { 10, 10, 0, 6, 10, 0, 6, 10, 0, 10, 10, 10 };
		uint64_t millis = 0;
		for ( int i = 0; i < 12; i++ ) {
			char ch = text[i], fmt = FORMAT[i];
			if ( fmt == 'd' ) {
				if ( ch >= '0' && ch < '0' + MULTIPLIER[i] ) {
					millis = millis * MULTIPLIER[i] + ( ch - '0' );
				} else {
					return false;
				}
			} else if ( ch != fmt ) {
				return false;
			}
		}
		offset = int( millis * PRIMARYFREQ / 1000 );
		return true;
	};

	idStrList lines = text.SplitLines();
	int lineno = 0;
	while ( lineno + 2 <= lines.Num() ) {
		const idStr &lineSeqno = lines[lineno++];
		const idStr &lineTiming = lines[lineno++];

		int seqno = atoi( lineSeqno.c_str() );
		if ( seqno != subtitles.Num() + 1 ) {
			common->Warning( "Wrong sequence number %d in %s", seqno, filename );
			return false;
		}

		if ( lineTiming.Length() != 29 || idStr::Cmpn(lineTiming.c_str() + 12, " --> ", 5) != 0 ) {
			common->Warning( "Bad timing at seqno %d in %s", seqno, filename );
			return false;
		}
		Subtitle sub;
		if ( !ParseTimeOffset(lineTiming.c_str() + 0, sub.offsetStart) || !ParseTimeOffset(lineTiming.c_str() + 17, sub.offsetEnd) ) {
			common->Warning( "Failed to parse timing at seqno %d in %s", seqno, filename );
			return false;
		}

		while ( lineno < lines.Num() && lines[lineno].Length() > 0 ) {
			if ( sub.text.Length() > 0 )
				sub.text += '\n';
			sub.text += lines[lineno++];
		}
		lineno++;

		if ( sub.offsetStart >= sub.offsetEnd ) {
			common->Warning( "Subtitle %d discarded in %s", seqno, filename );
			continue;
		}
		subtitles.Append( sub );
	}

	bool sorted = true;
	for ( int i = 1; i < subtitles.Num(); i++ )
		if ( subtitles[i-1].offsetStart > subtitles[i].offsetStart ) {
			common->Warning( "Subtitles %d and %d not sorted by start offset", i, i+1 );
			sorted = false;
			break;
		}
	if ( !sorted ) {
		auto CmpSub = [](const Subtitle *a, const Subtitle *b) -> int {
			int diff = a->offsetStart - b->offsetStart;
			return ( diff < 0 ? -1 : ( diff > 0 ? 1 : 0 ) );
		};
		subtitles.Sort( CmpSub );
	}

	return true;
}

idCVar tdm_subtitles_inlineDurationExtension(
	"tdm_subtitles_inlineDurationExtension", "0.2", CVAR_FLOAT | CVAR_SOUND | CVAR_ARCHIVE,
	"Inline type of subtitles is extended by this number of seconds after sound ends."
);
idCVar tdm_subtitles_inlineDurationMinimum(
	"tdm_subtitles_inlineDurationMinimum", "1.0", CVAR_FLOAT | CVAR_SOUND | CVAR_ARCHIVE,
	"Inline type of subtitles lasts at least this number of seconds."
);
idCVar tdm_subtitles_durationExtensionLimit(
	"tdm_subtitles_durationExtensionLimit", "5.0", CVAR_FLOAT | CVAR_SOUND | CVAR_ARCHIVE,
	"Any subtitle can last no more that this number of seconds after the sound ends."
);

/*
===================
idSoundCache::LoadSubtitles()
===================
*/
void idSoundSample::LoadSubtitles() {
	subtitles.Clear();
	subtitlesVerbosity = SUBL_MISSING;
	subtitleTotalDuration = 0;

	const idDeclSubtitles *allSubs = (idDeclSubtitles *) declManager->FindType( DECL_SUBTITLES, "tdm_root" );
	if ( !allSubs )
		return;

	const subtitleMapping_t *mapping = allSubs->FindSubtitleForSound( name.c_str() );
	if ( !mapping )
		return;

	subtitlesVerbosity = mapping->verbosityLevel;

	if ( mapping->srtFileName.Length() ) {
		const char *srtname = mapping->srtFileName.c_str();
		// load .srt file
		if ( !LoadSrtFile( srtname, subtitles ) ) {
			common->Warning(
				"Couldn't load SRT file '%s' for sound '%s' according to decl '%s'",
				srtname, name.c_str(), mapping->owner->GetName()
			);
			subtitles.Clear();
			subtitlesVerbosity = SUBL_MISSING;
		}
	}
	else {
		// inline subtitle
		float durationExtend = tdm_subtitles_inlineDurationExtension.GetFloat();
		if ( mapping->inlineDurationExtend >= 0.0f )
			durationExtend = mapping->inlineDurationExtend;
		int durAdd = int( durationExtend * PRIMARYFREQ );
		int durMin = int( tdm_subtitles_inlineDurationMinimum.GetFloat() * PRIMARYFREQ );
		Subtitle sub;
		sub.offsetStart = 0;
		// #6262: inline subtitles are extended slightly
		sub.offsetEnd = idMath::Imax( DurationIn44kHzSamples() + durAdd, durMin );
		sub.text = mapping->inlineText;
		subtitles.Append( sub );
	}

	for ( int i = 0; i < subtitles.Num(); i++ ) {
		int endsAt = subtitles[i].offsetEnd;
		subtitleTotalDuration = idMath::Imax( subtitleTotalDuration, endsAt );
	}
	// hard-cap extension of subtitles beyond sound for safety
	int durCap = DurationIn44kHzSamples() + int( tdm_subtitles_durationExtensionLimit.GetFloat() * PRIMARYFREQ );
	if ( subtitleTotalDuration > durCap )
		subtitleTotalDuration = durCap;
}

/*
===================
idSoundCache::idSoundCache()
===================
*/
idSoundCache::idSoundCache() {
	soundCacheAllocator.Init();
	soundCacheAllocator.SetLockMemory( true );
	listCache.SetGranularity( 256 );
	cacheHash.ClearFree( 1024, 1024 );
	cacheHash.SetGranularity( 256 );
	insideLevelLoad = false;
	insideParallelLoading = false;
}

/*
===================
idSoundCache::~idSoundCache()
===================
*/
idSoundCache::~idSoundCache() {
	listCache.DeleteContents( true );
	cacheHash.ClearFree();
	soundCacheAllocator.Shutdown();
}

/*
===================
idSoundCache::::GetObject

returns a single cached object pointer
===================
*/
const idSoundSample* idSoundCache::GetObject( const int index ) const {
	if (index < 0 || index >= listCache.Num()) {
		return NULL;
	}
	return listCache[index]; 
}

idCVar s_levelLoadParallel(
	"s_levelLoadParallel", "1", CVAR_SOUND | CVAR_BOOL,
	"Delay loading of sound samples during level load, but then load them all in parallel"
);

/*
===================
idSoundCache::FindSound

Adds a sound object to the cache and returns a handle for it.
===================
*/
idSoundSample *idSoundCache::FindSound( const idStr& filename ) {
	idStr fname;
	fname = filename;
	fname.BackSlashesToSlashes();
	fname.ToLower();

	declManager->MediaPrint( "%s\n", fname.c_str() );

	// check to see if object is already in cache
	int hash = cacheHash.GenerateKey( fname, true );
	int index;
	for ( index = cacheHash.First( hash ); index != -1; index = cacheHash.Next( index ) ) {
		idSoundSample *def = listCache[index];
		if ( def->name == fname )
			break;		// already in cache
	}

	if ( index < 0 ) {	// missing in cache

#if _DEBUG
		// verify that hash index is correct and we don't miss sound
		for( int i = 0; i < listCache.Num(); i++ ) {
			idSoundSample *def = listCache[i];
			assert( !( def && def->name == fname ) );
		}
#endif
		// create a new entry
		idSoundSample *def = new idSoundSample;

		assert( listCache.FindNull() == -1 );
		index = listCache.Append( def );
		cacheHash.Add( hash, index );

		def->name = fname;
		def->purged = true;
	}

	// continue loading (regardless of whether sound is new or not)
	idSoundSample *def = listCache[index];
	def->levelLoadReferenced = true;

	// save/update reference path for this sound
	delete def->loadStack;
	def->loadStack = new LoadStack(declManager->GetLoadStack());

	bool delayDueToLevelLoad = s_levelLoadParallel.GetBool() && insideLevelLoad;
	if ( def->purged && !delayDueToLevelLoad ) {
		// this may make it a default sound if it can't be loaded
		def->Load();
	}
	return def;
}

/*
===================
idSoundCache::ReloadSounds

Completely nukes the current cache
===================
*/
void idSoundCache::ReloadSounds( bool force ) {
	for( int i = 0; i < listCache.Num(); i++ ) {
		idSoundSample *def = listCache[i];
		if ( def ) {
			def->Reload( force );
		}
	}
}

/*
===================
idSoundCache::ReloadSubtitles
===================
*/
void idSoundCache::ReloadSubtitles() {
	for( int i = 0; i < listCache.Num(); i++ ) {
		idSoundSample *def = listCache[i];
		if ( def ) {
			def->LoadSubtitles();
		}
	}
}

/*
====================
BeginLevelLoad

Mark all file based images as currently unused,
but don't free anything.  Calls to ImageFromFile() will
either mark the image as used, or create a new image without
loading the actual data.
====================
*/
void idSoundCache::BeginLevelLoad() {
	insideLevelLoad = true;

	for ( int i = 0 ; i < listCache.Num() ; i++ ) {
		idSoundSample *sample = listCache[ i ];
		if ( !sample ) {
			continue;
		}

		if ( com_purgeAll.GetBool() ) {
			sample->PurgeSoundSample();
		}

		sample->levelLoadReferenced = false;
	}

	soundCacheAllocator.FreeEmptyBaseBlocks();
}

/*
====================
EndLevelLoad

Free all samples marked as unused
====================
*/
void idSoundCache::EndLevelLoad() {
	int	useCount, purgeCount;
	common->Printf( "----- idSoundCache::EndLevelLoad -----\n" );

	insideLevelLoad = false;
	session->UpdateLoadingProgressBar( PROGRESS_STAGE_SOUNDS, 0.0f );

	// purge the ones we don't need
	useCount = 0;
	purgeCount = 0;
	// stgatilov: collect list of sounds we still need to load
	idList<idSoundSample*> soundsToLoad;

	for ( int i = 0 ; i < listCache.Num() ; i++ ) {
		idSoundSample *sample = listCache[ i ];
		if ( !sample->levelLoadReferenced ) {
			if ( !sample->purged ) {
				purgeCount += sample->objectMemSize;
				sample->PurgeSoundSample();
			}
		} else {
			if ( s_levelLoadParallel.GetBool() && sample->purged )
				soundsToLoad.AddGrow(sample);
		}
	}

	if ( soundsToLoad.Num() > 0 ) {
		insideParallelLoading = true;

		static idProducerConsumerQueue<idSoundSample**> queue;
		queue.ClearFree();
		// I suppose we don't need bounded buffer here...

		auto LoadSoundJobFunc = []( void *param ) {
			idSoundSample **ppSound = (idSoundSample **)param;
			(*ppSound)->Load_Stage1();
			queue.Append( ppSound );
		};
		RegisterJob( LoadSoundJobFunc, "loadSound" );

		int n = soundsToLoad.Num();
		idParallelJobList *joblist = parallelJobManager->AllocJobList( JOBLIST_UTILITY, JOBLIST_PRIORITY_MEDIUM, n, 0, nullptr );
		for ( int i = 0; i < n; i++ ) {
			joblist->AddJob( LoadSoundJobFunc, &soundsToLoad[i] );
		}
		joblist->Submit( nullptr, JOBLIST_PARALLELISM_NONINTERACTIVE | JOBLIST_PARALLELISM_FLAG_DISK );

		int finishedCount = 0;
		while ( finishedCount < n ) {
			idSoundSample **ppImage = queue.Pop();
			(*ppImage)->Load_Stage2();
			finishedCount++;
			session->UpdateLoadingProgressBar( PROGRESS_STAGE_SOUNDS, float(finishedCount) / n );
		}

		parallelJobManager->FreeJobList( joblist );
		queue.ClearFree();

		insideParallelLoading = false;
	}

	soundCacheAllocator.FreeEmptyBaseBlocks();

	for ( int i = 0 ; i < listCache.Num() ; i++ ) {
		idSoundSample *sample = listCache[ i ];
		if ( !sample->purged )
			useCount += sample->objectMemSize;
	}

	common->Printf( "%5ik referenced\n", useCount / 1024 );
	common->Printf( "%5ik purged\n", purgeCount / 1024 );
	common->Printf( "----------------------------------------\n" );
	session->UpdateLoadingProgressBar( PROGRESS_STAGE_SOUNDS, 1.0f );
}

/*
===================
idSoundCache::PrintMemInfo
===================
*/
void idSoundCache::PrintMemInfo( MemInfo_t *mi ) {
	int i, j, num = 0, total = 0;
	int *sortIndex;
	idFile *f;

	f = fileSystem->OpenFileWrite( mi->filebase + "_sounds.txt" );
	if ( !f ) {
		return;
	}

	// count
	for ( i = 0; i < listCache.Num(); i++, num++ ) {
		if ( !listCache[i] ) {
			break;
		}
	}

	// sort first
	sortIndex = new int[num];

	for ( i = 0; i < num; i++ ) {
		sortIndex[i] = i;
	}

	for ( i = 0; i < num - 1; i++ ) {
		for ( j = i + 1; j < num; j++ ) {
			if ( listCache[sortIndex[i]]->objectMemSize < listCache[sortIndex[j]]->objectMemSize ) {
				int temp = sortIndex[i];
				sortIndex[i] = sortIndex[j];
				sortIndex[j] = temp;
			}
		}
	}

	// print next
	for ( i = 0; i < num; i++ ) {
		idSoundSample *sample = listCache[sortIndex[i]];

		// this is strange
		if ( !sample ) {
			continue;
		}

		total += sample->objectMemSize;
		f->Printf( "%s %s\n", idStr::FormatNumber( sample->objectMemSize ).c_str(), sample->name.c_str() );
	}

	mi->soundAssetsTotal = total;

	f->Printf( "\nTotal sound bytes allocated: %s\n", idStr::FormatNumber( total ).c_str() );
	fileSystem->CloseFile( f );
	delete[] sortIndex;
}

byte *idSoundCache::AllocLocked( int bytes ) const {
	idScopedCriticalSection lock;
	if ( insideParallelLoading ) {
		lock.Lock( soundCacheAllocatorMutex );
	}
	return soundCacheAllocator.Alloc( bytes );
}

void idSoundCache::FreeLocked( byte *ptr ) const {
	idScopedCriticalSection lock;
	if ( insideParallelLoading ) {
		lock.Lock( soundCacheAllocatorMutex );
	}
	return soundCacheAllocator.Free( ptr );
}


/*
==========================================================================

idSoundSample

==========================================================================
*/

/*
===================
idSoundSample::idSoundSample
===================
*/
idSoundSample::idSoundSample() {
	memset( &objectInfo, 0, sizeof(waveformatex_t) );
	objectSize = 0;
	objectMemSize = 0;
	nonCacheData = NULL;
	amplitudeData = NULL;
	openalBuffer = 0;
	hardwareBuffer = false;
	defaultSound = false;
	purged = false;
	levelLoadReferenced = false;
	cinematic = NULL;
	subtitlesVerbosity = SUBL_MISSING;
	subtitleTotalDuration = 0;
	loadStack = nullptr;
}

/*
===================
idSoundSample::~idSoundSample
===================
*/
idSoundSample::~idSoundSample() {
	PurgeSoundSample();
}

/*
===================
idSoundSample::LengthIn44kHzSamples
===================
*/
int idSoundSample::LengthIn44kHzSamples( void ) const {
	// objectSize is samples
	if ( objectInfo.nSamplesPerSec == 11025 ) {
		return objectSize << 2;
	} else if ( objectInfo.nSamplesPerSec == 22050 ) {
		return objectSize << 1;
	} else {
		return objectSize << 0;			
	}
}

/*
===================
idSoundSample::DurationIn44kHzSamples
===================
*/
int idSoundSample::DurationIn44kHzSamples( void ) const
{
	int length = LengthIn44kHzSamples();
	if ( objectInfo.nChannels == 1 )
		return length;
	if ( objectInfo.nChannels == 2 )
		return length >> 1;
	return length / objectInfo.nChannels;
}

/*
===================
idSoundSample::MakeDefault
===================
*/
void idSoundSample::MakeDefault( void ) {	
	memset( &objectInfo, 0, sizeof( objectInfo ) );
	objectInfo.wFormatTag = WAVE_FORMAT_TAG_PCM;
	objectInfo.nChannels = 1;
	objectInfo.wBitsPerSample = 16;
	objectInfo.nSamplesPerSec = 44100;

	objectSize = MIXBUFFER_SAMPLES * 2;
	objectMemSize = objectSize * sizeof( short );

	nonCacheData = (byte *)soundCacheAllocator.Alloc( objectMemSize );

	short *ncd = (short *)nonCacheData;
	for ( int i = 0; i < MIXBUFFER_SAMPLES; i ++ ) {
		float v = sin( idMath::PI * 2 * i / 64 );
		int sample = v * 0x4000;
		ncd[i*2+0] = sample;
		ncd[i*2+1] = sample;
	}

	// preload sound data and play in non-streaming way
	// stgatilov #6330: the new default is to play everything as streaming and avoid non-streaming code path completely
	if ( idSoundSystemLocal::s_realTimeDecoding.GetInteger() < 2 ) {
		alGetError();
		alGenBuffers(1, &openalBuffer);
		if (alGetError() != AL_NO_ERROR) {
			common->Error("idSoundCache: error generating OpenAL hardware buffer");
		}

		alGetError();
		alBufferData(openalBuffer, objectInfo.nChannels == 1 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16, nonCacheData, objectMemSize, objectInfo.nSamplesPerSec);
		if (alGetError() != AL_NO_ERROR) {
			common->Error("idSoundCache: error loading data into OpenAL hardware buffer");
		}
		else {
			hardwareBuffer = true;
		}
	}

	defaultSound = true;

	subtitles.Clear();
	subtitlesVerbosity = SUBL_MISSING;
	subtitleTotalDuration = 0;
}

/*
===================
idSoundSample::CheckForDownSample
===================
*/
void idSoundSample::CheckForDownSample( void ) {
	if ( !idSoundSystemLocal::s_force22kHz.GetBool() ) {
		return;
	}
	if ( objectInfo.wFormatTag != WAVE_FORMAT_TAG_PCM || objectInfo.nSamplesPerSec != 44100 ) {
		return;
	}
	int shortSamples = objectSize >> 1;
	short *converted = (short *)soundSystemLocal.soundCache->AllocLocked( shortSamples * sizeof( short ) );

	if ( objectInfo.nChannels == 1 ) {
		for ( int i = 0; i < shortSamples; i++ ) {
			converted[i] = ((short *)nonCacheData)[i*2];
		}
	} else {
		for ( int i = 0; i < shortSamples; i += 2 ) {
			converted[i+0] = ((short *)nonCacheData)[i*2+0];
			converted[i+1] = ((short *)nonCacheData)[i*2+1];
		}
	}
	soundSystemLocal.soundCache->FreeLocked( nonCacheData );
	nonCacheData = (byte *)converted;
	objectSize = shortSamples;
	objectMemSize = shortSamples * sizeof( short );
	objectInfo.nAvgBytesPerSec >>= 1;
	objectInfo.nSamplesPerSec >>= 1;
}

/*
===================
idSoundSample::GetNewTimeStamp
===================
*/
ID_TIME_T idSoundSample::GetNewTimeStamp( void ) const {
	ID_TIME_T timestamp;

	fileSystem->ReadFile( name, NULL, &timestamp );
	if ( timestamp == FILE_NOT_FOUND_TIMESTAMP ) {
		idStr oggName = name;
		oggName.SetFileExtension( ".ogg" );
		fileSystem->ReadFile( oggName, NULL, &timestamp );
	}
	return timestamp;
}

inline void IssueSoundSampleFailure(const char* message, ALenum errorCode, const char* soundName)
{
	common->Error("%s: file: %s, error code: %d", message, soundName, errorCode);
}

void idSoundSample::LoadFromCinematic(idCinematic *cin) {
	if (!cin) {
		MakeDefault();
		return;
	}

	cinematic = cin;

	objectInfo.wFormatTag = WAVE_FORMAT_TAG_STREAM_CINEMATICS;
	objectInfo.nChannels = 2;	//stereo
	objectInfo.nSamplesPerSec = PRIMARYFREQ;	//44100 hz
	objectInfo.nAvgBytesPerSec = 0;	//nobody cares
	objectInfo.nBlockAlign = sizeof(float) * objectInfo.nChannels;
	objectInfo.wBitsPerSample = sizeof(float) * 8;
	objectInfo.cbSize = 0;

	//cinematic decides when it ends: set infinite duration here
	objectSize = INT_MAX / 2;

	LoadSubtitles();
}

/*
===================
idSoundSample::Load

Loads based on name, possibly doing a MakeDefault if necessary
===================
*/
void idSoundSample::Load( void ) {	
	Load_Stage1();
	Load_Stage2();
}

void idSoundSample::Load_Stage1() {
	TRACE_CPU_SCOPE_STR("LoadStage1:Sound", name)

	defaultSound = false;
	purged = false;
	hardwareBuffer = false;
	loadError.Clear();

	// special cases better be handled in sequental phase
	if (name.IcmpPrefix("fromVideo __testvideo") == 0) 
		return;
	if (name.IcmpPrefix("fromVideo ") == 0)
		return;

	// load it
	idWaveFile fh;
	waveformatex_t info;

	if ( fh.Open( name, &info ) == -1 ) {
		loadError = "%%file not found%%";	// special string
		timestamp = -1;
		return;
	}

	// save timestamp of opened file
	timestamp = fh.Timestamp();

	if ( info.nChannels != 1 && info.nChannels != 2 ) {
		loadError = va( "idSoundSample: %s has %i channels, using default", name.c_str(), info.nChannels );
		fh.Close();
		return;
	}

	if ( info.wBitsPerSample != 16 ) {
		loadError = va( "idSoundSample: %s is %dbits, expected 16bits using default", name.c_str(), info.wBitsPerSample );
		fh.Close();
		return;
	}

	if ( info.nSamplesPerSec != 44100 && info.nSamplesPerSec != 22050 && info.nSamplesPerSec != 11025 ) {
		loadError = va( "idSoundCache: %s is %dHz, expected 11025, 22050 or 44100 Hz. Using default", name.c_str(), info.nSamplesPerSec );
		fh.Close();
		return;
	}

	objectInfo = info;
	objectSize = fh.GetOutputSize();
	objectMemSize = fh.GetMemorySize();

	nonCacheData = soundSystemLocal.soundCache->AllocLocked( objectMemSize );
	fh.Read( nonCacheData, objectMemSize, NULL );

	fh.Close();

	// optionally convert it to 22kHz to save memory
	CheckForDownSample();
}

void idSoundSample::Load_Stage2() {
	TRACE_CPU_SCOPE_STR("LoadStage2:Sound", name)

	if ( !loadError.IsEmpty() ) {
		if ( loadError == "%%file not found%%" ) {
			common->Warning( "Couldn't load sound '%s' using default", name.c_str() );
			if ( loadStack )
				loadStack->PrintStack( 2, LoadStack::LevelOf(this) );
		} else {
			common->Warning( "%s", loadError.c_str() );
		}
		loadError.ClearFree();
		MakeDefault();
		return;
	}

	//stgatilov #4847: check if this sound was created via testVideo command
	if (name.IcmpPrefix("fromVideo __testvideo") == 0) {
		idCinematic *cin = 0;
		if (sscanf(name.c_str() + 22, "%p__", &cin) == 1)
			return LoadFromCinematic(cin);
	}
	if (name.IcmpPrefix("fromVideo ") == 0) {
		//stgatilov #4534: material name is specified after "fromVideo" prefix
		//in such case this material must have cinematics, and sound is taken from there
		const char *materialName = name.c_str() + 10;
		const idMaterial *material = declManager->FindMaterial(materialName);
		idCinematic *cin = material->GetCinematic();
		return LoadFromCinematic(cin);
	}

	if ( idSoundSystemLocal::s_realTimeDecoding.GetInteger() < 2 ) {
		// create hardware audio buffers 
		// stgatilov #6330: this case is disabled because non-streaming sounds cause various problems

		// PCM loads directly
		if (objectInfo.wFormatTag == WAVE_FORMAT_TAG_PCM) {
			alGetError();
			alGenBuffers(1, &openalBuffer);

			ALenum errorCode = alGetError();

			if (errorCode != AL_NO_ERROR)
				IssueSoundSampleFailure("idSoundCache: WAV error generating OpenAL hardware buffer", errorCode, name.c_str());
			if (alIsBuffer(openalBuffer)) {
				alGetError();
				alBufferData(openalBuffer, objectInfo.nChannels == 1 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16, nonCacheData, objectMemSize, objectInfo.nSamplesPerSec);

				errorCode = alGetError();
				if (errorCode != AL_NO_ERROR)
				{
					IssueSoundSampleFailure("idSoundCache: WAV error loading data into OpenAL hardware buffer", errorCode, name.c_str());
				}
				else {
					hardwareBuffer = true;
				}
			}
		}

		// OGG decompressed at load time (when smaller than s_decompressionLimit seconds, 6 seconds by default)
		if ( objectInfo.wFormatTag == WAVE_FORMAT_TAG_OGG ) {
			if ( ( objectSize < ( ( int ) objectInfo.nSamplesPerSec * idSoundSystemLocal::s_decompressionLimit.GetInteger() ) ) ) {
				alGetError();
				alGenBuffers( 1, &openalBuffer );
				ALenum errorCode = alGetError();
				if (errorCode != AL_NO_ERROR)
				{
					IssueSoundSampleFailure("idSoundCache: OGG error generating OpenAL hardware buffer", errorCode, name.c_str());
				}
				if ( alIsBuffer( openalBuffer ) ) {
					idSampleDecoder *decoder = idSampleDecoder::Alloc();
					float *floatData = (float *)soundCacheAllocator.Alloc( ( LengthIn44kHzSamples() + 1 ) * sizeof( float ) );
					short *shortData = (short *)soundCacheAllocator.Alloc( ( objectSize + 1 ) * sizeof( short ) );

					// Decoder *always* outputs 44 kHz data
					decoder->Decode( this, 0, LengthIn44kHzSamples(), floatData );

					// Downsample back to original frequency (save memory)
					assert( 44100 % objectInfo.nSamplesPerSec == 0 );
					int downsampleDivisor = 44100 / objectInfo.nSamplesPerSec;
					for ( int i = 0; i < objectSize; i++ ) {
						shortData[i] = idMath::FtoiRound( idMath::ClampFloat( -32768.0f, 32767.0f, floatData[i * downsampleDivisor] ) );
					}

					alGetError();
					alBufferData( openalBuffer, objectInfo.nChannels == 1 ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16, shortData, objectSize * sizeof( short ), objectInfo.nSamplesPerSec );
					ALenum alError = alGetError();
					if ( alError != AL_NO_ERROR )
						IssueSoundSampleFailure("idSoundCache Load OGG: error loading data into OpenAL hardware buffer", alError, name.c_str());
					else {
						hardwareBuffer = true;
					}

					soundCacheAllocator.Free( (byte *)floatData );
					soundCacheAllocator.Free( (byte *)shortData );
					idSampleDecoder::Free( decoder );
				}
			}
		}
	}

	LoadSubtitles();
}

/*
===================
idSoundSample::PurgeSoundSample
===================
*/
void idSoundSample::PurgeSoundSample() {
	purged = true;

	alGetError();
	alDeleteBuffers(1, &openalBuffer);
	if (alGetError() != AL_NO_ERROR) {
		common->Error("idSoundCache: error unloading data from OpenAL hardware buffer");
	}
	else {
		openalBuffer = 0;
		hardwareBuffer = false;
	}

	if ( amplitudeData ) {
		soundCacheAllocator.Free( amplitudeData );
		amplitudeData = NULL;
	}

	if ( nonCacheData ) {
		soundCacheAllocator.Free( nonCacheData );
		nonCacheData = NULL;
	}

	delete loadStack;
	loadStack = nullptr;
}

/*
===================
idSoundSample::Reload
===================
*/
void idSoundSample::Reload( bool force ) {
	if ( !force ) {
		ID_TIME_T newTimestamp;

		// check the timestamp
		newTimestamp = GetNewTimeStamp();

		if ( newTimestamp == FILE_NOT_FOUND_TIMESTAMP ) {
			if ( !defaultSound ) {
				common->Warning( "Couldn't load sound '%s' using default", name.c_str() );
				MakeDefault();
			}
			return;
		}
		if ( newTimestamp == timestamp ) {
			return;	// don't need to reload it
		}
	}

	common->Printf( "reloading %s\n", name.c_str() );
	PurgeSoundSample();
	Load();
}

/*
===================
idSoundSample::FetchFromCache

Returns true on success.
===================
*/
bool idSoundSample::FetchFromCache( int offset, const byte **output, int *position, int *size, const bool allowIO ) {
	offset &= 0xfffffffe;

	if ( objectSize == 0 || offset < 0 || offset > objectSize * (int)sizeof( short ) || !nonCacheData ) {
		return false;
	}

	if ( output ) {
		*output = nonCacheData + offset;
	}
	if ( position ) {
		*position = 0;
	}
	if ( size ) {
		*size = objectSize * sizeof( short ) - offset;
		if ( *size > SCACHE_SIZE ) {
			*size = SCACHE_SIZE;
		}
	}
	return true;
}

bool idSoundSample::FetchFromCinematic(int sampleOffset, int *sampleSize, float *output) {
	assert(cinematic);
	return cinematic->SoundForTimeInterval(sampleOffset, sampleSize, output);
}

int idSoundSample::FetchSubtitles( int offset, idList<SubtitleMatch> &matches ) {
	if ( cinematic ) {
		//ask cinematic about current video time
		//otherwise subtitles will get out of sync with sound and video
		//easy to check by pausing with debugger while video is playing
		offset = cinematic->GetRealSoundOffset( offset );
	}

	//note: if this ever becomes too slow, we can implement "decoder" for subtitles
	//it can keep track of currently active matches, and position of current offset in the array...

	int cnt = 0;
	for ( int i = 0; i < subtitles.Num(); i++ ) {
		if ( offset >= subtitles[i].offsetStart && offset < subtitles[i].offsetEnd ) {
			SubtitleMatch m;
			m.subtitle = &subtitles[i];
			m.sample = this;
			m.verbosity = subtitlesVerbosity;
			m.channel = nullptr;		// will be set by caller
			m.emitter = nullptr;		// ...
			matches.AddGrow( m );
			cnt++;
		}
	}

	return cnt;
}

bool idSoundSample::HaveSubtitlesFinished( int offset ) const {
	if ( cinematic )
		offset = cinematic->GetRealSoundOffset( offset );
	return offset >= subtitleTotalDuration;
}
