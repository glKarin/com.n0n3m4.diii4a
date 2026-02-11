/* ALL LIGHTS SHOULD BE 0 1 0 IN COLOR ALL OTHER ITEMS SHOULD
BE .8 .3 .4 IN COLOR */

void (float weap) W_WeaponSwitch;
void (entity ent) W_UpdateAmmoCounts;

void() SUB_regen =
{
	self.model = self.mdl;          // restore original model
	self.solid = SOLID_TRIGGER;     // allow it to be touched again
	sound (self, CHAN_VOICE, "items/itembk2.wav", 1, ATTN_NORM);    // play respawn sound
	setorigin (self, self.origin);
};



/*QUAKED noclass (0 0 0) (-8 -8 -8) (8 8 8)
prints a warning message when spawned
*/
void() noclass =
{
	dprint ("noclass spawned at");
	dprint (vtos(self.origin));
	dprint ("\n");
	remove (self);
};

// names of weapons function
string(float weap) I_WeaponName =
{
	switch (weap)
	{
	case IT_AXE:
		return "Axe";
	case IT_SHOTGUN:
		return "Shotgun";
	case IT_SUPER_SHOTGUN:
		return "Double-barrelled Shotgun";
	case IT_NAILGUN:
		return "Nailgun";
	case IT_SUPER_NAILGUN:
		return "Super Nailgun";
	case IT_GRENADE_LAUNCHER:
		return "Grenade Launcher";
	case IT_ROCKET_LAUNCHER:
		return "Rocket Launcher";
	case IT_LIGHTNING:
		return "Thunderbolt";
	}

	return "";
}

void() q_touch;

void() q_touch =
{
	local string s, t;

	if (other.classname != "player")
		return;
	if (other.health <= 0)
		return;

	self.mdl = self.model;

	sound (other, CHAN_VOICE, self.noise, 1, ATTN_NORM);
	stuffcmd (other, "bf\n");
	self.solid = SOLID_NOT;
	other.items = other.items | IT_QUAD;
	self.model = string_null;
		if (deathmatch == 4)
		{
			other.armortype = 0;
			other.armorvalue = 0;
			other.ammo_cells_real = 0;
			W_UpdateAmmoCounts(other);
		}

// do the apropriate action
	other.super_time = 1;
	other.super_damage_finished = self.cnt;

	s=ftos(rint(other.super_damage_finished - time));
	t = " recovered a Quad with ";
	if (deathmatch == 4)
		t = " recovered an OctaPower with ";

	bprint4 (PRINT_LOW, other.netname, t, s, " seconds remaining!\n");

	activator = other;
	SUB_UseTargets();                               // fire all targets / killtargets
};


void(float timeleft) DropQuad =
{
	local entity    item;

	item = spawn();
	item.origin = self.origin;
	
	item.velocity_z = 300;
	item.velocity_x = -100 + (random() * 200);
	item.velocity_y = -100 + (random() * 200);
	
	item.flags = FL_ITEM;
	item.solid = SOLID_TRIGGER;
	item.movetype = MOVETYPE_TOSS;
	item.noise = "items/damage.wav";
	setmodel (item, "progs/quaddama.mdl");
	setsize (item, '-16 -16 -24', '16 16 32');
	item.cnt = time + timeleft;
	item.touch = q_touch;
	item.nextthink = time + timeleft;    // remove it with the time left on it
	item.think = SUB_Remove;
};


void() r_touch;

void() r_touch =
{
local string    s;

	if (other.classname != "player")
		return;
	if (other.health <= 0)
		return;

	self.mdl = self.model;

	sound (other, CHAN_VOICE, self.noise, 1, ATTN_NORM);
	stuffcmd (other, "bf\n");
	self.solid = SOLID_NOT;
	other.items = other.items | IT_INVISIBILITY;
	self.model = string_null;

// do the apropriate action
	other.invisible_time = 1;
	other.invisible_finished = self.cnt;
	s=ftos(rint(other.invisible_finished - time));
	bprint4 (PRINT_LOW, other.netname, " recovered a Ring with ", s, " seconds remaining!\n");
      

	activator = other;
	SUB_UseTargets();                               // fire all targets / killtargets
};


void(float timeleft) DropRing =
{
	local entity    item;

	item = spawn();
	item.origin = self.origin;
	
	item.velocity_z = 300;
	item.velocity_x = -100 + (random() * 200);
	item.velocity_y = -100 + (random() * 200);
	
	item.flags = FL_ITEM;
	item.solid = SOLID_TRIGGER;
	item.movetype = MOVETYPE_TOSS;
	item.noise = "items/inv1.wav";
	setmodel (item, "progs/invisibl.mdl");
	setsize (item, '-16 -16 -24', '16 16 32');
	item.cnt = time + timeleft;
	item.touch = r_touch;
	item.nextthink = time + timeleft;    // remove after 30 seconds
	item.think = SUB_Remove;
};

