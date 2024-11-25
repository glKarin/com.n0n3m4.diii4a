#include "PrecompCommon.h"
#include "Weapon.h"
#include "gmWeapon.h"
#include "ScriptManager.h"

//////////////////////////////////////////////////////////////////////////

Weapon::WeaponFireMode::WeaponFireMode() 
	: m_WeaponType			(None)
	, m_WeaponFlags			(0)
	, m_ShootButton			(BOT_BUTTON_ATTACK1)
	, m_ZoomButton			(BOT_BUTTON_AIM)
	, m_ProjectileSpeed		(0.f)
	, m_ProjectileGravity	(0.f)
	, m_MinChargeTime		(0)
	, m_MaxChargeTime		(0)
	, m_MinLeadError		(0.f)
	, m_MaxLeadError		(0.f)
	, m_DelayAfterFiring	(0)
	, m_FuseTime			(0.f)
	, m_SplashRadius		(0.f)
	, m_PitchOffset			(0.f)
	, m_LowAmmoThreshold	(0)
	, m_LowAmmoPriority		(0.f)
	, m_LowAmmoGetAmmoAmount(1)
	, m_HeatController		(1.f, 0.4f, 0.4f)
	, m_CurrentBurstWindow	(0)
	, m_AmmoCurrent			(0)
	, m_AmmoMax				(0)
	, m_ClipCurrent			(0)
	, m_ClipMax				(0)
	, m_AimOffset			(Vector3f::ZERO)
	, m_AimOffsetZ			(0)
	, m_AimErrorMax			(Vector2f::ZERO)
	, m_AimErrorCurrent		(Vector3f::ZERO)
	, m_MinAimAdjustmentSecs	(0.0f)
	, m_MaxAimAdjustmentSecs	(2.0f)
	, m_NextAimAdjustmentTime	(0)
	, m_LastDesirability	(-1.0f)
	, m_DefaultDesirability	(0.0f)
	, m_WeaponBias			(1.0f)
	, m_ChargeTime			(0)
	, m_DelayChooseTime		(0)
	, m_BurstTime			(0)
	, m_BurstRound			(0)
	, m_LastAmmoUpdate(0)
	, m_LastClipAmmoUpdate(-1)
{
	// Initialize Default Properties
	SetFlag(Waterproof, true);
	SetFlag(RequiresAmmo, true);
	
	for(int i = 0; i < MaxDesirabilities; ++i)
	{
		m_Desirabilities[i].m_MinRange = 0.f;
		m_Desirabilities[i].m_MaxRange = 0.f;
		m_Desirabilities[i].m_Desirability = 0.f;
	}
	for(int i = 0; i < MaxBurstWindows; ++i)
	{
		m_BurstWindows[i].m_BurstRounds = 0;
		m_BurstWindows[i].m_MinBurstDelay = 0.f;
		m_BurstWindows[i].m_MaxBurstDelay = 0.f;
		m_BurstWindows[i].m_MinRange = 0.f;
		m_BurstWindows[i].m_MaxRange = 0.f;
	}

	fl.Charging = false;
}

Weapon::WeaponFireMode::~WeaponFireMode()
{
	if(m_ScriptObject)
	{
		gmBind2::Class<WeaponFireMode>::NullifyUserObject(m_ScriptObject);
		m_ScriptObject = 0; //NULL;
	}
}

Weapon::WeaponFireMode &Weapon::WeaponFireMode::operator=(const WeaponFireMode &_rh)
{
	m_WeaponType = _rh.m_WeaponType;
	m_WeaponFlags = _rh.m_WeaponFlags;
	m_ShootButton = _rh.m_ShootButton;
	m_ZoomButton = _rh.m_ZoomButton;
	m_ProjectileSpeed = _rh.m_ProjectileSpeed;
	m_ProjectileGravity = _rh.m_ProjectileGravity;
	m_MinChargeTime = _rh.m_MinChargeTime;
	m_MaxChargeTime = _rh.m_MaxChargeTime;
	m_DelayAfterFiring = _rh.m_DelayAfterFiring;
	m_FuseTime = _rh.m_FuseTime;
	m_SplashRadius = _rh.m_SplashRadius;

	m_LowAmmoThreshold = _rh.m_LowAmmoThreshold;
	m_LowAmmoPriority = _rh.m_LowAmmoPriority;
	m_LowAmmoGetAmmoAmount = _rh.m_LowAmmoGetAmmoAmount;

	for(int i = 0; i < MaxBurstWindows; ++i)
		m_BurstWindows[i] = _rh.m_BurstWindows[i];

	/*for(int i = 0; i < NumStances; ++i)
		m_StancePreference[i] = _rh.m_StancePreference[i];*/

	m_PitchOffset = _rh.m_PitchOffset;
	
	m_MinLeadError = _rh.m_MinLeadError;
	m_MaxLeadError = _rh.m_MaxLeadError;

	m_HeatController = _rh.m_HeatController;
	m_TargetBias = _rh.m_TargetBias;
	m_TargetEntFlagIgnore = _rh.m_TargetEntFlagIgnore;

	for(int i = 0; i < MaxDesirabilities; ++i)
		m_Desirabilities[i] = _rh.m_Desirabilities[i];

	m_AmmoCurrent = _rh.m_AmmoCurrent;
	m_AmmoMax = _rh.m_AmmoMax;
	m_ClipCurrent = _rh.m_ClipCurrent;
	m_ClipMax = _rh.m_ClipMax;
	m_AimOffset = _rh.m_AimOffset;
	m_AimOffsetZ = _rh.m_AimOffsetZ;
	m_AimErrorMax = _rh.m_AimErrorMax;
	m_AimErrorCurrent = _rh.m_AimErrorCurrent;
	m_MinAimAdjustmentSecs = _rh.m_MinAimAdjustmentSecs;
	m_MaxAimAdjustmentSecs = _rh.m_MaxAimAdjustmentSecs;
	m_NextAimAdjustmentTime = _rh.m_NextAimAdjustmentTime;
	m_LastDesirability = _rh.m_LastDesirability;
	m_DefaultDesirability = _rh.m_DefaultDesirability;
	m_WeaponBias = _rh.m_WeaponBias;
	
	m_scrCalcDefDesir = _rh.m_scrCalcDefDesir;
	m_scrCalcDesir = _rh.m_scrCalcDesir;
	m_scrCalcAimPoint = _rh.m_scrCalcAimPoint;

	// duplicate the script table
	gmMachine *pM = ScriptManager::GetInstance()->GetMachine();
	gmBind2::Class<WeaponFireMode>::CloneTable(pM,_rh.GetScriptObject(pM),GetScriptObject(pM));

	return *this;
}

bool Weapon::WeaponFireMode::HasAmmo(int _amount) const
{
	if(CheckFlag(RequiresAmmo))
	{
		if(_amount>0)
			return (m_AmmoCurrent >= _amount) || (m_ClipCurrent >= _amount);
		return (m_AmmoCurrent > 0) || (m_ClipCurrent > 0);
	}	
	return true;
}

bool Weapon::WeaponFireMode::NeedsAmmo() const
{
	return CheckFlag(RequiresAmmo) ? (m_AmmoMax > 0) : false;
}

