// #ifdef MOD_BOTS

idCVar harm_si_autoFillBots( "harm_si_autoFillBots", "0", CVAR_INTEGER | CVAR_GAME | CVAR_NOCHEAT | CVAR_ARCHIVE, "[Harmattan]: Automatic fill bots after map loaded in multiplayer game(0: disable; other number: bot num).", 0, botAi::BOT_MAX_BOTS );
//karin: auto gen aas file for mp game map with bot
idCVar harm_g_autoGenAASFileInMPGame( "harm_g_autoGenAASFileInMPGame", "1", CVAR_BOOL | CVAR_GAME | CVAR_ARCHIVE, "For bot in Multiplayer-Game, if AAS file load fail and not exists, server can generate AAS file for Multiplayer-Game map automatic.");

#include "BotAI_cfg.cpp"

/*
===================
botAi::Addbot_f
TinMan: Console command. Bring in teh b0tz!
*todo* set default def to something sensible
*todo* random name if default bot
*todo* get passed in args working for no added classname
===================
*/
void botAi::Addbot_f( const idCmdArgs &args )
{
    const char *key, *value;
    int			i;
    idDict		dict;

	if(!CanAddBot())
        return;

	if (args.Argc() < 2)
	{
		common->Warning("USAGE: addBot <botdef> ...key/value pair. e.g. addBot bot_sabot_tinman - see def/bot_sabot_characters.def for more details\n");
		return;
	}

    value = args.Argv( 1 );

    // TinMan: Add rest of key/values passed in
    for( i = 2; i < args.Argc() - 1; i += 2 )
    {
        key = args.Argv( i );
        value = args.Argv( i + 1 );

        dict.Set( key, value );
    }

    i = AddBot(value, dict);
    if (i < 0)
    {
        common->Warning("AddBot fail -> %d\n", i);
        return;
    }
}

/*
===================
botAi::Removebot_f
TinMan: Console command. Bye bye botty.
===================
*/
void botAi::Removebot_f( const idCmdArgs &args )
{
    const char *value;
    int killBotID;

    if ( !gameLocal.isMultiplayer )
    {
        gameLocal.Printf( "This isn't multiplayer, so there no bots to remove, so yeah, you're mental.\n" );
        return;
    }

    if ( !gameLocal.isServer )
    {
        gameLocal.Printf( "Bots may only be removed on server, only it has the powah!\n" );
        return;
    }

    value = args.Argv( 1 );

    killBotID = atoi( value );

    //gameLocal.Printf( "[botAi::Removebot][Bot #%d]\n", killBotID ); // TinMan: *debug*

    if ( killBotID < BOT_MAX_BOTS && bots[killBotID].inUse && gameLocal.entities[ bots[killBotID].clientID ] && gameLocal.entities[ bots[killBotID].entityNum ] )
    {
        gameLocal.ServerClientDisconnect( bots[killBotID].clientID ); // TinMan: KaWhoosh! Youuure outa there!
    }
    else
    {
        gameLocal.Printf( "There is no spoon, I mean, bot #%d\n", killBotID );
        return;
    }
}

