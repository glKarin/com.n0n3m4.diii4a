#ifdef BOTRIX_BORZH

#ifndef __BOTRIX_TYPES_BORZH_H__
#define __BOTRIX_TYPES_BORZH_H__


#include "types.h"


//****************************************************************************************************************
/// Bot task for BorzhMod. Ordered by priority.
//****************************************************************************************************************
enum TBorzhTasks
{
    EBorzhTaskInvalid = -1,                      ///< Bot has no tasks.

    // Next tasks are atomic.
    EBorzhTaskWait = 0,                          ///< Wait for timer to expire. Used during/after talk. Argument is time to wait in msecs.
    EBorzhTaskLook,                              ///< Look at door/button/box.
    EBorzhTaskMove,                              ///< Move to given waypoint.
    EBorzhTaskMoveAndCarry,                      ///< Move to given waypoint carrying a box.
    EBorzhTaskSpeak,                             ///< Speak about something. Argument represents door/button/box/weapon, etc.

    EBorzhTaskPushButton,                        ///< Push a button.
    EBorzhTaskWeaponSet,                         ///< Set current weapon. Argument can be 0xFF for crowbar or CModBorzh::iVarValueWeapon*.
    EBorzhTaskWeaponZoom,                        ///< Zoom weapon.
    EBorzhTaskWeaponRemoveZoom,                  ///< Remove zoom from weapon.
    EBorzhTaskWeaponShoot,                       ///< Shoot weapon.

    EBorzhTaskCarryBox,                          ///< Start carrying box. Argument is box number.
    EBorzhTaskDropBox,                           ///< Drop box at needed position in current area. Arguments are box number and area number.

    EBorzhTaskWaitAnswer,                        ///< Wait for other player to accept/reject task.
    EBorzhTaskWaitPlanner,                       ///< Wait for planner to finish.
    EBorzhTaskWaitPlayer,                        ///< Wait for other player to do something. The bot will say "done" phrase when it finishes.
    EBorzhTaskWaitButton,                        ///< Wait for other player to push a button. The other player must say "i will push button now".
    EBorzhTaskWaitDoor,                          ///< Wait for other player to check a door. The other player must say "the door is opened/closed".
    EBorzhTaskWaitIndications,                   ///< Wait for commands of other player.

    // Next tasks are tasks that consist of several atomic tasks. Ordered by priority, i.e. explore has higher priority than try button.
    EBorzhTaskButtonTryDoor,                     ///< Check which doors opens a button / check if button toggles one door in particular.
    EBorzhTaskBringBox,                          ///< Put box near a wall to climb it.
    EBorzhTaskExplore,                           ///< Exploring new area.
    EBorzhTaskGoToGoal,                          ///< Performing go to goal task.

    EBorzhTaskTotal                              ///< Amount of tasks.
};
typedef int TBorzhTask;                          ///< Bot task.


//****************************************************************************************************************
/// Bot atomic actions for planner.
//****************************************************************************************************************
enum TBotActions
{
    EBotActionInvalid = -1,                      ///< Invalid action.
    EBotActionMove = 0,                          ///< Go to given area.
    EBotActionPushButton,                        ///< Push given button.
    EBotActionShootButton,                       ///< Shoot given button.
    EBotActionCarryBox,                          ///< Carry a box.
    EBotActionCarryBoxFar,                       ///< Carry a box from far distance.
    EBotActionDropBox,                           ///< Drop a box.
    EBotActionClimbBox,                          ///< Climb to a box.
    EBotActionFall,                              ///< Fall from a wall.

    EBotActionTotal                              ///< Amount of actions.
};
typedef int TBotAction;                          ///< Bot action.


#endif // __BOTRIX_TYPES_BORZH_H__

#endif // BOTRIX_BORZH
