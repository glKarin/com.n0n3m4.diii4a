#define mag1 currentammo
#define maxmag1 cnt


void () army_load1;

//PISTOL
void (float tmp, float dam) army_fire =
{
	local vector src;
	local vector dir;
	local vector direction;
	local entity en;
	local vector org;

	if (self.enemy.sneak == 1)
		tmp = tmp * 2;

	if (self.mag1 == 0)
	{
		sound (self, CHAN_WEAPON, "misc/greload.wav", PLAT_LOW_TRIGGER, ATTN_NORM);
		army_load1();
		self.mag1 = self.maxmag1;
		return;
	}

	self.mag1 = self.mag1 - 1;


	makevectors (self.angles);

	sound (self, CHAN_WEAPON, "weapons/1911.wav", PLAT_LOW_TRIGGER, ATTN_NORM);

	src = self.origin + v_forward*10;
	src_z = self.absmin_z + self.size_z * 0.7;

	en = self.enemy;
	
	dir = en.origin - en.velocity*0.2;
	dir = normalize (dir - self.origin);

	direction = dir;

	traceline (src, src + direction*2048 + v_right*crandom()*tmp + v_up*crandom()*tmp, FALSE, self);

	if (trace_fraction == PLAT_LOW_TRIGGER)
		return;

	if (trace_ent.takedamage)
	{
		dam = 1 + random()*dam + random()*dam;
		dam = dam * (1 - (trace_fraction/2));
		SpawnBlood (org, PLAT_LOW_TRIGGER);
		T_Damage (trace_ent, self, self, dam);
	}
	else
	{
		WriteByte (MSG_MULTICAST, SVC_TEMPENTITY);
		WriteByte (MSG_MULTICAST, TE_SPIKE);
		WriteCoord (MSG_MULTICAST, trace_endpos_x);
		WriteCoord (MSG_MULTICAST, trace_endpos_y);
		WriteCoord (MSG_MULTICAST, trace_endpos_z);
		multicast (trace_endpos, MULTICAST_PVS);
	}
};


//RIFLE
void (float tmp, float dam) army_fire1 =
{
	local vector src;
	local vector dir;
	local vector direction;
	local entity en;
	local vector org;

	if (self.enemy.sneak == 1)
		tmp = tmp * 2;

	if (self.mag1 == 0)
	{
		sound (self, CHAN_WEAPON, "misc/greload.wav", PLAT_LOW_TRIGGER, ATTN_NORM);
		army_load1();
		self.mag1 = self.maxmag1;
		return;
	}

	self.mag1 = self.mag1 - 1;

	makevectors (self.angles);

	sound (self, CHAN_WEAPON, "weapons/rangem.wav", PLAT_LOW_TRIGGER, ATTN_NORM);

	src = self.origin + v_forward*10;
	src_z = self.absmin_z + self.size_z * 0.7;

	en = self.enemy;
	
	dir = en.origin - en.velocity*0.2;
	dir = normalize (dir - self.origin);

	direction = dir;

	traceline (src, src + direction*2048 + v_right*crandom()*100 + v_up*crandom()*100, FALSE, self);

	if (trace_fraction == PLAT_LOW_TRIGGER)
		return;

	if (trace_ent.takedamage)
	{
		SpawnBlood (org, PLAT_LOW_TRIGGER);
		dam = 10 + random()*dam + random()*dam;
		dam = dam * (1 - (trace_fraction/2));
		T_Damage (trace_ent, self, self, dam);
	}
	else
	{
		WriteByte (MSG_MULTICAST, SVC_TEMPENTITY);
		WriteByte (MSG_MULTICAST, TE_SPIKE);
		WriteCoord (MSG_MULTICAST, trace_endpos_x);
		WriteCoord (MSG_MULTICAST, trace_endpos_y);
		WriteCoord (MSG_MULTICAST, trace_endpos_z);
		multicast (trace_endpos, MULTICAST_PVS);
	}
};


