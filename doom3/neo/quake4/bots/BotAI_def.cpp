
#define BOT_SET_DEF_KV(k, v) dict.Set(k, v)

idDict botAi::GetBotAASDef(void)
{
    idDict dict;
    BOT_SET_DEF_KV("mins", "-16 -16 0");
    BOT_SET_DEF_KV("maxs", "16 16 80");
    BOT_SET_DEF_KV("usePatches", "0");
    BOT_SET_DEF_KV("writeBrushMap", "0");
    BOT_SET_DEF_KV("playerFlood", "0");
    BOT_SET_DEF_KV("allowSwimReachabilities", "0");
    BOT_SET_DEF_KV("allowFlyReachabilities", "1");
    BOT_SET_DEF_KV("fileExtension", "botaas32");
    BOT_SET_DEF_KV("gravity", "0 0 -1050");
    BOT_SET_DEF_KV("maxStepHeight", "16");
    BOT_SET_DEF_KV("maxBarrierHeight", "48");
    BOT_SET_DEF_KV("maxWaterJumpHeight", "0");
    BOT_SET_DEF_KV("maxFallHeight", "1024"); // TinMan: Actually haven't nailed down exact value of no damage fall, it's > 248 < 256. *note* there is no fatal fall in mp
    BOT_SET_DEF_KV("minFloorCos", "0.6999999881");
    BOT_SET_DEF_KV("tt_barrierJump", "100");
    BOT_SET_DEF_KV("tt_startCrouching", "100");
    BOT_SET_DEF_KV("tt_waterJump", "100");
    BOT_SET_DEF_KV("tt_startWalkOffLedge", "100");
    BOT_SET_DEF_KV("debugColor", "1.0 0.0 0.0");
    BOT_SET_DEF_KV("generateTacticalFeatures", "1");
    return dict;
}

