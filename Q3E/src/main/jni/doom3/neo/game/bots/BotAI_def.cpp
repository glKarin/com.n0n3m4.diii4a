
#define BOT_SET_DEF_KV(k, v) dict.Set(k, v)

idDict botAi::GetBotAASDef(void)
{
    idDict dict;
    BOT_SET_DEF_KV("defName", "botaas48");
    BOT_SET_DEF_KV("mins", "-24 -24 0");
    BOT_SET_DEF_KV("maxs", "24 24 82");
    BOT_SET_DEF_KV("usePatches", "0");
    BOT_SET_DEF_KV("writeBrushMap", "0");
    BOT_SET_DEF_KV("playerFlood", "0");
    BOT_SET_DEF_KV("allowSwimReachabilities", "0");
    BOT_SET_DEF_KV("allowFlyReachabilities", "1");
    BOT_SET_DEF_KV("fileExtension", "botaas48");
    BOT_SET_DEF_KV("gravity", "0 0 -1050");
    BOT_SET_DEF_KV("maxStepHeight", "18");
    BOT_SET_DEF_KV("maxBarrierHeight", "48");
    BOT_SET_DEF_KV("maxWaterJumpHeight", "20");
    BOT_SET_DEF_KV("maxFallHeight", "193");
    BOT_SET_DEF_KV("minFloorCos", "0.6999999881");
    BOT_SET_DEF_KV("tt_barrierJump", "100");
    BOT_SET_DEF_KV("tt_startCrouching", "100");
    BOT_SET_DEF_KV("tt_waterJump", "100");
    BOT_SET_DEF_KV("tt_startWalkOffLedge", "100");
    return dict;
}

/***************************************************************************
Doom3Bots - Open bot framework for Doom 3
release ? - "Beware of TinMans horrifying code");

bot_base.def
Base settings for bots.

***************************************************************************/
idDict botAi::GetBotBaseDef(void)
{
    idDict dict;
    BOT_SET_DEF_KV("defName", "bot_base");
    BOT_SET_DEF_KV("spawnclass", "botAi");
    BOT_SET_DEF_KV("scriptobject", "bot_base");

    BOT_SET_DEF_KV("npc_name", "BotBase");

    // TinMan: 0-5 - Puke, Red, Blue, Green, Yellow *todo* randomise
    BOT_SET_DEF_KV("mp_skin", "2");

    // TinMan: 0-1, Red, Blue
    BOT_SET_DEF_KV("team", "0");

    BOT_SET_DEF_KV("aim_rate", "0.5"); // TinMan: Size of steps blending aimPos each frame: lower = slower bot aiming/turning, higher = quicker.
    return dict;
}

