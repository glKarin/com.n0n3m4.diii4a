#ifndef __CRITERIA_H__
#define __CRITERIA_H__

//////////////////////////////////////////////////////////////////////////

#define HANDLE_OPERATORS(VAR,OPERAND0,OPERAND1) \
	switch(crit.m_Operator) \
{ \
	case OP_NONE: \
{ \
	break; \
} \
	case OP_LESSTHAN: \
{ \
	VAR = OPERAND0 < OPERAND1; \
	break; \
} \
	case OP_GREATERTHAN: \
{ \
	VAR = OPERAND0 > OPERAND1; \
	break; \
} \
	case OP_EQUALS: \
{ \
	VAR = OPERAND0 == OPERAND1; \
	break; \
} \
}

//////////////////////////////////////////////////////////////////////////

namespace Criteria
{
	enum eCriteria
	{
		NONE,
		ON_ENTITY_DELETED,
		ON_ENTITY_FLAG,
		ON_ENTITY_HEALTH,
		ON_ENTITY_WEAPON,
		ON_ENTITY_VELOCITY,
		ON_ENTITY_WEAPONCHARGED,
		ON_ENTITY_WEAPONHASAMMO,
		ON_MAPGOAL_AVAILABLE,

		FIRST_USER_CRITERIA,
	};

	enum Operator
	{
		OP_NONE,
		OP_LESSTHAN,
		OP_GREATERTHAN,
		OP_EQUALS,
	};
}


//////////////////////////////////////////////////////////////////////////
typedef bool (*pfnCheckCriteria)(CheckCriteria &crit, Client * bot);

class CheckCriteria
{
public:
	enum Token
	{
		TOK_CRITERIA,
		TOK_OPERATOR,
		TOK_OPERAND,
		TOK_DONE,
	};

	enum { MaxOperands=2 };

	Criteria::eCriteria		m_Criteria;
	obUserData				m_Subject;
	obUserData				m_Operand[MaxOperands];
	Criteria::Operator		m_Operator;

	bool ParseOperator(obuint32 sHash);

	void Clear() { m_Criteria = Criteria::NONE; }

	bool Check(Client * bot);

	bool		m_Persistent : 1;
	bool		m_Negated : 1;

	static pfnCheckCriteria m_pfnCheckCriteria;

	CheckCriteria()
		: m_Criteria(Criteria::NONE)
		, m_Operator(Criteria::OP_NONE)
		, m_Persistent(false)
		, m_Negated(false)
	{
	}
};

bool Base_CheckCriteria(CheckCriteria &crit,Client * bot);

#endif
