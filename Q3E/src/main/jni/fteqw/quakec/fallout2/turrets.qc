void() tesla_think =
{
	local entity e;
	local vector org;

	self.nextthink = time + 0.1;
	self.frame = self.frame + 1;

	if (!self.aflag)
	{
		if (!self.enemy || self.enemy.health <= 0)
		{
			e = findradius(self.origin, 800);
			while (e != world)
			{
				if (e.health > 0)
				{
					if (e.team != self.team)
					{
						if (e.solid)
						{
							if (e.takedamage)
							{
								traceline (self.origin, e.origin, FALSE, self);
								if (trace_ent == e)
								{
									break;
								}
							}
						}
					}
				}
				e = e.chain;
			}
			self.enemy = e;
			if (e != world)
				sound (self, CHAN_AUTO, "weapons/lstart.wav", 1, ATTN_NORM);
		}
		if (self.enemy)
		{
			if (self.frame < 7)
				self.frame = 7;

			org = self.origin + '0 0 32';

			traceline (org, self.enemy.origin, FALSE, self);

			if (self.t_width < time)
			{
				sound (self, CHAN_WEAPON, "weapons/lhit.wav", 1, ATTN_NORM);
				self.t_width = time + 0.6;
			}

			//mwa ha ha!!
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

			if (trace_ent != self.enemy)
				self.enemy = world;
			trace_endpos = normalize(trace_endpos - org)*8 + trace_endpos;
			LightningDamage (self.origin, trace_endpos, self, 10);

			if (self.frame > 12)
				self.frame = 0;
			return;
		}
	}
	if (self.frame >= 7)
		self.frame = 0;
};

void (float tmp, float dam) turret_mg_fire =
{
	local vector src;
	local vector dir;
	local vector direction;
	local entity en;
	local vector org;

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

	muzzleflash();
};

void() GunTop_Think =
{
	local entity e;
	local vector org;
	local float delta;

	self.nextthink = time + 0.1;

	if (self.owner.health < 1)
	{
		remove(self);
		return;
	}

	setorigin(self, self.owner.origin);

	if (!self.owner.aflag)
	{
		if (self.attack_finished < time)
		if (!self.enemy || self.enemy.health < 1)
		{
			e = findradius(self.origin, 800);
			while (e != world)
			{
				if (e.health > 0)
				{
					if (e.team != self.owner.team)
					{
						if (e.solid)
						{
							if (e.takedamage)
							{
								traceline (self.origin+'0 0 16', e.origin, FALSE, self);
								if (trace_ent == e)
								{
									break;
								}
							}
						}
					}
				}
				e = e.chain;
			}
			self.enemy = e;
			if (e == world)
				self.attack_finished = time + 0.3;

		}
		if (self.enemy)
		{
			self.ideal_yaw = vectoyaw(self.enemy.origin - self.origin);
			ChangeYaw();
			if (self.attack_finished < time)
			{
				delta = anglemod(self.angles_y - self.ideal_yaw);
				if (delta > 10 && delta < 350)
					return;

				traceline (self.owner.origin+'0 0 16', self.enemy.origin, FALSE, self.owner);
				if (trace_ent != self.enemy)
				{
					self.enemy = world;
					return;
				}

				if (self.owner.buildtype == IID_BUILD_RTURRET)
				{
					sound (self, CHAN_WEAPON, "weapons/sgun1.wav", 1, ATTN_NORM);
					self.attack_finished = time + 0.7;
					newmis = spawn ();
					newmis.owner = self.owner;
					newmis.movetype = MOVETYPE_FLYMISSILE;
					newmis.solid = SOLID_BBOX;

// set newmis speed    
					newmis.velocity = normalize(self.enemy.origin - self.origin)*1000;
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
				}
				if (self.owner.buildtype == IID_BUILD_GTURRET)
				{
					turret_mg_fire(0.4, 3+random()*4);
					self.attack_finished = time + 0.05;
					self.nextthink = time + 0.05;
				}
			}
		}
	}
};

void() FinishTesla =
{
	self.netname = "tesla turret";
	self.think = tesla_think;
	setmodel (self, "progs/coil.mdl");
	setsize (self, '-16 -16 -24', '16 16 32');
	self.nextthink = time + 0.1;
};
void() FinishGunPost =
{
	local entity top;

	top = spawn();
	top.owner = self;
	top.think = GunTop_Think;
	top.nextthink = time + 0.1;
	top.yaw_speed = 20;
	if (self.buildtype == IID_BUILD_RTURRET)
	{
		setmodel(top, "progs/turrgun.mdl");
		setmodel(self, "progs/turrbase.mdl");
	}
	else
	{
		setmodel(top, "progs/miniturr.mdl");
		setmodel(self, "progs/minibase.mdl");
	}
	setorigin(top, self.origin);
	top.angles = self.angles;
};
void() FinishTurret =
{
	if (self.buildtype == IID_BUILD_TTURRET)
		FinishTesla();
	else
		FinishGunPost();
};