/***************************************************************************
	SABot - Stupid Angry Bot - release alpha 7 - "I'm not a puppet! I'm a real boy!");

 	bot_sabot.def
 	Main settings for SABot - flamable, handle with care.
 	*todo* tweaking, zero thought has been put into most values
***************************************************************************/
idDict botAi::GetBotSabotDef(void)
{
    idDict dict = botAi::GetBotBaseDef();
    BOT_SET_DEF_KV("defName", "bot_sabot");
    BOT_SET_DEF_KV("inherit", "bot_base");
    BOT_SET_DEF_KV("spawnclass", "botSabot");
    BOT_SET_DEF_KV("scriptobject", "bot_sabot");

    BOT_SET_DEF_KV("npc_name", "SABot");

    // TinMan: 0-5 - Puke, Red, Blue, Green, Yellow
    BOT_SET_DEF_KV("mp_skin", "3");

    // TinMan: 0-1, Red, Blue
    BOT_SET_DEF_KV("team", "0");
    // TinMan:
    BOT_SET_DEF_KV("fov", "120"); //"90");
    BOT_SET_DEF_KV("aim_accuracy", "2");		// TinMan: Variation on aiming, more = bigger variation [0-4].
    BOT_SET_DEF_KV("aim_rate", "0.5"); // TinMan: Size of steps blending aimPos each frame: lower = slower bot aiming/turning, higher = quicker [0.1-1].

    // TinMan: Priorities  - Note: At the moment getItemPriority adds a random [0-15]
    BOT_SET_DEF_KV("priority_goal_item", "10");
    BOT_SET_DEF_KV("priority_goal_enemy", "40");
    BOT_SET_DEF_KV("priotity_goal_team", "60");

    // TinMan: Item Priorities - Higher = more desirable.
    // Priority is 0 if full health/armor, this if less, and more if more hurt.
    BOT_SET_DEF_KV("priority_item_medkit", "40");
    BOT_SET_DEF_KV("priority_item_medkit_small", "40");
    BOT_SET_DEF_KV("priority_item_armor_security", "40");
    BOT_SET_DEF_KV("priority_item_armor_shard", "20");

    BOT_SET_DEF_KV("priority_item_backpack", "30");

    // TinMan: Will be increased in code if bot want's it for weapon
    BOT_SET_DEF_KV("priority_ammo_bullets_small", "10");
    BOT_SET_DEF_KV("priority_ammo_bullets_large", "10");
    BOT_SET_DEF_KV("priority_ammo_shells_small", "10");
    BOT_SET_DEF_KV("priority_ammo_shells_large", "10");
    BOT_SET_DEF_KV("priority_ammo_clip_small", "10");
    BOT_SET_DEF_KV("priority_ammo_clip_large", "10");
    BOT_SET_DEF_KV("priority_ammo_grenade_small", "10");
    BOT_SET_DEF_KV("priority_ammo_rockets_small", "10");
    BOT_SET_DEF_KV("priority_ammo_rockets_large", "10");
    BOT_SET_DEF_KV("priority_ammo_cells_small", "10");
    BOT_SET_DEF_KV("priority_ammo_cells_large", "10");
    BOT_SET_DEF_KV("priority_ammo_belt_small", "10");
    BOT_SET_DEF_KV("priority_ammo_bfg_small", "10");

    // TinMan: 
    BOT_SET_DEF_KV("priority_powerup_megahealth", "90");
    BOT_SET_DEF_KV("priority_powerup_berserk", "85");
    BOT_SET_DEF_KV("priority_powerup_invisibility", "75");
    BOT_SET_DEF_KV("priority_powerup_adrenaline", "75");
    BOT_SET_DEF_KV("powerup_invulnerability", "90");

    // TinMan: Weapon Priorities - Higher = more desirable. My preciouses!
    // *todo* split priories, range, pred into it's own defs. Will need game code func to load/grab funcs.
    BOT_SET_DEF_KV("priority_weapon_fists", "1");
    BOT_SET_DEF_KV("priority_weapon_pistol", "10");
    BOT_SET_DEF_KV("priority_weapon_shotgun", "20");
    BOT_SET_DEF_KV("priority_weapon_machinegun", "30");
    BOT_SET_DEF_KV("priority_weapon_shotgun_double", "35");
    BOT_SET_DEF_KV("priority_weapon_chaingun", "40");
    BOT_SET_DEF_KV("priority_weapon_plasmagun", "50");
    BOT_SET_DEF_KV("priority_weapon_rocketlauncher", "60");
    BOT_SET_DEF_KV("priority_weapon_bfg", "70");
    BOT_SET_DEF_KV("priority_weapon_handgrenade", "1");
    BOT_SET_DEF_KV("priority_weapon_chainsaw", "30");
    BOT_SET_DEF_KV("priority_weapon_flashlight", "1");

    /* 
    TinMan: Weapon ranges
    Frikenstein says:
    _x "sweet spot range" - try to maintain this projectile if possible
    _y minimum range bot can be to be effective (rl/bfg) (move away)
    _z maximum range bot can be to be effective (sg, fists) (move in)
    */
    BOT_SET_DEF_KV("range_weapon_fists", "32 0 64");
    BOT_SET_DEF_KV("range_weapon_pistol", "180 0 99999");
    BOT_SET_DEF_KV("range_weapon_shotgun", "128 0 2000");
    BOT_SET_DEF_KV("range_weapon_shotgun_double", "128 0 1500");
    BOT_SET_DEF_KV("range_weapon_machinegun", "180 0 99999");
    BOT_SET_DEF_KV("range_weapon_chaingun", "180 0 99999");
    BOT_SET_DEF_KV("range_weapon_plasmagun", "180 0 3000");
    BOT_SET_DEF_KV("range_weapon_rocketlauncher", "180 64 3000"); // TinMan: SP splash radius 175, MP 125
    BOT_SET_DEF_KV("range_weapon_bfg", "180 64 3000");	// TinMan: Splash radius 150
    BOT_SET_DEF_KV("range_weapon_handgrenade", "180 64 2000"); // TinMan: SP splash radius 175, MP 150
    BOT_SET_DEF_KV("range_weapon_chainsaw", "32 0 64");
    BOT_SET_DEF_KV("range_weapon_flashlight", "32 0 64");

    /* 
    TinMan: Prediction Types
    1 = Explosive - aim at feet
    */
    BOT_SET_DEF_KV("predict_weapon_rocketlauncher", "1");

    // TinMan: Projectile speeds for prediction 
    // *todo* write func to grab these off weapon type, dunno some of them are wierd.
    BOT_SET_DEF_KV("projectile_weapon_pistol", "7200");
    BOT_SET_DEF_KV("projectile_weapon_pistol_mp", "20000");
    BOT_SET_DEF_KV("projectile_weapon_shotgun", "2000"); // TinMan: What the heck? 60 30 10
    BOT_SET_DEF_KV("projectile_weapon_shotgun_mp", "20000");
    BOT_SET_DEF_KV("projectile_weapon_shotgun_double", "7200");
    BOT_SET_DEF_KV("projectile_weapon_shotgun_double_mp", "20000");
    BOT_SET_DEF_KV("projectile_weapon_machinegun", "7200");
    BOT_SET_DEF_KV("projectile_weapon_machinegun_mp", "20000");
    BOT_SET_DEF_KV("projectile_weapon_chaingun", "2000");
    BOT_SET_DEF_KV("projectile_weapon_chaingun_mp", "20000");
    BOT_SET_DEF_KV("projectile_weapon_plasmagun", "700");
    BOT_SET_DEF_KV("projectile_weapon_rocketlauncher", "900");
    BOT_SET_DEF_KV("projectile_weapon_bfg", "350");
    BOT_SET_DEF_KV("projectile_weapon_bfg_mp", "250");
    BOT_SET_DEF_KV("projectile_weapon_handgrenade", "300"); // TinMan: ?? 300 0 40

    BOT_SET_DEF_KV("projectile_weapon_fists", "20000");
    BOT_SET_DEF_KV("projectile_weapon_chainsaw", "20000");
    BOT_SET_DEF_KV("projectile_weapon_flashlight", "20000");
    return dict;
}

