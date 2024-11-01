#include "PrecompCommon.h"
#include "Criteria.h"

bool CheckCriteria::ParseOperator(obuint32 sHash)
{
	switch(sHash)
	{
	case 0x862a54e3 /* lessthan */:
	case 0x390caefb /* < */:
		m_Operator = Criteria::OP_LESSTHAN;
		return true;
	case 0x81bc04be /* greaterthan */:
	case 0x3b0cb221 /* > */:
		m_Operator = Criteria::OP_GREATERTHAN;
		return true;
	case 0x513c8d94 /* equals */:
	case 0x90f4dccf /* == */:
		m_Operator = Criteria::OP_EQUALS;
		return true;
	}
	return false;
}

bool CheckCriteria::Check(Client * bot)
{
	return m_pfnCheckCriteria ? m_pfnCheckCriteria(*this,bot) : false;
}

bool Base_CheckCriteria(CheckCriteria &crit,Client * bot)
{
	using namespace Criteria;

	switch(crit.m_Criteria)
	{
	case ON_ENTITY_DELETED:
		{
			if(!IGame::IsEntityValid(crit.m_Subject.GetEntity()))
				return true;
			break;
		}
	case ON_ENTITY_FLAG:
		{
			BitFlag64 f;
			if(SUCCESS(g_EngineFuncs->GetEntityFlags(crit.m_Subject.GetEntity(),f)))
			{
				bool bResult = f.CheckFlag(crit.m_Operand[0].GetInt());
				if(crit.m_Negated)
					bResult = !bResult;
				return bResult;
			}
			break;
		}
	case ON_ENTITY_HEALTH:
		{
			Msg_HealthArmor hlth;
			if(InterfaceFuncs::GetHealthAndArmor(crit.m_Subject.GetEntity(), hlth))
			{
				bool bResult = false;
				HANDLE_OPERATORS(bResult,hlth.m_CurrentHealth, crit.m_Operand[0].GetNumAsInt());
				if(crit.m_Negated)
					bResult = !bResult;
				return bResult;
			}
			break;
		}
	case ON_ENTITY_WEAPON:
		{
			WeaponStatus ws = InterfaceFuncs::GetEquippedWeapon(crit.m_Subject.GetEntity());
			bool bResult = ws.m_WeaponId==crit.m_Operand[0].GetInt();
			if(crit.m_Negated)
				bResult = !bResult;
			return bResult;
		}
	case ON_ENTITY_VELOCITY:
		{
			Vector3f v;
			if(EngineFuncs::EntityVelocity(crit.m_Subject.GetEntity(), v))
			{
				bool bResult = false;
				switch(crit.m_Operator)
				{
				case OP_NONE:
					{
						break;
					}
				case OP_LESSTHAN:
					{
						if(crit.m_Operand[0].IsVector())
							bResult = v < Vector3f(crit.m_Operand[0].GetVector());
						else if(crit.m_Operand[0].IsFloatOrInt())
							bResult = v.Length() < Vector3f(crit.m_Operand[0].GetVector()).Length();
						break;
					}
				case OP_GREATERTHAN:
					{
						if(crit.m_Operand[0].IsVector())
							bResult = v > Vector3f(crit.m_Operand[0].GetVector());
						else if(crit.m_Operand[0].IsFloatOrInt())
							bResult = v.Length() > Vector3f(crit.m_Operand[0].GetVector()).Length();
						break;
					}
				case OP_EQUALS:
					{
						if(crit.m_Operand[0].IsVector())
							bResult = v == Vector3f(crit.m_Operand[0].GetVector());
						else if(crit.m_Operand[0].IsFloatOrInt())
							bResult = v.Length() == Vector3f(crit.m_Operand[0].GetVector()).Length();
						break;
					}
				}
				if(crit.m_Negated)
					bResult = !bResult;
				return bResult;
			}
		}
	case ON_ENTITY_WEAPONCHARGED:
		{
			bool bResult = InterfaceFuncs::IsEntWeaponCharged(crit.m_Subject.GetEntity(), crit.m_Operand[0].GetInt());
			if(crit.m_Negated)
				bResult = !bResult;
			return bResult;
		}
	case ON_ENTITY_WEAPONHASAMMO:
		{
			bool bResult = false;
			WeaponPtr wpn = bot->GetWeaponSystem()->GetWeapon(crit.m_Operand[0].GetInt());
			if(wpn)
				bResult = wpn->GetFireMode(Primary).HasAmmo(crit.m_Operand[1].GetInt());
			if(crit.m_Negated)
				bResult = !bResult;
			return bResult;
		}
	case ON_MAPGOAL_AVAILABLE:
		{
			bool bResult = false;
			MapGoalPtr mg = GoalManager::GetInstance()->GetGoal(crit.m_Operand[0].GetInt());
			if(mg && mg->IsAvailable(bot->GetTeam()))
				bResult = true;
			if(crit.m_Negated)
				bResult = !bResult;
			return bResult;
		}
	case NONE:
	case FIRST_USER_CRITERIA:
		break;
	}
	return false;
}

pfnCheckCriteria CheckCriteria::m_pfnCheckCriteria = Base_CheckCriteria;