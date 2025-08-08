/*
===========================================================================

Return to Castle Wolfenstein single player GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Return to Castle Wolfenstein single player GPL Source Code (RTCW SP Source Code).  

RTCW SP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW SP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW SP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW SP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW SP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

// g_survival_buy.c

#include "g_local.h"
#include "g_survival.h"

int Survival_GetDefaultWeaponPrice(int weapon) {
	switch (weapon) {
		case WP_KNIFE:        return svParams.knifePrice;
		// Pistols
		case WP_LUGER:        return svParams.lugerPrice;
		case WP_SILENCER:     return svParams.silencerPrice;
		case WP_COLT:         return svParams.coltPrice;
		case WP_TT33:         return svParams.tt33Price;
		case WP_REVOLVER:     return svParams.revolverPrice;
		case WP_DUAL_TT33:    return svParams.dualtt33Price;
		case WP_AKIMBO:       return svParams.akimboPrice;
		case WP_HDM:          return svParams.hdmPrice;

		// SMGs
		case WP_STEN:         return svParams.stenPrice;
		case WP_MP40:         return svParams.mp40Price;
		case WP_MP34:         return svParams.mp34Price;
		case WP_THOMPSON:     return svParams.thompsonPrice;
		case WP_PPSH:         return svParams.ppshPrice;

		// Rifles
		case WP_MAUSER:       return svParams.mauserPrice;
		case WP_MOSIN:        return svParams.mosinPrice;
		case WP_DELISLE:      return svParams.delislePrice;
		case WP_SNIPERRIFLE:  return svParams.sniperriflePrice;
		case WP_SNOOPERSCOPE: return svParams.snooperScopePrice;

		// Auto Rifles
		case WP_M1GARAND:     return svParams.m1garandPrice;
		case WP_G43:          return svParams.g43Price;
		case WP_M1941:        return svParams.m1941Price;

		// Assault Rifles
		case WP_MP44:         return svParams.mp44Price;
		case WP_FG42:         return svParams.fg42Price;
		case WP_BAR:          return svParams.barPrice;

		// Shotguns
		case WP_M97:          return svParams.shotgunPrice;
		case WP_AUTO5:        return svParams.auto5Price;

		// Heavy
		case WP_MG42M:        return svParams.mg42mPrice;
		case WP_PANZERFAUST:  return svParams.panzerPrice;
		case WP_BROWNING:     return svParams.browningPrice;
		case WP_FLAMETHROWER: return svParams.flamerPrice;
		case WP_VENOM:        return svParams.venomPrice;
		case WP_TESLA:        return svParams.teslaPrice;

		// Grenades
		case WP_GRENADE_LAUNCHER:   return svParams.grenPrice;
		case WP_GRENADE_PINEAPPLE:  return svParams.pineapplePrice;

		default: return svParams.defaultWeaponPrice;
	}
}

/*
============
Survival_HandleRandomWeaponBox
============
*/
qboolean Survival_HandleRandomWeaponBox(gentity_t *ent, gentity_t *activator, char *itemName, int *itemIndex) {
	if (!activator || !activator->client) return qfalse;

	static const weapon_t random_box_weapons[] = {
		WP_LUGER, WP_SILENCER, WP_COLT, WP_TT33, WP_REVOLVER, WP_DUAL_TT33,
		WP_AKIMBO, WP_HDM, WP_MP40, WP_THOMPSON, WP_STEN, WP_PPSH, WP_MP34,
		WP_MAUSER, WP_SNIPERRIFLE, WP_SNOOPERSCOPE, WP_MOSIN,
		WP_M1GARAND, WP_G43, WP_MP44, WP_FG42, WP_BAR, WP_M97,
		WP_BROWNING, WP_MG42M, WP_PANZERFAUST, WP_FLAMETHROWER, WP_VENOM, WP_TESLA
	};

	static const weapon_t random_box_weapons_dlc[] = {
		WP_LUGER, WP_SILENCER, WP_COLT, WP_TT33, WP_REVOLVER, WP_DUAL_TT33,
		WP_AKIMBO, WP_HDM, WP_MP40, WP_THOMPSON, WP_STEN, WP_PPSH, WP_MP34,
		WP_MAUSER, WP_SNIPERRIFLE, WP_SNOOPERSCOPE, WP_MOSIN,
		WP_M1GARAND, WP_G43, WP_M1941, WP_MP44, WP_FG42, WP_BAR, WP_M97, WP_AUTO5,
		WP_BROWNING, WP_MG42M, WP_PANZERFAUST, WP_FLAMETHROWER, WP_VENOM, WP_TESLA, WP_DELISLE
	};

	const weapon_t *selected_weapons = g_dlc1.integer ? random_box_weapons_dlc : random_box_weapons;
	int numWeapons = g_dlc1.integer 
		? sizeof(random_box_weapons_dlc) / sizeof(random_box_weapons_dlc[0]) 
		: sizeof(random_box_weapons) / sizeof(random_box_weapons[0]);

	int price = ent->price > 0 ? ent->price : svParams.randomWeaponPrice;

	if (activator->client->ps.persistant[PERS_SCORE] < price) {
		trap_SendServerCommand(-1, "mu_play sound/items/use_nothing.wav 0\n");
		return qfalse;
	}

	// Pick a random weapon the player doesn't have
	weapon_t chosen;
	int tries = 10;
	do {
		chosen = selected_weapons[rand() % numWeapons];
		tries--;
	} while (G_FindWeaponSlot(activator, chosen) >= 0 && tries > 0);

	if (tries <= 0) {
		trap_SendServerCommand(-1, "mu_play sound/items/use_nothing.wav 0\n");
		return qfalse;
	}

	// Find the item
	for (int i = 1; bg_itemlist[i].classname; i++)
	{
		if (bg_itemlist[i].giWeapon != chosen)
			continue;

		*itemIndex = i;
		itemName = bg_itemlist[i].classname;
		gitem_t *item = &bg_itemlist[i];

		// Give weapon
		Give_Weapon_New_Inventory(activator, chosen, qfalse);

		// Give full ammo (twice to fill both reserve and clip)
		int maxAmmo = BG_GetMaxAmmo(&activator->client->ps, chosen, svParams.ltAmmoBonus);
		Add_Ammo(activator, chosen, maxAmmo, qtrue);  // fill clip
		Add_Ammo(activator, chosen, maxAmmo, qfalse); // top off reserve

		// Also refill base pistol ammo if akimbo weapon
		if (chosen == WP_AKIMBO)
		{
			int coltMax = BG_GetMaxAmmo(&activator->client->ps, WP_COLT, svParams.ltAmmoBonus);
			Add_Ammo(activator, WP_COLT, coltMax, qtrue);
			Add_Ammo(activator, WP_COLT, coltMax, qfalse);
		}
		else if (chosen == WP_DUAL_TT33)
		{
			int tt33Max = BG_GetMaxAmmo(&activator->client->ps, WP_TT33, svParams.ltAmmoBonus);
			Add_Ammo(activator, WP_TT33, tt33Max, qtrue);
			Add_Ammo(activator, WP_TT33, tt33Max, qfalse);
		}

		// Bonus: give M7 for Garand
		if (chosen == WP_M1GARAND)
		{
			Give_Weapon_New_Inventory(activator, WP_M7, qfalse);
			int m7MaxAmmo = BG_GetMaxAmmo(&activator->client->ps, WP_M7, svParams.ltAmmoBonus);
			Add_Ammo(activator, WP_M7, m7MaxAmmo, qfalse);
		}

		// Select weapon
		activator->client->ps.weapon = chosen;
		activator->client->ps.weaponstate = WEAPON_READY;

		// Deduct points
		activator->client->ps.persistant[PERS_SCORE] -= price;

		// SFX & confirmation
		G_AddPredictableEvent(activator, EV_ITEM_PICKUP, item - bg_itemlist);
		trap_SendServerCommand(-1, "mu_play sound/misc/buy.wav 0\n");

		return qtrue;
	}

	return qfalse;
}

