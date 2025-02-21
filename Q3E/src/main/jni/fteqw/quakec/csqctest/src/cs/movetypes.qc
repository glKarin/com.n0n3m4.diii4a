//Emulates the movetype using the 'predraw' function.

.float starttime;

#define RunThink() do {if (self.nextthink && self.nextthink <= time){float otime;otime = time;time = self.nextthink;self.think();time = otime;}} while(0)

void(entity e1, entity e2) Impact =
{
	entity oself;
	entity oother;
	oself = self;
	oother = other;
	if (e1.touch && e1.solid)
	{
		self = e1;
		other = e2;
		self.touch();
	}
	if (e2.touch && e2.solid)
	{
		self = e2;
		other = e1;
		self.touch();
	}
	self = oself;
	other = oother;
};

//returns the time that it didn't manage to move.
float(float time_left) FlyMove =
{
	float impacts;
	vector end;

	for(impacts = 4; impacts; impacts--)
	{
		end = self.origin + self.velocity*time_left;
		tracebox(self.origin, self.mins, self.maxs, end, FALSE, self);
		if (trace_startsolid)
			return time_left;

		self.origin = trace_endpos;
		if (trace_fraction == 1)
		{
			return 0;	//w00t
		}

		if (trace_plane_normal_z > 0.7)
			self.flags |= FL_ONGROUND;

		Impact(self, trace_ent);
		if (wasfreed(self))
			return 0;

		//slide them along the wall.
		self.velocity = self.velocity - (trace_plane_normal*self.velocity)*trace_plane_normal;


		time_left -= time_left * trace_fraction;
	}
	return time_left;
};
void(vector end) PushSelf =
{
	end += self.origin;
	tracebox(self.origin, self.mins, self.maxs, end, FALSE, self);
	if (trace_startsolid)
		return;

	self.origin = trace_endpos;
	if (trace_fraction == 1)
		return;

	Impact(self, trace_ent);
};

//MOVETYPE_BOUNCE and MOVETYPE_TOSS
nonstatic void() Movetype_Bounce =
{
	local vector dest;
	local float td;

	RunThink();

	td = time - self.starttime;
	self.starttime = time;

	if (self.flags & FL_ONGROUND)
		return;

	self.velocity = self.velocity - '0 0 800'*td;
	dest = self.origin + self.velocity * td;
	tracebox(self.origin, self.mins, self.maxs, dest, TRUE, self);
	if (trace_startsolid)
	{
		trace_fraction = 0;
		trace_endpos = self.origin;
	}
	if (trace_fraction < 1)
	{
		dest = trace_endpos;
		self.avelocity = self.avelocity*0.5;
		//bounce the gib off the surface.
		if (self.movetype == MOVETYPE_BOUNCE)
			self.velocity = self.velocity - (trace_plane_normal*self.velocity)*trace_plane_normal*1.5;	//vectorize the bounce vector, and multiply with the bounce value
		else
			self.velocity = self.velocity - (trace_plane_normal*self.velocity)*trace_plane_normal;	//vectorize the bounce vector, and multiply with the bounce value

		if (trace_plane_normal_z > 0.7 && self.velocity_z < 60)
		{
			self.velocity = '0 0 0';
			self.flags |= FL_ONGROUND;
		}
	}
	self.angles = self.angles + self.avelocity*td;

	//Do the trail thing, if possible.

	self.origin = dest;
	setorigin(self, self.origin);
};

void() Movetype_Walk =
{
	float tl, td;
	RunThink();

	td = time - self.starttime;
	self.starttime = time;

	tl = FlyMove(td);
	if (self.flags & FL_WATERJUMP)	//waterjumps, yay.
		return;

	if (!trace_plane_normal_z)
	{	//a vertical wall
		PushSelf('0 0 22');	//so step up
		self.velocity_z = 0;
		FlyMove(tl);		//continue going forwards
		PushSelf('0 0 -22');	//and step down again
	}
	setorigin(self, self.origin);
};

void() Movetype_Noclip =
{
	float td;
	RunThink();

	td = time - self.starttime;
	self.starttime = time;

	self.angles += self.avelocity*td;
	self.origin += self.velocity*td;
};

void() Movetype_Nomove =
{
	RunThink();
};