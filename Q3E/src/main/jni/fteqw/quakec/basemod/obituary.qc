// client death messages
enum {
	MOD_NONE,
	MOD_AXE,
	MOD_SHOTGUN,
	MOD_SUPERSHOTGUN,
	MOD_SPIKE,
	MOD_SUPERSPIKE,
	MOD_GRENADE,
	MOD_ROCKET,
	MOD_ROCKETRADIUS,
	MOD_SHAFT,
	MOD_SHAFTWATER,
	MOD_SHAFTSLIME,
	MOD_SHAFTLAVA,
	MOD_TELEFRAG,
	MOD_TELEFRAGDEFLECT,
	MOD_SQUISH,
	MOD_DROWN,
	MOD_SLIME,
	MOD_LAVA,
	MOD_EXPLOBOX,
	MOD_FALL,
	MOD_FIREBALL,
	MOD_EXIT,
	MOD_LASER,
	MOD_SELFWATER,
	MOD_HURT,
	MOD_DOG,
	MOD_SOLDIER,
	MOD_ENFORCER,
	MOD_OGRE,
	MOD_WIZARD,
	MOD_DEMON,
	MOD_KNIGHT,
	MOD_HKNIGHT,
	MOD_SHALRATH,
	MOD_SHAMBLER,
	MOD_FISH,
	MOD_TARBABY,
	MOD_ZOMBIE,
	MOD_CHTHON
};

void(string targ, INTEGER mod) SuicideMessage =
{
	string s, t, u;

	s = "";
	t = "";
	u = "";

	switch (mod)
	{
	case MOD_GRENADE:
		s = targ;
		t = " tries to put the pin back in";
		break;
	case MOD_SHAFTWATER:
		s = targ;
		t = " discharges into the water.";
		break;
	case MOD_SHAFTSLIME:
		s = targ;
		t = " discharges into the slime";
		break;
	case MOD_SHAFTLAVA:
		s = targ;
		t = " discharges into the lava";
		break;
	case MOD_TELEFRAGDEFLECT:
		s = "Satan's power deflects ";
		t = targ;
		u = "'s telefrag";
		break;
	case MOD_SELFWATER:
		s = targ;
		t = " electrocutes himself.";
		break;
	case MOD_EXPLOBOX:
		s = targ;
		t = " blew himself up";
		break;
	default:
		s = targ;
		t = " becomes bored with life";
	}

	bprint4(PRINT_MEDIUM, s, t, u, "\n");
}

void(string targ, string attacker, INTEGER mod) KillMessage =
{
	string s, t, u, v, w;

	s = "";
	t = "";
	u = "";
	v = "";
	w = "";

	switch (mod)
	{
	case MOD_AXE:
		s = targ;
		t = " was ax-murdered by ";
		u = attacker;
		break;
	case MOD_SHOTGUN:
		s = targ;
		t = " chewed on ";
		u = attacker;
		v = "'s boomstick";
		break;
	case MOD_SUPERSHOTGUN:
		s = targ;
		t = " ate two loads of ";
		u = attacker;
		v = "'s buckshot";
		break;
	case MOD_SPIKE:
		s = targ;
		t = " was nailed by ";
		u = attacker;
		break;
	case MOD_SUPERSPIKE:
		s = targ;
		t = " was punctured by ";
		u = attacker;
		break;
	case MOD_GRENADE:
		s = targ;
		t = " eats ";
		u = attacker;
		v = "'s pineapple";
		break;
	case MOD_ROCKET:
	case MOD_ROCKETRADIUS:
		s = targ;
		t = " rides ";
		u = attacker;
		v = "'s rocket";
		break;
	case MOD_SHAFT:
		s = targ;
		t = " accepts ";
		u = attacker;
		v = "'s shaft";
		break;
	case MOD_SHAFTWATER:
	case MOD_SHAFTSLIME:
	case MOD_SHAFTLAVA:
		s = targ;
		t = " accepts ";
		u = attacker;
		v = "'s discharge";
		break;
	case MOD_TELEFRAG:
		s = targ;
		t = " was telefragged by ";
		u = attacker;
		break;
	case MOD_TELEFRAGDEFLECT:
		s = targ;
		t = " was telefragged by ";
		u = attacker;
		v = "'s Satan's power";
		break;
	case MOD_SQUISH:
		s = attacker;
		t = " squishes ";
		u = targ;
		break;
	case MOD_EXPLOBOX:
		s = targ;
		t = " was blown up by ";
		u = attacker;
		break;
	default:
		s = targ;
		t = " was killed by ";
		u = attacker;
	}

	bprint6(PRINT_MEDIUM, s, t, u, v, w, "\n");
}