/*
============
PlaceItem

plants the object on the floor
============
*/
void() PlaceItem =
{
	local float     oldz;

	self.mdl = self.model;          // so it can be restored on respawn
	self.flags = FL_ITEM;           // make extra wide
	self.solid = SOLID_TRIGGER;
	self.movetype = MOVETYPE_TOSS;  
	self.velocity = '0 0 0';
	self.origin_z = self.origin_z + 6;
	oldz = self.origin_z;
	if (!droptofloor())
	{
		dprint ("Bonus item fell out of level at ");
		dprint (vtos(self.origin));
		dprint ("\n");
		remove(self);
		return;
	}
};

/*
============
StartItem

Sets the clipping size and plants the object on the floor
============
*/
void() StartItem =
{
	self.nextthink = time + 0.2;    // items start after other solids
	self.think = PlaceItem;
};

/*
=========================================================================

HEALTH BOX

=========================================================================
*/
//
// T_Heal: add health to an entity, limiting health to max_health
// "ignore" will ignore max_health limit
//
float (entity e, float hamount, float ignore) T_Heal =
{
	if (e.health <= 0)
		return 0;
	if ((!ignore) && (e.health >= other.max_health))
		return 0;
	hamount = ceil(hamount);

	e.health = e.health + hamount;
	if ((!ignore) && (e.health >= other.max_health))
		e.health = other.max_health;
	
		
	if (e.health > e.max_health + 150)
		e.health = e.max_health + 150;

	e.healdecay = time + 5;
	return 1;
};

/*QUAKED item_health (.3 .3 1) (0 0 0) (32 32 32) rotten megahealth
Health box. Normally gives 25 points.
Rotten box heals 5-10 points,
megahealth will add 100 health, then 
rot you down to your maximum health limit, 
one point per second.
*/

void() health_touch;

void() item_health =
{       
	self.touch = health_touch;

	if (self.spawnflags & H_ROTTEN)
	{
		precache_model("maps/b_bh10.bsp");

		precache_sound("items/r_item1.wav");
		setmodel(self, "maps/b_bh10.bsp");
		self.noise = "items/r_item1.wav";
		self.healamount = 15;
		self.healtype = 0;
	}
	else
	if (self.spawnflags & H_MEGA)
	{
		precache_model("maps/b_bh100.bsp");
		precache_sound("items/r_item2.wav");
		setmodel(self, "maps/b_bh100.bsp");
		self.noise = "items/r_item2.wav";
		self.healamount = 100;
		self.healtype = 2;
	}
	else
	{
		precache_model("maps/b_bh25.bsp");
		precache_sound("items/health1.wav");
		setmodel(self, "maps/b_bh25.bsp");
		self.noise = "items/health1.wav";
		self.healamount = 25;
		self.healtype = 1;
	}
	setsize (self, '0 0 0', '32 32 56');
	StartItem ();
};


void() health_touch =
{
	local   string  s;
	
	if (deathmatch == 4)
		if (other.invincible_time > time)
			return;

	if (other.classname != "player")
		return;
	
	if (self.healtype == H_MEGA) // Megahealth?  Ignore max_health...
	{
		if (other.health >= other.max_health + 150)
			return;
		if (!T_Heal(other, self.healamount, 1))
			return;
	}
	else
	{
		if (!T_Heal(other, self.healamount, 0))
			return;
	}
	
	s = ftos(self.healamount);
	sprint3(other, PRINT_LOW, "You receive ", s, " health\n");
	
// health touch sound
	sound(other, CHAN_ITEM, self.noise, 1, ATTN_NORM);

	stuffcmd (other, "bf\n");
	
	self.model = string_null;
	self.solid = SOLID_NOT;

	// Megahealth = rot down the player's super health
	if (deathmatch) // TODO: are these rules right?
	{
		if (deathmatch != 2)            // deathmatch 2 is the silly old rules
		{
			if (self.healtype == H_MEGA)
				self.nextthink = time + 45; // should I account for health healed instead?
			else
				self.nextthink = time + 20;
			self.think = SUB_regen;
		}
	}
	
	activator = other;
	SUB_UseTargets();                               // fire all targets / killtargets
};      

/*
===============================================================================

ARMOR

===============================================================================
*/

void() armor_touch;

void() armor_touch =
{
	local   float   type, value, bit;
	
	if (other.health <= 0)
		return;
	if (other.classname != "player")
		return;

	if (deathmatch == 4)

		if (other.invincible_time > 0)
			return;

	switch (self.classname)
	{
	case "item_armor1":
		type = 0.3;
		value = 100;
		bit = IT_ARMOR1;
		break;
	case "item_armor2":
		type = 0.6;
		value = 150;
		bit = IT_ARMOR2;
		break;
	case "item_armorInv":
		type = 0.8;
		value = 200;
		bit = IT_ARMOR3;
		break;
	}

	if (other.armortype*other.armorvalue >= type*value)
		return;
		
	other.armortype = type;
	other.armorvalue = value;
	other.items = other.items - (other.items & (IT_ARMOR1 | IT_ARMOR2 | IT_ARMOR3)) + bit;

	self.solid = SOLID_NOT;
	self.model = string_null;
	if (deathmatch && deathmatch != 2)
	{
		self.nextthink = time + 20;
		self.think = SUB_regen;
	}

	sprint1(other, PRINT_LOW, "You got armor\n");
// armor touch sound
	sound(other, CHAN_ITEM, "items/armor1.wav", 1, ATTN_NORM);
	stuffcmd (other, "bf\n");
	
	activator = other;
	SUB_UseTargets();                               // fire all targets / killtargets
};


