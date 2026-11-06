// Copyright (C) 2007 Id Software, Inc.
//

#include "idlib/precompiled.h"

#include "sys_keyboard.h"

#include "framework/KeyInput.h"

idKeyboardGeneric::idKeyboardGeneric() {
}

idKeyboardGeneric::~idKeyboardGeneric() {
}

bool idKeyboardGeneric::Init() {
  return true;
}

void idKeyboardGeneric::Shutdown() {
}

void idKeyboardGeneric::Activate() {
}

void idKeyboardGeneric::Deactivate() {
}

int idKeyboardGeneric::PollInputEvents( bool postEvents ) {
  return 0;
}

int idKeyboardGeneric::ReturnInputEvent( const int n, keyNum_t& key, bool& isDown ) {
  return 0;
}

void idKeyboardGeneric::EndInputEvents() {
}

keyNum_t idKeyboardGeneric::ConvertScanToKey( unsigned int scanCode ) const {
  return K_INVALID;
}

keyNum_t idKeyboardGeneric::ConvertCharToKey( char ch ) const {
  return K_INVALID;
}

char idKeyboardGeneric::ConvertScanToChar( unsigned int scanCode ) const {
  return ' ';
}

unsigned int idKeyboardGeneric::ConvertCharToScan( char ch ) const {
  return 0;
}

char idKeyboardGeneric::ConvertKeyToChar( const keyNum_t keyNum ) const {
  return ' ';
}

bool idKeyboardGeneric::IsConsoleKey( const sdSysEvent& event ) const {
  return event.evValue == Sys_GetConsoleKey(false) || event.evValue == Sys_GetConsoleKey(true);
}

void idKeyboardGeneric::AllocateKeys() {
}

unsigned int idKeyboardGeneric::StringToScanCode( const char* str ) {
  return 0;
}

keyNum_t idKeyboardGeneric::StringToKeyNum( const char* str ) {
  return K_INVALID;
}

void idKeyboardGeneric::KeyNumToString( const keyNum_t keyNum, idWStr& fixedText, idStr& locName ) {
}

idKey& idKeyboardGeneric::GetStandardKey( const keyNum_t key ) {
  static idKey k(0, "", "", L"");
  return k;
}

static idKeyboardGeneric keyboardGeneric;
idKeyboard *globalKeyboard = &keyboardGeneric;
