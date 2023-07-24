// Copyright (C) 2007 Id Software, Inc.
//

#include "../../precompiled.h"
#pragma hdrstop

#include "AAS.h"
#include "AASCallback_FindFlaggedArea.h"

/*
============
idAASCallback_FindFlaggedArea::idAASCallback_FindFlaggedArea
============
*/
idAASCallback_FindFlaggedArea::idAASCallback_FindFlaggedArea( const int areaFlag, bool set ) {
	this->areaFlag = areaFlag;
	this->test = ( set != false );
}

/*
============
idAASCallback_FindFlaggedArea::~idAASCallback_FindFlaggedArea
============
*/
idAASCallback_FindFlaggedArea::~idAASCallback_FindFlaggedArea() {
}

/*
============
idAASCallback_FindFlaggedArea::AreaIsGoal
============
*/
bool idAASCallback_FindFlaggedArea::AreaIsGoal( const idAAS *aas, int areaNum ) {
	return ( aas->GetAreaFlags( areaNum ) & areaFlag ) == test;
}
