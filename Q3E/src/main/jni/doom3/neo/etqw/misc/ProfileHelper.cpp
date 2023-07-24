// Copyright (C) 2007 Id Software, Inc.
//


#include "../precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "ProfileHelper.h"

/*
===============================================================================

	sdProfileHelper

===============================================================================
*/

/*
================
sdProfileHelper::sdProfileHelper
================
*/
sdProfileHelper::sdProfileHelper( void ) {
	m_ProfileNode.SetOwner( this );
	m_ProfileName = "";
	m_HasCount = false;
	m_HasTotal = false;
	m_LogFileUpto = 0;
}

/*
================
sdProfileHelper::~sdProfileHelper
================
*/
sdProfileHelper::~sdProfileHelper( void ) {
	m_ProfileNode.Remove();
}

/*
================
sdProfileHelper::~sdProfileHelper
================
*/
void sdProfileHelper::Init( const char* name, bool hasTotal, bool hasCount ) {
	m_ProfileName = name;
	m_HasTotal = hasTotal;
	m_HasCount = hasCount;
	m_ProfileNode.AddToFront( sdProfileHelperManager::GetInstance().m_InactiveProfilers );
}

/*
================
sdProfileHelper::Start
================
*/
void sdProfileHelper::Start( void ) {
	if ( IsActive() ) {
		return;
	}

	// clear the list
	for ( int i = 0; i < m_SampleValues.Num(); i++ ) {
		delete *m_SampleValues.GetIndex( i );
	}
	m_SampleValues.Clear();

	if ( m_HasCount ) {
		for ( int i = 0; i < m_SampleCounts.Num(); i++ ) {
			delete *m_SampleCounts.GetIndex( i );
		}
		m_SampleCounts.Clear();
	}

	m_SampleUpto = 0;
	m_LogSubFileUpto = 0;
	m_LogFileUpto++;

	m_ProfileNode.AddToFront( sdProfileHelperManager::GetInstance().m_ActiveProfilers );
}

/*
================
sdProfileHelper::Stop
================
*/
void sdProfileHelper::Stop( void ) {
	if ( !IsActive() ) {
		return;
	}

	DumpLog( NULL, m_SampleValues );
	AssembleLogs( NULL );

	if ( m_HasCount ) {
		DumpLog( "count", m_SampleCounts );
		AssembleLogs( "count" );
	}

	m_LogSubFileUpto++;
	m_SampleUpto = 1;

	m_ProfileNode.AddToFront( sdProfileHelperManager::GetInstance().m_InactiveProfilers );
}

/*
================
sdProfileHelper::IsActive
================
*/
bool sdProfileHelper::IsActive( void ) const {
	if ( m_ProfileNode.ListHead() == &sdProfileHelperManager::GetInstance().m_ActiveProfilers ) {
		return true;
	}

	return false;
}

/*
================
sdProfileHelper::Update
================
*/
void sdProfileHelper::Update( void ) {
	if ( !IsActive() ) {
		return;
	}

	m_SampleUpto++;
	if ( m_SampleUpto == PROFILE_HELPER_MAX_FRAMES ) {
		DumpLog( NULL, m_SampleValues );
		if ( m_HasCount ) {
			DumpLog( "count", m_SampleCounts );
		}

		m_LogSubFileUpto++;
		m_SampleUpto = 1;
	}

	m_SampleTimes[ m_SampleUpto ] = gameLocal.time;
}

/*
================
sdProfileHelper::LogValue
================
*/
void sdProfileHelper::LogValue( const char* sampleName, double value ) {
	if ( !IsActive() ) {
		return;
	}

	sampleSet_t**	infoPtr;
	sampleSet_t*	info;
	if ( !m_SampleValues.Get( sampleName, &infoPtr ) ) {
		info = new sampleSet_t;
		for ( int i = 0; i < PROFILE_HELPER_MAX_FRAMES; i++ ) {
			info->frameInfo[ i ] = 0.0;
		}

		m_SampleValues.Set( sampleName, info );
		m_SampleValues.Get( sampleName, &infoPtr );
	}

	info = *infoPtr;
	info->frameInfo[ m_SampleUpto ] += value;

	if ( m_HasCount ) {
		if ( !m_SampleCounts.Get( sampleName, &infoPtr ) ) {
			info = new sampleSet_t;
			for ( int i = 0; i < PROFILE_HELPER_MAX_FRAMES; i++ ) {
				info->frameInfo[ i ] = 0.0;
			}

			m_SampleCounts.Set( sampleName, info );
			m_SampleCounts.Get( sampleName, &infoPtr );
		}

		info = *infoPtr;
		info->frameInfo[ m_SampleUpto ] += 1.0;	
	}
}

