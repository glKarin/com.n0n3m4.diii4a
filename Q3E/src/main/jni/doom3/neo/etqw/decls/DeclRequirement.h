// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __DECLREQUIREMENT_H__
#define __DECLREQUIREMENT_H__

class sdRequirementCheck {
public:
	virtual										~sdRequirementCheck( void ) { ; }

	virtual void								Init( const idDict& parms ) = 0;
	virtual bool								Check( idEntity* main, idEntity* other ) const = 0;

	static sdRequirementCheck*					AllocChecker( const char* typeName );

	static void									InitFactory( void );
	static void									ShutdownFactory( void );

private:
	static sdFactory< sdRequirementCheck >		checkerFactory;
	static bool									factoryInited;
};

class sdDeclRequirement : public idDecl {
public:
												sdDeclRequirement( void );
	virtual										~sdDeclRequirement( void );

	virtual const char*							DefaultDefinition( void ) const;
	virtual bool								Parse( const char *text, const int textLength );
	virtual void								FreeData( void );

	bool										Check( idEntity* main, idEntity* other ) const { return checker ? checker->Check( main, other ) : false; }

private:
	sdRequirementCheck*							checker;
};

#endif // __DECLREQUIREMENT_H__

