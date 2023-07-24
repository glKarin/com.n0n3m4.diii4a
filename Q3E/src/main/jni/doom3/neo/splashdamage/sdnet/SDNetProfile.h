// Copyright (C) 2007 Id Software, Inc.
//

#if !defined( __SDNETPROFILE_H__ )
#define __SDNETPROFILE_H__

//===============================================================
//
//	sdNetProfile
//
//===============================================================

class sdNetTask;

class sdNetProfile {
public:
	virtual					~sdNetProfile() {}

	virtual idDict&			GetProperties() = 0;
	virtual const idDict&	GetProperties() const = 0;

	//
	// Online functionality
	//

#if !defined( SD_DEMO_BUILD )
	// Assures the profile exists, creates it if it isn't there
	virtual sdNetTask*		AssureExists() = 0;

	// Stores the profile properties remotely
	virtual sdNetTask*		Store( bool publicProfile = true, bool privateProfile = true ) const = 0;

	// Restores the remote profile properties, merging with local properties as required
	virtual sdNetTask*		Restore() = 0;
#endif /* !SD_DEMO_BUILD */
};

#endif /* !__SDNETPROFILE_H__ */