/*
============
Survival_HandleRandomPerkBox
============
*/
qboolean Survival_HandleRandomPerkBox(gentity_t *ent, gentity_t *activator, char **itemName, int *itemIndex) {
	if (!activator || !activator->client) return qfalse;

	static char *random_perks[] = {
		"perk_resilience", "perk_scavenger", "perk_runner",
		"perk_weaponhandling", "perk_rifling", "perk_secondchance"
	};

	int price = (ent->price > 0) ? ent->price : svParams.randomPerkPrice;
	const int numPerks = sizeof(random_perks) / sizeof(random_perks[0]);

	// Perk count limit
	int perkCount = 0;
	for (int i = 0; i < MAX_PERKS; i++) {
		if (activator->client->ps.perks[i] > 0)
			perkCount++;
	}
	int maxPerks = (activator->client->ps.stats[STAT_PLAYER_CLASS] == PC_ENGINEER) ? svParams.maxPerksEng : svParams.maxPerks;
	if (perkCount >= maxPerks) {
		G_AddEvent(activator, EV_GENERAL_SOUND, G_SoundIndex("sound/items/use_nothing.wav"));
		return qfalse;
	}

	int randomIndex = rand() % numPerks;
	*itemName = random_perks[randomIndex];

	for (int i = 1; bg_itemlist[i].classname; i++) {
		if (!Q_strcasecmp(*itemName, bg_itemlist[i].classname)) {
			*itemIndex = i;
			gitem_t *perkItem = &bg_itemlist[i];

			if (activator->client->ps.perks[perkItem->giTag] > 0 || 
				activator->client->ps.persistant[PERS_SCORE] < price) {
				G_AddEvent(activator, EV_GENERAL_SOUND, G_SoundIndex("sound/items/use_nothing.wav"));
				return qfalse;
			}

			activator->client->ps.perks[perkItem->giTag]++;
			activator->client->ps.stats[STAT_PERK] |= (1 << perkItem->giTag);
			activator->client->ps.persistant[PERS_SCORE] -= price;

			G_AddPredictableEvent(activator, EV_ITEM_PICKUP, perkItem - bg_itemlist);
			trap_SendServerCommand(-1, "mu_play sound/misc/buy_perk.wav 0\n");
			return qtrue;
		}
	}

	return qfalse;
}

