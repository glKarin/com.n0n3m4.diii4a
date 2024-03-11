/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#ifndef __SURFACE_PATCH_H__
#define __SURFACE_PATCH_H__

/*
===============================================================================

	Bezier patch surface.

===============================================================================
*/

class idSurface_Patch : public idSurface {

public:
						idSurface_Patch( void );
						idSurface_Patch( int maxPatchWidth, int maxPatchHeight );
						idSurface_Patch( const idSurface_Patch &patch );
						~idSurface_Patch( void );

	void				SetSize( int patchWidth, int patchHeight );
	int					GetWidth( void ) const;
	int					GetHeight( void ) const;

						// subdivide the patch mesh based on error
	void				Subdivide( float maxHorizontalError, float maxVerticalError, float maxLength, bool genNormals = false );
						// subdivide the patch up to an explicit number of horizontal and vertical subdivisions
	void				SubdivideExplicit( int horzSubdivisions, int vertSubdivisions, bool genNormals, bool removeLinear = false );

protected:
	int					width;			// width of patch
	int					height;			// height of patch
	int					maxWidth;		// maximum width allocated for
	int					maxHeight;		// maximum height allocated for
	bool				expanded;		// true if vertices are spaced out

private:
						// put the approximation points on the curve
	void				PutOnCurve( void );
						// remove columns and rows with all points on one line
	void				RemoveLinearColumnsRows( void );
						// resize verts buffer
	void				ResizeExpanded( int height, int width );
						// space points out over maxWidth * maxHeight buffer
	void				Expand( void );
						// move all points to the start of the verts buffer
	void				Collapse( void );
						// project a point onto a vector to calculate maximum curve error
	void				ProjectPointOntoVector( const idVec3 &point, const idVec3 &vStart, const idVec3 &vEnd, idVec3 &vProj );
						// generate normals
	void				GenerateNormals( void );
						// generate triangle indexes
	void				GenerateIndexes( void );
						// lerp point from two patch point
	void				LerpVert( const idDrawVert &a, const idDrawVert &b, idDrawVert &out ) const;
						// sample a single 3x3 patch
	void				SampleSinglePatchPoint( const idDrawVert ctrl[3][3], float u, float v, idDrawVert *out ) const;
	void				SampleSinglePatch( const idDrawVert ctrl[3][3], int baseCol, int baseRow, int width, int horzSub, int vertSub, idDrawVert *outVerts ) const;
};

/*
=================
idSurface_Patch::idSurface_Patch
=================
*/
ID_INLINE idSurface_Patch::idSurface_Patch( void ) {
	height = width = maxHeight = maxWidth = 0;
	expanded = false;
}

/*
=================
idSurface_Patch::idSurface_Patch
=================
*/
ID_INLINE idSurface_Patch::idSurface_Patch( int maxPatchWidth, int maxPatchHeight ) {
	width = height = 0;
	maxWidth = maxPatchWidth;
	maxHeight = maxPatchHeight;
	verts.SetNum( maxWidth * maxHeight );
	expanded = false;
}

/*
=================
idSurface_Patch::idSurface_Patch
=================
*/
ID_INLINE idSurface_Patch::idSurface_Patch( const idSurface_Patch &patch ) {
	(*this) = patch;
}

/*
=================
idSurface_Patch::~idSurface_Patch
=================
*/
ID_INLINE idSurface_Patch::~idSurface_Patch() {
}

/*
=================
idSurface_Patch::GetWidth
=================
*/
ID_INLINE int idSurface_Patch::GetWidth( void ) const {
	return width;
}

/*
=================
idSurface_Patch::GetHeight
=================
*/
ID_INLINE int idSurface_Patch::GetHeight( void ) const {
	return height;
}

#endif /* !__SURFACE_PATCH_H__ */
