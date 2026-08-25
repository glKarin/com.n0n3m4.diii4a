
#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"
#include "../client/ClientModel.h"


class riMonsterRetch : public idAI {
public:

    CLASS_PROTOTYPE( riMonsterRetch );

private:
    CLASS_STATES_PROTOTYPE ( riMonsterRetch );
};

CLASS_STATES_DECLARATION ( riMonsterRetch )
END_CLASS_STATES

CLASS_DECLARATION( idAI, riMonsterRetch )
END_CLASS