/*
============
Survival_HandleAmmoPurchase
============
*/
qboolean Survival_HandleAmmoPurchase(gentity_t *ent, gentity_t *activator, int price) {
	if (!activator || !activator->client)
		return qfalse;

	int heldWeap = activator->client->ps.weapon;
	if (heldWeap <= WP_NONE || heldWeap >= WP_NUM_WEAPONS)
		return qfalse;

	// Skip utility weapons
	if (heldWeap == WP_DYNAMITE_ENG || heldWeap == WP_AIRSTRIKE || heldWeap == WP_POISONGAS_MEDIC)
		return qfalse;

	int ammoIndex = BG_FindAmmoForWeapon(heldWeap);
	if (ammoIndex < 0)
		return qfalse;

	// Use upgrade-aware max ammo
	int maxAmmo = BG_GetMaxAmmo(&activator->client->ps, heldWeap, svParams.ltAmmoBonus);

	// Check if already full
	if (activator->client->ps.ammo[ammoIndex] >= maxAmmo)
		return qfalse;

	// Base price fallback: half weapon price
	int basePrice = Survival_GetDefaultWeaponPrice(heldWeap);
	int ammoPrice = basePrice / 2;

	// Upgrade modifier
	if (price <= 0 && activator->client->ps.weaponUpgraded[heldWeap]) {
		ammoPrice = svParams.upgradedAmmoPrice;
	}

	// Mapper override
	if (price > 0) {
		ammoPrice = price;
	}

	// Check score
	if (activator->client->ps.persistant[PERS_SCORE] < ammoPrice) {
		trap_SendServerCommand(-1, "mu_play sound/items/use_nothing.wav 0\n");
		return qfalse;
	}

	// Refill ammo and clip using upgrade-aware cap
	Add_Ammo(activator, heldWeap, maxAmmo, qtrue);
	Add_Ammo(activator, heldWeap, maxAmmo, qfalse);

	// Also refill ammo for base pistol if akimbo
	if (heldWeap== WP_AKIMBO)
	{
		Add_Ammo(activator, WP_COLT, BG_GetMaxAmmo(&activator->client->ps, WP_COLT, svParams.ltAmmoBonus), qtrue);
		Add_Ammo(activator, WP_COLT, BG_GetMaxAmmo(&activator->client->ps, WP_COLT, svParams.ltAmmoBonus), qfalse);
	}
	else if (heldWeap == WP_DUAL_TT33)
	{
		Add_Ammo(activator, WP_TT33, BG_GetMaxAmmo(&activator->client->ps, WP_TT33, svParams.ltAmmoBonus), qtrue);
		Add_Ammo(activator, WP_TT33, BG_GetMaxAmmo(&activator->client->ps, WP_TT33, svParams.ltAmmoBonus), qfalse);
	}


	// Deduct score
	activator->client->ps.persistant[PERS_SCORE] -= ammoPrice;

	trap_SendServerCommand(-1, "mu_play sound/misc/buy.wav 0\n");
	return qtrue;
}