//SHOTGUN
void (float tmp, float dam) army_fire2 =
{
	local vector src;
	local vector dir;
	local vector direction;
	local entity en;
	local vector org;
	local float var_u, var_r, var_o;
	local float shot;

	if (self.enemy.sneak == 1)
		tmp = tmp * 2;

	if (self.mag1 == 0)
	{
		sound (self, CHAN_WEAPON, "misc/greload.wav", PLAT_LOW_TRIGGER, ATTN_NORM);
		army_load1();
		self.mag1 = self.maxmag1;
		return;
	}

	self.mag1 = self.mag1 - 1;

	makevectors (self.angles);

	sound (self, CHAN_WEAPON, "weapons/shotgun2.wav", PLAT_LOW_TRIGGER, ATTN_NORM);

	src = self.origin + v_forward*10;
	src_z = self.absmin_z + self.size_z * 0.7;

	en = self.enemy;
	
	dir = en.origin - en.velocity*0.2;
	dir = normalize (dir - self.origin);

	direction = dir;

	shot = 5;
	var_o = crandom()*50;

	while (shot > 0)
	{
			if (shot == 5)
			{
				var_r = 30;
				var_u = 30;
			}
			if (shot == 4)
			{
				var_r = -30;
				var_u = 30;
			}
			if (shot == 3)
			{
				var_r = 30;
				var_u = -30;
			}
			if (shot == 2)
			{
				var_r = -30;
				var_u = -30;
			}
			if (shot == 1)
			{
				var_r = 0;
				var_u = 0;
			}

			traceline (src, src + direction*2048 + v_right*(var_o+var_r) + v_up*(var_o+var_u), FALSE, self);

			shot = (shot - 1);

			if (trace_fraction == PLAT_LOW_TRIGGER)
				return;

			if (trace_ent.takedamage)
			{
				SpawnBlood (org, PLAT_LOW_TRIGGER);
				dam = 1 + random()*dam + random()*dam;
				dam = dam * (1 - (trace_fraction/2));
				T_Damage (trace_ent, self, self, dam);
			}
			else
			{
				WriteByte (MSG_MULTICAST, SVC_TEMPENTITY);
				WriteByte (MSG_MULTICAST, TE_SPIKE);
				WriteCoord (MSG_MULTICAST, trace_endpos_x);
				WriteCoord (MSG_MULTICAST, trace_endpos_y);
				WriteCoord (MSG_MULTICAST, trace_endpos_z);
				multicast (trace_endpos, MULTICAST_PVS);
			}
	}
};

//SMG
void (float tmp, float dam) army_fire3 =
{
	local vector src;
	local vector dir;
	local vector direction;
	local entity en;
	local vector org;

	if (self.mag1 == 0)
	{
		sound (self, CHAN_WEAPON, "misc/greload.wav", PLAT_LOW_TRIGGER, ATTN_NORM);
		army_load1();
		self.mag1 = self.maxmag1;
		return;
	}

	if (self.enemy.sneak == 1)
		tmp = tmp * 2;

	self.mag1 = self.mag1 - 1;

	makevectors (self.angles);

	src = self.origin + v_forward*10;
	src_z = self.absmin_z + self.size_z * 0.7;

	en = self.enemy;
	
	dir = en.origin - en.velocity*0.2;
	dir = normalize (dir - self.origin);

	direction = dir;

	traceline (src, src + direction*1024 + v_right*crandom()*tmp + v_up*crandom()*tmp, FALSE, self);

	if (trace_fraction == PLAT_LOW_TRIGGER)
		return;

	if (trace_ent.takedamage)
	{
		SpawnBlood (org, PLAT_LOW_TRIGGER);
		dam = 1 + random()*dam + random()*dam;
		dam = dam * (1 - (trace_fraction/2));
		T_Damage (trace_ent, self, self, dam);
	}
	else
	{
		WriteByte (MSG_MULTICAST, SVC_TEMPENTITY);
		WriteByte (MSG_MULTICAST, TE_SPIKE);
		WriteCoord (MSG_MULTICAST, trace_endpos_x);
		WriteCoord (MSG_MULTICAST, trace_endpos_y);
		WriteCoord (MSG_MULTICAST, trace_endpos_z);
		multicast (trace_endpos, MULTICAST_PVS);
	}
};

