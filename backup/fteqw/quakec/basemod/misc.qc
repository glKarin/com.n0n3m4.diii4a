
/*QUAKED info_null (0 0.5 0) (-4 -4 -4) (4 4 4)
Used as a positional target for spotlights, etc.
*/
var void() info_null = SUB_Remove;

/*QUAKED info_notnull (0 0.5 0) (-4 -4 -4) (4 4 4)
Used as a positional target for lightning.
*/
void() info_notnull =
{
};

//============================================================================

float START_OFF = 1;

void() light_use =
{
	if (self.spawnflags & START_OFF)
	{
		lightstyle(self.style, "m");
		self.spawnflags = self.spawnflags - START_OFF;
	}
	else
	{
		lightstyle(self.style, "a");
		self.spawnflags = self.spawnflags | START_OFF;
	}
};

/*QUAKED light (0 1 0) (-8 -8 -8) (8 8 8) START_OFF
Non-displayed light.
Default light value is 300
Default style is 0
If targeted, it will toggle between on or off.
*/
void() light =
{
	if (!self.targetname)
	{       // inert light
		remove(self);
		return;
	}
	
	if (self.style >= 32)
	{
		self.use = light_use;
		if (self.spawnflags & START_OFF)
			lightstyle(self.style, "a");
		else
			lightstyle(self.style, "m");
	}
};

/*QUAKED light_fluoro (0 1 0) (-8 -8 -8) (8 8 8) START_OFF
Non-displayed light.
Default light value is 300
Default style is 0
If targeted, it will toggle between on or off.
Makes steady fluorescent humming sound
*/
void() light_fluoro =
{
	if (self.style >= 32)
	{
		self.use = light_use;
		if (self.spawnflags & START_OFF)
			lightstyle(self.style, "a");
		else
			lightstyle(self.style, "m");
	}
	
	precache_sound ("ambience/fl_hum1.wav");
	ambientsound (self.origin, "ambience/fl_hum1.wav", 0.5, ATTN_STATIC);
};

/*QUAKED light_fluorospark (0 1 0) (-8 -8 -8) (8 8 8)
Non-displayed light.
Default light value is 300
Default style is 10
Makes sparking, broken fluorescent sound
*/
void() light_fluorospark =
{
	if (!self.style)
		self.style = 10;

	precache_sound ("ambience/buzz1.wav");
	ambientsound (self.origin, "ambience/buzz1.wav", 0.5, ATTN_STATIC);
};

/*QUAKED light_globe (0 1 0) (-8 -8 -8) (8 8 8)
Sphere globe light.
Default light value is 300
Default style is 0
*/
void() light_globe =
{
	precache_model ("progs/s_light.spr");
	setmodel (self, "progs/s_light.spr");
	self.alpha = 0.5;
	makestatic (self);
};

void() FireAmbient =
{
	precache_sound ("ambience/fire1.wav");
// attenuate fast
	ambientsound (self.origin, "ambience/fire1.wav", 0.5, ATTN_STATIC);
};

/*QUAKED light_torch_small_walltorch (0 .5 0) (-10 -10 -20) (10 10 20)
Short wall torch
Default light value is 200
Default style is 0
*/
void() light_torch_small_walltorch =
{
	precache_model ("progs/flame.mdl");
	setmodel (self, "progs/flame.mdl");
	FireAmbient ();
	makestatic (self);
};

/*QUAKED light_flame_large_yellow (0 1 0) (-10 -10 -12) (12 12 18)
Large yellow flame ball
*/
void() light_flame_large_yellow =
{
	precache_model ("progs/flame2.mdl");
	setmodel (self, "progs/flame2.mdl");
	self.frame = 1;
	self.alpha = 0.5;
	FireAmbient ();
	makestatic (self);
};

/*QUAKED light_flame_small_yellow (0 1 0) (-8 -8 -8) (8 8 8) START_OFF
Small yellow flame ball
*/
void() light_flame_small_yellow =
{
	precache_model ("progs/flame2.mdl");
	setmodel (self, "progs/flame2.mdl");
	self.alpha = 0.5;
	FireAmbient ();
	makestatic (self);
};

/*QUAKED light_flame_small_white (0 1 0) (-10 -10 -40) (10 10 40) START_OFF
Small white flame ball
*/
void() light_flame_small_white =
{
	precache_model ("progs/flame2.mdl");
	setmodel (self, "progs/flame2.mdl");
	self.alpha = 0.5;
	FireAmbient ();
	makestatic (self);
};

//============================================================================


