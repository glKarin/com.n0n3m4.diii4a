//This class represents a weapon as a whole, and a container for the
//various fire modes that it has. Some mods may only use 1 fire mod, while others may support many.

#ifndef __WEAPON_H__
#define __WEAPON_H__

#include "PIDController.h"
#include "gmGCRoot.h"

class Client;
class TargetInfo;

class gmMachine;
class gmTableObject;

//////////////////////////////////////////////////////////////////////////

class WeaponScriptResource : public ScriptResource
{
public:
	virtual bool InitScriptSource(const filePath &_path);
};

//////////////////////////////////////////////////////////////////////////
// class: Weapon
class Weapon : public WeaponScriptResource
{
	public:
		friend class gmWeapon;

		typedef enum
		{
			Run,
			Walk,
			Still
		} MoveMode;

		typedef enum
		{
			Stand,
			Crouch,
			Prone,
			
			Ignore,
			NumStances
		} Stance;

		typedef enum
		{
			None,
			Melee,
			InstantHit,
			Projectile,
			Grenade,
			Item
		} WeaponType;

		typedef enum
		{
			RequiresAmmo,
			Waterproof,
			HasClip,
			HasZoom,
			Stealth,			
			InheritsVelocity,
			MustBeOnGround,
			FireOnRelease,
			ManageHeat,
			IgnoreReload,
			UseMortarTrajectory,
			RequireTargetOutside,
			RequireShooterOutside,
			ChargeToIntercept,
			MeleeWeapon,
			ManualDetonation, // TODO
			WalkWhileZoomed,
			StopWhileZoomed,
			CrouchToMoveWhenZoomed,

			// THIS MUST STAY LAST
			Num_Properties
		} WeaponProperty;

		// class: FireMode
		class WeaponFireMode
		{
		public:
			friend class Weapon;
			friend class gmFireMode;

			//////////////////////////////////////////////////////////////////////////
			enum { MaxDesirabilities = 4 };
			struct DesirabilityWindow
			{
				float	m_MinRange;
				float	m_MaxRange;
				float	m_Desirability;
			};
			//////////////////////////////////////////////////////////////////////////
			enum { MaxBurstWindows = 4 };
			struct BurstWindow
			{
				int		m_BurstRounds;
				float	m_MinRange;
				float	m_MaxRange;
				float	m_MinBurstDelay;
				float	m_MaxBurstDelay;
			};
			//////////////////////////////////////////////////////////////////////////

			// function: CalculateDefaultDesirability
			//		Calculates the default desirability for this weapon. Default desirability
			//		is the desirability of a weapon when the bot has no target. Basically his 'idle' weapon
			//
			// Parameters:
			//
			//		None
			//
			// Returns:
			//		obReal - The default desirability for this weapon
			obReal CalculateDefaultDesirability(Client *_bot);

			// function: CalculateDesirability
			//		Calculates the desirability for this weapon, based on the current targets info.
			//
			// Parameters:
			//
			//		<TargetInfo> - Known information about the current target.
			//
			// Returns:
			//		obReal - The desirability for this weapon against the target.
			obReal CalculateDesirability(Client *_bot, const TargetInfo &_targetinfo);

			// function: UpdateBurstWindow
			//		Updates the burst window currently in use by the weapon burst fire.
			//
			// Parameters:
			//
			//		<TargetInfo> - Known information about the current target.
			//
			// Returns:
			//		None
			void UpdateBurstWindow(const TargetInfo *_targetinfo);

			// function: GetCurrentClip
			//		Number of rounds in the current clip
			//
			// Parameters:
			//
			//		None
			//
			// Returns:
			//		int - rounds in the current clip.
			inline int GetCurrentClip() const				{ return m_ClipCurrent; }

			// function: GetMaxClip
			//		Number of max rounds in the current clip
			//
			// Parameters:
			//
			//		None
			//
			// Returns:
			//		int - max rounds in the clip
			inline int GetMaxClip() const				{ return m_ClipMax; }

			// function: GetCurrentAmmo
			//		Current amount of ammo for a given ammo type
			//
			// Parameters:
			//
			//		None
			//
			// Returns:
			//		int - number of rounds
			inline int GetCurrentAmmo() const			{ return m_AmmoCurrent; }

			// function: GetMaxAmmo
			//		Max amount of ammo for a given ammo type
			//
			// Parameters:
			//
			//		None
			//
			// Returns:
			//		int - max number of rounds
			inline int GetMaxAmmo() const				{ return m_AmmoMax; }

