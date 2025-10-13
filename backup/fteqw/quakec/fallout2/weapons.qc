/*
*/
void (entity targ, entity inflictor, entity attacker, float damage) T_Damage;
void () player_run;
void(entity bomb, entity attacker, float rad, entity ignore, string dtype) T_RadiusDamage;
void(vector org, float damage) SpawnBlood;
void() SuperDamageSound;
void (float rec, float number, float dam, float var, float ran, float auto) W_FireShotgun;
void (float dam, float rec, string snd, float rng, float rate) FireAssaultRifle;
void (float dam, float rec, string snd, float rng, float rate) FirePistol;
void (float dam, float rec, string snd, float rng, float rate) FireSMG;
float() FireToolkit;
void () FireAlienBlaster;
void () W_PlayerMenu;
//void () UseChem;
void () Special;
void () BuyMenu;
void() Sneak;
//void() Bandage;
void() Shield;
void () player_throw1;
void() player_knife1;
void() player_knifea;
void() ExitScreen;
void() CharacterSheet;
void() UseEquipment;
void (float slot, float snd, float drop) DropFromSlot;
void() PositionControl;
void() autofire;
void() autofire_s;
float(float iid) UseBoostingChem;
float(float iid) UseHealingChem;
float(float slotno) DecreaseDestroySlot;
void () DropAmmo;

#define weightx() (self.weight)


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
	precache_sound ("weapons/ric4.wav");    // ricochet (used in c code)
	precache_sound ("weapons/ric5.wav");    // ricochet (used in c code)
	precache_sound ("weapons/spike2.wav");  // super spikes
	precache_sound ("weapons/tink1.wav");   // spikes tink (used in c code)
	precache_sound ("weapons/grenade.wav"); // grenade launcher
	precache_sound ("weapons/bounce.wav");          // grenade bounce
	precache_sound ("weapons/shotgn2.wav"); // super shotgun
};

float() crandom =
{
	return 2*(random() - 0.5);
};

/*
================
W_FireMelee
================
*/
void(float damage, float dist, float rate) FireMelee =
{
	local   vector  source;
	local   vector  org;

	makevectors (self.v_angle);
	source = self.origin + '0 0 16';
	traceline (source, source + v_forward*dist, FALSE, self);
	if (trace_fraction == 1.0)
		return;
	
	org = trace_endpos - v_forward*4;

	if (trace_ent.takedamage)
	{
		trace_ent.axhitme = 1;
		SpawnBlood (org, 1);
		T_Damage (trace_ent, self, self, damage+random()*damage);
		sound (self, CHAN_WEAPON, trace_ent.armornoise, 1, ATTN_NORM);
	}
	else
	{       // hit wall
		sound (self, CHAN_WEAPON, "player/axhit2.wav", 1, ATTN_NORM);

		WriteByte (MSG_MULTICAST, SVC_TEMPENTITY);
		WriteByte (MSG_MULTICAST, TE_SPIKE);
		WriteCoord (MSG_MULTICAST, org_x);
		WriteCoord (MSG_MULTICAST, org_y);
		WriteCoord (MSG_MULTICAST, org_z);
		multicast (self.origin, MULTICAST_PHS);
	}
};

void() W_FireMelee =
{
	local float iid;
	iid = ToIID(self.(SlotField(self.current_slot)));
	if (iid == IID_NONE)
		FireMelee(3, 32, 0.6);
	else if (iid == IID_WP_TOOLKIT)
		FireMelee(5, 32, 0.2);
	else if (iid == IID_WP_KNIFE)
		FireMelee(5, 32, 0.2);
	else if (iid == IID_WP_AXE)
		FireMelee(10, 64, 0.2);
	else if (iid == IID_WP_VIBROBLADE)	
		FireMelee(25, 64, 0.2);
	else if (iid == IID_WP_POWERAXE)
		FireMelee(50, 96, 0.2);

	else if (iid == IID_CHEM_STIMPACK ||
		   iid == IID_CHEM_MEDICALBAG ||
		   iid == IID_CHEM_SUPERSTIM)
	{
		if (UseHealingChem(iid))
			DecreaseDestroySlot(self.current_slot);
	}
	else if (iid == IID_CHEM_ADRENALINE ||
		   iid == IID_CHEM_PSYCHO ||
		   iid == IID_CHEM_BESERK)
	{
		if (UseBoostingChem(iid))
			DecreaseDestroySlot(self.current_slot);
	}
	else
		dprint(strcat("W_FireMelee - ", ftos(iid), "not implemented\n"));
};


//============================================================================

vector() wall_velocity =
{
	local vector    vel;
	
	vel = normalize (self.velocity);
	vel = normalize(vel + v_up*(random()- 0.5) + v_right*(random()- 0.5));
	vel = vel + 2*trace_plane_normal;
	vel = vel * 200;
	
	return vel;
};


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
================
SpawnBlood
================
*/
void(vector org, float damage) SpawnBlood =
{
	WriteByte (MSG_MULTICAST, SVC_TEMPENTITY);
	WriteByte (MSG_MULTICAST, TE_BLOOD);
	WriteByte (MSG_MULTICAST, 1);
	WriteCoord (MSG_MULTICAST, org_x);
	WriteCoord (MSG_MULTICAST, org_y);
	WriteCoord (MSG_MULTICAST, org_z);
	multicast (org, MULTICAST_PVS);
};

/*
================
spawn_touchblood
================
*/
void(float damage) spawn_touchblood =
{
	local vector    vel;

	vel = wall_velocity () * 0.2;
	SpawnBlood (self.origin + vel*0.01, damage);
};

/*
==============================================================================

MULTI-DAMAGE

Collects multiple small damages into a single damage

==============================================================================
*/

entity  multi_ent;
float   multi_damage;

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
};

void() ApplyMultiDamage =
{
	if (!multi_ent)
		return;
	T_Damage (multi_ent, self, self, multi_damage);
};

