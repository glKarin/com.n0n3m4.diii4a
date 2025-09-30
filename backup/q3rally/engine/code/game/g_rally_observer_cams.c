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

#include "g_local.h"


void SP_info_observer_spot( gentity_t *ent ){
	G_SetOrigin(ent, ent->s.origin);

	if( ent->target )
	{
		ent->spawnflags |= OBSERVERCAM_FIXED;
	}
}


gentity_t *FindBestObserverSpot( gentity_t *self, gentity_t *target, vec3_t spot, vec3_t angles){
	gentity_t		*ent;
	trace_t			tr;
	vec3_t			delta;
	static vec3_t	mins = { -4, -4, -4 };
	static vec3_t	maxs = { 4, 4, 4 };
	float			dist, bestDist;
	gentity_t		*foundSpot;

	foundSpot = NULL;
	dist = 0;
	bestDist = 0;
	ent = NULL;
	while ( (ent = G_Find (ent, FOFS(classname), "info_observer_spot")) != NULL )
	{
//		if ( !trap_InPVS( ent->s.origin, target->s.origin) ) continue;

//		Com_Printf("Found an observer spot in PVS\n");
//		VectorCopy(ent->s.origin, spot);
//		foundSpot = ent;
//		return foundSpot;
		
		trap_Trace(&tr, ent->r.currentOrigin, mins, maxs, target->client->ps.origin, target->s.number, CONTENTS_SOLID);

		if (tr.startsolid || tr.allsolid || tr.fraction < 1.0) continue;

		VectorSubtract(target->s.origin, ent->s.origin, delta);
		dist = VectorNormalize(delta);

		// check for spot with locked angles
		if (ent->spawnflags & OBSERVERCAM_FIXED)
		{
			vec3_t	forward;

			AngleVectors(ent->s.angles, forward, NULL, NULL);
			if (DotProduct(delta, forward) < -0.40)
			{
				VectorCopy(ent->s.origin, spot);
				VectorCopy(ent->s.angles, angles);

				self->spotflags = ent->spawnflags;

				// use this one
				return ent;
			}
		}

		if (dist < bestDist || bestDist == 0)
		{
			bestDist = dist;
			VectorCopy(ent->s.origin, spot);
			VectorCopy(ent->s.angles, angles);

//			Com_Printf("Found a valid observer spot\n");
			self->spotflags = ent->spawnflags;
			foundSpot = ent;
		}
	}

	return foundSpot;
}

void UpdateObserverSpot( gentity_t *ent, qboolean forceUpdate ){
	vec3_t			origin, angles;
	trace_t			tr;
	int				clientNum;
	static vec3_t	mins = { -4, -4, -4 };
	static vec3_t	maxs = { 4, 4, 4 };

	clientNum = ent->client->sess.spectatorClient;
	if ( clientNum == -1 )
		clientNum = level.follow1;
	else if ( clientNum == -2 )
		clientNum = level.follow2;

	if (clientNum < 0)
	{
//		ent->client->sess.spectatorState = SPECTATOR_FREE;
//		G_DebugLogPrintf( "UpdateObserverSpot: drop back to free\n" );
		StopFollowing( ent );
//		ClientSpawn( ent );
		return;
	}

	trap_Trace( &tr, ent->client->ps.origin, mins, maxs, level.clients[clientNum].ps.origin, ent->s.number, CONTENTS_SOLID );
	if ( forceUpdate || tr.fraction < 1 )
	{
		if ( !FindBestObserverSpot(ent, &g_entities[clientNum], origin, angles) )
		{
			if (ent->updateTime + 500 < level.time){
				ent->updateTime = level.time;
				trap_SendServerCommand( ent - g_entities, "print \"Couldnt find valid observer spot, dropping back to follow mode.\n\"" );
				ent->client->sess.spectatorState = SPECTATOR_FOLLOW;
				return;
			}
		}
		else
		{
//			Com_Printf( "Updating observer position" );

			G_SetOrigin(ent, origin);
			VectorCopy(origin, ent->client->ps.origin);
			VectorCopy(angles, ent->client->ps.viewangles);
			ent->updateTime = level.time;
		}
	}
	else {
		ent->updateTime = level.time;
	}
}

