
#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"
#include "../client/ClientModel.h"


class riMonsterWalker : public idAI {
public:

    CLASS_PROTOTYPE( riMonsterWalker );

private:
    CLASS_STATES_PROTOTYPE ( riMonsterWalker );
};

CLASS_DECLARATION( idAI, riMonsterWalker )
END_CLASS

CLASS_STATES_DECLARATION ( riMonsterWalker )
END_CLASS_STATES