//Assault Rifle
void (float tmp, float dam) army_fire4 =
{
	local vector src;
	local vector dir;
	local vector direction;
	local entity en;
	local vector org;

	if (self.mag1 == 0)
	{
		sound (self, CHAN_WEAPON, "misc/greload.wav", PLAT_LOW_TRIGGER, ATTN_NORM);
		army_load1();
		self.mag1 = self.maxmag1;
		return;
	}

	if (self.enemy.sneak == 1)
		tmp = tmp * 2;

	self.mag1 = self.mag1 - 1;

	makevectors (self.angles);

	src = self.origin + v_forward*10;
	src_z = self.absmin_z + self.size_z * 0.7;

	en = self.enemy;
	
	dir = en.origin - en.velocity*0.2;
	dir = normalize (dir - self.origin);

	direction = dir;

	traceline (src, src + direction*2048 + v_right*crandom()*tmp + v_up*crandom()*tmp, FALSE, self);

	if (trace_fraction == PLAT_LOW_TRIGGER)
		return;

	if (trace_ent.takedamage)
	{
		SpawnBlood (org, PLAT_LOW_TRIGGER);
		dam = 1 + random()*dam + random()*dam;
		dam = dam * (1 - (trace_fraction/2));
		T_Damage (trace_ent, self, self, dam);
	}
	else
	{
		WriteByte (MSG_MULTICAST, SVC_TEMPENTITY);
		WriteByte (MSG_MULTICAST, TE_SPIKE);
		WriteCoord (MSG_MULTICAST, trace_endpos_x);
		WriteCoord (MSG_MULTICAST, trace_endpos_y);
		WriteCoord (MSG_MULTICAST, trace_endpos_z);
		multicast (trace_endpos, MULTICAST_PVS);
	}
};


void () army_load1 = [ 29, army_load2 ]
{
	ai_stand ();
};
void () army_load2 = [ 29, army_load3 ]
{
	ai_stand ();
};
void () army_load3 = [ 30, army_load4 ]
{
	ai_stand ();
};
void () army_load4 = [ 30, army_load5 ]
{
	ai_stand ();
};
void () army_load5 = [ 31, army_load6 ]
{
	ai_stand ();
};
void () army_load6 = [ 31, army_load7 ]
{
	ai_stand ();
};
void () army_load7 = [ 32, army_load8 ]
{
	ai_stand ();
};
void () army_load8 = [ 32, army_load9 ]
{
	ai_stand ();
};
void () army_load9 = [ 33, army_load10 ]
{
	ai_stand ();
};
void () army_load10 = [ 33, army_load11 ]
{
	ai_stand ();
};
void () army_load11 = [ 34, army_load12 ]
{
	ai_stand ();
};

void () army_load12 = [ 34, army_load13 ]
{
	ai_stand ();
};
void () army_load13 = [ 35, army_load14 ]
{
	ai_stand ();
};
void () army_load14 = [ 35, army_load15 ]
{
	ai_stand ();
};
void () army_load15 = [ 36, army_load16 ]
{
	ai_stand ();
};
void () army_load16 = [ 36, army_load17 ]
{
	ai_stand ();
};
void () army_load17 = [ 37, army_load18 ]
{
	ai_stand ();
};
void () army_load18 = [ 37, army_load19 ]
{
	ai_stand ();
};
void () army_load19 = [ 38, army_load20 ]
{
	ai_stand ();
};
void () army_load20 = [ 38, army_load21 ]
{
	ai_stand ();
};
void () army_load21 = [ 39, army_load22 ]
{
	ai_stand ();
};
void () army_load22 = [ 39, army_run1 ]
{
	ai_stand ();
};

void () army_stand1 = [ 0, army_stand2 ]
{
	ai_stand ();
};

void () army_stand2 = [ 1, army_stand3 ]
{
	ai_stand ();
};

void () army_stand3 = [ 2, army_stand4 ]
{
	ai_stand ();
};

void () army_stand4 = [ 3, army_stand5 ]
{
	ai_stand ();
};

void () army_stand5 = [ 4, army_stand6 ]
{
	ai_stand ();
};

void () army_stand6 = [ 5, army_stand7 ]
{
	ai_stand ();
};

void () army_stand7 = [ 6, army_stand8 ]
{
	ai_stand ();
};

void () army_stand8 = [ 7, army_stand1 ]
{
	ai_stand ();
};

