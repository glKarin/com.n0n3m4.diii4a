#ifndef __PATHQUERY_H__
#define __PATHQUERY_H__

// class: PathQuery
//		Represents a simple or complex path query for the path planner to search for any number of
//		up to <MaxGoals> target goals.
class PathQuery
{
public:
	PathQuery &Starts(const DestinationVector *_starts);
	PathQuery &Goals(const DestinationVector *_goals);

	PathQuery &AddTeam(int _team);
	PathQuery &AddClass(int _class);

	PathQuery &SetMovementCaps(const BitFlag32 &_caps);

	PathQuery &MaxPathDistance(float _dist);

	PathQuery(Client *_client = 0);
private:
	Client				*m_Client;

	// specific goal destinations
	const DestinationVector		*m_Starts;
	const DestinationVector		*m_Goals;
	BitFlag32					m_Team;
	BitFlag32					m_Class;
	BitFlag32					m_MovementCaps;
	float						m_MaxPathDistance;

	bool						m_ReturnPartial;
};

#endif