bool Weapon::WeaponFireMode::FullClip() const
{
	// Only if this is a clipped weapon
	return (m_ClipMax > 0) ? (m_ClipCurrent >= m_ClipMax) : true;
}

bool Weapon::WeaponFireMode::EmptyClip() const
{
	return UsesClip() ? (m_ClipCurrent == 0) : (m_AmmoCurrent == 0);
}

bool Weapon::WeaponFireMode::UsesClip() const
{
	return CheckFlag(HasClip);
}

bool Weapon::WeaponFireMode::EnoughAmmoToReload() const
{
	return (m_AmmoCurrent > m_ClipCurrent);
}

obReal Weapon::WeaponFireMode::CalculateDefaultDesirability(Client *_bot)
{
	//////////////////////////////////////////////////////////////////////////
	// Script callback if necessary.
	if(m_scrCalcDefDesir)
	{
		gmMachine *pMachine = ScriptManager::GetInstance()->GetMachine();
		gmCall call;
		if(call.BeginFunction(pMachine, m_scrCalcDefDesir, gmVariable(GetScriptObject(pMachine))))
		{
			OBASSERT(_bot->GetScriptObject(), "Invalid Script Object For Bot!");
			call.AddParamUser(GetScriptObject(pMachine));
			call.AddParamUser(_bot->GetScriptObject());
			call.End();

			if(!call.GetReturnedFloat(m_DefaultDesirability))
			{
				Utils::OutputDebug(kError, "Invalid Return Value From CalculateDefaultDesirability");				
			}
		}
		else
		{
			Utils::OutputDebug(kError, "Invalid Weapon CalculateDefaultDesirability Callback!");
		}
	}
	//////////////////////////////////////////////////////////////////////////
	return m_DefaultDesirability * m_WeaponBias;
}

obReal Weapon::WeaponFireMode::CalculateDesirability(Client *_bot, const TargetInfo &_targetinfo)
{
	m_LastDesirability = m_DefaultDesirability;
	//////////////////////////////////////////////////////////////////////////
	// Script callback if necessary.
	if(m_scrCalcDesir)
	{
		gmMachine *pMachine = ScriptManager::GetInstance()->GetMachine();

		gmCall call;
		if(call.BeginFunction(pMachine, m_scrCalcDesir, gmVariable(GetScriptObject(pMachine))))
		{
			OBASSERT(_bot->GetScriptObject(), "Invalid Script Object For Bot!");
			//call.AddParamUser(GetScriptObject(pMachine));
			call.AddParamUser(_bot->GetScriptObject());
			call.AddParamUser(_targetinfo.GetScriptObject(pMachine));
			call.End();

			if(!call.GetReturnedFloat(m_LastDesirability))
			{
				Utils::OutputDebug(kError, "Invalid Return Value From CalculateDesirability");				
			}
			m_LastDesirability *= m_WeaponBias;
			return m_LastDesirability;	
		}
		else
		{
			Utils::OutputDebug(kError, "Invalid Weapon CalculateDesirability Callback!");
		}
	}
	//////////////////////////////////////////////////////////////////////////

	obReal fTargetBias = GetTargetBias(_targetinfo.m_EntityClass, _targetinfo.m_EntityFlags);
	if(fTargetBias == 0.f)
	{
		m_LastDesirability = 0.0;
		return m_LastDesirability;
	}

	obReal fBestDesirability = -1;
	for(int i = 0; i < MaxDesirabilities; ++i)
	{
		if(m_Desirabilities[i].m_MaxRange != 0.f)
		{
			if(InRangeT(_targetinfo.m_DistanceTo, m_Desirabilities[i].m_MinRange, m_Desirabilities[i].m_MaxRange))
			{
				if(m_Desirabilities[i].m_Desirability > fBestDesirability)
					fBestDesirability = m_Desirabilities[i].m_Desirability;
			}
		}
	}
	if(fBestDesirability >= 0){
		m_LastDesirability = fBestDesirability * fTargetBias;
	}

	m_LastDesirability *= m_WeaponBias;
	return m_LastDesirability;	
}

void Weapon::WeaponFireMode::UpdateBurstWindow(const TargetInfo *_targetinfo)
{
	if(_targetinfo)
	{
		for(int i = 0; i < MaxBurstWindows; ++i)
		{
			if(m_BurstWindows[i].m_BurstRounds > 0)
			{
				if(InRangeT(_targetinfo->m_DistanceTo, m_BurstWindows[i].m_MinRange, m_BurstWindows[i].m_MaxRange))
				{
					m_CurrentBurstWindow = i;
					break;
				}
			}
		}
	}
}

//obReal Weapon::WeaponFireMode::CalculateAmmoDesirability(int &_ammotype) 
//{
//	// TODO? OR DELETE
//	return 0.0; 
//}

bool Weapon::WeaponFireMode::SetDesirabilityWindow(float _minrange, float _maxrange, float _desir)
{
	for(int i = 0; i < MaxDesirabilities; ++i)
	{
		if(m_Desirabilities[i].m_MinRange == _minrange &&
			m_Desirabilities[i].m_MaxRange == _maxrange)
		{
			m_Desirabilities[i].m_MinRange = _minrange;
			m_Desirabilities[i].m_MaxRange = _maxrange;
			m_Desirabilities[i].m_Desirability = _desir;
			return true;
		}
	}

	for(int i = 0; i < MaxDesirabilities; ++i)
	{
		if(m_Desirabilities[i].m_MaxRange == 0.f) // maxrange 0 determines un-used slots
		{
			m_Desirabilities[i].m_MinRange = _minrange;
			m_Desirabilities[i].m_MaxRange = _maxrange;
			m_Desirabilities[i].m_Desirability = _desir;
			return true;
		}
	}
	return false;
}

bool Weapon::WeaponFireMode::SetBurstWindow(float _minrange, float _maxrange, int _burst, float _mindelay, float _maxdelay)
{
	for(int i = 0; i < MaxBurstWindows; ++i)
	{
		if(m_BurstWindows[i].m_MinRange == _minrange &&
			m_BurstWindows[i].m_MaxRange == _maxrange)
		{
			m_BurstWindows[i].m_MinRange = _minrange;
			m_BurstWindows[i].m_MaxRange = _maxrange;
			m_BurstWindows[i].m_BurstRounds = _burst;
			m_BurstWindows[i].m_MinBurstDelay = _mindelay;
			m_BurstWindows[i].m_MaxBurstDelay = _maxdelay;
			return true;
		}
	}

	for(int i = 0; i < MaxBurstWindows; ++i)
	{
		if(m_BurstWindows[i].m_BurstRounds == 0) // m_BurstRounds 0 determines un-used slots
		{
			m_BurstWindows[i].m_MinRange = _minrange;
			m_BurstWindows[i].m_MaxRange = _maxrange;
			m_BurstWindows[i].m_BurstRounds = _burst;
			m_BurstWindows[i].m_MinBurstDelay = _mindelay;
			m_BurstWindows[i].m_MaxBurstDelay = _maxdelay;
			return true;
		}
	}
	return false;
}

