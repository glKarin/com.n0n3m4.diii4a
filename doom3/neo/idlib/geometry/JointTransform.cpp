/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "../precompiled.h"
#pragma hdrstop


/*
=============
idJointMat::ToJointQuat
=============
*/
idJointQuat idJointMat::ToJointQuat(void) const
{
	idJointQuat	jq;
	float		trace;
	float		s;
	float		t;
	int     	i;
	int			j;
	int			k;

	static int 	next[3] = { 1, 2, 0 };

	trace = mat[0 * 4 + 0] + mat[1 * 4 + 1] + mat[2 * 4 + 2];

	if (trace > 0.0f) {

		t = trace + 1.0f;
		s = idMath::InvSqrt(t) * 0.5f;

		jq.q[3] = s * t;
		jq.q[0] = (mat[1 * 4 + 2] - mat[2 * 4 + 1]) * s;
		jq.q[1] = (mat[2 * 4 + 0] - mat[0 * 4 + 2]) * s;
		jq.q[2] = (mat[0 * 4 + 1] - mat[1 * 4 + 0]) * s;

	} else {

		i = 0;

		if (mat[1 * 4 + 1] > mat[0 * 4 + 0]) {
			i = 1;
		}

		if (mat[2 * 4 + 2] > mat[i * 4 + i]) {
			i = 2;
		}

		j = next[i];
		k = next[j];

		t = (mat[i * 4 + i] - (mat[j * 4 + j] + mat[k * 4 + k])) + 1.0f;
		s = idMath::InvSqrt(t) * 0.5f;

		jq.q[i] = s * t;
		jq.q[3] = (mat[j * 4 + k] - mat[k * 4 + j]) * s;
		jq.q[j] = (mat[i * 4 + j] + mat[j * 4 + i]) * s;
		jq.q[k] = (mat[i * 4 + k] + mat[k * 4 + i]) * s;
	}

	jq.t[0] = mat[0 * 4 + 3];
	jq.t[1] = mat[1 * 4 + 3];
	jq.t[2] = mat[2 * 4 + 3];
#ifdef _SPLASHDAMAGE
    jq.w = 0.0f;
#endif

	return jq;
}

#ifdef _SPLASHDAMAGE
void idJointMat::ToDualQuat( float dq[2][4] ) const
{
    float		trace;
    float		s;
    float		t;
    int     	i;
    int			j;
    int			k;

    static int 	next[3] = { 1, 2, 0 };

    trace = mat[0 * 4 + 0] + mat[1 * 4 + 1] + mat[2 * 4 + 2];

    if ( trace > 0.0f ) {

        t = trace + 1.0f;
        s = idMath::InvSqrt( t ) * 0.5f;

        dq[0][0] = s * t;
        dq[0][1] = -( mat[1 * 4 + 2] - mat[2 * 4 + 1] ) * s;
        dq[0][2] = -( mat[2 * 4 + 0] - mat[0 * 4 + 2] ) * s;
        dq[0][3] = -( mat[0 * 4 + 1] - mat[1 * 4 + 0] ) * s;

    } else {

        i = 0;
        if ( mat[1 * 4 + 1] > mat[0 * 4 + 0] ) {
            i = 1;
        }
        if ( mat[2 * 4 + 2] > mat[i * 4 + i] ) {
            i = 2;
        }
        j = next[i];
        k = next[j];

        t = ( mat[i * 4 + i] - ( mat[j * 4 + j] + mat[k * 4 + k] ) ) + 1.0f;
        s = idMath::InvSqrt( t ) * 0.5f;

        dq[0][i+1] = -s * t;
        dq[0][0] = ( mat[j * 4 + k] - mat[k * 4 + j] ) * s;
        dq[0][j+1] = -( mat[i * 4 + j] + mat[j * 4 + i] ) * s;
        dq[0][k+1] = -( mat[i * 4 + k] + mat[k * 4 + i] ) * s;
    }

    float t0 = mat[0 * 4 + 3];
    float t1 = mat[1 * 4 + 3];
    float t2 = mat[2 * 4 + 3];
    dq[1][0] = -0.5*(t0*dq[0][1] + t1*dq[0][2] + t2*dq[0][3]);
    dq[1][1] = 0.5*( t0*dq[0][0] + t1*dq[0][3] - t2*dq[0][2]);
    dq[1][2] = 0.5*(-t0*dq[0][3] + t1*dq[0][0] + t2*dq[0][1]);
    dq[1][3] = 0.5*( t0*dq[0][2] - t1*dq[0][1] + t2*dq[0][0]);
}
#endif