/***************************************************************************
	SABot - Stupid Angry Bot - release alpha 9
	"I obey these words, written in my head. I think they are spelled wrong.");

 	bot_sabot.def
 	Main settings for SABot - flamable, handle with care.
***************************************************************************/
idDict botAi::GetBotSabotDef(void)
{
    idDict dict;
#if 0
    const idDict *declDict = gameLocal.FindEntityDefDict("player_marine_mp", false);
    if(declDict)
        dict = *declDict;
#endif
    BOT_SET_DEF_KV("defName", "bot_sabot");
    //BOT_SET_DEF_KV("inherit", "player_marine_mp"); // SABot a9 using fake client, but no source code. so I still use old DOOM3 source code
    BOT_SET_DEF_KV("spawnclass", "botSabot");
    BOT_SET_DEF_KV("scriptobject", "bot_sabot");

    BOT_SET_DEF_KV("ui_name", "SABota8");
    BOT_SET_DEF_KV("ui_clan", "[SABot]");

    BOT_SET_DEF_KV("ui_model", "model_player_marine");
    BOT_SET_DEF_KV("ui_skin", "base");
    BOT_SET_DEF_KV("ui_model_marine", "");
    BOT_SET_DEF_KV("ui_model_strogg", "");
    BOT_SET_DEF_KV("ui_hitScanTint", "120.0 0.6 1.0"); // Rail Green // id: a tint applied to select hitscan effects.  Specified as a value in HSV color space. Hue [0.0-360.0] Saturation [0.0-1.0] Value [0.75-1.0]

    BOT_SET_DEF_KV("ui_autoSwitch", "0");

    // TinMan: 0-1, Marine, Strogg. Autoballence may have it's wicked way.
    BOT_SET_DEF_KV("team", "0");

    // TinMan:
    BOT_SET_DEF_KV("fov", "120"); // "90" // TinMan: Field of view bot has

    BOT_SET_DEF_KV("aim_ang_delta", "0.2"); // TinMan: delta between current and aimed direction to fire at

    // TinMan: delays, rather ad hoc and buggy.
    BOT_SET_DEF_KV("react_to_enemy", "0.2");
    BOT_SET_DEF_KV("react_to_enemy_sound", "0.3");
    BOT_SET_DEF_KV("react_fire", "0.2"); // TinMan: Delay on firing

    // TinMan: Priorities. Things are barely holding together now
    BOT_SET_DEF_KV("priority_goal_item", "10");

    BOT_SET_DEF_KV("priority_goal_ammo", "10");
    BOT_SET_DEF_KV("priority_goal_weapon", "20");
    BOT_SET_DEF_KV("priority_goal_health", "30");
    BOT_SET_DEF_KV("priority_goal_armor", "35");
    BOT_SET_DEF_KV("priority_goal_powerup", "35");
    BOT_SET_DEF_KV("priority_goal_enemy", "30");
    BOT_SET_DEF_KV("priority_goal_team", "40");

    // TinMan: Item Priorities - Higher = more desirable.
    // item_health_oneHP?
    BOT_SET_DEF_KV("priority_item_health_shard", "1");
    BOT_SET_DEF_KV("priority_item_health_small", "2");
    BOT_SET_DEF_KV("priority_item_health_large", "3");

    BOT_SET_DEF_KV("priority_item_health_mega", "5");
    //entityDef item_health_mega	"inv_bonushealth", "100" // bonus health will go above 100 and tick down

    BOT_SET_DEF_KV("priority_item_armor_shard", "1");
    BOT_SET_DEF_KV("priority_item_armor_small", "2");
    BOT_SET_DEF_KV("priority_item_armor_large", "3");

    // TinMan: Will be increased in code if bot wants it for weapon
    BOT_SET_DEF_KV("priority_ammo_blaster", "1");
    BOT_SET_DEF_KV("priority_ammo_machinegun", "1");
    BOT_SET_DEF_KV("priority_ammo_nailgun", "1");
    BOT_SET_DEF_KV("priority_ammo_railgun", "1");
    BOT_SET_DEF_KV("priority_ammo_shotgun", "1");
    BOT_SET_DEF_KV("priority_ammo_hyperblaster", "1");
    BOT_SET_DEF_KV("priority_ammo_rocketlauncher", "1");
    BOT_SET_DEF_KV("priority_ammo_grenadelauncher", "1");
    BOT_SET_DEF_KV("priority_ammo_lightninggun", "1");
    BOT_SET_DEF_KV("priority_ammo_dmg", "1");

    // TinMan: Powerups
    BOT_SET_DEF_KV("priority_powerup_quad_damage", "3");
    BOT_SET_DEF_KV("priority_powerup_haste", "2");
    BOT_SET_DEF_KV("priority_powerup_regeneration", "2");
    BOT_SET_DEF_KV("priority_powerup_invisibility", "2");
    BOT_SET_DEF_KV("priority_powerup_ammoregen", "1");
    BOT_SET_DEF_KV("priority_powerup_guard", "1");
    BOT_SET_DEF_KV("priority_powerup_doubler", "1");
    BOT_SET_DEF_KV("priority_powerup_scout", "1");

    // TinMan: Team goals
    BOT_SET_DEF_KV("priority_mp_ctf_marine_flag", "1");
    BOT_SET_DEF_KV("priority_mp_ctf_strogg_flag", "1");

    // TinMan: Weapon Priorities - Higher = more desirable. My preciouses!
    //"priority_weapon_blaster", "1");
    BOT_SET_DEF_KV("priority_weapon_gauntlet", "1");
    BOT_SET_DEF_KV("priority_weapon_machinegun", "2");
    BOT_SET_DEF_KV("priority_weapon_shotgun", "3");
    BOT_SET_DEF_KV("priority_weapon_hyperblaster", "4");
    BOT_SET_DEF_KV("priority_weapon_grenadelauncher", "5");
    BOT_SET_DEF_KV("priority_weapon_nailgun", "6");
    BOT_SET_DEF_KV("priority_weapon_lightninggun", "7");
    BOT_SET_DEF_KV("priority_weapon_rocketlauncher", "8");
    BOT_SET_DEF_KV("priority_weapon_railgun", "9");
    BOT_SET_DEF_KV("priority_weapon_dmg", "10");


    /*
    TinMan: Weapon ranges *todo* think of proper ranges and splash
    Frikenstein says:
    _x "sweet spot range" - try to maintain this projectile if possible
    _y minimum range bot can be to be effective (rl/bfg) (move away)
    _z maximum range bot can be to be effective (sg, fists) (move in)
    */
    BOT_SET_DEF_KV("range_weapon_gauntlet", "0 0 8"); // TinMan: *todo* messed to da max
    //"range_weapon_blaster", "180 0 99999");
    BOT_SET_DEF_KV("range_weapon_machinegun", "180 0 99999");
    BOT_SET_DEF_KV("range_weapon_shotgun", "128 0 1500");
    BOT_SET_DEF_KV("range_weapon_hyperblaster", "180 0 3000");
    BOT_SET_DEF_KV("range_weapon_grenadelauncher", "180 64 600");
    BOT_SET_DEF_KV("range_weapon_nailgun", "180 0 3000");
    BOT_SET_DEF_KV("range_weapon_rocketlauncher", "180 64 3000");
    BOT_SET_DEF_KV("range_weapon_railgun", "300 250 99999");
    BOT_SET_DEF_KV("range_weapon_lightninggun", "180 0 768");
    BOT_SET_DEF_KV("range_weapon_dmg", "250 200 3000");


    /*
    TinMan: Aim settings
    */

    /*
    TinMan: aim_accuracy - Variation on aiming, more = bigger variation [0-4].
    */
    BOT_SET_DEF_KV("aim_accuracy", "2");

    // TinMan: variation per weapon - addition to base
    BOT_SET_DEF_KV("aim_accuracy_weapon_gauntlet", "0");
    BOT_SET_DEF_KV("aim_accuracy_weapon_blaster", "0");
    BOT_SET_DEF_KV("aim_accuracy_weapon_machinegun", "0.5");
    BOT_SET_DEF_KV("aim_accuracy_weapon_shotgun", "0");
    BOT_SET_DEF_KV("aim_accuracy_weapon_hyperblaster", "0");
    BOT_SET_DEF_KV("aim_accuracy_weapon_grenadelauncher", "0");
    BOT_SET_DEF_KV("aim_accuracy_weapon_nailgun", "0");
    BOT_SET_DEF_KV("aim_accuracy_weapon_rocketlauncher", "0.5");
    BOT_SET_DEF_KV("aim_accuracy_weapon_railgun", "1");
    BOT_SET_DEF_KV("aim_accuracy_weapon_lightninggun", "0.5");
    BOT_SET_DEF_KV("aim_accuracy_weapon_dmg", "0");

    /*
    TinMan: aim_rate Size of steps blending aimDir each frame: lower = slower bot aiming/turning, higher = quicker [0.1-1].
    */
    BOT_SET_DEF_KV("aim_rate", "0.5");

    BOT_SET_DEF_KV("aim_rate_weapon_gauntlet", "0");
    BOT_SET_DEF_KV("aim_rate_weapon_blaster", "0");
    BOT_SET_DEF_KV("aim_rate_weapon_machinegun", "0");
    BOT_SET_DEF_KV("aim_rate_weapon_shotgun", "0");
    BOT_SET_DEF_KV("aim_rate_weapon_hyperblaster", "-0.1");
    BOT_SET_DEF_KV("aim_rate_weapon_grenadelauncher", "0");
    BOT_SET_DEF_KV("aim_rate_weapon_nailgun", "0");
    BOT_SET_DEF_KV("aim_rate_weapon_rocketlauncher", "0");
    BOT_SET_DEF_KV("aim_rate_weapon_railgun", "-0.1");
    BOT_SET_DEF_KV("aim_rate_weapon_lightninggun", "0");
    BOT_SET_DEF_KV("aim_rate_weapon_dmg", "0");

    /*
    TinMan: aim target - used by getaimdir, will test preferred then rest
    0 = Anywhere
    1 = Eye
    2 = Chest
    3 = Feet
    */
    BOT_SET_DEF_KV("target_weapon_rocketlauncher", "3");
    BOT_SET_DEF_KV("target_weapon_grenadelauncher", "3");
    return dict;
}

