#ifndef _MODEL_COMMON_TEST_H
#define _MODEL_COMMON_TEST_H

#include "../../framework/Session_local.h"

enum {
    CAN_TEST = 0,
    CAN_TEST_NO_WORLD,
    CAN_TEST_NO_PLAYER,
    CAN_TEST_CURRENT_WORLD_IS_NOT_SESSION,
    CAN_TEST_OTHER,
};

ID_INLINE static idRenderWorld * R_ModelTest_RenderWorld(void)
{
    return sessLocal.rw;
}

static int R_ModelTest_IsAllowTest(void)
{
    idRenderWorld *world = R_ModelTest_RenderWorld();
    if(!world)
        return CAN_TEST_NO_WORLD;
    if(tr.primaryWorld != world)
        return CAN_TEST_CURRENT_WORLD_IS_NOT_SESSION;
    if(!gameEdit->PlayerIsValid())
        return CAN_TEST_NO_PLAYER;
    return CAN_TEST;
}

static bool R_ModelTest_CheckCanTest(int res)
{
    if(res == CAN_TEST)
        return true;
    switch (res) {
        case CAN_TEST_NO_WORLD:
            common->Printf("No session render world!\n");
            break;
        case CAN_TEST_NO_PLAYER:
            common->Printf("No player!\n");
            break;
        case CAN_TEST_CURRENT_WORLD_IS_NOT_SESSION:
            common->Printf("Current render world is not session\n");
            break;
        case CAN_TEST_OTHER:
        default:
            common->Printf("Other reason!\n");
            break;
    }
    return false;
}

#endif