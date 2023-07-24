// Copyright (C) 2007 Id Software, Inc.
//


#include "../precompiled.h"
#pragma hdrstop


/*
=============
idMat3x4::ToString
=============
*/
const char *idMat3x4::ToString( int precision ) const {
	return idStr::FloatArrayToString( ToFloatPtr(), 3*4, precision );
}