/***************************************************************************
	SABot - Stupid Angry Bot - release alpha 8x

 	bot_sabot.def
 	Main settings for SABot - flamable, handle with care.
***************************************************************************/
idDict botAi::GetBotSabotA8xDef(void)
{
    idDict dict = botAi::GetBotBaseDef();
    BOT_SET_DEF_KV("defName", "bot_sabot_a8");
    BOT_SET_DEF_KV("inherit", "bot_base");
    BOT_SET_DEF_KV("spawnclass", "botSabot");
    BOT_SET_DEF_KV("scriptobject", "bot_sabot_a8");

    BOT_SET_DEF_KV("npc_name", "SABota8x");

    // TinMan: 0-5 - Puke, Red, Blue, Green, Yellow
    BOT_SET_DEF_KV("mp_skin", "3");

    // TinMan: 0-1, Red, Blue
    BOT_SET_DEF_KV("team", "0");

    // TinMan:
    BOT_SET_DEF_KV("fov", "120"); //"90");
    BOT_SET_DEF_KV("aim_accuracy", "2");		// TinMan: Variation on aiming, more = bigger variation [0-4].
    BOT_SET_DEF_KV("aim_rate", "0.5"); // TinMan: Size of steps blending aimPos each frame: lower = slower bot aiming/turning, higher = quicker [0.1-1].

    // TinMan: Priorities
    BOT_SET_DEF_KV("priority_goal_item", "10");

    BOT_SET_DEF_KV("priority_goal_ammo", "10");
    BOT_SET_DEF_KV("priority_goal_weapon", "20");
    BOT_SET_DEF_KV("priority_goal_health", "30");
    BOT_SET_DEF_KV("priority_goal_armor", "30");
    BOT_SET_DEF_KV("priority_goal_powerup", "30");
    BOT_SET_DEF_KV("priority_goal_enemy", "40");
    BOT_SET_DEF_KV("priority_goal_team", "50");

    // TinMan: Item Priorities - Higher = more desirable.
    // Priority is 0 if full health/armor, this if less, and more if more hurt.
    BOT_SET_DEF_KV("priority_item_medkit_small", "1");
    BOT_SET_DEF_KV("priority_item_medkit", "2");
    BOT_SET_DEF_KV("priority_item_armor_shard", "1");
    BOT_SET_DEF_KV("priority_item_armor_security", "2");

    BOT_SET_DEF_KV("priority_item_backpack", "30");

    // TinMan: Will be increased in code if bot want's it for weapon
    BOT_SET_DEF_KV("priority_ammo_bullets_small", "1");
    BOT_SET_DEF_KV("priority_ammo_bullets_large", "2");
    BOT_SET_DEF_KV("priority_ammo_shells_small", "1");
    BOT_SET_DEF_KV("priority_ammo_shells_large", "2");
    BOT_SET_DEF_KV("priority_ammo_clip_small", "1");
    BOT_SET_DEF_KV("priority_ammo_clip_large", "2");
    BOT_SET_DEF_KV("priority_ammo_grenade_small", "1");
    BOT_SET_DEF_KV("priority_ammo_rockets_small", "1");
    BOT_SET_DEF_KV("priority_ammo_rockets_large", "2");
    BOT_SET_DEF_KV("priority_ammo_cells_small", "1");
    BOT_SET_DEF_KV("priority_ammo_cells_large", "2");
    BOT_SET_DEF_KV("priority_ammo_belt_small", "1");
    BOT_SET_DEF_KV("priority_ammo_bfg_small", "1");

    // TinMan:
    BOT_SET_DEF_KV("priority_powerup_adrenaline", "1");
    BOT_SET_DEF_KV("priority_powerup_megahealth", "2");
    BOT_SET_DEF_KV("priority_powerup_berserk", "3");
    BOT_SET_DEF_KV("priority_powerup_invisibility", "4");
    BOT_SET_DEF_KV("powerup_invulnerability", "5");

    // TinMan: Weapon Priorities - Higher = more desirable. My preciouses!
    // *todo* split priories, range, pred into it's own defs. Will need game code func to load/grab funcs.
    BOT_SET_DEF_KV("priority_weapon_fists", "1");
    BOT_SET_DEF_KV("priority_weapon_flashlight", "2");
    BOT_SET_DEF_KV("priority_weapon_pistol", "3");
    BOT_SET_DEF_KV("priority_weapon_shotgun", "4");
    BOT_SET_DEF_KV("priority_weapon_machinegun", "5");
    BOT_SET_DEF_KV("priority_weapon_shotgun_double", "6");
    BOT_SET_DEF_KV("priority_weapon_chaingun", "7");
    BOT_SET_DEF_KV("priority_weapon_plasmagun", "8");
    BOT_SET_DEF_KV("priority_weapon_rocketlauncher", "9");
    BOT_SET_DEF_KV("priority_weapon_bfg", "10");
    BOT_SET_DEF_KV("priority_weapon_handgrenade", "1");
    BOT_SET_DEF_KV("priority_weapon_chainsaw", "6");


    /*
    TinMan: Weapon ranges
    Frikenstein says:
    _x "sweet spot range" - try to maintain this projectile if possible
    _y minimum range bot can be to be effective (rl/bfg) (move away)
    _z maximum range bot can be to be effective (sg, fists) (move in)
    */
    BOT_SET_DEF_KV("range_weapon_fists", "32 0 64");
    BOT_SET_DEF_KV("range_weapon_pistol", "180 0 99999");
    BOT_SET_DEF_KV("range_weapon_shotgun", "128 0 2000");
    BOT_SET_DEF_KV("range_weapon_shotgun_double", "128 0 1500");
    BOT_SET_DEF_KV("range_weapon_machinegun", "180 0 99999");
    BOT_SET_DEF_KV("range_weapon_chaingun", "180 0 99999");
    BOT_SET_DEF_KV("range_weapon_plasmagun", "180 0 3000");
    BOT_SET_DEF_KV("range_weapon_rocketlauncher", "180 64 3000"); // TinMan: SP splash radius 175, MP 125
    BOT_SET_DEF_KV("range_weapon_bfg", "180 64 3000");	// TinMan: Splash radius 150
    BOT_SET_DEF_KV("range_weapon_handgrenade", "180 64 2000"); // TinMan: SP splash radius 175, MP 150
    BOT_SET_DEF_KV("range_weapon_chainsaw", "32 0 64");
    BOT_SET_DEF_KV("range_weapon_flashlight", "32 0 64");

    /*
    TinMan: Prediction Types
    1 = Explosive - aim at feet
    */
    BOT_SET_DEF_KV("predict_weapon_rocketlauncher", "1");

    // TinMan: Projectile speeds for prediction
    // *todo* write func to grab these off weapon type, dunno some of them are wierd.
    BOT_SET_DEF_KV("projectile_weapon_pistol", "7200");
    BOT_SET_DEF_KV("projectile_weapon_pistol_mp", "20000");
    BOT_SET_DEF_KV("projectile_weapon_shotgun", "2000"); // TinMan: What the heck? 60 30 10
    BOT_SET_DEF_KV("projectile_weapon_shotgun_mp", "20000");
    BOT_SET_DEF_KV("projectile_weapon_shotgun_double", "7200");
    BOT_SET_DEF_KV("projectile_weapon_shotgun_double_mp", "20000");
    BOT_SET_DEF_KV("projectile_weapon_machinegun", "7200");
    BOT_SET_DEF_KV("projectile_weapon_machinegun_mp", "20000");
    BOT_SET_DEF_KV("projectile_weapon_chaingun", "2000");
    BOT_SET_DEF_KV("projectile_weapon_chaingun_mp", "20000");
    BOT_SET_DEF_KV("projectile_weapon_plasmagun", "700");
    BOT_SET_DEF_KV("projectile_weapon_rocketlauncher", "900");
    BOT_SET_DEF_KV("projectile_weapon_bfg", "350");
    BOT_SET_DEF_KV("projectile_weapon_bfg_mp", "250");
    BOT_SET_DEF_KV("projectile_weapon_handgrenade", "300"); // TinMan: ?? 300 0 40

    BOT_SET_DEF_KV("projectile_weapon_fists", "20000");
    BOT_SET_DEF_KV("projectile_weapon_chainsaw", "20000");
    BOT_SET_DEF_KV("projectile_weapon_flashlight", "20000");

    return dict;
}

