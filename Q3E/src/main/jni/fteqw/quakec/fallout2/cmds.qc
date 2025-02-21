void(entity e, string s) clientcommand = #440
float(string s) tokenize = #441;
string(float n) argv = #442;

string(string s, float start, float length) substring = #116;
float(string str, string sub) strstrofs = #221;

float(string desc) itemtoslot =
{
	local float slot;
	local float coma;

	while((coma = strstrofs(desc, "+")) > 0)
	{
		slot = itemtoslot(substring(desc, 0, coma));
		if (slot)
			return slot;
		desc = substring(desc, coma+1, -1);
	}

	slot = stof(desc);
	if (slot >= 1 && slot <= MAXSLOTS)
		return slot;

	if (desc == "current")
		return self.current_slot;
	slot = ItemIDOfName(desc);
	if (!slot)
	{
		sprint(self, PRINT_HIGH, "Not an item name\n");
		return 0;
	}
	else
	{
		slot = SlotOfItem(self, slot);
		if (slot)
			return slot;
		return 0;
	}
};

float(float slotno) DecreaseDestroySlot =
{
	local float it;
	local float iid;
	local float num;
	it = ItemInSlot(self, slotno);
	if (ToStatus(it) <= 1)
	{
		SetItemSlot(self, slotno, 0);
		return true;
	}
	else
	{
		iid = ToIID(it);
		num = ToStatus(it);
		num = num - 1;
		it = SlotVal(iid, num);
		SetItemSlot(self, slotno, it);
		return false;
	}
};

void(string arg1) Cmd_InvUse =
{
	local float it, iid;
	local float slotno;
	slotno = itemtoslot(arg1);
	if (!slotno)
	{
		sprint(self, PRINT_MEDIUM, "Can't use - you don't have one\n");
		return;
	}


	it = ItemInSlot(self, slotno);
	iid = ToIID(it);

	if (iid == 0)
		return;

	if (WeaponAmmoType(iid))
	{
		//weapons are reloaded
		ReloadWeapon(slotno);
		return;
	}

	if (iid == IID_CHEM_STIMPACK ||
	    iid == IID_CHEM_MEDICALBAG ||
	    iid == IID_CHEM_SUPERSTIM)
	{
		if (UseHealingChem(iid))
			DecreaseDestroySlot(slotno);
		return;
	}

	if (iid == IID_CHEM_ADRENALINE ||
	    iid == IID_CHEM_PSYCHO ||
	    iid == IID_CHEM_BESERK)
	{
		if (UseBoostingChem(iid))
			DecreaseDestroySlot(slotno);
		return;
	}

	if (iid == IID_BUILD_MRAMMO ||
	    iid == IID_BUILD_SHIELDGEN ||
	    iid == IID_BUILD_AUTODOC ||
	    iid == IID_BUILD_ROBOFANG ||
	    iid == IID_BUILD_TTURRET || 
	    iid == IID_BUILD_RTURRET ||
	    iid == IID_BUILD_GTURRET)
	{
		if (spawn_station(iid))
			DecreaseDestroySlot(slotno);
		return;
	}

	if (iid == IID_EQUIP_CLIMBINGGEAR)
	{
		if (self.equipment_slot == slotno)
		{
			self.equipment_slot = 0;
			sprint(self, PRINT_HIGH, "Climbing gear deactivated\n");
		}
		else
		{
			self.equipment_slot = slotno;
			sprint(self, PRINT_HIGH, "Climbing gear activated\n");
		}
		return;
	}
	if (iid == IID_EQUIP_SPRINTKIT)
	{
		if (self.equipment_slot == slotno)
		{
			self.equipment_slot = 0;
			sprint(self, PRINT_HIGH, "What a downer\n");
		}
		else
		{
			self.equipment_slot = slotno;
			sprint(self, PRINT_HIGH, "SUGAR RUSH!!!\n");
		}
		return;
	}
	if (iid == IID_EQUIP_STEALTHBOY)
	{
		if (self.equipment_slot == slotno)
		{
			self.equipment_slot = 0;
			sprint(self, PRINT_HIGH, "stealthboy deactivated\n");
		}
		else
		{
			self.equipment_slot = slotno;
			sprint(self, PRINT_HIGH, "stealthboy activated, fading into the shadows\n");
		}
		return;
	}
	if (iid == IID_EQUIP_HOVERBOOTS)
	{
		if (self.equipment_slot == slotno)
		{
			self.equipment_slot = 0;
			sprint(self, PRINT_HIGH, "Hoverboots deactivated\n");
		}
		else
		{
			self.equipment_slot = slotno;
			sprint(self, PRINT_HIGH, "Hoverboots activated\n");
		}
		return;
	}

	sprint(self, PRINT_HIGH, "Don't know how to 'use' item\n");
};

