


void(float cost, float type) BuyGrenade =
{
	local string z;
	local float x, y;

	if (self.ammo_shells < cost)
	{
		self.currentmenu = "none";
		sprint(self, PRINT_HIGH, "not enough money.\n");
		return;
	}

	sound (self, CHAN_BODY, "misc/item1.wav", 1, ATTN_NORM);
	self.ammo_shells = self.ammo_shells - cost;

	sprint(self, PRINT_HIGH, "you bought a ");

	if (type == 1)
		y = IID_GREN_SMOKE;
	if (type == 2)
		y = IID_GREN_FRAG;
	if (type == 3)
		y = IID_GREN_EMP;

	z = GetItemName(y);
	sprint(self, PRINT_HIGH, z);
	sprint(self, PRINT_HIGH, "\n");

	//put grenade in inventory
	//finds existing grenades, otherwise, empty slot

	x = SlotOfItem(self, y);

	SlotVal(IID_GREN_SMOKE, 500)

	if (x != 0) //found existing grenades
		SetItemSlot(self, x, SlotVal(y, 1));
	if (x == 0)
		x = FindEmptySlot(self, y);

		SetItemSlot(self, x, SlotVal(y, 1));
};


void() BuyBandages =
{
	local string y;

	if (self.ammo_shells < 2)
	{
		self.currentmenu = "none";
		sprint(self, PRINT_HIGH, "not enough money.\n");
		return;
	}
	if (self.class != 2)
	{
		self.currentmenu = "none";
		sprint(self, PRINT_HIGH, "not a medic.\n");
		return;
	}
	if (self.bandages >= 50)
	{
		self.currentmenu = "none";
		sprint(self, PRINT_HIGH, "too many bandages.\n");
		return;
	}

	sound (self, CHAN_BODY, "misc/item1.wav", 1, ATTN_NORM);
	self.bandages = self.bandages + 25;
	self.ammo_shells = self.ammo_shells - 2;
	if (self.bandages > 50)
		self.bandages = 50;
	y = ftos(self.bandages);
	sprint(self, PRINT_HIGH, "you now have ");
	sprint(self, PRINT_HIGH, y);
	sprint(self, PRINT_HIGH, "/50 bandages.\n");	
};

void(float cost, float type) BuyChem =
{
	local string x;
	local float y;

	if (self.ammo_shells < cost)
	{
		self.currentmenu = "none";
		sprint(self, PRINT_HIGH, "not enough money.\n");
		return;
	}
	if (type >= 2 && self.class != 1)
	{
		self.currentmenu = "none";
		sprint(self, PRINT_HIGH, "not a medic.\n");
		return;
	}
	if (self.equipment == 1)
		y = 4;
	else
		y = 2;

	if (self.chemcount >= y)
	{
		self.currentmenu = "none";
		sprint(self, PRINT_HIGH, "you can't hold any more chems.\n");
		return;
	}

	sound (self, CHAN_BODY, "misc/item1.wav", 1, ATTN_NORM);
	self.chemcount = y;
	self.chem = type;
	self.ammo_shells = self.ammo_shells - cost;
	x = GetChemName();
	sprint (self, PRINT_HIGH, x);
	sprint(self, PRINT_HIGH, " purchased.\n");
};

void(string type) ChangeAmmo =
{

	if (self.ammo_shells < 1)
	{
		self.currentmenu = "none";
		sprint(self, PRINT_HIGH, "not enough money.\n");
		return;
	}

	
	sound (self, CHAN_BODY, "misc/item1.wav", 1, ATTN_NORM);

	if (self.current_slot == 1)
		self.ammotype1 = type;
	if (self.current_slot == 2)
		self.ammotype2 = type;
};