void () army_walk1 = [ 73, army_walk2 ]
{
	ai_walk (TE_TELEPORT);
};

void () army_walk2 = [ 74, army_walk3 ]
{
	ai_walk (15);
};

void () army_walk3 = [ 75, army_walk4 ]
{
	ai_walk (TE_LAVASPLASH);
};

void () army_walk4 = [ 76, army_walk5 ]
{
	ai_walk (TE_LAVASPLASH);
};

void () army_walk5 = [ 77, army_walk6 ]
{
	ai_walk (SECRET_NO_SHOOT);
};

void () army_walk6 = [ 78, army_walk7 ]
{
	ai_walk (15);
};

void () army_walk7 = [ 79, army_walk8 ]
{
	ai_walk (TE_LAVASPLASH);
};

void () army_walk8 = [ 80, army_walk1 ]
{
	ai_walk (SECRET_NO_SHOOT);
};

void () army_run1 = [ 73, army_run2 ]
{

	ai_run (TE_TELEPORT);
};

void () army_run2 = [ 74, army_run3 ]
{
	ai_run (15);
};

void () army_run3 = [ 75, army_run4 ]
{
	if (self.active == 1)
		self.action_points = self.action_points - 1;

	ai_run (TE_LAVASPLASH);
};

void () army_run4 = [ 76, army_run5 ]
{
	ai_run (TE_LAVASPLASH);
};

void () army_run5 = [ 77, army_run6 ]
{
	ai_run (SECRET_NO_SHOOT);
};

void () army_run6 = [ 78, army_run7 ]
{
	ai_run (15);
};

void () army_run7 = [ 79, army_run8 ]
{
	ai_run (TE_LAVASPLASH);
};

void () army_run8 = [ 80, army_run1 ]
{
	ai_run (SECRET_NO_SHOOT);
};

void () army_atk1 = [ 81, army_atk2 ]
{
	ai_face ();
};

void () army_atk2 = [ 82, army_atk3 ]
{
	ai_face ();
};

void () army_atk3 = [ 83, army_atk4 ]
{
	ai_face ();
};

void () army_atk4 = [ 84, army_atk5 ]
{
	ai_face ();
};

void () army_atk5 = [ 85, army_atk6 ]
{
	ai_face ();
};

void () army_atk6 = [ 86, army_atk7 ]
{
	ai_face ();
};

void () army_atk7 = [ 87, army_atk8 ]
{
	ai_face ();
	army_fire (120, 14);
};

void () army_atk8 = [ 88, army_atk9 ]
{
	ai_face ();
};

void () army_atk9 = [ 89, army_run1 ]
{
	ai_face ();
};

void () army_atka1 = [ 81, army_atka2 ]
{
	ai_face ();
};

void () army_atka2 = [ 82, army_atka3 ]
{
	ai_face ();
};

void () army_atka3 = [ 83, army_atka4 ]
{
	ai_face ();
};

void () army_atka4 = [ 84, army_atka5 ]
{
	ai_face ();
};

void () army_atka5 = [ 85, army_atka6 ]
{
	ai_face ();
	army_fire2 (200, 4);
};

void () army_atka6 = [ 86, army_atka7 ]
{
	ai_face ();
};

void () army_atka7 = [ 87, army_atka8 ]
{
	ai_face ();
};

void () army_atka8 = [ 88, army_atka9 ]
{
	ai_face ();
};

void () army_atka9 = [ 89, army_run1 ]
{
	ai_face ();
};

void () army_atkb1 = [ 81, army_atkb2 ]
{
	ai_face ();
};

void () army_atkb2 = [ 82, army_atkb4 ]
{
	ai_face ();
};

void () army_atkb3 = [ 83, army_atkb4 ]
{
	ai_face ();
};

void () army_atkb4 = [ 84, army_atkb5 ]
{
	ai_face ();
};

void () army_atkb5 = [ 85, army_atkb6 ]
{
	local float r;

	ai_face ();
	r = range (self.enemy);

	self.recoil = 0;

	if (r == RANGE_NEAR || r == RANGE_MELEE)
		self.recoil = 1;

	if (self.recoil == 1)
		sound (self, CHAN_WEAPON, "weapons/burst.wav", PLAT_LOW_TRIGGER, ATTN_NORM);
	else
		sound (self, CHAN_WEAPON, "weapons/mp7.wav", PLAT_LOW_TRIGGER, ATTN_NORM);

	army_fire3 (150, 13);
};