			// function: GetLowAmmoThreshold
			//		Get the script defined value at which the bot should attempt to resupply ammo.
			//
			// Parameters:
			//
			//		None
			//
			// Returns:
			//		int - max number of total ammo
			inline int GetLowAmmoThreshold() const		{ return m_LowAmmoThreshold; }

			// function: GetLowAmmoPriority
			//		If the weapon needs ammo, what's the priority for it.
			//
			// Parameters:
			//
			//		None
			//
			// Returns:
			//		float - priority if the weapon needs ammo
			inline obReal GetLowAmmoPriority() const		{ return m_LowAmmoPriority; }
			
			inline int GetLowAmmoGetAmmoAmount() const { return m_LowAmmoGetAmmoAmount; }
			// function: HasAmmo
			//		Checks if this weapon currently has enough ammo to fire.
			//
			// Parameters:
			//
			//		None
			//
			// Returns:
			//		bool - true if it has enough ammo to fire, false if not
			bool HasAmmo(int _amount = 0) const;

			// function: NeedsAmmo
			//		Checks if this weapon currently needs ammo to use.
			//
			// Parameters:
			//
			//		None
			//
			// Returns:
			//		bool - true if this weapon needs ammo to fire, false if not
			bool NeedsAmmo() const;

			// function: FullClip
			//		Checks if this weapon currently has a full clip.
			//
			// Parameters:
			//
			//		None
			//
			// Returns:
			//		bool - true if it has a full clip, false if not
			bool FullClip() const;

			// function: EmptyClip
			//		Checks if this weapon currently has an empty clip.
			//
			// Parameters:
			//
			//		None
			//
			// Returns:
			//		bool - true if it has an empty clip, false if not
			bool EmptyClip() const;

			bool UsesClip() const;

			// function: EnoughAmmoToReload
			//		Checks if there is enough ammo in the bots inventory to reload this weapon.
			//
			// Parameters:
			//
			//		None
			//
			// Returns:
			//		bool - true if there is enough ammo to reload, false if not.
			bool EnoughAmmoToReload() const;

			Vector3f GetAimPoint(Client *_bot, const GameEntity &_target, const TargetInfo &_targetinfo);
			void AddAimError(Client *_bot, Vector3f &_aimpoint, const TargetInfo &_info);

			bool SetDesirabilityWindow(float _minrange, float _maxrange, float _desir);
			bool SetBurstWindow(float _minrange, float _maxrange, int _burst, float _mindelay, float _maxdelay);

			obReal GetWeaponBias() const { return m_WeaponBias; }

			obReal GetTargetBias(int _targetclass, const BitFlag64 & entFlags);

			void SetTargetBias(int _targetclass, obReal _bias);

			BitFlag64 & GetIgnoreEntFlags() { return m_TargetEntFlagIgnore; }

			bool IsCharging() const;
			bool HasChargeTimes() const;
			bool IsBurstDelaying() const;

			//////////////////////////////////////////////////////////////////////////
			inline int GetChargeTime() const { return m_ChargeTime; }
			inline int GetBurstTime() const { return m_BurstTime; }
			inline int GetBurstRound() const { return m_BurstRound; }
			inline const BurstWindow &GetBurstWindow() const { return m_BurstWindows[m_CurrentBurstWindow]; }

			//////////////////////////////////////////////////////////////////////////
			// Events the fire mode can respond to.
			void OnPreFire(Weapon *_weapon, Client *_client, const TargetInfo *_target);
			void OnStartShooting(Weapon *_weapon, Client *_client);
			void OnStopShooting(Weapon *_weapon, Client *_client);
			void OnReload(Weapon *_weapon, Client *_client);
			bool OnNeedToReload(Weapon *_weapon, Client *_client);
			void OnChargeWeapon(Weapon *_weapon, Client *_client);
			void OnZoomWeapon(Weapon *_weapon, Client *_client);
			void OnShotFired(Weapon *_weapon, Client *_client, GameEntity _projectile = GameEntity());

			//////////////////////////////////////////////////////////////////////////
			// Utility
			bool IsDefined() const;
			bool CheckDefined() const;
			bool CheckFlag(obint32 _flag) const { return BitFlag32(m_WeaponFlags).CheckFlag(_flag); }
			void SetFlag(obint32 _flag, bool _set) { BitFlag32 bf(m_WeaponFlags); bf.SetFlag(_flag,_set); m_WeaponFlags = bf.GetRawFlags(); }

