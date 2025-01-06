/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
//
#include "g_local.h"


void InitTrigger( gentity_t *self ) {
	if (!VectorCompare (self->s.angles, vec3_origin))
		G_SetMovedir (self->s.angles, self->movedir);

	if(!self->sandboxObject){
	trap_SetBrushModel( self, self->model );
	}
	self->r.contents = CONTENTS_TRIGGER;		// replaces the -1 from trap_SetBrushModel
	self->r.svFlags = SVF_NOCLIENT;
}


// the wait time has passed, so set back up for another activation
void multi_wait( gentity_t *ent ) {
	ent->nextthink = 0;
}


// the trigger was just activated
// ent->activator should be set to the activator so it can be held through a delay
// so wait for the delay time before firing
void multi_trigger( gentity_t *ent, gentity_t *activator ) {
	//if nobots flag is set and activator is a bot, do nothing
	if ( (ent->flags & FL_NO_BOTS) && IsBot( activator ) )
		return;

	//if nohumans flag is set and activtaor is a human, do nothing
	if ( (ent->flags & FL_NO_HUMANS) && !IsBot( activator ) )
		return;
	
	if(strlen(ent->message) >= 1){
	if(!Q_stricmp (activator->botspawn->message, ent->message)){
	
	} else {
	return;
	}
	}
	
	
	ent->activator = activator;
	if ( ent->nextthink ) {
		return;		// can't retrigger until the wait is over
	}

	if ( activator->client ) {
		if ( ( ent->spawnflags & 1 ) &&
			activator->client->sess.sessionTeam != TEAM_RED ) {
			return;
		}
		if ( ( ent->spawnflags & 2 ) &&
			activator->client->sess.sessionTeam != TEAM_BLUE ) {
			return;
		}
	}

	G_UseTargets (ent, ent->activator);

	if ( ent->wait > 0 ) {
		ent->think = multi_wait;
		ent->nextthink = level.time + ( ent->wait + ent->random * crandom() ) * 1000;
	} else {
		// we can't just remove (self) here, because this is a touch function
		// called while looping through area links...
		ent->touch = 0;
		ent->nextthink = level.time + FRAMETIME;
		ent->think = G_FreeEntity;
	}
}

void Use_Multi( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	usercmd_t	*ucmd;
	
ucmd = &activator->client->pers.cmd;
		
if(ent->owner != activator->s.clientNum + 1){
if(ent->owner != 0){
trap_SendServerCommand( activator->s.clientNum, va( "cp \"Owned by %s\n\"", ent->ownername ));
return;
}	
}
if(ent->locked != 0){
return;
}

if(ent->price > 0){
	
if(activator->client->pers.oldmoney < ent->price){	
if(ucmd->buttons & BUTTON_GESTURE){
trap_SendServerCommand( activator->s.clientNum, va( "lp \"%i is not enough\"\n", ent->price - activator->client->pers.oldmoney ));
return;	
} else {
trap_SendServerCommand( activator->s.clientNum, va( "lp \"^1%s %i$\"\n", ent->message, ent->price ));
return;		
}
}

if(activator->client->pers.oldmoney >= ent->price){	
if(ucmd->buttons & BUTTON_GESTURE){
trap_SendServerCommand( activator->s.clientNum, va( "lp \"%s purchased\"\n", ent->message, ent->price ));
} else {
trap_SendServerCommand( activator->s.clientNum, va( "lp \"^2%s %i$\"\n", ent->message, ent->price ));
return;		
}
}

}


if(ent->price == -1){
	
if(ucmd->buttons & BUTTON_GESTURE){
trap_SendServerCommand( activator->s.clientNum, va( "lp \"%s activated\"\n", ent->message));
} else {
trap_SendServerCommand( activator->s.clientNum, va( "lp \"^2%s\"\n", ent->message));
return;
}

}



	multi_trigger( ent, activator );
}