/*
============
Survival_HandleWeaponUpgrade
============
*/
qboolean Survival_HandleWeaponUpgrade(gentity_t *ent, gentity_t *activator, int price)
{
	playerState_t *ps = &activator->client->ps;
	int weap = ps->weapon;

	if (weap <= WP_NONE || weap >= WP_NUM_WEAPONS)
		return qfalse;

	// Weapons that cannot be upgraded
	if (weap == WP_KNIFE || weap == WP_SNIPERRIFLE || weap == WP_M1941SCOPE || weap == WP_FG42SCOPE || weap== WP_SNOOPERSCOPE || weap == WP_DELISLESCOPE || weap == WP_DYNAMITE || weap == WP_M7 || weap == WP_AIRSTRIKE || weap == WP_POISONGAS_MEDIC || weap == WP_DYNAMITE_ENG || weap == WP_GRENADE_LAUNCHER || weap == WP_GRENADE_PINEAPPLE) 
	{
		G_AddEvent(activator, EV_GENERAL_SOUND, G_SoundIndex("sound/items/use_nothing.wav"));
		return qfalse;
	}

	// Only allow one upgrade per weapon
	if (ps->weaponUpgraded[weap])
	{
		G_AddEvent(activator, EV_GENERAL_SOUND, G_SoundIndex("sound/items/use_nothing.wav"));
		return qfalse;
	}

	// Use fallback price
	int upgradePrice = svParams.weaponUpgradePrice;

	// Mapper override
	if (price > 0)
	{
		upgradePrice = price;
	}

	// FIXED: check actual value being subtracted
	if (activator->client->ps.persistant[PERS_SCORE] < upgradePrice)
	{
		G_AddEvent(activator, EV_GENERAL_SOUND, G_SoundIndex("sound/items/use_nothing.wav"));
		return qfalse;
	}

	ps->weaponUpgraded[weap] = 1;

	// If main weapon is upgraded upgrade alt too
	if (weap == WP_M1GARAND)
		ps->weaponUpgraded[WP_M7] = 1;

	if (weap == WP_MAUSER)
		ps->weaponUpgraded[WP_SNIPERRIFLE] = 1;

	if (weap == WP_DELISLE)
		ps->weaponUpgraded[WP_DELISLESCOPE] = 1;

    if (weap == WP_GARAND)
		ps->weaponUpgraded[WP_SNOOPERSCOPE] = 1;
	
	if (weap == WP_FG42)
		ps->weaponUpgraded[WP_FG42SCOPE] = 1;

	if (weap == WP_M1941)
		ps->weaponUpgraded[WP_M1941SCOPE] = 1;

    // Handle akimbo dual weapon logic
	if (weap == WP_AKIMBO)
		ps->weaponUpgraded[WP_COLT] = 1;
	else if (weap == WP_DUAL_TT33)
		ps->weaponUpgraded[WP_TT33] = 1;
	else if (weap == WP_COLT && ps->weaponUpgraded[WP_AKIMBO])
		ps->weaponUpgraded[WP_COLT] = 1;
	else if (weap == WP_TT33 && ps->weaponUpgraded[WP_DUAL_TT33])
		ps->weaponUpgraded[WP_TT33] = 1;

	activator->client->ps.persistant[PERS_SCORE] -= upgradePrice;

	// Refill ammo
	Add_Ammo(activator, weap, BG_GetMaxAmmo(&activator->client->ps, weap, svParams.ltAmmoBonus), qtrue);
	Add_Ammo(activator, weap, BG_GetMaxAmmo(&activator->client->ps, weap, svParams.ltAmmoBonus), qfalse);

	// Refill ammo for upgraded weapon
	Add_Ammo(activator, weap, BG_GetMaxAmmo(&activator->client->ps, weap, svParams.ltAmmoBonus), qtrue);
	Add_Ammo(activator, weap, BG_GetMaxAmmo(&activator->client->ps, weap, svParams.ltAmmoBonus), qfalse);

	if (weap == WP_M1GARAND)
	{
		Add_Ammo(activator, WP_M7, BG_GetMaxAmmo(&activator->client->ps, WP_M7, svParams.ltAmmoBonus), qtrue);
		Add_Ammo(activator, WP_M7, BG_GetMaxAmmo(&activator->client->ps, WP_M7, svParams.ltAmmoBonus), qfalse);
	}

	// Also refill ammo for base pistol if upgrading akimbo
	if (weap == WP_AKIMBO)
	{
		Add_Ammo(activator, WP_COLT, BG_GetMaxAmmo(&activator->client->ps, WP_COLT, svParams.ltAmmoBonus), qtrue);
		Add_Ammo(activator, WP_COLT, BG_GetMaxAmmo(&activator->client->ps, WP_COLT, svParams.ltAmmoBonus), qfalse);
	}
	else if (weap == WP_DUAL_TT33)
	{
		Add_Ammo(activator, WP_TT33, BG_GetMaxAmmo(&activator->client->ps, WP_TT33, svParams.ltAmmoBonus), qtrue);
		Add_Ammo(activator, WP_TT33, BG_GetMaxAmmo(&activator->client->ps, WP_TT33, svParams.ltAmmoBonus), qfalse);
	}

	trap_SendServerCommand(-1, "mu_play sound/misc/wpn_upgrade.wav 0\n");
	return qtrue;
}