/*QUAKED item_armor1 (0 .5 .8) (-16 -16 0) (16 16 32)
*/

void() item_armor1 =
{
	self.touch = armor_touch;
	precache_model ("progs/armor.mdl");
	setmodel (self, "progs/armor.mdl");
	self.skin = 0;
	setsize (self, '-16 -16 0', '16 16 56');
	StartItem ();
};

/*QUAKED item_armor2 (0 .5 .8) (-16 -16 0) (16 16 32)
*/

void() item_armor2 =
{
	self.touch = armor_touch;
	precache_model ("progs/armor.mdl");
	setmodel (self, "progs/armor.mdl");
	self.skin = 1;
	setsize (self, '-16 -16 0', '16 16 56');
	StartItem ();
};

/*QUAKED item_armorInv (0 .5 .8) (-16 -16 0) (16 16 32)
*/

void() item_armorInv =
{
	self.touch = armor_touch;
	precache_model ("progs/armor.mdl");
	setmodel (self, "progs/armor.mdl");
	self.skin = 2;
	setsize (self, '-16 -16 0', '16 16 56');
	StartItem ();
};

/*
===============================================================================

WEAPONS

===============================================================================
*/

void() bound_other_ammo =
{
	if (other.ammo_shells_real > 100)
		other.ammo_shells_real = 100;
	if (other.ammo_nails_real > 200)
		other.ammo_nails_real = 200;
	if (other.ammo_rockets_real > 100)
		other.ammo_rockets_real = 100;               
	if (other.ammo_cells_real > 100)
		other.ammo_cells_real = 100;         
};


float(float w) RankForWeapon =
{
	if (w == IT_LIGHTNING)	
		return 1;
	if (w == IT_ROCKET_LAUNCHER)
		return 2;
	if (w == IT_SUPER_NAILGUN)
		return 3;
	if (w == IT_GRENADE_LAUNCHER)
		return 4;
	if (w == IT_SUPER_SHOTGUN)
		return 5;
	if (w == IT_NAILGUN)
		return 6;
	return 7;
};

float (float w) WeaponCode =
{
	if (w == IT_SUPER_SHOTGUN)
		return 3;
	if (w == IT_NAILGUN)
		return 4;
	if (w == IT_SUPER_NAILGUN)
		return 5;
	if (w == IT_GRENADE_LAUNCHER)
		return 6;
	if (w == IT_ROCKET_LAUNCHER)
		return 7;
	if (w == IT_LIGHTNING)
		return 8;
	return 1;
};

/*
=============
Deathmatch_Weapon

Deathmatch weapon change rules for picking up a weapon

.float          ammo_shells, ammo_nails, ammo_rockets, ammo_cells;
=============
*/
void(float old, float new) Deathmatch_Weapon =
{
	local float or, nr;

// change self.weapon if desired
	or = RankForWeapon (self.weapon);
	nr = RankForWeapon (new);

	if ( nr == IT_LIGHTNING && self.waterlevel > 1 )
		return;

	if ( nr < or )
		W_WeaponSwitch (new);
};

/*
=============
weapon_touch
=============
*/
float() W_BestWeapon;

void() weapon_touch =
{
	local   float   new, old;
	local   float   leave;
	local   entity  stemp;

	// For client weapon_switch
	local   float   w_switch;

	if (!(other.flags & FL_CLIENT))
		return;

	w_switch = numberclientinfokey(other,"w_switch");
	if (w_switch == 0)
		w_switch = 8;
	
	if (deathmatch == 2 || deathmatch == 3 || deathmatch == 5)
		leave = 1;
	else
		leave = 0;

	// check if we have the weapon already
	if (leave && (other.items & self.weapon))
		return;
	new = self.weapon;

	// add ammo here
	switch (new)
	{
	case IT_SUPER_SHOTGUN:		
		other.ammo_shells_real += 5;
		break;
	case IT_NAILGUN:
	case IT_SUPER_NAILGUN:
		other.ammo_nails_real += 30;
		break;
	case IT_GRENADE_LAUNCHER:
	case IT_ROCKET_LAUNCHER:
		other.ammo_rockets_real += 5;
		break;
	case IT_LIGHTNING:
		other.ammo_cells_real += 15;
		break;
	}
	
	sprint3 (other, PRINT_LOW, "You got the ", I_WeaponName(new), "\n");
// weapon touch sound
	sound (other, CHAN_ITEM, "weapons/pkup.wav", 1, ATTN_NORM);
	stuffcmd (other, "bf\n");

	bound_other_ammo ();
	W_UpdateAmmoCounts(other);

// change to the weapon
	old = other.items;
	other.items = other.items | new;
	
	stemp = self;
	self = other;

	if ( WeaponCode(new) <= w_switch )
	{
		if (self.flags & FL_INWATER)
		{
			if (new != IT_LIGHTNING)
			{
				Deathmatch_Weapon (old, new);
			}
		}
		else
		{                
			Deathmatch_Weapon (old, new);
		}
	}

	self = stemp;

	if (leave)
		return;

	if (deathmatch != 3 || deathmatch != 5)
	{
	// remove it in single player, or setup for respawning in deathmatch
		self.model = string_null;
		self.solid = SOLID_NOT;
		if (deathmatch && deathmatch != 2)
		{
			self.nextthink = time + 30;
			self.think = SUB_regen;
		}
	}
	activator = other;
	SUB_UseTargets();                               // fire all targets / killtargets
};


