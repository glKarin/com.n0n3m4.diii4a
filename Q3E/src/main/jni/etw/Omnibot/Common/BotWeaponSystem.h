#ifndef __BOTWEAPONSYSTEM_H__
#define __BOTWEAPONSYSTEM_H__

#include "FilterSensory.h"
#include "BotBaseStates.h"
#include "Weapon.h"

class Client;
class MemoryRecord;

// typedef: WeaponList
//		Holds a list of weapons the bot currently has.
typedef std::list<WeaponPtr> WeaponList;

namespace AiState
{
	// class: AttackTarget
	class AttackTarget : public StateChild, public AimerUser
	{
	public:

		void RenderDebug();
		void GetDebugString(StringStr &out);

		obReal GetPriority();
		void Enter();
		void Exit();
		StateStatus Update(float fDt);

		bool TargetExceedsWeaponLimits() const { return m_TargetExceedsWeaponLimits; }

		// AimerUser functions.
		bool GetAimPosition(Vector3f &_aimpos);
		void OnTarget();

		AttackTarget();
	private:
		WeaponLimits	m_WeaponLimits;
		Vector3f		m_AimPosition;
		obuint32		m_CurrentWeaponHash;
		FireMode	m_FireMode;
		bool			m_ShootTheBastard : 1;
		bool			m_TargetExceedsWeaponLimits : 1;
	};

	// class: WeaponSystem
	//		The weapon system is the centerpoint for weapon selection and
	//		firing on a target using the weapons available to the bot.
	class WeaponSystem : public StateFirstAvailable
	{
	public:
		struct WeaponRequest
		{
			Priority::ePriority		m_Priority;
			obuint32 				m_Owner;
			int						m_WeaponId;

			void Reset();
			WeaponRequest();
		private:
		};

		bool AddWeaponRequest(Priority::ePriority _prio, obuint32 _owner, int _weaponId);
		void ReleaseWeaponRequest(const String &_owner);
		void ReleaseWeaponRequest(obuint32 _owner);
		bool UpdateWeaponRequest(obuint32 _owner, int _weaponId);
		int GetWeaponNeedingReload();

		BitFlag128 GetWeaponMask() const { return m_WeaponMask; }

		// function: GrabAllWeapons
		//		Gets a copy of all weapons from the weapon database.
		void GrabAllWeapons();

		// function: RefreshWeapon
		//		Refreshes a copy of a particular weapon.
		void RefreshWeapon(int _weaponId);

		// function: RefreshAllWeapons
		//		Refreshes a copy of all weapons from the weapon database.
		void RefreshAllWeapons();

		// function: AddWeaponToInventory
		//		Adds a pointer to a weapon into the inventory weapon list.
		bool AddWeaponToInventory(int _weaponId);

		// function: AddWeapon
		//		Gives a weapon to the bot for it to use
		void AddWeapon(WeaponPtr _weapon);

		// function: RemoveWeapon
		//		Removes a weapon from the bots inventory by its id.
		void RemoveWeapon(int _weaponId);

		// function: ClearWeapons
		//		Strips the bot of all weapons
		void ClearWeapons();

		// function: SelectBestWeapon
		//		Evaluates all the bots current weapons and chooses the best one,
		//		given the current target
		int SelectBestWeapon(GameEntity _targetent = GameEntity());

		// function: SelectRandomWeapon
		//		Returns weapon id for a random weapon
		int SelectRandomWeapon();

		// function: UpdateAllWeaponAmmo
		//		Allows each weapon to update the current status of its ammo
		void UpdateAllWeaponAmmo();

		// function: ReadyToFire
		//		Asks the game if the bot is currently ready to fire the current weapon
		bool ReadyToFire();

		// function: IsReloading
		//		Asks the game if the bot is currently reloading
		bool IsReloading();

		// function: ChargeWeapon
		//		Attempts to start 'charging' the current weapon.
		void ChargeWeapon(FireMode _mode = Primary);

		// function: ZoomWeapon
		//		Attempts to zoom the weapon(if HasZoom)
		void ZoomWeapon(FireMode _mode = Primary);

		// function: FireWeapon
		//		Attempts to fire the current weapon
		void FireWeapon(FireMode _mode = Primary);

		// function: HasWeapon
		//		Checks if the bot has this weapon
		bool HasWeapon(int _weaponId) const;

		// function: HasAmmo
		//		Checks is the current weapon has ammo
		bool HasAmmo(FireMode _mode) const;

		// function: HasAmmo
		//		Checks if a specific weapon has ammo
		bool HasAmmo(int _weaponid, FireMode _mode = Primary, int _amount = 0) const;

