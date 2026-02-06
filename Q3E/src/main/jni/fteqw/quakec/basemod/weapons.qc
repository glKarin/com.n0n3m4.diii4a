/*
*/
void (entity targ, entity inflictor, entity attacker, float damage, INTEGER mod) T_Damage;
void () player_run;
void(entity inflictor, entity attacker, float damage, float radius, entity ignore, INTEGER mod) T_RadiusDamage;
void(vector org, float damage) SpawnBlood;
void() SuperDamageSound;


// called by worldspawn
void() W_Precache =
{
	precache_sound ("weapons/r_exp3.wav");  // new rocket explosion
	precache_sound ("weapons/rocket1i.wav");        // spike gun
	precache_sound ("weapons/sgun1.wav");
	precache_sound ("weapons/guncock.wav"); // player shotgun
	precache_sound ("weapons/ric1.wav");    // ricochet (used in c code)
	precache_sound ("weapons/ric2.wav");    // ricochet (used in c code)
	precache_sound ("weapons/ric3.wav");    // ricochet (used in c code)
	precache_sound ("weapons/spike2.wav");  // super spikes
	precache_sound ("weapons/tink1.wav");   // spikes tink (used in c code)
	precache_sound ("weapons/grenade.wav"); // grenade launcher
	precache_sound ("weapons/bounce.wav");          // grenade bounce
	precache_sound ("weapons/shotgn2.wav"); // super shotgun
};

#define crandom() (2*(random()-0.5))

/*
Ammo update functions
*/
void(entity ent) W_UpdateAmmoCounts =
{
	// update current ammo
	switch (ent.ammo_type)
	{
	case AT_SHELLS:
		ent.currentammo = ent.ammo_shells_real;
		break;
	case AT_NAILS:
		ent.currentammo = ent.ammo_nails_real;
		break;
	case AT_ROCKETS:
		ent.currentammo = ent.ammo_rockets_real;
		break;
	case AT_CELLS:
		ent.currentammo = ent.ammo_cells_real;
		break;
	default:
		ent.currentammo = 0;
	}
	
	// update ammo display (FTE progs converts to floats here)
	ent.ammo_shells = ent.ammo_shells_real;
	ent.ammo_nails = ent.ammo_nails_real;
	ent.ammo_rockets = ent.ammo_rockets_real;
	ent.ammo_cells = ent.ammo_cells_real;
};

/*
================
W_FireAxe
================
*/
void() W_FireAxe =
{
	local   vector  source;
	local   vector  org;

	makevectors (self.v_angle);
	source = self.origin + '0 0 16';
	traceline (source, source + v_forward*64, FALSE, self);
	if (trace_fraction == 1.0)
		return;
	
	org = trace_endpos - v_forward*4;

	if (trace_ent.takedamage)
	{
		SpawnBlood (org, 20);
		if (deathmatch > 3)
			T_Damage (trace_ent, self, self, 75, MOD_AXE);
		else
			T_Damage (trace_ent, self, self, 20, MOD_AXE);
	}
	else
	{       // hit wall
		sound (self, CHAN_WEAPON, "player/axhit2.wav", 1, ATTN_NORM);

		TE_gunshot(org);
	}
};


//============================================================================

/*
================
SpawnMeatSpray
================
*/
void(vector org, vector vel) SpawnMeatSpray =
{
	local   entity missile;

	missile = spawn ();
	missile.owner = self;
	missile.movetype = MOVETYPE_BOUNCE;
	missile.solid = SOLID_NOT;

	makevectors (self.angles);

	missile.velocity = vel;
	missile.velocity_z = missile.velocity_z + 250 + 50*random();

	missile.avelocity = '3000 1000 2000';
	
// set missile duration
	missile.nextthink = time + 1;
	missile.think = SUB_Remove;

	setmodel (missile, "progs/zom_gib.mdl");
	setsize (missile, '0 0 0', '0 0 0');            
	setorigin (missile, org);
};

/*
==============================================================================

MULTI-DAMAGE

Collects multiple small damages into a single damage

==============================================================================
*/

entity  multi_ent;
float   multi_damage;
INTEGER multi_mod;

vector  blood_org;
float   blood_count;

vector  puff_org;
float   puff_count;