void () army_atkb6 = [ 86, army_atkb7 ]
{
	ai_face ();
	if (self.recoil == 1)
	{
		army_fire3 (170, 11);
		army_fire3 (190, 9);
	}
};

void () army_atkb7 = [ 87, army_atkb8 ]
{
	ai_face ();
	if (self.recoil == 1)
		army_fire3 (210, 11);
};

void () army_atkb8 = [ 88, army_atkb9 ]
{
	ai_face ();
	if (self.recoil == 1)
	{
		army_fire3 (230, 11);
		army_fire3 (250, 9);
	}
};

void () army_atkb9 = [ 89, army_run1 ]
{
	ai_face ();
	if (self.recoil == 1)
	{
		army_fire3 (270, 11);
		army_fire3 (290, 9);
	}
};

void () army_atkcs1 = [ 81, army_atkcs2 ]
{
	ai_face ();
};

void () army_atkcs2 = [ 82, army_atkcs3 ]
{
	ai_face ();
};

void () army_atkcs3 = [ 83, army_atkcs4 ]
{
	ai_face ();
};

void () army_atkcs4 = [ 84, army_atkcs5 ]
{
	ai_face ();
};

void () army_atkcs5 = [ 85, army_atkcs6 ]
{
	ai_face ();
	sound (self, CHAN_WEAPON, "weapons/ak112.wav", PLAT_LOW_TRIGGER, ATTN_NORM);
	army_fire4 (100, 16);
};

void () army_atkcs6 = [ 85, army_atkcs7 ]
{

	ai_face ();
};

void () army_atkcs7 = [ 87, army_atkcs8 ]
{

	ai_face ();
};

void () army_atkcs8 = [ 88, army_atkcs9 ]
{

	ai_face ();
};

void () army_atkcs9 = [ 89, army_run1 ]
{
	ai_face ();
};

void () army_atkc1 = [ 81, army_atkc2 ]
{
	ai_face ();
};

void () army_atkc2 = [ 82, army_atkc3 ]
{
	ai_face ();
};

void () army_atkc3 = [ 83, army_atkc4 ]
{
	ai_face ();
};

void () army_atkc4 = [ 84, army_atkc5 ]
{
	ai_face ();
};

void () army_assault_rifle =
{
	local float r;

	r = range (self.enemy);

	if (r == RANGE_FAR || r == RANGE_MID)//single shot at range
		army_atkcs1();
	if (r == RANGE_NEAR || r == RANGE_MELEE)//open up when close
		army_atkc1();
};

void () army_atkc5 = [ 85, army_atkc6 ]
{
	ai_face ();

	sound (self, CHAN_WEAPON, "weapons/auto.wav", PLAT_LOW_TRIGGER, ATTN_NORM);
	army_fire4 (100, 16);
};

void () army_atkc6 = [ 84, army_atkc7 ]
{
	ai_face ();
	army_fire4 (140, 16);
};

void () army_atkc7 = [ 85, army_atkc8 ]
{
	ai_face ();
	army_fire4 (180, 16);
};

void () army_atkc8 = [ 84, army_atkc9 ]
{
	ai_face ();
	army_fire4 (220, 16);
};

void () army_atkc9 = [ 85, army_atkc10 ]
{
	ai_face ();
	army_fire4 (260, 16);
};

void () army_atkc10 = [ 84, army_atkc11 ]
{
	ai_face ();
	army_fire4 (300, 16);
};

void () army_atkc11 = [ 85, army_atkc12 ]
{
	ai_face ();
	army_fire4 (340, 16);
};

void () army_atkc12 = [ 84, army_atkc13 ]
{
	ai_face ();
	army_fire4 (380, 16);
};

void () army_atkc13 = [ 86, army_atkc14 ]
{
	ai_face ();
};

void () army_atkc14 = [ 87, army_atkc15 ]
{
	ai_face ();
};

void () army_atkc15 = [ 88, army_atkc16 ]
{
	ai_face ();
};