void() BuyScraps =
{
	local string y;

	if (self.ammo_shells < 5)
	{
		self.currentmenu = "none";
		sprint(self, PRINT_HIGH, "not enough money.\n");
		return;
	}
	if (self.class != 6)
	{
		self.currentmenu = "none";
		sprint(self, PRINT_HIGH, "not a scientist.\n");
		return;
	}
	if (self.scraps >= 15)
	{
		self.currentmenu = "none";
		sprint(self, PRINT_HIGH, "too many metal scraps.\n");
		return;
	}

	sound (self, CHAN_BODY, "items/armor1.wav", 1, ATTN_NORM);
	self.scraps = self.scraps + 5;
	self.ammo_shells = self.ammo_shells - 5;
	if (self.scraps > 15)
		self.scraps = 15;
	y = ftos(self.scraps);
	sprint(self, PRINT_HIGH, "you now have ");
	sprint(self, PRINT_HIGH, y);
	sprint(self, PRINT_HIGH, "/15 scraps.\n");

};


void (float wt, float cost, float item) BuyWeapon =
{
	sprint (self, PRINT_HIGH, "The shop is not open today.\n");
	self.currentmenu = "none";
	SetWeaponModel ();
	return;

/*
	local float lbs;
	local string qq;
	local float curr;
	local float c3;
	local string c1;
	local string c2;

	lbs = weightx ();

	if ((self.current_slot == 1))
	{
		curr = self.slot1_weight;

		if (self.slot2 == 0)
			self.slot2 = self.slot1;
		if (self.slot1 != 0)
		{
			DropWeapon (self.slot1, 1, 0);
			self.slot1 = 0;
		}
		GetWeaponModel ();
	}
	if ((self.current_slot == 2))
	{
		curr = self.slot2_weight;

		if (self.slot1 == 0)
			self.slot1 = self.slot2;
		if (self.slot2 != 0)
		{
			DropWeapon (self.slot2, 1, 0);
			self.slot2 = 0;
		}

		GetWeaponModel ();
	}
	if ((lbs + wt - curr) > self.max_weight)
	{
		sprint (self, PRINT_HIGH, "you can't carry that much.\n");
		self.currentmenu = "none";
		GetWeaponModel ();
		return;
	}
	if (cost > self.ammo_shells)
	{
		sprint (self, PRINT_HIGH, "not enough money.\n");
		self.currentmenu = "none";
		GetWeaponModel ();
		return;
	}
	self.ammo_shells = self.ammo_shells - cost;
	if ((self.current_slot == 1))
	{
		GetWeaponWeight (self, self.current_slot);
		stuffcmd (self, "impulse 1\n");
		self.slot1 = item;
		self.items = (self.items | IT_SUPER_NAILGUN);
		GetWeaponModel ();
	}
	if (self.current_slot == 2)
	{
		GetWeaponWeight (self, self.current_slot);
		stuffcmd (self, "impulse 2\n");
		self.slot2 = item;
		self.items = (self.items | IT_GRENADE_LAUNCHER);
		GetWeaponModel ();
	}
	qq = GetWeaponName (self, item);
	sprint (self, PRINT_HIGH, qq);
	sprint (self, PRINT_HIGH, " purchased.\n");
	sound (self, CHAN_BODY, "misc/item2.wav", 1, ATTN_NORM);
	self.currentmenu = "none";
	WeaponAmmo (SPAWNFLAG_SUPERSPIKE);
	WeaponAmmo (SPAWNFLAG_LASER);
	GetWeaponWeight (self, 1);
	GetWeaponWeight (self, 2);
	GetWeaponModel ();
	return;
*/
};

void (float cost, float item) BuyPerk =
{
	if (self.frags < cost)
	{
		sprint (self, PRINT_HIGH, "not enough kills\n");
		self.currentmenu = "none";
		return;
	}
	sprint (self, PRINT_HIGH, "ok, ability gained.\n");
	self.perk = item;
	self.currentmenu = "none";
	return;
};