void() ClearMultiDamage =
{
	multi_ent = world;
	multi_damage = 0;
	blood_count = 0;
	puff_count = 0;
	multi_mod = MOD_NONE;
};

void() ApplyMultiDamage =
{
	if (!multi_ent)
		return;
	T_Damage (multi_ent, self, self, multi_damage, multi_mod);
};

void(entity hit, float damage, INTEGER mod) AddMultiDamage =
{
	if (!hit)
		return;
	
	if (hit != multi_ent || mod != multi_mod)
	{
		ApplyMultiDamage ();
		multi_damage = damage;
		multi_ent = hit;
	}
	else
		multi_damage = multi_damage + damage;
};

void() Multi_Finish =
{
	if (puff_count)
		TE_gunshot(puff_org);

	if (blood_count)
		SpawnBlood(blood_org, blood_count);
};

/*
==============================================================================
BULLETS
==============================================================================
*/

/*
================
TraceAttack
================
*/
void(float damage, vector dir, INTEGER mod) TraceAttack =
{
	local   vector  vel, org;
	
	vel = normalize(dir + v_up*crandom() + v_right*crandom());
	vel = vel + 2*trace_plane_normal;
	vel = vel * 200;

	org = trace_endpos - dir*4;

	if (trace_ent.takedamage)
	{
		blood_count = blood_count + 1;
		blood_org = org;
		AddMultiDamage (trace_ent, damage, mod);
	}
	else
	{
		puff_count = puff_count + 1;
	}
};

/*
================
FireBullets

Used by shotgun, super shotgun, and enemy soldier firing
Go to the trouble of combining multiple pellets into a single damage call.
================
*/
void(float shotcount, vector dir, vector spread, INTEGER mod) FireBullets =
{
	local   vector direction;
	local   vector  src;
	
	makevectors(self.v_angle);

	src = self.origin + v_forward*10;
	src_z = self.absmin_z + self.size_z * 0.7;

	ClearMultiDamage ();

	traceline (src, src + dir*2048, FALSE, self);
	puff_org = trace_endpos - dir*4;

	while (shotcount > 0)
	{
		direction = dir + crandom()*spread_x*v_right + crandom()*spread_y*v_up;
		traceline (src, src + direction*2048, FALSE, self);
		if (trace_fraction != 1.0)
			TraceAttack (4, direction, mod);

		shotcount = shotcount - 1;
	}
	ApplyMultiDamage ();
	Multi_Finish ();
};

/*
================
W_FireShotgun
================
*/
void() W_FireShotgun =
{
	local vector dir;

	sound (self, CHAN_WEAPON, "weapons/guncock.wav", 1, ATTN_NORM); 

	VK_smallkick(self);
	
	if (deathmatch != 4)
	{
		self.ammo_shells_real -= 1;
		W_UpdateAmmoCounts(self);
	}

	dir = aim (self, 100000);
	FireBullets (6, dir, '0.04 0.04 0', MOD_SHOTGUN);
};


/*
================
W_FireSuperShotgun
================
*/
void() W_FireSuperShotgun =
{
	local vector dir;

	if (self.currentammo == 1)
	{
		W_FireShotgun ();
		return;
	}
		
	sound (self ,CHAN_WEAPON, "weapons/shotgn2.wav", 1, ATTN_NORM); 

	VK_bigkick(self);
	
	if (deathmatch != 4)
	{
		self.ammo_shells_real -= 2;
		W_UpdateAmmoCounts(self);
	}

	dir = aim (self, 100000);
	FireBullets (14, dir, '0.14 0.08 0', MOD_SUPERSHOTGUN);
};


/*
==============================================================================

ROCKETS

==============================================================================
*/

/*
================
W_FireRocket
================
*/
void() W_FireRocket =
{
	if (deathmatch != 4)
	{
		self.ammo_rockets_real -= 1;
		W_UpdateAmmoCounts(self);
	}
	
	sound (self, CHAN_WEAPON, "weapons/sgun1.wav", 1, ATTN_NORM);

	VK_smallkick(self);
	PRJ_FireProjectile(self, 
		"progs/missile.mdl", 
		self.origin + v_forward*8 + '0 0 16', 
		aim(self, 1000) * 1000,
		PE_EXPLOSION, 
		100+random()*20, 
		MOD_ROCKET, 
		5);
	PRJ_SetRadiusDamage(120, 160, MOD_ROCKETRADIUS);
};