void(string targ, string attacker, INTEGER mod) TeamKillMessage =
{
	string s, t, u, v, w;
	float rnum;

	s = "";
	t = "";
	u = "";
	v = "";
	w = "";

	switch (mod)
	{
	case MOD_SQUISH:
		s = attacker;
		t = " squishes teammate ";
		u = targ;
		break;
	default:
		rnum = random();

		s = attacker;

		if (rnum < 0.25)
			t = " mows down teammate ";
		else if (rnum < 0.5)
		{
			t = " checks his glasses and sees ";
			v = " dead";
		}
		else if (rnum < 0.75)
		{
			t = " frags ";
			v = " for the other team";
		}
		else
			t = " loses his friend ";

		u = targ;
	}

	bprint6(PRINT_MEDIUM, s, t, u, v, w, "\n");
}

void(string targ, INTEGER mod) WorldKillMessage =
{
	string s, t, u;

	s = "";
	t = "";
	u = "";

	switch (mod)
	{
	case MOD_SPIKE:
	case MOD_SUPERSPIKE:
		s = targ;
		t = " was spiked";
		break;
	case MOD_SQUISH:
		s = targ;
		t = "  was squished";
		break;
	case MOD_DROWN:
		s = targ;

		if (random() < 0.5)
			t = " sleeps with the fishes";
		else
			t = " sucks it down";
		break;
	case MOD_SLIME:
		s = targ;

		if (random() < 0.5)
			t = " gulped a load of slime";
		else
			t = " can't exist on slime alone";
		break;
	case MOD_LAVA:
		s = targ;

		if (random() < 0.5)
			t = " turned into hot slag";
		else
			t = " visits the Volcano God";
		break;
	case MOD_EXPLOBOX:
		s = targ;
		t = " blew up";
		break;
	case MOD_FALL:
		s = targ;
		t = " fell to his death";
		break;
	case MOD_FIREBALL:
		s = targ;
		t = " ate a lavaball";
		break;
	case MOD_EXIT:
		s = targ;
		t = " tried to leave";
		break;
	case MOD_LASER:
		s = targ;
		t = " was zapped";
		break;
#ifdef MONSTERS
	case MOD_DOG:
		s = targ;
		t = " was mauled by a Rottweiler";
		break;
	case MOD_SOLDIER:
		s = targ;
		t = " was shot by a Grunt";
		break;
	case MOD_ENFORCER:
		s = targ;
		t = " was blasted by an Enforcer";
		break;
	case MOD_OGRE:
		s = targ;
		t = " was destroyed by an Ogre";
		break;
	case MOD_WIZARD:
		s = targ;
		t = " was scragged by a Scrag";
		break;
	case MOD_DEMON:
		s = targ;
		t = " was eviscerated by a Fiend";
		break;
	case MOD_KNIGHT:
		s = targ;
		t = " was slashed by a Knight";
		break;
	case MOD_HKNIGHT:
		s = targ;
		t = " was slain by a Death Knight";
		break;
	case MOD_SHALRATH:
		s = targ;
		t = "  was exploded by a Vore";
		break;
	case MOD_SHAMBLER:
		s = targ;
		t = " was smashed by a Shambler";
		break;
	case MOD_FISH:
		s = targ;
		t = " was fed to the Rotfish";
		break;
	case MOD_TARBABY:
		s = targ;
		t = " was slimed by a Spawn";
		break;
	case MOD_ZOMBIE:
		s = targ;
		t = " joins the Zombies";
		break;
	case MOD_CHTHON:
		s = targ;
		t = " fell to Chthon's wrath";
		break;
#endif
	default:
		s = targ;
		t = " died";
	}

	bprint4(PRINT_MEDIUM, s, t, u, "\n");
}