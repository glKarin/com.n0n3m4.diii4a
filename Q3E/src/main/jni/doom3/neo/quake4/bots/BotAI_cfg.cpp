
idCVar botAi::harm_si_botLevel( "harm_si_botLevel", "0", CVAR_INTEGER | CVAR_GAME | CVAR_NOCHEAT | CVAR_ARCHIVE, "Bot level(0 = auto; 1 - 8 = default difficult level)." );
idCVar botAi::harm_si_botWeapons( "harm_si_botWeapons", "", CVAR_GAME | CVAR_NOCHEAT | CVAR_ARCHIVE, "Bot weapons when spawn, separate by comma(,); 0=none, *=all. Allow weapon index(e.g. 2,3), weapon short name(e.g. shotgun,machinegun), weapon full name(e.g. weapon_machinegun,weapon_shotgun), and allow mix(e.g. machinegun,3,weapon_rocketlauncher). All weapon: 1=machinegun, 2=shotgun, 3=hyperblaster, 4=grenadelauncher, 5=nailgun, 6=rocketlauncher, 7=railgun, 8=lightninggun, 9=dmg, 10=napalmgun." );
idCVar botAi::harm_si_botAmmo( "harm_si_botAmmo", "0", CVAR_INTEGER | CVAR_GAME | CVAR_NOCHEAT | CVAR_ARCHIVE, "Bot weapon ammo when spawn, depend on `harm_si_botWeapons`. -1=max ammo, 0=none, >0=ammo" );

bool botAi::botAvailable = false;
bool botAi::botInitialized = false;

void botAi::InitBotSystem(void)
{
    if(botInitialized)
    {
        gameLocal.Warning("BotAI has initialized!");
        return;
    }

    int i;
    int num;
    bool valid = false;

    num = declManager->GetNumDecls(DECL_ENTITYDEF);

    // check def
    for (i = 0; i < num; i++) {
        const idDeclEntityDef *decl = (const idDeclEntityDef *)declManager->DeclByIndex(DECL_ENTITYDEF, i , false);
        if(!decl)
            continue;
        if(!idStr(decl->GetName()).IcmpPrefix("bot_sabot"))
        {
            valid = true;
            break;
        }
    }
    // check script
    if(valid)
    {
        valid = fileSystem->ReadFile(BOT_SCRIPT_FILE, NULL, NULL) > 0;
    }
    botAvailable = valid;
    botInitialized = true;
    gameLocal.Printf("BotAI initialized: available=%d\n", botAvailable);
}

int botAi::GetPlayerModelNames(idStrList &list, int team)
{
	int i;
	int num = 0;
	int numPlayerModel;
    const idDecl *decl;
    const rvDeclPlayerModel *playerModel;

	numPlayerModel = declManager->GetNumDecls(DECL_PLAYER_MODEL);
	for(i = 0; i < numPlayerModel; i++)
	{
		decl = declManager->DeclByIndex(DECL_PLAYER_MODEL, i, true);
		if(!decl)
			continue;
		playerModel = static_cast<const rvDeclPlayerModel *>(decl);
		if(team == TEAM_STROGG)
		{
			if(idStr::Icmp(playerModel->team , "strogg"))
				continue;
		}
		else if(team == TEAM_MARINE)
		{
			if(idStr::Icmp(playerModel->team , "marine"))
				continue;
		}
		else if(team == TEAM_NONE)
		{
			if(idStr::Icmp(playerModel->team , ""))
				continue;
		}
		list.Append(playerModel->GetName());
		num++;
	}
	return num;
}

int botAi::GetBotDefs( idStrList &list )
{
    int num;
    int i;
    int res = 0;
    const idDeclEntityDef *decl;

    num = declManager->GetNumDecls(DECL_ENTITYDEF);

    for (i = 0; i < num; i++) {
        decl = (const idDeclEntityDef *)declManager->DeclByIndex(DECL_ENTITYDEF, i , false);
        if(!decl)
            continue;
        if(!idStr(decl->GetName()).IcmpPrefix("bot_sabot"))
        {
            list.Append(decl->GetName());
            res++;
        }
    }

    return res;
}

