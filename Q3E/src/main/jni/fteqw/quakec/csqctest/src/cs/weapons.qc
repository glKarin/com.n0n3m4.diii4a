
#define crandom() (random()*2 - 1)
.float starttime;
.vector source;
void() SUB_Remove =
{
	remove(self);
};

//Moves the entity to where it should be
float() RocketProject =
{
	local vector oldorg;
	oldorg = self.origin;
	self.origin = self.source + self.velocity * (time - self.starttime);

	//Do the trail thing, if possible.
	trailparticles(particleeffectnum("tr_rocket"), self, self.origin, oldorg);

	dynamiclight_add(self.origin, 400, '0 1 0');
	return PREDRAW_AUTOADD;
};

nonstatic void(float isnew) ParseRocketClass =
{
	self.starttime = time;
	self.source_x = readcoord();
	self.source_y = readcoord();
	self.source_z = readcoord();
	self.angles_x = (readshort()*360)/65535;
	self.angles_y = (readshort()*360)/65535;
	self.angles_z = random()*360;

	self.angles_x*=autocvar(r_meshpitch,-1);
	makevectors(self.angles);
	self.angles_x*=autocvar(r_meshpitch,-1);
	self.velocity = v_forward*1000;
	self.origin = self.source;

	setmodel(self, "progs/missile.mdl");
	self.drawmask = MASK_NORMAL;	//general view.
	self.predraw = RocketProject;
};

nonstatic float() NailProject =
{
	local vector oldorg;
	oldorg = self.origin;
	self.origin = self.source + self.velocity * (time - self.starttime);

	//Do the trail thing, if possible.
	trailparticles(particleeffectnum("tr_nail"), self, self.origin, oldorg);

//	adddynamiclight(self.origin, 100, '1 0 0');
	return PREDRAW_AUTOADD;
};

nonstatic void(float isnew) ParseNailClass =
{
	local float speed;
	self.starttime = time;
	self.source_x = readcoord();
	self.source_y = readcoord();
	self.source_z = readcoord();
	self.modelindex = readbyte();
	speed = readshort();
	self.angles_x = (readshort()*360)/65535;
	self.angles_y = (readshort()*360)/65535;
	self.angles_z = 0;	//don't rotate nails.

	self.angles_x*=autocvar(r_meshpitch,-1);
	makevectors(self.angles);
	self.angles_x*=autocvar(r_meshpitch,-1);
	self.velocity = v_forward*speed;
	self.origin = self.source;

	setmodelindex(self, self.modelindex);
	self.drawmask = MASK_NORMAL;	//general view.
	self.predraw = NailProject;
};

vector(float dm) VelocityForDamage =
{
	local vector v;

	v_x = 100 * crandom();
	v_y = 100 * crandom();
	v_z = 200 + 100 * random();

	dm*=-1;

	if (dm > -50)
	{
//		print ("level 1\n");
		v = v * 0.7;
	}
	else if (dm > -200)
	{
//		print ("level 2\n");
		v = v * 2;
	}
	else
	{
//		print ("level 3\n");
		v = v * 10;
	}

	return v;
};

float() GibProject =
{
	local float td;
	local vector oldorg;
	oldorg = self.origin;

	td = time - self.starttime;
	if (self.nextthink < time)
	{
		if (self.alpha <= td)
		{
			remove(self);
			return PREDRAW_NEXT;
		}
		self.alpha -= td;
	}

	Movetype_Bounce();
	trailparticles(particleeffectnum("tr_blood"), self, self.origin, oldorg);
	return PREDRAW_AUTOADD;
};

void(string gibname, float dm, vector org) ThrowGib =
{
	local	entity new;

	new = spawn();
	new.origin = org;
	setmodel (new, gibname);
	setsize (new, '0 0 0', '0 0 0');
	new.velocity = VelocityForDamage (dm);
	new.movetype = MOVETYPE_BOUNCE;
	new.solid = SOLID_NOT;
	new.avelocity_x = random()*600;
	new.avelocity_y = random()*600;
	new.avelocity_z = random()*600;
	new.think = SUB_Remove;
	new.nextthink = time + 10 + random()*10;
	new.frame = 0;
	new.flags = 0;

	new.alpha = 1;
	new.drawmask = MASK_NORMAL;

	new.predraw = GibProject;
	new.starttime = time;
};

