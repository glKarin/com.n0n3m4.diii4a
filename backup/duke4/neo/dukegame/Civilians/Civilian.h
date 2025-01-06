// Civilian.h
//

class DnCivilian : public DnAI
{
	CLASS_PROTOTYPE(DnCivilian);
public:
	stateResult_t				state_Begin(stateParms_t* parms);
	stateResult_t				state_Idle(stateParms_t* parms);
};