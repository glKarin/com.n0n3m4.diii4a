// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __DECLTABLE_H__
#define __DECLTABLE_H__

#include "../framework/declManager.h"

/*
===============================================================================

	tables are used to map a floating point input value to a floating point
	output value, with optional wrap / clamp and interpolation

===============================================================================
*/

class idDeclTable : public idDecl {
public:

	virtual					~idDeclTable( void ) {}

	virtual size_t			Size( void ) const;
	virtual const char *	DefaultDefinition( void ) const;
	virtual bool			Parse( const char *text, const int textLength );
	virtual void			FreeData( void );

// RAVEN BEGIN
// jscott: for BSE
			float			GetMaxValue( void ) const { return( maxValue ); }
			float			GetMinValue( void ) const { return( minValue ); }
// bdube: made virtual so it can be accessed in game
	virtual float			TableLookup( float index ) const;
	int						NumValues( void ) const { return values.Num(); }
	float					GetValue( int index ) const { return values[ index ]; }

private:
	bool					clamp;
	bool					snap;
	bool					discontinuous;
	bool					isLinear;
// RAVEN BEGIN
// jscott: for BSE
	float					minValue;
	float					maxValue;
// RAVEN END
	idList<float>			values;
};

#endif /* !__DECLTABLE_H__ */