void(entity hit, float damage) AddMultiDamage =
{
	if (!hit)
		return;
	
	if (hit != multi_ent)
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
	{
		WriteByte (MSG_MULTICAST, SVC_TEMPENTITY);
		WriteByte (MSG_MULTICAST, TE_GUNSHOT);
		WriteByte (MSG_MULTICAST, puff_count);
		WriteCoord (MSG_MULTICAST, puff_org_x);
		WriteCoord (MSG_MULTICAST, puff_org_y);
		WriteCoord (MSG_MULTICAST, puff_org_z);
		multicast (puff_org, MULTICAST_PVS);
	}

	if (blood_count)
	{
		WriteByte (MSG_MULTICAST, SVC_TEMPENTITY);
		WriteByte (MSG_MULTICAST, TE_BLOOD);
		WriteByte (MSG_MULTICAST, blood_count);
		WriteCoord (MSG_MULTICAST, blood_org_x);
		WriteCoord (MSG_MULTICAST, blood_org_y);
		WriteCoord (MSG_MULTICAST, blood_org_z);
		multicast (puff_org, MULTICAST_PVS);
	}
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
void(float damage, vector dir) TraceAttack =
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
		AddMultiDamage (trace_ent, damage);
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


/*
==============================================================================

ROCKETS

==============================================================================
*/

void() T_MissileTouch =
{
	local float     damg;

//	if (deathmatch == 4)
//	{
//	if ( ((other.weapon == 32) || (other.weapon == 16)))
//		{	
//			if (random() < 0.1)
//			{
//				if (other != world)
//				{
//	//				bprint (PRINT_HIGH, "Got here\n");
//					other.deathtype = "blaze";
//					T_Damage (other, self, self.owner, 1000 );
//					T_RadiusDamage (self, self.owner, 1000, other);
//				}
//			}
//		}	
//	}

	if (other == self.owner)
		return;         // don't explode on owner

	if (self.voided) {
		return;
	}
	self.voided = 1;

	if (pointcontents(self.origin) == CONTENT_SKY)
	{
		remove(self);
		return;
	}

	damg = 100 + random()*20;
	
	if (other.health)
	{
		other.deathtype = "rocket";
		T_Damage (other, self, self.owner, damg );
	}

	// don't do radius damage to the other, because all the damage
	// was done in the impact


	T_RadiusDamage (self, self.owner, 120, other, "rocket");

//  sound (self, CHAN_WEAPON, "weapons/r_exp3.wav", 1, ATTN_NORM);
	self.origin = self.origin - 8 * normalize(self.velocity);

	WriteByte (MSG_MULTICAST, SVC_TEMPENTITY);
	WriteByte (MSG_MULTICAST, TE_EXPLOSION);
	WriteCoord (MSG_MULTICAST, self.origin_x);
	WriteCoord (MSG_MULTICAST, self.origin_y);
	WriteCoord (MSG_MULTICAST, self.origin_z);
	multicast (self.origin, MULTICAST_PHS);

	remove(self);
};



/*
================
W_FireRocket
================
*/
void() W_FireRocket =
{
	DropAmmo ();
	self.attack_finished = time + 0.7;
	
	sound (self, CHAN_WEAPON, "weapons/sgun1.wav", 1, ATTN_NORM);

	msg_entity = self;
	WriteByte (MSG_ONE, SVC_SMALLKICK);

	newmis = spawn ();
	newmis.owner = self;
	newmis.movetype = MOVETYPE_FLYMISSILE;
	newmis.solid = SOLID_BBOX;
		
// set newmis speed     

	makevectors (self.v_angle);
	newmis.velocity = aim(self, 1000);
	newmis.velocity = newmis.velocity * 1000;
	newmis.angles = vectoangles(newmis.velocity);
	
	newmis.touch = T_MissileTouch;
	newmis.voided = 0;
	
// set newmis duration
	newmis.nextthink = time + 5;
	newmis.think = SUB_Remove;
	newmis.classname = "rocket";

	setmodel (newmis, "progs/missile.mdl");
	setsize (newmis, '0 0 0', '0 0 0');             
	setorigin (newmis, self.origin + v_forward*8 + '0 0 16');
};

/*
===============================================================================
LIGHTNING
===============================================================================
*/

void(entity from, float damage) LightningHit =
{
	WriteByte (MSG_MULTICAST, SVC_TEMPENTITY);
	WriteByte (MSG_MULTICAST, TE_LIGHTNINGBLOOD);
	WriteCoord (MSG_MULTICAST, trace_endpos_x);
	WriteCoord (MSG_MULTICAST, trace_endpos_y);
	WriteCoord (MSG_MULTICAST, trace_endpos_z);
	multicast (trace_endpos, MULTICAST_PVS);

	T_Damage (trace_ent, from, from, damage);
};

/*
=================
LightningDamage
=================
*/
void(vector p1, vector p2, entity from, float damage) LightningDamage =
{
	local entity            e1, e2;
	local vector            f;
	
	f = p2 - p1;
	normalize (f);
	f_x = 0 - f_y;
	f_y = f_x;
	f_z = 0;
	f = f*16;

	e1 = e2 = world;

	traceline (p1, p2, FALSE, self);

	if (trace_ent.takedamage)
	{
		LightningHit (from, damage);
/*
		if (self.classname == "player")
		{
			if (other.classname == "player")
				trace_ent.velocity_z = trace_ent.velocity_z + 400;
		}
*/
	}

	e1 = trace_ent;

	traceline (p1 + f, p2 + f, FALSE, self);
	if (trace_ent != e1 && trace_ent.takedamage)
	{
		LightningHit (from, damage);
	}
	e2 = trace_ent;

	traceline (p1 - f, p2 - f, FALSE, self);
	if (trace_ent != e1 && trace_ent != e2 && trace_ent.takedamage)
	{
		LightningHit (from, damage);
	}
};




void() W_FireLightning =
{
	local   vector          org;
	local   float           cells;

	if (self.ammo_cells < 1)
	{
		self.weapon = W_BestWeapon ();
		W_SetCurrentAmmo ();
		return;
	}

// explode if under water
	if (self.waterlevel > 1)
	{
		if (deathmatch > 3)
		{
			if (random() <= 0.5)
			{
				self.deathtype = "selfwater";
				T_Damage (self, self, self.owner, 4000 );
			}
			else
			{
				cells = self.ammo_cells;
				self.ammo_cells = 0;
				W_SetCurrentAmmo ();
				T_RadiusDamage (self, self, 35*cells, world, "");
				return;
			}
		}
		else
		{
			cells = self.ammo_cells;
			self.ammo_cells = 0;
			W_SetCurrentAmmo ();
			T_RadiusDamage (self, self, 35*cells, world,"");
			return;
		}
	}

	if (self.t_width < time)
	{
		sound (self, CHAN_WEAPON, "weapons/lhit.wav", 1, ATTN_NORM);
		self.t_width = time + 0.6;
	}
	msg_entity = self;
	WriteByte (MSG_ONE, SVC_SMALLKICK);

	if (deathmatch != 4)
		self.currentammo = self.ammo_cells = self.ammo_cells - 1;

	org = self.origin + '0 0 16';
	
	traceline (org, org + v_forward*600, TRUE, self);

	WriteByte (MSG_MULTICAST, SVC_TEMPENTITY);
	WriteByte (MSG_MULTICAST, TE_LIGHTNING2);
	WriteEntity (MSG_MULTICAST, self);
	WriteCoord (MSG_MULTICAST, org_x);
	WriteCoord (MSG_MULTICAST, org_y);
	WriteCoord (MSG_MULTICAST, org_z);
	WriteCoord (MSG_MULTICAST, trace_endpos_x);
	WriteCoord (MSG_MULTICAST, trace_endpos_y);
	WriteCoord (MSG_MULTICAST, trace_endpos_z);
	multicast (org, MULTICAST_PHS);

	LightningDamage (self.origin, trace_endpos + v_forward*4, self, 30);
};


//=============================================================================


void() GrenadeExplode =
{
	if (self.voided) {
		return;
	}
	self.voided = 1;

	T_RadiusDamage (self, self.owner, 120, world, "grenade");

	WriteByte (MSG_MULTICAST, SVC_TEMPENTITY);
	WriteByte (MSG_MULTICAST, TE_EXPLOSION);
	WriteCoord (MSG_MULTICAST, self.origin_x);
	WriteCoord (MSG_MULTICAST, self.origin_y);
	WriteCoord (MSG_MULTICAST, self.origin_z);
	multicast (self.origin, MULTICAST_PHS);

	remove (self);
};

void() GrenadeTouch =
{
	if (other == self.owner)
		return;         // don't explode on owner
	if (other.takedamage == DAMAGE_AIM)
	{
		GrenadeExplode();
		return;
	}
	sound (self, CHAN_WEAPON, "weapons/bounce.wav", 1, ATTN_NORM);  // bounce sound
	if (self.velocity == '0 0 0')
		self.avelocity = '0 0 0';
};

/*
================
W_FireGrenade
================
*/
void() W_FireGrenade =
{       
	if (deathmatch != 4)
		self.currentammo = self.ammo_rockets = self.ammo_rockets - 1;
	
	sound (self, CHAN_WEAPON, "weapons/grenade.wav", 1, ATTN_NORM);

	msg_entity = self;
	WriteByte (MSG_ONE, SVC_SMALLKICK);

	newmis = spawn ();
	newmis.voided=0;
	newmis.owner = self;
	newmis.movetype = MOVETYPE_BOUNCE;
	newmis.solid = SOLID_BBOX;
	newmis.classname = "grenade";
		
// set newmis speed     

	makevectors (self.v_angle);

	if (self.v_angle_x)
		newmis.velocity = v_forward*600 + v_up * 200 + crandom()*v_right*10 + crandom()*v_up*10;
	else
	{
		newmis.velocity = aim(self, 10000);
		newmis.velocity = newmis.velocity * 600;
		newmis.velocity_z = 200;
	}

	newmis.avelocity = '300 300 300';

	newmis.angles = vectoangles(newmis.velocity);
	
	newmis.touch = GrenadeTouch;
	
// set newmis duration
	if (deathmatch == 4)
	{
		newmis.nextthink = time + 2.5;		
		self.attack_finished = time + 1.1;
//		self.health = self.health - 1;
		T_Damage (self, self, self.owner, 10 );
	}
	else
		newmis.nextthink = time + 2.5;

	newmis.think = GrenadeExplode;

	setmodel (newmis, "progs/grenade.mdl");
	setsize (newmis, '0 0 0', '0 0 0');             
	setorigin (newmis, self.origin);
};


//=============================================================================

void() spike_touch;
void() superspike_touch;


/*
===============
launch_spike

Used for both the player and the ogre
===============
*/
void(vector org, vector dir) launch_spike =
{
	newmis = spawn ();
	newmis.voided=0;
	newmis.owner = self;
	newmis.movetype = MOVETYPE_FLYMISSILE;
	newmis.solid = SOLID_BBOX;

	newmis.angles = vectoangles(dir);
	
	newmis.touch = spike_touch;
	newmis.classname = "spike";
	newmis.think = SUB_Remove;
	newmis.nextthink = time + 6;
	setmodel (newmis, "progs/spike.mdl");
	setsize (newmis, VEC_ORIGIN, VEC_ORIGIN);               
	setorigin (newmis, org);

	newmis.velocity = dir * 1000;
};

void() W_FireSuperSpikes =
{
	local vector    dir;
	
	sound (self, CHAN_WEAPON, "weapons/spike2.wav", 1, ATTN_NORM);
	self.attack_finished = time + 0.2;
	if (deathmatch != 4) 
		self.currentammo = self.ammo_nails = self.ammo_nails - 2;
	dir = aim (self, 1000);
	launch_spike (self.origin + '0 0 16', dir);
	newmis.touch = superspike_touch;
	setmodel (newmis, "progs/s_spike.mdl");
	setsize (newmis, VEC_ORIGIN, VEC_ORIGIN);               
	msg_entity = self;
	WriteByte (MSG_ONE, SVC_SMALLKICK);
};

void(float ox) W_FireSpikes =
{
	local vector    dir;
	
	makevectors (self.v_angle);
	
	if (self.ammo_nails >= 2 && self.weapon == IT_SUPER_NAILGUN)
	{
		W_FireSuperSpikes ();
		return;
	}

	if (self.ammo_nails < 1)
	{
		self.weapon = W_BestWeapon ();
		W_SetCurrentAmmo ();
		return;
	}

	sound (self, CHAN_WEAPON, "weapons/rocket1i.wav", 1, ATTN_NORM);
	self.attack_finished = time + 0.2;
	if (deathmatch != 4)
		self.currentammo = self.ammo_nails = self.ammo_nails - 1;
	dir = aim (self, 1000);
	launch_spike (self.origin + '0 0 16' + v_right*ox, dir);

	msg_entity = self;
	WriteByte (MSG_ONE, SVC_SMALLKICK);
};



//.float hit_z;
void() spike_touch =
{
	if (other == self.owner)
		return;

	if (self.voided) {
		return;
	}
	self.voided = 1;

	if (other.solid == SOLID_TRIGGER)
		return; // trigger field, do nothing

	if (pointcontents(self.origin) == CONTENT_SKY)
	{
		remove(self);
		return;
	}
	
// hit something that bleeds
	if (other.takedamage)
	{
		spawn_touchblood (9);
		other.deathtype = "nail";
		T_Damage (other, self, self.owner, 9);
	}
	else
	{
		WriteByte (MSG_MULTICAST, SVC_TEMPENTITY);
		if (self.classname == "wizspike")
		{
			T_Damage (other, self, self.owner, 9+random()*18);
			WriteByte (MSG_MULTICAST, TE_WIZSPIKE);
		}
		else if (self.classname == "knightspike")
			WriteByte (MSG_MULTICAST, TE_KNIGHTSPIKE);
		else
			WriteByte (MSG_MULTICAST, TE_SPIKE);
		WriteCoord (MSG_MULTICAST, self.origin_x);
		WriteCoord (MSG_MULTICAST, self.origin_y);
		WriteCoord (MSG_MULTICAST, self.origin_z);
		multicast (self.origin, MULTICAST_PHS);
	}

	remove(self);

};

void() superspike_touch =
{
	if (other == self.owner)
		return;

	if (self.voided) {
		return;
	}
	self.voided = 1;


	if (other.solid == SOLID_TRIGGER)
		return; // trigger field, do nothing

	if (pointcontents(self.origin) == CONTENT_SKY)
	{
		remove(self);
		return;
	}
	
// hit something that bleeds
	if (other.takedamage)
	{
		spawn_touchblood (18);
		other.deathtype = "supernail";
		T_Damage (other, self, self.owner, 18);
	}
	else
	{
		WriteByte (MSG_MULTICAST, SVC_TEMPENTITY);
		WriteByte (MSG_MULTICAST, TE_SUPERSPIKE);
		WriteCoord (MSG_MULTICAST, self.origin_x);
		WriteCoord (MSG_MULTICAST, self.origin_y);
		WriteCoord (MSG_MULTICAST, self.origin_z);
		multicast (self.origin, MULTICAST_PHS);
	}

	remove(self);

};


/*
===============================================================================

PLAYER WEAPON USE

===============================================================================
*/

void() SetWeaponModel =
{
	self.weaponmodel = GetItemVModel(ToIID(ItemInSlot(self, self.current_slot)));
};

void() W_SetCurrentAmmo =
{
	local string x;

	player_run ();          // get out of any weapon firing states

	self.items = self.items - ( self.items & (IT_SHELLS | IT_NAILS | IT_ROCKETS | IT_CELLS) );
	

/*SPIKEREMOVED
	if (self.current_slot == 1)
		self.currentammo = ToStatus(self.islot1);
	else if (self.current_slot == 2)
		self.currentammo = ToStatus(self.islot2);
	else if (self.current_slot == 4)
	{
		if (self.team == 1)
		{
			sound (self, CHAN_WEAPON, "misc/tools.wav", 1, ATTN_NORM);
			self.items = (self.items | IT_LIGHTNING);
			self.currentammo = 0;
			self.weaponmodel = "";
		}
		else
			sound (self, CHAN_WEAPON, "misc/menu3.wav", 1, ATTN_NORM);
	}
*/
//	else
		sound (self, CHAN_WEAPON, "misc/menu3.wav", 1, ATTN_NORM);

		if (self.current_slot == 1)
			x = GetItemName(ToIID(self.islot1));
		if (self.current_slot == 2)
			x = GetItemName(ToIID(self.islot2));
/*SPIKEREMOVED
		if (self.current_slot == 3)
			x = "grenade";
		if (self.current_slot == 4)
			x = "tools";
*/

		sprint(self, 2, x);
		sprint (self, PRINT_HIGH, " selected.\n");
		sound (self, CHAN_WEAPON, "misc/weapon.wav", 1, ATTN_NORM);
		SetWeaponModel();

	self.currentammo = ToStatus(ItemInSlot(self, self.current_slot));
};

float() W_BestWeapon =
{
	local   float   it;
	
	it = self.items;

	if (self.waterlevel <= 1 && self.ammo_cells >= 1 && (it & IT_LIGHTNING) )
		return IT_LIGHTNING;
	else if(self.ammo_nails >= 2 && (it & IT_SUPER_NAILGUN) )
		return IT_SUPER_NAILGUN;
	else if(self.ammo_shells >= 2 && (it & IT_SUPER_SHOTGUN) )
		return IT_SUPER_SHOTGUN;
	else if(self.ammo_nails >= 1 && (it & IT_NAILGUN) )
		return IT_NAILGUN;
	else if(self.ammo_shells >= 1 && (it & IT_SHOTGUN)  )
		return IT_SHOTGUN;
		
/*
	if(self.ammo_rockets >= 1 && (it & IT_ROCKET_LAUNCHER) )
		return IT_ROCKET_LAUNCHER;
	else if(self.ammo_rockets >= 1 && (it & IT_GRENADE_LAUNCHER) )
		return IT_GRENADE_LAUNCHER;

*/

	return IT_AXE;
};

void (float slotno) ReloadWeapon =
{
	local float at;
	local float as;
	local float asv;
	local float ac;
	local float x;
	local float wid;

	local .float wslot;

	if (self.rtime > time || self.attack_finished > time)
		return;

	wslot = SlotField(slotno);

	if (self.wslot == 0)
		return;	//already empty..

	wid = ToIID(self.wslot);
	at = WeaponAmmoType(wid);
	as = SlotOfItem(self, at);
	if (as == 0)
	{
		sprint(self, 2, "no suitable ammo\n");
		return;
	}
	asv = ItemInSlot(self, as);
	ac = ToStatus(asv);

	x = WeaponMagQuant(wid);	//get the max ammo capacity

	if (ToStatus(self.wslot) == x) //weapon full already
		return;

	x = x - ToStatus(self.wslot);	//take away the ammo currently in the weapon
	if (x > ac)	//make sure there's enough ammo
		x = ac;
	ac -= x;

	if (ac)
		asv = SlotVal(at, ac);
	else
		asv = 0;	//totally used up.

	SetItemSlot(self, as, asv);

	self.wslot = SlotVal(wid, x + ToStatus(self.wslot));

	if (wid == IID_WP_ROCKETLAUNCHER || wid == IID_WP_PIPERIFLE || wid == IID_WP_WINCHESTER || wid == IID_WP_MOSSBERG)
	{
		sound (self, CHAN_WEAPON, "weapons/shell.wav", TRUE, ATTN_NORM);
		self.attack_finished = time + 1;
		self.rtime = time + 1;
		if (self.current_slot == slotno)
			self.currentammo = ToStatus(ItemInSlot(self, slotno));

		return;
	}
	else
		sound (self, CHAN_WEAPON, "weapons/reload.wav", TRUE, ATTN_NORM);

	sprint(self, 2, "reloading...\n");

	if (self.perk == 3)
	{
		self.attack_finished = time + 1;
		self.rtime = time + 1;
	}
	else
	{
		self.attack_finished = time + 2;
		self.rtime = time + 2;
	}

	if (self.current_slot == slotno)
		self.currentammo = ToStatus(ItemInSlot(self, slotno));

	player_run();
};


float() W_CheckNoAmmo =
{
	local float at;
	local .float slotfield;
	slotfield = SlotField(self.current_slot);

	if (ToIID(self.slotfield) == IID_NONE)
		return FALSE;

	if (ToStatus(self.slotfield) < 1)
	{
		at = WeaponAmmoType(ToIID(self.slotfield));
		if (at == 0)
			return FALSE;

		at = SlotOfItem(self, at);
		if (at == 0)
		{
			self.attack_finished = (time + 0.2);
			stuffcmd (self, "-attack\n");
			sound (self, CHAN_WEAPON, "weapons/click.wav", TRUE, ATTN_NORM);
			return TRUE;
		}
		else
		{
			stuffcmd (self, "-attack\n");
			ReloadWeapon (self.current_slot);
			return TRUE;
		}
	}

	return FALSE;
};

/*
============
W_Attack

An attack impulse can be triggered now
============
*/

void() W_Attack =
{
	local float weap;

	makevectors     (self.v_angle);                 // calculate forward angle for velocity
	self.show_hostile = time + 1;   // wake monsters up

	if (self.rtime > time)
		return;
	if (self.attack_finished > time)
		return;

	if (W_CheckNoAmmo())
		return;

	weap = ToIID(self.(SlotField(self.current_slot)));


	if (weap == IID_NONE ||
	    weap == IID_WP_KNIFE ||
	    weap == IID_WP_AXE ||
	    weap == IID_WP_VIBROBLADE ||
	    weap == IID_WP_POWERAXE)
	{
		self.attack_finished = time + 0.50;
		player_knife1 ();
	}
	else if (weap == IID_WP_USP)
		FirePistol(12, 2, "weapons/1911.wav", 2000, 0.25);
	else if (weap == IID_WP_DEAGLE)
		FirePistol(15, 2, "weapons/deagle.wav", 2000, 0.25);
	else if (weap == IID_WP_NEEDLER)
		FirePistol(10, 2, "weapons/needler.wav", 2000, 0.25);
	else if (weap == IID_WP_ALIENBLASTER)
		FireAlienBlaster();
	else if (weap == IID_WP_PIPERIFLE)
		FireAssaultRifle(18, 2, "weapons/rangem.wav", 3000, 0.1);
	else if (weap == IID_WP_WINCHESTER)
		W_FireShotgun (1, 5, 6, 160, 3000, 0);
	else if (weap == IID_WP_MOSSBERG)
		W_FireShotgun (1, 5, 6, 160, 3000, 0);
	else if (weap == IID_WP_JACKHAMMER)
		W_FireShotgun (1, 5, 6, 160, 3000, 1);
	else if (weap == IID_WP_MP9)
		FireSMG(12, 2, "weapons/burst.wav", 2000, 0.09);
	else if (weap == IID_WP_MP7)
		FireSMG(12, 2, "weapons/mp7.wav", 2000, 0.09);
	else if (weap == IID_WP_RANGEMASTER)
		FireAssaultRifle(14, 2, "weapons/rangem.wav", 4000, 0.5);
	else if (weap == IID_WP_AK112)
		FireAssaultRifle(14, 2, "weapons/ak112.wav", 4000, 0.09);
	else if (weap == IID_WP_AK74)
		FireAssaultRifle(18, 2, "weapons/ak47.wav", 4000, 0.1);
	else if (weap == IID_WP_DKS1)
		FireAssaultRifle(24, 2, "weapons/dks-1.wav", 8000, 0.5);
	else if (weap == IID_WP_MOONLIGHT)
		FireAssaultRifle(14, 2, "weapons/m4-nw.wav", 4000, 0.09);
	else if (weap == IID_WP_SA80)
		FireAssaultRifle(17, 2, "weapons/sa80.wav", 4000, 0.08);
	else if (weap == IID_WP_FNFAL)
		FireAssaultRifle(12, 3, "weapons/fnfal.wav", 6000, 0.12);

//float IID_WP_GAUSERIFLE = 421;
//float IID_WP_PULSERIFLE = 422;

	else if (weap == IID_GREN_FRAG)
		player_throw1();
	else if (weap == IID_GREN_EMP)
		player_throw1();
	else if (weap == IID_GREN_SMOKE)
		player_throw1();
	else if (weap == IID_GREN_FLASH)
		player_throw1();

	else if (weap == IID_WP_ROCKETLAUNCHER)
		W_FireRocket();

	else if (weap == IID_WP_TOOLKIT)
	{
		if (!FireToolkit())
		{
			self.attack_finished = time + 0.25;
			player_knife1 ();
		}
	}
	else if (IsChem(weap))
	{
		self.attack_finished = time + 0.25;
		player_knife1 ();
	}
	else
		centerprint(self, "Not implemented (", ftos(weap), ")");
};

/*
============
W_ChangeWeapon

============
*/
void() W_ChangeWeapon =
{
	local   float   it, am, fl;
	
	it = self.items;
	am = 0;
	
	if (self.impulse == 1)
	{
		fl = IT_NAILGUN;
		self.current_slot = 1;
	}
	if (self.impulse == 2)
	{
		fl = IT_SUPER_NAILGUN;
		self.current_slot = 2;
	}

	self.weapon = fl;               
	W_SetCurrentAmmo ();
};

/*
============
CheatCommand
============
*/
void() CheatCommand =
{

};

/*
============
CycleWeaponCommand

Go to the next weapon with ammo
============
*/
void() CycleWeaponCommand =
{
	local   float   it, am;
	
	it = self.items;
	self.impulse = 0;

	while (1)
	{
		am = 0;

		if (self.weapon == IT_LIGHTNING)
		{
			self.weapon = IT_AXE;
		}
		else if (self.weapon == IT_AXE)
		{
			self.weapon = IT_SHOTGUN;
			if (self.ammo_shells < 1)
				am = 1;
		}
		else if (self.weapon == IT_SHOTGUN)
		{
			self.weapon = IT_SUPER_SHOTGUN;
			if (self.ammo_shells < 2)
				am = 1;
		}               
		else if (self.weapon == IT_SUPER_SHOTGUN)
		{
			self.weapon = IT_NAILGUN;
			if (self.ammo_nails < 1)
				am = 1;
		}
		else if (self.weapon == IT_NAILGUN)
		{
			self.weapon = IT_SUPER_NAILGUN;
			if (self.ammo_nails < 2)
				am = 1;
		}
		else if (self.weapon == IT_SUPER_NAILGUN)
		{
			self.weapon = IT_GRENADE_LAUNCHER;
			if (self.ammo_rockets < 1)
				am = 1;
		}
		else if (self.weapon == IT_GRENADE_LAUNCHER)
		{
			self.weapon = IT_ROCKET_LAUNCHER;
			if (self.ammo_rockets < 1)
				am = 1;
		}
		else if (self.weapon == IT_ROCKET_LAUNCHER)
		{
			self.weapon = IT_LIGHTNING;
			if (self.ammo_cells < 1)
				am = 1;
		}
	
		if ( (self.items & self.weapon) && am == 0)
		{
			W_SetCurrentAmmo ();
			return;
		}
	}

};




void () ProneOff =
{
	setsize(self, '-16 -16 -24', '16 16 32');

	if (!walkmove(0, 0))
	{
		sprint (self, 2, "not enough room to stand up.\n");
		setsize(self, '-16 -16 -24', '16 16 0');
		return;
	}

	sprint (self, 2, "position: stand.\n");
	self.position = 0;
	PositionControl();
	player_run ();
};

void () ProneOn =
{
	if (self.velocity_z != 0)
		return;

	if (self.position == 2)
	{
		ProneOff();
		return;
	}

	self.maxspeed = (self.maxspeed * 0.25);
	self.position = 2;
	self.view_ofs = '0 0 -10';
	sprint (self, 2, "position: prone.\n");
	PositionControl();
};


void () DuckOff =
{
	setsize(self, '-16 -16 -24', '16 16 32');

	if (!walkmove(0, 0))
	{
		sprint (self, 2, "not enough room to stand up.\n");
		setsize(self, '-16 -16 -24', '16 16 16');
		return;
	}
	sprint (self, 2, "position: stand.\n");
	self.view_ofs = '0 0 22';
	self.position = 0;
	PositionControl();
	player_run ();
};


void () DuckOn =
{
	if (self.velocity_z != 0)
		return;

	if (self.position == 1)
	{
		DuckOff();
		return;
	}

	self.maxspeed = (self.maxspeed * 0.50);
	self.position = 1;
	sprint (self, 2, "position: duck.\n");
	PositionControl();
};


/*
============
CycleWeaponReverseCommand

Go to the prev weapon with ammo
============
*/
void() CycleWeaponReverseCommand =
{
	local   float   it, am;
	
	it = self.items;
	self.impulse = 0;

	while (1)
	{
		am = 0;

		if (self.weapon == IT_LIGHTNING)
		{
			self.weapon = IT_ROCKET_LAUNCHER;
			if (self.ammo_rockets < 1)
				am = 1;
		}
		else if (self.weapon == IT_ROCKET_LAUNCHER)
		{
			self.weapon = IT_GRENADE_LAUNCHER;
			if (self.ammo_rockets < 1)
				am = 1;
		}
		else if (self.weapon == IT_GRENADE_LAUNCHER)
		{
			self.weapon = IT_SUPER_NAILGUN;
			if (self.ammo_nails < 2)
				am = 1;
		}
		else if (self.weapon == IT_SUPER_NAILGUN)
		{
			self.weapon = IT_NAILGUN;
			if (self.ammo_nails < 1)
				am = 1;
		}
		else if (self.weapon == IT_NAILGUN)
		{
			self.weapon = IT_SUPER_SHOTGUN;
			if (self.ammo_shells < 2)
				am = 1;
		}               
		else if (self.weapon == IT_SUPER_SHOTGUN)
		{
			self.weapon = IT_SHOTGUN;
			if (self.ammo_shells < 1)
				am = 1;
		}
		else if (self.weapon == IT_SHOTGUN)
		{
			self.weapon = IT_AXE;
		}
		else if (self.weapon == IT_AXE)
		{
			self.weapon = IT_LIGHTNING;
			if (self.ammo_cells < 1)
				am = 1;
		}
	
		if ( (it & self.weapon) && am == 0)
		{
			W_SetCurrentAmmo ();
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
	serverflags = serverflags * 2 + 1;
};


/*
============
ImpulseCommands

============
*/
void() ImpulseCommands =
{
	if (self.impulse >= 1 && self.impulse <= 4 && self.currentmenu == "none")
		W_ChangeWeapon ();

	if (self.impulse >= 1 && self.impulse <= 10 && self.currentmenu != "none")
		W_PlayerMenu ();

	if (self.impulse == 11)
		ServerflagsCommand ();
	if (self.impulse == 12)
		CycleWeaponReverseCommand ();
	if (self.impulse == 200)
		DuckOn ();
	if (self.impulse == 201)
		ProneOn ();

	if (self.impulse == 50)
		ReloadWeapon (self.current_slot);
//	if (self.impulse == 51)
//		UseChem ();
	if (self.impulse == 52)
		BuyMenu ();
	if (self.impulse == 53)
		Special ();
	if (self.impulse == 54)
		ExitScreen ();
	if (self.impulse == 55)
		DropFromSlot (self.current_slot, 1, 0);
	if (self.impulse == 56)
		CharacterSheet ();
	if (self.impulse == 57)
		UseEquipment ();

	if (self.impulse == 255)
		self.ammo_shells = 200;

	self.impulse = 0;
};

/*
============
W_WeaponFrame

Called every frame so impulse events can be handled as well as possible
============
*/
void() W_WeaponFrame =
{
	if (time < self.attack_finished)
		return;

	ImpulseCommands ();
	
// check for attack
	if (self.button0)
	{
		SuperDamageSound ();
		W_Attack ();
	}
	if (!self.button0)
	{
		if (self.attack > 0)
			player_run ();

		self.attack = 0;
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


void () DropAmmo =
{
	local .float wslot;

	wslot = SlotField(self.current_slot);

	self.currentammo = (ToStatus(self.wslot) )- 1;
	self.wslot = SlotVal(ToIID(self.wslot), self.currentammo);
	self.currentammo = ToStatus(self.wslot);
};

void() muzzleflash =
{
	WriteByte (MSG_MULTICAST, SVC_MUZZLEFLASH);
	WriteEntity (MSG_MULTICAST, self);
	multicast (self.origin, MULTICAST_PVS);
};

void () autofire =
{
	if (self.frame == 88)
		self.frame = 89;
	else
		self.frame = 88;


	if (self.weaponframe == 1)
		self.weaponframe = 2;
	else
		self.weaponframe = 1;

	muzzleflash ();
};

void () autofire_s =
{
	if (self.frame == 88)
		self.frame = 89;
	else
		self.frame = 88;

	if (self.weaponframe == 1)
		self.weaponframe = 2;
	else
		self.weaponframe = 1;

	muzzleflash ();
};


void () player_single1 = [ 88, player_single2 ]
{
	self.weaponframe = 1;
	muzzleflash ();
};

void () player_single2 = [ 89, player_run ]
{
	self.weaponframe = 2;
};

void () player_single1_s = [ 183, player_single2_s ]
{
	self.weaponframe = 1;
	muzzleflash ();
};

void () player_single2_s = [ 184, player_run ]
{
	self.weaponframe = 2;
};

void (vector org) bullet_hole =
{
	local float r;
	local entity ric;

	WriteByte (MSG_MULTICAST, SVC_TEMPENTITY);
	WriteByte (MSG_MULTICAST, TE_SPIKE);
	WriteCoord (MSG_MULTICAST, org_x);
	WriteCoord (MSG_MULTICAST, org_y);
	WriteCoord (MSG_MULTICAST, org_z);
	multicast (self.origin, MULTICAST_PHS);

	r = random();
	ric = spawn();
	setorigin(ric, org);

	if (r <= 0.20)
		sound (ric, CHAN_WEAPON, "weapons/ric1.wav", TRUE, ATTN_NORM);
	else if (r <= 0.40)
		sound (ric, CHAN_WEAPON, "weapons/ric2.wav", TRUE, ATTN_NORM);
	else if (r <= 0.60)
		sound (ric, CHAN_WEAPON, "weapons/ric3.wav", TRUE, ATTN_NORM);
	else if (r <= 0.80)
		sound (ric, CHAN_WEAPON, "weapons/ric4.wav", TRUE, ATTN_NORM);
	else
		sound (ric, CHAN_WEAPON, "weapons/ric5.wav", TRUE, ATTN_NORM);

	remove(ric);
};

void (vector test, float length, float dam) penetrate =
{
	local vector org;
	local vector start;
	local vector end;
	local float go;
	local float tl;


	go = 0;
	tl = 8;

	length = 32 + dam;

	while (tl < length)
	{
		makevectors (self.v_angle);
		start = (test + v_forward*tl);

		if (pointcontents (start) != CONTENT_SOLID && go == 0) //object penetrated
		{
				makevectors (self.v_angle);
				end = (test + (v_forward * 8 * length));
				traceline (start, end, FALSE, self);
				if (trace_fraction == 1) //nothing behind object
					return;

				if (trace_fraction > 0)
				{
					go = 1;

					if (trace_ent.takedamage)
					{
                                                
						if (trace_ent.solid != SOLID_BSP)
							SpawnBlood (org, 1);
	
						T_Damage (trace_ent, self, self, dam);
					}
					else
					{
						WriteByte (MSG_MULTICAST, SVC_TEMPENTITY);
						WriteByte (MSG_MULTICAST, TE_SPIKE);
						WriteCoord (MSG_MULTICAST, trace_endpos_x);
						WriteCoord (MSG_MULTICAST, trace_endpos_y);
						WriteCoord (MSG_MULTICAST, trace_endpos_z);
						multicast (trace_endpos, MULTICAST_PHS);
					}
				}
		}
		tl = tl + 4;
}
};

void (entity temp, vector org, float damage) SpawnWood =
{

		if (random()*6 <= 3)
			sound (temp, CHAN_WEAPON, "misc/woodhit.wav", TRUE, ATTN_NORM);
		else
			sound (temp, CHAN_WEAPON, "misc/woodhit2.wav", TRUE, ATTN_NORM);

		WriteByte (MSG_MULTICAST, SVC_TEMPENTITY);
		WriteByte (MSG_MULTICAST, TE_SUPERSPIKE);
		WriteCoord (MSG_MULTICAST, org_x);
		WriteCoord (MSG_MULTICAST, org_y);
		WriteCoord (MSG_MULTICAST, org_z);
		multicast (self.origin, MULTICAST_PHS);
};

void () EMPExplode =
{
	local entity te;

	self.velocity = VEC_ORIGIN;
	WriteByte (MSG_BROADCAST, SVC_TEMPENTITY);
	WriteByte (MSG_BROADCAST, TE_TAREXPLOSION);
	WriteCoord (MSG_BROADCAST, self.origin_x);
	WriteCoord (MSG_BROADCAST, self.origin_y);
	WriteCoord (MSG_BROADCAST, self.origin_z);
	sound (self, CHAN_BODY, "misc/flash.wav", 1, ATTN_NORM);
	sound (self, CHAN_BODY, "misc/flash.wav", 0.2, ATTN_NONE);
	te = findradius (self.origin, 300);
	while (te)
	{
		if ((((te.classname == "camera") || (te.classname == "alarm")) && (te.owner.pcamera == 0)))
		{
			te.owner.pcamera = 0;
			te.owner.pcamera2 = 0;
			te.owner.equipment_slot = 0;
			sprint (self.owner, 2, te.owner.netname);
			sprint (self.owner, 2, "'s ");
			sprint (self.owner, 2, te.classname);
			sprint (self.owner, 2, " was wiped out!\n");
			remove (te);
		}
		te = te.chain;
	}
	T_RadiusDamage (self, self.owner, 45+random()*45, other, "");
	remove (self);
};

void (vector org) CreateSmoke =
{
	newmis = spawn ();
	setorigin (newmis, org);
	newmis.movetype = MOVETYPE_NONE;
	newmis.solid = SOLID_NOT;
	newmis.velocity = VEC_ORIGIN;
	newmis.nextthink = (time + SVC_BIGKICK);
	newmis.think = SUB_Remove;
	newmis.touch = SUB_Null;
	newmis.classname = "smoke";
	newmis.frame = 0;
	newmis.cnt = 0;
	newmis.avelocity_x = (random () * 100);
	newmis.avelocity_y = (random () * 100);
	setsize (newmis, VEC_ORIGIN, VEC_ORIGIN);
};


void () SmokeThink =
{
	self.cnt = (self.cnt + 1);

	WriteByte (MSG_MULTICAST, SVC_TEMPENTITY);
	WriteByte (MSG_MULTICAST, TE_GUNSHOT);
	WriteByte (MSG_MULTICAST, 2);
	WriteCoord (MSG_MULTICAST, self.origin_x);
	WriteCoord (MSG_MULTICAST, self.origin_y);
	WriteCoord (MSG_MULTICAST, self.origin_z);
	multicast (self.origin, MULTICAST_PVS);

	self.nextthink = (time + 0.33);
	if (self.cnt >= 90)
		remove (self);
};

void () FragExplode =
{
	local float r;

	sound (self, CHAN_VOICE, "ambience/gunfire7.wav", 1, ATTN_NONE);
	self.origin = (self.origin + '0 0 16');
	WriteByte (MSG_BROADCAST, SVC_TEMPENTITY);
	WriteByte (MSG_BROADCAST, TE_EXPLOSION);
	WriteCoord (MSG_BROADCAST, self.origin_x);
	WriteCoord (MSG_BROADCAST, self.origin_y);
	WriteCoord (MSG_BROADCAST, self.origin_z);
	r = random ();
	if ((r < 0.3))
	{
		sound (self, CHAN_BODY, "misc/exp1.wav", 1, ATTN_NORM);
	}
	if ((r < 0.65))
	{
		sound (self, CHAN_BODY, "misc/exp2.wav", 1, ATTN_NORM);
	}
	else
	{
		sound (self, CHAN_BODY, "misc/exp3.wav", 1, ATTN_NORM);
	}
	T_RadiusDamage (self, self.owner, 50+random()*50, other, "");
	remove (self);
};


void () PlasmaExplode =
{
	sound (self, CHAN_VOICE, "ambience/gunfire7.wav", 1, ATTN_NONE);
	self.origin = (self.origin + '0 0 16');
	self.velocity = VEC_ORIGIN;
	WriteByte (MSG_BROADCAST, SVC_TEMPENTITY);
	WriteByte (MSG_BROADCAST, TE_TAREXPLOSION);
	WriteCoord (MSG_BROADCAST, self.origin_x);
	WriteCoord (MSG_BROADCAST, self.origin_y);
	WriteCoord (MSG_BROADCAST, self.origin_z);
	sound (self, CHAN_BODY, "misc/flash.wav", 1, ATTN_NORM);
	T_RadiusDamage (self, self.owner, 80+random()*80, other, "");
	remove (self);
};

void () FlashExplode =
{
	local entity te;
	local float dot;
	local vector vec;

	self.velocity = VEC_ORIGIN;
	setmodel (self, "progs/blast.mdl");
	WriteByte (0, SVC_TEMPENTITY);
	WriteByte (0, WEAPON_SPIKES);
	WriteCoord (0, self.origin_x);
	WriteCoord (0, self.origin_y);
	WriteCoord (0, self.origin_z);
	sound (self, CHAN_BODY, "misc/flash.wav", 1, ATTN_NORM);
	te = findradius (self.origin, 1200);
	while (te)
	{
		if (te.flags & FL_MONSTER && te.classname != te.owner.classname)
			te.attack_finished = time + 6;

		if ((te.classname == "player"))
		{
/*			makevectors (te.angles);
			vec = normalize ((self.origin - te.origin));
			dot = (vec * v_forward);
*/
			if (/*dot > 0.3 &&*/ CanDamage (self, te))
			{
				stuffcmd (te, "v_cshift 255 255 255 255\n");
				stuffcmd (te, "v_idlescale 10\n");
				if (self.owner.flags & FL_MONSTER)
					te.flash = time + 2;
				else
					te.flash = time + 6;
			}
		}
		te = te.chain;
	}
	remove (self);
};

/*
void () HandGrenExplode =
{
	if ((self.cnt == 0))
	{
		FragExplode ();
	}
	else
	{
		if ((self.cnt == 1))
		{
			EMPExplode ();
		}
		else
		{
			if ((self.cnt == 2))
			{
				self.nextthink = (time + 0.5);
				self.think = SmokeThink;
			}
			else
			{
				if ((self.cnt == AS_MELEE))
				{
					FlashExplode ();
				}
				else
				{
					if ((self.cnt == WEAPON_SPIKES))
					{
						PlasmaExplode ();
					}
				}
			}
		}
	}
};
*/

void () HandGrenBounce =
{
	local float r;

	r = (random () * TE_LIGHTNING3);

	self.velocity = self.velocity * 0.75;

	if ((r < AS_MELEE))
	{
		sound (self, CHAN_VOICE, "misc/bounce_1.wav", 0.9, ATTN_NORM);
	}
	else
	{
		if ((r < TE_LIGHTNING2))
		{
			sound (self, CHAN_VOICE, "misc/bounce_2.wav", 0.9, ATTN_NORM);
		}
		else
		{
			sound (self, CHAN_VOICE, "misc/bounce_3.wav", 0.9, ATTN_NORM);
		}
	}
};


void () FireHandGrenade =
{
	local float item, iid;
	local float amcount;

	item = ItemInSlot(self, self.current_slot);
	iid = ToIID(item);
	amcount = ToStatus(item);
	if (iid == 0)
		return;
	
	self.currentammo = amcount-1;
	if (amcount <= 1)
		SetItemSlot(self, self.current_slot, 0);
	else
		SetItemSlot(self, self.current_slot, SlotVal(iid, self.currentammo));




	msg_entity = self;
	WriteByte (MSG_ONE, SVC_SMALLKICK);
	newmis = spawn ();
	newmis.owner = self;
	newmis.movetype = MOVETYPE_BOUNCE;
	newmis.solid = SOLID_BBOX;
	newmis.classname = "grenade";

	newmis.skin = 0;
	makevectors (self.v_angle);
	newmis.velocity = aim (self, 800);
	newmis.velocity = (newmis.velocity * 800);
	newmis.velocity_z = (newmis.velocity_z + 200);
	newmis.angles = vectoangles (newmis.velocity);
	newmis.avelocity_x = (random () * 300);
	newmis.avelocity_y = (random () * 300);
	newmis.avelocity_z = (random () * 300);
	newmis.touch = HandGrenBounce;
	newmis.nextthink = (time + 2.5);

	if (iid == IID_GREN_FRAG)
		newmis.think = FragExplode;
	else if (iid == IID_GREN_EMP)
		newmis.think = EMPExplode;
	else if (iid == IID_GREN_SMOKE)
		newmis.think = SmokeThink;
	else //if (iid == IID_GREN_FLASH)
		newmis.think = FlashExplode;

	newmis.frame = 1;
	setmodel (newmis, "progs/handgren.mdl");
	setsize (newmis, VEC_ORIGIN, VEC_ORIGIN);
	setorigin (newmis, ((self.origin + (v_right * TE_BLOOD)) + (v_up * TE_BLOOD)));
};



void(float slot, float magazine) GiveAmmo =
{
	local float wi;
	local float wt;
	local float at;
	local float as;

	//give ammo to the weapon in a slot
	wi = ItemInSlot(self, slot);
	wt = ToIID(wi);
	SetItemSlot(self, slot, SlotVal(wt, magazine));

	magazine = magazine*4;
	if (magazine < 30)
		magazine = 30;	//be generous.

	at = WeaponAmmoType(wt);
	//make sure we have enough ammo
	as = SlotOfItem(self, at);
	if (as == 0)
		as = SlotOfItem(self, IID_NONE);
	if (as == 0)
		return;	//no free slots, so we can't give them ammo
	if (ToStatus(ItemInSlot(self, as)) < magazine)
		SetItemSlot(self, as, SlotVal(at, magazine));
};


void (float dam, float rec, string snd, float rng, float rate) FirePistol =
{
      local float weap, tmp, zdif, xdif, ydif, is_headshot;
      local vector dir, source, targ, org, org2, org3, adjust;

	weap = ToIID(self.(SlotField(self.current_slot)));

	stuffcmd(self, "-attack\n");
	sound (self, CHAN_WEAPON, snd, 1, ATTN_NORM);
	self.attack_finished = (time + rate);


	if (self.position == POS_STAND)
		player_single1 ();
	if (self.position >= POS_DUCK)
		player_single1_s ();

	if (self.position == 0)
		adjust = '0 0 0';
	if (self.position == 1)
		adjust = '0 0 -16';
	if (self.position == 2)
		adjust = '0 0 -32';


	DropAmmo ();
	makevectors (self.v_angle);
	if (self.recoil >= 15)
		self.recoil = 15;


	tmp = 50;

	if (self.velocity != '0 0 0')
		tmp = 400;

	tmp = tmp + (40 * self.recoil);

	if (self.attack <= 3 && self.position == 1 && self.velocity_z == 0)
		tmp = (tmp * 0.75);

	if (self.attack <= 3 && self.position == 2 && self.velocity_z == 0)
		tmp = (tmp * 0.5);

	self.attack = self.attack + 1;
	self.recoil = self.recoil + 4;

	source = self.origin + '0 0 22';

	targ = self.origin + '0 0 22' + v_right*crandom()* tmp + v_up*crandom()*tmp;

	traceline (source+adjust, targ+adjust+v_forward*4000, FALSE, self);
	if (trace_fraction == 1)
		return;

	org3 = self.origin + v_forward*32 + '0 0 22';

	org = trace_endpos - v_forward * 1;
	org2 = trace_endpos + (v_forward * ((trace_ent.size_y / 2) + (trace_ent.size_x / 2)));
	if (trace_ent.takedamage)
	{
		org2 = (trace_endpos + (v_forward * ((trace_ent.size_y / 2) + (trace_ent.size_x / 2))));
		zdif = org_z - trace_ent.origin_z;
		ydif = org2_y - trace_ent.origin_y;
		xdif = org2_x - trace_ent.origin_x;


		is_headshot = 0;

		if (((ydif >= CONTENT_SKY) && (ydif <= TE_LIGHTNING2)))
			is_headshot = 1;
		if (((xdif >= CONTENT_SKY) && (xdif <= TE_LIGHTNING2)))
			is_headshot = 1;
		if (self.attack <= 5 && is_headshot == 1 && zdif >= (trace_ent.size_z / 2 * 0.8))
			self.critical = 3;

		dam = (dam * (1 - trace_fraction));
		if (trace_ent.solid != SOLID_BSP)
			SpawnBlood (org, 1);

		if (trace_ent.solid == SOLID_BSP)
			SpawnWood (trace_ent, org, 1);

		dam = dam + random()*dam;
		if (weap == IID_WP_NEEDLER)
			X_Damage (trace_ent, self, self, dam);
		else
			T_Damage (trace_ent, self, self, dam);

		self.critical = 0;
	}
	else
	{
		bullet_hole (org);
		dir = vectoangles (source - targ);
		return;
	}
};

void (float dam, float rec, string snd, float rng, float rate) FireSMG =
{
      local float weap, tmp, zdif, xdif, ydif, is_headshot;
      local vector dir, source, targ, org, org2, adjust;

	weap = ToIID(self.(SlotField(self.current_slot)));

	if (weap != IID_WP_MP9)
		sound (self, CHAN_WEAPON, snd, 1, ATTN_NORM);
	if (weap == IID_WP_MP9 && self.attack == 0)
		sound (self, CHAN_WEAPON, snd, 1, ATTN_NORM);
	if (weap == IID_WP_MP9 && self.attack == 6)
	{
		sound (self, CHAN_WEAPON, snd, 1, ATTN_NORM);
		self.attack = 1;
	}

	self.attack_finished = (time + rate);


	if (self.position == POS_STAND)
		autofire ();
	if (self.position >= POS_DUCK)
		autofire_s ();

	if (self.position == 0)
		adjust = '0 0 0';
	if (self.position == 1)
		adjust = '0 0 -16';
	if (self.position == 2)
		adjust = '0 0 -32';


	DropAmmo ();
	makevectors (self.v_angle);
	if (self.recoil >= 15)
		self.recoil = 15;

	tmp = 200;

	if (self.velocity != '0 0 0')
		tmp = 200;

	tmp = tmp + (40 * self.recoil);

	if (self.attack <= 3 && self.position == 1 && self.velocity_z == 0)
		tmp = (tmp * 0.75);

	if (self.attack <= 3 && self.position == 2 && self.velocity_z == 0)
		tmp = (tmp * 0.5);

	self.attack = self.attack + 1;
	self.recoil = self.recoil + 3;

	source = self.origin + '0 0 22';

	targ = self.origin + '0 0 22' + v_right*crandom()* tmp + v_up*crandom()*tmp;

	traceline (source+adjust, targ+adjust+v_forward*4000, FALSE, self);
	if (trace_fraction == 1)
		return;

	org = trace_endpos - v_forward * 2;
	org2 = trace_endpos + (v_forward * ((trace_ent.size_y / 2) + (trace_ent.size_x / 2)));
	if (trace_ent.takedamage)
	{
		org2 = (trace_endpos + (v_forward * ((trace_ent.size_y / 2) + (trace_ent.size_x / 2))));
		zdif = org_z - trace_ent.origin_z;
		ydif = org2_y - trace_ent.origin_y;
		xdif = org2_x - trace_ent.origin_x;
		is_headshot = 0;
		if (((ydif >= CONTENT_SKY) && (ydif <= TE_LIGHTNING2)))
			is_headshot = 1;
		if (((xdif >= CONTENT_SKY) && (xdif <= TE_LIGHTNING2)))
			is_headshot = 1;
		if (self.attack <= 5 && is_headshot == 1 && zdif >= (trace_ent.size_z / 2 * 0.8))
			self.critical = 3;

		dam = (dam * (1 - trace_fraction));
		if (trace_ent.solid != SOLID_BSP)
			SpawnBlood (org, 1);

		if (trace_ent.solid == SOLID_BSP)
			SpawnWood (trace_ent, org, 1);

		dam = dam + random()*dam;
		T_Damage (trace_ent, self, self, dam);
		self.critical = 0;

		if (trace_ent.solid == SOLID_BSP)
			penetrate (org, (dam / 2), (dam / 2));
	}
	else
	{
		bullet_hole (org);
		dir = vectoangles (source - targ);
		penetrate (org, (dam / 2), (dam / 2));
		return;
	}
};

void (float dam, float rec, string snd, float rng, float rate) FireAssaultRifle =
{
      local float weap, hs, tmp, zdif, is_headshot, z;
      local vector dir, source, targ, org, adjust, headshot_check;

	weap = ToIID(self.(SlotField(self.current_slot)));

	sound (self, CHAN_WEAPON, snd, 1, ATTN_NORM);

	self.attack_finished = (time + rate);

	if (self.position == 0)
		adjust = '0 0 0';
	if (self.position == 1)
		adjust = '0 0 -16';
	if (self.position == 2)
		adjust = '0 0 -32';

	DropAmmo ();
	makevectors (self.v_angle);
	if (self.recoil >= 15)
		self.recoil = 15;

	if (self.attack == 0)
	{
		if (self.position == 0)
			player_single1 ();
		if (self.position == 1)
			player_single1_s ();
		if (self.position == 2)
			player_single1 ();
	}
	if (self.attack >= 1)
	{
		if (self.position == 0)
			autofire ();
		if (self.position == 1)
			autofire_s ();
		if (self.position == 2)
			player_single1 ();
	}

	tmp = 50;

	if (self.velocity_y < 0)
		z = z + (self.velocity_y*-1);
	else if (self.velocity_y > 0)
		z = z + (self.velocity_y);

	if (self.velocity_x < 0)
		z = z + (self.velocity_x*-1);
	else if (self.velocity_x > 0)
		z = z + (self.velocity_x);

	tmp = tmp + z;
	tmp = tmp + (40 * self.recoil);

	if (self.attack <= 3 && self.position == 1 && self.velocity_z == 0)
		tmp = (tmp * 0.75);

	if (self.attack <= 3 && self.position == 2 && self.velocity_z == 0)
		tmp = (tmp * 0.5);

	self.attack = self.attack + 1;
	self.recoil = self.recoil + 5;

	source = self.origin + '0 0 22';

	targ = self.origin + '0 0 22' + v_right*crandom()* tmp + v_up*crandom()*tmp;

	traceline (source+adjust, targ+adjust+v_forward*4000, FALSE, self);
	if (trace_fraction == 1)
		return;

	org = trace_endpos - v_forward * 2;
	headshot_check = trace_endpos + v_forward * (trace_ent.size_x / 2);

	if (trace_ent.takedamage)
	{


		is_headshot = 0;
		hs = headshot_check_x - trace_ent.origin_x;
		hs = hs + headshot_check_y - trace_ent.origin_y;

		if (hs < 0)
			hs = hs * -1;

		if (hs <= 4)
			is_headshot = 1;

		zdif = trace_endpos_z - trace_ent.origin_z;

		if (is_headshot == 1 && zdif >= (trace_ent.size_z / 2 * 0.8))
			self.critical = 3;



		dam = (dam * (1 - trace_fraction));
		if (trace_ent.solid != SOLID_BSP && self.critical == 3)
			SpawnBlood (org, 1);

		if (trace_ent.solid == SOLID_BSP)
			SpawnWood (trace_ent, org, 1);

		dam = dam + random()*dam;

		if (weap != IID_WP_DKS1 && weap != IID_WP_FNFAL)
			T_Damage (trace_ent, self, self, dam);
		else if (weap == IID_WP_DKS1 || weap == IID_WP_FNFAL)
			X_Damage (trace_ent, self, self, dam);
		self.critical = 0;

		if (trace_ent.solid == SOLID_BSP)
			penetrate (org, (dam / 2), (dam / 2));
	}
	else
	{
		bullet_hole (org);
		dir = vectoangles (source - targ);
		penetrate (org, (dam / 2), (dam / 2));
		return;
	}
};


void () Screenshake =
{
	local entity te;

	te = findradius (self.origin, 700);
	while (te)
	{
		if (((te.classname == "player") && (te.ghost == 0)))
		{
			stuffcmd (te, "v_iyaw_level 2\n");
			stuffcmd (te, "v_iyaw_cycle 32\n");
			stuffcmd (te, "v_ipitch_level 2\n");
			stuffcmd (te, "v_ipitch_cycle 32\n");
			stuffcmd (te, "v_iroll_level 2\n");
			stuffcmd (te, "v_iroll_cycle 32\n");
			stuffcmd (te, "v_idlescale 1\n");
		}
		te = te.chain;
	}
};

void () ScreenshakeSingle =
{
	stuffcmd (self, "v_iyaw_level 2\n");
	stuffcmd (self, "v_iyaw_cycle 32\n");
	stuffcmd (self, "v_ipitch_level 2\n");
	stuffcmd (self, "v_ipitch_cycle 32\n");
	stuffcmd (self, "v_iroll_level 2\n");
	stuffcmd (self, "v_iroll_cycle 32\n");
	stuffcmd (self, "v_idlescale 1\n");
};

void () ExplosionFrames =
{
	self.avelocity = '300 300 250';
	self.nextthink = (time + 0.02);
	self.frame = (self.frame + 1);
	if (self.frame == 16)
		remove (self);
};

void (float input) Explosion =
{
	local float r;

	self.effects = EF_DIMLIGHT;
	self.touch = SUB_Null;
	setmodel (self, "progs/blast.mdl");
	r = random ();
	if ((r < 0.3))
		sound (self, CHAN_BODY, "misc/exp1.wav", 1, ATTN_NORM);
	if ((r < 0.65))
		sound (self, CHAN_BODY, "misc/exp2.wav", 1, ATTN_NORM);
	else
		sound (self, CHAN_BODY, "misc/exp3.wav", 1, ATTN_NORM);

	WriteByte (MSG_BROADCAST, SVC_TEMPENTITY);
	WriteByte (MSG_BROADCAST, TE_EXPLOSION);
	WriteCoord (MSG_BROADCAST, self.origin_x);
	WriteCoord (MSG_BROADCAST, self.origin_y);
	WriteCoord (MSG_BROADCAST, self.origin_z);
	Screenshake ();
	self.frame = AS_MELEE;
	self.velocity = VEC_ORIGIN;
	self.avelocity = '300 300 250';
	self.think = ExplosionFrames;
	self.nextthink = (time + 0.02);
};


void () WeaponTouch =
{
	local float slotnum;
	if (other.classname != "player")
		return;

	if (other.ghost != 0)
		return;

	if (ItemInSlot(other, other.current_slot) == 0 && FitsInSlot(other.current_slot, ToIID(self.islot1)))
		slotnum = other.current_slot;
	if (!slotnum)
		slotnum = FindSuitableEmptySlot(other, ToIID(self.islot1));
	if (!slotnum)
		return;	//can't get it.

	sound (other, CHAN_BODY, "misc/item1.wav", 1, ATTN_NORM);

	SetItemSlot(other, slotnum, self.islot1);

	sprint(other, PRINT_MEDIUM, "You pick up the ");
	sprint(other, PRINT_MEDIUM, GetItemName(ToIID(self.islot1)));
	sprint(other, PRINT_MEDIUM, "\n");
	remove (self);

	self = other;
	if (self.current_slot == slotnum)
		W_SetCurrentAmmo ();
};

void (float slotnum, float snd, float force) DropFromSlot =
{
	local float weap;
	local string mdel;

	if (self.attack_finished > time && !force)
		return;

	if (slotnum == 0)
		return;

	weap = ItemInSlot(self, slotnum);
	if (weap == 0)
	{
		sprint(self, PRINT_HIGH, "Can't drop nothing\n");
		return;
	}

	if (snd == 1)
		sound (self, CHAN_WEAPON, "weapons/lock4.wav", 1, ATTN_NORM);

	makevectors (self.v_angle);
	newmis = spawn ();
	newmis.owner = self;
	newmis.classname = "dropped_weapon";
	newmis.movetype = MOVETYPE_TOSS;
	newmis.solid = SOLID_TRIGGER;
	newmis.flags = FL_ITEM;	//item makes it bigger to player touches.
	newmis.velocity = aim (self, 500);
	newmis.velocity = (newmis.velocity * 500);
	newmis.angles_y = (random () * 360);

	mdel = GetItemWModel(ToIID(weap));

	setmodel (newmis, mdel);
	setsize (newmis, '-2 -2 0', '2 2 1');
	makevectors (self.v_angle);
	traceline (self.origin, ((self.origin + (v_forward * IT_LIGHTNING)) + '0 0 32'), FALSE, self);
	trace_endpos = (trace_endpos - (v_forward * WEAPON_SPIKES));
	setorigin (newmis, trace_endpos);
	newmis.origin_z = self.origin_z;
	newmis.nextthink = (time + 180);
	newmis.think = SUB_Remove;
	newmis.touch = WeaponTouch;

	newmis.islot1 = weap;

	SetItemSlot(self, slotnum, 0);

	if (self.attack_finished < time+0.6)
		self.attack_finished = time+0.6;

	if (self.current_slot == slotnum)
		SetWeaponModel ();

	if (self.equipment_slot == slotnum)
		self.equipment_slot = 0;
		
};

void (float slot) WeaponAmmo =
{
	local float weap, amount;

	weap = ToIID(ItemInSlot(self, slot));

	amount = WeaponMagQuant(weap);
	GiveAmmo (slot, amount);
};

void() Crosshair =
{
	local float r;
	local string new;

	r = 32 + (self.recoil*8);
	if (r > 256)
		r = 256;
	if (r < 0)
		r = 0;

	new = ftos(r);	
	stuffcmd(self, "crosshairsize ");
	stuffcmd(self, new);
	stuffcmd(self, "\n");
	r = 0.75 - (self.recoil*0.03);
	if (r <= 0.25)
		r = 0.25;

	new = ftos(r);	
	stuffcmd(self, "crosshairalpha ");
	stuffcmd(self, new);
	stuffcmd(self, "\n");
};

float (entity healer, entity saved) RevivePlayer =
{
	local entity oself;
	oself = self;

	self = saved;
/*	if (!walkmove(0, 0))
	{
		//too close?
		return FALSE;
	}
*/
	saved.deadflag = DEAD_NO;
	saved.takedamage = DAMAGE_AIM;
	saved.movetype = MOVETYPE_WALK;
	saved.solid = SOLID_NOT;

	self = oself;

	saved.materialize = 200;
	saved.ghost = 0;
	saved.health = 2;
	saved.air_finished = time + 10;
	saved.view_ofs = '0 0 22';
	self = saved;
	player_run();
	self = oself;
	stuffcmd(saved, "impulse 1\n");
	if (healer.classname == "player")
	{
		sprint (healer, PRINT_HIGH, "you revive ");
		sprint (healer, PRINT_HIGH, trace_ent.netname);
		sprint (healer, PRINT_HIGH, ".\n ");
	}
	sprint (saved, PRINT_HIGH, healer.netname);
	sprint (saved, PRINT_HIGH, " saves you from death.\n");
	saved.view2 = world;

	saved.flags (-) FL_FINDABLE_NONSOLID;

	return TRUE;
};

float(float iid) UseBoostingChem =
{
	local vector source;
	local string x;
	local float duration;

	if (self.attack_finished > time)
		return false;

	x = GetItemName (iid);
	if (iid == IID_CHEM_ADRENALINE)
		duration = 30+random()*30;
	else if (iid == IID_CHEM_PSYCHO)
		duration = 30+random()*30;
	else if (iid == IID_CHEM_BESERK)
		duration = 30+random()*30;
	else
	{
		sprint (self, PRINT_HIGH, "Not a boosting chem\n");
		return false;
	}

	self.attack_finished = time + 1;

	makevectors (self.v_angle);
	source = self.origin + '0 0 0';
	traceline (source, source + v_forward*64, FALSE, self);
	if (trace_ent.classname == "player" && trace_ent.team == self.team)
	{
		if (trace_ent.health <= 0)
			return false;

		if (trace_ent.rage >= 1)
		{
			sprint (self, 2, trace_ent.netname);
			sprint (self, PRINT_HIGH, " is already affected.\n");
			return false;
		}
		sprint (trace_ent, 2, self.netname);
		sprint (trace_ent, PRINT_HIGH, " used a ");
		sprint (trace_ent, PRINT_HIGH, x);
		sprint (trace_ent, PRINT_HIGH, " on you.\n");
		sound (trace_ent, CHAN_BODY, "player/berserk.wav", 1, ATTN_NORM);
		trace_ent.rage = iid;
		trace_ent.ragetime = duration;
		return true;
	}
	if (self.health < self.max_health && self.rage == 0)
	{
		sound (self, CHAN_BODY, "player/berserk.wav", 1, ATTN_NORM);
		self.rage = iid;
		self.ragetime = duration;
		return true;
	}
	return false;
};

float(float iid) UseHealingChem =
{
	local vector source;
	local float heal;
	local string x;
	local float friendly;


	if (self.attack_finished > time)
		return false;

	x = GetItemName (iid);
	if (iid == IID_CHEM_STIMPACK)
		heal = 2*5;
	else if (iid == IID_CHEM_MEDICALBAG)
	{
		heal = 3*5;
		x = "bandage";
	}
	else if (iid == IID_CHEM_SUPERSTIM)
		heal = 4*5;
	else
	{
		sprint (self, PRINT_HIGH, "Not a healing chem\n");
		return false;
	}
	

	self.attack_finished = time + 1;

	makevectors (self.v_angle);
	source = self.origin + '0 0 0';
	traceline (source, source + v_forward*64, 32, self);
	if (trace_ent.classname == "player" && trace_ent.team == self.team)
	{
		if (coop)
			friendly = true;	//all players are friendly in coop
		if (teamplay)
			friendly = trace_ent.team == self.team;
		if (trace_ent.health <= 0 && friendly)
		{
			return RevivePlayer(self, trace_ent);
		}
		if (!friendly)
			return false;

		if (trace_ent.regen >= 1)
		{
			sprint (self, PRINT_HIGH, trace_ent.netname);
			sprint (self, PRINT_HIGH, " is already healing.\n");
			return false;
		}
		if (trace_ent.health >= trace_ent.max_health)
		{
			sprint(self, PRINT_HIGH, trace_ent.netname);
			sprint(self, PRINT_HIGH, " isn't injured.\n");
			return false;
		}
		sprint (trace_ent, PRINT_HIGH, self.netname);
		sprint (trace_ent, PRINT_HIGH, " used a ");
		sprint (trace_ent, PRINT_HIGH, x);
		sprint (trace_ent, PRINT_HIGH, " to heal you.\n");

		trace_ent.regen = heal;
		trace_ent.health = trace_ent.health + heal;
		stuffcmd (trace_ent, "v_cshift 0 0 0 0\n");
		sound (self, CHAN_BODY, "items/r_item2.wav", 1, ATTN_NORM);
		return true;
	}
	if (self.health < self.max_health && self.regen == 0)
	{
		sound (self, CHAN_BODY, "items/r_item2.wav", 1, ATTN_NORM);
		self.health = self.health + heal;
		self.regen = heal;
		return true;
	}
	return false;
};

void () DisplayMenu =
{
	local string menu;

	if (self.currentmenu == "none")
		return;

	if (self.currentmenu == "shop_list")
	{
		menu = ShopString ();
		centerprint (self, menu);
	}
	if (self.currentmenu == "shop_trait")
	{
		menu = TraitString ();
		centerprint (self, menu);
	}
	if (self.currentmenu == "shop_perks")
	{
		menu = PerkString ();
		centerprint (self, menu);
	}
	if (self.currentmenu == "shop_armor")
	{
		menu = ArmorString ();
		centerprint (self, menu);
	}
	if (self.currentmenu == "shop_protect")
	{
		menu = ProtectString ();
		centerprint (self, menu);
	}
	if (self.currentmenu == "shop_melee")
	{
		menu = MeleeString ();
		centerprint (self, menu);
	}
	if (self.currentmenu == "shop_thrown")
	{
		menu = ThrownString ();
		centerprint (self, menu);
	}
	if (self.currentmenu == "shop_pistols")
	{
		menu = PistolString ();
		centerprint (self, menu);
	}
	if (self.currentmenu == "shop_shotguns")
	{
		menu = ShotgunString ();
		centerprint (self, menu);
	}
	if (self.currentmenu == "shop_rifles")
	{
		menu = RifleString ();
		centerprint (self, menu);
	}
	if (self.currentmenu == "shop_heavy")
	{
		menu = HeavyString ();
		centerprint (self, menu);
	}
	if (self.currentmenu == "shop_chems")
	{
		menu = ChemString ();
		centerprint (self, menu);
	}
	if (self.currentmenu == "shop_other")
	{
		menu = OtherString ();
		centerprint (self, menu);
	}
	if (self.currentmenu == "shop_equipment")
	{
		menu = EquipmentString ();
		centerprint (self, menu);
	}
	if (self.currentmenu == "shop_weapons")
	{
		menu = WeaponString ();
		centerprint (self, menu);
	}
	if (((self.currentmenu == "select_skill") && (self.team == 1)))
	{
		centerprint (self, "êCHOOSE SKILL SETë\n\nê1ë Medic    \nê2ë Assassin \nê3ë Soldier  \nê4ë Scientist\n");
	}
	if (((self.currentmenu == "select_skill") && (self.team == 2)))
	{
		centerprint (self, "êCHOOSE SKILL SETë\n\nê1ë Medic    \nê2ë Assassin \nê3ë Soldier  \nê4ë Scientist\n");
	}
	if ((self.currentmenu == "select_team"))
	{
		if (self.class == 0)
			centerprint (self, "êCHOOSE YOUR TEAMë\n\nê1ë Rangers     (good)\nê2ë Raiders     (evil)\nê3ë Auto-Assign       \n");
		if (self.class >= 0 && self.oldteam == 0)
			centerprint (self, "êCHOOSE YOUR TEAMë\n\nê1ë Rangers     (good)\nê2ë Raiders     (evil)\nê3ë Auto-Assign       \n");
		if (self.class > 0 && self.oldteam > 0)
			centerprint (self, "êCHOOSE YOUR TEAMë\n\nê1ë Rangers     (good)\nê2ë Raiders     (evil)\nê3ë Auto-Assign       \nê4ë Keep Previous     \n");
	}
	if (self.currentmenu == "confirm_team")
	{
		sound (self, CHAN_BODY, "player/yourturn.wav", 1, ATTN_NORM);

		if (self.team == 1)
			centerprint (self, "êyou will respawn asë\n\nRanger - OK?\nê1ë Yes     \nê2ë No      \n");
		if (self.team == 2)
			centerprint (self, "êyou will respawn asë\n\nRaider - OK?\nê1ë Yes     \nê2ë No      \n");
	}
	if (self.currentmenu == "confirm_skill")
	{
		sound (self, CHAN_BODY, "player/yourturn.wav", 1, ATTN_NORM);

		if (self.class == 1)
			centerprint (self, "êyour class will beë\n\nMedic - OK?\nê1ë Yes     \nê2ë No      \n");
		if (self.class == 2)
			centerprint (self, "êyour class will beë\n\nAssassin - OK?\nê1ë Yes     \nê2ë No      \n");
		if (self.class == 3)
			centerprint (self, "êyour class will beë\n\nSoldiier - OK?\nê1ë Yes     \nê2ë No      \n");
		if (self.class == 4)
			centerprint (self, "êyour class will beë\n\nScientist - OK?\nê1ë Yes     \nê2ë No      \n");
	}

	if (self.currentmenu == "menu_build")
	{
		menu = BuildString ();
		centerprint (self, menu);
	}
};


void () Special =
{
#if 0
<<<< <<< weapons.qc
/*
==== ===
	if (self.class == 1)
		Bandage ();
>>>> >>> 1.2
	if (self.class == 2)
<<< <<<< weapons.qc
		Cmd_InvUse(GetItemName(IID_CHEM_MEDICALBAG));
	if (self.class == 3)
		Cmd_InvUse("sneak-boy");
	if (self.class == 4)
		Cmd_InvUse(GetItemName(IID_GREN_FRAG));
	if (self.class == 6)
		Cmd_InvUse("shield");
*/

===== ==
		Sneak ();
	if (self.class == 4)
		self.currentmenu = "menu_build";
>>>> >>> 1.2
#endif
};

void () hos_run1;
void () hos_stand1;
float (float iid) spawn_station;

void () ExitScreen =
{
	if (self.class == 0)
		return;

	if (self.ghost == 1)
		return;

	if (self.team == 0)
		return;

	if (self.attack_finished > time)
		return;

	if (trace_ent.classname == "hostage" && trace_ent.health > 0 && trace_fraction < 1)
	{
		if (self.team != 1)
			return;

		if (self.currentmenu == "menu_build")
			return;

		if (trace_ent.cnt == 0)
		{
			sprint (self, 2, "hostage is now following you.\n");
			trace_ent.nextthink = (time + 0.1);
			trace_ent.think = hos_run1;
			trace_ent.cnt = 1;
			trace_ent.friend = self;
			return;
		}
		else
		{
			if (trace_ent.cnt == 1)
			{
				sprint (self, 2, "hostage stopped following you.\n");
				trace_ent.nextthink = (time + 0.1);
				trace_ent.think = hos_stand1;
				trace_ent.cnt = 0;
				trace_ent.friend = trace_ent;
				return;
			}
		}
		return;
	}

	if ((self.currentmenu != "none"))
	{
		centerprint (self, "\n");
		self.currentmenu = "none";
	}
};

void () Sneak =
{
	local float w;

	w = weightx();

	if (self.sneak >= 1)
	{
		sound (self, CHAN_BODY, "items/inv1.wav", 1, ATTN_NORM);
		centerprint (self, "ç Uncloaked ç\n");
		setmodel (self, "progs/guy.mdl");
		self.sneak = 0;
		return;
	}
	if (self.ammo_cells < 20)
	{
		sprint (self, PRINT_HIGH, "wait for stealth-boy to recharge.\n");
		return;
	}
	if (self.sneak == 0 && self.class == 2)
	{
		sound (self, CHAN_BODY, "items/inv1.wav", 1, ATTN_NORM);
		centerprint (self, "ç cloaked and sneaking ç\n(1% detection chance)");
		self.sneak = 1;
		return;
	}
	else if (self.sneak == 0 && self.class != 2)
	{
		sound (self, CHAN_BODY, "items/inv1.wav", 1, ATTN_NORM);
		centerprint (self, "ç cloaked ç\n(15% detection chance)");
		self.sneak = 3;
		return;
	}
	else if (self.sneak == 0 && w > 20)
	{
		sound (self, CHAN_BODY, "items/inv1.wav", 1, ATTN_NORM);
		centerprint (self, "ç too much gear ç\n");
		setmodel (self, "progs/guy.mdl");
		self.sneak = 0;
		return;
	}
};

void () Shield =
{
	if (self.class != 6)
	{
		centerprint (self, "You can't shield yourself!\n");
		return;
	}
	if (self.sneak == 2)
	{
		centerprint (self, "ççç Unshielded ççç\n");
		self.sneak = 0;
		sound (self, CHAN_BODY, "items/protect2.wav", 1, ATTN_NORM);
		return;
	}
	if (self.ammo_cells < 10)
	{
		centerprint (self, "wait for your shield to recharge.\n");
		return;
	}
	if (self.sneak == 0)
	{
		centerprint (self, "ççç Energy Shield ççç\n");
		self.sneak = 2;
		sound (self, CHAN_BODY, "items/protect.wav", 1, ATTN_NORM);
		return;
	}

};

void () station_die =
{
	if ((self.buildtype == IID_BUILD_MRAMMO))
	{
		if ((self.team == 1))
		{
			blue_weapon = 0;
		}
		else
		{
			if ((self.team == 2))
			{
				red_weapon = 0;
			}
		}
	}
	if ((self.buildtype == IID_BUILD_SHIELDGEN))
	{
		if ((self.team == 1))
		{
			blue_armor = 0;
		}
		else
		{
			if ((self.team == 2))
			{
				red_armor = 0;
			}
		}
	}
	if ((self.buildtype == IID_BUILD_AUTODOC))
	{
		if ((self.team == 1))
		{
			blue_gadget = 0;
		}
		else
		{
			if ((self.team == 2))
			{
				red_gadget = 0;
			}
		}
	}
	Explosion (2);
};

void () station_think =
{
#define chemcount attack_finished
	local entity te;
	local float r;

	self.nextthink = time + 2;

	if (self.track.team != self.team)
	{
		station_die ();
		return;
	}

	if (self.chemcount <= 0)
	{
		station_die ();
		return;
	}

	if (self.buildtype == IID_BUILD_SHIELDGEN)//barricade
	{
		sound (self, CHAN_BODY, "items/protect2.wav", 1, ATTN_NORM);

		te = findradius (self.origin, 256);
		while (te)
		{
			if (te.classname == "player" && te.team == self.team && !te.deadflag)
			{
				if (self.chemcount <= 0)
				{
					sound (self, CHAN_BODY, "misc/menu2.wav", TRUE, ATTN_NORM);
					sprint (te, 2, "the shield generator is out of power.\n");
					return;
				}
				if (te.classname == "player")
				{
					te.protect = 3;
					self.chemcount = self.chemcount - 1;
					stuffcmd(te, "v_cshift 0 100 100 100\n");
				}
			}
			te = te.chain;
		}
	}
	if (self.buildtype == IID_BUILD_AUTODOC)//autodoc
	{
		te = findradius (self.origin, 70);
		while (te)
		{
			if (te.classname == "player" && te.team == self.team && !te.deadflag)
			{
				if (self.chemcount <= 0)
				{
					sound (self, CHAN_BODY, "misc/item1.wav", TRUE, ATTN_NORM);
					sprint (te, 2, "the autodoc is out of medical supplies.\n");
					return;
				}
				if (te.deadflag)
					RevivePlayer(self, te);
				else if (te.health < te.max_health)
				{
					sound (self, CHAN_BODY, "items/r_item2.wav", 1, ATTN_NORM);
					sprint (te, PRINT_MEDIUM, "the auto-doc heals you for 3 health.\n");
					te.health = te.health + 3;
					self.chemcount = self.chemcount - 1;
					if (te.health > te.max_health)
						te.health = te.max_health;
				}
				if (random() < 0.001)
				{
					r = random()*6;
					if (r < 1)
						sprint(te, PRINT_HIGH, "the auto-doc heals you of AIDS\n");
					else if (r < 2)
						sprint(te, PRINT_HIGH, "the auto-doc heals you of Genital Herpes\n");
					else if (r < 3)
						sprint(te, PRINT_HIGH, "the auto-doc heals you of Gonorrhea\n");
					else if (r < 4)
						sprint(te, PRINT_HIGH, "the auto-doc heals you of Syphillis\n");
					else if (r < 5)
						sprint(te, PRINT_HIGH, "the auto-doc rids you of Crabs\n");
					else if (r < 6)
						sprint(te, PRINT_HIGH, "the auto-doc heals you of Chlamydia\n");
				}
			}
			te = te.chain;
		}
	}

	if (self.buildtype == IID_BUILD_MRAMMO)//mr. ammo
	{
		te = findradius (self.origin, 60);
		while (te)
		{
			if (te.classname == "player" && te.team == self.team && !te.deadflag)
			{
				if (self.chemcount <= 0)
				{
					sound (self, CHAN_BODY, "misc/item1.wav", TRUE, ATTN_NORM);
					sprint (te, 2, "this mr.ammo is out of ammunition.\n");
					return;
				}

				local float x;

				x = 300;

				local float curweap;
				local float ammotype;
				local float ammocount;
				local float ammoslot;
				curweap = ToIID(ItemInSlot(te, te.current_slot));
				ammotype = WeaponAmmoType(curweap);
				if (ammotype)
				{
					ammocount = TotalQuantity(te, ammotype);
					x = x - ammocount;
				}
				else
					x = 0;
				if (x > 0)
				{
					ammocount = WeaponMagQuant(curweap);
					if (x > ammocount/2)
						x = ammocount/2;

					ammoslot = SlotOfItem(te, ammotype);
					if (!ammoslot)
						ammoslot = FindEmptySlot(te);
					if (ammoslot)
						SetItemSlot(te, ammoslot, SlotVal(ammotype, x + ToStatus(ItemInSlot(te, ammoslot))));

					r = random()*6;
					if (r < 1)
						sprint (te, PRINT_MEDIUM, ftos(ceil(x)), " ammo was received from the mr.ammo.\n");
					else if (r < 2)
						sprint (te, PRINT_MEDIUM, ftos(ceil(x)), " potatos were received from the mr.ammo.\n");
					else if (r < 3)
						sprint (te, PRINT_MEDIUM, ftos(ceil(x)), " ammo was given by the mr.ammo.\n");
					else if (r < 4)
						sprint (te, PRINT_MEDIUM, ftos(ceil(x)), " ammo was stolen from the mr.ammo.\n");
					else if (r < 5)
						sprint (te, PRINT_MEDIUM, ftos(ceil(x)), " ammo was extracted from the mr.ammo.\n");
					else
						sprint (te, PRINT_MEDIUM, ftos(ceil(x)), " ammo was given by the mr.ammo, along with an std\n");

					self.chemcount = self.chemcount - 1;
				}
			}
			te = te.chain;
		}
	}
};

void () Bar_Think =
{
	local float dot1;

	if ((self.owner.health < WEAPON_SHOTGUN))
	{
		remove (self);
		return;
	}
	if ((self.owner.health >= self.owner.max_health))
	{
		remove (self);
		return;
	}
	self.flags = self.flags;

	if ((self.owner.position == WEAPON_SHOTGUN))
	{
		dot1 = WEAPON_SHOTGUN;
	}
	if ((dot1 == WEAPON_SHOTGUN))
	{
		self.frame = MULTICAST_ALL;
		setmodel (self, "");
		self.nextthink = (time + 0.01);
		return;
	}
	if (((self.owner.health >= WEAPON_SHOTGUN) && (dot1 == MULTICAST_ALL)))
	{
		self.frame = floor (((self.owner.health / self.owner.max_health) * TE_WIZSPIKE));
		setorigin (self, (self.owner.origin + '0 0 48'));
		self.nextthink = (time + 0.01);
		setmodel (self, "progs/hbar.spr");
	}
};

void (entity guy) spawn_dot =
{
	local entity hologram;

	hologram = spawn ();
	hologram.movetype = MOVETYPE_NONE;
	hologram.solid = SOLID_NOT;
	hologram.owner = self;
	setmodel (hologram, "progs/hbar.spr");
	hologram.skin = self.skin;
	setorigin (hologram, self.origin);
	setsize (hologram, VEC_ORIGIN, VEC_ORIGIN);
	hologram.angles = self.angles;
	hologram.colormap = self.colormap;
	hologram.cnt = MULTICAST_ALL;
	hologram.think = Bar_Think;
	hologram.nextthink = (time + 0.01);
};

void() FinishTurret;
float (float iid) spawn_station =
{
	local entity oself;
	local entity te;
	local vector org;

	if (iid == IID_BUILD_ROBOFANG)
	{
		te = find (world, classname, "robofang");
		while (te)
		{
			if (te.track == self && te.buildtype == IID_BUILD_ROBOFANG)
			{
				makevectors (self.v_angle);
				setorigin(te, self.origin + v_forward*32);
				return false;
			}
			te = find (te, classname, "robofang");
		}
	}

	te = find (world, classname, "station");
	while (te)
	{
		if (te.track == self && te.buildtype == iid)
		{
			sprint (self, 2, "already have one.\n");
			return false;
		}
		te = find (te, classname, "station");
	}
	if (iid == IID_BUILD_ROBOFANG)
	{
			te = findradius (self.origin, 128);
			while (te)
			{
				if (te != self && te.classname == "player" && te.health > 0)
				{
					sprint(self, PRINT_HIGH, "not with other players nearby.\n");
					return false;
				}
				te = te.chain;
			}
	}

	makevectors (self.v_angle);
	org = ((self.origin + (v_forward * IT_LIGHTNING)) + (v_up * EF_FLAG2));
	if ((pointcontents ((org + '0 20 0')) != CONTENT_EMPTY))
	{
		sprint (self, 2, "can't build there.\n");
		return false;
	}
	if ((pointcontents ((org + '0 -20 0')) != CONTENT_EMPTY))
	{
		sprint (self, 2, "can't build there.\n");
		return false;
	}
	if ((pointcontents ((org + '20 0 0')) != CONTENT_EMPTY))
	{
		sprint (self, 2, "can't build there.\n");
		return false;
	}
	if ((pointcontents ((org + '-20 0 0')) != CONTENT_EMPTY))
	{
		sprint (self, 2, "can't build there.\n");
		return false;
	}
	if ((pointcontents ((org + '0 0 50')) != CONTENT_EMPTY))
	{
		sprint (self, 2, "can't build there.\n");
		return false;
	}
	if ((pointcontents ((org + '0 0 -10')) != CONTENT_EMPTY))
	{
		sprint (self, 2, "can't build there.\n");
		return false;
	}
	self.impulse = 0;
	te = findradius (org, 40);
	while (te)
	{
		if (te.classname == "spawn1")
		{
			sprint (self, 2, "can't build at spawn.\n");
			return false;
		}
		if (te.classname == "spawn2")
		{
			sprint (self, 2, "can't build at spawn.\n");
			return false;
		}
		if (te.classname == "ghoul")
		{
			sprint (self, 2, "somethings in the way.\n");
			return false;
		}
		if (((te.classname == "player") && (te.health > 0)))
		{
			sprint (self, 2, "can't build on players.\n");
			return false;
		}
		if (((te.classname == "station") && (te.health > 0)))
		{
			sprint (self, 2, "can't build on other stations.\n");
			return false;
		}
		te = te.chain;
	}

	oself = self;
	self = spawn ();
	self.team = oself.team;
	self.track = oself;

	spawn_dot (self);
	self.origin = org;
	self.takedamage = DAMAGE_YES;
	self.solid = SOLID_SLIDEBOX;
	self.movetype = MOVETYPE_TOSS;
	setsize (self, '-16 -16 0', '16 16 40');
	self.health = 30;
	self.max_health = 300;
	self.th_die = station_die;
	setmodel (self, "progs/station.mdl");
	self.classname = "station";
	self.think = station_think;
	self.helmet = 2;
	self.buildtype = iid;
	self.aflag = true;

	self.frame = 4;

	self.chemcount = 75;

	if (self.buildtype == IID_BUILD_MRAMMO)
		self.netname = "mr. ammo";
	if (self.buildtype == IID_BUILD_SHIELDGEN)
		self.netname = "shield generator";
	if (self.buildtype == IID_BUILD_AUTODOC)
		self.netname = "autodoc";
	if (self.buildtype == IID_BUILD_ROBOFANG)
	{
		setsize(self, '-24 -24 -24', '24 24 24');
		self.netname = "robofang";
		setmodel (self, "progs/dog.mdl");
	}
	if (self.buildtype == IID_BUILD_TTURRET || self.buildtype == IID_BUILD_RTURRET || self.buildtype == IID_BUILD_GTURRET)
	{
		FinishTurret();
	}

	self = oself;

	return true;
};

float() FireToolkit =
{
	local float rr;

	makevectors (self.v_angle);
	traceline (self.origin, self.origin + v_forward*64, FALSE, self);

	//track is used for the owning player (without using the .owner field)
	if (trace_ent.track == self && trace_ent.classname == "station" && trace_ent.health > 0)
	{
		if (trace_ent.health >= trace_ent.max_health)	//already built - don't whack it!
			return true;

		//play a random build sound
		rr = random ();
		if (rr < 0.3)
			sound (self, CHAN_WEAPON, "misc/build1.wav", TRUE, ATTN_NORM);
		else if (rr < 0.6)
			sound (self, CHAN_WEAPON, "misc/build2.wav", TRUE, ATTN_NORM);
		else
			sound (self, CHAN_WEAPON, "misc/build3.wav", TRUE, ATTN_NORM);


		if (!trace_ent.nextthink)
			if (trace_ent.health > 200)
				trace_ent.frame = 5;
		self.attack_finished = time + 0.1 + (random () * 0.1);
		trace_ent.health = trace_ent.health + 6;
		

		if (trace_ent.health >= trace_ent.max_health)
		{	//it's built!
			trace_ent.health = trace_ent.max_health;
			if (!trace_ent.aflag)
				return true;	//already complete
			trace_ent.aflag = false;

			trace_ent.nextthink = time + 1;
			sprint (self, PRINT_MEDIUM, "BUILDING COMPLETED.\n");


			if (trace_ent.buildtype == IID_BUILD_ROBOFANG)
			{
				trace_ent.solid = SOLID_NOT;
				trace_ent.think = SUB_Remove;
				trace_ent.nextthink = time;
				makevectors (self.v_angle);
sprint(self, PRINT_HIGH, "Robofang appears to have malfunctioned\n");
//				spawn_dog (self.origin + v_forward * 32);
			}
			if (trace_ent.buildtype == IID_BUILD_AUTODOC)
				trace_ent.frame = 1;
			if (trace_ent.buildtype == IID_BUILD_MRAMMO)
				trace_ent.frame = 0;
			if (trace_ent.buildtype == IID_BUILD_SHIELDGEN)
				trace_ent.frame = 2;
		}
		return true;
	}
	return false;
};

void () BuyMenu =
{
	local float is_shop;
	local entity te;
	local string menu;

	if (self.currentmenu == "777")
		return;

	is_shop = 0;

	if (coop == 0)
	{
		te = findradius (self.origin, 250);
		while (te)
		{
			if (te.classname == "buyzone1" && self.team == 1)
				is_shop = 1;
			if (te.classname == "buyzone2" && self.team == 2)
				is_shop = 1;

			te = te.chain;
		}
	}

	if (coop == 1)
	{

		te = findradius (self.origin, 80);
		while (te)
		{
			if (te.classname == "buyzone")
				is_shop = 1;
			if (te.classname == "merchant")
				is_shop = 1;

			te = te.chain;
		}
	}

	if ((is_shop == 0))
	{
		centerprint (self, "nothing but the afterglow, here.\n(find a trader)");
		return;
	}
	if ((is_shop == 1))
	{
		menu = ShopString ();
		centerprint (self, menu);
		self.currentmenu = "shop_list";

		if (self.current_slot != 0 && self.current_slot != 2)
			self.current_slot = 1;

		return;
	}
};



void () CharacterSheet =
{
	local string x;
	local float qq;
	local float ratio;
	local float r1;
	local float r2;

	stuffcmd (self, "toggleconsole\n");
	sprint (self, PRINT_HIGH, "\n\n\n\n ** INFO ** \n");
	sprint (self, PRINT_HIGH, "êClass     ë ");
		if (self.class == 1)
			sprint (self, 2, "Medic");
		if (self.class == 2)
			sprint (self, 2, "Assassin");
		if (self.class == 3)
			sprint (self, 2, "Soldier");
		if (self.class == 4)
			sprint (self, 2, "Scientist");

	sprint (self, PRINT_HIGH, "\nêTeam      ë ");

	if (self.team == 1)
		sprint (self, 2, "Rangers\n");
	if (self.team == 2)
		sprint (self, 2, "Raiders\n");

	sprint (self, PRINT_HIGH, "êScore     ë ");
	r1 = (self.dead);
	r2 = (self.kills);
	if (r1 == 0)
		r1 = 1;

	ratio = (r2 / r1);
	x = ftos (ratio);
	sprint (self, 2, x);
	sprint (self, 2, " (");
	x = ftos (self.kills);
	sprint (self, 2, x);
	sprint (self, PRINT_HIGH, "/");
	x = ftos (self.dead);
	sprint (self, 2, x);
	sprint (self, 2, ")   ");
	sprint (self, PRINT_HIGH, "\nêSpeed     ë ");
	x = ftos (self.maxspeed);
	sprint (self, 2, x);
	sprint (self, PRINT_HIGH, "/300\n");
	sprint (self, PRINT_HIGH, "êMoney     ë ");

	x = ftos (self.ammo_shells);
	sprint (self, 2, x);
	sprint (self, PRINT_HIGH, "$\n");
	if (self.class == 2)
	{
		sprint (self, PRINT_HIGH, "êBandages  ë ");
		x = ftos (self.bandages);
		sprint (self, 2, x);
		sprint (self, PRINT_HIGH, "\n");
	}
	if (self.class == 6)
	{
		sprint (self, PRINT_HIGH, "êScraps    ë ");
		x = ftos (self.scraps);
		sprint (self, 2, x);
		sprint (self, PRINT_HIGH, "\n");
	}
/*
	sprint (self, PRINT_HIGH, "\nêArmor     ë ");
	x = GetArmorName();
	sprint (self, 2, x);
	sprint (self, 2, "  (");
	x = ftos (self.armor_weight);
	sprint (self, 2, x);
	sprint (self, 2, ")\n");
*/
	sprint (self, PRINT_HIGH, "êProtectiveë ");
	x = GetProtectName();
	sprint (self, 2, x);
	sprint (self, 2, "\n");
/*
	sprint (self, PRINT_HIGH, "êChem      ë ");
	x = GetChemName();
	sprint (self, 2, x);
	x = ftos (self.chemcount);
	sprint (self, 2, "[");
	sprint (self, 2, x);
	sprint (self, 2, "]");
	sprint (self, 2, "\n");
*/
	sprint (self, PRINT_HIGH, "êGadget    ë ");

	x = GetEquipmentName();
	sprint (self, 2, x);
	sprint (self, 2, "\n");
	sprint (self, PRINT_HIGH, "êPerk      ë ");
	x = GetPerkName();
	sprint (self, 2, x);
	sprint (self, 2, "\n");
	sprint (self, PRINT_HIGH, "êTrait     ë ");
	x = GetTraitName();
	sprint (self, 2, x);
	sprint (self, 2, "\n");
/*
	sprint (self, PRINT_HIGH, "êWeapon 1  ë ");
	x = GetWeaponName (self, self.slot1);
	sprint (self, 2, x);
	sprint (self, 2, "  (");
	x = ftos (self.slot1_weight);
	sprint (self, 2, x);
	sprint (self, 2, ") (");
	sprint (self, 2, self.ammotype1);
	sprint (self, 2, ")\n");

	sprint (self, PRINT_HIGH, "êWeapon 2  ë ");
	x = GetWeaponName (self, self.slot2);
	sprint (self, 2, x);
	sprint (self, 2, "  (");
	x = ftos (self.slot2_weight);
	sprint (self, 2, x);
	sprint (self, 2, ") (");
	sprint (self, 2, self.ammotype2);
	sprint (self, 2, ")\n\n");
*/
	qq = weightx ();
	sprint (self, PRINT_HIGH, "êWeight    ë ");
	x = ftos (qq);
	sprint (self, 2, x);
	qq = self.max_weight;
	sprint (self, PRINT_HIGH, "/");
	x = ftos (qq);
	sprint (self, 2, x);
	sprint (self, PRINT_HIGH, "\n");
	return;
};

void () UseEquipment =
{

	if (self.equipment == 0)
	{
		centerprint(self, "êë no extra equipment êë\n");
		return;
	}
	if (self.equipment == 1)
	{
		centerprint(self, "êë medic's bag êë\nlets you carry more stimpacks\n");
		return;
	}
	if (self.equipment == 4)
	{
		centerprint(self, "êë belt pouch êë\ngives you room for two extra grenades\n");
		return;
	}
	if (self.equipment == 5)
	{
		centerprint(self, "êë backpack êë\nlets you carry more ammunition\n");
		return;
	}
	if (self.equipment == 6)
	{
		centerprint(self, "êë toolkit mark ii êë\nbuild, defuse and open doors faster\n");
		return;
	}
/*
	if (self.equipment == 7 && self.equipment_slot == 0)
	{
		sprint (self, PRINT_HIGH, "climbing gear in place.\n");
		sound (self, CHAN_BODY, "misc/item2.wav", 1, ATTN_NORM);
		self.maxspeed = 100;
		self.velocity_z = 0;
		self.equipment_state = 1;
	}
	if (self.equipment == 7 && self.equipment_state == 1)
	{
		sprint (self, PRINT_HIGH, "climbing gear retrieved.\n");
		sound (self, CHAN_BODY, "misc/item2.wav", 1, ATTN_NORM);
		self.grab = 0;
		self.equipment_state = 0;
		return;
	}
*/
	if (self.equipment == 8)
	{
		centerprint(self, "êë enhanced battery êë\nallows longer cloaking\n");
		return;
	}
	if (self.equipment == 9)
	{
		Sneak();
		return;
	}

};

void (vector s_aim, float dam, float tmp, float ran) W_FireBuckshotSpread1 =
{
	local vector source;
	local vector targ;
	local vector org;
	local float zdif;
	local float ydif;
	local float xdif;
	local float is_headshot;

	makevectors (self.v_angle);

	if (self.position == 0)
		source = (self.origin + '0 0 20');
	if (self.position == 1)
		source = (self.origin + '0 0 4');
	if (self.position == 2)
		source = (self.origin + '0 0 -12');

	targ = ((((((s_aim + ((v_right * random ()) * tmp)) - ((v_right * random ()) * tmp)) + ((v_up * random ()) * tmp)) - ((v_up * random ()) * tmp)) + (((v_up * random ()) * tmp) * 0.5)) + (v_forward * ran));

	traceline (source, targ, FALSE, self);
	if ((trace_fraction == 1))
	{
		return;
	}
	org = (trace_endpos - (v_forward * 2));
	if (trace_ent.takedamage)
	{
		dam = ((random () * dam) + dam);
		dam = (dam * (1 - trace_fraction));

		if (trace_ent.solid != SOLID_BSP)
			SpawnBlood (org, 1);
		if (trace_ent.solid == SOLID_BSP)
			SpawnWood (trace_ent, org, 1);

		zdif = (org_z - trace_ent.origin_z);
		ydif = (org_y - trace_ent.origin_y);
		xdif = (org_x - trace_ent.origin_x);
		is_headshot = 0;
		if (((ydif >= CONTENT_SLIME) && (ydif <= WEAPON_SPIKES)))
		{
			is_headshot = 1;
		}
		if (((xdif >= CONTENT_SLIME) && (xdif <= WEAPON_SPIKES)))
		{
			is_headshot = 1;
		}
		if (((is_headshot == 1) && (zdif >= ((trace_ent.size_z / 2) * 0.8))))
		{
			if (self.attack > 2)
				dam = (dam * 0.4);

			self.critical = 3;
		}

		dam = dam + random()*dam;
		T_Damage (trace_ent, self, self, dam);
		self.critical = 0;
	}
	else
	{
		bullet_hole (org);
	}
};

void (float rec, float number, float dam, float spread, float ran, float auto) W_FireShotgun =
{
	local vector dir;
	local vector p_aim;
	local float var1;
	local float var2;
	local float var3;
	local float var4;

	if (self.velocity != '0 0 0')
	{
		var1 = ((random () * 48) * (6 + self.recoil * 1.5));
		var2 = ((random () * 48) * (6 + self.recoil * 1.5));
		var3 = ((random () * 48) * (6 + self.recoil * 1.5));
		var4 = ((random () * 48) * (6 + self.recoil * 1.5));
	}
	if (self.position == 2)
	{
		var1 = ((random () * 12) * (6 + self.recoil * 1.5));
		var2 = ((random () * 12) * (6 + self.recoil * 1.5));
		var3 = ((random () * 12) * (6 + self.recoil * 1.5));
		var4 = ((random () * 12) * (6 + self.recoil * 1.5));
	}
	if (self.position == 1)
	{
		var1 = ((random () * IDLE3A) * (6 + self.recoil * 1.5));
		var2 = ((random () * IDLE3A) * (6 + self.recoil * 1.5));
		var3 = ((random () * IDLE3A) * (6 + self.recoil * 1.5));
		var4 = ((random () * IDLE3A) * (6 + self.recoil * 1.5));
	}
	if (self.position == 0)
	{
		var1 = ((random () * IDLE10A) * (6 + (self.recoil * 3)));
		var2 = ((random () * IDLE10A) * (6 + (self.recoil * 3)));
		var3 = ((random () * IDLE10A) * (6 + (self.recoil * 3)));
		var4 = ((random () * IDLE10A) * (6 + (self.recoil * 3)));
	}

	if (self.position == 0)
		player_single1 ();
	if (self.position == 2)
		player_single1 ();
	if (self.position == 1)
		player_single1_s ();

	stuffcmd (self, "+lookup\n");
	stuffcmd (self, "wait\n");
	stuffcmd (self, "-lookup\n");

	sound (self, CHAN_WEAPON, "weapons/shotgun1.wav", 1.5, ATTN_NORM);

	
	p_aim = (((((self.origin + '0 0 20') + (v_right * var1)) - (v_right * var2)) + (v_up * var3)) - (v_up * var4));

	msg_entity = self;
	WriteByte (MSG_ONE, SVC_BIGKICK);
	DropAmmo ();
	self.attack = (self.attack + 1);
	self.recoil = (self.recoil + 8);
	Crosshair();

	if (self.recoil >= 30)
		self.recoil = 30;

	if (auto == 0)
		self.attack_finished = (time + 0.5);

	if (auto == 1)
		self.attack_finished = (time + 0.2);

	dir = aim (self, 10000);
	spread = 30;
	if ((number == MULTICAST_PVS_R))
	{
		W_FireBuckshotSpread1 (p_aim, dam, spread, ran);
		W_FireBuckshotSpread1 ((p_aim + (v_right * 80)), dam, spread, ran);
		W_FireBuckshotSpread1 ((p_aim - (v_right * 80)), dam, spread, ran);
		W_FireBuckshotSpread1 ((p_aim + (v_up * 80)), dam, spread, ran);
		W_FireBuckshotSpread1 ((p_aim - (v_up * 80)), dam, spread, ran);
	}
	else
	{
		if ((number == AS_MELEE))
		{
			W_FireBuckshotSpread1 (p_aim, dam, spread, ran);
			W_FireBuckshotSpread1 (p_aim, dam, spread, ran);
			W_FireBuckshotSpread1 (p_aim, dam, spread, ran);
		}
	}
};

void () PlasmaBolt =
{
	local float dam;
	local float zdif;
	local float ydif;
	local float xdif;
	local float tru;

	if (other.solid == SOLID_TRIGGER)
		return;	//they're not really solid

	if (pointcontents (self.origin) == CONTENT_SKY)
	{
		remove (self);
		return;
	}

	if (other.takedamage)
	{
		dam = 30 + (random () * 30);

		T_Damage (other, self, self.owner, dam);
		WriteByte (MSG_MULTICAST, SVC_TEMPENTITY);
		WriteByte (MSG_MULTICAST, TE_LIGHTNINGBLOOD);
		WriteCoord (MSG_MULTICAST, self.origin_x);
		WriteCoord (MSG_MULTICAST, self.origin_y);
		WriteCoord (MSG_MULTICAST, self.origin_z);
		multicast (self.origin, MULTICAST_PHS);
		sound (self, CHAN_WEAPON, "enforcer/enfstop.wav", 1, ATTN_NORM);
	}
	else
	{
		WriteByte (MSG_MULTICAST, SVC_TEMPENTITY);
		WriteByte (MSG_MULTICAST, TE_LIGHTNINGBLOOD);
		WriteCoord (MSG_MULTICAST, self.origin_x);
		WriteCoord (MSG_MULTICAST, self.origin_y);
		WriteCoord (MSG_MULTICAST, self.origin_z);
		multicast (self.origin, MULTICAST_PHS);
		sound (self, CHAN_WEAPON, "enforcer/enfstop.wav", 1, ATTN_NORM);
	}
	remove (self);
};

//the alien blaster
void () FireAlienBlaster =
{
	local float tmp;

	DropAmmo ();
	self.recoil = self.recoil + 8;

	msg_entity = self;
	WriteByte (MSG_ONE, SVC_SMALLKICK);

	newmis = spawn ();
	newmis.owner = self;
	newmis.movetype = MOVETYPE_FLYMISSILE;
	newmis.solid = SOLID_BBOX;
	newmis.effects = EF_DIMLIGHT;
	makevectors (self.v_angle);
	newmis.velocity = aim (self, 3000);
	newmis.velocity = (newmis.velocity * 3000);
	newmis.angles = vectoangles (newmis.velocity);

	newmis.nextthink = (time + IDLE3A);
	newmis.think = SUB_Remove;
	setmodel (newmis, "progs/plasma.mdl");
	tmp = ((30 + self.velocity_y) + self.velocity_x);


	{
		newmis.velocity = aim (self, 1700);
		newmis.velocity = newmis.velocity * 1700;
		newmis.angles = vectoangles (newmis.velocity);
		newmis.nextthink = time + 1.4;
		sound (self, CHAN_WEAPON, "weapons/blaster.wav", WEAPON_SHOTGUN, ATTN_NORM);
		newmis.touch = PlasmaBolt;
		setmodel (newmis, "progs/ray.mdl");
		tmp = 90 + self.velocity_y + self.velocity_x;
	}

	setsize (newmis, '0 0 0', '0 0 0');
	setorigin (newmis, self.origin + (v_right * WEAPON_BIG) + (v_forward * WEAPON_ROCKET) + '0 0 20');
	newmis.velocity = newmis.velocity + (v_right * random () * tmp) - (v_right * random () * tmp) + (v_up * random () * tmp) - (v_up * random () * tmp);

	self.attack_finished = time + 0.5;
};
