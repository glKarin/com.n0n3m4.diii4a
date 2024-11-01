#include "PrecompCommon.h"
#include "PathPlannerBase.h"
#include "PathQuery.h"

PathQuery::PathQuery(Client *_client) :
	m_Client(_client),
	m_Starts(0),
	m_Goals(0),
	m_MaxPathDistance(Utils::FloatMax),
	m_ReturnPartial(false)
{
	/*for(int i = 0; i < MaxDesirabilities; ++i)
	m_Desirabilities[i] = 0.f;*/
}

PathQuery &PathQuery::Starts(const DestinationVector *_starts)
{
	m_Starts = _starts;
	return *this; 
}

PathQuery &PathQuery::Goals(const DestinationVector *_goals)
{
	m_Goals = _goals;
	return *this; 
}

PathQuery &PathQuery::AddTeam(int _team)
{
	m_Team.SetFlag(_team, true);
	return *this; 
}

PathQuery &PathQuery::AddClass(int _class)
{
	m_Class.SetFlag(_class, true);
	return *this; 
}

PathQuery &PathQuery::SetMovementCaps(const BitFlag32 &_caps)
{
	m_MovementCaps = _caps;
	return *this;
}

//PathQuery &PathQuery::AddDesirabiliry(int _type, float _desir)
//{
//	if(InRangeT<int>(_type, 0, MaxDesirabilities))
//	{
//		m_Desirabilities[_type] = _desir;
//	}
//	else
//	{
//		OBASSERT(0, "Desirability Out of Range!");
//	}
//	return *this;
//}