
void(entity to, float iid, float amount) AddNonStackable =
{
	local float slot;

	slot = SlotOfItem(to, iid);
	if (!slot)
		slot = FindSuitableEmptySlot(to, iid);
	if (slot)
		SetItemSlot(to, slot, SlotVal(iid, amount));
};

void(entity to, float iid, float amount) AddStackable =
{
	local float slot;

	slot = FindSuitableEmptySlot(to, iid);
	if (slot)
		SetItemSlot(to, slot, SlotVal(iid, amount + ToStatus(ItemInSlot(to, slot))));
}

void(float cost, float iid) BuyStackable =
{
	local string z;

	if (self.ammo_shells < cost)
	{
		self.currentmenu = "none";
		sprint(self, PRINT_HIGH, "not enough money.\n");
		return;
	}

	sound (self, CHAN_BODY, "misc/item1.wav", 1, ATTN_NORM);
	self.ammo_shells = self.ammo_shells - cost;

	sprint(self, PRINT_HIGH, "you bought a ");

	z = GetItemName(iid);
	sprint(self, PRINT_HIGH, z);
	sprint(self, PRINT_HIGH, "\n");

	//put grenade in inventory
	//finds existing grenades, otherwise, empty slot

	TryGiveStackable(self, iid, 1);
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
	local string chemname;
	local float max;
	local float alreadygot;

	if (self.ammo_shells < cost)
	{
		self.currentmenu = "none";
		sprint(self, PRINT_HIGH, "not enough money.\n");
		return;
	}
	if (type != IID_CHEM_STIMPACK && self.class != 1)
	{
		self.currentmenu = "none";
		sprint(self, PRINT_HIGH, "not a medic.\n");
		return;
	}
	if (self.equipment == 1)
		max = 4;
	else
		max = 2;

	if (type == IID_CHEM_MEDICALBAG)
		max = 50;

	alreadygot = TotalQuantity(self, type);
	if (alreadygot >= max)
	{
		self.currentmenu = "none";
		sprint(self, PRINT_HIGH, "you can't hold any more chems.\n");
		return;
	}

	sound (self, CHAN_BODY, "misc/item1.wav", 1, ATTN_NORM);
	self.ammo_shells = self.ammo_shells - cost;
	chemname = GetItemName(type);
	sprint(self, PRINT_HIGH, chemname);
	sprint(self, PRINT_HIGH, " purchased.\n");
	TryGiveStackable(self, type, 1);
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

void() SetWeaponModel;

void (float wt, float cost, float wid) BuyWeapon =
{
	local string itname;
	local float ammotype, ammocount;
	local float slotnum;
	local float curweap;

	if (self.ammo_shells < cost)
	{
		self.currentmenu = "none";
		sound (self, CHAN_BODY, "misc/menu3.wav", 1, ATTN_IDLE);
		sprint(self, PRINT_HIGH, "not enough money.\n");
		return;
	}

	sound (self, CHAN_BODY, "misc/item1.wav", 1, ATTN_NORM);
	self.ammo_shells = self.ammo_shells - cost;

	sprint(self, PRINT_HIGH, "you bought ");

	itname = GetItemName(wid);
	sprint(self, PRINT_HIGH, itname);
	sprint(self, PRINT_HIGH, ".\n");

	//put new weapon in current slot
	//put old weapon in empty slot (but not armor slot!)
      //otherwise, drop it.

	curweap = ItemInSlot(self, self.current_slot);
	slotnum = FindSuitableEmptySlot(self, ToIID(curweap));

	if (slotnum == 0)//no more room
		DropFromSlot (self.current_slot, 1, 0);
	else       //found a place to stick old weapon
		SetItemSlot(self, slotnum, curweap);

	ammocount = WeaponMagQuant(wid);//load weapon with full ammo (may be changed)
	SetItemSlot(self, self.current_slot, SlotVal(wid, ammocount));


	ammotype = WeaponAmmoType(wid);//load weapon with full ammo (may be changed)
	TryGiveStackable(self, ammotype, ammocount*3);

	SetWeaponModel();
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

/*
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
	if ((((lbs + wt) - GetItemsWeight(self.islot3)) > self.max_weight))
	{
		sprint (self, PRINT_HIGH, "you can't carry that much.\n");
		self.currentmenu = "none";
		return;
	}
	sound (self, CHAN_BODY, "misc/item2.wav", 1, ATTN_NORM);
	self.ammo_shells = (self.ammo_shells - cost);
//	self.armor_weight = wt;
	self.armor = item;
	x = GetArmorName();
	sprint (self, PRINT_HIGH, x);
	sprint (self, PRINT_HIGH, " purchased.\n");

	return;

};*/

void (float wt, float cost, float item) BuyArmor =
{
	local string z;
	local float x;

	if (self.ammo_shells < cost)
	{
		self.currentmenu = "none";
		sound (self, CHAN_BODY, "misc/menu3.wav", 1, ATTN_IDLE);
		sprint(self, PRINT_HIGH, "not enough money.\n");
		return;
	}

	sound (self, CHAN_BODY, "misc/item1.wav", 1, ATTN_NORM);
	self.ammo_shells = self.ammo_shells - cost;

	sprint(self, PRINT_HIGH, "you bought ");

	z = GetItemName(item);
	sprint(self, PRINT_HIGH, z);
	sprint(self, PRINT_HIGH, ".\n");

	//put new armor in armor slot
	//put old armor in inventory

	x = FindEmptySlot(self);

	if (x == 0)
		sprint(self, 2, "no more room. giving armor to merchant.\n");

	if (x != 0) //found a place to stick old armor
		SetItemSlot(self, x, SlotVal(self.islot3, 1));

		SetItemSlot(self, 3, SlotVal(item, 1));
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

/*
	if ((self.armor >= TE_LIGHTNING2))
	{
		sprint (self, PRINT_HIGH, "too many electronics in\nyour currently worn armor!");
		self.currentmenu = "none";
		return;
	}
*/
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

	tmp = input + self.weight;
	max = self.max_weight;
	if (self.class == TE_LIGHTNING2)
	{
		max = max + SPAWNFLAG_LASER;
	}
	if (self.perk == TE_LAVASPLASH)
	{
		return (FALSE);
	}
	if (tmp > max)
	{
		return (TRUE);
	}
	else
	{
		return (FALSE);
	}
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
                  BuyArmor(3, 03, IID_ARM_SHIRT);         //weight, cost, item
		if (self.impulse == 2)
                  BuyArmor(7, 05, IID_ARM_LEATHER);         //weight, cost, item
		if (self.impulse == 3)
                  BuyArmor(9, 10, IID_ARM_KEVLAR);         //weight, cost, item
		if (self.impulse == 4)
                  BuyArmor(15, 12, IID_ARM_METAL);         //weight, cost, item
		if (self.impulse == 5)
                  BuyArmor(12, 25, IID_ARM_COMBAT);         //weight, cost, item
		if (self.impulse == 6)
                  BuyArmor(17, 35, IID_ARM_BROTHERHOOD);   //weight, cost, item
		if (self.impulse == 7)
                  BuyArmor(5, 45, IID_ARM_FORCE);         //weight, cost, item
		if (self.impulse == 8)
                  BuyArmor(20, 55, IID_ARM_LPOWER);         //weight, cost, item

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
		if (self.impulse == 6)
			self.currentmenu = "shop_heavy";

		DisplayMenu();
		return;
	}
	if (self.currentmenu == "shop_melee")
	{
		if (self.impulse == 1)
                  BuyWeapon(1, 1, IID_WP_KNIFE);         //weight, cost, item
		if (self.impulse == 2)
                  BuyWeapon(6, 3, IID_WP_AXE);         //weight, cost, item
		if (self.impulse == 3)
                  BuyWeapon(3, 10, IID_WP_VIBROBLADE);         //weight, cost, item
		if (self.impulse == 4)
                  BuyWeapon(10, 15, IID_WP_POWERAXE);         //weight, cost, item

			return;
	}
	if (self.currentmenu == "shop_thrown")
	{
		if (self.impulse == 1)
                  BuyStackable(3, IID_GREN_SMOKE);         //weight, cost, item
		if (self.impulse == 2)
                  BuyStackable(4, IID_GREN_FRAG);         //weight, cost, item
		if (self.impulse == 3)
                  BuyStackable(5, IID_GREN_EMP);         //weight, cost, item
		if (self.impulse == 4)
                  BuyStackable(6, IID_GREN_FLASH);         //weight, cost, item

			return;
	}
	if (self.currentmenu == "shop_pistols")
	{
		if (self.impulse == 1)
                  BuyWeapon(1, 5, IID_WP_USP);         //weight, cost, item
		if (self.impulse == 2)
                  BuyWeapon(2, 7, IID_WP_DEAGLE);         //weight, cost, item
		if (self.impulse == 3)
                  BuyWeapon(2, 9, IID_WP_NEEDLER);         //weight, cost, item
		if (self.impulse == 4)
                  BuyWeapon(3, 14, IID_WP_MP9);         //weight, cost, item
		if (self.impulse == 5)
                  BuyWeapon(3, 17, IID_WP_MP7);         //weight, cost, item
		if (self.impulse == 6)
                  BuyWeapon(2, 21, IID_WP_ALIENBLASTER);         //weight, cost, item

			return;
	}
	if (self.currentmenu == "shop_shotguns")
	{
		if (self.impulse == 1)
                  BuyWeapon(3, 4, IID_WP_PIPERIFLE);         //weight, cost, item
		if (self.impulse == 2)
                  BuyWeapon(4, 8, IID_WP_WINCHESTER);         //weight, cost, item
		if (self.impulse == 3)
                  BuyWeapon(5, 12, IID_WP_MOSSBERG);         //weight, cost, item
		if (self.impulse == 4)
                  BuyWeapon(7, 34, IID_WP_JACKHAMMER);         //weight, cost, item

			return;
	}
	if (self.currentmenu == "shop_rifles")
	{
		if (self.impulse == 1)
                  BuyWeapon(3, 11, IID_WP_RANGEMASTER);    //weight, cost, item
		if (self.impulse == 2)
                  BuyWeapon(4, 21, IID_WP_AK112);          //weight, cost, item
		if (self.impulse == 3)
                  BuyWeapon(5, 25, IID_WP_AK74);           //weight, cost, item
		if (self.impulse == 4)
                  BuyWeapon(6, 28, IID_WP_DKS1);           //weight, cost, item
		if (self.impulse == 5)
                  BuyWeapon(5, 30, IID_WP_MOONLIGHT);      //weight, cost, item
		if (self.impulse == 6)
                  BuyWeapon(4, 32, IID_WP_SA80);           //weight, cost, item
		if (self.impulse == 7)
                  BuyWeapon(9, 25, IID_WP_FNFAL);          //weight, cost, item

			return;
	}
	if (self.currentmenu == "shop_heavy")
	{
		if (self.impulse == 1)
                  BuyWeapon(10, 30, IID_WP_ROCKETLAUNCHER);//weight, cost, item
		if (self.impulse == 2)
                  BuyWeapon(7, 32, IID_WP_GAUSERIFLE);     //weight, cost, item
		if (self.impulse == 3)
                  BuyWeapon(12, 45, IID_WP_PULSERIFLE);    //weight, cost, item
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
                  BuyChem(3, IID_CHEM_STIMPACK);         //cost, item
		if (self.impulse == 2)
                  BuyChem(5, IID_CHEM_MEDICALBAG);         //cost, item
		if (self.impulse == 3)
                  BuyChem(10, IID_CHEM_SUPERSTIM);         //cost, item
		if (self.impulse == 4)
                  BuyChem(12, IID_CHEM_ADRENALINE);         //cost, item
		if (self.impulse == 5)
                  BuyChem(18, IID_CHEM_PSYCHO);         //cost, item
		if (self.impulse == 6)
                  BuyChem(21, IID_CHEM_BESERK);         //cost, item
		return;
	}

	if (self.currentmenu == "shop_other")
	{
		if (self.impulse == 1)
			BuyChem(5, IID_CHEM_MEDICALBAG);
		if (self.impulse == 3)
			BuyStackable(20, IID_BUILD_MRAMMO);
		if (self.impulse == 4)
			BuyStackable(20, IID_BUILD_AUTODOC);
		if (self.impulse == 5)
			BuyStackable(20, IID_BUILD_SHIELDGEN);
		if (self.impulse == 6)
			BuyStackable(20, IID_BUILD_TTURRET);
		return;
	}


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
				centerprint(self, "");
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