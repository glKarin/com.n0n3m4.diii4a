// Copyright (C) 2004 Id Software, Inc.
//

#ifndef __DECLENTITYDEF_H__
#define __DECLENTITYDEF_H__

/*
===============================================================================

	idDeclEntityDef

===============================================================================
*/

// RAVEN BEGIN
// jsinger: added to support serialization/deserialization of binary decls
#ifdef RV_BINARYDECLS
class idDeclEntityDef : public idDecl, public Serializable<'DED '> {
public:
	virtual void			AddReferences() const;
	virtual void			Write(SerialOutputStream &stream) const;
							idDeclEntityDef(SerialInputStream &stream);
#else
class idDeclEntityDef : public idDecl {
#endif
// RAVEN END
public:
							idDeclEntityDef();
	idDict					dict;

	virtual size_t			Size( void ) const;
	virtual const char *	DefaultDefinition() const;
	virtual bool			Parse( const char *text, const int textLength, bool noCaching );
	virtual void			FreeData( void );
	virtual void			Print( void );

// RAVEN BEGIN
// jscott: to prevent a recursive crash
	virtual	bool			RebuildTextSource( void ) { return( false ); }
// scork: for detailed error-reporting
	virtual bool			Validate( const char *psText, int iTextLength, idStr &strReportTo ) const;
// RAVEN END

};

#endif /* !__DECLENTITYDEF_H__ */
