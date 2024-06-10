/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company. 

This file is part of the Doom 3 BFG Edition GPL Source Code ("Doom 3 BFG Edition Source Code").  

Doom 3 BFG Edition Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 BFG Edition Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 BFG Edition Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 BFG Edition Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 BFG Edition Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#ifndef __GLMATRIX_H__
#define __GLMATRIX_H__

/*
==========================================================================================

This deals with column-major (OpenGL style) matrices where transforms are
applied with right-multiplication.

This is the old DOOM3 matrix code that should really to be replaced with idRenderMatrix.

==========================================================================================
*/

void R_AxisToModelMatrix( const idMat3 &axis, const idVec3 &origin, float modelMatrix[16] );
void R_MatrixTranspose( const float in[16], float out[16] );
void R_MatrixMultiply( const float *a, const float *b, float *out );

void R_TransformModelToClip( const idVec3 &src, const float *modelMatrix, const float *projectionMatrix, idPlane &eye, idPlane &dst );
void R_TransformClipToDevice( const idPlane &clip, idVec3 &ndc );
void R_GlobalToNormalizedDeviceCoordinates( const idVec3 &global, idVec3 &ndc );

// note that these assume a normalized matrix, and will not work with scaled axis
void R_GlobalPointToLocal( const float modelMatrix[16], const idVec3 &in, idVec3 &out );
void R_LocalPointToGlobal( const float modelMatrix[16], const idVec3 &in, idVec3 &out );

void R_GlobalVectorToLocal( const float modelMatrix[16], const idVec3 &in, idVec3 &out );
void R_LocalVectorToGlobal( const float modelMatrix[16], const idVec3 &in, idVec3 &out );

void R_GlobalPlaneToLocal( const float modelMatrix[16], const idPlane &in, idPlane &out );
void R_LocalPlaneToGlobal( const float modelMatrix[16], const idPlane &in, idPlane &out );

void R_SetupViewMatrix( idRenderWorldCommitted *viewDef );
void R_SetupProjectionMatrix( idRenderWorldCommitted *viewDef );
void R_SetupProjectionMatrix(idRenderWorldCommitted* viewDef, float custom_fov_x, float custom_fov_y, float projectionMatrix[16]);

void R_MatrixFullInverse(const float a[16], float r[16]);
void R_SetupUnprojection(idRenderWorldCommitted* viewDef);