Vector3f Weapon::WeaponFireMode::GetAimPoint(Client *_bot, const GameEntity &_target, const TargetInfo &_targetinfo)
{
	Vector3f vAimPoint;

	//////////////////////////////////////////////////////////////////////////
	// Script callback if necessary.
	if(m_scrCalcAimPoint)
	{
		gmMachine *pMachine = ScriptManager::GetInstance()->GetMachine();

		gmCall call;
		if(call.BeginFunction(pMachine, m_scrCalcAimPoint, gmVariable(GetScriptObject(pMachine))))
		{
			OBASSERT(_bot->GetScriptObject(), "Invalid Script Object For Bot!");
			call.AddParamUser(GetScriptObject(pMachine));
			call.AddParamUser(_bot->GetScriptObject());
			call.AddParamUser(_targetinfo.GetScriptObject(pMachine));
			call.End();

			const gmVariable &ret = call.GetReturnedVariable();
			if(ret.m_type == GM_VEC3)
			{
				ret.GetVector(vAimPoint.x,vAimPoint.y,vAimPoint.z);
				vAimPoint += m_AimOffset;
				return vAimPoint;
			}
			else
			{
				Utils::OutputDebug(kError, "Invalid Return Value From CalculateAimPoint");				
			}
		}
		else
		{
			Utils::OutputDebug(kError, "Invalid Weapon CalculateAimPoint Callback!");
		}
	}
	//////////////////////////////////////////////////////////////////////////
	
	switch(m_WeaponType)
	{
	case Melee:
		vAimPoint = _GetAimPoint_Melee(_bot, _target, _targetinfo);
		break;
	case Projectile:
		vAimPoint = _GetAimPoint_Projectile(_bot, _target, _targetinfo);
		break;
	case Grenade:
		vAimPoint = _GetAimPoint_Grenade(_bot, _target, _targetinfo);
		break;
	default:
		Utils::OutputDebug(kError, "Invalid Weapon Type!");
	case Item:
	case InstantHit:
		vAimPoint = _GetAimPoint_InstantHit(_bot, _target, _targetinfo);
		break;
	}

	vAimPoint += m_AimOffset;
	return vAimPoint;
}

Vector3f Weapon::WeaponFireMode::_GetAimPoint_Melee(Client *_bot, const GameEntity &_target, const TargetInfo &_targetinfo)
{
	return _targetinfo.m_LastPosition + (_targetinfo.m_LastVelocity * IGame::GetDeltaTimeSecs());
}

Vector3f Weapon::WeaponFireMode::_GetAimPoint_InstantHit(Client *_bot, const GameEntity &_target, const TargetInfo &_targetinfo)
{
	return _targetinfo.m_LastPosition + (_targetinfo.m_LastVelocity * IGame::GetDeltaTimeSecs());
}

Vector3f Weapon::WeaponFireMode::_GetAimPoint_Projectile(Client *_bot, const GameEntity &_target, const TargetInfo &_targetinfo)
{
	const Vector3f vEyePos = _bot->GetPosition();
	const Vector3f vMyVelocity = CheckFlag(InheritsVelocity) ? _bot->GetVelocity() : Vector3f::ZERO;
	Vector3f vAimPt = Utils::PredictFuturePositionOfTarget(
		vEyePos, 
		m_ProjectileSpeed, 
		_targetinfo,
		vMyVelocity,
		m_MinLeadError,
		m_MaxLeadError);

	if(m_ProjectileGravity != 0)
	{
		///////////////////////////////////////////////////////////////
		//float fProjSpeedAngleMod = 0.f;
		//static Vector3f s_ProjSpeed(3000*1.1f, 3000*1.1f, 1500*1.1f);
		//Vector3f vProjVel = _bot->GetFacingVector();
		//vProjVel.x *= s_ProjSpeed.x;
		//vProjVel.y *= s_ProjSpeed.y;
		//vProjVel.z *= s_ProjSpeed.z;
		////vProjVel.Length();
		//float fProjectileSpeed = vProjVel.Normalize();
		//Quaternionf q;
		//q.Align(_bot->GetFacingVector(), vProjVel);

		//Vector3f vAxis;
		//q.ToAxisAngle(vAxis, fProjSpeedAngleMod);

		//fProjSpeedAngleMod = Mathf::ATan(vProjVel.z - _bot->GetFacingVector().z);

		//EngineFuncs::ConsoleMessage("Angle Diff: %f", Mathf::RadToDeg(fProjSpeedAngleMod));
		///////////////////////////////////////////////////////////////

		Trajectory::AimTrajectory traj[2];
		int t = Trajectory::Calculate(
			vEyePos, 
			vAimPt, 
			m_ProjectileSpeed, 
			IGame::GetGravity() * m_ProjectileGravity, 
			traj);

		// TODO: test each trajectory
		//bool bTrajectoryGood = true;

		int iTrajectoryNum = CheckFlag(UseMortarTrajectory) ? 1 : 0;

		Vector3f vAimDir = vAimPt - vEyePos;
		const float fAimLength2d = Vector2f(vAimDir).Length();
		if(t > iTrajectoryNum)
		{
			vAimPt.z = vEyePos.z + Mathf::Tan(traj[iTrajectoryNum].m_Angle - m_PitchOffset) * fAimLength2d;

			/*if(_bot->IsDebugEnabled(BOT_DEBUG_AIMPOINT))
			{
			}*/
		}
		else
			vAimPt.z = vEyePos.z + Mathf::Tan(Mathf::DegToRad(45.0f)) * fAimLength2d;
	}
	return vAimPt;
}

Vector3f Weapon::WeaponFireMode::_GetAimPoint_Grenade(Client *_bot, const GameEntity &_target, const TargetInfo &_targetinfo)
{
	// TODO: account for fusetime
	return _GetAimPoint_Projectile(_bot, _target, _targetinfo);
}

void Weapon::WeaponFireMode::AddAimError(Client *_bot, Vector3f &_aimpoint, const TargetInfo &_info)
{
	// Check if we need to get a new aim error.
	if(IGame::GetTime() > m_NextAimAdjustmentTime)
	{
		m_AimErrorCurrent.x = (Mathf::SymmetricRandom() * m_AimErrorMax.x);
		m_AimErrorCurrent.y = (Mathf::SymmetricRandom() * m_AimErrorMax.x);
		m_AimErrorCurrent.z = (Mathf::SymmetricRandom() * m_AimErrorMax.y);

		const float fDelaySecs = Mathf::IntervalRandom(m_MinAimAdjustmentSecs, m_MaxAimAdjustmentSecs);
		m_NextAimAdjustmentTime = IGame::GetTime() + Utils::SecondsToMilliseconds(fDelaySecs);
	}

	// Add the aim offset.
	_aimpoint += m_AimErrorCurrent;
}

void Weapon::WeaponFireMode::SetTargetBias(int _targetclass, obReal _bias)
{
	if(_targetclass < ENT_CLASS_GENERIC_START)
	{
		if((int)m_TargetBias.size() <= _targetclass)
			m_TargetBias.resize(_targetclass+1, 1.f);
		m_TargetBias[_targetclass] = _bias;
	}
}