int botAi::GetBotLevels( idDict &list )
{
    int num;
    int i;
    int res = 0;
    const idDeclEntityDef *decl;

    num = declManager->GetNumDecls(DECL_ENTITYDEF);

    for (i = 0; i < num; i++) {
        decl = (const idDeclEntityDef *)declManager->DeclByIndex(DECL_ENTITYDEF, i , false);
        if(!decl)
            continue;
        if(!idStr(decl->GetName()).IcmpPrefix("bot_level"))
        {
            list.Set(decl->GetName() + strlen("bot_level"), decl->GetName());
            res++;
        }
    }

    return res;
}

int botAi::GetBotLevelData( int level, idDict &ret )
{
    idDict dict;
    int num;
    int botLevel;
    const char *defName;
    const idDeclEntityDef *decl;

    num = GetBotLevels(dict);
    if(num == 0)
        return -1;

    botLevel = level;
    if(botLevel <= 0)
    {
        int n = gameLocal.random.RandomInt(num);
        const idKeyValue *kv = dict.GetKeyVal(n);
        if(kv)
            botLevel = atoi(kv->GetKey());
    }
    defName = dict.GetString(va("%d", botLevel), "");
    if(!defName || !defName[0])
        return -2;

    decl = (const idDeclEntityDef *)declManager->FindType(DECL_ENTITYDEF, defName , false);
    if(!decl)
        return -3;
    ret = decl->dict;
    return botLevel;
}

idStr botAi::GetBotName( int index )
{
    const idDeclEntityDef *decl = (const idDeclEntityDef *)declManager->FindType(DECL_ENTITYDEF, "bot_names" , false);
    if(!decl)
        return idStr();
    if(index > 0)
    {
        const char *name = decl->dict.GetString(va("ui_name%d", index), "");
        if(name && name[0])
            return name;
    }
    else if(index == 0)
    {
        const char *name = decl->dict.GetString("ui_name", "");
        if(name && name[0])
            return name;
        name = decl->dict.GetString("ui_name0", "");
        if(name && name[0])
            return name;
    }
    return decl->dict.RandomPrefix("ui_name", gameLocal.random);
}

void botAi::SetBotLevel(int level)
{
    if(level >= 0)
    {
        idDict botLevelDict;
        int find = GetBotLevelData(level, botLevelDict);
        if(find > 0)
        {
            float fovDegrees;
            botLevelDict.GetFloat( "fov", "90", fovDegrees );
            SetFOV( fovDegrees );

            aimRate			= botLevelDict.GetFloat( "aim_rate", "0.1" );
            aimRate = idMath::ClampFloat(0.1f, 1.0f, aimRate);

			findRadius		= botLevelDict.GetFloat("find_radius", "-1.0");

            botLevel = find;

            return;
        }
    }

    float fovDegrees;
    spawnArgs.GetFloat( "fov", "90", fovDegrees );
    SetFOV( fovDegrees );

    aimRate			= spawnArgs.GetFloat( "aim_rate", "0.1" );
    aimRate = idMath::ClampFloat(0.1f, 1.0f, aimRate);
	findRadius		= spawnArgs.GetFloat("find_radius", "-1.0");

    botLevel = spawnArgs.GetInt( "botLevel", "0" );
}

bool botAi::IsGametypeTeamBased(void)   /* CTF */
{
    return gameLocal.IsTeamGame();
}

int botAi::MakeWeaponMask(const char *wp)
{
    if(!idStr::Cmp("*", wp))
        return BOT_ALL_MP_WEAPON;
    idStrList list = idStr::SplitUnique(wp, ',');
    if(list.Num() == 0)
        return 0;
    for(int i = 0; i < list.Num(); i++)
        idStr::StripWhitespace(list[i]);
    return MakeWeaponMask(list);
}