void () army_atkc16 = [ 89, army_run1 ]
{
	ai_face ();
};

void () army_atkp1 = [ 81, army_atkp2 ]
{
	ai_face ();
};

void () army_atkp2 = [ 82, army_atkp3 ]
{
	ai_face ();
};

void () army_atkp3 = [ 83, army_atkp4 ]
{
	ai_face ();
};

void () army_atkp4 = [ 84, army_atkp5 ]
{
	ai_face ();
};

void () army_atkp5 = [ 85, army_atkp6 ]
{
	ai_face ();
	army_fire1 (200, 15);
};

void () army_atkp6 = [ 86, army_atkp7 ]
{
	ai_face ();
};

void () army_atkp7 = [ 87, army_atkp8 ]
{
	ai_face ();
};

void () army_atkp8 = [ 88, army_atkp9 ]
{
	ai_face ();
};

void () army_atkp9 = [ 89, army_run1 ]
{
	ai_face ();
};

void () army_pain1 = [ 40, army_pain2 ]
{
};

void () army_pain2 = [ 41, army_pain3 ]
{
};

void () army_pain3 = [ 42, army_pain4 ]
{
};

void () army_pain4 = [ 43, army_pain5 ]
{
};

void () army_pain5 = [ 44, army_pain6 ]
{
};

void () army_pain6 = [ 45, army_run1 ]
{
	ai_pain (PLAT_LOW_TRIGGER);
};

void () army_painb1 = [ 46, army_painb2 ]
{
};

void () army_painb2 = [ 47, army_painb3 ]
{
	ai_painforward (TE_LIGHTNINGBLOOD);
};

void () army_painb3 = [ 48, army_painb4 ]
{
	ai_painforward (TE_LIGHTNING3);
};

void () army_painb4 = [ 49, army_painb5 ]
{
};

void () army_painb5 = [ 50, army_painb6 ]
{
};

void () army_painb6 = [ 51, army_painb7 ]
{
};

void () army_painb7 = [ 52, army_painb8 ]
{
};

void () army_painb8 = [ 53, army_painb9 ]
{
};

void () army_painb9 = [ 54, army_painb10 ]
{
};

void () army_painb10 = [ 55, army_painb11 ]
{
};

void () army_painb11 = [ 56, army_painb12 ]
{
};

void () army_painb12 = [ 57, army_painb13 ]
{
	ai_pain (SILENT);
};

void () army_painb13 = [ 58, army_painb14 ]
{
};

void () army_painb14 = [ 59, army_run1 ]
{
};

void () army_painc1 = [ 60, army_painc2 ]
{
};

void () army_painc2 = [ 61, army_painc3 ]
{
	ai_pain (PLAT_LOW_TRIGGER);
};

void () army_painc3 = [ 62, army_painc4 ]
{
};

void () army_painc4 = [ 63, army_painc5 ]
{
};

void () army_painc5 = [ 64, army_painc6 ]
{
	ai_painforward (PLAT_LOW_TRIGGER);
};

void () army_painc6 = [ 65, army_painc7 ]
{
	ai_painforward (PLAT_LOW_TRIGGER);
};

void () army_painc7 = [ 66, army_painc8 ]
{
};

void () army_painc8 = [ 67, army_painc9 ]
{
	ai_pain (PLAT_LOW_TRIGGER);
};

void () army_painc9 = [ 68, army_painc10 ]
{
	ai_painforward (SECRET_1ST_DOWN);
};

void () army_painc10 = [ 69, army_painc11 ]
{
	ai_painforward (AS_MELEE);
};

void () army_painc11 = [ 70, army_painc12 ]
{
	ai_painforward (TE_LIGHTNING2);
};

void () army_painc12 = [ 71, army_painc13 ]
{
	ai_painforward (SECRET_NO_SHOOT);
};

void () army_painc13 = [ 72, army_run1 ]
{
};

void (entity attacker, float damage) army_pain =
{
	local float r;

	if ((self.pain_finished > time))
	{
		return;
	}
	r = random ();
	if ((r < 0.05))
	{
		self.pain_finished = (time + 0.6);
		army_pain1 ();
		sound (self, CHAN_VOICE, "player/paina.wav", PLAT_LOW_TRIGGER, ATTN_NORM);
	}
	else
	{
		if ((r < 0.1))
		{
			self.pain_finished = (time + 1.1);
			army_painb1 ();
			sound (self, CHAN_VOICE, "player/painb.wav", PLAT_LOW_TRIGGER, ATTN_NORM);
		}
	}
};

