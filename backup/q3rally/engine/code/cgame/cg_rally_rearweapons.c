/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2002-2021 Q3Rally Team (Per Thormann - q3rally@gmail.com)

This file is part of q3rally source code.

q3rally source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

q3rally source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with q3rally; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "cg_local.h"

/*
===================================================================================================

REAR WEAPON EVENTS

===================================================================================================
*/

/*
================
CG_FireRearWeapon

Caused by an EV_FIRE_REARWEAPON event
================
*/
void CG_FireRearWeapon( centity_t *cent, int weapon ) {
	entityState_t *ent;
	int				c;
	weaponInfo_t	*weap;

	ent = &cent->currentState;
	if ( weapon < RWP_SMOKE ) {
		return;
	}
	if ( weapon >= WP_NUM_WEAPONS ) {
		CG_Error( "CG_FireRearWeapon: weapon >= WP_NUM_WEAPONS" );
		return;
	}
	weap = &cg_weapons[ weapon ];

	// play a sound
	for ( c = 0 ; c < 4 ; c++ ) {
		if ( !weap->flashSound[c] ) {
			break;
		}
	}
	if ( c > 0 ) {
		c = rand() % c;
		if ( weap->flashSound[c] ) {
			trap_S_StartSound( NULL, ent->number, CHAN_WEAPON, weap->flashSound[c] );
		}
	}

	switch( weapon ){
	default:
	case RWP_SMOKE:
		CG_StartSmokeScreen( cent );
		break;

	case RWP_OIL:
		CG_DropOil( cent );
		break;

	case RWP_FLAME:
		CG_StartFlameTrail( cent );
		break;

	case RWP_BIO:
		CG_DropBio( cent );
		break;
	}
}

/*
================
CG_StartSmokeScreen

Sets up the centity to spit out smoke for 600 milliseconds after this is called.
================
*/
void CG_StartSmokeScreen( centity_t *cent ){
}

/*
================
CG_StartFlameTrail

Sets up the centity to spit out flame for 600 milliseconds after this is called.
================
*/
void CG_StartFlameTrail( centity_t *cent ){
}

/*
================
CG_DropOil

Sets up the centity to drop a single pool of oil.  If there is another pool
nearby it will add to that one instead.
================
*/
void CG_DropOil( centity_t *cent ){
}

/*
================
CG_DropBio

Sets up the centity to drop a single pool of bio.  If there is another pool
nearby it will add to that one instead.
================
*/
void CG_DropBio( centity_t *cent ){
}
