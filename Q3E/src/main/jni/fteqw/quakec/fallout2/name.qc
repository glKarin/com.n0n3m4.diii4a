



string () GetEquipmentName =
{
	local string name;
	local float slot;

	slot = self.equipment;

	if (slot == 0)
		name = "none";
	else if (slot == 1)
		name = "medic's bag";
	else if (slot == 2)
		name = "security alarm";
	else if (slot == 3)
		name = "remote camera";
	else if (slot == 4)
		name = "belt pouch";
	else if (slot == 5)
		name = "backpack";
	else if (slot == 6)
		name = "toolkit mark ii";
	else if (slot == 7)
		name = "climbing gear";
	else if (slot == 8)
		name = "enhanced battery";
	else if (slot == 9)
		name = "stealth-boy";

	return name;
};

string () GetProtectName =
{
	local string name;
	local float slot;

	slot = self.protect;

	if (slot == 0)
		name = "none";
	if (slot == 1)
		name = "";
	else if (slot == 2)
		name = "sacred talisman";
	else if (slot == 3)
		name = "forcefield ring";
	else if (slot == 4)
		name = "";
	else if (slot == 5)
		name = "";

	return name;
};

string () GetPerkName =
{
	local string name;
	local float slot;

	slot = self.perk;

	if (slot == 0)
		name = "none";
	if (slot == 1)
		name = "medic's bag";
	else if (slot == 2)
		name = "security alarm";
	else if (slot == 3)
		name = "remote camera";
	else if (slot == 4)
		name = "belt pouch";
	else if (slot == 5)
		name = "backpack";
	else if (slot == 6)
		name = "toolkit mark ii";
	else if (slot == 7)
		name = "climbing gear";
	else if (slot == 8)
		name = "enhanced battery";
	else if (slot == 9)
		name = "stealth-boy";
	return name;
};

string () GetTraitName =
{
	local string name;
	local float slot;

	slot = self.trait;

	if (slot == 0)
		name = "none";
	else if (slot == 1)
		name = "medic's bag";
	else if (slot == 2)
		name = "security alarm";
	else if (slot == 3)
		name = "remote camera";
	else if (slot == 4)
		name = "belt pouch";
	else if (slot == 5)
		name = "backpack";
	else if (slot == 6)
		name = "toolkit mark ii";
	else if (slot == 7)
		name = "climbing gear";
	else if (slot == 8)
		name = "enhanced battery";
	else if (slot == 9)
		name = "stealth-boy";

	return name;
};