void () army_die1 = [ 8, army_die2 ]
{

	if (random()*4 <= 2)
		sound (self, CHAN_VOICE, "player/agdie2.wav", 1, ATTN_NORM);
	else
		sound (self, CHAN_VOICE, "player/agdie3.wav", 1, ATTN_NORM);

};

void () army_die2 = [ 9, army_die3 ]
{
	self.solid = SOLID_NOT;
};

void () army_die3 = [ 10, army_die4 ]
{
};

void () army_die4 = [ 11, army_die5 ]
{
};

void () army_die5 = [ 12, army_die6 ]
{
};

void () army_die6 = [ 13, army_die7 ]
{
};

void () army_die7 = [ 14, army_die8 ]
{
};

void () army_die8 = [ 15, army_die9 ]
{
};

void () army_die9 = [ 16, army_die10 ]
{
};

void () army_die10 = [ 17, army_die10 ]
{
};

void () army_cdie1 = [ 18, army_cdie2 ]
{
};

void () army_cdie2 = [ 19, army_cdie3 ]
{
	self.solid = SOLID_NOT;
	ai_back (MULTICAST_PVS_R);
};

void () army_cdie3 = [ 20, army_cdie4 ]
{
	ai_back (SECRET_1ST_DOWN);
};

void () army_cdie4 = [ 21, army_cdie5 ]
{
	ai_back (TE_LIGHTNINGBLOOD);
};

void () army_cdie5 = [ 22, army_cdie6 ]
{
	ai_back (AS_MELEE);
};

void () army_cdie6 = [ 23, army_cdie7 ]
{
	ai_back (SECRET_1ST_DOWN);
};

void () army_cdie7 = [ 24, army_cdie8 ]
{
};

void () army_cdie8 = [ 25, army_cdie9 ]
{
};

void () army_cdie9 = [ 26, army_cdie10 ]
{
};

void () army_cdie10 = [ 27, army_cdie11 ]
{
};

void () army_cdie11 = [ 28, army_cdie11 ]
{
};


void () grunt_pain =
{
	if (self.frame == 41)
	{
		self.frame = 42;
		self.health = 180;
		self.think = army_die1;
		self.nextthink = time + 0.12;
	}
	else
	{
		ThrowGib ("progs/zom_gib.mdl", -40);
		self.frame = 41;
		self.health = 180;
		self.think = army_cdie1;
		self.nextthink = time + 0.12;
	}

	self.attack = self.attack + 1;

	if (self.attack == 8)
	{
		if (random()*4 <= 2)
			sound (self, CHAN_VOICE, "player/agdie1.wav", 1, ATTN_NORM);
		else
			sound (self, CHAN_VOICE, "player/agdie5.wav", 1, ATTN_NORM);
	}
	if (self.attack >= 16)
	{
		self.solid = SOLID_NOT;

		army_cdie1();
		ThrowGib ("progs/zom_gib.mdl", -35);
		ThrowGib ("progs/zom_gib.mdl", -35);
		ThrowGib ("progs/gib1.mdl", -35);
	}
};

void (vector stuff, vector ang) spawn_live_grunt =
{
	local entity grunt;

	grunt = spawn ();
	grunt.origin = stuff;
	grunt.enemy = world;
	grunt.attack_finished = time + 10;
	grunt.solid = SOLID_SLIDEBOX;
	grunt.movetype = MOVETYPE_STEP;
	grunt.takedamage = DAMAGE_YES;
	setmodel (grunt, "progs/soldier.mdl");
	setsize (grunt, VEC_HULL_MIN, '16 16 40');
	grunt.classname = "body";
	grunt.netname = "dead";
	grunt.health = 180;
	grunt.angles = ang;
	grunt.max_health = grunt.health;
	grunt.th_pain = grunt_pain;
	grunt.think = grunt_pain;
	grunt.nextthink = time + 0.05;
};

