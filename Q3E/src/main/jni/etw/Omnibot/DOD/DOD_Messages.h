////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
// Title: DOD Message Structure Definitions
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __DOD_MESSAGES_H__
#define __DOD_MESSAGES_H__

#pragma pack(push)
#pragma pack(4)

//////////////////////////////////////////////////////////////////////////

struct Event_Death_DOD
{
	GameEntity	m_WhoKilledMe;
	char		m_MeansOfDeath[32];
	int			m_Dominated;
	int			m_Dominated_Assister;
	int			m_Revenge;
	int			m_Revenge_Assister;
};

struct Event_KilledSomeone_DOD
{
	GameEntity	m_WhoIKilled;
	char		m_MeansOfDeath[32];
	int			m_Dominated;
	int			m_Dominated_Assister;
	int			m_Revenge;
	int			m_Revenge_Assister;
};

//////////////////////////////////////////////////////////////////////////

#pragma pack(pop)

#endif
