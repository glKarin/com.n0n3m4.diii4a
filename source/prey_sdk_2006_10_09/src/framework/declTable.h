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

class idDeclTable : public idDecl {
public:
	virtual size_t			Size( void ) const;
	virtual const char *	DefaultDefinition( void ) const;
	virtual bool			Parse( const char *text, const int textLength );
	virtual void			FreeData( void );

	virtual	//HUMANHEAD pdm: made virtual so it can be called from game code
	float					TableLookup( float index ) const;

	//HUMANHEAD: aob - interfaces to allow integrating tables
	virtual int				NumElements() const { return values.Num(); }
	virtual float			Integrate( float frac ) const;

protected:
	virtual float			IntegratePreviousAreas( int numAreas, float inverseNumAreas ) const;
	virtual float			IntegrateArea( int fromElement, int toElement, float inverseNumAreas ) const;
	//HUMANHEAD END

private:
	bool					clamp;
	bool					snap;
	idList<float>			values;
};

#endif /* !__DECLTABLE_H__ */