/*
===============================================================================
LIGHTNING
===============================================================================
*/

void(entity from, float damage, INTEGER lmod) LightningHit =
{
	TE_lightningblood(trace_endpos);

	T_Damage (trace_ent, from, from, damage, lmod);
};

/*
=================
LightningDamage
=================
*/
void(vector p1, vector p2, entity from, float damage, INTEGER lmod) LightningDamage =
{
	local entity            e1, e2;
	local vector            f;
	
	f = p2 - p1;
	f = normalize(f);
	f_x = 0 - f_y;
	f_y = f_x;
	f_z = 0;
	f = f*16;

	e1 = e2 = world;

	traceline (p1, p2, FALSE, self);

	if (trace_ent.takedamage)
		LightningHit (from, damage, lmod);
	e1 = trace_ent;

	traceline (p1 + f, p2 + f, FALSE, self);
	if (trace_ent != e1 && trace_ent.takedamage)
		LightningHit (from, damage, lmod);
	e2 = trace_ent;

	traceline (p1 - f, p2 - f, FALSE, self);
	if (trace_ent != e1 && trace_ent != e2 && trace_ent.takedamage)
		LightningHit (from, damage, lmod);
};


void() W_FireLightning =
{
	local   vector          org;
	local   float           cells;
	local   INTEGER		expmod;

	if (self.ammo_cells_real < 1)
	{
		W_WeaponSwitch (W_BestWeapon ());
		return;
	}

// explode if under water
	if (self.waterlevel > 1)
	{
		if (deathmatch > 3)
		{
			if (random() <= 0.5)
			{
				T_Damage (self, self, self.owner, 4000, MOD_SELFWATER);
				return;
			}
		}

		cells = self.ammo_cells_real;
		self.ammo_cells_real = 0;
		W_WeaponSwitch (W_BestWeapon ());
		expmod = MOD_SHAFTWATER;
		if (self.watertype == CONTENT_SLIME)
			expmod = MOD_SHAFTSLIME;
		else if (self.watertype == CONTENT_LAVA)
			expmod = MOD_SHAFTLAVA;
		T_RadiusDamage (self, self, 35*cells, 40+35*cells, world, expmod);
		return;
	}

	if (self.lightning_sound < time)
	{
		sound (self, CHAN_WEAPON, "weapons/lhit.wav", 1, ATTN_NORM);
		self.lightning_sound = time + 0.6;
	}
	VK_smallkick(self);

	if (deathmatch != 4)
	{
		self.ammo_cells_real -= 1;
		W_UpdateAmmoCounts(self);
	}

	org = self.origin + '0 0 16';
	
	traceline (org, org + v_forward*600, TRUE, self);

	TE_lightning2(self, org, trace_endpos);

	LightningDamage (self.origin, trace_endpos + v_forward*4, self, 30, MOD_SHAFT);
};

/*
================
W_FireGrenade
================
*/
void() W_FireGrenade =
{
	local vector vel;

	if (deathmatch != 4)
	{
		self.ammo_rockets_real -= 1;
		W_UpdateAmmoCounts(self);
	}
	
	sound (self, CHAN_WEAPON, "weapons/grenade.wav", 1, ATTN_NORM);

	if (self.v_angle_x)
		vel = v_forward*600 + v_up * 200 + crandom()*v_right*10 + crandom()*v_up*10;
	else
	{
		vel = aim(self, 10000) * 600;
		vel_z = 200;
	}

	VK_smallkick(self);
	PRJ_FireProjectile(self, "progs/grenade.mdl", self.origin, vel, PE_EXPLOSIONGROUND, 0, 0, 2.5);
	PRJ_SetRadiusDamage(120, 160, MOD_GRENADE);
	PRJ_SetBouncyProjectile();

	if (deathmatch == 4)
	{
		self.attack_finished = time + 1.1;
		T_Damage (self, self, self.owner, 10, MOD_GRENADE);
	}
};