			gmGCRoot<gmUserObject> GetScriptObject(gmMachine *_machine) const;

			WeaponFireMode & operator=(const WeaponFireMode &_rh);

			static void Bind(gmMachine *_m);

			WeaponFireMode();
			~WeaponFireMode();
		private:
			// var: m_WeaponType
			//		The enumerated type of the weapon.
			WeaponType		m_WeaponType;

			// var: m_WeaponFlags
			//		Bit flags representing weapon properties.
			int				m_WeaponFlags;

			int				m_ShootButton;
			int				m_ZoomButton;

			obReal			m_ProjectileSpeed;
			obReal			m_ProjectileGravity;

			obReal			m_MinChargeTime;
			obReal			m_MaxChargeTime;

			obReal			m_MinLeadError;
			obReal			m_MaxLeadError;

			obReal			m_DelayAfterFiring;

			obReal			m_FuseTime;
			obReal			m_SplashRadius;

			float			m_PitchOffset;

			int				m_LowAmmoThreshold;
			obReal			m_LowAmmoPriority;
			int				m_LowAmmoGetAmmoAmount;

			PIDController	m_HeatController;

			typedef std::vector<obReal> TargetBiasList;
			TargetBiasList	m_TargetBias;

			//////////////////////////////////////////////////////////////////////////
			DesirabilityWindow	m_Desirabilities[MaxDesirabilities];

			BurstWindow			m_BurstWindows[MaxBurstWindows];
			int					m_CurrentBurstWindow;
			//////////////////////////////////////////////////////////////////////////

			// var: m_AmmoCurrent
			//		The current amount of ammo this weapon has.
			int				m_AmmoCurrent;

			// var: m_AmmoMax
			//		The max amount of ammo this weapon can use
			int				m_AmmoMax;

			// var: m_ClipCurrent
			//		The ammo count in the current clip.
			int				m_ClipCurrent;

			// var: m_ClipMax
			//		The ammo count in the max clip.
			int				m_ClipMax;

			// var: m_AimOffset
			//		The <Vector3> aim offset that should be applied to weapon aim points to modify targeting.
			Vector3f		m_AimOffset;

			// var: m_AimOffsetZ
			//		The vertical offset that should be added to AimOffset if AdjustAim is 1 (used in goal_difficulty.gm).
			obReal			m_AimOffsetZ;

			// var: m_AimErrorMax
			//		The max <Vector2> aim error range to apply to aiming. horizontal(x), vertical(y)
			Vector2f		m_AimErrorMax;

			// var: m_AimErrorCurrent
			//		The current <Vector3> aim error used to apply a error to the aim functions points.
			Vector3f		m_AimErrorCurrent;

			// var: m_MinAimAdjustmentDelay;
			//		The min time between aim adjustments.
			obReal			m_MinAimAdjustmentSecs;

			// var: m_MaxAimAdjustmentDelay;
			//		The max time between aim adjustments.
			obReal			m_MaxAimAdjustmentSecs;

			// var: m_NextAimAdjustmentTime
			//		The time the next aim adjustment should take place.
			int				m_NextAimAdjustmentTime;

			// var: m_LastDesirability
			//		The last desirability calculated for this weapon.
			obReal			m_LastDesirability;

			// var: m_DefaultDesirability
			//		The last default desirability calculated for this weapon.
			obReal			m_DefaultDesirability;

			// var: m_WeaponBias
			//		The m_Client's current bias for this weapon.
			obReal			m_WeaponBias;

			// var: m_TargetEntFlagIgnore
			//		If any entity flags exist on a target, this weapon should ignore it
			BitFlag64		m_TargetEntFlagIgnore;

			// For scripting
			mutable gmGCRoot<gmUserObject> m_ScriptObject;

			gmGCRoot<gmFunctionObject>		m_scrCalcDefDesir;
			gmGCRoot<gmFunctionObject>		m_scrCalcDesir;
			gmGCRoot<gmFunctionObject>		m_scrCalcAimPoint;
		private:

			Vector3f _GetAimPoint_Melee(Client *_bot, const GameEntity &_target, const TargetInfo &_targetinfo);
			Vector3f _GetAimPoint_InstantHit(Client *_bot, const GameEntity &_target, const TargetInfo &_targetinfo);
			Vector3f _GetAimPoint_Projectile(Client *_bot, const GameEntity &_target, const TargetInfo &_targetinfo);
			Vector3f _GetAimPoint_Grenade(Client *_bot, const GameEntity &_target, const TargetInfo &_targetinfo);

