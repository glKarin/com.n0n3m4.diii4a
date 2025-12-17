#ifndef __BOTRIX_WEAPON_H__
#define __BOTRIX_WEAPON_H__


#include "item.h"
#include "mod.h"
#include "source_engine.h"
#include "server_plugin.h"

#include <good/bitset.h>
#include <good/memory.h>

#include "vector.h"


//****************************************************************************************************************
/// Weapon abstract class. Used to get needed angles to aim.
//****************************************************************************************************************
class CWeapon
{
public:
    /// Default constructor.
    CWeapon()
    {
        memset(this, 0, sizeof(CWeapon));
        iAttackBullets[0] = iAttackBullets[1] = 1;
        fMaxDistanceSqr[0] = fMaxDistanceSqr[1] = CUtil::iMaxMapSizeSqr;
        iBotPreference = EBotNormal;
    }

	/// Get ammo index from name.
	int GetAmmoIndexFromName( const good::string& sName, bool& bIsSecondary ) const
	{
		for ( int i = PRIMARY; i <= SECONDARY; ++i )
		{
			for ( int j = 0; j <= aAmmos[i].size(); ++j )
				if ( aAmmos[i][j]->sClassName == sName )
				{
					bIsSecondary = i == SECONDARY;
					return j;
				}
		}
		return -1;
	}

    static const int PRIMARY = 0;                ///< Index for primary ammo.
    static const int SECONDARY = 1;              ///< Index for secondary ammo.

    TWeaponId iId;                               ///< Weapon id.
    const CItemClass* pWeaponClass;            ///< Pointer to weapon class.

    TClass iClass;                               ///< Classes that can uses this weapon. Those are really flags.
    TTeam iTeam;                                 ///< Only this team can buy this weapon. Those are really flags.

    TWeaponType iType;                           ///< Weapon type.
    TWeaponAim iAim[2];                          ///< Where to aim with this weapon.
    TWeaponFlags iFlags[2];                      ///< Attack flags.

    float fMinDistanceSqr[2];                    ///< Minimum distance to enemy to safely use this weapon (0 by default).
    float fMaxDistanceSqr[2];                    ///< Maximum distance to enemy to be able to use this weapon (0 by default).
    float fHolsterTime;                          ///< Time from change weapon to be able to shoot.
    float fHoldTime[2];                          ///< Time to hold down attack button. Useful for grenades.
    float fShotTime[2];                          ///< Duration of shoot (primary and secondary).
    float fReloadStartTime[2];                   ///< Time to start reload (primary and secondary).
    float fReloadTime[2];                        ///< Duration of reload (primary and secondary).
    float fDamage[2];                            ///< How much damage one bullet does.

    unsigned char iClipSize[2];                  ///< How much bullets fit in one clip.
    unsigned char iDefaultAmmo[2];               ///< Count of bullets this weapon gives by default.
    unsigned char iMaxAmmo[2];                   ///< Count of max bullets this weapon can have (besides the clip in the weapon).
    unsigned char iAttackBullets[2];             ///< How many bullets are used in attack.
    unsigned char iReloadBy[2];                  ///< Bullets are used in one reload time.
    int iParabolicDistance0[2];                  ///< Distance that bullet does in straight line until falls.
    int iParabolicDistance45[2];                 ///< Distance that bullet does looking up 45grades until falls.

    TBotIntelligence iBotPreference;             ///< Smart bots will prefer weapons with higher preference.
    bool bForbidden;                             ///< True if weapon is forbidden.

    good::vector<const CItemClass*> aAmmos[2];   ///< Ammo item classes.
    good::vector<int> aAmmoBullets[2];           ///< Bullets for ammo at the same index.
};


//****************************************************************************************************************
/// Weapon with ammunition.
//****************************************************************************************************************
class CWeaponWithAmmo
{
public:
    /// Constructor.
    CWeaponWithAmmo( const CWeapon* pWeapon )
    {
        memset(this, 0, sizeof(CWeaponWithAmmo));
        m_pWeapon = pWeapon;
    }

    /// Game frame.
    void GameFrame( int& iButtons );

    /// Return base weapon.
    inline const CWeapon* GetBaseWeapon() const { return m_pWeapon; }

    /// Return weapon's entity name.
    inline const good::string& GetName() const { return m_pWeapon->pWeaponClass->sClassName; }

    /// Return false if there is only ammo for this weapon, but without weapon itself.
    inline bool IsPresent() const { return m_bWeaponPresent; }

