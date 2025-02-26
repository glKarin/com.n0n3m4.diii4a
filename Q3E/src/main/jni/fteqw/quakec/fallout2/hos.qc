void (float xx) hos_forward;

void () hos_stand1 = [ 11, hos_stand2 ]
{
};

void () hos_stand2 = [ 12, hos_stand3 ]
{
};

void () hos_stand3 = [ 13, hos_stand4 ]
{
};

void () hos_stand4 = [ 14, hos_stand5 ]
{
};

void () hos_stand5 = [ 15, hos_stand1 ]
{
};

void () hos_run1 = [ 0, hos_run2 ]
{
	hos_forward (MULTICAST_PVS_R);
};

void () hos_run2 = [ 1, hos_run3 ]
{
	hos_forward (AS_MELEE);
};

void () hos_run3 = [ 2, hos_run4 ]
{
	hos_forward (MULTICAST_PVS_R);
};

void () hos_run4 = [ 3, hos_run5 ]
{
	hos_forward (AS_MELEE);
};

void () hos_run5 = [ 4, hos_run6 ]
{
	hos_forward (MULTICAST_PVS_R);
};

void () hos_run6 = [ 5, hos_run1 ]
{
	hos_forward (AS_MELEE);
};

void () hos_walk1 = [ 16, hos_walk2 ]
{
	hos_forward (MULTICAST_PVS_R);
};

void () hos_walk2 = [ 17, hos_walk3 ]
{
	hos_forward (AS_MELEE);
};

void () hos_walk3 = [ 18, hos_walk4 ]
{
	hos_forward (MULTICAST_PVS_R);
};

void () hos_walk4 = [ 19, hos_walk5 ]
{
	hos_forward (AS_MELEE);
};

void () hos_walk5 = [ 20, hos_walk6 ]
{
	hos_forward (MULTICAST_PVS_R);
};

void () hos_walk6 = [ 21, hos_walk7 ]
{
	hos_forward (AS_MELEE);
};

void () hos_walk7 = [ 22, hos_walk8 ]
{
	hos_forward (MULTICAST_PVS_R);
};

void () hos_walk8 = [ 23, hos_walk9 ]
{
	hos_forward (AS_MELEE);
};

void () hos_walk9 = [ 24, hos_walk10 ]
{
	hos_forward (MULTICAST_PVS_R);
};

void () hos_walk10 = [ 25, hos_walk1 ]
{
	hos_forward (AS_MELEE);
};

void () hos_deatha1 = [ 6, hos_deatha2 ]
{
};

void () hos_deatha2 = [ 7, hos_deatha3 ]
{
};

void () hos_deatha3 = [ 8, hos_deatha4 ]
{
};

void () hos_deatha4 = [ 9, hos_deatha5 ]
{
};

void () hos_deatha5 = [ 10, hos_deatha5 ]
{
};

void (float xx) hos_forward =
{
	local float rng;
	local entity te;
	local string qqq;
	local float qq;
	local float dir1;
	local float dir2;

	self.movetype = MOVETYPE_STEP;
	if ((self.cnt == MULTICAST_ALL))
	{
		return;
	}
	self.goalentity = self.friend;
	if ((vlen ((self.origin - self.goalentity.origin)) > 96))
	{
		movetogoal ((SECRET_1ST_DOWN * xx));
		self.angles = vectoangles ((self.goalentity.origin - self.origin));
		traceline (self.origin, ((self.origin + (v_right * SVC_BIGKICK)) + (v_forward * 90)), FALSE, self);
		dir1 = trace_fraction;
		traceline (self.origin, ((self.origin + (v_right * 50)) + (v_forward * 90)), FALSE, self);
		dir1 = (dir1 + trace_fraction);
		traceline (self.origin, ((self.origin + (v_right * 70)) + (v_forward * 90)), FALSE, self);
		dir1 = (dir1 + trace_fraction);
		traceline (self.origin, ((self.origin + (v_right * 90)) + (v_forward * 90)), FALSE, self);
		dir1 = (dir1 + trace_fraction);
		traceline (self.origin, ((self.origin + (v_right * 110)) + (v_forward * 90)), FALSE, self);
		dir1 = (dir1 + trace_fraction);
		traceline (self.origin, ((self.origin - (v_right * SVC_BIGKICK)) + (v_forward * 90)), FALSE, self);
		dir2 = trace_fraction;
		traceline (self.origin, ((self.origin - (v_right * 50)) + (v_forward * 90)), FALSE, self);
		dir2 = (dir2 + trace_fraction);
		traceline (self.origin, ((self.origin - (v_right * 70)) + (v_forward * 90)), FALSE, self);
		dir2 = (dir2 + trace_fraction);
		traceline (self.origin, ((self.origin - (v_right * 90)) + (v_forward * 90)), FALSE, self);
		dir2 = (dir2 + trace_fraction);
		traceline (self.origin, ((self.origin - (v_right * 110)) + (v_forward * 90)), FALSE, self);
		dir2 = (dir2 + trace_fraction);
		if ((self.oldorg == self.origin))
		{
			if ((dir1 > dir2))
			{
				self.angles_y = (self.angles_y - ((AS_MELEE * random ()) * SVC_INTERMISSION));
			}
			else
			{
				if ((dir2 > dir1))
				{
					self.angles_y = (self.angles_y + ((AS_MELEE * random ()) * SVC_INTERMISSION));
				}
				else
				{
					self.oldorg = (self.oldorg + '0 0 1');
				}
			}
			walkmove (self.angles_y, (SECRET_1ST_DOWN * xx));
		}
	}
	if ((vlen ((self.origin - self.goalentity.origin)) < IT_LIGHTNING))
	{
		self.angles = vectoangles ((self.goalentity.origin - self.origin));
		walkmove (self.angles_y, (-7 * xx));
	}
	if ((self.friend.team == PLAT_LOW_TRIGGER))
	{
		te = find (world, classname, "buyzone1");
	}
	if ((self.friend.team == SILENT))
	{
		te = find (world, classname, "buyzone2");
	}
	while (te)
	{
		rng = range (te);
		if (((rng <= RANGE_NEAR) && (self.rescued == MULTICAST_ALL)))
		{
			sound (self, CHAN_BODY, "misc/rescued.wav", PLAT_LOW_TRIGGER, ATTN_NONE);
			te = find (world, classname, "hostage");
			while (te)
			{
				if (((te.health > MULTICAST_ALL) && (te.rescued == MULTICAST_ALL)))
				{
					qq = (qq + PLAT_LOW_TRIGGER);
				}
				te = find (te, classname, "hostage");
			}
			self.rescued = PLAT_LOW_TRIGGER;
			qqq = ftos ((qq - PLAT_LOW_TRIGGER));
			bprint (SILENT, qqq);
			bprint (SILENT, " hostages left\n");
			setorigin (self, (self.origin - '0 0 512'));
		}
		te = find (te, classname, "buyzone");
	}
	self.oldorg = self.origin;
};
