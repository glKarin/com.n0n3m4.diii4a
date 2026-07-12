// Copyright (C) 2007 Id Software, Inc.
//

#if !defined( __SDNETUSER_LOCAL_H__ )
#define __SDNETUSER_LOCAL_H__

//===============================================================
//
//	sdNetUser
//
//===============================================================

#include "SDNetProfile_local.h"
#include "SDNetAccount_local.h"

#include "SDNetUser.h"

class sdNetUser_Local : public sdNetUser {
public:
	sdNetUser_Local();
	virtual						~sdNetUser_Local();

	virtual userState_e			GetState() const;
	virtual const char*			GetUsername() const;
	virtual const char*			GetRawUsername() const;

	virtual sdNetProfile&		GetProfile();
	virtual const sdNetProfile&	GetProfile() const;

	// Make this user the currently active one
	virtual void				Activate();

	// Deactivate the user, effectively logging them out
	virtual void				Deactivate();

	// Write user to permanent storage
	virtual bool				Save( int saveItems = ( SI_PROFILE | SI_CVARS | SI_BINDINGS ) ) const;

	// Get online account
	virtual sdNetAccount&		GetAccount();

	void						SetRawUsername(const char *name);
	void						Init(void);
	void						Create(void);
	void						Remove(void);
	void						SaveModified(void);

private:
	bool						SaveOffline( int saveItem ) const;
	bool						SaveProfileOffline(idFile *file) const;
	bool						SaveBindingsOffline(idFile *file) const;
	bool						SaveCVarsOffline(idFile *file) const;

	bool						Restore( int saveItems );
	bool						RestoreOffline(int item);
	bool						RestoreBindingsOffline(const char *data);
	bool						RestoreProfileOffline(const char *data);
	bool						RestoreCVarsOffline(const char *data);

	void						ProfilePath(idStr &out, const char *type) const;

private:
	userState_e userState;
	idStr rawUsername;
	sdNetProfile_Local profile;
	sdNetAccount_Local account;
	bool noPrint;
	friend class sdNetService_Local;
};

#endif /* !__SDNETUSER_LOCAL_H__ */