/*QUAKED weapon_supershotgun (0 .5 .8) (-16 -16 0) (16 16 32)
*/

void() weapon_supershotgun =
{
	if (deathmatch <= 3)
	{
		precache_model ("progs/g_shot.mdl");
		setmodel (self, "progs/g_shot.mdl");
		self.weapon = IT_SUPER_SHOTGUN;
		self.touch = weapon_touch;
		setsize (self, '-16 -16 0', '16 16 56');
		StartItem ();
	}
	else
		remove(self);
};

/*QUAKED weapon_nailgun (0 .5 .8) (-16 -16 0) (16 16 32)
*/

void() weapon_nailgun =
{
	if (deathmatch <= 3)
	{
		precache_model ("progs/g_nail.mdl");
		setmodel (self, "progs/g_nail.mdl");
		self.weapon = IT_NAILGUN;
		self.touch = weapon_touch;
		setsize (self, '-16 -16 0', '16 16 56');
		StartItem ();
	}
	else
		remove(self);
};

/*QUAKED weapon_supernailgun (0 .5 .8) (-16 -16 0) (16 16 32)
*/

void() weapon_supernailgun =
{
	if (deathmatch <= 3)
	{
		precache_model ("progs/g_nail2.mdl");
		setmodel (self, "progs/g_nail2.mdl");
		self.weapon = IT_SUPER_NAILGUN;
		self.touch = weapon_touch;
		setsize (self, '-16 -16 0', '16 16 56');
		StartItem ();
	}
	else
		remove(self);
};

/*QUAKED weapon_grenadelauncher (0 .5 .8) (-16 -16 0) (16 16 32)
*/

void() weapon_grenadelauncher =
{
	if (deathmatch <= 3)
	{
		precache_model ("progs/g_rock.mdl");
		setmodel (self, "progs/g_rock.mdl");
		self.weapon = IT_GRENADE_LAUNCHER;
		self.touch = weapon_touch;
		setsize (self, '-16 -16 0', '16 16 56');
		StartItem ();
	}
	else
		remove(self);
};

/*QUAKED weapon_rocketlauncher (0 .5 .8) (-16 -16 0) (16 16 32)
*/

void() weapon_rocketlauncher =
{
	if (deathmatch <= 3)
	{
		precache_model ("progs/g_rock2.mdl");
		setmodel (self, "progs/g_rock2.mdl");
		self.weapon = IT_ROCKET_LAUNCHER;
		self.touch = weapon_touch;
		setsize (self, '-16 -16 0', '16 16 56');
		StartItem ();
	}
	else
		remove(self);
};


/*QUAKED weapon_lightning (0 .5 .8) (-16 -16 0) (16 16 32)
*/

void() weapon_lightning =
{
	if (deathmatch <= 3)
	{
		precache_model ("progs/g_light.mdl");
		setmodel (self, "progs/g_light.mdl");
		self.weapon = IT_LIGHTNING;
		self.touch = weapon_touch;
		setsize (self, '-16 -16 0', '16 16 56');
		StartItem ();
	}
	else
		remove(self);
};


/*
===============================================================================

AMMO

===============================================================================
*/

void() ammo_touch =
{
local entity    stemp;
local float             best;

	if (other.classname != "player")
		return;
	if (other.health <= 0)
		return;

// if the player was using his best weapon, change up to the new one if better          
	stemp = self;
	self = other;
	best = W_BestWeapon();
	self = stemp;


	switch (self.weapon)
	{
	case 1: // shotgun
		if (other.ammo_shells_real >= 100)
			return;
		other.ammo_shells_real = other.ammo_shells_real + self.ammo_count;
		break;
	case 2: // spikes
		if (other.ammo_nails_real >= 200)
			return;
		other.ammo_nails_real = other.ammo_nails_real + self.ammo_count;
		break;
	case 3: // rockets
		if (other.ammo_rockets_real >= 100)
			return;
		other.ammo_rockets_real = other.ammo_rockets_real + self.ammo_count;
		break;
	case 4: // cells
		if (other.ammo_cells_real >= 100)
			return;
		other.ammo_cells_real = other.ammo_cells_real + self.ammo_count;
		break;
	}

	bound_other_ammo ();
	
	sprint3 (other, PRINT_LOW, "You got the ", self.netname, "\n");
// ammo touch sound
	sound (other, CHAN_ITEM, "weapons/lock4.wav", 1, ATTN_NORM);
	stuffcmd (other, "bf\n");

// change to a better weapon if appropriate

	stemp = self;
	self = other;

	if ( self.weapon == best )
		W_WeaponSwitch (W_BestWeapon ());
	else
		W_UpdateAmmoCounts (self);

	self = stemp;

// remove it in single player, or setup for respawning in deathmatch
	self.model = string_null;
	self.solid = SOLID_NOT;
	if (deathmatch)
	{
		if (deathmatch != 2)
			self.nextthink = time + 30;

		// Xian -- If playing in DM 3.0 mode, halve the time ammo respawns        
		if (deathmatch == 3 || deathmatch == 5)        
			self.nextthink = time + 15;

		self.think = SUB_regen;
	}

	activator = other;
	SUB_UseTargets();                               // fire all targets / killtargets
};