    /// Return true if this weapon is melee.
    inline bool IsMelee() const { return (m_pWeapon->iType == EWeaponMelee); }

    /// Return true if this weapon is ranged (not grenade).
    inline bool IsRanged() const { return (m_pWeapon->iType > EWeaponGrenade); }

    /// Return true if need to throw.
    inline bool IsGrenade() const { return (m_pWeapon->iType == EWeaponGrenade); }

    /// Return true if weapon is sniper.
    inline bool IsSniper() const { return FLAG_SOME_SET(FWeaponZoom, m_pWeapon->iFlags[1]) != 0; }

    /// Return true if weapon is medic.
    inline bool IsMedic() const { return FLAG_SOME_SET(FWeaponCure, m_pWeapon->iFlags[0]) != 0; }

    /// Return true if weapon is physics.
    inline bool IsPhysics() const { return (m_pWeapon->iType == EWeaponPhysics); }

    /// Return true if currently reloading.
    inline bool IsReloading() const { return m_bReloading || m_bReloadingStart; }

    /// Return true if currently shooting.
    inline bool IsShooting() const { return m_bShooting || m_bHolding; }

    /// Return true if currently changing to this weapon.
    inline bool IsChanging() const { return m_bChanging; }

    /// Return true if currently using zoom.
    inline bool IsUsingZoom() const { return m_bUsingZoom; }

    // Return true if can use this weapon for given distance to enemy (it is safe).
    bool IsDistanceSafe( float fDistanceSqr, int iSecondary ) const
    {
        return (m_pWeapon->fMinDistanceSqr[iSecondary] <= fDistanceSqr) &&
               (fDistanceSqr <= m_pWeapon->fMaxDistanceSqr[iSecondary]);
    }

    /// Return true if weapon has secondary function with different ammo than primary.
    inline bool HasSecondary() const
    {
        return FLAG_CLEARED( FWeaponSameBullets, m_pWeapon->iFlags[1] );
    }

    /// Return true if weapon has ammo.
    inline bool HasAmmoInClip( int iSecondary ) const { return m_iBulletsInClip[iSecondary] > 0; }

    /// Return true if weapon has ammo, beside the ammo in clip.
    inline bool HasAmmoExtra( int iSecondary ) const { return m_iBulletsExtra[iSecondary] > 0; }

    /// Return true if weapon has full ammunition.
    inline bool HasFullAmmo( int iSecondary ) const { return m_iBulletsExtra[iSecondary] == m_pWeapon->iMaxAmmo[iSecondary]; }

    /// Return true if weapon has ammo.
    inline bool HasAmmo( int iSecondary ) const { return HasAmmoInClip(iSecondary) || HasAmmoExtra(iSecondary); }

    /// Return true if weapon has ammo.
    inline bool HasAmmo() const { return HasAmmo(0) || HasAmmo(1); }

    /// Return true if this weapon has no bullets.
    inline bool Empty() const { return !IsMelee() && !IsPhysics() && !HasAmmo(); }

    /// Return time when weapon can be used.
    inline float GetEndTime() const { return m_fEndTime; }

    /// Return true if weapon can be changed.
    inline bool CanChange() const { return !IsShooting(); }

    /// Return true if weapon is not reloading or shooting or changing zoom or changing to other weapon.
    inline bool CanUse() const
    {
        // Shotgun and rocket types can stop reloading and shoot if weapon has bullets in clip.
        return !IsShooting() && !IsChanging() && ( !IsReloading() || ( (m_pWeapon->iType >= EWeaponShotgun) && Bullets(0) ) );
    }

    /// Return true if weapon can be used from distance to enemy.
    inline bool CanBeUsed( float fDistanceSqrToEnemy ) const
    {
        return ( HasAmmoInClip(CWeapon::PRIMARY) && IsDistanceSafe(fDistanceSqrToEnemy, CWeapon::PRIMARY) ) ||
               ( HasAmmoInClip(CWeapon::SECONDARY) && IsDistanceSafe(fDistanceSqrToEnemy, CWeapon::SECONDARY) );
    }

    // 
    inline bool NeedsToBeCloser( float fDistanceSqrToEnemy ) const
    {
        return ( HasAmmoInClip(CWeapon::PRIMARY) && (m_pWeapon->fMaxDistanceSqr[CWeapon::PRIMARY] < fDistanceSqrToEnemy) ) ||
               ( HasAmmoInClip(CWeapon::SECONDARY) && (m_pWeapon->fMaxDistanceSqr[CWeapon::SECONDARY] < fDistanceSqrToEnemy) );
    }

