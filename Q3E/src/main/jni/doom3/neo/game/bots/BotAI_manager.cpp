
void botAi::DisconnectAll(void)
{
    for ( int botID = 0; botID < botAi::BOT_MAX_BOTS; botID++ ) {
        if ( botAi::bots[botID].inUse ) {
            idEntity * ent = gameLocal.entities[ botAi::bots[botID].entityNum ];
            if ( ent ) {
                //common->Printf( "[idGameLocal::ServerClientDisconnect][Found bot #%d ent]\n", botID );
                delete gameLocal.entities[ botAi::bots[botID].entityNum ];
                // TinMan: TODO: memset?
                // memset( botAi::bots[botID], 0, sizeof( botInfo_t ) );
                botAi::bots[botID].inUse = false;
                botAi::bots[botID].clientID = -1;
                botAi::bots[botID].entityNum = -1;
            }
        }
    }
}

void botAi::Disconnect(int clientNum)
{
    int botID = clientNum - botAi::BOT_START_INDEX;
    if ( botAi::bots[botID].inUse ) {
        idEntity * ent = gameLocal.entities[ botAi::bots[botID].entityNum ];
        if ( ent ) {
            //common->Printf( "[idGameLocal::ServerClientDisconnect][Found bot #%d ent]\n", botID );
            delete gameLocal.entities[ botAi::bots[botID].entityNum ];
            // TinMan: TODO: memset?
            // memset( botAi::bots[botID], 0, sizeof( botInfo_t ) );
            botAi::bots[botID].inUse = false;
            botAi::bots[botID].clientID = -1;
            botAi::bots[botID].entityNum = -1;
        }
    }
}

int botAi::FindIdleBotSlot(void)
{
    int i;

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

    return i;
}

void botAi::UpdateUI(void)
{
    for ( int clientID = botAi::BOT_START_INDEX; clientID < MAX_CLIENTS; clientID++ ) {
        if ( gameLocal.entities[ clientID ] && gameLocal.entities[ clientID ]->IsType( idPlayer::Type ) ) {
            // core is in charge of syncing down userinfo changes
            // it will also call back game through SetUserInfo with the current info for update
            cmdSystem->BufferCommandText( CMD_EXEC_NOW, va( "updateUI %d\n", clientID ) );
        }
    }
}

bool botAi::GenerateAAS(void)
{
    int i;

    gameLocal.Printf("[Harmattan]: Check AAS load result......\n");
    bool aasLoadSuc = false;
    for( i = 0; i < gameLocal.aasNames.Num(); i++ ) {
        if(gameLocal.aasList[ i ]->GetSettings()) {
            aasLoadSuc = true;
            break;
        }
    }
    gameLocal.Printf("[Harmattan]: AAS load %s.\n", aasLoadSuc ? "success" : "fail");
    if(!aasLoadSuc) {
        gameLocal.Printf("[Harmattan]: Check AAS file exists......\n");
        bool aasFileExists = false;
        for( i = 0; i < gameLocal.aasNames.Num(); i++ ) {
            idStr aasFilePath( gameLocal.mapFileName );
            aasFilePath.SetFileExtension( gameLocal.aasNames[ i ] );
            if (fileSystem->ReadFile(aasFilePath, NULL, NULL) > 0) {
                aasFileExists = true;
                break;
            }
        }
        gameLocal.Printf("[Harmattan]: AAS file %s.\n", aasFileExists ? "exists" : "not found");
        if(!aasFileExists) {
            gameLocal.Printf("[Harmattan]: Generate AAS file %s......\n", gameLocal.mapFileName.c_str());
            cmdSystem->BufferCommandText( CMD_EXEC_NOW, va("botRunAAS %s", gameLocal.mapFileName.c_str()) );
            gameLocal.Printf("[Harmattan]: Generate AAS file %s completed. Try reload AAS.\n", gameLocal.mapFileName.c_str());
            aasLoadSuc = false;
            for( i = 0; i < gameLocal.aasNames.Num(); i++ ) {
                if(gameLocal.aasList[ i ]->Init( idStr( gameLocal.mapFileName ).SetFileExtension( gameLocal.aasNames[ i ] ).c_str(), gameLocal.mapFile->GetGeometryCRC() ))
                    aasLoadSuc = true;
            }
            gameLocal.Printf("[Harmattan]: AAS reload %s.\n", aasLoadSuc ? "success" : "fail");
        }
    }

    return aasLoadSuc;
}