void Touch_Multi( gentity_t *self, gentity_t *other, trace_t *trace ) {
	usercmd_t	*ucmd;
	
ucmd = &other->client->pers.cmd;

	if( !other->client ) {
		return;
	}
if(self->owner != other->s.clientNum + 1){
if(self->owner != 0){
trap_SendServerCommand( other->s.clientNum, va( "cp \"Owned by %s\n\"", self->ownername ));
return;
}	
}
if(self->locked != 0){
return;
}

if(self->price > 0){
	
if(other->client->pers.oldmoney < self->price){	
if(ucmd->buttons & BUTTON_GESTURE){
trap_SendServerCommand( other->s.clientNum, va( "lp \"%i is not enough\"\n", self->price - other->client->pers.oldmoney ));
return;	
} else {
trap_SendServerCommand( other->s.clientNum, va( "lp \"^1%s %i$\"\n", self->message, self->price ));
return;		
}
}

if(other->client->pers.oldmoney >= self->price){	
if(ucmd->buttons & BUTTON_GESTURE){
trap_SendServerCommand( other->s.clientNum, va( "lp \"%s purchased\"\n", self->message, self->price ));
} else {
trap_SendServerCommand( other->s.clientNum, va( "lp \"^2%s %i$\"\n", self->message, self->price ));
return;		
}
}

}


if(self->price == -1){
	
if(ucmd->buttons & BUTTON_GESTURE){
trap_SendServerCommand( other->s.clientNum, va( "lp \"%s activated\"\n", self->message));
} else {
trap_SendServerCommand( other->s.clientNum, va( "lp \"^2%s\"\n", self->message));
return;
}

}

	multi_trigger( self, other );
}

/*QUAKED trigger_multiple (.5 .5 .5) ?
"wait" : Seconds between triggerings, 0.5 default, -1 = one time only.
"random"	wait variance, default is 0
Variable sized repeatable trigger.  Must be targeted at one or more entities.
so, the basic time between firing is a random time between
(wait - random) and (wait + random)
*/
void SP_trigger_multiple( gentity_t *ent ) {
	int		i;

	G_SpawnInt( "nobots", "0", &i);
	if ( i ) {
		ent->flags |= FL_NO_BOTS;
	}
	G_SpawnInt( "nohumans", "0", &i );
	if ( i ) {
		ent->flags |= FL_NO_HUMANS;
	}
	
	G_SpawnFloat( "wait", "0.5", &ent->wait );
	G_SpawnFloat( "random", "0", &ent->random );

	if ( ent->random >= ent->wait && ent->wait >= 0 ) {
		ent->random = ent->wait - FRAMETIME;
                G_Printf( "trigger_multiple has random >= wait\n" );
	}

	ent->touch = Touch_Multi;
	ent->use = Use_Multi;

	InitTrigger( ent );
	trap_LinkEntity (ent);
}



/*
==============================================================================

trigger_always

==============================================================================
*/

void trigger_always_think( gentity_t *ent ) {
	G_UseTargets(ent, ent);
	G_FreeEntity( ent );
}

/*QUAKED trigger_always (.5 .5 .5) (-8 -8 -8) (8 8 8)
This trigger will always fire.  It is activated by the world.
*/
void SP_trigger_always (gentity_t *ent) {
	// we must have some delay to make sure our use targets are present
	ent->nextthink = level.time + 1000;
	ent->think = trigger_always_think;
}


/*
==============================================================================

trigger_push

==============================================================================
*/

/*QUAKED trigger_push SILENT 
This trigger will push a player or bot towards a targeted entity.
*/
void trigger_push_touch (gentity_t *self, gentity_t *other, trace_t *trace ) {

	if ( !other->client ) {
		return;
	}

	BG_TouchJumpPad( &other->client->ps, &self->s, !(self->spawnflags & 1) );
}


/*
=================
AimAtTarget

Calculate origin2 so the target apogee will be hit
=================
*/
void AimAtTarget( gentity_t *self ) {
	gentity_t	*ent;
	vec3_t		origin;
	float		height, gravity, time, forward;
	float		dist;

	VectorAdd( self->r.absmin, self->r.absmax, origin );
	VectorScale ( origin, 0.5, origin );

	ent = G_PickTarget( self->target );
	if ( !ent ) {
		G_FreeEntity( self );
		return;
	}

	height = ent->s.origin[2] - origin[2];
	gravity = g_gravity.value*g_gravityModifier.value;
	time = sqrt( height / ( .5 * gravity ) );
	if ( !time ) {
		G_FreeEntity( self );
		return;
	}

	// set s.origin2 to the push velocity
	VectorSubtract ( ent->s.origin, origin, self->s.origin2 );
	self->s.origin2[2] = 0;
	dist = VectorNormalize( self->s.origin2);

	forward = dist / time;
	VectorScale( self->s.origin2, forward, self->s.origin2 );

	self->s.origin2[2] = time * gravity;
}


