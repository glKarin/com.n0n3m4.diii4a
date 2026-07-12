// Copyright (C) 2007 Id Software, Inc.
//

#include "idlib/precompiled.h"

#include "SDNetMessageHistory.h"

//===============================================================
//
//	sdNetMessageHistory
//
//===============================================================

const int sdNetMessageHistory::MAX_ENTRIES = 30;
const wchar_t* const sdNetMessageHistory::MESSAGE_STORE_VERSION = L"";
const wchar_t* const sdNetMessageHistory::MESSAGE_STORE_HEADER = L"";

sdNetMessageHistory::sdNetMessageHistory()
	: loaded(false)
{

}

void sdNetMessageHistory::AddEntry( const wchar_t* msg ) {
	messageHistoryEntry_t item;
	item.message = msg;
	item.timeStamp = sys->Milliseconds();
	entries.Append(item);
}

bool sdNetMessageHistory::Load( const char* fileName ) {
	return false;
}

bool sdNetMessageHistory::Store() {
	return false;
}

void sdNetMessageHistory::Unload() {
	loaded = false;
}
