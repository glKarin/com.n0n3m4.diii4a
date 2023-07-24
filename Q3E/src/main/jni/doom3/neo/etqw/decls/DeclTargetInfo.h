// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __DECLTARGETINFO_H__
#define __DECLTARGETINFO_H__

#include "../roles/RoleManager.h"

class idEntity;
class sdEntityCollection;

class sdDeclTargetInfo : public idDecl {
public:
							sdDeclTargetInfo( void );
	virtual					~sdDeclTargetInfo( void );

	bool					FilterEntity( idEntity* entity ) const;
	bool					ParseType( idParser& src, bool include );

	static void				CacheFromDict( const idDict& dict );

	virtual const char*		DefaultDefinition( void ) const;
	virtual bool			Parse( const char *text, const int textLength );
	virtual void			FreeData( void );

protected:
	typedef enum targetInfoType_e {
		TI_CLASS,
		TI_REFERENCE,
		TI_COLLECTION,
	} targetInfoType_t;

	typedef struct targetInfo_s {
		targetInfoType_t			type;
		int							index;
		bool						include;
		const sdEntityCollection*	collection;
	} targetInfo_t;

	idList< targetInfo_t >		filters;
	sdRequirementContainer		requirements;
};

#endif // __DECLTARGETINFO_H__

