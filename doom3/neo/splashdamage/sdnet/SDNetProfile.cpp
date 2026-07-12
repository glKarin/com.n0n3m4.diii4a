// Copyright (C) 2007 Id Software, Inc.
//

#include "idlib/precompiled.h"

#include "SDNetProfile_local.h"
#include "SDNetTask_local.h"

//===============================================================
//
//	sdNetProfile
//
//===============================================================

sdNetProfile_Local::sdNetProfile_Local() {
}

sdNetProfile_Local::~sdNetProfile_Local() {

}

idDict&	sdNetProfile_Local::GetProperties() {
	return properties;
}

const idDict& sdNetProfile_Local::GetProperties() const {
	return properties;
}


	//
	// Online functionality
	//

#if !defined( SD_DEMO_BUILD )
	// Assures the profile exists, creates it if it isn't there
sdNetTask* sdNetProfile_Local::AssureExists() {
	return new sdNetTask_AssureExists;
}


	// Stores the profile properties remotely
sdNetTask* sdNetProfile_Local::Store( bool publicProfile, bool privateProfile ) const {
	return new sdNetTask_Store(publicProfile, privateProfile);
}


	// Restores the remote profile properties, merging with local properties as required
sdNetTask* sdNetProfile_Local::Restore() {
	return new sdNetTask_Restore;
}

#endif /* !SD_DEMO_BUILD */

void sdNetProfile_Local::SetUsername(const char *name)
{
	properties.Set("username", name);
}

void sdNetProfile_Local::Clear(void)
{
	properties.Clear();
}