/*
==========================
myGlMultMatrix
==========================
*/
ID_INLINE void myGlMultMatrix(const float a[16], const float b[16], float out[16]) {
#if 0
	int		i, j;

	for (i = 0; i < 4; i++) {
		for (j = 0; j < 4; j++) {
			out[i * 4 + j] =
				a[i * 4 + 0] * b[0 * 4 + j]
				+ a[i * 4 + 1] * b[1 * 4 + j]
				+ a[i * 4 + 2] * b[2 * 4 + j]
				+ a[i * 4 + 3] * b[3 * 4 + j];
		}
	}
#else
	out[0 * 4 + 0] = a[0 * 4 + 0] * b[0 * 4 + 0] + a[0 * 4 + 1] * b[1 * 4 + 0] + a[0 * 4 + 2] * b[2 * 4 + 0] + a[0 * 4 + 3] * b[3 * 4 + 0];
	out[0 * 4 + 1] = a[0 * 4 + 0] * b[0 * 4 + 1] + a[0 * 4 + 1] * b[1 * 4 + 1] + a[0 * 4 + 2] * b[2 * 4 + 1] + a[0 * 4 + 3] * b[3 * 4 + 1];
	out[0 * 4 + 2] = a[0 * 4 + 0] * b[0 * 4 + 2] + a[0 * 4 + 1] * b[1 * 4 + 2] + a[0 * 4 + 2] * b[2 * 4 + 2] + a[0 * 4 + 3] * b[3 * 4 + 2];
	out[0 * 4 + 3] = a[0 * 4 + 0] * b[0 * 4 + 3] + a[0 * 4 + 1] * b[1 * 4 + 3] + a[0 * 4 + 2] * b[2 * 4 + 3] + a[0 * 4 + 3] * b[3 * 4 + 3];
	out[1 * 4 + 0] = a[1 * 4 + 0] * b[0 * 4 + 0] + a[1 * 4 + 1] * b[1 * 4 + 0] + a[1 * 4 + 2] * b[2 * 4 + 0] + a[1 * 4 + 3] * b[3 * 4 + 0];
	out[1 * 4 + 1] = a[1 * 4 + 0] * b[0 * 4 + 1] + a[1 * 4 + 1] * b[1 * 4 + 1] + a[1 * 4 + 2] * b[2 * 4 + 1] + a[1 * 4 + 3] * b[3 * 4 + 1];
	out[1 * 4 + 2] = a[1 * 4 + 0] * b[0 * 4 + 2] + a[1 * 4 + 1] * b[1 * 4 + 2] + a[1 * 4 + 2] * b[2 * 4 + 2] + a[1 * 4 + 3] * b[3 * 4 + 2];
	out[1 * 4 + 3] = a[1 * 4 + 0] * b[0 * 4 + 3] + a[1 * 4 + 1] * b[1 * 4 + 3] + a[1 * 4 + 2] * b[2 * 4 + 3] + a[1 * 4 + 3] * b[3 * 4 + 3];
	out[2 * 4 + 0] = a[2 * 4 + 0] * b[0 * 4 + 0] + a[2 * 4 + 1] * b[1 * 4 + 0] + a[2 * 4 + 2] * b[2 * 4 + 0] + a[2 * 4 + 3] * b[3 * 4 + 0];
	out[2 * 4 + 1] = a[2 * 4 + 0] * b[0 * 4 + 1] + a[2 * 4 + 1] * b[1 * 4 + 1] + a[2 * 4 + 2] * b[2 * 4 + 1] + a[2 * 4 + 3] * b[3 * 4 + 1];
	out[2 * 4 + 2] = a[2 * 4 + 0] * b[0 * 4 + 2] + a[2 * 4 + 1] * b[1 * 4 + 2] + a[2 * 4 + 2] * b[2 * 4 + 2] + a[2 * 4 + 3] * b[3 * 4 + 2];
	out[2 * 4 + 3] = a[2 * 4 + 0] * b[0 * 4 + 3] + a[2 * 4 + 1] * b[1 * 4 + 3] + a[2 * 4 + 2] * b[2 * 4 + 3] + a[2 * 4 + 3] * b[3 * 4 + 3];
	out[3 * 4 + 0] = a[3 * 4 + 0] * b[0 * 4 + 0] + a[3 * 4 + 1] * b[1 * 4 + 0] + a[3 * 4 + 2] * b[2 * 4 + 0] + a[3 * 4 + 3] * b[3 * 4 + 0];
	out[3 * 4 + 1] = a[3 * 4 + 0] * b[0 * 4 + 1] + a[3 * 4 + 1] * b[1 * 4 + 1] + a[3 * 4 + 2] * b[2 * 4 + 1] + a[3 * 4 + 3] * b[3 * 4 + 1];
	out[3 * 4 + 2] = a[3 * 4 + 0] * b[0 * 4 + 2] + a[3 * 4 + 1] * b[1 * 4 + 2] + a[3 * 4 + 2] * b[2 * 4 + 2] + a[3 * 4 + 3] * b[3 * 4 + 2];
	out[3 * 4 + 3] = a[3 * 4 + 0] * b[0 * 4 + 3] + a[3 * 4 + 1] * b[1 * 4 + 3] + a[3 * 4 + 2] * b[2 * 4 + 3] + a[3 * 4 + 3] * b[3 * 4 + 3];
#endif
}

/*
================
R_TransposeGLMatrix
================
*/
ID_INLINE void R_TransposeGLMatrix(const float in[16], float out[16]) {
	int		i, j;

	for (i = 0; i < 4; i++) {
		for (j = 0; j < 4; j++) {
			out[i * 4 + j] = in[j * 4 + i];
		}
	}
}

#endif /* !__GLMATRIX_H__ */