/*QUAKED misc_fireball (0 .5 .8) (-8 -8 -8) (8 8 8)
Lava Balls
*/

void() fire_fly =
{
	local vector vel;

	vel_x = (random() * 100) - 50;
	vel_y = (random() * 100) - 50;
	vel_z = self.speed + (random() * 200);

	PRJ_FireProjectile (world, "progs/lavaball.mdl", self.origin, vel, PE_NONE, 20, MOD_FIREBALL, 5);
	newmis.movetype = MOVETYPE_BOUNCE;
	newmis.alpha = 0.9;
	
	self.nextthink = time + (random() * 5) + 3;
	self.think = fire_fly;
};

void() misc_fireball =
{
	
	precache_model ("progs/lavaball.mdl");
	self.classname = "fireball";
	self.nextthink = time + 0.1 + (random() * 4.9);
	self.think = fire_fly;
	if (!self.speed)
		self.speed = 1000;
};

//============================================================================


void() barrel_explode =
{
	self.takedamage = DAMAGE_NO;
	self.classname = "explo_box";
	// did say self.owner, self.enemy should be set by Killed function
	T_RadiusDamage (self, self.enemy, 160, 200, self, MOD_EXPLOBOX);
	TE_explosion(self.origin + '0 0 32');
	remove (self);
};



/*QUAKED misc_explobox (0 .5 .8) (0 0 0) (32 32 64)
TESTING THING
*/

void() misc_explobox =
{
	local float     oldz;
	
	self.solid = SOLID_BBOX;
	self.movetype = MOVETYPE_NONE;
	precache_model ("maps/b_explob.bsp");
	setmodel (self, "maps/b_explob.bsp");
	setsize (self, '0 0 0', '32 32 64');
	precache_sound ("weapons/r_exp3.wav");
	self.health = 20;
	self.th_die = barrel_explode;
	self.takedamage = DAMAGE_AIM;

	self.origin_z = self.origin_z + 2;
	oldz = self.origin_z;
	droptofloor();
	if (oldz - self.origin_z > 250)
	{
		dprint ("item fell out of level at ");
		dprint (vtos(self.origin));
		dprint ("\n");
		remove(self);
	}
};




/*QUAKED misc_explobox2 (0 .5 .8) (0 0 0) (32 32 64)
Smaller exploding box, REGISTERED ONLY
*/

void() misc_explobox2 =
{
	local float     oldz;
	
	self.solid = SOLID_BBOX;
	self.movetype = MOVETYPE_NONE;
	precache_model2 ("maps/b_exbox2.bsp");
	setmodel (self, "maps/b_exbox2.bsp");
	setsize (self, '0 0 0', '32 32 32');
	precache_sound ("weapons/r_exp3.wav");
	self.health = 20;
	self.th_die = barrel_explode;
	self.takedamage = DAMAGE_AIM;

	self.origin_z = self.origin_z + 2;
	oldz = self.origin_z;
	droptofloor();
	if (oldz - self.origin_z > 250)
	{
		dprint ("item fell out of level at ");
		dprint (vtos(self.origin));
		dprint ("\n");
		remove(self);
	}
};

//============================================================================

float SPAWNFLAG_SUPERSPIKE      = 1;
float SPAWNFLAG_LASER = 2;

void() spikeshooter_use =
{
	if (self.spawnflags & SPAWNFLAG_LASER)
	{
		sound (self, CHAN_VOICE, "enforcer/enfire.wav", 1, ATTN_NORM);
		PRJ_FireProjectile(self,
			"progs/laser.mdl",
			self.origin,
			self.movedir * 600,
			PE_LASER,
			15,
			MOD_LASER,
			5);
		newmis.effects |= EF_DIMLIGHT;
		newmis.alpha = 0.5;
	}
	else
	{
		sound (self, CHAN_VOICE, "weapons/spike2.wav", 1, ATTN_NORM);
		if (self.spawnflags & SPAWNFLAG_SUPERSPIKE)
		{
			PRJ_FireProjectile(self,
				"progs/s_spike.mdl",
				self.origin,
				self.movedir * 500,
				PE_SUPERSPIKE,
				18,
				MOD_SUPERSPIKE,
				6);
		}
		else
		{
			PRJ_FireProjectile(self,
				"progs/spike.mdl",
				self.origin,
				self.movedir * 500,
				PE_SPIKE,
				9,
				MOD_SPIKE,
				6);
		}
	}
};

void() shooter_think =
{
	spikeshooter_use ();
	self.nextthink = time + self.wait;
	newmis.velocity = self.movedir * 500;
};


