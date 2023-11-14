// #ifdef MOD_BOTS

#define MAX_BOT_LEVEL 8

static idCVar harm_si_botLevel( "harm_si_botLevel", "0", CVAR_INTEGER | CVAR_GAME | CVAR_NOCHEAT | CVAR_ARCHIVE, "[Harmattan]: Bot level(0 - auto; 1 - 8: difficult level).", 0, MAX_BOT_LEVEL );

static int Bot_GetPlayerModelNames(idStrList &list, int team)
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

static int Bot_GetBotDefs( idStrList &list )
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

static int Bot_GetBotLevels( idDict &list )
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

static int Bot_GetBotLevelData( idDict &ret )
{
    idDict dict;
    int num;
    int botLevel;
    const char *defName;
    const idDeclEntityDef *decl;

    num = Bot_GetBotLevels(dict);
    if(num == 0)
        return -1;

    botLevel = harm_si_botLevel.GetInteger();
    if(botLevel <= 0 || botLevel > MAX_BOT_LEVEL)
        botLevel = gameLocal.random.RandomInt(MAX_BOT_LEVEL) + 1;
    defName = dict.GetString(va("%d", botLevel), "");
    if(!defName || !defName[0])
        return -2;

    decl = (const idDeclEntityDef *)declManager->FindType(DECL_ENTITYDEF, defName , false);
    if(!decl)
        return -3;
    ret = decl->dict;
    return botLevel;
}

static idStr Bot_GetBotName( int index = -1 )
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

// #endif
