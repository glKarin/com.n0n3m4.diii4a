// #ifdef MOD_BOTS

// #define _CTF_BOT_CODE_D3

typedef enum {
    FLAGSTATUS_INBASE = 0,
    FLAGSTATUS_TAKEN  = 1,
    FLAGSTATUS_STRAY  = 2,
    FLAGSTATUS_NONE   = 3
} d3_flagStatus_t;

ID_INLINE static bool IsCTFGame(void)
{
    return gameLocal.isMultiplayer && gameLocal.IsFlagGameType();
}

static rvItemCTFFlag * GetTeamFlag(int team)
{
    assert(team == TEAM_MARINE || team == TEAM_STROGG);

    if (!IsCTFGame() || (team != TEAM_MARINE && team != TEAM_STROGG))     /* CTF */
        return NULL;

    const char *flagDefs[] = {
            "mp_ctf_marine_flag",
            "mp_ctf_strogg_flag",
            // "mp_ctf_one_flag",
    };

    rvItemCTFFlag *teamFlags[TEAM_MAX] = { NULL, NULL, };
    const int size = sizeof(flagDefs) / sizeof(flagDefs[0]);
    for (int i = 0; i < size; i++) {
        idEntity *entity = gameLocal.FindEntityUsingDef(NULL, flagDefs[i]);

        do {
            if (entity == NULL)
                break;

            rvItemCTFFlag *flag = static_cast<rvItemCTFFlag *>(entity);

            if (flag->Team() == i) {
                teamFlags[i] = flag;
                break;
            }

            entity = gameLocal.FindEntityUsingDef(entity, flagDefs[i]);
        } while (entity);
    }

    // common->Printf("GetTeamFlag %d | %p %p\n", team, teamFlags[0], teamFlags[1]);
    return teamFlags[team];
}

static d3_flagStatus_t GetFlagStatus(int team)
{
#ifdef _CTF_BOT_CODE_D3
    //assert( IsGametypeFlagBased() );

    rvItemCTFFlag *teamFlag = GetTeamFlag(team);
    //assert( teamFlag != NULL );

    if (teamFlag != NULL) {
        bool carried = teamFlag->Powerup() == POWERUP_CTF_MARINEFLAG || teamFlag->Powerup() == POWERUP_CTF_STROGGFLAG|| teamFlag->Powerup() == POWERUP_CTF_ONEFLAG;
        if (carried == false && teamFlag->Dropped() == false)
            return FLAGSTATUS_INBASE;

        if (carried == true)
            return FLAGSTATUS_TAKEN;

        if (carried == false && teamFlag->Dropped() == true)
            return FLAGSTATUS_STRAY;
    }

    //assert( !"Invalid flag state." );
    return FLAGSTATUS_NONE;
#else
    rvCTFGameState *gameState = ((rvCTFGameState*)gameLocal.mpGame.GetGameState());
    flagState_t state = gameState->GetFlagState( team );
    switch (state) {
        default:
        case FS_AT_BASE:
            return FLAGSTATUS_INBASE;
        case FS_DROPPED:
            return FLAGSTATUS_STRAY;
        case FS_TAKEN:
            return FLAGSTATUS_TAKEN;
        case FS_TAKEN_STROGG:
            return FLAGSTATUS_TAKEN;
        case FS_TAKEN_MARINE:
            return FLAGSTATUS_TAKEN;
    }
#endif
}

static int GetFlagCarrier(int team)
{
#ifdef _CTF_BOT_CODE_D3
    int iFlagCarrier = -1;

    for (int i = 0; i < gameLocal.numClients; i++) {
        idEntity *ent = gameLocal.entities[ i ];

        if (!ent || !ent->IsType(idPlayer::Type)) {
            continue;
        }

        idPlayer *player = static_cast<idPlayer *>(ent);

        if (player->team != team)
            continue;

        bool carryingFlag = player->PowerUpActive(POWERUP_CTF_MARINEFLAG) || player->PowerUpActive(POWERUP_CTF_STROGGFLAG) || player->PowerUpActive(POWERUP_CTF_ONEFLAG);
        if (carryingFlag) {
            if (iFlagCarrier != -1)
                gameLocal.Warning("BUG: more than one flag carrier on %s team", team == 0 ? "red" : "blue");

            iFlagCarrier = i;
        }
    }

    // common->Printf("GetFlagCarrier %d | %d\n", team, iFlagCarrier);
    return iFlagCarrier;
#else
    rvCTFGameState *gameState = ((rvCTFGameState*)gameLocal.mpGame.GetGameState());
    return gameState->GetFlagCarrier( team );
#endif
}

// #endif