/*QUAKED trigger_push (.5 .5 .5) ?
Must point at a target_position, which will be the apex of the leap.
This will be client side predicted, unlike target_push
*/
void SP_trigger_push( gentity_t *self ) {
	InitTrigger (self);

	// unlike other triggers, we need to send this one to the client
	self->r.svFlags = SVF_NOCLIENT;

	// make sure the client precaches this sound
	G_SoundIndex("sound/world/jumppad.wav");

	self->s.eType = ET_PUSH_TRIGGER;
	self->touch = trigger_push_touch;
	self->think = AimAtTarget;
	self->nextthink = level.time + FRAMETIME;
	trap_LinkEntity (self);
}


void Use_target_push( gentity_t *self, gentity_t *other, gentity_t *activator ) {
	if ( !activator->client ) {
		return;
	}

	if ( activator->client->ps.pm_type != PM_NORMAL ) {
		return;
	}
	if ( activator->client->ps.powerups[PW_FLIGHT] ) {
		return;
	}
if(self->locked != 0){
return;
}

	VectorCopy (self->s.origin2, activator->client->ps.velocity);

	// play fly sound every 1.5 seconds
	if ( activator->fly_sound_debounce_time < level.time ) {
		activator->fly_sound_debounce_time = level.time + 1500;
		G_Sound( activator, CHAN_AUTO, self->noise_index );
	}
}

/*QUAKED target_push (.5 .5 .5) (-8 -8 -8) (8 8 8) bouncepad
Pushes the activator in the direction.of angle, or towards a target apex.
"speed"		defaults to 1000
if "bouncepad", play bounce noise instead of windfly
*/
void SP_target_push( gentity_t *self ) {
	if (!self->speed) {
		self->speed = 1000;
	}
	G_SetMovedir (self->s.angles, self->s.origin2);
	VectorScale (self->s.origin2, self->speed, self->s.origin2);

	if ( self->spawnflags & 1 ) {
		self->noise_index = G_SoundIndex("sound/world/jumppad.wav");
	} else if (self->spawnflags & 2) {
		self->noise_index = G_SoundIndex("*jump1.wav");
	} else {
		self->noise_index = G_SoundIndex("sound/misc/windfly.wav");
	}
	if ( self->target ) {
		VectorCopy( self->s.origin, self->r.absmin );
		VectorCopy( self->s.origin, self->r.absmax );
		self->think = AimAtTarget;
		self->nextthink = level.time + FRAMETIME;
	}
	self->use = Use_target_push;
}

/*
==============================================================================

trigger_teleport

==============================================================================
*/

void trigger_teleporter_touch (gentity_t *self, gentity_t *other, trace_t *trace ) {
	gentity_t	*dest;
    vec3_t		origin, angles;
	vec3_t originDiff;
	vec3_t originDiffto;
	vec3_t anglesto;
	vec3_t destRelOrigin;

	if ( !other->client ) {
		return;
	}
if(self->locked != 0){
return;
}
	if ( other->client->ps.pm_type == PM_DEAD ) {
		return;
	}
	// Spectators only?
	/*if ( ( self->spawnflags & 1 ) && 
		(other->client->sess.sessionTeam != TEAM_SPECTATOR && other->client->ps.pm_type != PM_SPECTATOR) ) {
		return;
	}*/


	dest = 	G_PickTarget( self->target );
	if (!dest) {
                G_Printf ("Couldn't find teleporter destination\n");
		return;
	}

	if (self->spawnflags & 1) {
		VectorSubtract(self->s.origin, other->client->ps.origin, originDiff);
		if (self->spawnflags & 2) {
			VectorCopy(originDiff, originDiffto);
			originDiff[0] = originDiffto[1];
			originDiff[1] = originDiffto[0];
		}
		if (self->spawnflags & 4) {
			VectorCopy(originDiff, originDiffto);
			originDiff[0] = -originDiffto[0];
			originDiff[1] = -originDiffto[1];
		}
		if (self->spawnflags & 8) {
			VectorCopy(originDiff, originDiffto);
			originDiff[0] = -originDiffto[1];
			originDiff[1] = -originDiffto[0];
		}
		VectorSubtract(dest->s.origin, originDiff, destRelOrigin);

		/*
		G_Printf("self->s.origin: %s\n", vtos(self->s.origin));
		G_Printf("dest->s.origin: %s\n", vtos(dest->s.origin));
		G_Printf("originDiff: %s\n", vtos(originDiff));
		G_Printf("destRelOrigin: %s\n", vtos(destRelOrigin));
		*/
		if (self->spawnflags & 2) {
		VectorCopy(other->client->ps.viewangles, anglesto);
		anglesto[1] += self->playerangle;
		TeleportPlayerNoKnockback(other, destRelOrigin, anglesto, 90);
		} else if (self->spawnflags & 4) {
		VectorCopy(other->client->ps.viewangles, anglesto);
		anglesto[1] += self->playerangle;
		TeleportPlayerNoKnockback(other, destRelOrigin, anglesto, 180);
		} else if (self->spawnflags & 8) {
		VectorCopy(other->client->ps.viewangles, anglesto);
		anglesto[1] += self->playerangle;
		TeleportPlayerNoKnockback(other, destRelOrigin, anglesto, 270);
		} else {
		VectorCopy(other->client->ps.viewangles, anglesto);
		anglesto[1] += self->playerangle;
		TeleportPlayerNoKnockback(other, destRelOrigin, anglesto, 0);	
		}
	} else {
    if ( g_randomteleport.integer ) {
        SelectSpawnPoint ( other->client->ps.origin, origin, angles );
        TeleportPlayer( other, origin, angles );
	    return;
	} else {
	TeleportPlayer( other, dest->s.origin, dest->s.angles );
	}
}
}


