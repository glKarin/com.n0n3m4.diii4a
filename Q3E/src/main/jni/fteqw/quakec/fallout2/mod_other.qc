


/*
########################################################################
This QC file houses all of the ugly code that no one wants to deal with.
########################################################################
*/

/*drain_energy    -  Makes energy weapons overheat*/
/*GetWeaponName   -  Tells the player what weapons he is carrying*/
/*GetWeaponWeight -  Calculates the total weight for weapons*/


void (vector org, entity guy, float input, float cost) spawn_station =
{
	local entity dog;
	local entity te;

	if ((self.ammo_shells < cost))
	{
		sprint (self, 2, "not enough money.\n");
		return;
	}
	self = guy;
	te = find (world, classname, "station");
	while (te)
	{
                if (((te.track == self) && (te.skin == input)))
		{
			sprint (self, 2, "already have one.\n");
			return;
		}
		te = find (te, classname, "station");
	}
	makevectors (self.v_angle);
	org = ((org + (v_forward * IT_LIGHTNING)) + (v_up * EF_FLAG2));
	if ((pointcontents ((org + '0 20 0')) != CONTENT_EMPTY))
	{
		sprint (self, 2, "can't build there.\n");
		return;
	}
	if ((pointcontents ((org + '0 -20 0')) != CONTENT_EMPTY))
	{
		sprint (self, 2, "can't build there.\n");
		return;
	}
	if ((pointcontents ((org + '20 0 0')) != CONTENT_EMPTY))
	{
		sprint (self, 2, "can't build there.\n");
		return;
	}
	if ((pointcontents ((org + '-20 0 0')) != CONTENT_EMPTY))
	{
		sprint (self, 2, "can't build there.\n");
		return;
	}
	if ((pointcontents ((org + '0 0 50')) != CONTENT_EMPTY))
	{
		sprint (self, 2, "can't build there.\n");
		return;
	}
	if ((pointcontents ((org + '0 0 -10')) != CONTENT_EMPTY))
	{
		sprint (self, 2, "can't build there.\n");
		return;
	}
	self.impulse = 0;
	te = findradius (org, 40);
	while (te)
	{
		if (te.classname == "spawn1")
		{
			sprint (self, 2, "can't build at spawn.\n");
			return;
		}
		if (te.classname == "spawn2")
		{
			sprint (self, 2, "can't build at spawn.\n");
			return;
		}
		if (te.classname == "ghoul")
		{
			sprint (self, 2, "somethings in the way.\n");
			return;
		}
		if (((te.classname == "player") && (te.health > 0)))
		{
			sprint (self, 2, "can't build on players.\n");
			return;
		}
		if (((te.classname == "station") && (te.health > 0)))
		{
			sprint (self, 2, "can't build on other stations.\n");
			return;
		}
		te = te.chain;
	}
	self.ammo_shells = (self.ammo_shells - cost);
	dog = spawn ();
	dog.team = self.team;
	dog.track = self;
	self = dog;
	//spawn_dot (dog);
	self.origin = org;
	self.takedamage = DAMAGE_YES;
	self.solid = SOLID_SLIDEBOX;
	self.movetype = MOVETYPE_STEP;
	setsize (self, '-16 -16 0', '16 16 40');
	self.health = SVC_INTERMISSION;
	self.max_health = (300 + (input * 50));
	self.th_die = station_die;
	setmodel (self, "progs/station.mdl");
	self.classname = "station";
	self.think = station_think;
	self.helmet = 2;
	self.skin = input;
	if ((self.skin == 0))
	{
		self.netname = "depot";
	}
	if ((self.skin == 1))
	{
		self.netname = "armslab";
	}
	if ((self.skin == 2))
	{
		self.netname = "armory";
	}
	if ((self.skin == 3))
	{
		self.netname = "protolab";
	}
	self.frame = WEAPON_SPIKES;
};

void (float dat) drain_energy =
{
	self.ammo_cells = (self.ammo_cells - dat);
	if ((self.ammo_cells <= 0))
	{
		self.recharge = 1;
	}
	if ((dat != WEAPON_SPIKES))
	{
		if ((self.recharge == 1))
		{
			centerprint (self, "\n\n\n\n\n\n\n\n\n\n\n\n\nH   TEMP    C                 \nÄÅ!COOLING!ÅÇ                 \n");
		}
		else
		{
			if ((self.ammo_cells >= 200))
			{
				return;
			}
			else
			{
				if ((self.ammo_cells >= (200 * 0.9)))
				{
					centerprint (self, "\n\n\n\n\n\n\n\n\n\n\n\n\nH   TEMP    C                 \nÄÅÅÅÅÅÅÅÅÅÉÅÇ                 \n");
				}
				else
				{
					if ((self.ammo_cells >= (200 * 0.8)))
					{
						centerprint (self, "\n\n\n\n\n\n\n\n\n\n\n\n\nH   TEMP    C                 \nÄÅÅÅÅÅÅÅÅÉÅÅÇ                 \n");
					}
					else
					{
						if ((self.ammo_cells >= (200 * 0.7)))
						{
							centerprint (self, "\n\n\n\n\n\n\n\n\n\n\n\n\nH   TEMP    C                 \nÄÅÅÅÅÅÅÅÉÅÅÅÇ                 \n");
						}
						else
						{
							if ((self.ammo_cells >= (200 * 0.6)))
							{
								centerprint (self, "\n\n\n\n\n\n\n\n\n\n\n\n\nH   TEMP    C                 \nÄÅÅÅÅÅÅÉÅÅÅÅÇ                 \n");
							}
							else
							{
								if ((self.ammo_cells >= (200 * 0.5)))
								{
									centerprint (self, "\n\n\n\n\n\n\n\n\n\n\n\n\nH   TEMP    C                 \nÄÅÅÅÅÅÉÅÅÅÅÅÇ                 \n");
								}
								else
								{
									if ((self.ammo_cells >= (200 * 0.4)))
									{
										centerprint (self, "\n\n\n\n\n\n\n\n\n\n\n\n\nH   TEMP    C                 \nÄÅÅÅÅÉÅÅÅÅÅÅÇ                 \n");
									}
									else
									{
										if ((self.ammo_cells >= (200 * 0.3)))
										{
											centerprint (self, "\n\n\n\n\n\n\n\n\n\n\n\n\nH   TEMP    C                 \nÄÅÅÅÉÅÅÅÅÅÅÅÇ                 \n");
										}
										else
										{
											if ((self.ammo_cells >= (200 * 0.2)))
											{
												centerprint (self, "\n\n\n\n\n\n\n\n\n\n\n\n\nH   TEMP    C                 \nÄÅÅÉÅÅÅÅÅÅÅÅÇ                 \n");
											}
											else
											{
												if ((self.ammo_cells <= (200 * 0.1)))
												{
													centerprint (self, "\n\n\n\n\n\n\n\n\n\n\n\n\nH   TEMP    C                 \nÄÅÉÅÅÅÅÅÅÅÅÅÇ                 \n");
												}
												else
												{
													centerprint (self, "\n\n\n\n\n\n\n\n\n\n\n\n\nH   TEMP    C                 \nÄÉÅÅÅÅÅÅÅÅÅÅÇ                 \n");
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
};
			

void (entity guy, float slot) GetWeaponWeight =
{

	return;
};