/***************************************************************************
	SABot - Stupid Angry Bot - release alpha 7 - "I'm not a puppet! I'm a real boy!");

 	bot_sabot_characters.def
 	Custom characters.
 	TinMan: Note I've done bugger all testing as to attack accuracy and aim rate settings, the functions that use these are pretty much temporary anyway.
***************************************************************************/
idDict botAi::GetBotSabotTinmanDef(void)
{
    idDict dict = botAi::GetBotSabotDef();
    BOT_SET_DEF_KV("defName", "bot_sabot_tinman");
    BOT_SET_DEF_KV("inherit", "bot_sabot");

    BOT_SET_DEF_KV("npc_name", "Jarad 'TinMan' Hansen");

    // TinMan: 0-5 - Puke, Red, Blue, Green, Yellow
    BOT_SET_DEF_KV("mp_skin", "4");

    // TinMan: 0-1, Red, Blue
    BOT_SET_DEF_KV("team", "0");

    BOT_SET_DEF_KV("aim_accuracy", "2");		// TinMan: Variation on aiming, more = bigger variation.
    BOT_SET_DEF_KV("aim_rate", "0.5"); // TinMan: Size of steps blending aimPos each frame: lower = slower bot aiming/turning, higher = quicker.
    return dict;
}

idDict botAi::GetBotSabotFluffyDef(void)
{
    idDict dict = botAi::GetBotSabotDef();
    BOT_SET_DEF_KV("defName", "bot_sabot_fluffy");
    BOT_SET_DEF_KV("inherit", "bot_sabot");
    BOT_SET_DEF_KV("npc_name", "Fluffy Bunny");
    BOT_SET_DEF_KV("mp_skin", "4");
    BOT_SET_DEF_KV("aim_accuracy", "3");		// TinMan: Variation on aiming, more = bigger variation.
    BOT_SET_DEF_KV("aim_rate", "0.7"); // TinMan: Size of steps blending aimPos each frame: lower = slower bot aiming/turning, higher = quicker.
    return dict;
}

