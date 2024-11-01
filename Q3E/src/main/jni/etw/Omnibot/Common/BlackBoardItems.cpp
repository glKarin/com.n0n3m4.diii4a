#include "PrecompCommon.h"
#include "BlackBoardItems.h"

//////////////////////////////////////////////////////////////////////////

bbItem::bbItem(int _type) 
	: m_Owner(0)
	, m_Target(0)
	, m_ExpireTime(std::numeric_limits<int>::max())
	, m_DeleteOnExpire(false)
	, m_DeleteOnRefCount1(false)
	, m_Type(_type)
	, m_InBB(false)
{
}

bool bbItem::FromScriptTable(gmMachine *_machine, gmTableObject *_from)
{
	gmVariable vOwner = _from->Get(_machine, "Owner");
	gmVariable vTarget = _from->Get(_machine, "Target");
	gmVariable vDuration = _from->Get(_machine, "Duration");
	gmVariable vDeleteOnExpire = _from->Get(_machine, "DeleteOnExpire");

	bool bDeleteOnExpire = 0;
	float fDuration = 0.f;

	vDuration.GetFloatSafe(fDuration, 0.f);
	vDuration.GetBoolSafe(bDeleteOnExpire, false);

	if(vOwner.IsInt() && vTarget.IsInt())
	{
		m_Owner = vOwner.GetInt();
		m_Target = vTarget.GetInt();
		m_ExpireTime = IGame::GetTime() + Utils::SecondsToMilliseconds(fDuration);
		m_DeleteOnExpire = vDeleteOnExpire.GetInt()!=0;
		return true;
	}
	return false;
}

bool bbItem::ToScriptTable(gmMachine *_machine, gmTableObject *&_to)
{
	_to->Set(_machine, "Owner", gmVariable(m_Owner));
	_to->Set(_machine, "Target", gmVariable(m_Target));
	_to->Set(_machine, "ExpireTime", gmVariable(m_ExpireTime));
	_to->Set(_machine, "DeleteOnExpire", gmVariable(m_DeleteOnExpire ? 1 : 0));
	return true;
}

//////////////////////////////////////////////////////////////////////////

//class bbRunAway : public bbItem
//{
//public:
//	bbRunAway() : bbItem(bbk_RunAway) {}
//};

//////////////////////////////////////////////////////////////////////////

bool bbScriptItem::FromScriptTable(gmMachine *_machine, gmTableObject *_from)
{
	m_DataTable.Set(_from, _machine);
	return bbItem::FromScriptTable(_machine, _from);
}

bool bbScriptItem::ToScriptTable(gmMachine *_machine, gmTableObject *&_to)
{
	_to = m_DataTable;
	return true;
}