//=============================================================================
void(float ox) W_FireSpikes =
{
	if (self.ammo_nails_real < 1)
	{
		W_WeaponSwitch (W_BestWeapon ());
		return;
	}

	sound (self, CHAN_WEAPON, "weapons/rocket1i.wav", 1, ATTN_NORM);
	if (deathmatch != 4)
	{
		self.ammo_nails_real -= 1;
		W_UpdateAmmoCounts(self);
	}

	VK_smallkick(self);
	PRJ_FireProjectile(self,
		"progs/spike.mdl",
		self.origin + '0 0 16' + v_right*ox,
		aim(self, 1000) * 1000,
		PE_SPIKE,
		9,
		MOD_SPIKE,
		6);
};

void() W_FireSuperSpikes =
{
	if (self.ammo_nails_real < 2)
	{
		W_FireSpikes(0);
		return;
	}
	
	sound (self, CHAN_WEAPON, "weapons/spike2.wav", 1, ATTN_NORM);
	if (deathmatch != 4)
	{
		self.ammo_nails_real -= 2;
		W_UpdateAmmoCounts(self);
	}

	VK_smallkick(self);
	PRJ_FireProjectile(self,
		"progs/s_spike.mdl",
		self.origin + '0 0 16',
		aim(self, 1000) * 1000,
		PE_SUPERSPIKE,
		18,
		MOD_SUPERSPIKE,
		6);
};

/*
===============================================================================

PLAYER WEAPON USE

===============================================================================
*/
// different from W_CheckNoAmmo due to SSG/SNG being able to fire 1 shot instead of 2...
BOOL(float wep) W_HasAmmo =
{
	switch (wep)
	{
	case IT_SHOTGUN:
		return self.ammo_shells_real >= 1;
	case IT_SUPER_SHOTGUN:
		return self.ammo_shells_real >= 2;
	case IT_NAILGUN:
		return self.ammo_nails_real >= 1;
	case IT_SUPER_NAILGUN:
		return self.ammo_nails_real >= 2;
	case IT_GRENADE_LAUNCHER:
	case IT_ROCKET_LAUNCHER:
		return self.ammo_rockets_real >= 1;
	case IT_LIGHTNING:
		return self.ammo_cells_real >= 1;
	}

	return TRUE;
};

void() W_UpdateWeapon =
{
	player_run ();          // get out of any weapon firing states

	self.items = self.items - ( self.items & (IT_SHELLS | IT_NAILS | IT_ROCKETS | IT_CELLS) );

	switch (self.weapon)
	{
	case IT_AXE:
		self.weaponmodel = "progs/v_axe.mdl";
		self.ammo_type = AT_NONE;
		break;
	case IT_SHOTGUN:
		self.weaponmodel = "progs/v_shot.mdl";
		self.items = self.items | IT_SHELLS;
		self.ammo_type = AT_SHELLS;
		break;
	case IT_SUPER_SHOTGUN:
		self.weaponmodel = "progs/v_shot2.mdl";
		self.items = self.items | IT_SHELLS;
		self.ammo_type = AT_SHELLS;
		break;
	case IT_NAILGUN:
		self.weaponmodel = "progs/v_nail.mdl";
		self.items = self.items | IT_NAILS;
		self.ammo_type = AT_NAILS;
		break;
	case IT_SUPER_NAILGUN:
		self.weaponmodel = "progs/v_nail2.mdl";
		self.items = self.items | IT_NAILS;
		self.ammo_type = AT_NAILS;
		break;
	case IT_GRENADE_LAUNCHER:
		self.weaponmodel = "progs/v_rock.mdl";
		self.items = self.items | IT_ROCKETS;
		self.ammo_type = AT_ROCKETS;
		break;
	case IT_ROCKET_LAUNCHER:
		self.weaponmodel = "progs/v_rock2.mdl";
		self.items = self.items | IT_ROCKETS;
		self.ammo_type = AT_ROCKETS;
		break;
	case IT_LIGHTNING:
		self.weaponmodel = "progs/v_light.mdl";
		self.items = self.items | IT_CELLS;
		self.ammo_type = AT_CELLS;
		break;
	default:
		self.weaponmodel = "";
	}

	self.weaponframe = 0;
};

void(float weap) W_WeaponSwitch =
{
	// skip weapon model/ammo_type update if this isn't a new weapon
	if (self.weapon != weap)
	{
		self.weapon = weap;
		W_UpdateWeapon();
	}

	// always update ammo count
	W_UpdateAmmoCounts(self);
};