			// Internal variables
			int		m_ChargeTime;
			int		m_DelayChooseTime;
			int		m_BurstTime;
			int		m_BurstRound;
			int		m_LastAmmoUpdate;
			int		m_LastClipAmmoUpdate;

			struct
			{
				bool Charging : 1;
			} fl;

			// script binding helpers
			static bool getType( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands );
			static bool setType( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands );
			static bool getMaxAimError( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands );
			static bool setMaxAimError( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands );
			static bool getAimOffset( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands );
			static bool setAimOffset( Weapon::WeaponFireMode *a_native, gmThread *a_thread, gmVariable *a_operands );
		};
		
		void Update(FireMode _mode);

		struct MoveOptions
		{
			bool	MustWalk;
			bool	MustStop;			
			bool	CrouchToMove;

			MoveOptions() 
				: MustWalk(false)
				, MustStop(false)
				, CrouchToMove(false)
			{
			}
		};
		MoveOptions UpdateMoveMode();

		// function: UpdateClipAmmo
		//		Updates the clip ammo stats for the weapon from the game.
		//
		// Parameters:
		//
		//		None
		//
		// Returns:
		//		None
		void UpdateClipAmmo(FireMode _mode = Primary);

		// function: UpdateAmmo
		//		Updates the ammo stats for the weapon from the game, using the 
		//		currently configured weapon id and ammo id.
		//
		// Parameters:
		//
		//		None
		//
		// Returns:
		//		None
		void UpdateAmmo(FireMode _mode = Primary);		

		// function: ReloadWeapon
		//		Initiates a reload action on the weapon.
		//
		// Parameters:
		//
		//		None
		//
		// Returns:
		//		None
		void ReloadWeapon(FireMode _mode = Primary);

		// function: ChargeWeapon
		//		Initiates a charge action on the weapon.
		//
		// Parameters:
		//
		//		None
		//
		// Returns:
		//		None
		void ChargeWeapon(FireMode _mode = Primary);

		// function: ZoomWeapon
		//		Initiates a zoom action on the weapon.
		//
		// Parameters:
		//
		//		None
		//
		// Returns:
		//		None
		void ZoomWeapon(FireMode _mode = Primary);

		// function: ShotFired
		//		Notifies the weapon that it fired a shot.
		//
		// Parameters:
		//
		//		None
		//
		// Returns:
		//		None
		void ShotFired(FireMode _mode, GameEntity _projectile = GameEntity());

		// function: Select
		//		Called when the weapon is initially selected, giving the weapon a
		//		chance to do any internal initialization.
		//
		// Parameters:
		//
		//		None
		//
		// Returns:
		//		bool - true if weapon is charged, false if still charging.
		void Select();

		bool SelectionLocked();

		bool CanShoot(FireMode _mode, const TargetInfo &_targetinfo);
		
		FireMode GetBestFireMode(const TargetInfo &_targetinfo);

		// function: PreShoot
		//		Called during the time the weapon has been chosen and the bot
		//		is attempting to fire at the enemy. Target won't necessarily be lined up yet.
		//
		// Parameters:
		//
		//		<FireMode> - FireMode that is getting ready to shoot.
		//		<TargetInfo> - Optional target being fired at
		//		bool - If the shot is lined up yet.
		//
		// Returns:
		//		bool - true if weapon is charged, false if still charging.
		void PreShoot(FireMode _mode, bool _facingTarget = false, const TargetInfo *_target = 0);

		// function: Shoot
		//		Initiates a shoot action on the weapon.
		//
		// Parameters:
		//
		//		<TargetInfo> - Optional target being fired at
		//
		// Returns:
		//		None
		void Shoot(FireMode _mode = Primary, const TargetInfo *_target = 0);

		// function: StopShooting
		//		Cancels a shooting action for the weapon.
		//
		// Parameters:
		//
		//		None
		//
		// Returns:
		//		None
		void StopShooting(FireMode _mode = Primary);

		// function: GetAimPoint
		//		Calculates an aim point for this weapon based on a targets information.
		//
		// Parameters:
		//
		//		_ent - The <GameEntity> we're getting the aim point for.
		//		_targetinfo - The <TargetInfo> for the game entity.
		//
		// Returns:
		//		Vector3 - The point we should attempt to aim for this weapon.
		Vector3f GetAimPoint(FireMode _mode, const GameEntity &_target, const TargetInfo &_targetinfo);

