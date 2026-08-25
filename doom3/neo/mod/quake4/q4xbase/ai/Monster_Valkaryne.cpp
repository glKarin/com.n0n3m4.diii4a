
#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"
#include "../client/ClientModel.h"


const idEventDef						AI_Docked("docked", "");
const idEventDef						AI_Undock("undock", "");


class riMonsterValkaryne : public idAI {
public:

    CLASS_PROTOTYPE( riMonsterValkaryne );

    void					Event_Docked(void);
    void					Event_Undock(void);

private:
    CLASS_STATES_PROTOTYPE ( riMonsterValkaryne );
};

CLASS_DECLARATION( idAI, riMonsterValkaryne )
    EVENT(AI_Docked,						riMonsterValkaryne::Event_Docked)
    EVENT(AI_Undock,						riMonsterValkaryne::Event_Undock)
END_CLASS


void riMonsterValkaryne::Event_Docked(void)
{

}

void riMonsterValkaryne::Event_Undock(void)
{

}


CLASS_STATES_DECLARATION ( riMonsterValkaryne )
END_CLASS_STATES
