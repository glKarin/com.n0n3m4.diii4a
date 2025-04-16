
static idCVar harm_si_botLevel( "harm_si_botLevel", "0", CVAR_INTEGER | CVAR_GAME | CVAR_NOCHEAT | CVAR_ARCHIVE, "Bot level(0 = auto; 1 - 8 = default difficult level)." );

bool botAi::botAvailable = false;
bool botAi::botInitialized = false;

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

void botAi::InitBotSystem(void)
{
    if(botInitialized)
    {
        gameLocal.Warning("BotAI has initialized!");
        return;
    }

    int i;
    int num;

    num = declManager->GetNumDecls(DECL_ENTITYDEF);

    for (i = 0; i < num; i++) {
        const idDeclEntityDef *decl = (const idDeclEntityDef *)declManager->DeclByIndex(DECL_ENTITYDEF, i , false);
        if(!decl)
            continue;
        if(!idStr(decl->GetName()).IcmpPrefix("bot_sabot"))
        {
            botAvailable = true;
            break;
        }
    }
    botInitialized = true;
    gameLocal.Printf("BotAI initialized: available=%d\n", botAvailable);
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