void(string arg1) Cmd_InvDrop =
{
	local float it, iid;
	local float slotno;
	slotno = itemtoslot(arg1);
	if (!slotno)
	{
		sprint(self, PRINT_MEDIUM, "Can't drop - you don't have one\n");
		return;
	}


	it = ItemInSlot(self, slotno);
	iid = ToIID(it);

	if (iid == 0)
		return;

	DropFromSlot(slotno, true, false);
};

void(string arg1, string arg2) Cmd_InvSwap =
{
	local float old1, old2;
	local float slotno1;
	local float slotno2;
	slotno1 = itemtoslot(arg1);
	if (!slotno1)
	{
		sprint(self, PRINT_HIGH, "No item\n");
		return;
	}
	slotno2 = itemtoslot(arg2);
	if (!slotno2)
	{
		sprint(self, PRINT_HIGH, "No item\n");
		return;
	}

	old1 = ItemInSlot(self, slotno1);
	old2 = ItemInSlot(self, slotno2);
	if (old1 == old2)
		return;

	if (!FitsInSlot(slotno1, ToIID(old2)))
	{
		sprint(self, PRINT_HIGH, "Not allowed to exchange items\n");
		return;
	}
	if (!FitsInSlot(slotno2, ToIID(old1)))
	{
		sprint(self, PRINT_HIGH, "Not allowed to exchange items\n");
		return;
	}
	SetItemSlot(self, slotno1, old2);
	SetItemSlot(self, slotno2, old1);

	if (slotno1 == self.current_slot || slotno2 == self.current_slot)
		W_SetCurrentAmmo();

//swap the equipment_slot over too.
	if (self.equipment_slot == slotno1)
		self.equipment_slot = slotno2;
	else if (self.equipment_slot == slotno2)
		self.equipment_slot = slotno1;

	self.rtime = time + 2;	//we don't check this here though - cost to get into inventory.
};

void(string arg1, float iid, float num) Cmd_InvGive =
{
	local float slotno;
	slotno = itemtoslot(arg1);
	if (!slotno)
	{
		sprint(self, PRINT_MEDIUM, "Can't give to that slot\n");
		return;
	}

	if (!FitsInSlot(slotno, iid))
	{
		sprint(self, PRINT_MEDIUM, "Can't give that item in that slot\n");
		return;
	}

	if (num <= 0)
		num = 1;
	SetItemSlot(self, slotno, SlotVal(iid, num));

	if (slotno == self.current_slot)
		W_SetCurrentAmmo();
};

void(string line) SV_ParseClientCommand =
{
	local string cmd;
	tokenize(line);

	cmd = argv(0);
	if (cmd == "invuse")
	{
		if (self.deadflag || self.current_slot==0)
			return;
		Cmd_InvUse(argv(1));
	}
	else if (cmd == "invdrop")
	{
		if (self.deadflag || self.current_slot==0)
			return;
		Cmd_InvDrop(argv(1));
	}
	else if (cmd == "invswap")
	{
		if (self.deadflag || self.current_slot==0)
			return;
		Cmd_InvSwap(argv(1), argv(2));
	}
	else if (cmd == "invgive")
	{
		if (self.deadflag || self.current_slot==0)
			return;
		Cmd_InvGive(argv(1), stof(argv(2)), stof(argv(3)));
	}
	else if (cmd == "god")
	{
		sprint(self, PRINT_HIGH, "sorry, but I'm an athiest\n");
	}
	else
		clientcommand(self, line);
};