float WEAPON_BIG2 = 1;

/*QUAKED item_shells (0 .5 .8) (0 0 0) (32 32 32) big
*/

void() item_shells =
{
	if (deathmatch == 4)
	{
		remove(self);
		return;
	}

	self.touch = ammo_touch;

	if (self.spawnflags & WEAPON_BIG2)
	{
		precache_model ("maps/b_shell1.bsp");
		setmodel (self, "maps/b_shell1.bsp");
		self.ammo_count = 40;
	}
	else
	{
		precache_model ("maps/b_shell0.bsp");
		setmodel (self, "maps/b_shell0.bsp");
		self.ammo_count = 20;
	}
	self.weapon = 1;
	self.netname = "shells";
	setsize (self, '0 0 0', '32 32 56');
	StartItem ();
};

/*QUAKED item_spikes (0 .5 .8) (0 0 0) (32 32 32) big
*/

void() item_spikes =
{
	if (deathmatch == 4)
	{
		remove(self);
		return;
	}

	self.touch = ammo_touch;

	if (self.spawnflags & WEAPON_BIG2)
	{
		precache_model ("maps/b_nail1.bsp");
		setmodel (self, "maps/b_nail1.bsp");
		self.ammo_count = 50;
	}
	else
	{
		precache_model ("maps/b_nail0.bsp");
		setmodel (self, "maps/b_nail0.bsp");
		self.ammo_count = 25;
	}
	self.weapon = 2;
	self.netname = "nails";
	setsize (self, '0 0 0', '32 32 56');
	StartItem ();

};

/*QUAKED item_rockets (0 .5 .8) (0 0 0) (32 32 32) big
*/

void() item_rockets =
{
	if (deathmatch == 4)
	{
		remove(self);
		return;
	}

	self.touch = ammo_touch;

	if (self.spawnflags & WEAPON_BIG2)
	{
		precache_model ("maps/b_rock1.bsp");
		setmodel (self, "maps/b_rock1.bsp");
		self.ammo_count = 10;
	}
	else
	{
		precache_model ("maps/b_rock0.bsp");
		setmodel (self, "maps/b_rock0.bsp");
		self.ammo_count = 5;
	}
	self.weapon = 3;
	self.netname = "rockets";
	setsize (self, '0 0 0', '32 32 56');
	StartItem ();

};


/*QUAKED item_cells (0 .5 .8) (0 0 0) (32 32 32) big
*/

void() item_cells =
{
	if (deathmatch == 4)
	{
		remove(self);
		return;
	}

	self.touch = ammo_touch;

	if (self.spawnflags & WEAPON_BIG2)
	{
		precache_model ("maps/b_batt1.bsp");
		setmodel (self, "maps/b_batt1.bsp");
		self.ammo_count = 12;
	}
	else
	{
		precache_model ("maps/b_batt0.bsp");
		setmodel (self, "maps/b_batt0.bsp");
		self.ammo_count = 6;
	}
	self.weapon = 4;
	self.netname = "cells";
	setsize (self, '0 0 0', '32 32 56');
	StartItem ();

};


/*QUAKED item_weapon (0 .5 .8) (0 0 0) (32 32 32) shotgun rocket spikes big
DO NOT USE THIS!!!! IT WILL BE REMOVED!
*/

float WEAPON_SHOTGUN = 1;
float WEAPON_ROCKET = 2;
float WEAPON_SPIKES = 4;
float WEAPON_MASK = 7;
float WEAPON_BIG = 8;
void() item_weapon =
{
	local float weap;
		
	weap = self.spawnflags & WEAPON_MASK;
	self.touch = ammo_touch;

	self.spawnflags = 0;
	if (self.spawnflags & WEAPON_BIG)

		self.spawnflags |= WEAPON_BIG2;

	switch (weap)
	{
	case WEAPON_SHOTGUN:
		self.classname = "item_shells";
		item_shells();
		return;
	case WEAPON_SPIKES:
		self.classname = "item_spikes";
		item_spikes();
		return;
	case WEAPON_ROCKET:
		self.classname = "item_rockets";
		item_rockets();
	}
};


