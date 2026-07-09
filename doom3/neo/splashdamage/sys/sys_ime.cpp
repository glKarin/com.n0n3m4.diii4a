// Copyright (C) 2007 Id Software, Inc.
//

#include "idlib/precompiled.h"

#include "sys_ime.h"

sdIMEGeneric::sdIMEGeneric()
    : enable(false)
{
    
}

bool sdIMEGeneric::Init() {
    return true;
}

void sdIMEGeneric::Shutdown() {
}


bool sdIMEGeneric::LangSupportsIME() const {
    return false;
}


void sdIMEGeneric::Enable( bool enable ) {
    this->enable = enable;
}

bool sdIMEGeneric::IsEnabled() const {
    return enable;
}

void sdIMEGeneric::FinalizeString( bool send ) {
}

int sdIMEGeneric::GetCursorChars() const {
    return '_';
}

bool sdIMEGeneric::IsReadingWindowActive() const {
    return false;
}

bool sdIMEGeneric::IsHorizontalReading() const {
    return false;
}

bool sdIMEGeneric::VerticalCandidateLine() const {
    return false;
}

sdIME::state_e sdIMEGeneric::GetState() const {
    return IME_STATE_OFF;
}

const wchar_t* sdIMEGeneric::GetIndicator() const {
    return L"_";
}

bool sdIMEGeneric::IsCandidateListActive() const {
    return false;
}

const wchar_t* sdIMEGeneric::GetCandidate( const unsigned int index ) const {
    return L"";
}

int sdIMEGeneric::GetCandidateCount() const {
    return 0;
}

int sdIMEGeneric::GetCandidateSelection() const {
    return 0;
}

const wchar_t* sdIMEGeneric::GetCompositionString() const {
    return L"";
}

const byte* sdIMEGeneric::GetCompositionStringAttributes() const {
    return NULL;
}

const sdIME::lang_e sdIMEGeneric::GetLanguage() const {
    return IME_LANG_NEUTRAL;
}

const sdIME::lang_e sdIMEGeneric::GetPrimaryLanguage() const {
    return IME_LANG_NEUTRAL;
}

static sdIMEGeneric IMEGeneric;
sdIME *globalIME = &IMEGeneric;