obReal Weapon::WeaponFireMode::GetTargetBias(int _targetclass, const BitFlag64 & entFlags)
{
	if ( (entFlags & m_TargetEntFlagIgnore).AnyFlagSet() )
		return 0.0f;
	if(_targetclass >= 0 && _targetclass < (int)m_TargetBias.size())
		return m_TargetBias[_targetclass];
	return 1.f;
}

static int GM_CDECL gmfSetIgnoreEntFlags(gmThread *a_thread) {
	Weapon::WeaponFireMode *NativePtr = 0;
	if(!gmBind2::Class<Weapon::WeaponFireMode>::FromThis(a_thread,NativePtr) || !NativePtr)
	{
		GM_EXCEPTION_MSG("Script Function on NULL MapGoal"); 
		return GM_EXCEPTION;
	}

	NativePtr->GetIgnoreEntFlags().ClearAll();
	for( int i = 0; i < a_thread->GetNumParams(); ++i ) {
		GM_CHECK_INT_PARAM( flg, i );
		NativePtr->GetIgnoreEntFlags().SetFlag( flg, true );
	}
	return GM_OK;
}

//////////////////////////////////////////////////////////////////////////
// Utilities
bool Weapon::WeaponFireMode::IsDefined() const
{
	return m_WeaponType != 0;
}

bool Weapon::WeaponFireMode::CheckDefined() const 
{
	OBASSERT(IsDefined(), "Weapon Fire Mode Not Defined!");
	return IsDefined(); 
}

gmGCRoot<gmUserObject> Weapon::WeaponFireMode::GetScriptObject(gmMachine *_machine) const
{
	if(!m_ScriptObject)
		m_ScriptObject = gmBind2::Class<WeaponFireMode>::WrapObject(_machine,const_cast<WeaponFireMode*>(this),true);
	return m_ScriptObject;
}

//////////////////////////////////////////////////////////////////////////
// Events

void Weapon::WeaponFireMode::OnPreFire(Weapon *_weapon, Client *_client, const TargetInfo *_target)
{
	UpdateBurstWindow(_target);
}

void Weapon::WeaponFireMode::OnChargeWeapon(Weapon *_weapon, Client *_client)
{
	if(CheckFlag(FireOnRelease))
	{
		_client->PressButton(m_ShootButton);
		if(!IsCharging())
		{
			m_ChargeTime = IGame::GetTime() + 
				Utils::SecondsToMilliseconds(Mathf::IntervalRandom(m_MinChargeTime, m_MaxChargeTime));
		}
		else
		{
			// keep the charge time from expiring while charging.
			if(m_ChargeTime <= IGame::GetTime())
				m_ChargeTime = IGame::GetTime()+1;
		}
	}
}

void Weapon::WeaponFireMode::OnZoomWeapon(Weapon *_weapon, Client *_client)
{
	if(CheckFlag(HasZoom))
	{
		_client->PressButton(m_ZoomButton);
	}
}

void Weapon::WeaponFireMode::OnStartShooting(Weapon *_weapon, Client *_client)
{
	CheckDefined();

	if(CheckFlag(MustBeOnGround) && !_client->HasEntityFlag(ENT_FLAG_ONGROUND))
		return;
	
	if(IsBurstDelaying())
		return;

	if(CheckFlag(FireOnRelease))
	{
		if(IsCharging())
		{
			if(CheckFlag(ChargeToIntercept))
			{
				const MemoryRecord *pRecord = _client->GetTargetingSystem()->GetCurrentTargetRecord();
				if(pRecord)
				{
					obint32 primeTime = m_ChargeTime - Utils::SecondsToMilliseconds(m_FuseTime);
					float fTimeToDet = m_FuseTime - Utils::MillisecondsToSeconds(IGame::GetTime() - primeTime);

					bool bThrow = fTimeToDet < 1.0;
					Vector3f vFirePos = _client->GetEyePosition();
					Vector3f vFacing = _client->GetFacingVector();
					Vector3f v2dFacing = vFacing.Flatten();
					
					Vector3f vDetPos = _client->GetEyePosition() + (v2dFacing*m_ProjectileSpeed) * fTimeToDet;
					float fExplodeDistToTarget = (vDetPos - vFirePos).Length();

					float fGrenadeRange = fTimeToDet * m_ProjectileSpeed;

					/*Utils::OutputDebug(kDebug,"fTimeToDet: %f, fExplodeDistToTarget: %f / %f", 
						fTimeToDet, fExplodeDistToTarget, fGrenadeRange);*/

					if(fExplodeDistToTarget >= fGrenadeRange)
						bThrow = true;

					if(bThrow)
					{
						m_ChargeTime = 0;
						_client->ReleaseButton(m_ShootButton);
					}
				}
			}
			else
			{
				if(IGame::GetTime() > m_ChargeTime)
				{
					m_ChargeTime = 0;
					_client->ReleaseButton(m_ShootButton);
					return;
				}
			}
		}
		else
		{
			// Not currently charging, begin charging.
			if(CheckFlag(ChargeToIntercept))
			{
				m_ChargeTime = IGame::GetTime() + Utils::SecondsToMilliseconds(m_FuseTime);
			}
			else
			{
				m_ChargeTime = IGame::GetTime() + 
					Utils::SecondsToMilliseconds(Mathf::IntervalRandom(m_MinChargeTime, m_MaxChargeTime));
			}
		}
		_client->PressButton(m_ShootButton);
	}
	else
	{
		if(m_ShootButton == BOT_BUTTON_THROWKNIFE)
			_client->GameCommand("throwknife");
		else
			_client->PressButton(m_ShootButton);
	}
}

void Weapon::WeaponFireMode::OnStopShooting(Weapon *_weapon, Client *_client)
{
	_client->ReleaseButton(m_ShootButton);
}

void Weapon::WeaponFireMode::OnReload(Weapon *_weapon, Client *_client)
{
	_client->PressButton(BOT_BUTTON_RELOAD);
	_client->ReleaseButton(m_ShootButton);
}

bool Weapon::WeaponFireMode::OnNeedToReload(Weapon *_weapon, Client *_client)
{
	if(CheckDefined())
	{
		if(!CheckFlag(IgnoreReload) && NeedsAmmo() && !FullClip() && EnoughAmmoToReload())
			return true;
	}
	return false;
}

void Weapon::WeaponFireMode::OnShotFired(Weapon *_weapon, Client *_client, GameEntity _projectile /*= NULL*/)
{
	const BurstWindow &w = m_BurstWindows[m_CurrentBurstWindow];
	if(w.m_BurstRounds > 0)
	{
		if(++m_BurstRound >= w.m_BurstRounds)
		{
			m_BurstTime = IGame::GetTime() + 
				Utils::SecondsToMilliseconds(Mathf::IntervalRandom(w.m_MinBurstDelay, w.m_MaxBurstDelay));
			m_BurstRound = 0;
		}
	}
	m_DelayChooseTime = IGame::GetTime() + Utils::SecondsToMilliseconds(m_DelayAfterFiring);
}

