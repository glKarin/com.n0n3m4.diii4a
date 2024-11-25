#include "PrecompCommon.h"

#include "ScriptManager.h"
#include "gmWeapon.h"
#include "WeaponDatabase.h"

WeaponDatabase g_WeaponDatabase;

WeaponDatabase::WeaponDatabase()
{
}

WeaponDatabase::~WeaponDatabase()
{
}

void WeaponDatabase::RegisterWeapon(int _weaponId, const WeaponPtr &_wpn)
{
	WeaponMap::const_iterator it = m_WeaponMap.find(_weaponId);
	if(it == m_WeaponMap.end())
	{
		m_WeaponMap.insert(std::make_pair(_weaponId, _wpn));
	}
	else
	{
		Utils::OutputDebug(kError, va("Duplicate Weapon Id: %d", _weaponId));
	}
}

WeaponPtr WeaponDatabase::CopyWeapon(Client *_client, int _weaponId)
{
	WeaponMap::const_iterator it = m_WeaponMap.find(_weaponId);
	if(it != m_WeaponMap.end())
	{
		WeaponPtr wp(new Weapon(_client, (*it).second.get()));
		return wp;
	}
	return WeaponPtr();
}

void WeaponDatabase::CopyAllWeapons(Client *_client, WeaponList &_list)
{
	WeaponMap::iterator it = m_WeaponMap.begin(), itEnd = m_WeaponMap.end();
	for(; it != itEnd; ++it)
	{
		WeaponPtr wp(new Weapon(_client, (*it).second.get()));
		_list.push_back(wp);
	}
}

String WeaponDatabase::GetWeaponName(int _weaponId)
{
	WeaponMap::const_iterator it = m_WeaponMap.find(_weaponId);
	if(it != m_WeaponMap.end())
	{
		return (*it).second->GetWeaponName();
	}
	return "";
}

WeaponPtr WeaponDatabase::GetWeapon(int _weaponId)
{
	WeaponMap::const_iterator it = m_WeaponMap.find(_weaponId);
	if(it != m_WeaponMap.end())
	{
		return (*it).second;
	}
	return WeaponPtr();
}

void WeaponDatabase::LoadDefaultWeapon()
{
	Weapon *weapon = new Weapon();
	gmGCRoot<gmUserObject> UserObj = weapon->GetScriptObject(ScriptManager::GetInstance()->GetMachine());
	gmVariable varThis(UserObj);

	int iThreadId;
	const filePath wpnDefault("scripts/weapons/weapon_defaults.gm");
	ScriptManager::GetInstance()->ExecuteFile(wpnDefault, iThreadId, &varThis);

	m_DefaultWeapon.reset(weapon);
}

void WeaponDatabase::LoadWeaponDefinitions(bool _clearall)
{
	if(_clearall)
		m_WeaponMap.clear();

	LoadDefaultWeapon();

	DirectoryList wpnFiles;
	FileSystem::FindAllFiles("scripts/weapons", wpnFiles, "weapon_.*\\.gm");

	LOG("Loading " << wpnFiles.size() << " weapon scripts from: scripts/weapons");
	DirectoryList::const_iterator cIt = wpnFiles.begin(), cItEnd = wpnFiles.end();
	for(; cIt != cItEnd; ++cIt)
	{
		// skip the default weapon script, we just use that for initializing other scripts.
		if((*cIt).filename() == "weapon_defaults.gm")
			continue;

		WeaponPtr wpn(new Weapon(0, m_DefaultWeapon.get()));

		//LOG("Loading Weapon Definition: " << (*cIt).string());

		filePath script( (*cIt).string().c_str() );
		if(wpn->InitScriptSource(script))
		{
			if(wpn->GetWeaponID() != 0 && wpn->GetWeaponNameHash())
			{
				RegisterWeapon(wpn->GetWeaponID(), wpn);
			}
		}
		else
		{
			LOGERR("Error Running Weapon Script: " << (*cIt).string());
			OBASSERT(0, "Error Running Weapon Script");
		}
	}
}

void WeaponDatabase::Unload()
{
	m_WeaponMap.clear();
	m_DefaultWeapon.reset();
}

void WeaponDatabase::ReloadScript(LiveUpdateKey _key)
{
	WeaponMap::iterator it = m_WeaponMap.begin(), itEnd = m_WeaponMap.end();
	for(; it != itEnd; ++it)
	{
		WeaponPtr wpn = (*it).second;
		if(wpn->GetLiveUpdateKey() == _key)
		{
			EngineFuncs::ConsoleMessage(va("File changed, reloading %s",wpn->GetScriptPath().c_str()));
			LOG("Re-Loading Weapon Definition: "<<wpn->GetScriptPath().c_str());
			
			LoadDefaultWeapon();

			WeaponPtr newwpn(new Weapon(0, m_DefaultWeapon.get()));

			if(newwpn->InitScriptSource(wpn->GetScriptPath()))
			{
				if(newwpn->GetWeaponID() != 0 && newwpn->GetWeaponNameHash())
				{
					(*it).second = newwpn;
					
					Event_RefreshWeapon d = { wpn->GetWeaponID() };
					MessageHelper evt(MESSAGE_REFRESHWEAPON, &d, sizeof(d));
					IGameManager::GetInstance()->GetGame()->DispatchGlobalEvent(evt);
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////

bool WeaponScriptResource::InitScriptSource(const filePath &_path)
{
	gmMachine * pMachine = ScriptManager::GetInstance()->GetMachine();

	int iThreadId;
	gmGCRoot<gmUserObject> UserObj = GetScriptObject(pMachine);
	gmVariable varThis(UserObj);

	gmTableObject * weaponTable = pMachine->GetGlobals()->Get(pMachine,"WEAPON").GetTableObjectSafe();
	gmTableObject * weaponTableOld = weaponTable ? pMachine->AllocTableObject() : 0;
	if(weaponTableOld)
	{
		weaponTable->CopyTo(pMachine,weaponTableOld);
	}

	const bool b = ScriptManager::GetInstance()->ExecuteFile( _path, iThreadId, &varThis );
	const bool c = ScriptResource::InitScriptSource(_path);

	if(b && c)
	{
		if(weaponTableOld)
		{
			gmTableIterator tIt;
			gmTableNode * pNode = weaponTable->GetFirst(tIt);
			while(pNode)
			{
				// if this entry didn't exist in the old table, we need to register it
				if(weaponTableOld->Get(pNode->m_key).IsNull())
				{
					const char * weaponName = pNode->m_key.GetCStringSafe(0);
					if(weaponName && pNode->m_value.IsInt())
					{
						if(IGameManager::GetInstance()->GetGame()->AddWeaponId(weaponName,pNode->m_value.GetInt()))
						{
							LOG("Adding new weapon enumeration: "<<weaponName<<"("<<pNode->m_value.GetInt()<<")");
						}
						else
						{
							LOG("Can't add new weapon enumeration: "<<weaponName<<"("<<pNode->m_value.GetInt()<<")");
						}
					}
				}
				pNode = weaponTable->GetNext(tIt);
			}
		}
		return true;
	}
	return false;
}
