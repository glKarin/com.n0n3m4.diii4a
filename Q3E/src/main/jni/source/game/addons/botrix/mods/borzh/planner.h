#ifdef BOTRIX_BORZH

#ifndef __BOTRIX_PLANNER_H__
#define __BOTRIX_PLANNER_H__


#include "good/string.h"
#include "good/vector.h"

#include "mods/borzh/types_borzh.h"


/// Planner atomic action.
class CAction
{
public:
    CAction(TBotAction iAction, TPlayerIndex iExecutioner, int iArgument):
        iAction(iAction), iExecutioner(iExecutioner), iArgument(iArgument) {}

    bool operator ==( CAction iOther ) const { return iAction == iOther.iAction; }
    bool operator ==( TBotAction iOther ) const { return iAction == iOther; }

    TBotAction iAction;                  ///< Action: move, shoot, push button, climb bot one on another etc.
    TPlayerIndex iExecutioner;           ///< Bot who must perform action.
    TPlayerIndex iHelper;                ///< Bot who helps.
    int iArgument;                       ///< Task argument.
};


class CBotBorzh; // Forward declaration.


/// Class that handles plan creation using FF planning system.
class CPlanner
{
public:
    typedef good::vector<CAction> CPlan; ///< Plan is just secuence of actions.

    /// Return true if planner is currently locked.
    static bool IsLocked() { return m_bLocked; }

    /// Lock the planner. Lock it while you are using planner and GetPlan() result.
    static void Lock( const CBotBorzh* pBot ) { BASSERT( !m_bLocked ); m_bLocked = true; m_pBot = pBot; }

    /// Unlock the planner.
    static void Unlock( const CBotBorzh* m_pBot ) { BASSERT( m_bLocked && (m_pBot == m_pBot) ); m_bLocked = false; }

    /// Return true if planner is currently running.
    static bool IsRunning();

    /// Generate PDDL from bot's beliefs and execute planner with generated pddl.
    static void Start( const CBotBorzh* pBot );

    /// Stop planner if planner is currently running.
    static void Stop();

    /// Return NULL in case of failure. Empty plan if no action is needed.
    static const CPlan* GetPlan();

protected:
    static bool m_bLocked;
    static const CBotBorzh* m_pBot;
};


#endif // __BOTRIX_PLANNER_H__

#endif // BOTRIX_BORZH