		// function: GetCurrentWeaponID
		//		Gets the Id of the current weapon
		inline int GetCurrentWeaponID() const			{ return m_CurrentWeapon ? m_CurrentWeapon->GetWeaponID() : -1; }

		// function: GetDesiredWeaponID
		//		Gets the Id of the desired weapon
		inline int GetDesiredWeaponID() const			{ return m_DesiredWeaponID; }

		inline obint32 GetDefaultWeaponID() const			{ return m_DefaultWeapon; }
		inline obint32 GetOverrideWeaponID() const			{ return m_OverrideWeapon; }

		inline void SetOverrideWeaponID(obint32 w) { m_OverrideWeapon = w; }
		inline void ClearOverrideWeaponID() { m_OverrideWeapon = 0; }

		// function: CurrentWeaponIs
		//		Accessor to check if the current weapon is a particular weapon
		inline bool CurrentWeaponIs(int _weaponid) const 
		{
			const int wId = m_CurrentWeapon ? m_CurrentWeapon->GetWeaponID() : 0;
			return wId && wId==_weaponid; 
		}

		inline bool CurrentWeaponIsAttackReady() const
		{
			if(m_CurrentWeapon)
			{
				return 
					m_CurrentWeapon->IsWeapon(GetDefaultWeaponID()) || 
					m_CurrentWeapon->IsWeapon(GetDesiredWeaponID());
			}
			return false;
			/*const int wId = m_CurrentWeapon ? m_CurrentWeapon->GetWeaponID() : 0;
			return wId && (
				wId==GetDefaultWeaponID() || 
				wId==GetOverrideWeaponID() || 
				wId==GetDesiredWeaponID()
				); */
		}

		// function: GetCurrentWeapon
		//		Gets the actual current weapon
		inline WeaponPtr GetCurrentWeapon()				{ return m_CurrentWeapon; }

		// function: GetWeapon
		//		Gets the ptr to a specific weapon, if available.
		WeaponPtr GetWeapon(int _weaponId, bool _inventory = true) const;

		WeaponPtr GetWeaponByIndex(int _index, bool _inventory = true);

		// function: GetMostDesiredAmmo
		//		Calculates the most desired ammo based on the bots current weapons
		//		fills the parameter with the ammo Id, and returns the desirability of it
		obReal GetMostDesiredAmmo(int &_weapon, int &_getammo);
		obuint32 GetCurrentRequestOwner() const { return m_CurrentRequestOwner; }

		bool CanShoot(const MemoryRecord &_record);

		inline int GetAimPersistance() const			{ return m_AimPersistance; }
		inline void SetAimPersistance(int _aimperms)	{ m_AimPersistance = _aimperms; }
		inline int GetReactionTime() const				{ return m_ReactionTimeInMS; }
		inline void SetReactionTime(int _reactionms)	{ m_ReactionTimeInMS = _reactionms; }

		void GetSpectateMessage(StringStr &_outstring);

		WeaponList &GetWeaponList() { return m_WeaponList; }

		// State Functions
		void Initialize();
		obReal GetPriority();
		void Enter();
		void Exit();
		StateStatus Update(float fDt);

		void GetDebugString(StringStr &out);

		const WeaponRequest &GetHighestWeaponRequest();

		WeaponSystem();
		virtual ~WeaponSystem();
	protected:
		// int: m_ReactionTimeInMS
		//		Minimum time the bot has to see an opponent before reacting
		int				m_ReactionTimeInMS;

		// int: m_AimPersistance
		//		Amount of time target will aim at the last enemy position,
		//		after the target leaves view.
		int				m_AimPersistance;

		BitFlag128		m_WeaponMask;

		// var: m_WeaponList
		//		The list of all the bots current weapons
		WeaponList		m_AllWeaponList;
		WeaponList		m_WeaponList;

		// int: m_DesiredWeaponID
		//		The Id of the currently desired weapon
		int				m_DesiredWeaponID;

		obint32			m_DefaultWeapon;
		obint32			m_OverrideWeapon;

		enum { MaxWeaponRequests = 8 };
		WeaponRequest	m_WeaponRequests[MaxWeaponRequests];
		int				m_CurrentRequestOwner;

		// var: m_CurrentWeapon
		//		The actual current weapon
		WeaponPtr		m_CurrentWeapon;

		// function: _UpdateWeaponFromGame
		//		Asks the game what weapon the bot currently has
		//		We do this because the game ultimately has authority
		//		and for various reasons the server may not always allow the 
		//		bot to have the weapon it asks for from the game.
		WeaponStatus _UpdateWeaponFromGame();

		// function: _UpdateCurrentWeapon
		//		Chooses the current weapon based on the result of <_UpdateWeaponFromGame>
		void _UpdateCurrentWeapon(FireMode _mode);
	};
}
#endif