    /// Return true if can start shooting at enemy right away.
    inline bool CanShoot( int iSecondary, float fDistanceSqr ) const
    {
        GoodAssert( CanUse() );
        return HasAmmoInClip(iSecondary) && IsDistanceSafe(fDistanceSqr, iSecondary);
    }

    /// Set weapon presence.
    inline void SetPresent( bool bPresent ) { m_bWeaponPresent = bPresent; }

    /// Remove all bullets for this weapon.
    inline void SetEmpty() { m_iBulletsInClip[0] = m_iBulletsInClip[1] = 0; m_iBulletsExtra[0] = m_iBulletsExtra[1] = 0; }

    /// Return damage per second this weapon can do with primary ammo.
    inline float DamagePerSecond() const { return m_pWeapon->fDamage[0] / m_pWeapon->fShotTime[0]; }

    /// Return amount of bullets this weapon has for primary or secondary attacks.
    inline int Bullets( int iSecondary ) const { return m_iBulletsInClip[iSecondary]; }

    /// Return amount of extra bullets this weapon has for primary or secondary attacks.
    inline int ExtraBullets( int iSecondary ) const { return m_iBulletsExtra[iSecondary]; }

    /// Return approximate damage this weapon can do with bullets in clip.
    inline float Damage( int iSecondary ) const { return m_pWeapon->fDamage[iSecondary]; }

    /// Return approximate damage this weapon can do with bullets in clip.
    inline float TotalDamage( int iSecondary ) const { return Bullets(iSecondary) * Damage(iSecondary); }

    /// Return approximate damage this weapon can do without reload.
    inline float TotalDamage() const { return TotalDamage(0) + TotalDamage(1); }

    /// Return true if weapon needs to be reloaded.
    bool NeedReload( int iSecondary ) const { return (m_iBulletsInClip[iSecondary] < m_pWeapon->iClipSize[iSecondary]) && HasAmmoExtra(iSecondary); }

    /// Return true if weapon should be reloaded.
    bool ShouldReload( int iSecondary ) const { return (m_iBulletsInClip[iSecondary] == 0) && HasAmmoExtra(iSecondary); }

    /// Return true if need to use zoom.
    bool ShouldZoom( float fDistanceToEnemySqr ) const
    {
        BASSERT( IsSniper(), return false );
        return (fDistanceToEnemySqr >= m_pWeapon->fMinDistanceSqr[1]);
    }

    /// Start to shoot weapon.
    void Shoot( int iSecondary );

    /// Start to reload weapon.
    void Reload( int iSecondary );

    /// Remove this weapon (called when player's team is different from this weapons's team).
    void RemoveWeapon() { m_bWeaponPresent = false; }

    /// Called when bot picks up this weapon.
    void AddWeapon();

    /// Add bullets to this weapon.
    void AddBullets( int iCount, int iSecondary )
    {
        m_iBulletsExtra[iSecondary] += iCount;
        if ( m_iBulletsExtra[iSecondary] > m_pWeapon->iMaxAmmo[iSecondary] )
            m_iBulletsExtra[iSecondary] = m_pWeapon->iMaxAmmo[iSecondary];
    }

    /// Change weapon.
    static void Holster( CWeaponWithAmmo* pSwitchFrom, CWeaponWithAmmo& cSwitchTo );

    /// Zoom in.
    void ToggleZoom()
    {
        GoodAssert( CanUse() && IsSniper() );
        Shoot(CWeapon::SECONDARY);
        m_bUsingZoom = !m_bUsingZoom;
    }

    /// Get where to look at enemy according to aim.
    void GetLook( const Vector& vFrom, const CPlayer* pTo, TBotIntelligence iIntelligence, int iSecondary, Vector& vResult ) const;

    /**
     * @brief Get vector to aim to.
     *
     * iBotIntelligence is used to get some errors in aim. Returns false if can't use
     * (invalid distance). Will return vector, with random based error and bot intelligence.
     */
    bool GetLook( const Vector& vFrom, const CPlayer* pTo, float fDistanceSqr,
                  TBotIntelligence iBotIntelligence, int iSecondary, Vector& vResult ) const;

protected:
    // End reloading weapon.
    void EndReload();

    // End holding attack button.
    void EndHold();

    // End using weapon function.
    void EndShoot();

