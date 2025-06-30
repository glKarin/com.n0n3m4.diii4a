/* Copyright (c) 2002-2012 Croteam Ltd. 
This program is free software; you can redistribute it and/or modify
it under the terms of version 2 of the GNU General Public License as published by
the Free Software Foundation


This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA. */

/*
======================================================================
vecmat.h

Basic vector and matrix functions.
====================================================================== */

#include <lwtypes.h>

#define vecangle(a,b) (float)acos(dot(a,b))    /* a and b must be unit vectors */

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus


float dot( LWFVector a, LWFVector b );
void cross( LWFVector a, LWFVector b, LWFVector c );
void normalize( LWFVector v );

#ifdef __cplusplus
}
#endif //__cplusplus