/*
================
sdProfileHelper::GetMiniLogName
================
*/
void sdProfileHelper::GetMiniLogName( const char* suffix, idStr& out, int logNum, int subLogNum ) {
	const char* extraNameInfo = gameLocal.isClient ? "client" : "server";

	if ( suffix == NULL ) {
		out = va( "profiling/temp/%s_%s_mini_%i_%i.temp", m_ProfileName.c_str(), extraNameInfo, logNum, subLogNum );
	} else {
		out = va( "profiling/temp/%s_%s_%s_mini_%i_%i.temp", m_ProfileName.c_str(), suffix, extraNameInfo, logNum, subLogNum );
	}
}

/*
================
sdProfileHelper::GetLogName
================
*/
void sdProfileHelper::GetLogName( const char* suffix, idStr& out, int logNum ) {
	const char* extraNameInfo = gameLocal.isClient ? "client" : "server";

	if ( suffix == NULL ) {
		out = va( "profiling/%s_%s_%i.csv", m_ProfileName.c_str(), extraNameInfo, logNum );
	} else {
		out = va( "profiling/%s_%s_%s_%i.csv", m_ProfileName.c_str(), suffix, extraNameInfo, logNum );
	}
}

/*
================
sdProfileHelper::DumpLog
================
*/
void sdProfileHelper::DumpLog( const char* suffix, sampleGroup_t& group ) {
	idStr fileName;
	GetMiniLogName( suffix, fileName, m_LogFileUpto, m_LogSubFileUpto );

	idFile* miniLog = fileSystem->OpenFileWrite( fileName.c_str() );
	if ( miniLog == NULL ) {
		gameLocal.Warning( "sdProfileHelper::DumpLog - failed to open %s", fileName.c_str() );
		return;
	}

	// output number of columns
	if ( m_HasTotal ) {
		miniLog->WriteInt( group.Num() + 2 );
	} else {
		miniLog->WriteInt( group.Num() + 1 );
	}

	// print line of titles
	miniLog->WriteString( "gameLocal.time" );
	for ( int i = 0; i < group.Num(); i++ ) {
		miniLog->WriteString( va( "%s", group.GetKey( i ).c_str() ) );
	}
	if ( m_HasTotal ) {
		miniLog->WriteString( "TOTAL" );
	}
	
	// print data, row by row
	for ( int sample = 1; sample < m_SampleUpto; sample++ ) {
		miniLog->WriteInt( m_SampleTimes[ sample ] );

		double total = 0.0;

		for ( int i = 0; i < group.Num(); i++ ) {
			sampleSet_t* info = *group.GetIndex( i );
			miniLog->WriteFloat( ( float )info->frameInfo[ sample ] );

			total += info->frameInfo[ sample ];

			info->frameInfo[ sample ] = 0.0;
		}
		
		if ( m_HasTotal ) {
			miniLog->WriteFloat( ( float )total );
		}
	}

	fileSystem->CloseFile( miniLog );
}

/*
================
sdProfileHelper::AssembleLogs
================
*/
void sdProfileHelper::AssembleLogs( const char* suffix ) {
	// assemble all the logs for this session into one big csv file & delete the mini-logs
	// first find all the logs & collate a list of titles
	idList< idStr > titles;
	int numLogs = 0;

	//
	// Initialize the titles list
	//
	while( 1 ) {
		idStr logName;
		GetMiniLogName( suffix, logName, m_LogFileUpto, numLogs );

		idFile* miniLog = fileSystem->OpenFileRead( logName.c_str() );
		if ( miniLog == NULL ) {
			break;
		}

		int numColumns;
		miniLog->ReadInt( numColumns );
		for ( int i = 0; i < numColumns; i++ ) {
			idStr title;
			miniLog->ReadString( title );
			titles.AddUnique( title );
		}
		numLogs++;
		fileSystem->CloseFile( miniLog );
	}

	if ( numLogs == 0 ) {
		gameLocal.Warning( "sdProfileHelper::AssembleLogs - no mini logs!" );
		return;
	}

	//
	// Sort the titles list, keeping gameLocal.time & TOTAL at the start
	//
	titles.RemoveFast( idStr( "gameLocal.time" ) );
	bool hadTotal = titles.RemoveFast( idStr( "TOTAL" ) );
	titles.Sort();
	if ( hadTotal ) {
		titles.Insert( idStr( "TOTAL" ), 0 );
	}
	titles.Insert( idStr( "gameLocal.time" ), 0 );

	//
	// Write the titles list
	//
	idStr fullLogName;
	GetLogName( suffix, fullLogName, m_LogFileUpto );

	idFile* log = fileSystem->OpenFileWrite( fullLogName.c_str() );
	if ( log == NULL ) {
		gameLocal.Warning( "sdProfileHelper::AssembleLogs - couldn't open file %s", fullLogName.c_str() );
		return;
	}

	for ( int i = 0; i < titles.Num(); i++ ) {
		log->Printf( "\"%s\",", titles[ i ].c_str() );
	}
	log->Printf( "\n" );

	//
	// Loop through the logs & write them all into the main log
	//
	idList< int > miniLogColumnToLogColumn;
	idList< int > logColumnToMiniLogColumn;
	idList< float > columnTimeValues;
	logColumnToMiniLogColumn.AssureSize( titles.Num() );

	for ( int logNum = 0; logNum < numLogs; logNum++ ) {
		idStr logName;
		GetMiniLogName( suffix, logName, m_LogFileUpto, logNum );

		idFile* miniLog = fileSystem->OpenFileRead( logName.c_str() );
		if ( miniLog == NULL ) {
			gameLocal.Warning( "sdProfileHelper::AssembleLogs - couldn't open file %s", logName.c_str() );
			break;
		}

		// create translation tables to & from the mini log & main log table
		int numColumns;
		miniLog->ReadInt( numColumns );
		miniLogColumnToLogColumn.AssureSize( numColumns );
		for ( int i = 0; i < titles.Num(); i++ ) {
			logColumnToMiniLogColumn[ i ] = -1;
		}
		for ( int i = 0; i < numColumns; i++ ) {
			idStr title;
			miniLog->ReadString( title );
			miniLogColumnToLogColumn[ i ] = titles.FindIndex( title );
			logColumnToMiniLogColumn[ miniLogColumnToLogColumn[ i ] ] = i;
		}

		// go through all the samples and write them into the main file
		columnTimeValues.AssureSize( numColumns );
		while ( miniLog->Tell() < miniLog->Length() - 1 ) {
			int time;
			miniLog->ReadInt( time );

			// read the samples
			for ( int i = 1; i < numColumns; i++ ) {
				miniLog->ReadFloat( columnTimeValues[ i ] );
			}

			// write the time samples
			log->Printf( "%i,", time );
			for ( int i = 1; i < titles.Num(); i++ ) {
				if ( logColumnToMiniLogColumn[ i ] == -1 ) {
					log->Printf( "0," );
				} else {
					log->Printf( "%.6f,", columnTimeValues[ logColumnToMiniLogColumn[ i ] ] );
				}
			}

			log->Printf( "\n" );
		}

		fileSystem->CloseFile( miniLog );
		fileSystem->RemoveFile( logName.c_str() );
	}

	fileSystem->CloseFile( log );
}