int botAi::AddBot(const char *defName, idDict &dict)
{
    const char *key, *value;
    int			i;
    idVec3		org;
    const char* name;
    idDict		userInfo;
    idEntity *	ent;

    int newBotID = 0;
    int newClientID = 0;

    if ( !CanAddBot() )
    {
        return -1;
    }

    if (!defName || !defName[0])
    {
        common->Warning("Must set bot entity def name!\n");
        return -2;
    }

    // Try to find an ID in the bots list
    for ( i = 0; i < BOT_MAX_BOTS; i++ )
    {
        if ( bots[i].inUse )
        {
            // TinMan: make sure it isn't an orphaned spot
            if ( gameLocal.entities[ bots[i].clientID ] && gameLocal.entities[ bots[i].entityNum ]  )
            {
                continue;
            }
            else
            {
                break;
            }
        }
        else
        {
            break;
        }
    }

    if ( i >= BOT_MAX_BOTS )
    {
        gameLocal.Warning("The maximum number of bots are already in the game.\n");
        return -3;
    }
    else
    {
        newBotID = i;
    }

    value = defName;

    // TinMan: Check to see if valid def
    const idDeclEntityDef *botDef = gameLocal.FindEntityDef( defName );
    const char *spawnclass = botDef->dict.GetString( "spawnclass" );
    idTypeInfo *cls = idClass::GetClass( spawnclass );
    if ( !cls || !cls->IsType( botAi::Type ) )
    {
        gameLocal.Warning( "def not of type botAi or no def name given\n" );
        return -4;
    }

    idDict botLevelDict;
    int botLevel = Bot_GetBotLevelData(botLevelDict);
    if(botLevel > 0)
    {
        const char *Bot_Level_Keys[] = {
                "fov",
                "aim_rate",
        };
        int keysLength = sizeof(Bot_Level_Keys) / sizeof(Bot_Level_Keys[0]);
        for(int i = 0; i < keysLength; i++)
        {
            const char *v = botLevelDict.GetString(Bot_Level_Keys[i], "");
            if(v && v[0])
                dict.Set(Bot_Level_Keys[i], v);
        }
    }
    idStr uiName = Bot_GetBotName();
    if(uiName && uiName[0])
        dict.Set("ui_name", uiName.c_str());

    dict.Set( "classname", value );

    dict.Set( "name", va( "bot_%d", newBotID ) ); // TinMan: Set entity name for easier debugging
    dict.SetInt( "botID", newBotID ); // TinMan: Bot needs to know this before it's spawned so it can sync up with the client
    newClientID = BOT_START_INDEX + newBotID; // TinMan: client id, bots use >16

    //gameLocal.Printf("Spawning bot as client %d\n", newClientID);

    // Start up client
    gameLocal.ServerClientConnect(newClientID, NULL);

    gameLocal.ServerClientBegin(newClientID); // TinMan: spawn the fakeclient (and send message to do likewise on other machines)

    idPlayer * botClient = static_cast< idPlayer * >( gameLocal.entities[ newClientID ] ); // TinMan: Make a nice and pretty pointer to fakeclient
    // TinMan: Put our grubby fingerprints on fakeclient. *todo* may want to make these entity flags and make sure they go across to clients so we can rely on using them for clientside code
    botClient->spawnArgs.SetBool( "isBot", true );
    botClient->spawnArgs.SetInt( "botID", newBotID );

    // TinMan: Add client to bot list
    bots[newBotID].inUse	= true;
    bots[newBotID].clientID = newClientID;

    // TinMan: Spawn bot with our dict
    gameLocal.SpawnEntityDef( dict, &ent, false );
    botAi * newBot = static_cast< botAi * >( ent ); // TinMan: Make a nice and pretty pointer to bot

    // TinMan: Add bot to bot list
    bots[newBotID].entityNum = newBot->entityNumber;

    // TinMan: Give me your name, licence and occupation.
    name = newBot->spawnArgs.GetString( "ui_name" );
    idStr botName(va( "[BOT%d] %s", newBotID, name));
    if(botLevel > 0)
        botName.Append(va(" (%d)", botLevel));
    userInfo.Set( "ui_name", botName ); // TinMan: *debug* Prefix [BOTn]

    // TinMan: I love the skin you're in.
    int skinNum = newBot->spawnArgs.GetInt( "mp_skin" );
    //k userInfo.Set( "ui_skin", ui_skinArgs[ skinNum ] );

    // TinMan: Set team
    if ( IsGametypeTeamBased() )
    {
        userInfo.Set( "ui_team", newBot->spawnArgs.GetInt( "team" ) ? "Marine" : "Strogg" );
    }
    else if ( gameLocal.gameType == GAME_SP )
    {
        botClient->team = newBot->spawnArgs.GetInt( "team" );
    }

    // TinMan: Finish up connecting - Called in SetUserInfo
    //gameLocal.mpGame.EnterGame(newClientID);
    //gameLocal.Printf("Bot has been connected, and client has begun.\n");

    userInfo.Set( "ui_ready", "Ready" );

	idStrList playerModelNames;
	int numMarinePlayerModel = Bot_GetPlayerModelNames(playerModelNames, TEAM_MARINE);
	if(numMarinePlayerModel > 0)
	{
		int index = gameLocal.random.RandomInt(numMarinePlayerModel);
		const char *modelName = playerModelNames[index];
		userInfo.Set("model", modelName);
		userInfo.Set("model_marine", modelName);
		userInfo.Set("ui_model", modelName);
		userInfo.Set("ui_model_marine", modelName);
		userInfo.Set("def_default_model", modelName);
		userInfo.Set("def_default_model_marine", modelName);
		botClient->spawnArgs.Set("model", modelName);
		botClient->spawnArgs.Set("model_marine", modelName);
		botClient->spawnArgs.Set("ui_model", modelName);
		botClient->spawnArgs.Set("ui_model_marine", modelName);
		botClient->spawnArgs.Set("def_default_model", modelName);
		botClient->spawnArgs.Set("def_default_model_marine", modelName);
	}
	int numStroggPlayerModel = Bot_GetPlayerModelNames(playerModelNames, TEAM_STROGG);
	if(numStroggPlayerModel > 0)
	{
		int index = gameLocal.random.RandomInt(numStroggPlayerModel) + numMarinePlayerModel;
		const char *modelName = playerModelNames[index];
		userInfo.Set("model_strogg", modelName);
		userInfo.Set("ui_model_strogg", modelName);
		userInfo.Set("def_default_model_strogg", modelName);
		botClient->spawnArgs.Set("model_strogg", modelName);
		botClient->spawnArgs.Set("ui_model_strogg", modelName);
		botClient->spawnArgs.Set("def_default_model_strogg", modelName);
	}
	int numPlayerModel = Bot_GetPlayerModelNames(playerModelNames, TEAM_NONE);
	if(playerModelNames.Num() > 0)
	{
		int index = gameLocal.random.RandomInt(playerModelNames.Num());
		const char *modelName = playerModelNames[index];
		userInfo.Set("model", modelName);
		userInfo.Set("ui_model", modelName);
		userInfo.Set("def_default_model", modelName);
		botClient->spawnArgs.Set("model", modelName);
		botClient->spawnArgs.Set("ui_model", modelName);
		botClient->spawnArgs.Set("def_default_model", modelName);
	}

    gameLocal.SetUserInfo( newClientID, userInfo, false ); // TinMan: apply the userinfo *note* func was changed slightly in 1.3
    botClient->Spectate( false ); // TinMan: Finally done, get outa spectate
    botClient->UpdateModelSetup(true);

    cmdSystem->BufferCommandText( CMD_EXEC_NOW, va( "updateUI %d\n", newClientID ) );

    return newClientID;
}