/*QUAKED trigger_teleport (.5 .5 .5) ? SPECTATOR
Allows client side prediction of teleportation events.
Must point at a target_position, which will be the teleport destination.

If spectator is set, only spectators can use this teleport
Spectator teleporters are not normally placed in the editor, but are created
automatically near doors to allow spectators to move through them
*/
void SP_trigger_teleport( gentity_t *self ) {
	InitTrigger (self);

	// unlike other triggers, we need to send this one to the client
	// unless is a spectator trigger
	if ( self->spawnflags & 1 ) {
		self->r.svFlags |= SVF_NOCLIENT;
	} else {
	self->r.svFlags = SVF_NOCLIENT;
	}

	// make sure the client precaches this sound
	G_SoundIndex("sound/world/jumppad.wav");

	self->s.eType = ET_TELEPORT_TRIGGER;
	self->touch = trigger_teleporter_touch;

	trap_LinkEntity (self);
}


/*
==============================================================================

trigger_hurt

==============================================================================
*/

/*QUAKED trigger_hurt (.5 .5 .5) ? START_OFF - SILENT NO_PROTECTION SLOW
Any entity that touches this will be hurt.
It does dmg points of damage each server frame
Targeting the trigger will toggle its on / off state.

SILENT			supresses playing the sound
SLOW			changes the damage rate to once per second
NO_PROTECTION	*nothing* stops the damage

"dmg"			default 5 (whole numbers only)

*/
void hurt_use( gentity_t *self, gentity_t *other, gentity_t *activator ) {
	if ( self->r.linked ) {
		trap_UnlinkEntity( self );
	} else {
		trap_LinkEntity( self );
	}
}

void hurt_touch( gentity_t *self, gentity_t *other, trace_t *trace ) {
	int		dflags;

	if ( !other->takedamage ) {
		return;
	}

	if ( self->timestamp > level.time ) {
		return;
	}

	if ( self->spawnflags & 16 ) {
		self->timestamp = level.time + 1000;
	} else {
		self->timestamp = level.time + FRAMETIME;
	}

	// play sound
	if ( !(self->spawnflags & 4) ) {
		G_Sound( other, CHAN_AUTO, self->noise_index );
	}

	if (self->spawnflags & 8)
		dflags = DAMAGE_NO_PROTECTION;
	else
		dflags = 0;
	G_Damage (other, self, self, NULL, NULL, self->damage, dflags, MOD_TRIGGER_HURT);
}

void SP_trigger_hurt( gentity_t *self ) {
	InitTrigger (self);

	self->noise_index = G_SoundIndex( "sound/world/electro.wav" );
	self->touch = hurt_touch;

	if ( !self->damage ) {
		self->damage = 5;
	}

	self->r.contents = CONTENTS_TRIGGER;

	if ( self->spawnflags & 2 ) {
	self->use = hurt_use;
        }

	// link in to the world if starting active
	if ( self->spawnflags & 1 ) {
            trap_UnlinkEntity (self);
        }
        else
        {
		trap_LinkEntity (self);
	}
}


