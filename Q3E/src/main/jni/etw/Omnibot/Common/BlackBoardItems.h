#ifndef __BLACKBOARDITEMS_H__
#define __BLACKBOARDITEMS_H__

#include "Omni-Bot_Events.h"

#include "gmGCRoot.h"

class gmMachine;
class gmTableObject;

//////////////////////////////////////////////////////////////////////////

class bbItem
{
public:
	int		m_Owner;
	int		m_Target;	
	int		m_ExpireTime;
	bool	m_DeleteOnExpire : 1;
	bool	m_DeleteOnRefCount1 : 1;

	virtual bool FromScriptTable(gmMachine *_machine, gmTableObject *_from);
	virtual bool ToScriptTable(gmMachine *_machine, gmTableObject *&_to);

	inline int GetType() const { return m_Type; }
	bbItem(int _type);
	virtual ~bbItem() {}
private:
	int		m_Type;
	bool	m_InBB;
};

//////////////////////////////////////////////////////////////////////////

class bbDelayGoal : public bbItem
{
public:
	bbDelayGoal() : bbItem(bbk_DelayGoal) {}
};

//////////////////////////////////////////////////////////////////////////

class bbIsTaken : public bbItem
{
public:
	bbIsTaken() : bbItem(bbk_IsTaken) {}
};

//////////////////////////////////////////////////////////////////////////

class bbRunAway : public bbItem
{
public:

	Vector3f	m_Position;
	float		m_Radius;

	bbRunAway() : bbItem(bbk_RunAway), m_Position(0.f,0.f,0.f), m_Radius(0.f) {}
};

//////////////////////////////////////////////////////////////////////////

class bbScriptItem : public bbItem
{
public:
	
	virtual bool FromScriptTable(gmMachine *_machine, gmTableObject *_from);
	virtual bool ToScriptTable(gmMachine *_machine, gmTableObject *&_to);

	bbScriptItem(int _type) : bbItem(_type) {}

	gmGCRoot<gmTableObject> m_DataTable;
};

#endif