void (float wt, float cost, float item) BuyArmor =
{
	local float curr;
	local float lbs;
	local string x;

	lbs = weightx ();
	if (((item >= TE_LIGHTNING2) && (self.protect >= SPAWNFLAG_SUPERSPIKE)))
	{
		sprint (self, PRINT_HIGH, "remove your hardware before buying\nany kind of advanced armor!\ntoo much interference otherwise.");
		self.currentmenu = "none";
		return;
	}
	if ((cost > self.ammo_shells))
	{
		sprint (self, PRINT_HIGH, "not enough money.\n");
		self.currentmenu = "none";
		return;
	}
	if ((((lbs + wt) - self.armor_weight) > self.max_weight))
	{
		sprint (self, PRINT_HIGH, "you can't carry that much.\n");
		self.currentmenu = "none";
		return;
	}
	sound (self, CHAN_BODY, "misc/item2.wav", 1, ATTN_NORM);
	self.ammo_shells = (self.ammo_shells - cost);
	self.armor_weight = wt;
	self.armor = item;
	x = GetArmorName();
	sprint (self, PRINT_HIGH, x);
	sprint (self, PRINT_HIGH, " purchased.\n");

	return;
};

void (float cost, float item) BuyEquipment =
{
	local float lbs;
	local string x;

	lbs = weightx ();
	if ((cost > self.ammo_shells))
	{
		sprint (self, PRINT_HIGH, "not enough money.\n");
		self.currentmenu = "none";
		return;
	}
	sound (self, CHAN_BODY, "items/item1.wav", 1, ATTN_NORM);
	self.equipment = item;
	x = GetEquipmentName();
	sprint (self, PRINT_HIGH, x);
	sprint (self, PRINT_HIGH, " purchased.\n");
	self.ammo_shells = (self.ammo_shells - cost);
	return;
};

void (float cost, float item) BuyProtect =
{
	local string x;

	if ((self.armor >= TE_LIGHTNING2))
	{
		sprint (self, PRINT_HIGH, "too many electronics in\nyour currently worn armor!");
		self.currentmenu = "none";
		return;
	}
	if ((cost > self.ammo_shells))
	{
		sprint (self, PRINT_HIGH, "not enough money.\n");
		self.currentmenu = "none";
		return;
	}
	sound (self, CHAN_BODY, "items/item1.wav", 1, ATTN_NORM);
	self.ammo_shells = (self.ammo_shells - cost);
	self.protect = item;
	x = GetProtectName();
	sprint (self, PRINT_HIGH, x);
	sprint (self, PRINT_HIGH, " purchased.\n");

	return;
};


float (float input) overweight =
{
	local float tmp;
	local float max;

	tmp = ((((input) + self.armor_weight) + self.slot1_weight) + self.slot2_weight);
	max = self.max_weight;
	if ((self.class == TE_LIGHTNING2))
	{
		max = (max + SPAWNFLAG_LASER);
	}
	if ((self.perk == TE_LAVASPLASH))
	{
		return (FALSE);
	}
	if ((tmp > max))
	{
		return (TRUE);
	}
	else
	{
		return (FALSE);
	}
};

float () weightx =
{
	local float tmp;

	tmp = self.slot1_weight + self.slot2_weight + self.armor_weight;
	return tmp;
};

void () W_GetClass =
{
	if ((self.currentmenu == "select_skill"))
	{
		sound (self, CHAN_WEAPON, "buttons/switch02.wav", TRUE, ATTN_NORM);
		if (self.impulse == 1)
		{
			self.currentmenu = "none";
			self.max_health = 80;
			self.class = 1;
			self.currentmenu = "confirm_skill";
			centerprint (self, "êyour class will beë\n\nMedic - OK?\nê1ë Yes     \nê2ë No      \n");
			self.ghost = 0;
			return;
		}
		if ((self.impulse == 2))
		{
			self.currentmenu = "none";
			self.max_health = 70;
			self.class = 2;
			self.currentmenu = "confirm_skill";
			centerprint (self, "êyour class will beë\n\nAssassin - OK?\nê1ë Yes     \nê2ë No      \n");
			self.ghost = 0;
			return;
		}
		if (self.impulse == 3)
		{
			self.currentmenu = "none";
			self.max_health = 100;
			self.class = 3;
			self.currentmenu = "confirm_skill";
			centerprint (self, "êyour class will beë\n\nSoldiier - OK?\nê1ë Yes     \nê2ë No      \n");
			self.ghost = 0;
			return;
		}
		if (self.impulse == 4)
		{
			self.max_health = 80;
			self.currentmenu = "none";
			self.class = 4;
			self.currentmenu = "confirm_skill";
			centerprint (self, "êyour class will beë\n\nScientist - OK?\nê1ë Yes     \nê2ë No      \n");
			self.ghost = 0;
			return;
		}
	}
	if (self.impulse > 4)
		return;

};