/*QUAKED trap_spikeshooter (0 .5 .8) (-8 -8 -8) (8 8 8) superspike laser
When triggered, fires a spike in the direction set in QuakeEd.
Laser is only for REGISTERED.
*/

void() trap_spikeshooter =
{
	SetMovedir ();
	self.use = spikeshooter_use;
	if (self.spawnflags & SPAWNFLAG_LASER)
	{
		precache_model2 ("progs/laser.mdl");
		
		precache_sound2 ("enforcer/enfire.wav");
		precache_sound2 ("enforcer/enfstop.wav");
	}
	else
		precache_sound ("weapons/spike2.wav");
};


/*QUAKED trap_shooter (0 .5 .8) (-8 -8 -8) (8 8 8) superspike laser
Continuously fires spikes.
"wait" time between spike (1.0 default)
"nextthink" delay before firing first spike, so multiple shooters can be stagered.
*/
void() trap_shooter =
{
	trap_spikeshooter ();
	
	if (self.wait == 0)
		self.wait = 1;
	self.nextthink = time + self.wait;
	self.think = shooter_think;
};



/*
===============================================================================


===============================================================================
*/

void() make_bubbles;
void() bubble_bob;

/*QUAKED air_bubbles (0 .5 .8) (-8 -8 -8) (8 8 8)

testing air bubbles
*/

noref var void() air_bubbles = SUB_Remove;

void() make_bubbles =
{
local entity    bubble;

	bubble = spawn();
	setmodel (bubble, "progs/s_bubble.spr");
	setorigin (bubble, self.origin);
	bubble.movetype = MOVETYPE_NOCLIP;
	bubble.solid = SOLID_NOT;
	bubble.velocity = '0 0 15';
	bubble.nextthink = time + 0.5;
	bubble.think = bubble_bob;
	bubble.classname = "bubble";
	bubble.frame = 0;
	bubble.bubble_state = 0;
	bubble.alpha = 0.4;
	setsize (bubble, '-8 -8 -8', '8 8 8');
	self.nextthink = time + random() + 0.5;
	self.think = make_bubbles;
};

void() bubble_split =
{
local entity    bubble;
	bubble = spawn();
	setmodel (bubble, "progs/s_bubble.spr");
	setorigin (bubble, self.origin);
	bubble.movetype = MOVETYPE_NOCLIP;
	bubble.solid = SOLID_NOT;
	bubble.velocity = self.velocity;
	bubble.nextthink = time + 0.5;
	bubble.think = bubble_bob;
	bubble.classname = "bubble";
	bubble.frame = 1;
	bubble.bubble_state = 10;
	setsize (bubble, '-8 -8 -8', '8 8 8');
	self.frame = 1;
	self.bubble_state = 10;
	if (self.waterlevel != 3)
		remove (self);
};

void() bubble_bob =
{
local float             rnd1, rnd2, rnd3;

	self.bubble_state = self.bubble_state + 1;
	if (self.bubble_state == 4)
		bubble_split();
	if (self.bubble_state == 20)
		remove(self);

	rnd1 = self.velocity_x + (-10 + (random() * 20));
	rnd2 = self.velocity_y + (-10 + (random() * 20));
	rnd3 = self.velocity_z + 10 + random() * 10;

	if (rnd1 > 10)
		rnd1 = 5;
	if (rnd1 < -10)
		rnd1 = -5;
		
	if (rnd2 > 10)
		rnd2 = 5;
	if (rnd2 < -10)
		rnd2 = -5;
		
	if (rnd3 < 10)
		rnd3 = 15;
	if (rnd3 > 30)
		rnd3 = 25;
	
	self.velocity_x = rnd1;
	self.velocity_y = rnd2;
	self.velocity_z = rnd3;
		
	self.nextthink = time + 0.5;
	self.think = bubble_bob;
};

/*~>~<~>~<~>~<~>~<~>~<~>~<~>~<~>~<~>~<~>~<~>~<~>~<~>~<~>~<~>~<~>~<~>~<~>
~>~<~>~<~>~<~>~<~>~<~>~<~>~<~>~<~>~<~>~<~>~<~>~<~>~<~>~<~>~<~>~<~>~<~>~*/

/*QUAKED viewthing (0 .5 .8) (-8 -8 -8) (8 8 8)

Just for the debugging level.  Don't use
*/

noref var void() viewthing = SUB_Remove;

/*
==============================================================================

SIMPLE BMODELS

==============================================================================
*/

void() func_wall_use =
{       // change to alternate textures
	self.frame = 1 - self.frame;
};