void () army_die =
{
	DropBackpack ();

	if (self.health <= -35)
	{
		self.solid = SOLID_NOT;
		sound (self, CHAN_VOICE, "player/udeath.wav", 1, ATTN_NORM);
		army_die1 ();
	}
	else
	{
		self.solid = SOLID_NOT;
		spawn_live_grunt(self.origin, self.angles);
		remove(self);
	}
};

void () monster_army =
{
	local float x;

	x = total_players;

	if (random()*10 <= x*0.5)
	{	
		precache_model2 ("progs/enforcer.mdl");
		precache_model2 ("progs/h_mega.mdl");
		precache_model2 ("progs/laser.mdl");

		precache_sound2 ("enforcer/death1.wav");
		precache_sound2 ("enforcer/enfire.wav");
		precache_sound2 ("enforcer/enfstop.wav");
		precache_sound2 ("enforcer/idle1.wav");
		precache_sound2 ("enforcer/pain1.wav");
		precache_sound2 ("enforcer/pain2.wav");
		precache_sound2 ("enforcer/sight1.wav");
		precache_sound2 ("enforcer/sight2.wav");
		precache_sound2 ("enforcer/sight3.wav");
		precache_sound2 ("enforcer/sight4.wav");
	
		self.solid = SOLID_SLIDEBOX;
		self.movetype = MOVETYPE_STEP;
		self.team = 3;

		setmodel (self, "progs/enforcer.mdl");
		self.classname = "monster";
		self.netname = "enforcer";

		setsize (self, '-16 -16 -24', '16 16 32');
		self.health = 120;
            self.islot3 = SlotVal(IID_ARM_COMBAT, 1);
		self.armortype = 35;
		self.th_stand = enf_stand1;
		self.th_walk = enf_walk1;
		self.th_run = enf_run1;
		self.th_pain = enf_pain;
		self.th_die = enf_die;
		self.th_missile = enf_atk1;
		self.armornoise = "misc/thud.wav";

		walkmonster_start();
		if (random()*4 <= 2)
			self.weapon = 5;
		else
			self.weapon = 6;
		self.th_missile = enf_atk1;
		self.mag1 = 24;
		self.maxmag1 = self.mag1;
		return;
	}

	precache_model ("progs/soldier.mdl");
	precache_model ("progs/h_guard.mdl");
	precache_model ("progs/gib1.mdl");
	precache_model ("progs/gib2.mdl");
	precache_model ("progs/gib3.mdl");
	precache_sound ("soldier/death1.wav");
	precache_sound ("soldier/idle.wav");
	precache_sound ("soldier/pain1.wav");
	precache_sound ("soldier/pain2.wav");
	precache_sound ("soldier/sattck1.wav");
	precache_sound ("soldier/sight1.wav");
	precache_sound ("player/udeath.wav");

	self.solid = SOLID_SLIDEBOX;
	self.classname = "monster";
	self.netname = "raider";
	self.movetype = MOVETYPE_STEP;
	setmodel (self, "progs/soldier.mdl");
	setsize (self, '-16 -16 -24', '16 16 28');
	self.health = 90;
	self.team = 3;
            self.islot3 = SlotVal(IID_ARM_LEATHER, 1);
	self.armortype = 0.2;
	self.armornoise = "misc/thud.wav";
	self.th_stand = army_stand1;
	self.th_walk = army_walk1;
	self.th_run = army_run1;
	self.th_missile = army_atk1;
	self.th_pain = army_pain;
	self.th_die = army_die;
	walkmonster_start ();
	
	self.weapon = ceil(random()*4);

	if (self.weapon == 1)
		self.th_missile = army_atkp1; //pistol
	if (self.weapon == 2)
		self.th_missile = army_atk1; //rifle
	if (self.weapon == 3)
		self.th_missile = army_atka1; //shotgun
	if (self.weapon == 4)
		self.th_missile = army_atkb1; //smg

	if (self.weapon == 1)
		self.mag1 = 12;
	if (self.weapon == 2)
		self.mag1 = 10;
	if (self.weapon == 3)
		self.mag1 = 6;
	if (self.weapon == 4)
		self.mag1 = 30;

	self.maxmag1 = self.mag1;
};

#undef mag1
#undef maxmag1