/*
============
Survival_HandleWeaponOrGrenade
============
*/
qboolean Survival_HandleWeaponOrGrenade(gentity_t *ent, gentity_t *activator, gitem_t *item, int price) {
	if (!activator || !item) return qfalse;

	const int weapon = item->giTag;
	const int ammoIndex = BG_FindAmmoForWeapon(weapon);

	if (weapon <= WP_NONE || weapon >= WP_NUM_WEAPONS || ammoIndex < 0)
		return qfalse;

	// Use fallback price if undefined
	if (price <= 0) {
		price = Survival_GetDefaultWeaponPrice(weapon);
	}

	// Special handling: grenades (no new weapon granted)
	if (item->giType == IT_AMMO && (
		weapon == WP_GRENADE_LAUNCHER ||
		weapon == WP_GRENADE_PINEAPPLE ||
		weapon == WP_M7
	)) {
		int maxAmmo = BG_GetMaxAmmo(&activator->client->ps, weapon, svParams.ltAmmoBonus);

		if (activator->client->ps.ammoclip[weapon] >= maxAmmo) {
			return qfalse; // Already full
		}

		if (COM_BitCheck(activator->client->ps.weapons, weapon)) {
			price /= 2; // Discount if already owned
		}

		if (activator->client->ps.persistant[PERS_SCORE] < price) {
			G_AddEvent(activator, EV_GENERAL_SOUND, G_SoundIndex("sound/items/use_nothing.wav"));
			return qfalse;
		}

		activator->client->ps.persistant[PERS_SCORE] -= price;
		Add_Ammo(activator, weapon, maxAmmo, qtrue);
		Add_Ammo(activator, weapon, maxAmmo, qfalse);

		G_AddPredictableEvent(activator, EV_ITEM_PICKUP, item - bg_itemlist);
		trap_SendServerCommand(-1, "mu_play sound/misc/buy.wav 0\n");

		return qtrue;
	}

	// Already owns weapon — refill ammo only
	if (COM_BitCheck(activator->client->ps.weapons, weapon)) {
		// Adjust refill price
		if (activator->client->ps.weaponUpgraded[weapon])
		{
			price = svParams.upgradedAmmoPrice;
		}
		else
		{
			price /= 2;
		}

		if (activator->client->ps.persistant[PERS_SCORE] < price) {
			G_AddEvent(activator, EV_GENERAL_SOUND, G_SoundIndex("sound/items/use_nothing.wav"));
			return qfalse;
		}

		int maxAmmo = BG_GetMaxAmmo(&activator->client->ps, weapon, svParams.ltAmmoBonus);
		if (activator->client->ps.ammo[weapon] >= maxAmmo) {
			G_AddEvent(activator, EV_GENERAL_SOUND, G_SoundIndex("sound/items/use_nothing.wav"));
			return qfalse; // Already full
		}

		activator->client->ps.persistant[PERS_SCORE] -= price;

	    // Also refill ammo for base pistol if akimbo
		if (weapon == WP_AKIMBO)
		{
			Add_Ammo(activator, WP_COLT, BG_GetMaxAmmo(&activator->client->ps, WP_COLT, svParams.ltAmmoBonus), qtrue);
			Add_Ammo(activator, WP_COLT, BG_GetMaxAmmo(&activator->client->ps, WP_COLT, svParams.ltAmmoBonus), qfalse);
		}
		else if (weapon == WP_DUAL_TT33)
		{
			Add_Ammo(activator, WP_TT33, BG_GetMaxAmmo(&activator->client->ps, WP_TT33, svParams.ltAmmoBonus), qtrue);
			Add_Ammo(activator, WP_TT33, BG_GetMaxAmmo(&activator->client->ps, WP_TT33, svParams.ltAmmoBonus), qfalse);
		}

		Add_Ammo(activator, weapon, maxAmmo, qtrue);
		Add_Ammo(activator, weapon, maxAmmo, qfalse);

		G_AddPredictableEvent(activator, EV_ITEM_PICKUP, item - bg_itemlist);
		trap_SendServerCommand(-1, "mu_play sound/misc/buy.wav 0\n");

		return qtrue;
	}

	// Buying a new weapon
	if (activator->client->ps.persistant[PERS_SCORE] < price) {
		G_AddEvent(activator, EV_GENERAL_SOUND, G_SoundIndex("sound/items/use_nothing.wav"));
		return qfalse;
	}

	activator->client->ps.persistant[PERS_SCORE] -= price;

	Give_Weapon_New_Inventory(activator, weapon, qfalse);

	int maxAmmo = BG_GetMaxAmmo(&activator->client->ps, weapon, svParams.ltAmmoBonus);
	Add_Ammo(activator, weapon, maxAmmo, qtrue);
	Add_Ammo(activator, weapon, maxAmmo, qfalse);

	// Bonus: give M7 launcher with Garand
	if (weapon == WP_M1GARAND) {
		Give_Weapon_New_Inventory(activator, WP_M7, qfalse);
		int m7MaxAmmo = BG_GetMaxAmmo(&activator->client->ps, WP_M7, svParams.ltAmmoBonus);
		Add_Ammo(activator, WP_M7, m7MaxAmmo, qfalse);
	}

	G_AddPredictableEvent(activator, EV_ITEM_PICKUP, item - bg_itemlist);
	trap_SendServerCommand(-1, "mu_play sound/misc/buy.wav 0\n");

	return qtrue;
}