/*
==============================================================================

timer

==============================================================================
*/


/*QUAKED func_timer (0.3 0.1 0.6) (-8 -8 -8) (8 8 8) START_ON START_DELAYED
This should be renamed trigger_timer...
Repeatedly fires its targets.
Can be turned on or off by using.

"wait"			base time between triggering all targets, default is 1
"random"		wait variance, default is 0
so, the basic time between firing is a random time between
(wait - random) and (wait + random)
START_DELAYED	When entity is turned on, the timer will activate its target after the wait period instead of immediately

*/
void func_timer_think( gentity_t *self ) {
	G_UseTargets (self, self->activator);

	// increase timer's damage value to indicate it has been used once more
	if ( self->count && self->damage < self->count )
		self->damage++;

	// set time before next firing
	if ( !self->count || self->damage < self->count )
	self->nextthink = level.time + 1000 * ( self->wait + crandom() * self->random );
	else {
		//timer has activated its targets [count] number of times, so turn it off and reset its trigger counter (damage)
		self->nextthink = 0;	
		self->damage = 0;
	}
}

void func_timer_use( gentity_t *self, gentity_t *other, gentity_t *activator ) {
	self->activator = activator;

	// if on, turn it off
	if ( self->nextthink ) {
		self->nextthink = 0;
		return;
	}

	// turn it on
	if ( self->spawnflags & 2 )
		self->nextthink = level.time + 1000 * ( self->wait + crandom() * self->random );
	else
	func_timer_think (self);
}

void SP_func_timer( gentity_t *self ) {
	G_SpawnFloat( "random", "0", &self->random);
	G_SpawnFloat( "wait", "1", &self->wait );

	self->use = func_timer_use;
	self->think = func_timer_think;

	if ( self->random >= self->wait ) {
		self->random = self->wait - FRAMETIME;
                G_Printf( "func_timer at %s has random >= wait\n", vtos( self->s.origin ) );
	}

	if ( self->spawnflags & 1 ) {
		if (self->spawnflags & 2)
			self->nextthink = level.time + 1000 * (self->wait + crandom() * self->random);
		else
		self->nextthink = level.time + FRAMETIME;
		self->activator = self;
	}

	G_SpawnInt( "count", "0", &self->count);
	self->damage = 0; //damage is used to keep track of the number of times this timer has activated its targets.

	self->r.svFlags = SVF_NOCLIENT;
}


/*
==============================================================================

trigger_death

==============================================================================
*/

/*QUAKED trigger_death(.5 .5 .5) (-8 -8 -8) (8 8 8) RED_ONLY BLUE_ONLY TRIGGER_ONCE
Entity that's triggered when a player dies.
activator is the player that died. other is unused.
If the TRIGGER_ONCE spawnflag is set, the entity can only be triggered once
*/
void trigger_death_use( gentity_t *self, gentity_t *other, gentity_t *activator ) {
	
	//filter red/blue players
	if ( ( self->spawnflags & 1 ) && activator->client->sess.sessionTeam != TEAM_RED ) {
		return;
	}
	if ( ( self->spawnflags & 2 ) && activator->client->sess.sessionTeam != TEAM_BLUE ) {
		return;
	}

	// if died player is a bot and bots aren't allowed, do nothing
	if ( ( self->flags & FL_NO_BOTS ) && ( activator->r.svFlags & SVF_BOT ) )
		return;

	// if died player is not a bot and humans aren't allowed, do nothing
	if ( ( self->flags & FL_NO_HUMANS ) && !( activator->r.svFlags & SVF_BOT ) )
		return;

	//damage is used to keep track of the number of times a player died
	self->damage++;

	if ( self->damage < self->count )
		return;

	self->damage = 0;
	
	G_UseTargets ( self, activator );

	//the entity is triggered and TRIGGER_ONCE is set, so remove the entity from the game.
	if ( ( self->spawnflags & 4 ) )
		G_FreeEntity( self );
}