float() W_BestWeapon =
{
	float fl;

	if (self.waterlevel <= 1)
		fl = IT_LIGHTNING;
	else
		fl = IT_SUPER_NAILGUN;

	while (1)
	{
		if ( (self.items & fl) && W_HasAmmo(fl) )
			return fl;

		// best weapon order
		switch (fl)
		{
		case IT_LIGHTNING:
			fl = IT_SUPER_NAILGUN;
			break;
		case IT_SUPER_NAILGUN:
			fl = IT_SUPER_SHOTGUN;
			break;
		case IT_SUPER_SHOTGUN:
			fl = IT_NAILGUN;
			break;
		case IT_NAILGUN:
			fl = IT_SHOTGUN;
			break;
		case IT_SHOTGUN:
		default:
			return IT_AXE; // so we don't get an infinite loop with certain engines
		}
	}
};

BOOL() W_CheckNoAmmo =
{
	if (self.currentammo > 0)
		return TRUE;

	if (self.weapon == IT_AXE)
		return TRUE;
	
	W_WeaponSwitch (W_BestWeapon ());
	
// drop the weapon down
	return FALSE;
};

/*
============
W_Attack

An attack impulse can be triggered now
============
*/
void()	player_axe1;
void()	player_axeb1;
void()	player_axec1;
void()	player_axed1;
void()	player_shot1;
void()	player_nail1;
void()	player_nail2;
void()	player_light1;
void()	player_light2;
void()	player_rocket1;
void()	muzzleflash;

void() W_Attack =
{
	float r;

	if (!W_CheckNoAmmo ())
		return;

	makevectors (self.v_angle);                 // calculate forward angle for velocity
	self.show_hostile = time + 1;   // wake monsters up

	if (self.weaponstate == WS_IDLE) // start delay
		self.delay = time + 0.1;

	// animations are dealt with here
	switch (self.weapon) 
	{
	case IT_AXE:
		r = random();
		if (r < 0.25)
		{
			self.weaponframe = 1;
			player_axe1 ();
		}
		else if (r < 0.5)
		{
			self.weaponframe = 5;
			player_axeb1 ();
		}
		else if (r < 0.75)
		{
			self.weaponframe = 1;
			player_axec1 ();
		}
		else
		{
			self.weaponframe = 5;
			player_axed1 ();
		}
		break;
	case IT_NAILGUN:
	case IT_SUPER_NAILGUN:
		muzzleflash();
		if (self.weaponframe == 0)
			self.weaponframe = 1;

		if (self.weaponframe & 1)
			player_nail1();
		else
			player_nail2();
		break;		
	case IT_GRENADE_LAUNCHER:
	case IT_ROCKET_LAUNCHER:
		self.weaponframe = 1;
		muzzleflash();
		player_rocket1();
		break;
	case IT_LIGHTNING:
		muzzleflash();
		if (self.weaponframe == 0)
		{
			sound (self, CHAN_AUTO, "weapons/lstart.wav", 1, ATTN_NORM);
			self.weaponframe = 1;
		}

		if (self.weaponframe & 1)
			player_light1();
		else
			player_light2();
		break;
	default:
		muzzleflash();
		self.weaponframe = 1;
		player_shot1();
	}

	SuperDamageSound();

	// firing is done here (r is used for round time instead of a temp here)
	switch (self.weapon)
	{
	case IT_AXE:
		// frame handles most of this so skip most of it
		sound (self, CHAN_WEAPON, "weapons/ax1.wav", 1, ATTN_NORM);
		r = 0.5;
		break;
	case IT_SHOTGUN:
		W_FireShotgun ();
		r = 0.5;
		break;
	case IT_SUPER_SHOTGUN:
		W_FireSuperShotgun ();
		r = 0.7;
		break;
	case IT_NAILGUN:
		if (self.weaponstate == WS_FIRING1)
		{
			W_FireSpikes(-4);
			self.weaponstate = WS_FIRING2;
		}
		else
		{
			W_FireSpikes(4);
			self.weaponstate = WS_FIRING1;
		}

		r = 0.1;
		break;
	case IT_SUPER_NAILGUN:
		W_FireSuperSpikes();
		r = 0.1;
		break;
	case IT_GRENADE_LAUNCHER:
		W_FireGrenade();
		r = 0.6;
		break;
	case IT_ROCKET_LAUNCHER:
		W_FireRocket();
		r = 0.8;
		break;
	case IT_LIGHTNING:
		W_FireLightning();
		r = 0.1;
		break;
	}

	if (self.weaponstate == WS_IDLE)
		self.weaponstate = WS_FIRING1;

	// advance attack time
	if (self.attack_finished <= time)
		self.attack_finished = self.attack_finished + r;
};