/*
===============================================================================

	sdProfileHelperManagerLocal

===============================================================================
*/

/*
================
sdProfileHelperManagerLocal::sdProfileHelperManagerLocal
================
*/
sdProfileHelperManagerLocal::sdProfileHelperManagerLocal( void ) {
	m_ActiveProfilers.Clear();
	m_InactiveProfilers.Clear();
}

/*
================
sdProfileHelperManagerLocal::~sdProfileHelperManagerLocal
================
*/
sdProfileHelperManagerLocal::~sdProfileHelperManagerLocal( void ) {
	StopAll();

	m_ActiveProfilers.Clear();
	m_InactiveProfilers.Clear();
}

/*
================
sdProfileHelperManagerLocal::Update
================
*/
void sdProfileHelperManagerLocal::Update( void ) {
	for ( sdProfileHelper* profiler = m_ActiveProfilers.Next(); profiler != NULL; profiler = profiler->m_ProfileNode.Next() ) {
		profiler->Update();
	}
}

/*
================
sdProfileHelperManagerLocal::StopAll
================
*/
void sdProfileHelperManagerLocal::StopAll( void ) {
	for ( sdProfileHelper* profiler = m_ActiveProfilers.Next(); profiler != NULL; ) {
		sdProfileHelper* nextProfiler = profiler->m_ProfileNode.Next();
		profiler->Stop();
		profiler = nextProfiler;
	}
}

/*
================
sdProfileHelperManagerLocal::FindProfiler
================
*/
sdProfileHelper* sdProfileHelperManagerLocal::FindProfiler( const char* name ) {
	for ( sdProfileHelper* profiler = m_ActiveProfilers.Next(); profiler != NULL; profiler = profiler->m_ProfileNode.Next() ) {
		if ( !idStr::Icmp( profiler->GetName(), name ) ) {
			return profiler;
		}
	}

	for ( sdProfileHelper* profiler = m_InactiveProfilers.Next(); profiler != NULL; profiler = profiler->m_ProfileNode.Next() ) {
		if ( !idStr::Icmp( profiler->GetName(), name ) ) {
			return profiler;
		}
	}

	return NULL;
}

/*
================
sdProfileHelperManagerLocal::LogValue
================
*/
void sdProfileHelperManagerLocal::LogValue( const char* profileName, const char* sampleName, double value, bool create, bool hasTotal, bool hasCount ) {
	sdProfileHelper* profiler = FindProfiler( profileName );

	if ( profiler == NULL && create ) {
		profiler = new sdProfileHelper();
		profiler->Init( profileName, hasTotal, hasCount );
		profiler->Start();
	}

	if ( profiler == NULL ) {
		return;
	}

	if ( !profiler->IsActive() ) {
		profiler->Start();
	}

	profiler->LogValue( sampleName, value );
}
