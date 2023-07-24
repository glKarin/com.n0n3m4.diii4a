// Copyright (C) 2007 Id Software, Inc.
//

#if !defined( __SDNETUSER_H__ )
#define __SDNETUSER_H__

//===============================================================
//
//	sdNetUser
//
//===============================================================

class sdNetTask;
class sdNetAccount;
class sdNetProfile;

class sdNetUser {
public:
	enum userState_e {
		US_INACTIVE,
		US_ACTIVE,
		US_ONLINE
	};

	enum saveItems_e {
		SI_PROFILE	= BITT< 0 >::VALUE,
		SI_CVARS	= BITT< 1 >::VALUE,
		SI_BINDINGS	= BITT< 2 >::VALUE,
	};

	static const int			MAX_SERVER_HISTORY = 10;

	virtual						~sdNetUser() {}

	virtual userState_e			GetState() const = 0;
	virtual const char*			GetUsername() const = 0;
	virtual const char*			GetRawUsername() const = 0;

	virtual sdNetProfile&		GetProfile() = 0;
	virtual const sdNetProfile&	GetProfile() const = 0;

	// Make this user the currently active one
	virtual void				Activate() = 0;

	// Deactivate the user, effectively logging them out
	virtual void				Deactivate() = 0;

	// Write user to permanent storage
	virtual bool				Save( int saveItems = ( SI_PROFILE | SI_CVARS | SI_BINDINGS ) ) const = 0;

	// Get online account
	virtual sdNetAccount&		GetAccount() = 0;

	// Utility functions
	static void					MakeRawUsername( const char* username, idStr& rawUsername );
	static void					MakeCleanUsername( const char* username, idStr& cleanUsername );
	static void					MakeProfileConfig( const char* username, idStr& profileConfig );
	static void					MakeAutoExecConfig( const char* username, idStr& profileConfig );
	static void					MakeProfileBindings( const char* username, idStr& profileBindings );
};

/*
================
sdNetUser::MakeRawUsername
================
*/
ID_INLINE void sdNetUser::MakeRawUsername( const char* username, idStr& rawUsername ) {
	rawUsername = username;
	rawUsername.RemoveColors();
	rawUsername.ToLower();
	rawUsername.ReplaceChar( '.', '#' );
	rawUsername.CleanFilename();
}

/*
================
sdNetUser::MakeCleanUsername
================
*/
ID_INLINE void sdNetUser::MakeCleanUsername( const char* username, idStr& rawUsername ) {
	rawUsername = username;
	// TODO : figure out what checks we want here
}

/*
============
sdNetUser::MakeProfileConfig
============
*/
ID_INLINE void sdNetUser::MakeProfileConfig( const char* username, idStr& profileConfig ) {
	profileConfig = fileSystem->BuildOSPath( fileSystem->GetUserPath(), "sdnet", username );
	profileConfig += PATHSEPARATOR_CHAR;
	profileConfig += fileSystem->GetGamePath();
	profileConfig += PATHSEPARATOR_CHAR;
	profileConfig += "profile.cfg";
}

/*
============
sdNetUser::MakeAutoExecConfig
============
*/
ID_INLINE void sdNetUser::MakeAutoExecConfig( const char* username, idStr& profileConfig ) {
	profileConfig = fileSystem->BuildOSPath( fileSystem->GetUserPath(), "sdnet", username );
	profileConfig += PATHSEPARATOR_CHAR;
	profileConfig += fileSystem->GetGamePath();
	profileConfig += PATHSEPARATOR_CHAR;
	profileConfig += "autoexec.cfg";
}

/*
============
sdNetUser::MakeProfileBindings
============
*/
ID_INLINE void sdNetUser::MakeProfileBindings( const char* username, idStr& profileBindings ) {
	profileBindings = fileSystem->BuildOSPath( fileSystem->GetUserPath(), "sdnet", username );
	profileBindings += PATHSEPARATOR_CHAR;
	profileBindings += fileSystem->GetGamePath();
	profileBindings += PATHSEPARATOR_CHAR;
	profileBindings += "bindings.cfg";
}

#endif /* !__SDNETUSER_H__ */