bool Weapon::WeaponFireMode::IsCharging() const 
{
	return m_ChargeTime != 0;
}

bool Weapon::WeaponFireMode::HasChargeTimes() const
{
	return m_MinChargeTime && m_MaxChargeTime;
}

bool Weapon::WeaponFireMode::IsBurstDelaying() const 
{
	return m_BurstTime > IGame::GetTime(); 
}

//////////////////////////////////////////////////////////////////////////

Weapon::Weapon(Client *_client/* = 0*/)
	: m_Client				(_client)
	, m_WeaponID			(0)
	, m_WeaponAliasID		(0)
	, m_WeaponLockTime		(0)
	, m_WeaponNameHash		(0)
	, m_MinUseTime			(0)
	, m_ScriptObject		(0)
{
	memset(&m_WeaponLimits, 0, sizeof(m_WeaponLimits));	
}

Weapon::Weapon(Client *_client, const Weapon *_wpn) 
	: m_Client				(_client)
	, m_ScriptObject		(0)
{
	m_WeaponID = _wpn->m_WeaponID;
	m_WeaponAliasID = _wpn->m_WeaponAliasID;

	m_WeaponNameHash = _wpn->m_WeaponNameHash;
	m_WeaponLockTime = _wpn->m_WeaponLockTime;
	m_MinUseTime = _wpn->m_MinUseTime;

	ScriptResource::operator=(*_wpn);

	memset(&m_WeaponLimits, 0, sizeof(m_WeaponLimits));	
	
	// duplicate the script table
	gmMachine *pM = ScriptManager::GetInstance()->GetMachine();
	gmBind2::Class<Weapon>::CloneTable(pM,_wpn->GetScriptObject(pM),GetScriptObject(pM));

	for(int i = 0; i < Num_FireModes; ++i)
	{
		m_FireModes[i] = _wpn->m_FireModes[i];
	}
}

Weapon::~Weapon()
{
	if(m_ScriptObject)
	{
		gmBind2::Class<Weapon>::NullifyUserObject(m_ScriptObject);
		m_ScriptObject = 0; //NULL;
	}
}

String Weapon::GetWeaponName() const 
{
	return Utils::HashToString(m_WeaponNameHash);
}

void Weapon::SetWeaponName(const char *_name)
{
	m_WeaponNameHash = _name ? Utils::MakeHash32(_name) : 0; 
}

gmGCRoot<gmUserObject> Weapon::GetScriptObject(gmMachine *_machine) const
{
	if(!m_ScriptObject)
		m_ScriptObject = gmBind2::Class<Weapon>::WrapObject(_machine,const_cast<Weapon*>(this),true);
	return m_ScriptObject;
}

void Weapon::GetSpectateMessage(StringStr &_outstring)
{
	_outstring << "Weapon: " << GetWeaponName() << " ";	
	for(int i = 0; i < Num_FireModes; ++i)
	{
		FireMode m = GetFireMode(i);
		if(GetFireMode(m).IsDefined())
		{
			if(m == Primary)
				_outstring << "P(";
			else
				_outstring << "S(";

			if(GetFireMode(m).IsCharging() && GetFireMode(m).HasChargeTimes())
				_outstring << "Charging, ";
			if(GetFireMode(m).IsBurstDelaying())
				_outstring << "BurstDelay, ";
			_outstring << ")";
		}
	}
}

obReal Weapon::LowOnAmmoPriority(FireMode _mode, int &_ammotype, int &_getammo)
{
	obReal fLowAmmoPriority = 0.f;
	if(GetFireMode(_mode).IsDefined())
	{
		_ammotype = GetWeaponID();//GetFireMode(_mode).m_AmmoType;
	
		// _getammo must be greater than threshold, otherwise UseCabinet goal would be 
		// immediately aborted after path to cabinet is found
		int threshold = GetFireMode(_mode).GetLowAmmoThreshold();
		_getammo = MaxT(threshold + 1, GetFireMode(_mode).GetLowAmmoGetAmmoAmount());
		if(GetFireMode(_mode).m_AmmoCurrent <= threshold)
		{
			fLowAmmoPriority = GetFireMode(_mode).GetLowAmmoPriority();
		}
	}
	return fLowAmmoPriority;
}

void Weapon::ReloadWeapon(FireMode _mode)
{
	if(GetFireMode(_mode).CheckDefined())
	{
		GetFireMode(_mode).OnReload(this, m_Client);
	}
}

void Weapon::Select()
{
	if(m_MinUseTime != 0.f)
		m_WeaponLockTime = IGame::GetTime() + Utils::SecondsToMilliseconds(m_MinUseTime);
}

bool Weapon::SelectionLocked()
{
	return m_WeaponLockTime > IGame::GetTime();
}

bool Weapon::CanShoot(FireMode _mode, const TargetInfo &_targetinfo)
{
	if(!_MeetsRequirements(_mode, _targetinfo))
		return false;

	return GetFireMode(_mode).CalculateDesirability(m_Client, _targetinfo) > 0;
}

FireMode Weapon::GetBestFireMode(const TargetInfo &_targetinfo)
{
	if(!GetFireMode(Secondary).IsDefined())
	{
		WeaponType type = GetFireMode(Primary).m_WeaponType;
		if(type == Weapon::Item) return InvalidFireMode;
		if(type != Weapon::Melee) return Primary;
	}
	
	FireMode bestFireMode = InvalidFireMode;
	obReal fBestDesir = 0.f;
	for(int i = 0; i < Num_FireModes; ++i)
	{
		FireMode m = GetFireMode(i);
		WeaponFireMode& fireMode = GetFireMode(m);
		if(fireMode.IsDefined())
		{
			if(!_MeetsRequirements(m, _targetinfo))
				continue;

			obReal fDesir = fireMode.CalculateDesirability(m_Client, _targetinfo);
			if(fDesir > fBestDesir &&
				(fireMode.m_WeaponType != Melee || fDesir > fireMode.m_DefaultDesirability * fireMode.m_WeaponBias))
			{
				fBestDesir = fDesir;
				bestFireMode = m;
			}
		}
	}
	return bestFireMode;
}

void Weapon::PreShoot(FireMode _mode, bool _facingTarget, const TargetInfo *_target)
{
	GetFireMode(_mode).OnPreFire(this, m_Client, _target);
}

void Weapon::ChargeWeapon(FireMode _mode)
{
	GetFireMode(_mode).OnChargeWeapon(this, m_Client);
}

void Weapon::ZoomWeapon(FireMode _mode)
{
	GetFireMode(_mode).OnZoomWeapon(this, m_Client);
}

void Weapon::ShotFired(FireMode _mode, GameEntity _projectile)
{
	GetFireMode(_mode).OnShotFired(this, m_Client, _projectile);
}

