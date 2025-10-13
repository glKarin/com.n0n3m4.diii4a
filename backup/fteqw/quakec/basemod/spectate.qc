// Spectator functions
// Added Aug11'97 by Zoid <zoid@idsoftware.com>
//
// These functions are called from the server if they exist.
// Note that Spectators only have one think since they movement code doesn't
// track them much.  Impulse commands work as usual, but don't call
// the regular ImpulseCommand handler in weapons.qc since Spectators don't
// have any weapons and things can explode.
//
//   --- Zoid.

#ifndef NETQUAKE

/*
===========
SpectatorConnect

called when a spectator connects to a server
============
*/
void() SpectatorConnect =
{
	bprint3 (PRINT_MEDIUM, "Spectator ", self.netname, " entered the game\n");

	self.goalentity = world; // used for impulse 1 below
};

/*
===========
SpectatorDisconnect

called when a spectator disconnects from a server
============
*/
void() SpectatorDisconnect =
{
	bprint3 (PRINT_MEDIUM, "Spectator ", self.netname, " left the game\n");
};

/*
================
SpectatorImpulseCommand

Called by SpectatorThink if the spectator entered an impulse
================
*/
void() SpectatorImpulseCommand =
{
	if (self.impulse == 1) {
		// teleport the spectator to the next spawn point
		// note that if the spectator is tracking, this doesn't do
		// much
		self.goalentity = find(self.goalentity, classname, "info_player_deathmatch");
		if (self.goalentity == world)
			self.goalentity = find(self.goalentity, classname, "info_player_deathmatch");
		if (self.goalentity != world) {
			setorigin(self, self.goalentity.origin);
			self.angles = self.goalentity.angles;
			self.fixangle = TRUE;           // turn this way immediately
		}
	}

	self.impulse = 0;
};

/*
================
SpectatorThink

Called every frame after physics are run
================
*/
void() SpectatorThink =
{
	// self.origin, etc contains spectator position, so you could
	// do some neat stuff here

	if (self.impulse)
		SpectatorImpulseCommand();
};

#endif

