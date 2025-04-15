
#define MAX_BOT_LEVEL 8

static idCVar harm_si_botLevel( "harm_si_botLevel", "0", CVAR_INTEGER | CVAR_GAME | CVAR_NOCHEAT | CVAR_ARCHIVE, "Bot level(0 = auto; 1 - 8 = difficult level).", 0, MAX_BOT_LEVEL );

static int Bot_GetPlayerModelNames(idStrList &list, int team)
{
    return 0;
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

static int Bot_GetBotLevelData( int level, idDict &ret )
{
    idDict dict;
    int num;
    int botLevel;
    const char *defName;
    const idDeclEntityDef *decl;

    num = Bot_GetBotLevels(dict);
    if(num == 0)
        return -1;

    botLevel = level;
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

static int Bot_GetBotLevelData( idDict &ret )
{
    return Bot_GetBotLevelData(harm_si_botLevel.GetInteger(), ret);
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
    if(level >= 0 && level <= MAX_BOT_LEVEL)
    {
        idDict botLevelDict;
        int find = Bot_GetBotLevelData(level, botLevelDict);
        if(find > 0)
        {
            float fovDegrees;
            botLevelDict.GetFloat( "fov", "90", fovDegrees );
            SetFOV( fovDegrees );

            aimRate			= botLevelDict.GetFloat( "aim_rate", "0.1" );
            aimRate = idMath::ClampFloat(0.1f, 1.0f, aimRate);

            botLevel = find;

            return;
        }
    }

    float fovDegrees;
    spawnArgs.GetFloat( "fov", "90", fovDegrees );
    SetFOV( fovDegrees );

    aimRate			= spawnArgs.GetFloat( "aim_rate", "0.1" );
    aimRate = idMath::ClampFloat(0.1f, 1.0f, aimRate);

    botLevel = spawnArgs.GetInt( "botLevel", "0" );
}

bool botAi::IsGametypeTeamBased(void)   /* CTF */
{
    switch (gameLocal.gameType)
    {
        case GAME_SP:
        case GAME_DM:
        case GAME_TOURNEY:
        case GAME_LASTMAN:
            return false;
#ifdef CTF
            case GAME_CTF:
#endif
        case GAME_TDM:
            return true;

        default:
            assert(!"Add support for your new gametype here.");
    }

    return false;
}