void Weapon::Shoot(FireMode _mode, const TargetInfo *_target)
{
	if(GetFireMode(_mode).EmptyClip() && 
		GetFireMode(_mode).NeedsAmmo()) 
	{
		if(GetFireMode(_mode).EnoughAmmoToReload()) 
		{
			ReloadWeapon(_mode);
		}
	} 
	else 
	{
		bool bFire = true;
		if(GetFireMode(_mode).CheckFlag(ManageHeat))
		{
			float fCurrentHeat = 0.f, fMaxHeat = 0.f;
			float fRatio = InterfaceFuncs::WeaponHeat(m_Client, _mode, fCurrentHeat, fMaxHeat);
			GetFireMode(_mode).m_HeatController.Update(0.7f, fRatio, IGame::GetDeltaTimeSecs());
			bFire = Mathf::Sign(GetFireMode(_mode).m_HeatController.GetControlValue()) < 0.f ? false : true;
		}

		if(bFire)
		{
			GetFireMode(_mode).OnStartShooting(this, m_Client);
		}
	}
}

void Weapon::StopShooting(FireMode _mode)
{
	GetFireMode(_mode).OnStopShooting(this, m_Client);
	if(GetFireMode(_mode).EmptyClip() && 
		GetFireMode(_mode).NeedsAmmo() && 
		GetFireMode(_mode).EnoughAmmoToReload())
	{
		ReloadWeapon(_mode);
	}
}

void Weapon::UpdateClipAmmo(FireMode _mode)
{
	WeaponFireMode& fireMode = GetFireMode(_mode);
	if(fireMode.IsDefined() && fireMode.CheckFlag(RequiresAmmo) && fireMode.m_LastClipAmmoUpdate < IGame::GetTime())
	{
		fireMode.m_LastClipAmmoUpdate = IGame::GetTime();
		g_EngineFuncs->GetCurrentWeaponClip(
			m_Client->GetGameEntity(), 
			_mode, 
			fireMode.m_ClipCurrent,
			fireMode.m_ClipMax);
	}
}

void Weapon::UpdateAmmo(FireMode _mode)
{
	WeaponFireMode& fireMode = GetFireMode(_mode);
	if(fireMode.m_LastAmmoUpdate == IGame::GetTime())
		return;

	fireMode.m_LastAmmoUpdate = IGame::GetTime();
	if(fireMode.CheckFlag(RequiresAmmo))
	{
		g_EngineFuncs->GetCurrentAmmo(
			m_Client->GetGameEntity(),
			m_Client->ConvertWeaponIdToMod(GetWeaponID()),
			_mode,
			fireMode.m_AmmoCurrent,
			fireMode.m_AmmoMax);

		if(fireMode.m_ShootButton == BOT_BUTTON_THROWKNIFE)
		{
			//UpdateClipAmmo can be called only for currently equipped weapon
			if(InterfaceFuncs::GetEquippedWeapon(m_Client->GetGameEntity()).m_WeaponId == GetWeaponID())
			{
				UpdateClipAmmo(_mode);
			}
			else
			{
				//bot does not know knives count after spawn, let's assume he has one knife
				if(fireMode.m_LastClipAmmoUpdate < m_Client->m_SpawnTime && fireMode.m_AmmoCurrent > 0) 
					fireMode.m_ClipCurrent = 1;
			}

			fireMode.m_AmmoMax = fireMode.m_AmmoCurrent - fireMode.m_ClipCurrent;
			if(fireMode.m_AmmoMax==0) fireMode.m_ClipCurrent = 0; //hack for Bastard mod
			fireMode.m_AmmoCurrent = fireMode.m_ClipCurrent;
		}
	}
}

void Weapon::Update(FireMode _mode)
{
	if(GetFireMode(_mode).IsBurstDelaying())
	{
		if(GetFireMode(_mode).m_BurstTime > IGame::GetTime())
			return;
		GetFireMode(_mode).m_BurstTime = 0;
	}

	/*if(GetFireMode(_mode).IsCharging())
	{
		GetFireMode(_mode).OnStartShooting(this,m_Client);
	}*/
}

Weapon::MoveOptions Weapon::UpdateMoveMode()
{
	MoveOptions _options;
	if(GetFireMode(Primary).IsDefined())
	{
		if(m_Client->GetEntityFlags().CheckFlag(ENT_FLAG_AIMING))
		{
			if(GetFireMode(Primary).CheckFlag(StopWhileZoomed))
			{			
				_options.MustStop = true;
			}
			else if(GetFireMode(Primary).CheckFlag(WalkWhileZoomed))
			{			
				_options.MustWalk = true;
			}
			else if(GetFireMode(Primary).CheckFlag(CrouchToMoveWhenZoomed))
			{			
				_options.CrouchToMove = true;
			}
		}
	}
	return _options;
}

Vector3f Weapon::GetAimPoint(FireMode _mode, const GameEntity &_target, const TargetInfo &_targetinfo)
{
	return GetFireMode(_mode).GetAimPoint(m_Client, _target, _targetinfo);
}

void Weapon::AddAimError(FireMode _mode, Vector3f &_aimpoint, const TargetInfo &_info)
{
	GetFireMode(_mode).AddAimError(m_Client, _aimpoint, _info);
}

FireMode Weapon::CanReload()
{
	FireMode ret = InvalidFireMode;
	for(int mode = Primary; mode < Num_FireModes; ++mode)
	{
		FireMode m = Weapon::GetFireMode(mode);
		if(GetFireMode(m).IsDefined() && GetFireMode(m).OnNeedToReload(this, m_Client))
		{
			ret = m;
			break;
		}
	}
	return ret;
}

FireMode Weapon::IsClipEmpty()
{
	FireMode ret = InvalidFireMode;
	for(int mode = Primary; mode < Num_FireModes; ++mode)
	{
		FireMode m = Weapon::GetFireMode(mode);
		if(GetFireMode(m).IsDefined() && GetFireMode(m).EmptyClip())
		{
			ret = m;
			break;
		}
	}
	return ret;
}

FireMode Weapon::OutOfAmmo()
{
	FireMode ret = InvalidFireMode;
	for(int mode = Primary; mode < Num_FireModes; ++mode)
	{
		FireMode m = Weapon::GetFireMode(mode);
		if(GetFireMode(m).IsDefined() && !GetFireMode(m).HasAmmo())
		{
			ret = m;
			break;
		}
	}
	return ret;
}

FireMode Weapon::GetFireMode(int _mode)
{
	switch(_mode)
	{
	case Primary:
		return Primary;
	case Secondary:
		return Secondary;
	default:
		return InvalidFireMode;
	}
}

//////////////////////////////////////////////////////////////////////////

obReal Weapon::CalculateDefaultDesirability()
{
	obReal fDesir = 0.f, fBestDesir = 0.f;
	for(int i = Primary; i < Num_FireModes; ++i)
	{
		FireMode m = GetFireMode(i);
		if(GetFireMode(m).IsDefined())
		{
			if(!_MeetsRequirements(m))
				continue;

			fDesir = GetFireMode(m).CalculateDefaultDesirability(m_Client);
			if(fDesir > fBestDesir)
			{
				fBestDesir = fDesir;
			}
		}
	}
	return fBestDesir;
}