int botAi::MakeWeaponMask(const idStrList &list)
{
#define BOT_MW(I, N, A, U, M) \
	if(list.FindIndex(#I) != -1 || list.FindIndex(N) != -1 || list.FindIndex("weapon_" N) != -1) { \
		i |= (1 << I); \
	}
    int i = 0;

    //BOT_MW(0, "blaster", "", -1, -1);
    //BOT_MW(0, "gauntlet", "", 0, 0);
    BOT_MW(1, "machinegun", "machinegun", 40, 300);
    BOT_MW(2, "shotgun", "shotgun", 8, 50);
    BOT_MW(3, "hyperblaster", "hyperblaster", 60, 400);
    BOT_MW(4, "grenadelauncher", "grenadelauncher", 8, 50);
    BOT_MW(5, "nailgun", "nailgun", 50, 300);
    BOT_MW(6, "rocketlauncher", "rocketlauncher", 1, 40);
    BOT_MW(7, "railgun", "railgun", 3, 50);
    BOT_MW(8, "lightninggun", "lightninggun", 100, 400);
    BOT_MW(9, "dmg", "dmg", 1, 25);
    BOT_MW(10, "napalmgun", "napalmgun", 20, 40);

    return i;
#undef BOT_MW
}

int botAi::InsertBasicWeaponMask(int i)
{
    // weapon_machinegun,weapon_gauntlet
    i |= 1;
    i |= (1 << 1);
    return i;
}

idStr botAi::MakeWeaponString(int w)
{
#define BOT_MW(I, N, A, U, M) \
	if(w & (1 << I)) { \
		str.Append(",weapon_" N); \
	}
    idStr str;
    if(w == 0)
        return str;

    //BOT_MW(0, "blaster", "", -1, -1);
    BOT_MW(0, "gauntlet", "", 0, 0);
    BOT_MW(1, "machinegun", "machinegun", 40, 300);
    BOT_MW(2, "shotgun", "shotgun", 8, 50);
    BOT_MW(3, "hyperblaster", "hyperblaster", 60, 400);
    BOT_MW(4, "grenadelauncher", "grenadelauncher", 8, 50);
    BOT_MW(5, "nailgun", "nailgun", 50, 300);
    BOT_MW(6, "rocketlauncher", "rocketlauncher", 1, 40);
    BOT_MW(7, "railgun", "railgun", 3, 50);
    BOT_MW(8, "lightninggun", "lightninggun", 100, 400);
    BOT_MW(9, "dmg", "dmg", 1, 25);
    BOT_MW(10, "napalmgun", "napalmgun", 20, 40);

    str.StripLeading(',');
    return str;
#undef BOT_MW
}

idDict botAi::MakeAmmoDict(int w, int num)
{
#define BOT_MW(I, N, A, U, M) \
	if(w & (1 << I)) {\
		dict.SetInt("ammo_" A, num >= 0 ? U * num : M); \
	}
    idDict dict;
    if(w == 0)
        return dict;

    //BOT_MW(0, "blaster", "", -1, -1);
    //BOT_MW(0, "gauntlet", "", 0, 0);
    BOT_MW(1, "machinegun", "machinegun", 40, 300);
    BOT_MW(2, "shotgun", "shotgun", 8, 50);
    BOT_MW(3, "hyperblaster", "hyperblaster", 60, 400);
    BOT_MW(4, "grenadelauncher", "grenadelauncher", 8, 50);
    BOT_MW(5, "nailgun", "nailgun", 50, 300);
    BOT_MW(6, "rocketlauncher", "rocketlauncher", 1, 40);
    BOT_MW(7, "railgun", "railgun", 3, 50);
    BOT_MW(8, "lightninggun", "lightninggun", 100, 400);
    BOT_MW(9, "dmg", "dmg", 1, 25);
    BOT_MW(10, "napalmgun", "napalmgun", 20, 40);

    return dict;
#undef BOT_MW
}

void botAi::InsertEmptyAmmo(idDict &dict)
{
#define BOT_MW(I, N, A, U, M) \
	if(!dict.FindKey("ammo_" A)) {\
		dict.SetInt("ammo_" A, 0); \
	}

    //BOT_MW(0, "blaster", "", -1, -1);
    //BOT_MW(0, "gauntlet", "", 0, 0);
    BOT_MW(1, "machinegun", "machinegun", 40, 300);
    BOT_MW(2, "shotgun", "shotgun", 8, 50);
    BOT_MW(3, "hyperblaster", "hyperblaster", 60, 400);
    BOT_MW(4, "grenadelauncher", "grenadelauncher", 8, 50);
    BOT_MW(5, "nailgun", "nailgun", 50, 300);
    BOT_MW(6, "rocketlauncher", "rocketlauncher", 1, 40);
    BOT_MW(7, "railgun", "railgun", 3, 50);
    BOT_MW(8, "lightninggun", "lightninggun", 100, 400);
    BOT_MW(9, "dmg", "dmg", 1, 25);
    BOT_MW(10, "napalmgun", "napalmgun", 20, 40);

#undef BOT_MW
}