/***************************************************************************
	SABot - Stupid Angry Bot - release alpha 9
	"I obey these words, written in my head. I think they are spelled wrong.");

 	bot_sabot_characters.def
 	Custom characters.
 	TinMan: Note I've done bugger all testing as to attack accuracy and aim rate settings, the functions that use these were supposed to be temporary :p
***************************************************************************/
idDict botAi::GetBotSabotTinmanDef(void)
{
    idDict dict = botAi::GetBotSabotDef();
    BOT_SET_DEF_KV("defName", "bot_sabot_tinman");
    BOT_SET_DEF_KV("inherit", "bot_sabot");

    BOT_SET_DEF_KV("ui_name", "Jarad 'TinMan' Hansen");
    BOT_SET_DEF_KV("ui_model", "model_player_marine_tech");

    // TinMan: Weapon Priorities - Higher = more desirable. My preciouses!
    BOT_SET_DEF_KV("priority_weapon_gauntlet", "1");
    BOT_SET_DEF_KV("priority_weapon_machinegun", "2");
    BOT_SET_DEF_KV("priority_weapon_shotgun", "3");
    BOT_SET_DEF_KV("priority_weapon_hyperblaster", "4");
    BOT_SET_DEF_KV("priority_weapon_grenadelauncher", "5");
    BOT_SET_DEF_KV("priority_weapon_nailgun", "6");
    BOT_SET_DEF_KV("priority_weapon_lightninggun", "7");
    BOT_SET_DEF_KV("priority_weapon_railgun", "8");
    BOT_SET_DEF_KV("priority_weapon_rocketlauncher", "9");
    BOT_SET_DEF_KV("priority_weapon_dmg", "10");

    BOT_SET_DEF_KV("aim_accuracy", "1");		// TinMan: Variation on aiming, more = bigger variation.
    BOT_SET_DEF_KV("aim_rate", "0.5"); // TinMan: Size of steps blending aimPos each frame: lower = slower bot aiming/turning, higher = quicker.

    BOT_SET_DEF_KV("aim_ang_delta", "0.2"); // TinMan: delta between current and aimed direction to fire at

    // TinMan: delays, rather ad hoc and buggy.
    BOT_SET_DEF_KV("react_to_enemy", "0.2");
    BOT_SET_DEF_KV("react_to_enemy_sound", "0.3");
    BOT_SET_DEF_KV("react_fire", "0.1"); // TinMan: Delay on firing
    return dict;
}