/*
===============================================================================

KEYS

===============================================================================
*/

void() key_touch =
{
	if (other.classname != "player")
		return;
	if (other.health <= 0)
		return;
	if (other.items & self.items)
		return;

	sprint3 (other, PRINT_LOW, "You got the ", self.netname, "\n");

	sound (other, CHAN_ITEM, self.noise, 1, ATTN_NORM);
	stuffcmd (other, "bf\n");
	other.items = other.items | self.items;

	self.solid = SOLID_NOT;
	self.model = string_null;

	activator = other;
	SUB_UseTargets();                               // fire all targets / killtargets
};


void() key_setsounds =
{
	switch (world.worldtype)
	{
	case WT_MEDIEVAL:
		precache_sound ("misc/medkey.wav");
		self.noise = "misc/medkey.wav";
		break;
	case WT_METAL:
		precache_sound ("misc/runekey.wav");
		self.noise = "misc/runekey.wav";
		break;
	case WT_BASE:
		precache_sound2 ("misc/basekey.wav");
		self.noise = "misc/basekey.wav";
		break;
	}
};

/*QUAKED item_key1 (0 .5 .8) (-16 -16 -24) (16 16 32)
SILVER key
In order for keys to work
you MUST set your maps
worldtype to one of the
following:
0: medieval
1: metal
2: base
*/

void() item_key1 =
{
	if (deathmatch)
	{
		remove(self);
		return;
	}

	switch (world.worldtype)
	{
	case WT_MEDIEVAL:
		precache_model ("progs/w_s_key.mdl");
		setmodel (self, "progs/w_s_key.mdl");
		self.netname = "silver key";
		break;
	case WT_METAL:
		precache_model ("progs/m_s_key.mdl");
		setmodel (self, "progs/m_s_key.mdl");
		self.netname = "silver runekey";
		break;
	case WT_BASE:
		precache_model2 ("progs/b_s_key.mdl");
		setmodel (self, "progs/b_s_key.mdl");
		self.netname = "silver keycard";
		break;
	}

	key_setsounds();
	self.touch = key_touch;
	self.items = IT_KEY1;
	setsize (self, '-16 -16 -24', '16 16 32');
	StartItem ();
};

/*QUAKED item_key2 (0 .5 .8) (-16 -16 -24) (16 16 32)
GOLD key
In order for keys to work
you MUST set your maps
worldtype to one of the
following:
0: medieval
1: metal
2: base
*/

void() item_key2 =
{
	if (deathmatch)
	{
		remove(self);
		return;
	}

	switch (world.worldtype)
	{
	case WT_MEDIEVAL:
		precache_model ("progs/w_g_key.mdl");
		setmodel (self, "progs/w_g_key.mdl");
		self.netname = "gold key";
		break;
	case WT_METAL:
		precache_model ("progs/m_g_key.mdl");
		setmodel (self, "progs/m_g_key.mdl");
		self.netname = "gold runekey";
		break;
	case WT_BASE:
		precache_model2 ("progs/b_g_key.mdl");
		setmodel (self, "progs/b_g_key.mdl");
		self.netname = "gold keycard";
		break;
	}
	key_setsounds();
	self.touch = key_touch;
	self.items = IT_KEY2;
	setsize (self, '-16 -16 -24', '16 16 32');
	StartItem ();
};



/*
===============================================================================

END OF LEVEL RUNES

===============================================================================
*/

void() sigil_touch =
{
	if (other.classname != "player")
		return;
	if (other.health <= 0)
		return;

	centerprint (other, "You got the rune!");

	sound (other, CHAN_ITEM, self.noise, 1, ATTN_NORM);
	stuffcmd (other, "bf\n");
	self.solid = SOLID_NOT;
	self.model = string_null;
	serverflags = serverflags | (self.spawnflags & 15);
	self.classname = "";            // so rune doors won't find it
	
	activator = other;
	SUB_UseTargets();                               // fire all targets / killtargets
};


/*QUAKED item_sigil (0 .5 .8) (-16 -16 -24) (16 16 32) E1 E2 E3 E4
End of level sigil, pick up to end episode and return to jrstart.
*/

void() item_sigil =
{
	if (deathmatch)
	{
		remove(self);
		return;
	}

	if (!self.spawnflags)
		objerror ("no spawnflags");

	precache_sound ("misc/runekey.wav");
	self.noise = "misc/runekey.wav";

	switch (self.spawnflags & 15)
	{
	case 1:
		precache_model ("progs/end1.mdl");
		setmodel (self, "progs/end1.mdl");
		break;
	case 2:
		precache_model2 ("progs/end2.mdl");
		setmodel (self, "progs/end2.mdl");
		break;
	case 4:
		precache_model2 ("progs/end3.mdl");
		setmodel (self, "progs/end3.mdl");
		break;
	case 8:
		precache_model2 ("progs/end4.mdl");
		setmodel (self, "progs/end4.mdl");
		break;
	}
	
	self.touch = sigil_touch;
	setsize (self, '-16 -16 -24', '16 16 32');
	StartItem ();
};

