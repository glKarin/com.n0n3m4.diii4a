
#ifndef __AASFILEMANAGER_H__
#define __AASFILEMANAGER_H__

/*
===============================================================================

	AAS File Manager

===============================================================================
*/

class idAASFileManager {
public:
	virtual						~idAASFileManager( void ) {}

	virtual idAASFile *			LoadAAS( const char *fileName, unsigned int mapFileCRC ) = 0;
	virtual void				FreeAAS( idAASFile *file ) = 0;
};

extern idAASFileManager *		AASFileManager;

#endif /* !__AASFILEMANAGER_H__ */