idDict botAi::GetBotSabotBlackstarDef(void)
{
    idDict dict = botAi::GetBotSabotDef();
    BOT_SET_DEF_KV("defName", "bot_sabot_blackstar");
    BOT_SET_DEF_KV("inherit", "bot_sabot");
    BOT_SET_DEF_KV("npc_name", "BlackStar Ninja");
    BOT_SET_DEF_KV("mp_skin", "2");

    BOT_SET_DEF_KV("priority_weapon_rocketlauncher", "50");
    BOT_SET_DEF_KV("priority_weapon_plasmagun_mp", "60");
    return dict;
}


idDict botAi::GetBotSabotNamesDef(void)
{
    idDict dict;
    BOT_SET_DEF_KV("defName", "bot_names");
    BOT_SET_DEF_KV("ui_name1", "Anna");
    BOT_SET_DEF_KV("ui_name2", "BJ");
    BOT_SET_DEF_KV("ui_name3", "Caitlyn");
    BOT_SET_DEF_KV("ui_name4", "DOOM");
    BOT_SET_DEF_KV("ui_name6", "Fiora");
    BOT_SET_DEF_KV("ui_name8", "Harmattan");
    BOT_SET_DEF_KV("ui_name9", "id");
    BOT_SET_DEF_KV("ui_name10", "Jessica");
    BOT_SET_DEF_KV("ui_name11", "Karin");
    BOT_SET_DEF_KV("ui_name12", "Leon");
    BOT_SET_DEF_KV("ui_name13", "Marine");
    BOT_SET_DEF_KV("ui_name14", "Natasha");
    BOT_SET_DEF_KV("ui_name17", "Quake");
    BOT_SET_DEF_KV("ui_name18", "Raven");
    BOT_SET_DEF_KV("ui_name19", "Slayer");
    BOT_SET_DEF_KV("ui_name22", "Verena");
    BOT_SET_DEF_KV("ui_name23", "Wolfenstein");
    BOT_SET_DEF_KV("ui_name26", "Zhao");

    BOT_SET_DEF_KV("ui_name27", "Ada");

    /* DOOM3 NPC */
    BOT_SET_DEF_KV("ui_name101", "Adrian Carmack");
    BOT_SET_DEF_KV("ui_name102", "Andy Chang");
    BOT_SET_DEF_KV("ui_name103", "Ari Braden");
    BOT_SET_DEF_KV("ui_name104", "Ben Peterson");
    BOT_SET_DEF_KV("ui_name105", "Brent Davis");
    BOT_SET_DEF_KV("ui_name106", "Brian");
    BOT_SET_DEF_KV("ui_name107", "Brian Franko");
    BOT_SET_DEF_KV("ui_name108", "Brian Jenkins");
    BOT_SET_DEF_KV("ui_name109", "Brian Mora");
    BOT_SET_DEF_KV("ui_name110", "Brian Wellington");
    BOT_SET_DEF_KV("ui_name111", "Chris Baughman");
    BOT_SET_DEF_KV("ui_name112", "Christian Antkow");
    BOT_SET_DEF_KV("ui_name113", "Corbin Hues");
    BOT_SET_DEF_KV("ui_name114", "David Carter");
    BOT_SET_DEF_KV("ui_name115", "David Robbins");
    BOT_SET_DEF_KV("ui_name116", "Derek Wayland");
    BOT_SET_DEF_KV("ui_name117", "Director W. Banks");
    BOT_SET_DEF_KV("ui_name118", "Donna Jackson");
    BOT_SET_DEF_KV("ui_name119", "Dr. Pat Harvey");
    BOT_SET_DEF_KV("ui_name120", "Dr. Peter Raleigh");
    BOT_SET_DEF_KV("ui_name121", "Duncan Mathews");
    BOT_SET_DEF_KV("ui_name122", "Dusty Welch");
    BOT_SET_DEF_KV("ui_name123", "Earl Besch");
    BOT_SET_DEF_KV("ui_name124", "Edward Sorrenson");
    BOT_SET_DEF_KV("ui_name125", "Elizabeth McNeil");
    BOT_SET_DEF_KV("ui_name126", "Elliot Swann");
    BOT_SET_DEF_KV("ui_name127", "Eric Grossman");
    BOT_SET_DEF_KV("ui_name128", "Eric Webb");
    BOT_SET_DEF_KV("ui_name129", "Erik Reeves");
    BOT_SET_DEF_KV("ui_name130", "Ethan Peterson");
    BOT_SET_DEF_KV("ui_name131", "Frank Cerano");
    BOT_SET_DEF_KV("ui_name132", "Frank Delahue");
    BOT_SET_DEF_KV("ui_name133", "Fredric Anubus");
    BOT_SET_DEF_KV("ui_name134", "Fredrik Nilsson");
    BOT_SET_DEF_KV("ui_name135", "George Poota");
    BOT_SET_DEF_KV("ui_name136", "Graham Fuchs");
    BOT_SET_DEF_KV("ui_name137", "Greg O'Brian");
    BOT_SET_DEF_KV("ui_name138", "Gregg Brandenburg");
    BOT_SET_DEF_KV("ui_name139", "Guy Harollson");
    BOT_SET_DEF_KV("ui_name140", "Heather Elaine");
    BOT_SET_DEF_KV("ui_name141", "Henry Bielefeldt");
    BOT_SET_DEF_KV("ui_name142", "Henry Nelson");
    BOT_SET_DEF_KV("ui_name143", "Henry Varela");
    BOT_SET_DEF_KV("ui_name144", "Human Resources");
    BOT_SET_DEF_KV("ui_name145", "Ian McCormick");
    BOT_SET_DEF_KV("ui_name146", "JMP van Waveren");
    BOT_SET_DEF_KV("ui_name147", "James Holiday");
    BOT_SET_DEF_KV("ui_name148", "James Houska");
    BOT_SET_DEF_KV("ui_name149", "James Torbin");
    BOT_SET_DEF_KV("ui_name150", "Jeff Dickens");
    BOT_SET_DEF_KV("ui_name151", "Jerry Keehan");
    BOT_SET_DEF_KV("ui_name152", "Jim Bowier");
    BOT_SET_DEF_KV("ui_name153", "Jim Dose");
    BOT_SET_DEF_KV("ui_name154", "John Bianga");
    BOT_SET_DEF_KV("ui_name155", "John Carmack");
    BOT_SET_DEF_KV("ui_name156", "John McDermott");
    BOT_SET_DEF_KV("ui_name157", "John Okonkwo");
    BOT_SET_DEF_KV("ui_name158", "Jordan Kenedy");
    BOT_SET_DEF_KV("ui_name159", "Karl Cullen");
    BOT_SET_DEF_KV("ui_name160", "Karl Roper");
    BOT_SET_DEF_KV("ui_name161", "Kenneth Scott");
    BOT_SET_DEF_KV("ui_name162", "Kevin Cloud");
    BOT_SET_DEF_KV("ui_name163", "Lee Pommeroy");
    BOT_SET_DEF_KV("ui_name164", "Liz McNeil");
    BOT_SET_DEF_KV("ui_name165", "Lloyd Renstrom");
    BOT_SET_DEF_KV("ui_name166", "Lowell Foshay");
    BOT_SET_DEF_KV("ui_name167", "Mal Blackwell");
    BOT_SET_DEF_KV("ui_name168", "Malcolm Betruger");
    BOT_SET_DEF_KV("ui_name169", "Mark Lamia");
    BOT_SET_DEF_KV("ui_name170", "Mark Robertson");
    BOT_SET_DEF_KV("ui_name171", "Marty Stratton");
    BOT_SET_DEF_KV("ui_name172", "Mathew Gaiser");
    BOT_SET_DEF_KV("ui_name173", "Mathew Morton");
    BOT_SET_DEF_KV("ui_name174", "Matt Hooper");
    BOT_SET_DEF_KV("ui_name175", "Matthew White");
    BOT_SET_DEF_KV("ui_name176", "Michael Abrams");
    BOT_SET_DEF_KV("ui_name177", "Nicholas Sadowayj");
    BOT_SET_DEF_KV("ui_name178", "Pat Duffy");
    BOT_SET_DEF_KV("ui_name179", "Patrick Thomas");
    BOT_SET_DEF_KV("ui_name180", "Paul Downing");
    BOT_SET_DEF_KV("ui_name181", "Phil Wilson");
    BOT_SET_DEF_KV("ui_name182", "Pierce Rogers");
    BOT_SET_DEF_KV("ui_name183", "Ray Gerhardt");
    BOT_SET_DEF_KV("ui_name184", "Richard Davis");
    BOT_SET_DEF_KV("ui_name185", "Robert Duffy");
    BOT_SET_DEF_KV("ui_name186", "Ron Ridge");
    BOT_SET_DEF_KV("ui_name187", "Russell Weilder");
    BOT_SET_DEF_KV("ui_name188", "Ryan S.");
    BOT_SET_DEF_KV("ui_name189", "Sam Harding");
    BOT_SET_DEF_KV("ui_name190", "Sarah Holsten");
    BOT_SET_DEF_KV("ui_name191", "Scott Johnson");
    BOT_SET_DEF_KV("ui_name192", "Seneca Menard");
    BOT_SET_DEF_KV("ui_name193", "Sergeant Kelly");
    BOT_SET_DEF_KV("ui_name194", "Steve Rescoe");
    BOT_SET_DEF_KV("ui_name195", "Steven Finch");
    BOT_SET_DEF_KV("ui_name196", "T. Brooks");
    BOT_SET_DEF_KV("ui_name197", "Thomas Franks");
    BOT_SET_DEF_KV("ui_name198", "Tim Willits");
    BOT_SET_DEF_KV("ui_name199", "Timmy Rogers");
    BOT_SET_DEF_KV("ui_name200", "Timothee Besset");
    BOT_SET_DEF_KV("ui_name201", "Todd Hollenshead");
    BOT_SET_DEF_KV("ui_name202", "Walter Connors");
    BOT_SET_DEF_KV("ui_name203", "William Landow");
    BOT_SET_DEF_KV("ui_name204", "Yon Brady");

    /* Quake4 NPC */
    BOT_SET_DEF_KV("ui_name301", "Anderson");
    BOT_SET_DEF_KV("ui_name302", "Bidwell");
    BOT_SET_DEF_KV("ui_name303", "Cortez");
    BOT_SET_DEF_KV("ui_name304", "Kane");
    BOT_SET_DEF_KV("ui_name305", "Morris");
    BOT_SET_DEF_KV("ui_name306", "Rhodes");
    BOT_SET_DEF_KV("ui_name307", "Sledge");
    BOT_SET_DEF_KV("ui_name308", "Strauss");
    BOT_SET_DEF_KV("ui_name309", "Voss");
    return dict;
}

