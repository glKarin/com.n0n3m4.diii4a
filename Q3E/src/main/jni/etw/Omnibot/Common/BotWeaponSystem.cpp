#include "PrecompCommon.h"
#include "BotWeaponSystem.h"
#include "Client.h"
#include "WeaponDatabase.h"
#include "ScriptManager.h"
#include "BotBaseStates.h"

namespace AiState
{
	//////////////////////////////////////////////////////////////////////////
	class ReloadOther : public StateChild
	{
	public:
		void GetDebugString(StringStr &out)
		{
			out << g_WeaponDatabase.GetWeaponName(m_WeaponNeedsReloading);
		}
		obReal GetPriority()
		{
			FINDSTATE(tsys, TargetingSystem, GetParent()->GetParent());
			if(!tsys || tsys->HasTarget() || GetClient()->CheckUserFlag(Client::FL_USINGMOUNTEDWEAPON))
				return 0.f;

			FINDSTATE(wsys, WeaponSystem, GetParent());
			if(wsys)
			{
				int wpn = wsys->GetWeaponNeedingReload();
				if(wpn!=m_WeaponNeedsReloading)
				{
					if(wpn && m_WeaponNeedsReloading){
						wsys->UpdateWeaponRequest(GetNameHash(), wpn);	
					}
					m_WeaponNeedsReloading = wpn;
				}
			}
			return m_WeaponNeedsReloading ? 1.f : 0.f;
		}
		void Enter()
		{
			FINDSTATEIF(WeaponSystem, GetParent(), AddWeaponRequest(Priority::Low, GetNameHash(), m_WeaponNeedsReloading));
		}
		void Exit()
		{
			m_WeaponNeedsReloading = 0;
			FINDSTATEIF(WeaponSystem, GetParent(), ReleaseWeaponRequest(GetNameHash()));
		}
		StateStatus Update(float fDt)
		{
			Prof(ReloadOther);
			{
				Prof(Update);

				FINDSTATE(wsys, WeaponSystem, GetParent());
				if(wsys != NULL && wsys->GetCurrentRequestOwner() == GetNameHash())
				{
					if(wsys && wsys->CurrentWeaponIs(m_WeaponNeedsReloading))
						wsys->GetCurrentWeapon()->ReloadWeapon();
				}
				return State_Busy;
			}
		}
		ReloadOther() : StateChild("ReloadOther"), m_WeaponNeedsReloading(0) { }
	private:
		int		m_WeaponNeedsReloading;
	};

	//////////////////////////////////////////////////////////////////////////

