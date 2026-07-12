// Copyright (C) 2007 Id Software, Inc.
//
#include "idlib/precompiled.h"

#include "SDNetAccount_local.h"

#include "SDNetTask_local.h"

sdNetAccount_Local::sdNetAccount_Local() {
	clientId.Invalidate();
}

sdNetAccount_Local::~sdNetAccount_Local() {
}

void sdNetAccount_Local::SetUsername( const char* username ) {
	this->username = username;
}

const char* sdNetAccount_Local::GetUsername() const {
	return username.c_str();
}

void sdNetAccount_Local::SetPassword( const char* password ) {
	this->password = password;
}

const char* sdNetAccount_Local::GetPassword() const {
	return password.c_str();
}

void sdNetAccount_Local::GetNetClientId( sdNetClientId& netClientId ) const {
	netClientId = clientId;
}

	//
	// Online functionality
	//

	// Create an account to sign to online service
sdNetTask* sdNetAccount_Local::CreateAccount( const char* username, const char* password, const char* key ) {
	return new sdNetTask_CreateAccount(username, password, key);
}

	// Change password
sdNetTask* sdNetAccount_Local::ChangePassword( const char* password, const char* newPassword ) {
	return new sdNetTask_ChangePassword(password, newPassword);
}

	// Reset password using license code
sdNetTask* sdNetAccount_Local::ResetPassword( const char* key, const char* newPassword ) {
	return new sdNetTask_ResetPassword(key, newPassword);
}

	// Delete an account
sdNetTask* sdNetAccount_Local::DeleteAccount() {
	return new sdNetTask_DeleteAccount;
}

	// Sign in to online service
sdNetTask* sdNetAccount_Local::SignIn() {
	return new sdNetTask_SignIn;
}

	// Sign out from online service
sdNetTask* sdNetAccount_Local::SignOut() {
	return new sdNetTask_SignOut;
}