/*
============
Survival_HandleArmorPurchase
============
*/
qboolean Survival_HandleArmorPurchase(gentity_t *activator, gitem_t *item, int price) {
	if (!activator || !item || !activator->client) return qfalse;

	if (activator->client->ps.stats[STAT_ARMOR] >= 200)
		return qfalse;

	// Fallback price if not set by mapper
	if (price <= 0)
		price = svParams.armorPrice;

	// Check score
	if (activator->client->ps.persistant[PERS_SCORE] < price) {
		trap_SendServerCommand(-1, "mu_play sound/items/use_nothing.wav 0\n");
		return qfalse;
	}

	// Deduct, apply, and notify
	activator->client->ps.persistant[PERS_SCORE] -= price;
	activator->client->ps.stats[STAT_ARMOR] = 200;

	G_AddPredictableEvent(activator, EV_ITEM_PICKUP, item - bg_itemlist);
	trap_SendServerCommand(-1, "mu_play sound/misc/buy.wav 0\n");

	return qtrue;
}

/*
============
Survival_GetDefaultPerkPrice
============
*/
int Survival_GetDefaultPerkPrice(int perk) {
	switch (perk) {
		case PERK_SECONDCHANCE:    return svParams.secondchancePrice;
		case PERK_RUNNER:          return svParams.runnerPrice;
		case PERK_SCAVENGER:       return svParams.scavengerPrice;
		case PERK_WEAPONHANDLING:  return svParams.fasthandsPrice;
		case PERK_RIFLING:         return svParams.doubleshotPrice;
		case PERK_RESILIENCE:      return svParams.resiliencePrice;
		default:                   return svParams.defaultPerkPrice;
	}
}


/*
============
Survival_HandlePerkPurchase
============
*/
qboolean Survival_HandlePerkPurchase(gentity_t *activator, gitem_t *item, int price) {
	if (!activator || !item || item->giType != IT_PERK)
		return qfalse;

	// Count how many perks player has
	int perkCount = 0;
	for (int i = 0; i < MAX_PERKS; i++) {
		if (activator->client->ps.perks[i] > 0)
			perkCount++;
	}

	// Max perks check
	int maxPerks = (activator->client->ps.stats[STAT_PLAYER_CLASS] == PC_ENGINEER) ?  svParams.maxPerksEng : svParams.maxPerks;
	if (perkCount >= maxPerks)
		return qfalse;

	// Already owns this perk?
	if (activator->client->ps.perks[item->giTag] > 0)
		return qfalse;

	// Fallback to default price if mapper didn't define it
	if (price <= 0) {
		price = Survival_GetDefaultPerkPrice(item->giTag);
	}

	// Not enough score?
	if (activator->client->ps.persistant[PERS_SCORE] < price) {
		G_AddEvent(activator, EV_GENERAL_SOUND, G_SoundIndex("sound/items/use_nothing.wav"));
		return qfalse;
	}

	// Grant perk
	activator->client->ps.perks[item->giTag]++;
	activator->client->ps.stats[STAT_PERK] |= (1 << item->giTag);
	activator->client->ps.persistant[PERS_SCORE] -= price;

	G_AddPredictableEvent(activator, EV_ITEM_PICKUP, item - bg_itemlist);
	trap_SendServerCommand(-1, "mu_play sound/misc/buy_perk.wav 0\n");

	return qtrue;
}


