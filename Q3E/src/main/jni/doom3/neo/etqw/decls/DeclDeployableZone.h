// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __DECLDEPLOYABLEZONE_H__
#define __DECLDEPLOYABLEZONE_H__

class sdTeamInfo;
class sdDeclDeployableObject;

class sdDeclDeployableZone : public idDecl {
public:
									sdDeclDeployableZone( void );
	virtual							~sdDeclDeployableZone( void );

	virtual const char*				DefaultDefinition( void ) const;
	virtual bool					Parse( const char *text, const int textLength );
	virtual void					FreeData( void );

	bool							ParseTeamInfo( sdTeamInfo* team, idParser& src );
	int								NumOptions( const sdTeamInfo* team ) const;
	const sdDeclDeployableObject*	GetDeployOption( const sdTeamInfo* team, int index ) const;

	static void						CacheFromDict( const idDict& dict );

private:
	typedef idList< const sdDeclDeployableObject* > teamInfo_t;

	idList< teamInfo_t >		teamInfo;
};

#endif // __DECLDEPLOYABLEZONE_H__

