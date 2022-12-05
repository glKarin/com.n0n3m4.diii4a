
#include "../precompiled.h"
#pragma hdrstop

idComplex complex_origin( 0.0f, 0.0f );

/*
=============
idComplex::ToString
=============
*/
const char *idComplex::ToString( int precision ) const {
	return idStr::FloatArrayToString( ToFloatPtr(), GetDimension(), precision );
}
