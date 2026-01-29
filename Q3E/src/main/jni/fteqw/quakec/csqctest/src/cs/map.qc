.string target;
.string targetname;
.vector mangle;
.string message;

void() cs_teleport_touch =
{
	local entity t;
	t = find(world, targetname, self.target);
	if (t)
	{
		makevectors(t.angles);
		Pred_Predict_Teleport(t.origin + '0 0 27', v_forward*300, t.angles);
	}
};

void() spawn_trigger_teleport =
{
	self.solid = SOLID_TRIGGER;
	setmodel(self, self.model);
	self.model = "";

	self.touch = cs_teleport_touch;
};

void() spawn_info_teleport_destination =
{
};

void() spawn_trigger_push =
{
	self.solid = SOLID_TRIGGER;
	setmodel(self, self.model);
	self.model = "";

//	self.touch = cs_teleport_touch;
};

void() spawn_worldspawn =
{
	levelname = self.message;
	remove(self);
};

float() parsenewmapentity =
{
	local string field, value;
	local entity nent;
	local void() spawnfunc = __NULL__;
	nent = spawn();

	while (1)
	{
		field = getentitytoken();
		if not (field)
			break;

		if (field == "}")
		{
			if (!nent.classname)
				break;
			if (spawnfunc)
			{
				self = nent;
				spawnfunc();
				return true;
			}

//			print("ignoring ", nent.classname, "\n");
			//I don't know what you are, go away.
			if (nent.classname!="")
				strunzone(nent.classname);
			if (nent.message!="")
				strunzone(nent.message);
			remove(nent);
			return true;
		}

		value = getentitytoken();
		if not (value)
			break;

//		print("ent field ", field, " has value ", value, "\n");

		if (field == "classname")
		{
			if (value == "trigger_teleport")
				spawnfunc = spawn_trigger_teleport;
			if (value == "info_teleport_destination")
				spawnfunc = spawn_info_teleport_destination;
			if (value == "trigger_push")
				spawnfunc = spawn_trigger_push;
			if (value == "worldspawn")
				spawnfunc = spawn_worldspawn;
			nent.classname = strzone(value);
		}
		else if (field == "targetname")
			nent.targetname = strzone(value);
		else if (field == "target")
			nent.target = strzone(value);
		else if (field == "origin")
			nent.origin = stov(value);
		else if (field == "angles")
			nent.angles = stov(value);
		else if (field == "angle")
		{
			nent.angles_x = 0;
			nent.angles_y = stof(value);
			nent.angles_z = 0;
		}
		else if (field == "mangle")
			nent.angles = stov(value);
		else if (field == "model")
			nent.model = value;
		else if (field == "message")
			nent.message = strzone(value);
/*		else if (field == "light");
		else if (field == "mangle");
		else if (field == "killtarget");
		else if (field == "wad");
		else if (field == "height");
		else if (field == "lip");
		else if (field == "count");
		else if (field == "dmg");
		else if (field == "delay");
		else if (field == "worldtype");
		else if (field == "sounds");
		else if (field == "map");
		else if (field == "spawnflags");
		else if (field == "wait");
		else if (field == "style");
		else if (field == "health");
		else if (field == "speed");
		else
			print("ignoring field ", field, "\n");
*/
	}

	remove(nent);

	return false;
};

void() CSQC_WorldLoaded =
{
	local string tok;

	while (1)
	{
		tok = getentitytoken();

		if (tok == "")
			break;

		if (tok != "{")
		{
			print("bad entity data\n");
			return;
		}
		
		if (!parsenewmapentity())
		{
			print("bad entity data\n");
			return;
		}
	}
};