/*QUAKED target_buy (1 0 0) (-8 -8 -8) (8 8 8)
Gives the activator all the items pointed to.
*/
void Use_Target_buy(gentity_t *ent, gentity_t *other, gentity_t *activator) {
	if (!activator || !activator->client || !ent->buy_item) return;

	int itemIndex = 0;
	char *itemName = ent->buy_item;
	int price = (ent->price > 0) ? ent->price : 0;
	int clientNum = activator->client->ps.clientNum;
	gitem_t *item = NULL;
	qboolean success = qfalse;

	// Special case: ammo
	if (!Q_stricmp(itemName, "ammo")) {
		if (Survival_HandleAmmoPurchase(ent, activator, price)) {
			activator->client->ps.persistant[PERS_SCORE] -= price;
			activator->client->hasPurchased = qtrue;
			ClientUserinfoChanged(clientNum);
		}
		return;
	}

	// Special case: random weapon
	if (!Q_stricmp(itemName, "random_weapon")) {
		success = Survival_HandleRandomWeaponBox(ent, activator, itemName, &itemIndex);
		if (success) {
			activator->client->hasPurchased = qtrue;
			ClientUserinfoChanged(clientNum);
		}
		return; // Don't flow into generic weapon handling
	}

	// Special case: upgrade weapon
	if (!Q_stricmp(itemName, "upgrade_weapon"))
	{
		if (Survival_HandleWeaponUpgrade(ent, activator, price))
		{
			activator->client->hasPurchased = qtrue;
			ClientUserinfoChanged(clientNum);
		}
		return;
	}

	// Special case: random perk
	if (!Q_stricmp(itemName, "random_perk")) {
		success = Survival_HandleRandomPerkBox(ent, activator, &itemName, &itemIndex);
		if (success) {
			activator->client->hasPurchased = qtrue;
			ClientUserinfoChanged(clientNum);
		}
		return;
	}

	// Fallback: find item by name
	if (itemIndex <= 0) {
		for (int i = 1; bg_itemlist[i].classname; i++) {
			if (!Q_strcasecmp(itemName, bg_itemlist[i].classname)) {
				itemIndex = i;
				break;
			}
		}
	}

	if (itemIndex <= 0) return;
	item = &bg_itemlist[itemIndex];

	// Not enough points?
	if (activator->client->ps.persistant[PERS_SCORE] < price) {
		trap_SendServerCommand(-1, "mu_play sound/items/use_nothing.wav 0\n");
		return;
	}

	switch (item->giType) {
		case IT_WEAPON:
		case IT_AMMO:
			success = Survival_HandleWeaponOrGrenade(ent, activator, item, price);
			break;
		case IT_ARMOR:
			success = Survival_HandleArmorPurchase(activator, item, price);
			break;
		case IT_PERK:
			success = Survival_HandlePerkPurchase(activator, item, price);
			break;
		default:
			return;
	}

	if (success) {
		activator->client->hasPurchased = qtrue;
		ClientUserinfoChanged(clientNum);
	}
}

#define AXIS_OBJECTIVE      1
#define ALLIED_OBJECTIVE    2