/*QUAKED func_wall (0 .5 .8) ?
This is just a solid wall if not inhibitted
*/
void() func_wall =
{
	self.angles = '0 0 0';
	self.movetype = MOVETYPE_PUSH;  // so it doesn't get pushed by anything
	self.solid = SOLID_BSP;
	self.use = func_wall_use;
	setmodel (self, self.model);
};


/*QUAKED func_illusionary (0 .5 .8) ?
A simple entity that looks solid but lets you walk through it.
*/
void() func_illusionary =
{
	self.angles = '0 0 0';
	self.movetype = MOVETYPE_NONE;
	self.solid = SOLID_NOT;
	setmodel (self, self.model);
	makestatic (self);
};

/*QUAKED func_episodegate (0 .5 .8) ? E1 E2 E3 E4
This bmodel will appear if the episode has allready been completed, so players can't reenter it.
*/
void() func_episodegate =
{
	if (!(serverflags & self.spawnflags))
		return;                 // can still enter episode

	self.angles = '0 0 0';
	self.movetype = MOVETYPE_PUSH;  // so it doesn't get pushed by anything
	self.solid = SOLID_BSP;
	self.use = func_wall_use;
	setmodel (self, self.model);
};

/*QUAKED func_bossgate (0 .5 .8) ?
This bmodel appears unless players have all of the episode sigils.
*/
void() func_bossgate =
{
	if ( (serverflags & 15) == 15)
	{
		remove(self);
		return;         // all episodes completed
	}
	self.angles = '0 0 0';
	self.movetype = MOVETYPE_PUSH;  // so it doesn't get pushed by anything
	self.solid = SOLID_BSP;
	self.use = func_wall_use;
	setmodel (self, self.model);
};

//============================================================================
/*QUAKED ambient_suck_wind (0.3 0.1 0.6) (-10 -10 -8) (10 10 8)
*/
void() ambient_suck_wind =
{
	precache_sound ("ambience/suck1.wav");
	ambientsound (self.origin, "ambience/suck1.wav", 1, ATTN_STATIC);
};

/*QUAKED ambient_drone (0.3 0.1 0.6) (-10 -10 -8) (10 10 8)
*/
void() ambient_drone =
{
	precache_sound ("ambience/drone6.wav");
	ambientsound (self.origin, "ambience/drone6.wav", 0.5, ATTN_STATIC);
};

/*QUAKED ambient_flouro_buzz (0.3 0.1 0.6) (-10 -10 -8) (10 10 8)
*/
void() ambient_flouro_buzz =
{
	precache_sound ("ambience/buzz1.wav");
	ambientsound (self.origin, "ambience/buzz1.wav", 1, ATTN_STATIC);
};
/*QUAKED ambient_drip (0.3 0.1 0.6) (-10 -10 -8) (10 10 8)
*/
void() ambient_drip =
{
	precache_sound ("ambience/drip1.wav");
	ambientsound (self.origin, "ambience/drip1.wav", 0.5, ATTN_STATIC);
};
/*QUAKED ambient_comp_hum (0.3 0.1 0.6) (-10 -10 -8) (10 10 8)
*/
void() ambient_comp_hum =
{
	precache_sound ("ambience/comp1.wav");
	ambientsound (self.origin, "ambience/comp1.wav", 1, ATTN_STATIC);
};
/*QUAKED ambient_thunder (0.3 0.1 0.6) (-10 -10 -8) (10 10 8)
*/
void() ambient_thunder =
{
	precache_sound ("ambience/thunder1.wav");
	ambientsound (self.origin, "ambience/thunder1.wav", 0.5, ATTN_STATIC);
};
/*QUAKED ambient_light_buzz (0.3 0.1 0.6) (-10 -10 -8) (10 10 8)
*/
void() ambient_light_buzz =
{
	precache_sound ("ambience/fl_hum1.wav");
	ambientsound (self.origin, "ambience/fl_hum1.wav", 0.5, ATTN_STATIC);
};
/*QUAKED ambient_swamp1 (0.3 0.1 0.6) (-10 -10 -8) (10 10 8)
*/
void() ambient_swamp1 =
{
	precache_sound ("ambience/swamp1.wav");
	ambientsound (self.origin, "ambience/swamp1.wav", 0.5, ATTN_STATIC);
};
/*QUAKED ambient_swamp2 (0.3 0.1 0.6) (-10 -10 -8) (10 10 8)
*/
void() ambient_swamp2 =
{
	precache_sound ("ambience/swamp2.wav");
	ambientsound (self.origin, "ambience/swamp2.wav", 0.5, ATTN_STATIC);
};
 