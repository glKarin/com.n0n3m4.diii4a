//----------------------------------------------------------------
// DeclPlayerModel.h
//
// Copyright 2002-2006 Raven Software
//----------------------------------------------------------------

#ifndef __DECLPLAYERMODEL_H__
#define __DECLPLAYERMODEL_H__

/*
===============================================================================

rvDeclPlayerModel

===============================================================================
*/

class rvDeclPlayerModel : public idDecl {
public:
	rvDeclPlayerModel();

	idStr					model;
	idStr					head;
	idVec3					headOffset;
	idStr					uiHead;
	idStr					team;
	idStr					skin;
	idStr					description;
	idDict					sounds;

	virtual size_t			Size( void ) const;
	virtual const char *	DefaultDefinition() const;
	virtual bool			Parse( const char *text, const int textLength );
	virtual void			FreeData( void );
	virtual void			Print( void );

	virtual	bool			RebuildTextSource( void ) { return( false ); }
// jmarshall - not used in quake 4 sdk.
	virtual bool			Validate(const char* psText, int iTextLength, idStr& strReportTo) const { return true; }
// jmarshall end
};

#endif 
