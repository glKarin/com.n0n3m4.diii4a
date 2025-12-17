#ifdef BOTRIX_MOD_CSS

#ifndef __BOTRIX_TYPES_CSS_H__
#define __BOTRIX_TYPES_CSS_H__



//****************************************************************************************************************
/// Bot task for Counter Strike: Source mod.
//****************************************************************************************************************
enum TBotTasksCSS
{
    EBotTaskGuardBomb = EBotTasksTotal,          ///< Guard C4 (ct, counter-strike mod).
    EBotTaskFindBomb,                            ///< Find C4 (tt, counter-strike mod).
    EBotTaskDefuseBomb,                          ///< Defuse C4 (ct, counter-strike mod).
    EBotTaskPlantBomb,                           ///< Plant C4 (tt, counter-strike mod).
    EBotTaskHelpTeammate,                        ///< Help teammate in sight, even if low armor or no weapon.
    EBotTaskCamping,                             ///< Go to camping waypoint and stay there.
    EBotTaskSniping,                             ///< Go to sniping waypoint and shoot enemies.
};


#endif // __BOTRIX_TYPES_CSS_H__

#endif // BOTRIX_MOD_CSS
