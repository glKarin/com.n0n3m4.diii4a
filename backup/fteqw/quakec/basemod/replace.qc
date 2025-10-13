
// Basemod replacement ent defines
#define ENT_REPLACE(f,t) void(void) f = { self.classname = #t; t(); }
#define ENT_REPLACE_FUNC(f,t,x) void(void) f = { self.classname = #t; x(); t(); }

// Replacement functions
void() health_setrotten = { self.spawnflags |= H_ROTTEN; };
void() health_setmega = { self.spawnflags |= H_MEGA; };
void() ammo_setlarge = { self.spawnflags |= WEAPON_BIG2; };

// -- Quake 3 entities
// weapons
ENT_REPLACE(weapon_gauntlet, weapon_supershotgun); // Gauntlet (not normally used)
ENT_REPLACE(weapon_machinegun, weapon_nailgun); // Machinegun (not normally used)
ENT_REPLACE(weapon_shotgun, weapon_supershotgun); // Shotgun
ENT_REPLACE(weapon_plasmagun, weapon_supernailgun); // Plasma gun
ENT_REPLACE(weapon_railgun, weapon_rocketlauncher); // Railgun
ENT_REPLACE(weapon_bfg, weapon_lightning); // BFG
// holdables
ENT_REPLACE(holdable_medikit, item_health); // personal medikit 
ENT_REPLACE(holdable_teleporter, info_null); // personal teleporter
// armor
ENT_REPLACE(item_armor_body, item_armorInv); // 100 armor
ENT_REPLACE(item_armor_combat, item_armor2); // 50 armor
ENT_REPLACE(item_armor_shard, info_null); // +5 armor shard
// powerups
ENT_REPLACE(item_enviro, item_artifact_envirosuit); // enviro powerup
ENT_REPLACE(item_flight, item_artifact_invisibility); // flight
ENT_REPLACE(item_haste, item_artifact_super_damage); // haste
ENT_REPLACE(item_quad, item_artifact_super_damage); // quad damage
ENT_REPLACE(item_invis, item_artifact_invisibility); // invisibility
ENT_REPLACE(item_regen, item_health); // regeneration
// health
ENT_REPLACE(item_health_large, item_health); // +50 health
ENT_REPLACE_FUNC(item_health_mega, item_health, health_setmega); // +100 health (ignore max)
ENT_REPLACE_FUNC(item_health_small, item_health, health_setrotten); // +5 health (ignore max)
// ammo
ENT_REPLACE(ammo_bullets, item_spikes); // Machinegun ammo
ENT_REPLACE(ammo_grenades, item_rockets); // Grenade launcher ammo
ENT_REPLACE(ammo_lightning, item_cells); // Lightning gun ammo
ENT_REPLACE_FUNC(ammo_slugs, item_rockets, ammo_setlarge); // Railgun ammo
ENT_REPLACE_FUNC(ammo_bfg, item_cells, ammo_setlarge); // BFG ammo
// other ents
ENT_REPLACE(misc_teleporter_dest, info_teleport_destination); // teleport destination
ENT_REPLACE(target_position, info_notnull); // "target" entity
ENT_REPLACE(info_player_intermission, info_intermission); // intermission

// -- Quake 2 entities
// weapons
ENT_REPLACE(weapon_chaingun, weapon_supernailgun); // chaingun
ENT_REPLACE(weapon_hyperblaster, weapon_lightning); // hyperblaster
// armor
ENT_REPLACE(item_armor_jacket, item_armor1);
ENT_REPLACE(item_power_screen, item_armor2); // power screen (not used often)
ENT_REPLACE(item_power_shield, item_armorInv); // power shield
 // powerups
ENT_REPLACE(item_bandolier, item_rockets, ammo_setlarge); // bandolier
ENT_REPLACE(item_breather, item_artifact_envirosuit); // underwater air breather
ENT_REPLACE(item_invulnerability, item_artifact_invulnerability); // invulnerable artifact
ENT_REPLACE(item_pack, item_rockets, ammo_setlarge); // ammo pack
ENT_REPLACE(item_silencer, item_artifact_invisibility); // weapon silencer
// health
ENT_REPLACE_FUNC(item_ancient_head, item_health, health_setmega); // ancient head (+2 max health)
ENT_REPLACE_FUNC(item_adrenaline, item_health, health_setmega); // adrenaline (+1 max health)

// -- Half-life entities
// weapons
ENT_REPLACE(weapon_357, weapon_grenadelauncher); // 357 magnum
ENT_REPLACE(weapon_9mmAR, weapon_supernailgun); // mp5
ENT_REPLACE(weapon_9mmhandgun, weapon_nailgun); // handgun
ENT_REPLACE(weapon_crossbow, weapon_grenadelauncher); // crossbow
ENT_REPLACE(weapon_crowbar, info_null); // crowbar
ENT_REPLACE(weapon_egon, weapon_lightning); // ghostbusters gun
ENT_REPLACE(weapon_gauss, weapon_lightning); // tau cannon
ENT_REPLACE(weapon_handgrenade, item_rockets); // hand grenades
ENT_REPLACE(weapon_hornetgun, info_null); // hornet gun
ENT_REPLACE(weapon_rpg, weapon_rocketlauncher); // rpg
ENT_REPLACE(weapon_satchel, item_rockets); // satchel charge
ENT_REPLACE(weapon_snark, item_cells); // snarks
ENT_REPLACE(weapon_tripmine, item_rockets); // tripmines
// armor
ENT_REPLACE(item_battery, info_null); // battery charge
// powerups
ENT_REPLACE(item_airtank, item_artifact_envirosuit); // air tank (not used often)
ENT_REPLACE(item_antidote, item_health); // antidote (not used often)
ENT_REPLACE(item_longjump, weapon_rocketlauncher); // long jump module
ENT_REPLACE(item_suit, item_artifact_envirosuit); // HEV suit
// health
ENT_REPLACE(item_healthkit, item_health); // floor health kit
// ammo
ENT_REPLACE(ammo_357, item_rockets); // 357 ammo
ENT_REPLACE(ammo_9mmAR, item_spikes); // mp5 clip
ENT_REPLACE(ammo_9mmbox, item_spikes, ammo_setlarge); // 9mm box
ENT_REPLACE(ammo_9mmclip, item_spikes); // 9mm clip
ENT_REPLACE(ammo_ARgrenades, item_rockets); // mp5 grenades
ENT_REPLACE(ammo_buckshot, item_shells); // shotgun shells
ENT_REPLACE(ammo_crossbow, item_rockets); // crossbow bolts
ENT_REPLACE(ammo_gaussclip, item_cells); // cells for gauss/egon
ENT_REPLACE(ammo_rpgclip, item_rockets, ammo_setlarge); // RPG ammo

// special replace
void(void() func) CheckSpawn =
{
	// these ents conflict with q1 field names
	switch (self.classname)
	{
	case "ammo_shells":
		self.classname = "item_shells";
		item_shells();
		break;
	case "ammo_nails":
		self.classname = "item_spikes";
		item_spikes();
		break;
	case "ammo_cells":
		self.classname = "item_cells";
		item_cells();
		break;
	default:
		if (func)
			func();
		else
		{
			dprint("Couldn't find spawn function ");
			dprint(self.classname);
			dprint("\n");
		}
	}
};