void SP_trigger_death( gentity_t *self ) {
	int		i;

	self->use = trigger_death_use;

	G_SpawnInt( "nobots", "0", &i);
	if ( i ) {
		self->flags |= FL_NO_BOTS;
	}
	G_SpawnInt( "nohumans", "0", &i );
	if ( i ) {
		self->flags |= FL_NO_HUMANS;
	}

	G_SpawnInt( "count", "1", &self->count );	//count is the number of times a player must die before the entity is triggered
	self->damage = 0;							//damage is used to keep track of the number of times a player died

	self->r.svFlags = SVF_NOCLIENT;
}

/*
==============================================================================

trigger_frag

==============================================================================
*/

/*QUAKED trigger_frag(.5 .5 .5) (-8 -8 -8) (8 8 8) RED_ONLY BLUE_ONLY TRIGGER_ONCE NO_SUICIDE
Entity that's triggered when a makes a frag.
activator is the entity that is responsible for the death of another player. other is the player that died.
If the TRIGGER_ONCE spawnflag is set, the entity can only be triggered once
*/
void trigger_frag_use( gentity_t *self, gentity_t *other, gentity_t *activator ) {
	
	// cannot be triggered by non-player entities
	if ( !activator->client )
		return;

	// the NO_SUICIDE spawnflag is set and player killed himself so we do nothing
	if ( ( self->spawnflags & 8 ) && activator == other )
		return;

	//filter red/blue players
	if ( ( self->spawnflags & 1 ) && activator->client->sess.sessionTeam != TEAM_RED ) {
		return;
	}
	if ( ( self->spawnflags & 2 ) && activator->client->sess.sessionTeam != TEAM_BLUE ) {
		return;
	}

	// if player scoring a frag is a bot and bots aren't allowed, do nothing
	if ( ( self->flags & FL_NO_BOTS ) && ( activator->r.svFlags & SVF_BOT ) )
		return;

	// if player scoring a frag is not a bot and humans aren't allowed, do nothing
	if ( ( self->flags & FL_NO_HUMANS ) && !( activator->r.svFlags & SVF_BOT ) )
		return;

	//damage is used to keep track of the number of times a frag was made
	self->damage++;

	if ( self->damage < self->count )
		return;
	
	self->damage = 0;
	
	G_UseTargets ( self, activator );

	//the entity is triggered and TRIGGER_ONCE is set, so remove the entity from the game.
	if ( ( self->spawnflags & 4 ) )
		G_FreeEntity( self );
}


void SP_trigger_frag( gentity_t *self ) {
	int		i;

	self->use = trigger_frag_use;

	G_SpawnInt( "nobots", "0", &i);
	if ( i ) {
		self->flags |= FL_NO_BOTS;
	}
	G_SpawnInt( "nohumans", "0", &i );
	if ( i ) {
		self->flags |= FL_NO_HUMANS;
	}

	G_SpawnInt( "count", "1", &self->count );	//count is the number of times a frag must be scored before the entity is triggered
	self->damage = 0;							//damage is used to keep track of the number of times a frag was scored

	self->r.svFlags = SVF_NOCLIENT;
}
/*
==============================================================================

EntityPlus: trigger_lock
Note: If NONE of the trigger_lock's KEY_* spawnflags have been set, it operates
like an ordinary trigger_multiple
==============================================================================
*/