idDict botAi::GetBotSabotLevel1Def(void)
{
    idDict dict;
    BOT_SET_DEF_KV("defName", "bot_level1");
    BOT_SET_DEF_KV("fov", "10");
    BOT_SET_DEF_KV("aim_rate", "0.01");
    BOT_SET_DEF_KV("find_radius", "-1");
    return dict;
}

idDict botAi::GetBotSabotLevel2Def(void)
{
    idDict dict;
    BOT_SET_DEF_KV("defName", "bot_level2");
    BOT_SET_DEF_KV("fov", "20");
    BOT_SET_DEF_KV("aim_rate", "0.02");
    BOT_SET_DEF_KV("find_radius", "300");
    return dict;
}

idDict botAi::GetBotSabotLevel3Def(void)
{
    idDict dict;
    BOT_SET_DEF_KV("defName", "bot_level3");
    BOT_SET_DEF_KV("fov", "45");
    BOT_SET_DEF_KV("aim_rate", "0.05");
    BOT_SET_DEF_KV("find_radius", "400");
    return dict;
}

idDict botAi::GetBotSabotLevel4Def(void)
{
    idDict dict;
    BOT_SET_DEF_KV("defName", "bot_level4");
    BOT_SET_DEF_KV("fov", "60");
    BOT_SET_DEF_KV("aim_rate", "0.08");
    BOT_SET_DEF_KV("find_radius", "500");
    return dict;
}