void() W_PlayerMenu =
{
	if (self.currentmenu == "none")
		return;

	if (self.currentmenu == "shop_list")
	{
		if (self.impulse == 1)
			self.currentmenu = "shop_trait";
		if (self.impulse == 2)
			self.currentmenu = "shop_perk";
		if (self.impulse == 3)
			self.currentmenu = "shop_armor";
		if (self.impulse == 4)
			self.currentmenu = "shop_protect";
		if (self.impulse == 5)
			self.currentmenu = "shop_weapons";
		if (self.impulse == 6)
			self.currentmenu = "shop_equipment";
		if (self.impulse == 7)
			self.currentmenu = "shop_chems";
		if (self.impulse == 8)
			self.currentmenu = "shop_other";

		DisplayMenu();
		return;
	}

	if (self.currentmenu == "shop_trait")
	{
		if (self.impulse == 1)
			self.currentmenu = "shop_trait";
		if (self.impulse == 2)
			self.currentmenu = "shop_perk";
		if (self.impulse == 3)
			self.currentmenu = "shop_armor";
		if (self.impulse == 4)
			self.currentmenu = "shop_protect";
		if (self.impulse == 5)
			self.currentmenu = "shop_weapons";
		if (self.impulse == 6)
			self.currentmenu = "shop_equipment";
		if (self.impulse == 7)
			self.currentmenu = "shop_chems";
		if (self.impulse == 8)
			self.currentmenu = "shop_other";

		DisplayMenu();
		return;
	}
	if (self.currentmenu == "shop_perk")
	{
		if (self.impulse == 1)
                  BuyPerk(1, 1);
		if (self.impulse == 2)
                  BuyPerk(1, 2);
		if (self.impulse == 3)
                  BuyPerk(1, 3);
		if (self.impulse == 4)
                  BuyPerk(2, 4);
		if (self.impulse == 5)
                  BuyPerk(2, 5);
		if (self.impulse == 6)
                  BuyPerk(2, 6);
		if (self.impulse == 7)
                  BuyPerk(2, 7);
		if (self.impulse == 8)
                  BuyPerk(3, 8);
		if (self.impulse == 9)
                  BuyPerk(4, 9);

			return;
	}
	if (self.currentmenu == "shop_armor")
	{
		if (self.impulse == 1)
                  BuyArmor(3, 03, 1);         //weight, cost, item
		if (self.impulse == 2)
                  BuyArmor(7, 05, 2);         //weight, cost, item
		if (self.impulse == 3)
                  BuyArmor(9, 10, 3);         //weight, cost, item
		if (self.impulse == 4)
                  BuyArmor(15, 12, 4);         //weight, cost, item
		if (self.impulse == 5)
                  BuyArmor(12, 25, 5);         //weight, cost, item
		if (self.impulse == 6)
                  BuyArmor(17, 35, 6);         //weight, cost, item
		if (self.impulse == 7)
                  BuyArmor(5, 45, 7);         //weight, cost, item
		if (self.impulse == 8)
                  BuyArmor(20, 55, 8);         //weight, cost, item

			return;
	}
	if (self.currentmenu == "shop_protect")
	{
		if (self.impulse == 1)
                  BuyProtect(20, 1);
		if (self.impulse == 2)
                  BuyProtect(20, 2);
		if (self.impulse == 3)
                  BuyProtect(30, 3);
		if (self.impulse == 4)
                  BuyProtect(30, 4);
		if (self.impulse == 5)
                  BuyProtect(40, 5);

			return;
	}
	if (self.currentmenu == "shop_weapons")
	{
		if (self.impulse == 1)
			self.currentmenu = "shop_melee";
		if (self.impulse == 2)
			self.currentmenu = "shop_thrown";
		if (self.impulse == 3)
			self.currentmenu = "shop_pistols";
		if (self.impulse == 4)
			self.currentmenu = "shop_shotguns";
		if (self.impulse == 5)
			self.currentmenu = "shop_rifles";

		DisplayMenu();
		return;
	}
	if (self.currentmenu == "shop_melee")
	{
		if (self.impulse == 1)
                  BuyWeapon(1, 1, 1);         //weight, cost, item
		if (self.impulse == 2)
                  BuyWeapon(6, 3, 2);         //weight, cost, item
		if (self.impulse == 3)
                  BuyWeapon(3, 10, 3);         //weight, cost, item
		if (self.impulse == 4)
                  BuyWeapon(10, 15, 4);         //weight, cost, item

			return;
	}
	if (self.currentmenu == "shop_thrown")
	{
		if (self.impulse == 1)
                  BuyGrenade(3, 1);         //weight, cost, item
		if (self.impulse == 2)
                  BuyGrenade(4, 2);         //weight, cost, item
		if (self.impulse == 3)
                  BuyGrenade(5, 3);         //weight, cost, item
		if (self.impulse == 4)
                  BuyGrenade(6, 4);         //weight, cost, item

			return;
	}
	if (self.currentmenu == "shop_pistols")
	{
		if (self.impulse == 1)
                  BuyWeapon(1, 5, 5);         //weight, cost, item
		if (self.impulse == 2)
                  BuyWeapon(2, 7, 6);         //weight, cost, item
		if (self.impulse == 3)
                  BuyWeapon(2, 9, 7);         //weight, cost, item
		if (self.impulse == 4)
                  BuyWeapon(2, 21, 8);         //weight, cost, item
		if (self.impulse == 5)
                  BuyWeapon(3, 14, 13);         //weight, cost, item
		if (self.impulse == 6)
                  BuyWeapon(3, 17, 14);         //weight, cost, item

			return;
	}
	if (self.currentmenu == "shop_shotguns")
	{
		if (self.impulse == 1)
                  BuyWeapon(3, 3, 9);         //weight, cost, item
		if (self.impulse == 2)
                  BuyWeapon(4, 7, 10);         //weight, cost, item
		if (self.impulse == 3)
                  BuyWeapon(5, 12, 11);         //weight, cost, item
		if (self.impulse == 4)
                  BuyWeapon(7, 34, 12);         //weight, cost, item

			return;
	}
	if (self.currentmenu == "shop_rifles")
	{
		if (self.impulse == 1)
                  BuyWeapon(3, 11, 15);         //weight, cost, item
		if (self.impulse == 2)
                  BuyWeapon(4, 21, 16);         //weight, cost, item
		if (self.impulse == 3)
                  BuyWeapon(5, 25, 17);         //weight, cost, item
		if (self.impulse == 4)
                  BuyWeapon(6, 28, 18);         //weight, cost, item
		if (self.impulse == 4)
                  BuyWeapon(5, 30, 19);         //weight, cost, item
		if (self.impulse == 4)
                  BuyWeapon(4, 32, 20);         //weight, cost, item
		if (self.impulse == 4)
                  BuyWeapon(7, 40, 21);         //weight, cost, item
		if (self.impulse == 4)
                  BuyWeapon(10, 45, 22);         //weight, cost, item

			return;
	}

	if (self.currentmenu == "shop_equipment")
	{
		if (self.impulse == 1)
                  BuyEquipment(20, 1);         //cost, item
		if (self.impulse == 2)
                  BuyEquipment(20, 2);         //cost, item
		if (self.impulse == 3)
                  BuyEquipment(20, 3);         //cost, item
		if (self.impulse == 4)
                  BuyEquipment(20, 4);         //cost, item
		if (self.impulse == 5)
                  BuyEquipment(20, 5);         //cost, item
		if (self.impulse == 6)
                  BuyEquipment(20, 6);         //cost, item
		if (self.impulse == 7)
                  BuyEquipment(20, 7);         //cost, item
		if (self.impulse == 8)
                  BuyEquipment(20, 8);         //cost, item
		if (self.impulse == 9)
                  BuyEquipment(20, 9);         //cost, item
		if (self.impulse == 10)
                  BuyEquipment(20, 10);         //cost, item

			return;
	}

	if (self.currentmenu == "shop_chems")
	{
		if (self.impulse == 1)
                  BuyChem(3, 1);         //cost, item
		if (self.impulse == 2)
                  BuyChem(5, 2);         //cost, item
		if (self.impulse == 3)
                  BuyChem(10, 3);         //cost, item
		if (self.impulse == 4)
                  BuyChem(12, 4);         //cost, item
		if (self.impulse == 5)
                  BuyChem(18, 5);         //cost, item
		if (self.impulse == 6)
                  BuyChem(21, 6);         //cost, item
	}

	if (self.currentmenu == "shop_other")
		return;


	if (self.currentmenu == "select_team")
	{
		if (self.impulse == 1)
		{
			bprint(2, self.netname);
			bprint(2, " has joined the rangers.\n");
			self.currentmenu = "confirm_team";
			DisplayMenu();
			self.team = 1;
			return;
		}
		if (self.impulse == 2)
		{
			bprint(2, self.netname);
			bprint(2, " has joined the raiders.\n");
			self.currentmenu = "confirm_team";
			DisplayMenu();
			self.team = 2;
			return;
		}

	}

	if (self.currentmenu == "select_skill")
	{
		sound (self, CHAN_WEAPON, "buttons/switch02.wav", TRUE, ATTN_NORM);

		if (self.impulse == 1)
		{
			self.currentmenu = "none";
			self.max_health = 80;
			self.class = 1;
			self.currentmenu = "confirm_skill";
			centerprint (self, "êyour class will beë\n\nMedic - OK?\nê1ë Yes     \nê2ë No      \n");
			self.ghost = 0;
			return;
		}
		if ((self.impulse == 2))
		{
			self.currentmenu = "none";
			self.max_health = 70;
			self.class = 2;
			self.currentmenu = "confirm_skill";
			centerprint (self, "êyour class will beë\n\nAssassin - OK?\nê1ë Yes     \nê2ë No      \n");
			self.ghost = 0;
			return;
		}
		if (self.impulse == 3)
		{
			self.currentmenu = "none";
			self.max_health = 100;
			self.class = 3;
			self.currentmenu = "confirm_skill";
			centerprint (self, "êyour class will beë\n\nSoldiier - OK?\nê1ë Yes     \nê2ë No      \n");
			self.ghost = 0;
			return;
		}
		if (self.impulse == 4)
		{
			self.max_health = 80;
			self.currentmenu = "none";
			self.class = 4;
			self.currentmenu = "confirm_skill";
			centerprint (self, "êyour class will beë\n\nScientist - OK?\nê1ë Yes     \nê2ë No      \n");
			self.ghost = 0;
			return;
		}
	}


		if (self.currentmenu == "confirm_team")
		{
			if (self.impulse == 1)
			{
				sound (self, CHAN_WEAPON, "buttons/switch02.wav", TRUE, ATTN_NORM);
				self.currentmenu = "select_skill";
				DisplayMenu();
				self.impulse = 0;
				return;
			}
			if (self.impulse == 2)
			{
				sound (self, CHAN_WEAPON, "buttons/switch02.wav", TRUE, ATTN_NORM);
				self.currentmenu = "select_team";
				DisplayMenu();
				self.impulse = 0;
				self.team = 0;
				return;
			}
		}
		if (self.currentmenu == "confirm_skill")
		{
			if (self.impulse == 1)
			{
				sound (self, CHAN_WEAPON, "buttons/switch02.wav", TRUE, ATTN_NORM);
				self.currentmenu = "none";
				PutClientInServer();
				self.impulse = 0;
				return;
			}
			if (self.impulse == 2)
			{
				sound (self, CHAN_WEAPON, "buttons/switch02.wav", TRUE, ATTN_NORM);
				self.currentmenu = "select_skill";
				DisplayMenu();
				self.impulse = 0;
				self.class = 0;
				return;
			}
		}

};