int botAi::AddBot(const char *name)
{
    idDict dict;
    const idDeclEntityDef *decl;

    decl = (const idDeclEntityDef *)declManager->FindType(DECL_ENTITYDEF, name , false);
    if(decl)
        dict = decl->dict;
    return AddBot(name, dict);
}

void botAi::Cmd_AddBot_f(const idCmdArgs& args)
{
	if(!CanAddBot())
        return;

    if (args.Argc() < 2)
    {
        common->Warning("USAGE: addbots <botdef> <...> e.g. addbots bot_sabot_tinman bot_sabot_fluffy bot_sabot_blackstar - see def/bot_sabot_characters.def for more details\n");
        return;
    }
    int num = args.Argc() - 1;
    if(num > botAi::BOT_MAX_BOTS)
    {
        common->Warning("Max bot num is %d\n", botAi::BOT_MAX_BOTS);
        return;
    }
    int rest = CheckRestClients(num);
    if(rest < 0)
    {
        common->Warning("bots has not enough (%d/%d)\n", rest + num, botAi::BOT_MAX_BOTS);
        return;
    }
    for(int i = 0; i < num; i++)
    {
        const char *value = args.Argv(i + 1);
        int r = AddBot(value);
        if (r < 0)
        {
            common->Warning("AddBot fail -> %d\n", i);
            return;
        }
    }
}