/*
============
W_ChangeWeapon

============
*/
void() W_ChangeWeapon =
{
	local   float   fl;
	
	switch (self.impulse)
	{
	case 1:
		fl = IT_AXE;
		break;
	case 2:
		fl = IT_SHOTGUN;
		break;
	case 3:
		fl = IT_SUPER_SHOTGUN;
		break;
	case 4:
		fl = IT_NAILGUN;
		break;
	case 5:
		fl = IT_SUPER_NAILGUN;
		break;
	case 6:
		fl = IT_GRENADE_LAUNCHER;
		break;
	case 7:
		fl = IT_ROCKET_LAUNCHER;
		break;
	case 8:
		fl = IT_LIGHTNING;
		break;
	}
	
	
	if (!(self.items & fl))
	{       // don't have the weapon or the ammo
		sprint1 (self, PRINT_HIGH, "no weapon.\n");
		return;
	}
	
	if (!W_HasAmmo(fl))
	{       // don't have the ammo
		sprint1 (self, PRINT_HIGH, "not enough ammo.\n");
		return;
	}

//
// set weapon, set ammo
//
	W_WeaponSwitch (fl);
};

/*
============
CheatCommand
============
*/
void() CheatCommand =
{
      if (deathmatch || coop)
		return;

	self.ammo_rockets_real = 100;
	self.ammo_nails_real = 200;
	self.ammo_shells_real = 100;
	self.ammo_cells_real = 100;
	self.items |= IT_AXE |
		IT_SHOTGUN |
		IT_SUPER_SHOTGUN |
		IT_NAILGUN |
		IT_SUPER_NAILGUN |
		IT_GRENADE_LAUNCHER |
		IT_ROCKET_LAUNCHER |
		IT_LIGHTNING |
		IT_KEY1 | IT_KEY2;

	W_WeaponSwitch (IT_ROCKET_LAUNCHER);
};

/*
============
CycleWeaponCommand

Go to the next weapon with ammo
============
*/
void() CycleWeaponCommand =
{
	local float w;
	w = self.weapon;

	while (1)
	{
		switch (w)
		{
		case IT_LIGHTNING:
			w = IT_AXE;
			break;
		case IT_AXE:
			w = IT_SHOTGUN;
			break;
		case IT_SHOTGUN:
			w = IT_SUPER_SHOTGUN;
			break;
		case IT_SUPER_SHOTGUN:
			w = IT_NAILGUN;
			break;
		case IT_NAILGUN:
			w = IT_SUPER_NAILGUN;
			break;
		case IT_SUPER_NAILGUN:
			w = IT_GRENADE_LAUNCHER;
			break;
		case IT_GRENADE_LAUNCHER:
			w = IT_ROCKET_LAUNCHER;
			break;
		case IT_ROCKET_LAUNCHER:
			w = IT_LIGHTNING;
			break;
		}
	
		if ( (self.items & w) && W_HasAmmo(w) )
		{
			W_WeaponSwitch (w);
			return;
		}
	}

};


/*
============
CycleWeaponReverseCommand

Go to the prev weapon with ammo
============
*/
void() CycleWeaponReverseCommand =
{
	local float w;
	w = self.weapon;

	while (1)
	{
		switch (w)
		{
		case IT_LIGHTNING:
			w = IT_ROCKET_LAUNCHER;
			break;
		case IT_ROCKET_LAUNCHER:
			w = IT_GRENADE_LAUNCHER;
			break;
		case IT_GRENADE_LAUNCHER:
			w = IT_SUPER_NAILGUN;
			break;
		case IT_SUPER_NAILGUN:
			w = IT_NAILGUN;
			break;
		case IT_NAILGUN:
			w = IT_SUPER_SHOTGUN;
			break;
		case IT_SUPER_SHOTGUN:
			w = IT_SHOTGUN;
			break;
		case IT_SHOTGUN:
			w = IT_AXE;
			break;
		case IT_AXE:
			w = IT_LIGHTNING;
			break;
		}
	
		if ( (self.items & w) && W_HasAmmo(w) )
		{
			W_WeaponSwitch (w);
			return;
		}
	}

};