	AttackTarget::AttackTarget() 
		: StateChild("AttackTarget")
		, m_AimPosition(Vector3f::ZERO)
		, m_CurrentWeaponHash(0)
		, m_ShootTheBastard(0)
		, m_TargetExceedsWeaponLimits(0)
	{
	}
	void AttackTarget::RenderDebug()
	{
		if(m_WeaponLimits.m_Limited==True)
		{
			Vector3f vGunCenter(m_WeaponLimits.m_CenterFacing);
			Vector3f vAimVector(Normalize(m_AimPosition-GetClient()->GetEyePosition()));

			const float fGunHeading = vGunCenter.XYHeading();
			const float fAimHeading = vAimVector.XYHeading();

			const float fGunPitch = vGunCenter.GetPitch();
			const float fAimPitch = vAimVector.GetPitch();

			float fHeadingDiff = Mathf::RadToDeg(Mathf::UnitCircleNormalize(fAimHeading-fGunHeading));
			float fPitchDiff = Mathf::RadToDeg(fAimPitch-fGunPitch);
			if(fHeadingDiff < m_WeaponLimits.m_MinYaw || fHeadingDiff > m_WeaponLimits.m_MaxYaw)
				m_TargetExceedsWeaponLimits = true;
			if(fPitchDiff < m_WeaponLimits.m_MinPitch || fPitchDiff > m_WeaponLimits.m_MaxPitch)
				m_TargetExceedsWeaponLimits = true;

			Quaternionf ql, qr;
			ql.FromAxisAngle(Vector3f::UNIT_Z, Mathf::DegToRad(m_WeaponLimits.m_MinYaw));
			qr.FromAxisAngle(Vector3f::UNIT_Z, Mathf::DegToRad(m_WeaponLimits.m_MaxYaw));
			Vector3f vLeft = ql.Rotate(vGunCenter);
			Vector3f vRght = qr.Rotate(vGunCenter);

			Vector3f vEye = GetClient()->GetEyePosition();
			Utils::DrawLine(vEye,vEye+vGunCenter*64.f,COLOR::ORANGE,0.1f);
			Utils::DrawLine(vEye,vEye+vLeft*64.f,COLOR::RED,0.1f);
			Utils::DrawLine(vEye,vEye+vRght*64.f,COLOR::RED,0.1f);
		}
	}
	void AttackTarget::GetDebugString(StringStr &out)
	{
		out << Utils::HashToString(m_CurrentWeaponHash);
	}
	obReal AttackTarget::GetPriority()
	{
		return GetClient()->GetTargetingSystem()->HasTarget() ? 1.f : 0.f;
	}
	void AttackTarget::Enter()
	{
		//FINDSTATEIF(Aimer, GetRootState(), AddAimRequest(Priority::LowMed, this, GetNameHash()));
	}
	void AttackTarget::Exit()
	{
		m_ShootTheBastard = false;
		m_CurrentWeaponHash = 0;
		FINDSTATEIF(Aimer, GetRootState(), ReleaseAimRequest(GetNameHash()));
		FINDSTATEIF(WeaponSystem, GetParent(), ReleaseWeaponRequest(GetNameHash()));
	}
	State::StateStatus AttackTarget::Update(float fDt)
	{
		Prof(AttackTarget);
		{
			Prof(Update);

			const MemoryRecord *pRecord = GetClient()->GetTargetingSystem()->GetCurrentTargetRecord();
			if(!pRecord)
				return State_Finished;

			// Add the aim request when reaction time has been met
			FINDSTATE(wsys, WeaponSystem, GetParent());
			if( wsys != NULL && 
				(pRecord->GetTimeTargetHasBeenVisible() >= wsys->GetReactionTime()) &&
				(pRecord->IsShootable() || (pRecord->GetTimeHasBeenOutOfView() < wsys->GetAimPersistance())))
			{
				FINDSTATEIF(Aimer, GetRootState(), AddAimRequest(Priority::LowMed, this, GetNameHash()));
			}

			return State_Busy;
		}
	}
	bool AttackTarget::GetAimPosition(Vector3f &_aimpos)
	{
		const MemoryRecord *pRecord = GetClient()->GetTargetingSystem()->GetCurrentTargetRecord();
		if(!pRecord)
		{
			m_ShootTheBastard = false;
			return false;
		}

		m_ShootTheBastard = pRecord->IsShootable();

		const GameEntity vTargetEnt = pRecord->GetEntity();
		const TargetInfo &targetInfo = pRecord->m_TargetInfo;

		FINDSTATE(wsys, WeaponSystem, GetParent());
		if(wsys)
		{
			WeaponPtr wpn = wsys->GetCurrentWeapon();
			if(wpn)
			{
				m_CurrentWeaponHash = wpn->GetWeaponNameHash();

				m_FireMode = wpn->GetBestFireMode(targetInfo);
				if(m_FireMode == InvalidFireMode){
					m_FireMode = Primary;
					m_ShootTheBastard = false;
				}

				// Calculate the position the bot will aim at, the weapon itself should account
				// for any leading that may need to take place 
				m_AimPosition = wpn->GetAimPoint(m_FireMode, vTargetEnt, targetInfo);
				wpn->AddAimError(m_FireMode, m_AimPosition, targetInfo);
				_aimpos = m_AimPosition;

				// Check limits
				m_TargetExceedsWeaponLimits = false;
				if(InterfaceFuncs::GetWeaponLimits(GetClient(), wpn->GetWeaponID(), m_WeaponLimits)
					&& m_WeaponLimits.m_Limited==True)
				{
					Vector3f vGunCenter(m_WeaponLimits.m_CenterFacing);
					Vector3f vAimVector(Normalize(_aimpos-GetClient()->GetEyePosition()));

					const float fGunHeading = vGunCenter.XYHeading();
					const float fAimHeading = vAimVector.XYHeading();

					const float fGunPitch = vGunCenter.GetPitch();
					const float fAimPitch = vAimVector.GetPitch();

					float fHeadingDiff = Mathf::RadToDeg(Mathf::UnitCircleNormalize(fAimHeading-fGunHeading));
					float fPitchDiff = Mathf::RadToDeg(fAimPitch-fGunPitch);
					if(fHeadingDiff < m_WeaponLimits.m_MinYaw || fHeadingDiff > m_WeaponLimits.m_MaxYaw)
						m_TargetExceedsWeaponLimits = true;
					if(fPitchDiff < m_WeaponLimits.m_MinPitch || fPitchDiff > m_WeaponLimits.m_MaxPitch)
						m_TargetExceedsWeaponLimits = true;
				}
				return true;
			}
		}
		_aimpos = m_AimPosition = pRecord->GetLastSensedPosition();
		return false;
	}
	void AttackTarget::OnTarget()
	{
		if(!m_ShootTheBastard)
			return;

		FINDSTATE(wsys, WeaponSystem, GetParent());
		if(wsys != NULL && wsys->CurrentWeaponIsAttackReady())
		{
			WeaponPtr wpn = wsys->GetCurrentWeapon();
			if(wpn && !GetClient()->CheckUserFlag(Client::FL_SHOOTINGDISABLED))
			{
				if(wsys->ReadyToFire())
				{
					wpn->PreShoot(m_FireMode);
					wpn->Shoot(m_FireMode);
				}
				else
				{
					wpn->StopShooting(m_FireMode);
				}
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	WeaponSystem::WeaponRequest::WeaponRequest()
	{
		Reset();
	}

	void WeaponSystem::WeaponRequest::Reset()
	{
		m_Priority = Priority::Zero;
		m_Owner = 0;
		m_WeaponId = 0;
	}

	WeaponSystem::WeaponSystem() 
		: StateFirstAvailable("WeaponSystem")
		, m_ReactionTimeInMS	(0)
		, m_AimPersistance		(2000)
		, m_DesiredWeaponID		(0)
		, m_DefaultWeapon		(0)
		, m_OverrideWeapon		(0)
		, m_CurrentRequestOwner	(0)
	{
		//AppendState(new AttackTargetOffhand);
		AppendState(new AttackTarget);
		//AppendState(new ChooseMountedWeapon);
		AppendState(new ReloadOther);
	}

	WeaponSystem::~WeaponSystem()
	{
	}

	void WeaponSystem::GrabAllWeapons()
	{
		g_WeaponDatabase.CopyAllWeapons(GetClient(), m_AllWeaponList);
	}

	void WeaponSystem::RefreshWeapon(int _weaponId)
	{
		const bool HasWeapon = m_WeaponMask.CheckFlag(_weaponId);

		RemoveWeapon(_weaponId);

		// update our inventory
		for(WeaponList::iterator it = m_AllWeaponList.begin(); it != m_AllWeaponList.end(); ++it)
		{
			if((*it)->GetWeaponID() == _weaponId)
			{
				WeaponPtr wpn = g_WeaponDatabase.CopyWeapon(GetClient(), _weaponId);
				OBASSERT(wpn,"Unknown Weapon!");
				(*it) = wpn;
			}
		}

		if(HasWeapon)
			AddWeaponToInventory(_weaponId);
	}

	void WeaponSystem::RefreshAllWeapons()
	{
		const BitFlag128 HasWeapons = m_WeaponMask;

		ClearWeapons();

		m_AllWeaponList.clear();
		g_WeaponDatabase.CopyAllWeapons(GetClient(), m_AllWeaponList);

		for(int i = 0; i < 128; ++i)
		{
			if(HasWeapons.CheckFlag(i))
				AddWeaponToInventory(i);
		}
	}

	bool WeaponSystem::AddWeaponToInventory(int _weaponId)
	{
		WeaponList::iterator it = m_AllWeaponList.begin();
		for( ; it != m_AllWeaponList.end(); ++it)
		{
			if((*it)->GetWeaponID() == _weaponId)
			{
				AddWeapon(*it);
				return true;
			}
		}
		return false;
	}

	void WeaponSystem::AddWeapon(WeaponPtr _weapon)
	{
		OBASSERT(!HasWeapon(_weapon->GetWeaponID()), "Already Have Weapon %s!", _weapon->GetWeaponName().c_str());
		if(!HasWeapon(_weapon->GetWeaponID()))
		{
			m_WeaponList.push_back(_weapon);
			m_WeaponMask.SetFlag(_weapon->GetWeaponID(), true);
		}
	}

	void WeaponSystem::RemoveWeapon(int _weaponId)
	{
		// clear the current weapon if we're removing it.
		if(m_CurrentWeapon && m_CurrentWeapon->GetWeaponID() == _weaponId)
			m_CurrentWeapon.reset();

		WeaponList::iterator it = m_WeaponList.begin();
		for( ; it != m_WeaponList.end(); )
		{
			if((*it)->GetWeaponID() == _weaponId)
			{
				m_WeaponList.erase(it++);
				m_WeaponMask.SetFlag(_weaponId, false);
			}
			else
				++it;
		}
	}

	void WeaponSystem::ClearWeapons()
	{
		// Clear all the weapons
		m_CurrentWeapon.reset();
		m_WeaponList.clear();
		m_WeaponMask.ClearAll();
	}

	void WeaponSystem::ChargeWeapon(FireMode _mode)
	{
		if(m_CurrentWeapon)
			m_CurrentWeapon->ChargeWeapon(_mode);
	}

	void WeaponSystem::ZoomWeapon(FireMode _mode)
	{
		if(m_CurrentWeapon)
			m_CurrentWeapon->ZoomWeapon(_mode);
	}

	void WeaponSystem::FireWeapon(FireMode _mode)
	{
		if(m_CurrentWeapon)
			m_CurrentWeapon->Shoot(_mode);
	}

	WeaponPtr WeaponSystem::GetWeapon(int _weaponId, bool _inventory) const
	{
		WeaponPtr ret;

		WeaponList::const_iterator it, itEnd;
		if(_inventory)
		{
			it = m_WeaponList.begin();
			itEnd = m_WeaponList.end();
		}
		else
		{
			it = m_AllWeaponList.begin();
			itEnd = m_AllWeaponList.end();
		}

		for( ; it != itEnd; ++it)
		{
			if((*it)->GetWeaponID() == _weaponId)
			{
				ret = (*it);
				break;
			}
		}
		return ret;
	}

	WeaponPtr WeaponSystem::GetWeaponByIndex(int _index, bool _inventory)
	{
		WeaponPtr ret;

		WeaponList::const_iterator it, itEnd;
		if(_inventory)
		{
			it = m_WeaponList.begin();
			itEnd = m_WeaponList.end();
		}
		else
		{
			it = m_AllWeaponList.begin();
			itEnd = m_AllWeaponList.end();
		}

		std::advance(it, _index);
		return *it;
	}

	bool WeaponSystem::HasWeapon(int _weaponId) const
	{
		return (bool)GetWeapon(_weaponId);
	}

	bool WeaponSystem::HasAmmo(FireMode _mode) const
	{
		return m_CurrentWeapon && m_CurrentWeapon->GetFireMode(_mode).IsDefined() ? 
			m_CurrentWeapon->GetFireMode(_mode).HasAmmo() : false;
	}

	bool WeaponSystem::HasAmmo(int _weaponid, FireMode _mode, int _amount) const
	{
		WeaponList::const_iterator it = m_WeaponList.begin(), itEnd = m_WeaponList.end();
		for( ; it != itEnd; ++it)
		{
			if((*it)->GetWeaponID() == _weaponid)
			{
				return (*it)->GetFireMode(_mode).IsDefined() ? (*it)->GetFireMode(_mode).HasAmmo(_amount) : false;
			}
		}
		return false;
	}

	bool WeaponSystem::CanShoot(const MemoryRecord &_record)
	{
		Prof(CanShoot);

		// Target filters use this, so if we're mounted only check current weapon(which should be the mounted weapon)
		if(m_CurrentWeapon && GetClient()->CheckUserFlag(Client::FL_USINGMOUNTEDWEAPON))
			return m_CurrentWeapon->CanShoot(Primary, _record.m_TargetInfo);

		WeaponList::const_iterator it = m_WeaponList.begin(), itEnd = m_WeaponList.end();
		for( ; it != itEnd; ++it)
		{
			if((*it)->CanShoot(Primary, _record.m_TargetInfo))
				return true;
		}
		return false;
	}

	WeaponStatus WeaponSystem::_UpdateWeaponFromGame()
	{
		return InterfaceFuncs::GetEquippedWeapon(GetClient()->GetGameEntity());	
	}

	void WeaponSystem::_UpdateCurrentWeapon(FireMode _mode)
	{
		// Get the weapon from the game.
		WeaponStatus mounted = InterfaceFuncs::GetMountedWeapon(GetClient());
		if(mounted.m_WeaponId != 0)
		{
			GetClient()->SetUserFlag(Client::FL_USINGMOUNTEDWEAPON, true);
			m_CurrentWeapon = GetWeapon(mounted.m_WeaponId, false);
			OBASSERT(m_CurrentWeapon, "Unknown Mountable Weapon: %d", mounted.m_WeaponId);
			m_DesiredWeaponID = mounted.m_WeaponId;
			m_CurrentRequestOwner = GetNameHash();
		}
		else
		{
			GetClient()->SetUserFlag(Client::FL_USINGMOUNTEDWEAPON, false);
			WeaponStatus currentWeapon = _UpdateWeaponFromGame();

			// If it has changed, set our new weapon.
			if(!m_CurrentWeapon || !m_CurrentWeapon->IsWeapon(currentWeapon.m_WeaponId))
			{
				//bool bFound = false;
				WeaponList::const_iterator it = m_WeaponList.begin(), itEnd = m_WeaponList.end();
				for( ; it != itEnd; ++it)
				{
					if((*it)->IsWeapon(currentWeapon.m_WeaponId))
					{
						m_CurrentWeapon = (*it);
						m_CurrentWeapon->Select();
						//bFound = true;

						Event_WeaponChanged weapChanged = { currentWeapon.m_WeaponId };
						MessageHelper hlpr(ACTION_WEAPON_CHANGE, &weapChanged, sizeof(weapChanged));
						GetClient()->SendEvent(hlpr);
						break;
					}
				}
				//OBASSERT(bFound, va("Unknown Weapon: %d", currentWeapon.m_WeaponId));
			}
		}

		if(m_CurrentWeapon)
		{
			m_CurrentWeapon->Update(_mode);
			m_CurrentWeapon->UpdateClipAmmo(Primary);
			m_CurrentWeapon->UpdateClipAmmo(Secondary);
		}
	}

	void WeaponSystem::UpdateAllWeaponAmmo()
	{
		// Update the primary weapons.
		WeaponList::iterator it = m_WeaponList.begin();
		WeaponList::iterator itEnd = m_WeaponList.end();
		for( ; it != itEnd; ++it)
		{
			(*it)->UpdateAmmo(Primary);
		}
	}

	bool WeaponSystem::ReadyToFire()
	{
		return InterfaceFuncs::IsReadyToFire(GetClient()->GetGameEntity());
	}

	bool WeaponSystem::IsReloading()
	{
		return InterfaceFuncs::IsReloading(GetClient()->GetGameEntity());
	}

	obReal WeaponSystem::GetMostDesiredAmmo(int &_weapon, int &_getammo)
	{
		obReal fMostDesirable = 0.0f;
		int iMostDesirableAmmo = 0;
		int iGetAmmoAmount = 1;

		// Get the most desirable ammo currently.
		WeaponList::iterator it = m_WeaponList.begin();
		WeaponList::iterator itEnd = m_WeaponList.end();
		for( ; it != itEnd; ++it)
		{
			(*it)->UpdateAmmo(Primary);

			int iThisWeaponType = 0;
			int iThisGetAmmo = 1;
			obReal dDesirability = (*it)->LowOnAmmoPriority(Primary, iThisWeaponType, iThisGetAmmo);
			if(dDesirability > fMostDesirable)
			{
				fMostDesirable = dDesirability;
				iMostDesirableAmmo = iThisWeaponType;
				iGetAmmoAmount = iThisGetAmmo;
			}
		}

		// Store the best results.
		_weapon = iMostDesirableAmmo;
		_getammo = iGetAmmoAmount;
		return fMostDesirable;
	}

	void WeaponSystem::GetSpectateMessage(StringStr &_outstring)
	{
		if(m_CurrentWeapon)
			m_CurrentWeapon->GetSpectateMessage(_outstring);

		String desired = g_WeaponDatabase.GetWeaponName(m_DesiredWeaponID);
		_outstring << " Desired: " << desired.c_str() << " ";
	}

	//////////////////////////////////////////////////////////////////////////

	void WeaponSystem::GetDebugString(StringStr &out)
	{
		out << 
			Utils::HashToString(m_CurrentRequestOwner) << 
			" : " <<
			g_WeaponDatabase.GetWeaponName(m_DesiredWeaponID);
	}

	obReal WeaponSystem::GetPriority()
	{
		return 1.f;
	}

	void WeaponSystem::Enter()
	{
		Prof(WeaponSystem);

		m_DefaultWeapon = SelectBestWeapon();
		AddWeaponRequest(Priority::Idle, GetNameHash(), m_DefaultWeapon);
	}

	void WeaponSystem::Exit()
	{
		ReleaseWeaponRequest(GetNameHash());
	}

	void WeaponSystem::Initialize()
	{
		GrabAllWeapons();
	}

	State::StateStatus WeaponSystem::Update(float fDt)
	{
		Prof(WeaponSystem);
		{
			Prof(Update);

			//////////////////////////////////////////////////////////////////////////
			m_DefaultWeapon = SelectBestWeapon();
			UpdateWeaponRequest(GetNameHash(), m_DefaultWeapon);
			//////////////////////////////////////////////////////////////////////////

			// Update the preferred weapon.
			const WeaponRequest &bestWpn = GetHighestWeaponRequest();
			m_DesiredWeaponID = bestWpn.m_WeaponId;
			m_CurrentRequestOwner = bestWpn.m_Owner;

			_UpdateCurrentWeapon(Primary);
		}
		return State_Busy;
	}

	bool WeaponSystem::AddWeaponRequest(Priority::ePriority _prio, obuint32 _owner, int _weaponId)
	{
		int iOpen = -1;
		for(obint32 i = 0; i < MaxWeaponRequests; ++i)
		{
			if(m_WeaponRequests[i].m_Owner == _owner)
			{
				iOpen = i;
				break;
			}
			if(m_WeaponRequests[i].m_Priority == Priority::Zero)
			{
				if(iOpen == -1)
					iOpen = i;
			}
		}

		if(iOpen != -1)
		{
			m_WeaponRequests[iOpen].m_Priority = _prio;
			m_WeaponRequests[iOpen].m_Owner = _owner;
			m_WeaponRequests[iOpen].m_WeaponId = _weaponId;
			return true;
		}
		return false;
	}

	void WeaponSystem::ReleaseWeaponRequest(obuint32 _owner)
	{
		for(obint32 i = 0; i < MaxWeaponRequests; ++i)
		{
			if(m_WeaponRequests[i].m_Owner == _owner)
			{
				m_WeaponRequests[i].Reset();
				break;
			}
		}
	}

	bool WeaponSystem::UpdateWeaponRequest(obuint32 _owner, int _weaponId)
	{
		for(obint32 i = 0; i < MaxWeaponRequests; ++i)
		{
			if(m_WeaponRequests[i].m_Owner == _owner)
			{
				m_WeaponRequests[i].m_WeaponId = _weaponId;
				return true;
			}
		}
		return false;
	}

	const WeaponSystem::WeaponRequest &WeaponSystem::GetHighestWeaponRequest()
	{
		int iBestWpn = 0;
		for(obint32 i = 1; i < MaxWeaponRequests; ++i)
		{
			if(m_WeaponRequests[i].m_Priority > m_WeaponRequests[iBestWpn].m_Priority)
			{
				iBestWpn = i;
			}
		}
		return m_WeaponRequests[iBestWpn];
	}

	int WeaponSystem::GetWeaponNeedingReload()
	{
		// Look for other weapons that need reloading.
		WeaponList::const_iterator it = m_WeaponList.begin(), itEnd = m_WeaponList.end();
		for( ; it != itEnd; ++it)
		{
			// Magik: Added enough ammo check
			FireMode m = (*it)->CanReload();
			if(m != InvalidFireMode)
				return (*it)->GetWeaponID();
		}
		return 0;
	}

	int WeaponSystem::SelectBestWeapon(GameEntity _targetent /*= NULL*/)
	{
		Prof(SelectBestWeaponNew);

		UpdateAllWeaponAmmo();

		int iBestWeaponID = 0;

		// If an override isn't provided, use the current target.
		if(!_targetent.IsValid())
			_targetent = GetClient()->GetTargetingSystem()->GetCurrentTarget();

		// Evaluate weapons with target data.
		if(_targetent.IsValid())
		{
			const TargetInfo *pTargetInfo = GetClient()->GetSensoryMemory()->GetTargetInfo(_targetent);

			if(!pTargetInfo)
				return iBestWeaponID;

			obReal fBestDesirability = 0.0;

			// Evaluate my primary weapons.
			WeaponList::const_iterator it = m_WeaponList.begin(), itEnd = m_WeaponList.end();
			for( ; it != itEnd; ++it)
			{
				obReal fDesirability = (*it)->CalculateDesirability(*pTargetInfo);
				if(fDesirability > fBestDesirability)
				{
					fBestDesirability = fDesirability;
					iBestWeaponID = (*it)->GetWeaponID();
				}
			}
		} 
		else
		{
			// Are we idly holding a weapon?
			obReal fBestIdleDesirability = 0.0;

			// Does the current weapon need reloading?
			/*FireMode m = m_CurrentWeapon ? m_CurrentWeapon->CanReload() : InvalidFireMode;
			if(m != InvalidFireMode)
			{
				m_CurrentWeapon->ReloadWeapon(m);
			} 
			else*/
			{
				// Look for other weapons that need reloading.
				WeaponList::const_iterator it = m_WeaponList.begin(), itEnd = m_WeaponList.end();
				for( ; it != itEnd; ++it)
				{
					// Get the best idle weapon.
					obReal fBestDesirability = (*it)->CalculateDefaultDesirability();
					if(fBestDesirability > fBestIdleDesirability)
					{
						fBestIdleDesirability = fBestDesirability;
						iBestWeaponID = (*it)->GetWeaponID();
					}
				}
			}
		}
		return iBestWeaponID;
	}

	int WeaponSystem::SelectRandomWeapon()
	{
		int WeaponIds[64] = {};
		int NumWeaponIds = 0;

		WeaponList::const_iterator it = m_WeaponList.begin(), itEnd = m_WeaponList.end();
		for( ; it != itEnd; ++it)
		{
			WeaponIds[NumWeaponIds++] = (*it)->GetWeaponID();
		}		
		return NumWeaponIds ? (WeaponIds[rand() % NumWeaponIds]) : 0;
	}
	
}