void botAi::Cmd_FillBots_f(const idCmdArgs& args)
{
	if(!CanAddBot())
        return;

    int num = CheckRestClients(0);
    if(args.Argc() > 1)
    {
        int n = atoi(args.Argv(1));
        if(n)
            num = n;
    }

    if(num > botAi::BOT_MAX_BOTS)
    {
        common->Warning("Max bot num is %d\n", botAi::BOT_MAX_BOTS);
        return;
    }

    int rest = CheckRestClients(num);
    if(rest < 0)
    {
        common->Warning("bots has not enough (%d/%d)\n", rest + num, botAi::BOT_MAX_BOTS);
        return;
    }

    idStrList botNames;
    int botNum = Bot_GetBotDefs(botNames);
    idStrList list;
    for(int i = 0; i < num; i++)
    {
        if(list.Num() == 0)
            list = botNames;
        int index = gameLocal.random.RandomInt(list.Num());
        idStr name = list[index];
        list.RemoveIndex(index);
        AddBot(name);
    }
#if 0
    AddBot("bot_sabot_tinman");
	AddBot("bot_sabot_fluffy");
	AddBot("bot_sabot_blackstar");
#endif
}

void botAi::ArgCompletion_addBot( const idCmdArgs &args, void(*callback)( const char *s ) )
{
    int i;
    int num;

    num = declManager->GetNumDecls(DECL_ENTITYDEF);

    for (i = 0; i < num; i++) {
        const idDeclEntityDef *decl = (const idDeclEntityDef *)declManager->DeclByIndex(DECL_ENTITYDEF, i , false);
        if(!decl)
            continue;
        if(!idStr(decl->GetName()).IcmpPrefix("bot_sabot"))
            callback(idStr(args.Argv(0)) + " " + decl->GetName());
    }
}

int botAi::GetNumCurrentActiveBots(void)
{
    int numBots = 0;
    for ( int i = 0; i < botAi::BOT_MAX_BOTS; i++ )
    {
        if ( botAi::bots[i].inUse )
        {
            numBots++;
        }
    }
    return numBots;
}

int botAi::CheckRestClients(int num)
{
    int numBots = GetNumCurrentActiveBots();
    int maxClients = Min(gameLocal.serverInfo.GetInt("si_maxPlayers"), botAi::BOT_MAX_BOTS);
    return maxClients - num - numBots;
}

bool botAi::IsAvailable(void)
{
    static bool _available = false;
    static bool _available_inited = false;

    if(!_available_inited)
    {
        int i;
        int num;
        const char *val;

        num = declManager->GetNumDecls(DECL_ENTITYDEF);

        for (i = 0; i < num; i++) {
            const idDeclEntityDef *decl = (const idDeclEntityDef *)declManager->DeclByIndex(DECL_ENTITYDEF, i , false);
            if(!decl)
                continue;
            if(!idStr(decl->GetName()).IcmpPrefix("bot_sabot"))
            {
                _available = true;
                break;
            }
        }
        _available_inited = true;
    }

    return _available;
}