idDict botAi::GetBotSabotLevel5Def(void)
{
    idDict dict;
    BOT_SET_DEF_KV("defName", "bot_level5");
    BOT_SET_DEF_KV("fov", "75");
    BOT_SET_DEF_KV("aim_rate", "0.1");
    BOT_SET_DEF_KV("find_radius", "600");
    return dict;
}

idDict botAi::GetBotSabotLevel6Def(void)
{
    idDict dict;
    BOT_SET_DEF_KV("defName", "bot_level6");
    BOT_SET_DEF_KV("fov", "90");
    BOT_SET_DEF_KV("aim_rate", "0.2");
    BOT_SET_DEF_KV("find_radius", "700");
    return dict;
}

idDict botAi::GetBotSabotLevel7Def(void)
{
    idDict dict;
    BOT_SET_DEF_KV("defName", "bot_level7");
    BOT_SET_DEF_KV("fov", "120");
    BOT_SET_DEF_KV("aim_rate", "0.5");
    BOT_SET_DEF_KV("find_radius", "800");
    return dict;
}

idDict botAi::GetBotSabotLevel8Def(void)
{
    idDict dict;
    BOT_SET_DEF_KV("defName", "bot_level8");
    BOT_SET_DEF_KV("fov", "150");
    BOT_SET_DEF_KV("aim_rate", "1.0");
    BOT_SET_DEF_KV("find_radius", "1000");
    return dict;
}