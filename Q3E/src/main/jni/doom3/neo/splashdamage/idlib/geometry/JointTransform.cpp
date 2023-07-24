// Copyright (C) 2007 Id Software, Inc.
//

#include "../precompiled.h"
#pragma hdrstop


/*
=============
idJointMat::ToJointQuat
=============
*/
idJointQuat idJointMat::ToJointQuat( void ) const {
	idJointQuat	jq;
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

		jq.q[3] = s * t;
		jq.q[0] = ( mat[1 * 4 + 2] - mat[2 * 4 + 1] ) * s;
		jq.q[1] = ( mat[2 * 4 + 0] - mat[0 * 4 + 2] ) * s;
		jq.q[2] = ( mat[0 * 4 + 1] - mat[1 * 4 + 0] ) * s;

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

		jq.q[i] = s * t;
		jq.q[3] = ( mat[j * 4 + k] - mat[k * 4 + j] ) * s;
		jq.q[j] = ( mat[i * 4 + j] + mat[j * 4 + i] ) * s;
		jq.q[k] = ( mat[i * 4 + k] + mat[k * 4 + i] ) * s;
	}

	jq.t[0] = mat[0 * 4 + 3];
	jq.t[1] = mat[1 * 4 + 3];
	jq.t[2] = mat[2 * 4 + 3];
	jq.w = 0.0f;
	return jq;
}


void idJointMat::ToDualQuat( float dq[2][4] ) const {
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
