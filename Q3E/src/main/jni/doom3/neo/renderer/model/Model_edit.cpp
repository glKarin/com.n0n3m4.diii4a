#ifndef _KARIN_MODEL_EDIT_H
#define _KARIN_MODEL_EDIT_H

#include "../../framework/Session_local.h"

namespace modeledit
{
	enum {
		CAN_TEST = 0,
		CAN_TEST_NO_WORLD,
		CAN_TEST_NO_PLAYER,
		CAN_TEST_CURRENT_WORLD_IS_NOT_SESSION,
		CAN_TEST_OTHER,
	};

	static idRenderWorld * RenderWorld(void)
	{
		return sessLocal.rw;
	}

    static int IsAllowTest(void)
    {
        idRenderWorld *world = RenderWorld();
        if(!world)
            return CAN_TEST_NO_WORLD;
        if(tr.primaryWorld != world)
            return CAN_TEST_CURRENT_WORLD_IS_NOT_SESSION;
        if(!gameEdit->PlayerIsValid())
            return CAN_TEST_NO_PLAYER;
        return CAN_TEST;
    }

    static bool CanTest(void)
    {
        int res;

        res = IsAllowTest();
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
};

#endif