void Touch_objective_info(gentity_t *ent, gentity_t *other, trace_t *trace) {
	gentity_t *buyEnt = NULL;
	int price = 0;
	int ammoPrice = 0;
	int isWeapon = ent->isWeapon;
	const char *weaponName = ent->translation;
	const char *techName = NULL;
	const gitem_t *item = NULL;

	if (other->aiCharacter)
	{
		return;
	}

	// Try to find the linked target_buy
	for (int i = 0; i < level.num_entities; i++)
	{
		buyEnt = &g_entities[i];
		if (!buyEnt->inuse)
			continue;
		if (Q_stricmp(buyEnt->classname, "target_buy") != 0)
			continue;
		if (ent->target && buyEnt->targetname && Q_stricmp(ent->target, buyEnt->targetname) == 0)
		{
			techName = buyEnt->buy_item;
			price = buyEnt->price; // This can be 0, and fallback will trigger
			break;
		}
	}

	// If no target_buy was found but a target exists, try to find linked func_invisible_user
	if (!techName && ent->target)
	{
		for (int i = 0; i < level.num_entities; i++)
		{
			gentity_t *funcUser = &g_entities[i];
			if (!funcUser->inuse)
				continue;
			if (Q_stricmp(funcUser->classname, "func_invisible_user") != 0)
				continue;
			if (funcUser->targetname && Q_stricmp(ent->target, funcUser->targetname) == 0)
			{
				price = funcUser->price;
				break;
			}
		}
	}

	// Handle special cases BEFORE item lookup
	if (techName) {
		if (!Q_stricmp(techName, "ammo")) {

		// Do not show price if holding dynamite
		if (other->client->ps.weapon == WP_DYNAMITE_ENG || other->client->ps.weapon == WP_POISONGAS_MEDIC || other->client->ps.weapon == WP_AIRSTRIKE ) {
			return;
		}
			price = (price > 0) ? price : Survival_GetDefaultWeaponPrice(other->client->ps.weapon) / 2;
			if (other->client->ps.weaponUpgraded[other->client->ps.weapon])
			{
				price = svParams.upgradedAmmoPrice;
			}
			if (weaponName && price > 0) {
				trap_SendServerCommand(other - g_entities, va(
					"cpbuy \"%s\nprice: %d\"",
					weaponName, price));
				return;
			}
		} else if (!Q_stricmp(techName, "random_weapon")) {
			price = (price > 0) ? price : svParams.randomWeaponPrice;
			if (weaponName && price > 0) {
				trap_SendServerCommand(other - g_entities, va(
					"cpbuy \"%s\nprice: %d\"",
					weaponName, price));
				return;
			}
		} else if (!Q_stricmp(techName, "upgrade_weapon")) {
			price = (price > 0) ? price : svParams.weaponUpgradePrice;
			if (weaponName && price > 0) {
				trap_SendServerCommand(other - g_entities, va(
					"cpbuy \"%s\nprice: %d\"",
					weaponName, price));
				return;
			}
		} else if (!Q_stricmp(techName, "random_perk")) {
			price = (price > 0) ? price : svParams.randomPerkPrice;
			if (weaponName && price > 0) {
				trap_SendServerCommand(other - g_entities, va(
					"cpbuy \"%s\nprice: %d\"",
					weaponName, price));
				return;
			}
		}
	}

	// Try to find the item definition
	if (techName) {
		item = BG_FindItem(techName);
	}

	if (!item && techName) {
		for (int i = 1; bg_itemlist[i].classname; i++) {
			if (!Q_stricmp(bg_itemlist[i].classname, techName)) {
				item = &bg_itemlist[i];
				break;
			}
		}
	}

	// If price not defined explicitly, fall back based on item type
	if (price <= 0 && item) {
		if (item->giType == IT_WEAPON || item->giType == IT_AMMO) {
			price = Survival_GetDefaultWeaponPrice(item->giTag);
		} else if (item->giType == IT_PERK) {
			price = Survival_GetDefaultPerkPrice(item->giTag);
		} else if (item->giType == IT_ARMOR) {
			price = svParams.armorPrice;
		}
	}

	// Ammo price only applies to weapons
	ammoPrice = isWeapon ? price / 2 : 0;

	if (other->client->ps.weaponUpgraded[other->client->ps.weapon])
	{
		ammoPrice = svParams.upgradedAmmoPrice;
	}

	// Display custom tip if price and name are known
	if (price > 0 && weaponName) {
		if (isWeapon) {
			trap_SendServerCommand(other - g_entities, va(
				"cpbuy \"%s\nprice: %d\nammo_price: %d\"",
				weaponName, price, ammoPrice));
		} else {
			trap_SendServerCommand(other - g_entities, va(
				"cpbuy \"%s\nprice: %d\"",
				weaponName, price));
		}
		return;
	}

	// Otherwise fallback to standard objective info
	if (other->timestamp <= level.time) {
		other->timestamp = level.time + 4500;

		const char *msg = ent->track ? ent->track : va("objective #%i", ent->count);
		int teamFlag = (ent->spawnflags & AXIS_OBJECTIVE) ? 0 :
		               (ent->spawnflags & ALLIED_OBJECTIVE) ? 1 : -1;

		trap_SendServerCommand(other - g_entities, va("oid %d \"You are near %s\n\"", teamFlag, msg));
	}
}