void lock_touch(gentity_t *self, gentity_t *other, trace_t *trace) {
	vec3_t size;
	int holdables;
	qboolean playerHasKeys;
	
	if (!other->client)
		return;

	if ( self->nextthink )
		return;

	holdables = other->client->ps.stats[STAT_HOLDABLE_ITEM];
	playerHasKeys = qtrue;

	//if player doesn't have all the required key(card)s, do nothing
	if ( (self->spawnflags & 4) && !(holdables & (1 << HI_KEY_RED)) ) 
		playerHasKeys = qfalse;
	if ( (self->spawnflags & 8) && !(holdables & (1 << HI_KEY_GREEN)) )
		playerHasKeys = qfalse;
	if ( (self->spawnflags & 16) && !(holdables & (1 << HI_KEY_BLUE)) )
		playerHasKeys = qfalse;
	if ( (self->spawnflags & 32) && !(holdables & (1 << HI_KEY_YELLOW)) )
		playerHasKeys = qfalse;
	if ( (self->spawnflags & 64) && !(holdables & (1 << HI_KEY_MASTER)) )
		playerHasKeys = qfalse;
	if ( (self->spawnflags & 128) && !(holdables & (1 << HI_KEY_GOLD)) )
		playerHasKeys = qfalse;
	if ( (self->spawnflags & 256) && !(holdables & (1 << HI_KEY_SILVER)) )
		playerHasKeys = qfalse;
	if ( (self->spawnflags & 512) && !(holdables & (1 << HI_KEY_IRON)) )
		playerHasKeys = qfalse;

	if ( !playerHasKeys ) {
		if ( self->message )
			trap_SendServerCommand( other-g_entities, va("cp \"%s\"", self->message ));

		// sound played when locked
		if ( self->soundPos1 )
		{
			vec3_t size, center;
			gentity_t *tent;

			VectorSubtract(self->r.maxs, self->r.mins, size);
			VectorScale(size, 0.5, size);
			VectorAdd(self->r.mins, size, center);
			tent = G_TempEntity( center, EV_GENERAL_SOUND );
			tent->s.eventParm = self->soundPos1;
		}

		self->think = multi_wait;
		self->nextthink = level.time + ( self->wait + self->random * crandom() ) * 1000;
		return;
	}
	else {
		self->message = 0;	//remove the message so it's not displayed anymore once the lock is opened
		
		// sound played when unlocked
		if ( self->soundPos2 ) {
			vec3_t size, center;
			gentity_t *tent;

			VectorSubtract(self->r.maxs, self->r.mins, size);
			VectorScale(size, 0.5, size);
			VectorAdd(self->r.mins, size, center);
			tent = G_TempEntity( center, EV_GENERAL_SOUND );
			tent->s.eventParm = self->soundPos2;

			self->soundPos1 = 0;
			self->soundPos2 = 0;
		}
	}

	// remove the required key(card)s if KEEP_KEYS spawnflag isn't set
	if (!(self->spawnflags & 1024)) {
		if (self->spawnflags & 4)
			other->client->ps.stats[STAT_HOLDABLE_ITEM] -= (1 << HI_KEY_RED);
		if (self->spawnflags & 8)
			other->client->ps.stats[STAT_HOLDABLE_ITEM] -= (1 << HI_KEY_GREEN);
		if (self->spawnflags & 16)
			other->client->ps.stats[STAT_HOLDABLE_ITEM] -= (1 << HI_KEY_BLUE);
		if (self->spawnflags & 32)
			other->client->ps.stats[STAT_HOLDABLE_ITEM] -= (1 << HI_KEY_YELLOW);
		if (self->spawnflags & 64)
			other->client->ps.stats[STAT_HOLDABLE_ITEM] -= (1 << HI_KEY_MASTER);
		if (self->spawnflags & 128)
			other->client->ps.stats[STAT_HOLDABLE_ITEM] -= (1 << HI_KEY_GOLD);
		if (self->spawnflags & 256)
			other->client->ps.stats[STAT_HOLDABLE_ITEM] -= (1 << HI_KEY_SILVER);
		if (self->spawnflags & 512)
			other->client->ps.stats[STAT_HOLDABLE_ITEM] -= (1 << HI_KEY_IRON);
	}

	// everything else is the same as a trigger_multiple
	multi_trigger(self, other);
}

/*
QUAKED trigger_lock (.5 .5 .5) ? RED_ONLY BLUE_ONLY KEY_RED KEY_GREEN KEY_BLUE KEY_YELLOW
Used in conjunction with a holdable_key_* to grant/deny access to some entity
(e.g. a door).
Spawnflags determine which key is needed to trigger this lock
*/
void SP_trigger_lock(gentity_t *self) {
	char  *lockedsound;
	char  *unlockedsound;
	
	InitTrigger(self);

	// default values
	G_SpawnFloat("wait", "0.5", &self->wait);
	G_SpawnFloat("random", "0", &self->random);
	G_SpawnString("lockedsound", "", &lockedsound);
	G_SpawnString("unlockedsound", "", &unlockedsound);

	// sounds
	self->soundPos1 = G_SoundIndex(lockedsound);
	self->soundPos2 = G_SoundIndex(unlockedsound);

	// random cannot be larger than wait
	if ( self->random >= self->wait && self->wait >= 0 ) {
		self->random = self->wait - FRAMETIME;
		G_Printf("trigger_lock has random >= wait\n");
	}

	self->touch = lock_touch;
	//self->touch = Touch_Multi;

	
	trap_LinkEntity(self);
}

