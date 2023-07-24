// Copyright (C) 2007 Id Software, Inc.
//

#if !defined( __SDNETACCOUNT_AUTH_H__ )
#define __SDNETACCOUNT_AUTH_H__

//===============================================================
//
//	sdNetAccount
//
//		username/password authentication based version
//
//===============================================================

class sdNetTask;

class sdNetAccount {
public:
	virtual					~sdNetAccount() {}

	virtual void			SetUsername( const char* username ) = 0;
	virtual const char*		GetUsername() const = 0;

	virtual void			SetPassword( const char* password ) = 0;
	virtual const char*		GetPassword() const = 0;

	virtual void			GetNetClientId( sdNetClientId& netClientId ) const = 0;

	//
	// Online functionality
	//

	// Create an account to sign to online service
	virtual sdNetTask*		CreateAccount( const char* username, const char* password, const char* key ) = 0;

	// Change password
	virtual sdNetTask*		ChangePassword( const char* password, const char* newPassword ) = 0;

	// Reset password using license code
	virtual sdNetTask*		ResetPassword( const char* key, const char* newPassword ) = 0;

	// Delete an account
	virtual sdNetTask*		DeleteAccount() = 0;

	// Sign in to online service
	virtual sdNetTask*		SignIn() = 0;

	// Sign out from online service
	virtual sdNetTask*		SignOut() = 0;
};

#endif /* !__SDNETACCOUNT_AUTH_H__ */
