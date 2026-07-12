// Copyright (C) 2007 Id Software, Inc.
//

#if !defined( __SDNETPROFILE_LOCAL_H__ )
#define __SDNETPROFILE_LOCAL_H__

//===============================================================
//
//	sdNetProfile
//
//===============================================================

#include "SDNetProfile.h"

class sdNetProfile_Local : public sdNetProfile {
public:
	sdNetProfile_Local();
	virtual					~sdNetProfile_Local();

	virtual idDict&			GetProperties();
	virtual const idDict&	GetProperties() const;
	void					SetUsername(const char *name);
	void					Clear(void);

	//
	// Online functionality
	//

#if !defined( SD_DEMO_BUILD )
	// Assures the profile exists, creates it if it isn't there
	virtual sdNetTask*		AssureExists();

	// Stores the profile properties remotely
	virtual sdNetTask*		Store( bool publicProfile = true, bool privateProfile = true ) const;

	// Restores the remote profile properties, merging with local properties as required
	virtual sdNetTask*		Restore();
#endif /* !SD_DEMO_BUILD */

private:
	idDict properties;
};

#endif /* !__SDNETPROFILE_LOCAL_H__ */
