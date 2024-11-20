#include "PrecompCommon.h"
#include "TargetInfo.h"
#include "ScriptManager.h"
#include "gmTargetInfo.h"

TargetInfo::TargetInfo() :
	m_DistanceTo		(0),
	m_EntityClass		(0),
	m_CurrentWeapon		(0),
	m_EntityCategory	(0),
	m_LastPosition		(Vector3f::ZERO),
	m_LastVelocity		(Vector3f::ZERO),
	m_LastFacing		(Vector3f::ZERO)
{
}

TargetInfo::~TargetInfo() 
{
	if(m_ScriptObject)
	{
		gmTargetInfo::NullifyObject(m_ScriptObject);
		m_ScriptObject = 0; //NULL;
	}
}

gmUserObject *TargetInfo::GetScriptObject(gmMachine *_machine) const
{
	if(!m_ScriptObject)
	{
		m_ScriptObject.Set(gmTargetInfo::WrapObject(_machine, const_cast<TargetInfo*>(this)), _machine);
	}
	return m_ScriptObject;
}