/*
===============================================================================

POWERUPS

===============================================================================
*/

void() powerup_touch =
{
	if (other.classname != "player")
		return;
	if (other.health <= 0)
		return;

	sprint3 (other, PRINT_LOW, "You got the ",  self.netname, "\n");

	self.mdl = self.model;

	if (deathmatch)
	{	
		if (self.items & (IT_INVULNERABILITY | IT_INVISIBILITY))
			self.nextthink = time + 60*5;
		else
			self.nextthink = time + 60;
	
		self.think = SUB_regen;
	}

	sound (other, CHAN_VOICE, self.noise, 1, ATTN_NORM);
	stuffcmd (other, "bf\n");
	self.solid = SOLID_NOT;
	other.items = other.items | self.items;
	self.model = string_null;

// do the apropriate action
	switch (self.items)
	{
	case IT_SUIT:
		other.rad_time = 1;
		other.radsuit_finished = time + 30;
		break;
	case IT_INVULNERABILITY:
		other.invincible_time = 1;
		other.invincible_finished = time + 30;
		break;
	case IT_INVISIBILITY:
		other.invisible_time = 1;
		other.invisible_finished = time + 30;
		break;
	case IT_QUAD:
		if (deathmatch == 4)
		{
			other.armortype = 0;
			other.armorvalue = 0;
			other.ammo_cells_real = 0;
			W_UpdateAmmoCounts(other);
		}
		other.super_time = 1;
		other.super_damage_finished = time + 30;
		break;
	}

	activator = other;
	SUB_UseTargets();                               // fire all targets / killtargets
};



/*QUAKED item_artifact_invulnerability (0 .5 .8) (-16 -16 -24) (16 16 32)
Player is invulnerable for 30 seconds
*/
void() item_artifact_invulnerability =
{
	self.touch = powerup_touch;

	precache_model ("progs/invulner.mdl");
	precache_sound ("items/protect.wav");
	precache_sound ("items/protect2.wav");
	precache_sound ("items/protect3.wav");
	self.noise = "items/protect.wav";
	setmodel (self, "progs/invulner.mdl");
	self.netname = "Pentagram of Protection";
	self.effects = self.effects | ef_red;
	self.items = IT_INVULNERABILITY;
	setsize (self, '-16 -16 -24', '16 16 32');
	StartItem ();
};

/*QUAKED item_artifact_envirosuit (0 .5 .8) (-16 -16 -24) (16 16 32)
Player takes no damage from water or slime for 30 seconds
*/
void() item_artifact_envirosuit =
{
	self.touch = powerup_touch;

	precache_model ("progs/suit.mdl");
	precache_sound ("items/suit.wav");
	precache_sound ("items/suit2.wav");
	self.noise = "items/suit.wav";
	setmodel (self, "progs/suit.mdl");
	self.netname = "Biosuit";
	self.items = IT_SUIT;
	setsize (self, '-16 -16 -24', '16 16 32');
	StartItem ();
};


/*QUAKED item_artifact_invisibility (0 .5 .8) (-16 -16 -24) (16 16 32)
Player is invisible for 30 seconds
*/
void() item_artifact_invisibility =
{
	self.touch = powerup_touch;

	precache_model ("progs/invisibl.mdl");
	precache_sound ("items/inv1.wav");
	precache_sound ("items/inv2.wav");
	precache_sound ("items/inv3.wav");
	self.noise = "items/inv1.wav";
	setmodel (self, "progs/invisibl.mdl");
	self.netname = "Ring of Shadows";
	self.items = IT_INVISIBILITY;
	setsize (self, '-16 -16 -24', '16 16 32');
	StartItem ();
};


/*QUAKED item_artifact_super_damage (0 .5 .8) (-16 -16 -24) (16 16 32)
The next attack from the player will do 4x damage
*/
void() item_artifact_super_damage =
{
	self.touch = powerup_touch;

	precache_model ("progs/quaddama.mdl");
	precache_sound ("items/damage.wav");
	precache_sound ("items/damage2.wav");
	precache_sound ("items/damage3.wav");
	self.noise = "items/damage.wav";
	setmodel (self, "progs/quaddama.mdl");
	if (deathmatch == 4)
		self.netname = "OctaPower";	
	else
		self.netname = "Quad Damage";
	self.items = IT_QUAD;
	self.effects = self.effects | ef_blue;
	setsize (self, '-16 -16 -24', '16 16 32');
	StartItem ();
};



/*
===============================================================================

PLAYER BACKPACKS

===============================================================================
*/

