
#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"
#include "../client/ClientModel.h"


class riMonsterTank : public idAI {
public:

    CLASS_PROTOTYPE( riMonsterTank );

private:
    CLASS_STATES_PROTOTYPE ( riMonsterTank );
};

CLASS_DECLARATION( idAI, riMonsterTank )
END_CLASS

CLASS_STATES_DECLARATION ( riMonsterTank )
END_CLASS_STATES
