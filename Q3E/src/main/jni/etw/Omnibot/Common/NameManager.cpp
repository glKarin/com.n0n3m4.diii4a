#include "PrecompCommon.h"
#include "NameManager.h"

//////////////////////////////////////////////////////////////////////////

NameReference::NameReference(const String &_name, const String &_profile) :
	m_Name(_name),
	m_ProfileName(_profile)
{
}

NameReference::~NameReference()
{
}

NameManager *NameManager::m_Instance = NULL;

NameManager *NameManager::GetInstance()
{
	if(!m_Instance)
		m_Instance = new NameManager;
	return m_Instance;
}

void NameManager::DeleteInstance()
{
	OB_DELETE(m_Instance);
}

bool NameManager::AddName(const String &_name, const String &_profile)
{
	NamesMap::const_iterator cIt = m_NamesMap.find(_name);
	if(cIt == m_NamesMap.end())
	{
		NamePtr np(new NameReference(_name, _profile));
		m_NamesMap.insert(std::make_pair(_name, np));
		return true;
	}
	return false;
}

void NameManager::DeleteName(const String &_name)
{
	NamesMap::iterator it = m_NamesMap.find(_name);
	if(it != m_NamesMap.end())
		m_NamesMap.erase(it);
}

const String NameManager::GetProfileForName(const String &_name) const
{
	NamesMap::const_iterator cIt = m_NamesMap.find(_name);
	if(cIt != m_NamesMap.end())
	{
		return cIt->second->GetProfileName();
	}
	return String();
}

const String NameManager::GetProfileForClass(const int _class) const
{
	DefaultProfileMap::const_iterator it = m_ProfileMap.find(_class);
	if(it != m_ProfileMap.end())
	{
		return it->second;
	}
	return String();
}

void NameManager::ClearNames()
{
	m_NamesMap.clear();
}

NamePtr NameManager::GetName(const String &_preferred)
{
	if(!_preferred.empty())
	{
		NamesMap::iterator it = m_NamesMap.find(_preferred);
		if(it != m_NamesMap.end())
			return it->second;
		return NamePtr(new NameReference(_preferred));
	}

	StringVector lst;
	NamesMap::iterator it = m_NamesMap.begin(),
		itEnd = m_NamesMap.end();
	for( ; it != itEnd; ++it)
	{
		if(it->second.use_count() <= 1)
			lst.push_back(it->first);
	}

	if(!lst.empty())
	{
#if __cplusplus >= 201703L //karin: random_shuffle removed since C++17
		compat::random_shuffle(lst.begin(), lst.end());
#else
        std::random_shuffle(lst.begin(), lst.end());
#endif
		return GetName(lst.front());
	}

	return NamePtr();
}

void NameManager::SetProfileForClass(const int _class, const String &_name)
{
	m_ProfileMap.insert(std::make_pair(_class, _name));
	const char *clsname = Utils::FindClassName(_class);
	LOG("Class " << (clsname?clsname:"unknown") << " : using profile " << _name.c_str());
}

/*void NameManager::LoadBotNames()
{
boost::regex ex("*.bot");
DirectoryList botFiles;
FileSystem::FindAllFiles("scripts/bots", botFiles, ex);

LOG((Format("Loading %1% bot scripts from: scripts/bots") % botFiles.size()).str());
DirectoryList::const_iterator cIt = botFiles.begin(), cItEnd = botFiles.end();
for(; cIt != cItEnd; ++cIt)
{
WeaponPtr wpn(new Weapon);

int iThreadId;
gmUserObject *pUserObj = wpn->GetScriptObject(ScriptManager::GetInstance()->GetMachine());
gmVariable varThis(pUserObj);

ScriptManager::GetInstance()->ExecuteFile("scripts/weapons/weapon_defaults.gm", iThreadId, &varThis);

if((*cIt).leaf() == "weapon_defaults.gm")
continue;

LOG((Format("Loading Weapon Definition: %1%") % (*cIt).string()).str());
if(ScriptManager::GetInstance()->ExecuteFile(*cIt, iThreadId, &varThis))
{
if(wpn->GetWeaponID() != 0 && wpn->GetWeaponNameHash())
{
RegisterWeapon(wpn->GetWeaponID(), wpn);
}
}
else
{
OBASSERT(0, "Error Running Weapon Script");
}
}
}*/