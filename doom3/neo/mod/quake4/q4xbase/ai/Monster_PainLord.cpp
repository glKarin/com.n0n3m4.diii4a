
#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "../Game_local.h"
#include "../client/ClientModel.h"


const idEventDef						AI_RequestProcessing("requestProcessing", "");
const idEventDef						AI_AtProcessingStation("atProcessingStation", "");


class riMonsterPainLord : public idAI {
public:

    CLASS_PROTOTYPE( riMonsterPainLord );

    void					Event_RequestProcessing(void);
    void					Event_AtProcessingStation(void);

private:
    CLASS_STATES_PROTOTYPE ( riMonsterPainLord );
};

CLASS_DECLARATION( idAI, riMonsterPainLord )
    EVENT(AI_RequestProcessing,						riMonsterPainLord::Event_RequestProcessing)
    EVENT(AI_AtProcessingStation,						riMonsterPainLord::Event_AtProcessingStation)
END_CLASS

void riMonsterPainLord::Event_RequestProcessing(void)
{

}

void riMonsterPainLord::Event_AtProcessingStation(void)
{

}

CLASS_STATES_DECLARATION ( riMonsterPainLord )
END_CLASS_STATES