idDict botAi::GetBotSabotFluffyDef(void)
{
    idDict dict = botAi::GetBotSabotDef();
    BOT_SET_DEF_KV("defName", "bot_sabot_fluffy");
    BOT_SET_DEF_KV("inherit", "bot_sabot");
    BOT_SET_DEF_KV("ui_name", "Fluffy Bunny");
    BOT_SET_DEF_KV("ui_model", "model_player_slimy_transfer");

    BOT_SET_DEF_KV("priority_weapon_gauntlet", "1");
    BOT_SET_DEF_KV("priority_weapon_machinegun", "2");
    BOT_SET_DEF_KV("priority_weapon_railgun", "3");
    BOT_SET_DEF_KV("priority_weapon_rocketlauncher", "4");
    BOT_SET_DEF_KV("priority_weapon_shotgun", "5");
    BOT_SET_DEF_KV("priority_weapon_hyperblaster", "6");
    BOT_SET_DEF_KV("priority_weapon_grenadelauncher", "7");
    BOT_SET_DEF_KV("priority_weapon_nailgun", "8");
    BOT_SET_DEF_KV("priority_weapon_lightninggun", "9");
    BOT_SET_DEF_KV("priority_weapon_dmg", "10");

    BOT_SET_DEF_KV("aim_accuracy", "3");		// TinMan: Variation on aiming, more = bigger variation.
    BOT_SET_DEF_KV("aim_rate", "0.7"); // TinMan: Size of steps blending aimPos each frame: lower = slower bot aiming/turning, higher = quicker.
    return dict;
}