obReal Weapon::CalculateDesirability(const TargetInfo &_targetinfo)
{
	obReal fDesir = 0.f, fBestDesir = 0.f;
	for(int i = Primary; i < Num_FireModes; ++i)
	{
		FireMode m = GetFireMode(i);
		if(GetFireMode(m).IsDefined())
		{
			if(!_MeetsRequirements(m, _targetinfo))
				continue;

			fDesir = GetFireMode(m).CalculateDesirability(m_Client, _targetinfo);
			if(fDesir > fBestDesir)
			{
				fBestDesir = fDesir;
			}
		}
	}
	return fBestDesir;	
}

//////////////////////////////////////////////////////////////////////////

bool Weapon::_MeetsRequirements(FireMode _mode)
{	
	WeaponFireMode& fireMode = GetFireMode(_mode);
	
	if(fireMode.m_WeaponType == Weapon::Item)
		return false;

	if(!fireMode.CheckFlag(Waterproof) && m_Client->HasEntityFlag(ENT_FLAG_UNDERWATER))
		return false;

	if(fireMode.CheckFlag(RequiresAmmo)){
		UpdateAmmo(_mode);
		if(!fireMode.HasAmmo())
			return false;
	}

	if(fireMode.m_DelayChooseTime > IGame::GetTime())
		return false;

	if(!InterfaceFuncs::IsWeaponCharged(m_Client, GetWeaponID(), _mode))
		return false;

	return true;
}

bool Weapon::_MeetsRequirements(FireMode _mode, const TargetInfo &_targetinfo)
{
	if(!_MeetsRequirements(_mode))
		return false;
	if(GetFireMode(_mode).CheckFlag(RequireShooterOutside) && !InterfaceFuncs::IsOutSide(m_Client->GetPosition()))
		return false;
	if(GetFireMode(_mode).CheckFlag(RequireTargetOutside) && !InterfaceFuncs::IsOutSide(_targetinfo.m_LastPosition))
		return false;
	return true;
}

//////////////////////////////////////////////////////////////////////////

bool Weapon::WeaponFireMode::getType( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands )
{
	const char* s;

	switch(a_native->m_WeaponType)
	{
	case Weapon::Melee:
		s = "melee";
		break;
	case Weapon::InstantHit:
		s = "instant";
		break;
	case Weapon::Projectile:
		s = "projectile";
		break;
	case Weapon::Grenade:
		s = "grenade";
		break;
	case Weapon::Item:
		s = "item";
		break;
	default:
		a_operands[0].Nullify();
		return true;
	}
	a_operands[0].SetString(a_thread->GetMachine()->AllocStringObject(s));
	return true;
}

bool Weapon::WeaponFireMode::setType( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands )
{
	gmStringObject *pObj = a_operands[1].GetStringObjectSafe();
	if(pObj != NULL && pObj->GetString())
	{
		if(!_gmstricmp(pObj->GetString(), "melee"))
			a_native->m_WeaponType = Weapon::Melee;
		else if(!_gmstricmp(pObj->GetString(), "instant"))
			a_native->m_WeaponType = Weapon::InstantHit;
		else if(!_gmstricmp(pObj->GetString(), "projectile"))
			a_native->m_WeaponType = Weapon::Projectile;
		else if(!_gmstricmp(pObj->GetString(), "grenade"))
			a_native->m_WeaponType = Weapon::Grenade;
		else if(!_gmstricmp(pObj->GetString(), "item"))
			a_native->m_WeaponType = Weapon::Item;
		else
			Utils::OutputDebug(kError, "Invalid Weapon Type specified: %s", pObj->GetString());
	}
	return true;
}

bool Weapon::WeaponFireMode::getMaxAimError( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands )
{
	a_operands[0].SetVector(a_native->m_AimErrorMax.x, a_native->m_AimErrorMax.y, 0);
	return true;
}

bool Weapon::WeaponFireMode::setMaxAimError( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands )
{
	if(a_operands[1].IsVector())
	{
		a_native->m_AimErrorMax = Vector2f(
			a_operands[1].m_value.m_vec3.x, 
			a_operands[1].m_value.m_vec3.y);
	}
	return true;
}

bool Weapon::WeaponFireMode::getAimOffset( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands )
{
	a_operands[0].SetVector(a_native->m_AimOffset.x, a_native->m_AimOffset.y, a_native->m_AimOffset.z);
	return true;
}

bool Weapon::WeaponFireMode::setAimOffset( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands )
{
	if(a_operands[1].IsVector())
	{
		a_native->m_AimOffset = Vector3f(
			a_operands[1].m_value.m_vec3.x, 
			a_operands[1].m_value.m_vec3.y,
			a_operands[1].m_value.m_vec3.z);
	}
	return true;
}

