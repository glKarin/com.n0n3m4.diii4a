// Copyright (C) 2007 Id Software, Inc.
//

#if !defined( __SDNETACCOUNT_LOCAL_H__ )
#define __SDNETACCOUNT_LOCAL_H__

//===============================================================
//
//	sdNetAccount
//
//		username/password authentication based version
//
//===============================================================

#include "SDNet.h"

#include "SDNetAccount_Auth.h"

class sdNetAccount_Local : public sdNetAccount {
public:
	sdNetAccount_Local();
	virtual					~sdNetAccount_Local();

	virtual void			SetUsername( const char* username );
	virtual const char*		GetUsername() const;

	virtual void			SetPassword( const char* password );
	virtual const char*		GetPassword() const;

	virtual void			GetNetClientId( sdNetClientId& netClientId ) const;

	//
	// Online functionality
	//

	// Create an account to sign to online service
	virtual sdNetTask*		CreateAccount( const char* username, const char* password, const char* key );

	// Change password
	virtual sdNetTask*		ChangePassword( const char* password, const char* newPassword );

	// Reset password using license code
	virtual sdNetTask*		ResetPassword( const char* key, const char* newPassword );

	// Delete an account
	virtual sdNetTask*		DeleteAccount();

	// Sign in to online service
	virtual sdNetTask*		SignIn();

	// Sign out from online service
	virtual sdNetTask*		SignOut();

private:
	idStr username;
	idStr password;
	sdNetClientId clientId;
};

#endif /* !__SDNETACCOUNT_LOCAL_H__ */