void botAi::Cmd_BotInfo_f(const idCmdArgs& args)
{
    gameLocal.Printf("SABot(a9)\n");
    gameLocal.Printf("gameLocal.isMultiplayer: %d\n", gameLocal.isMultiplayer);
    gameLocal.Printf("gameLocal.isServer: %d\n", gameLocal.isServer);
    gameLocal.Printf("gameLocal.isClient: %d\n", gameLocal.isClient);
    gameLocal.Printf("botAi::IsAvailable(): %d\n", botAi::IsAvailable());
    gameLocal.Printf("BOT_ENABLED(): %d\n", BOT_ENABLED());
    gameLocal.Printf("Bot slots: total(%d)\n", botAi::BOT_MAX_BOTS);

    int numAAS = gameLocal.GetNumAAS();
    bool botAasLoaded = false;
    gameLocal.Printf("Bot AAS: total(%d)\n", numAAS);
    for(int i = 0; i < numAAS; i++)
    {
        idAAS *aas = gameLocal.GetAAS(i);
        if(!aas)
            continue;
        const idAASFile *aasFile = aas->GetFile();
        if(!aasFile)
            continue;
        gameLocal.Printf("\t%d: %s\n", i, aasFile->GetName());
        if (idStr(aasFile->GetName()).Find( BOT_AAS, false ) > 0)
        {
            botAasLoaded = true;
            break;
        }
    }
    gameLocal.Printf("Bot AAS loaded: %d\n", botAasLoaded);

    int numBots = 0;
    idList<int> botIds;
    for ( int i = 0; i < botAi::BOT_MAX_BOTS; i++ )
    {
        gameLocal.Printf("\t[%d]: inUse(%d), clientID(%d), entityNum(%d), selected(%d)\n", i, botAi::bots[i].inUse, botAi::bots[i].clientID, botAi::bots[i].entityNum, botAi::bots[i].selected);
        if ( botAi::bots[i].inUse )
        {
            botIds.Append(botAi::bots[i].clientID);
            numBots++;
        }
    }
    gameLocal.Printf("Bot slots: used(%d)\n", numBots);

    idStrList botNames;
    int botNum = Bot_GetBotDefs(botNames);
    gameLocal.Printf("Bot defs: total(%d)\n", botNum);
    for(int i = 0; i < botNum; i++)
    {
        gameLocal.Printf("\t%d: %s\n", i, botNames[i].c_str());
    }

    int client = 0;
    gameLocal.Printf("gameLocal.numClients: total(%d)\n", gameLocal.numClients);
    for ( int i = 0; i < gameLocal.numClients ; i++ )
    {
        const idEntity *ent = gameLocal.entities[ i ];

        if(!ent)
            gameLocal.Printf("\t%d: <NULL>\n", i);
        else if ( ent->IsType( idPlayer::Type ) )
        {
            if(botIds.FindIndex(i) < 0)
                gameLocal.Printf("\t%d: Player(%s)\n", i, gameLocal.userInfo[ i ].GetString( "ui_name" ));
            else
                gameLocal.Printf("\t%d: Bot(%s)\n", i, gameLocal.userInfo[ i ].GetString( "ui_name" ));
            client++;
        }
        else
            gameLocal.Printf("\t%d: <NOT PLAYER>\n", i);
    }
    gameLocal.Printf("gameLocal.numClients: connected(%d)\n", client);
}

bool botAi::CanAddBot(void)
{
    if ( !gameLocal.isMultiplayer )
    {
        gameLocal.Warning( "You may only add a bot to a multiplayer game\n" );
        return false;
    }

    if ( !gameLocal.isServer )
    {
        gameLocal.Warning( "Bots may only be added on server, only it has the powah!\n" );
        return false;
    }

    if ( !IsAvailable() )
    {
        gameLocal.Warning( "SABot(a9) mod file missing!\n" );
        return false;
    }

    int numAAS = gameLocal.GetNumAAS();
    bool botAasLoaded = false;
    for(int i = 0; i < numAAS; i++)
    {
        idAAS *aas = gameLocal.GetAAS(i);
        if(!aas)
            continue;
        const idAASFile *aasFile = aas->GetFile();
        if(!aasFile)
            continue;
        if (idStr(aasFile->GetName()).Find( BOT_AAS, false ) > 0)
        {
            botAasLoaded = true;
            break;
        }
    }
    if ( !botAasLoaded )
    {
        gameLocal.Warning( "bot aas file not loaded!\n" );
        return false;
    }
	return true;
}

// #endif