void Weapon::WeaponFireMode::Bind(gmMachine *_m)
{
	gmBind2::Class<WeaponFireMode>("FireMode",_m)
		.func(&WeaponFireMode::SetDesirabilityWindow,		"SetDesirabilityRange","Set the desirability for a target within a certain min/max range.")
		.func(&WeaponFireMode::SetBurstWindow,				"SetBurstRange","Set a burst shot behavior for a target within a certain min/max range.")
		.func(&WeaponFireMode::SetTargetBias,				"SetTargetBias","Set a desirability multiplier versus a target class.")
		.func(gmfSetIgnoreEntFlags,							"SetIgnoreEntFlags","Sets one or more entity flags that should be ignored for this weapon.")

		.var(getType,setType,								"WeaponType","string","melee, instant, projectile, grenade, or item")
		
		.var(&WeaponFireMode::m_ShootButton,				"ShootButton",0,"The button to press to fire the weapon. Default ATTACK1.")
		.var(&WeaponFireMode::m_ZoomButton,					"ZoomButton",0,"The button to press to zoom the weapon. Default AIM.")

		// resupply
		.var(&WeaponFireMode::m_LowAmmoThreshold,			"LowAmmoThreshold",0,"Bot will desire ammo if <= to this value.")
		.var(&WeaponFireMode::m_LowAmmoPriority,			"LowAmmoPriority",0,"The priority to get ammo when the threshold is met.")
		.var(&WeaponFireMode::m_LowAmmoGetAmmoAmount,		"LowAmmoGetAmmoAmount",0,"How much ammo to get to satisfy.")

		// projectile data
		.var(&WeaponFireMode::m_FuseTime,					"FuseTime",0,"Fuse time to predict when ChargeToIntercept is set.")
		.var(&WeaponFireMode::m_ProjectileSpeed,			"ProjectileSpeed",0,"How fast the projectile moves. Used for prediction.")
		.var(&WeaponFireMode::m_ProjectileGravity,			"ProjectileGravity",0,"Gravity multiplier for how projectile is effected by gravity.")
		.var(&WeaponFireMode::m_SplashRadius,				"SplashRadius",0,"The radius of the hits splash damage.")
		
		.var(&WeaponFireMode::m_MinChargeTime,				"MinChargeTime",0,"Minimum time to charge FireOnRelease Shots")
		.var(&WeaponFireMode::m_MaxChargeTime,				"MaxChargeTime",0,"Maximum time to charge FireOnRelease Shots")

		// misc
		.var(&WeaponFireMode::m_DelayAfterFiring,			"DelayAfterFiring",0,"Time after shooting to delay choosing this weapon again.")

		.var(&WeaponFireMode::m_DefaultDesirability,		"DefaultDesirability",0,"Desirability vs no target.")

		.var(&WeaponFireMode::m_WeaponBias,					"Bias",0,"Multiplier to final desirability.")

		.var(&WeaponFireMode::m_MinAimAdjustmentSecs,		"MinAimAdjustmentTime",0,"Minimum time between aim adjustments.")
		.var(&WeaponFireMode::m_MaxAimAdjustmentSecs,		"MaxAimAdjustmentTime",0,"Maximum time between aim adjustments.")

		// aiming
		.var(getMaxAimError,setMaxAimError,					"MaxAimError","vec3","Horizontal and vertical aim error.")
		.var(getAimOffset,setAimOffset,						"AimOffset","vec3","Offset added to targeting aim point.")
		.var(&WeaponFireMode::m_AimOffsetZ,					"AimOffsetZ",0,"Vertical offset added to AimOffset if AdjustAim is 1.")
		
		.var(&WeaponFireMode::m_PitchOffset,				"PitchOffset",0,"Pitch offset to projectile spawn point.")

		.var(&WeaponFireMode::m_MinLeadError,				"MinLeadError",0,"Minimum lead time error when firing weapon.")
		.var(&WeaponFireMode::m_MaxLeadError,				"MaxLeadError",0,"Maximum lead time error when firing weapon.")

		// functions
		.var(&WeaponFireMode::m_scrCalcDefDesir,			"CalculateDefaultDesirability", "Callback", "Allows weapon to calculate default desirability.")
		.var(&WeaponFireMode::m_scrCalcDesir,				"CalculateDesirability", "Callback", "Allows weapon to calculate desirability.")
		.var(&WeaponFireMode::m_scrCalcAimPoint,			"CalculateAimPoint", "Callback", "Allows weapon to calculate aim point.")

		.var_bitfield(&WeaponFireMode::m_WeaponFlags,RequiresAmmo,			"RequiresAmmo",0,"Weapon requires ammo to use. False means ammo is always assumed.")
		.var_bitfield(&WeaponFireMode::m_WeaponFlags,Waterproof,			"WaterProof",0,"Weapon may be used the user is underwater.")		
		.var_bitfield(&WeaponFireMode::m_WeaponFlags,HasClip,				"HasClip",0,"Weapon has a clip. False means it simply has an ammo repository and doesn't need to reload.")
		/*replace with zoomfov?*/
		.var_bitfield(&WeaponFireMode::m_WeaponFlags,HasZoom,				"HasZoom",0,"Weapon has zoom functionality.")
		.var_bitfield(&WeaponFireMode::m_WeaponFlags,InheritsVelocity,		"InheritsVelocity",0,"Weapon projectile inherits user velocity.")
		.var_bitfield(&WeaponFireMode::m_WeaponFlags,ManualDetonation,		"ManualDetonation",0,"Weapon projectiles must be manually detonated.")
		.var_bitfield(&WeaponFireMode::m_WeaponFlags,MustBeOnGround,		"MustBeOnGround",0,"Weapon can only fire if user is on ground.")
		.var_bitfield(&WeaponFireMode::m_WeaponFlags,FireOnRelease,			"FireOnRelease",0,"Weapon fires when the ShootButton is released, as opposed to when pressed.")
		.var_bitfield(&WeaponFireMode::m_WeaponFlags,ManageHeat,			"ManageHeat",0,"Weapon may overheat, so user should fan the fire button to prevent.")
		.var_bitfield(&WeaponFireMode::m_WeaponFlags,IgnoreReload,			"IgnoreReload",0,"Weapon should not be checked for reload.")
		.var_bitfield(&WeaponFireMode::m_WeaponFlags,UseMortarTrajectory,	"UseMortarTrajectory",0,"Weapon should use mortar trajectory in prediction checks.")
		.var_bitfield(&WeaponFireMode::m_WeaponFlags,RequireTargetOutside,	"RequiresTargetOutside",0,"Weapon cannot fire on target unless it is outside.")
		.var_bitfield(&WeaponFireMode::m_WeaponFlags,RequireShooterOutside,	"RequiresShooterOutside",0,"Weapon cannot fire unless user is outside.")
		.var_bitfield(&WeaponFireMode::m_WeaponFlags,ChargeToIntercept,		"ChargeToIntercept",0,"Weapon should be 'primed' with MinChargeTime/MaxChargeTime before firing.")
		.var_bitfield(&WeaponFireMode::m_WeaponFlags,MeleeWeapon,			"MeleeWeapon",0,"Weapon is a melee weapon, user should use melee attack behavior.")
		.var_bitfield(&WeaponFireMode::m_WeaponFlags,WalkWhileZoomed,		"WalkWhileZoomed",0,"Weapon requires user to hold walk key when zoomed.")
		.var_bitfield(&WeaponFireMode::m_WeaponFlags,StopWhileZoomed,		"StopWhileZoomed",0,"Weapon requires user to stop moving when zoomed.")
		.var_bitfield(&WeaponFireMode::m_WeaponFlags,CrouchToMoveWhenZoomed,"CrouchToMoveWhenZoomed",0,"Weapon requires user to crouch to move when zoomed.")
		;
}

bool Weapon::getName( Weapon *a_native, gmThread *a_thread, gmVariable *a_operands )
{
	a_operands[0].SetString(a_thread->GetMachine(),a_native->GetWeaponName().c_str());
	return true;
}

bool Weapon::setName( Weapon *a_native, gmThread *a_thread, gmVariable *a_operands )
{
	gmStringObject *pStr = a_operands[1].GetStringObjectSafe();
	if(pStr != NULL && pStr->GetString())
		a_native->SetWeaponName(pStr->GetString());
	return true;
}

bool Weapon::getPrimaryFire( Weapon *a_native, gmThread *a_thread, gmVariable *a_operands )
{
	gmUserObject *pObj = a_native->m_FireModes[Primary].GetScriptObject(a_thread->GetMachine());
	a_operands[0].SetUser(pObj);
	return true;
}

bool Weapon::getSecondaryFire( Weapon *a_native, gmThread *a_thread, gmVariable *a_operands )
{
	gmUserObject *pObj = a_native->m_FireModes[Secondary].GetScriptObject(a_thread->GetMachine());
	a_operands[0].SetUser(pObj);
	return true;
}

void Weapon::Bind(gmMachine *_m)
{
	WeaponFireMode::Bind(_m);

	gmBind2::Class<Weapon>("Weapon",_m)
		//.constructor()
		.var(getName, setName,			"Name","string","Name of the weapon.")
		.var(&Weapon::m_WeaponID,		"WeaponId","int","Numeric Id of the weapon.")
		.var(&Weapon::m_WeaponAliasID,	"WeaponAliasId","Numeric Id that will also match to this weapon.")
		
		.var(&Weapon::m_MinUseTime,		"MinUseTime","float","Weapon must be used for a minimum amount of time when chosen.")
		.var(getPrimaryFire, NULL,		"PrimaryFire","firemode","Access to primary fire mode.")
		.var(getSecondaryFire, NULL,	"SecondaryFire","firemode","Access to secondary fire mode.")
		;
}