void() BackpackTouch =
{
	local string    s;
	local   float   best, old, new;
	local           entity  stemp;
	local   float   acount;
	local   float   b_switch;

	if (deathmatch == 4)
		if (other.invincible_time > 0)
			return;

	b_switch = numberclientinfokey(other,"b_switch");
	if (b_switch == 0)
		b_switch = 8;

	if (other.classname != "player")
		return;
	if (other.health <= 0)
		return;
		
	acount = 0;
	if (deathmatch == 4)
	{       
		other.health = other.health + 10;
		sprint1 (other, PRINT_LOW, "You get 10 additional health\n");
		if ((other.health > 250) && (other.health < 300))
			sound (other, CHAN_ITEM, "items/protect3.wav", 1, ATTN_NORM);
		else
			sound (other, CHAN_ITEM, "weapons/lock4.wav", 1, ATTN_NORM);
		stuffcmd (other, "bf\n");
		remove(self);

		if (other.health >299)
		{               
			if (other.invincible_time != 1)
			{                       
				other.invincible_time = 1;
				other.invincible_finished = time + 30;
				other.items = other.items | IT_INVULNERABILITY;
				
				other.super_time = 1;
				other.super_damage_finished = time + 30;
				other.items = other.items | IT_QUAD;

				other.ammo_cells_real = 0;
				W_UpdateAmmoCounts(other);
	
				sound (other, CHAN_VOICE, "boss1/sight1.wav", 1, ATTN_NORM);
				stuffcmd (other, "bf\n");               
				bprint2 (PRINT_HIGH, other.netname, " attains bonus powers!!!\n");
			}
		}
		self = other;
		return;
	}

	sprint1 (other, PRINT_LOW, "You get ");

	if ((other.items & self.items) == 0)
	{
		acount = 1;
		sprint2 (other, PRINT_LOW, "the ", I_WeaponName(self.items));
	}
 
// if the player was using his best weapon, change up to the new one if better          
	stemp = self;
	self = other;
	best = W_BestWeapon();
	self = stemp;

// change weapons
	other.ammo_shells_real = other.ammo_shells_real + self.ammo_shells;
	other.ammo_nails_real = other.ammo_nails_real + self.ammo_nails;
	other.ammo_rockets_real = other.ammo_rockets_real + self.ammo_rockets;
	other.ammo_cells_real = other.ammo_cells_real + self.ammo_cells;

	new = self.items;
	if (!new)
		new = other.weapon;
	old = other.items;
	other.items = other.items | self.items;
	
	bound_other_ammo ();

	if (self.ammo_shells)
	{
		s = ftos(self.ammo_shells);
		if (acount)
			sprint3(other, PRINT_LOW, ". ", s, " shells");
		else
			sprint2(other, PRINT_LOW, s, " shells");
		acount = 1;
	}
	if (self.ammo_nails)
	{
		s = ftos(self.ammo_nails);
		if (acount)
			sprint3(other, PRINT_LOW, ". ", s, " nails");
		else
			sprint2(other, PRINT_LOW, s, " nails");
		acount = 1;
	}
	if (self.ammo_rockets)
	{
		s = ftos(self.ammo_rockets);
		if (acount)
			sprint3(other, PRINT_LOW, ". ", s, " rockets");
		else
			sprint2(other, PRINT_LOW, s, " rockets");
		acount = 1;
	}
	if (self.ammo_cells)
	{
		s = ftos(self.ammo_cells);
		if (acount)
			sprint3(other, PRINT_LOW, ". ", s, " cells");
		else
			sprint2(other, PRINT_LOW, s, " cells");
		acount = 1;
	}
	
	if ( (deathmatch==3 || deathmatch == 5) & ( (WeaponCode(new)==6) || (WeaponCode(new)==7) ) & (other.ammo_rockets_real < 5) )
		other.ammo_rockets_real = 5;

	sprint1 (other, PRINT_LOW, "\n");
// backpack touch sound
	sound (other, CHAN_ITEM, "weapons/lock4.wav", 1, ATTN_NORM);
	stuffcmd (other, "bf\n");

	W_UpdateAmmoCounts(other);

	remove(self);
	self = other;
	
// change to the weapon
	
	if ( WeaponCode(new) <= b_switch )
	{
		if (self.flags & FL_INWATER)
		{
			if (new != IT_LIGHTNING)
			{
				Deathmatch_Weapon (old, new);
			}
		}
		else
		{                
			Deathmatch_Weapon (old, new);
		}
	}
};

/*
===============
DropBackpack
===============
*/
void() DropBackpack =
{
	local entity    item;

	if (!(self.ammo_shells_real + self.ammo_nails_real + self.ammo_rockets_real + self.ammo_cells_real))
		return; // nothing in it

	item = spawn();
	item.origin = self.origin - '0 0 24';
	
	item.items = self.weapon;

	item.ammo_shells = self.ammo_shells_real;
	item.ammo_nails = self.ammo_nails_real;
	item.ammo_rockets = self.ammo_rockets_real;
	item.ammo_cells = self.ammo_cells_real;

	item.velocity_z = 300;
	item.velocity_x = -100 + (random() * 200);
	item.velocity_y = -100 + (random() * 200);
	
	item.flags = FL_ITEM;
	item.solid = SOLID_TRIGGER;
	item.movetype = MOVETYPE_TOSS;
	setmodel (item, "progs/backpack.mdl");
	setsize (item, '-16 -16 0', '16 16 56');
	item.touch = BackpackTouch;
	
	item.nextthink = time + 120;    // remove after 2 minutes
	item.think = SUB_Remove;
};


 