idDict botAi::GetBotSabotBlackstarDef(void)
{
    idDict dict = botAi::GetBotSabotDef();
    BOT_SET_DEF_KV("defName", "bot_sabot_blackstar");
    BOT_SET_DEF_KV("inherit", "bot_sabot");

    BOT_SET_DEF_KV("ui_name", "BlackStar Ninja");
    BOT_SET_DEF_KV("ui_model", "model_player_marine_helmeted");

    BOT_SET_DEF_KV("priority_weapon_gauntlet", "1");
    BOT_SET_DEF_KV("priority_weapon_machinegun", "2");
    BOT_SET_DEF_KV("priority_weapon_shotgun", "3");
    BOT_SET_DEF_KV("priority_weapon_railgun", "4");
    BOT_SET_DEF_KV("priority_weapon_grenadelauncher", "5");
    BOT_SET_DEF_KV("priority_weapon_nailgun", "6");
    BOT_SET_DEF_KV("priority_weapon_lightninggun", "7");
    BOT_SET_DEF_KV("priority_weapon_rocketlauncher", "8");
    BOT_SET_DEF_KV("priority_weapon_hyperblaster", "9");
    BOT_SET_DEF_KV("priority_weapon_dmg", "10");
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

idList<idDict> botAi::GetBotSabotLevelDef(void)
{
    idList<idDict> list;
    {
        idDict dict;
        BOT_SET_DEF_KV("defName", "bot_level1");
        BOT_SET_DEF_KV("fov", "10");
        BOT_SET_DEF_KV("aim_rate", "0.01");
        BOT_SET_DEF_KV("find_radius", "-1");
        list.Append(dict);
    }
    {
        idDict dict;
        BOT_SET_DEF_KV("defName", "bot_level2");
        BOT_SET_DEF_KV("fov", "20");
        BOT_SET_DEF_KV("aim_rate", "0.02");
        BOT_SET_DEF_KV("find_radius", "300");
        list.Append(dict);
    }
    {
        idDict dict;
        BOT_SET_DEF_KV("defName", "bot_level3");
        BOT_SET_DEF_KV("fov", "45");
        BOT_SET_DEF_KV("aim_rate", "0.05");
        BOT_SET_DEF_KV("find_radius", "400");
        list.Append(dict);
    }
    {
        idDict dict;
        BOT_SET_DEF_KV("defName", "bot_level4");
        BOT_SET_DEF_KV("fov", "60");
        BOT_SET_DEF_KV("aim_rate", "0.08");
        BOT_SET_DEF_KV("find_radius", "500");
        list.Append(dict);
    }
    {
        idDict dict;
        BOT_SET_DEF_KV("defName", "bot_level5");
        BOT_SET_DEF_KV("fov", "75");
        BOT_SET_DEF_KV("aim_rate", "0.1");
        BOT_SET_DEF_KV("find_radius", "600");
        list.Append(dict);
    }
    {
        idDict dict;
        BOT_SET_DEF_KV("defName", "bot_level6");
        BOT_SET_DEF_KV("fov", "90");
        BOT_SET_DEF_KV("aim_rate", "0.2");
        BOT_SET_DEF_KV("find_radius", "700");
        list.Append(dict);
    }
    {
        idDict dict;
        BOT_SET_DEF_KV("defName", "bot_level7");
        BOT_SET_DEF_KV("fov", "120");
        BOT_SET_DEF_KV("aim_rate", "0.5");
        BOT_SET_DEF_KV("find_radius", "800");
        list.Append(dict);
    }
    {
        idDict dict;
        BOT_SET_DEF_KV("defName", "bot_level8");
        BOT_SET_DEF_KV("fov", "150");
        BOT_SET_DEF_KV("aim_rate", "1.0");
        BOT_SET_DEF_KV("find_radius", "1000");
        list.Append(dict);
    }
    return list;
}