		// function: AddAimError
		//		Adds an error offset to an aim point vector.
		//
		// Parameters:
		//
		//		_aimpoint - The <Vector3> aim point we should apply an error to.
		//
		// Returns:
		//		None
		void AddAimError(FireMode _mode, Vector3f &_aimpoint, const TargetInfo &_info);

		// function: CanReload
		//		Checks if either fire mode of this weapon can reload(not full clip), and we have ammo to do it.
		//
		// Parameters:
		//
		//		None
		//
		// Returns:
		//		FireMode - Which fire mode if any needs to reload.
		FireMode CanReload();

		// function: IsClipEmpty
		//		Checks if either fire mode of this weapon is currently empty.
		//
		// Parameters:
		//
		//		None
		//
		// Returns:
		//		FireMode - Which fire mode is empty.
		FireMode IsClipEmpty();
		FireMode OutOfAmmo();

		// function: GetWeaponID
		//		Gets the weapon Id for this weapon.
		//
		// Parameters:
		//
		//		None
		//
		// Returns:
		//		int - The weapon Id
		inline int GetWeaponID() const			{ return m_WeaponID; }
		inline int GetWeaponAliasID() const		{ return m_WeaponAliasID; }

		// function: GetWeaponName
		//		Gets the weapon name for this weapon.
		//
		// Parameters:
		//
		//		None
		//
		// Returns:
		//		string - The weapon name.
		obuint32 GetWeaponNameHash() const { return m_WeaponNameHash; }
		String GetWeaponName() const;
		void SetWeaponName(const char *_name);
		gmGCRoot<gmUserObject> GetScriptObject(gmMachine *_machine) const;
		
		WeaponFireMode &GetFireMode(FireMode _mode)		{ return m_FireModes[_mode]; }
		const WeaponFireMode &GetFireMode(FireMode _mode) const { return m_FireModes[_mode]; }
		
		void GetSpectateMessage(StringStr &_outstring);

		obReal CalculateDefaultDesirability();
		obReal CalculateDesirability(const TargetInfo &_targetinfo);

		obReal LowOnAmmoPriority(FireMode _mode, int &_ammotype, int &_getammo);

		bool IsWeapon(int _id) const { return GetWeaponID()==_id || GetWeaponAliasID()==_id; }

		static FireMode GetFireMode(int _mode);

		static void Bind(gmMachine *_m);

		Weapon(Client *_client = 0);
		Weapon(Client *_client, const Weapon *_wpn);
		~Weapon();
	protected:
		// var: m_Client
		//		The client that owns this weapon evaluator.
		Client			*m_Client;

		// var: m_WeaponID
		//		The weapon type Id that this weapon uses. See global WEAPON table.
		int				m_WeaponID;
		int				m_WeaponAliasID;
		
		int				m_WeaponLockTime;

		//////////////////////////////////////////////////////////////////////////

		WeaponLimits	m_WeaponLimits;

		// var: m_WeaponName
		//		The name for this weapon.
		obuint32		m_WeaponNameHash;

		WeaponFireMode	m_FireModes[Num_FireModes];

		// var: m_MinUseTime
		//		The minimum time to use a weapon when equipped.
		obReal			m_MinUseTime;

		// For scripting
		mutable gmGCRoot<gmUserObject>	m_ScriptObject;		
	private:
		bool _MeetsRequirements(FireMode _mode);
		bool _MeetsRequirements(FireMode _mode, const TargetInfo &_targetinfo);

		// script binding helpers
		static bool getName( Weapon *a_native, gmThread *a_thread, gmVariable *a_operands );
		static bool setName( Weapon *a_native, gmThread *a_thread, gmVariable *a_operands );
		static bool getPrimaryFire( Weapon *a_native, gmThread *a_thread, gmVariable *a_operands );
		static bool getSecondaryFire( Weapon *a_native, gmThread *a_thread, gmVariable *a_operands );
};

//////////////////////////////////////////////////////////////////////////
// typedef: WeaponPtr
//		Shorthand typedef for a smart weapon pointer.
#if __cplusplus >= 201103L //karin: using C++11 instead of boost
typedef std::shared_ptr<Weapon> WeaponPtr;
#else
typedef boost::shared_ptr<Weapon> WeaponPtr;
#endif
//////////////////////////////////////////////////////////////////////////

#endif