    const CWeapon* m_pWeapon; ///< Weapon itself.
    int m_iBulletsInClip[2];  ///< Bullets in current clip (inside weapon).
    int m_iBulletsExtra[2];   ///< Amount of bullets extra.
    int m_iSecondary;         ///< Reloading / shooting secondary ammo or using zoom.

    float m_fEndTime;         ///< Time to end reloading/shooting.

    bool m_bWeaponPresent:1;  ///< True, if weapon is present, false if only ammo is present.
    bool m_bReloadingStart:1; ///< True, if started to reload weapon.
    bool m_bReloading:1;      ///< True, if continuing to reload weapon.
    bool m_bHolding:1;        ///< True, if currently holding attack button.
    bool m_bShooting:1;       ///< True, if currently shooting.
    bool m_bChanging:1;       ///< True, if started to change weapon.
    bool m_bUsingZoom:1;      ///< True, if started to zoom in / zoom out.
};


typedef good::unique_ptr<CWeaponWithAmmo> CWeaponWithAmmoPtr; ///< Unique pointer for weapon with ammo.



//****************************************************************************************************************
/// Available weapons.
//****************************************************************************************************************
class CWeapons
{

public:
	/// Return true if this weapon id is valid.
	static bool IsValid(TWeaponId iId) { return 0 <= iId && iId < Size(); }

    /// Return weapons count.
	static inline int Size() { return m_aWeapons.size(); }

    /// Get weapon from weapon id.
    static const CWeaponWithAmmo* Get( TWeaponId iWeaponId ) { return iWeaponId < 0 || iWeaponId >= m_aWeapons.size() ? NULL : &m_aWeapons[iWeaponId]; }

    /// Add weapon.
    static TWeaponId Add( CWeaponWithAmmo& cWeapon ) { m_aWeapons.push_back(cWeapon); return m_aWeapons.size()-1; }

    /// Clear all weapons.
    static void Clear()
    {
        m_aWeapons.reserve(16);
        for ( int i=0; i < m_aWeapons.size(); ++i )
            delete m_aWeapons[i].GetBaseWeapon();
        m_aWeapons.clear();
    }

    /// Get default weapons with which player respawns.
    static void GetRespawnWeapons( good::vector<CWeaponWithAmmo>& aWeapons, TTeam iTeam, TClass iClass );

    /// Get weapon from weapon name.
    static TWeaponId GetIdFromWeaponName( const good::string& sName )
    {
        for ( int i=0; i < m_aWeapons.size(); ++i )
            if ( m_aWeapons[i].GetName() == sName )
                return i;
        return EWeaponIdInvalid;
    }

    /// Get weapon from weapon class. Faster.
    static TWeaponId GetIdFromWeaponClass( const CItemClass* pWeaponClass )
    {
        for ( int i=0; i < m_aWeapons.size(); ++i )
            if ( m_aWeapons[i].GetBaseWeapon()->pWeaponClass == pWeaponClass )
                return i;
        return EWeaponIdInvalid;
    }

    /// Add weapon to weapons.
    static TWeaponId AddWeapon( const CItemClass* pWeaponClass, good::vector<CWeaponWithAmmo>& aWeapons )
    {
        for ( TWeaponId iWeapon = 0; iWeapon < aWeapons.size(); ++iWeapon )
        {
            if ( aWeapons[iWeapon].GetBaseWeapon()->pWeaponClass == pWeaponClass )
            {
                aWeapons[iWeapon].AddWeapon();
                return iWeapon;
            }
        }
        return EWeaponIdInvalid;
    }

    /// Add ammo to weapons.
    static bool AddAmmo( const CItemClass* pAmmoClass, good::vector<CWeaponWithAmmo>& aWeapons );

    /// Allow given weapon.
    static void Allow( TWeaponId iWeaponId ) { ((CWeapon*)m_aWeapons[iWeaponId].GetBaseWeapon())->bForbidden = false; }

    /// Forbid given weapon.
    static void Forbid( TWeaponId iWeaponId ) { ((CWeapon*)m_aWeapons[iWeaponId].GetBaseWeapon())->bForbidden = true; }

    /// Get best ranged weapon.
    static TWeaponId GetBestWeapon( const good::vector<CWeaponWithAmmo>& aWeapons );

    /// Get random weapon, based on bot intelligence.
    static TWeaponId GetRandomWeapon( TBotIntelligence iIntelligence, const good::bitset& cSkipWeapons );

protected:
    static good::vector< CWeaponWithAmmo > m_aWeapons; // Array of available weapons for this mod.
};


#endif // __BOTRIX_WEAPONS_H__
