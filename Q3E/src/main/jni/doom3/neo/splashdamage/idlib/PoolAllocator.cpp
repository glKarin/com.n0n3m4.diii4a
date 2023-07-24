// Copyright (C) 2007 Id Software, Inc.
//


#include "precompiled.h"
#pragma hdrstop

const char sdPoolAllocator_DefaultIdentifier[] = "sdPoolAllocator";

/*
============
sdDynamicBlockManagerBase::MemoryReport
============
*/
void sdDynamicBlockManagerBase::MemoryReport( const idCmdArgs& args ) {
	int baseBytes = 0;
	int freeBytes = 0;

	bool print = args.Argc() > 1 && !idStr::Icmp( args.Argv( 1 ), "all" );
	for( int i = 0; i < GetList().Num(); i++ ) {
		if( print )	{
			GetList()[ i ]->PrintInfo();
		}
		GetList()[ i ]->Accumulate( baseBytes, freeBytes );
	}

	common->Printf( "\n%i allocators with a total of %i bytes (%i bytes free)\n", GetList().Num(), baseBytes, freeBytes );
}

/*
============
sdDynamicBlockManagerBase::InitPools
============
*/
void sdDynamicBlockManagerBase::InitPools() {
}

/*
============
sdDynamicBlockManagerBase::ShutdownPools
============
*/
void sdDynamicBlockManagerBase::ShutdownPools() {
	for( int i = 0; i < GetList().Num(); i++ ) {
		GetList()[ i ]->Shutdown();
	}
	GetList().Clear();
}

/*
============
sdDynamicBlockManagerBase::CompactPools
============
*/
void sdDynamicBlockManagerBase::CompactPools() {	
//	idLib::Printf( "Compacting %i memory pools...\n", GetList().Num() );

	int totalBytesFreed = 0;
	int bytesFreed;
	for( int i = 0; i < GetList().Num(); i++ ) {
		bytesFreed = GetList()[ i ]->Compact();
//		if( bytesFreed > 0 ) {
//			idLib::Printf( "%s: freed %i bytes\n", GetList()[ i ]->GetName(), bytesFreed );		
//		}
		totalBytesFreed += bytesFreed;
	}
//	idLib::Printf( "Compacted %i bytes\n", totalBytesFreed );
}
