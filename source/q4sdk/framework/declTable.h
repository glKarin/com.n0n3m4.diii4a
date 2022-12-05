// Copyright (C) 2004 Id Software, Inc.
//

#ifndef __DECLTABLE_H__
#define __DECLTABLE_H__

/*
===============================================================================

	tables are used to map a floating point input value to a floating point
	output value, with optional wrap / clamp and interpolation

===============================================================================
*/

// RAVEN BEGIN
// jsinger: allow support for serialization/deserialization of binary decls
#ifdef RV_BINARYDECLS
class idDeclTable : public idDecl, public Serializable<'DTAB'> {
public:
// jsinger: allow exporting of this decl type in a preparsed form
	virtual void			Write( SerialOutputStream &stream) const;
	virtual void			AddReferences() const;
							idDeclTable( SerialInputStream &stream);
#else
class idDeclTable : public idDecl {
#endif
public:
							idDeclTable();
	virtual size_t			Size( void ) const;
	virtual const char *	DefaultDefinition( void ) const;
	virtual bool			Parse( const char *text, const int textLength, bool noCaching );
	virtual void			FreeData( void );

// RAVEN BEGIN
// jscott: for BSE
			float			GetMaxValue( void ) const { return( maxValue ); }
			float			GetMinValue( void ) const { return( minValue ); }
// bdube: made virtual so it can be accessed in game
	virtual float			TableLookup( float index ) const;
// jscott: to prevent a recursive crash
	virtual	bool			RebuildTextSource( void ) { return( false ); }
// scork: for detailed error-reporting
	virtual	bool			Validate( const char *psText, int iTextLength, idStr &strReportTo ) const;
// RAVEN END

private:
	bool					clamp;
	bool					snap;
// RAVEN BEGIN
// jscott: for BSE
	float					minValue;
	float					maxValue;
// RAVEN END
	idList<float>			values;
};

#endif /* !__DECLTABLE_H__ */