/*
============
ServerflagsCommand

Just for development
============
*/
void() ServerflagsCommand =
{
      if (deathmatch || coop)
		return;

	serverflags = serverflags * 2 + 1;
};


/*
============
ImpulseCommands

============
*/
void() ImpulseCommands =
{
	switch (self.impulse) {
	case 1 .. 8:
		W_ChangeWeapon ();
		break;
	case 9:
		CheatCommand ();
		break;
	case 10:
		CycleWeaponCommand ();
		break;
	case 11:
		ServerflagsCommand ();
		break;
	case 12:
		CycleWeaponReverseCommand ();
		break;
	}

	self.impulse = 0;
};

/*
============
W_HandlePlayerFrame

Handle player weapon model
============
*/
void() W_HandlePlayerFrame =
{
	if (!self.weaponframe)
		return;

	if (self.weaponframe_time >= time)
		return;

	switch (self.weapon)
	{
	case IT_AXE:
		// axe frames can start at 1 or 5
		self.weaponframe_time = time + 0.1;
		self.weaponframe = self.weaponframe + 1;
		if (self.weaponframe == 5)
			self.weaponframe = 0;
		else if (self.weaponframe > 8)
			self.weaponframe = 0;
		
		return;
	case IT_NAILGUN:
	case IT_SUPER_NAILGUN:
		// cycle until fire button is released
		if (self.weaponstate != WS_IDLE)
 		{
			self.weaponframe_time = time + 0.1;
  			self.weaponframe = self.weaponframe + 1;
  			if (self.weaponframe > 8)
  				self.weaponframe = 1;
  		}
  		else
  			self.weaponframe = 0;

		return;
	case IT_LIGHTNING:
		// cycle until fire button is released
		if (self.weaponstate != WS_IDLE)
		{
			self.weaponframe_time = time + 0.1;
			self.weaponframe = self.weaponframe + 1;
			if (self.weaponframe > 4)
				self.weaponframe = 1;
		}
		else
			self.weaponframe = 0;
		return;
	default:
		self.weaponframe = self.weaponframe + 1;
		self.weaponframe_time = time + 0.1;
		if (self.weaponframe > 6)
			self.weaponframe = 0;
	}
};

/*
============
W_WeaponFrame

Called every frame so impulse events can be handled as well as possible
============
*/
void() W_WeaponFrame =
{
	local INTEGER scount;

	W_HandlePlayerFrame();

	if (time < self.attack_finished)
		return;

	if (self.impulse)
		ImpulseCommands ();
	
// check for attack
	if (self.button0)
	{
		scount = 0;
		// play catchup but don't allow more than 4 shots per frame
		while (self.attack_finished <= time)
		{
			if (scount >= 4)
			{
				self.attack_finished = time;
				break;
			}

			W_Attack();
			scount++;
		}
	}
	else
	{
		self.attack_finished = time;
		self.weaponstate = WS_IDLE;
	}
};

/*
========
SuperDamageSound

Plays sound if needed
========
*/
void() SuperDamageSound =
{
	if (self.super_damage_finished > time)
	{
		if (self.super_sound < time)
		{
			self.super_sound = time + 1;
			sound (self, CHAN_BODY, "items/damage3.wav", 1, ATTN_NORM);
		}
	}
	return;
};

/*
void() testfunction =
{
	local vector v;
	local float a, b, c;

	a = 2;
	b = 4;
	c = 6;
	a *= 2;
	b *= 2;
	c *= 2;
	v = '2 4 6';
	v *= 2;
	if (!a && !b && !c)
		return;

	if (!v)
		return;

	v_x = 23;

	if (self.health && self.ammo_shells && self.ammo_cells)
		return;
};
*/
 