void TossGibs(vector org, float type, float dm)
{
	local float giblevel;

	switch(type)
	{
	case GIB_ZOMBIE:
		sound (self, CHAN_VOICE, "zombie/z_gib.wav", 1, ATTN_NORM);
		break;
	default:
		sound (self, CHAN_VOICE, "player/udeath.wav", 1, ATTN_NORM);
		break;
	}

	giblevel = cvar("cg_giblevel");
	if (giblevel > 20)
		giblevel = 20;

	while(giblevel > 0)
	{
		giblevel--;
		switch(type)
		{
		case GIB_PLAYER:
			ThrowGib("progs/h_player.mdl", dm, org);
			ThrowGib("progs/gib1.mdl", dm, org);
			ThrowGib("progs/gib2.mdl", dm, org);
			ThrowGib("progs/gib3.mdl", dm, org);
			break;
		case GIB_SOLDIER:	//CHUNKY KIBBLES!!!
			ThrowGib("progs/h_guard.mdl", dm, org);
			ThrowGib("progs/gib1.mdl", dm, org);
			ThrowGib("progs/gib2.mdl", dm, org);
			ThrowGib("progs/gib3.mdl", dm, org);
			break;
		case GIB_DOG:
			ThrowGib("progs/h_guard.mdl", dm, org);
			ThrowGib("progs/gib3.mdl", dm, org);
			ThrowGib("progs/gib3.mdl", dm, org);
			ThrowGib("progs/gib3.mdl", dm, org);
			break;
		case GIB_DEMON:
			ThrowGib("progs/h_demon.mdl", dm, org);
			ThrowGib("progs/gib1.mdl", dm, org);
			ThrowGib("progs/gib2.mdl", dm, org);
			ThrowGib("progs/gib3.mdl", dm, org);
			break;
		case GIB_OGRE:
			ThrowGib("progs/h_ogre.mdl", dm, org);
			ThrowGib("progs/gib3.mdl", dm, org);
			ThrowGib("progs/gib3.mdl", dm, org);
			ThrowGib("progs/gib3.mdl", dm, org);
			break;
		case GIB_WIZARD:
			ThrowGib("progs/h_wizard.mdl", dm, org);
			ThrowGib("progs/gib2.mdl", dm, org);
			ThrowGib("progs/gib2.mdl", dm, org);
			ThrowGib("progs/gib2.mdl", dm, org);
			break;
		case GIB_SHAMBLER:
			ThrowGib("progs/h_shams.mdl", dm, org);
			ThrowGib("progs/gib1.mdl", dm, org);
			ThrowGib("progs/gib2.mdl", dm, org);
			ThrowGib("progs/gib3.mdl", dm, org);
			break;
		case GIB_ZOMBIE:
			ThrowGib("progs/h_zombie.mdl", dm, org);
			ThrowGib("progs/gib1.mdl", dm, org);
			ThrowGib("progs/gib2.mdl", dm, org);
			ThrowGib("progs/gib3.mdl", dm, org);
			break;
		case GIB_KNIGHT:
			ThrowGib("progs/h_knight.mdl", dm, org);
			ThrowGib("progs/gib1.mdl", dm, org);
			ThrowGib("progs/gib2.mdl", dm, org);
			ThrowGib("progs/gib3.mdl", dm, org);
			break;

		case GIB_HELLKNIGHT:
			ThrowGib("progs/h_hellkn.mdl", dm, org);
			ThrowGib("progs/gib1.mdl", dm, org);
			ThrowGib("progs/gib2.mdl", dm, org);
			ThrowGib("progs/gib3.mdl", dm, org);
			break;
		case GIB_FISH:
			ThrowGib("progs/gib1.mdl", dm, org);
			ThrowGib("progs/gib2.mdl", dm, org);
			ThrowGib("progs/gib3.mdl", dm, org);
			break;
		case GIB_ENFORCER:
			ThrowGib("progs/h_mega.mdl", dm, org);
			ThrowGib("progs/gib1.mdl", dm, org);
			ThrowGib("progs/gib2.mdl", dm, org);
			ThrowGib("progs/gib3.mdl", dm, org);
			break;
		case GIB_SHALRATH:
			ThrowGib("progs/h_shal.mdl", dm, org);
			ThrowGib("progs/gib1.mdl", dm, org);
			ThrowGib("progs/gib2.mdl", dm, org);
			ThrowGib("progs/gib3.mdl", dm, org);
			break;
		default:
			ThrowGib("progs/gib1.mdl", dm, org);
			ThrowGib("progs/gib2.mdl", dm, org);
			ThrowGib("progs/gib3.mdl", dm, org);
			break;
		}
	}
};

nonstatic void(float isnew) ParseGibbing =
{
	float type;
	vector org;
	float dm;

	type = readbyte();
	dm = readbyte();
	org_x = readcoord();
	org_y = readcoord();
	org_z = readcoord();

	if (!isnew)	//doesn't count if it's not new.
		return;

	self.origin = org;
	TossGibs(org, type, dm);
}

nonstatic void(float isnew) ParseExplosion =
{
	local vector org;
	local entity e;
	org_x = readcoord();
	org_y = readcoord();
	org_z = readcoord();
	self.origin = org;
	if (isnew)
	{
		sound (self, CHAN_WEAPON, "weapons/r_exp3.wav", 1, ATTN_NORM);
#ifdef WORKINDP
		pointparticles(particleeffectnum("te_explosion"), org, '0 0 0', 1);
#else
		pointparticles(particleeffectnum("te_explosion"), org);
#endif

		e = world;//findradius(org, 120);
		while (e)
		{
			//a bit of fun - allow the local client to gib csqc bodies
			if (e.gibbable)
			{
				TossGibs(e.origin+'0 0 16', e.gibbable, 20);
				e.gibbable = GIB_BAD;
				e.removetime = 0;
			}
			e = e.chain;
		}
	}

	self.drawmask = 0;	//don't draw it any more.
};
