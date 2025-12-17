#include "players.h"
#include "source_engine.h"
#include "weapon.h"

#include "in_buttons.h"


#if defined(DEBUG) || defined(_DEBUG)
    #define WEAPON_TRACE(...)    BLOG_T(__VA_ARGS__)
#else
    #define WEAPON_TRACE(...)
#endif

//----------------------------------------------------------------------------------------------------------------
good::vector<CWeaponWithAmmo> CWeapons::m_aWeapons;


//----------------------------------------------------------------------------------------------------------------
void CWeaponWithAmmo::GameFrame( int& iButtons )
{
    if ( CBotrixPlugin::fTime >= m_fEndTime )
    {
        m_bShooting = false;
        m_bChanging = false;

        if ( m_bHolding )
        {
            m_bHolding = false;
            EndHold();
        }

        else if ( m_bReloadingStart )
        {
            WEAPON_TRACE( "%.5f end reload start.", CBotrixPlugin::fTime );
            m_bReloadingStart = false;
            m_bReloading = true;
            m_fEndTime = CBotrixPlugin::fTime + m_pWeapon->fReloadTime[m_iSecondary];
        }

        else if ( m_bReloading )
            EndReload();

        // Check to reload secondary ammo. It is automatic, no need to press reload button.
        else if ( !HasAmmoInClip(1) && HasAmmoExtra(1) )
            Reload(1);

        // Check to reload primary ammo.
        else if ( !HasAmmoInClip(0) && HasAmmoExtra(0) )
        {
            Reload(0);
            FLAG_SET(IN_RELOAD, iButtons);
        }
    }
    else
    {
        if ( m_bHolding )
        {
            if ( m_iSecondary )
                FLAG_SET(IN_ATTACK2, iButtons);
            else
                FLAG_SET(IN_ATTACK, iButtons);
        }
        else if ( m_bShooting )
        {
            if ( !m_iSecondary && (m_pWeapon->iType == EWeaponRifle) )
                FLAG_SET(IN_ATTACK, iButtons);
        }
    }
}


//----------------------------------------------------------------------------------------------------------------
void CWeaponWithAmmo::Shoot( int iSecondary )
{
    GoodAssert( CanUse() && ( HasAmmoInClip(iSecondary) || IsMelee() || IsPhysics() || FLAG_SOME_SET(FWeaponHasSecondary, m_pWeapon->iFlags[iSecondary]) ) );

    m_bReloading = m_bReloadingStart = false; // Stop reloading if weapon time is shotgun-like.
    m_iSecondary = iSecondary;

    float fHold = m_pWeapon->fHoldTime[iSecondary];
    if ( fHold )
    {
        WEAPON_TRACE( "%.5f hold %d.", CBotrixPlugin::fTime, iSecondary );
        m_bHolding = true;
        m_fEndTime = CBotrixPlugin::fTime + fHold;
    }
    else
        EndHold();
}


//----------------------------------------------------------------------------------------------------------------
void CWeaponWithAmmo::EndHold()
{
    // Shotgun type: uses same bullets for primary and secondary attack.
    if ( m_iSecondary && ( FLAG_SOME_SET(FWeaponSameBullets, m_pWeapon->iFlags[m_iSecondary]) ) )
        m_iBulletsInClip[1-m_iSecondary] -= m_pWeapon->iAttackBullets[m_iSecondary];
    else
        m_iBulletsInClip[m_iSecondary] -= m_pWeapon->iAttackBullets[m_iSecondary];

    float fShotTime = m_pWeapon->fShotTime[m_iSecondary];
    if ( fShotTime )
    {
        WEAPON_TRACE( "Time %.5f shoot %d.", CBotrixPlugin::fTime, m_iSecondary );
        m_bShooting = true;
        m_fEndTime = CBotrixPlugin::fTime + fShotTime;
    }
}


//----------------------------------------------------------------------------------------------------------------
void CWeaponWithAmmo::Reload( int iSecondary )
{
    GoodAssert( !IsReloading() && CanUse() && NeedReload(iSecondary) );
    m_iSecondary = iSecondary;
    if ( m_pWeapon->fReloadStartTime[iSecondary] )
    {
        WEAPON_TRACE( "%.5f reload start.", CBotrixPlugin::fTime );
        m_bReloadingStart = true;
        m_fEndTime = CBotrixPlugin::fTime + m_pWeapon->fReloadStartTime[iSecondary];
    }
    else
    {
        WEAPON_TRACE( "%.5f reload.", CBotrixPlugin::fTime );
        m_bReloading = true;
        if ( m_pWeapon->fReloadTime[iSecondary] )
            m_fEndTime = CBotrixPlugin::fTime + m_pWeapon->fReloadTime[iSecondary];
        else
            EndReload();
    }
}


//----------------------------------------------------------------------------------------------------------------
void CWeaponWithAmmo::AddWeapon()
{
    AddBullets(m_pWeapon->iDefaultAmmo[0], 0);
    AddBullets(m_pWeapon->iDefaultAmmo[1], 1);

    if ( m_bWeaponPresent ) // Just add bullets.
    {
        if ( FLAG_CLEARED(FWeaponDontAddClip, m_pWeapon->iFlags[0]) )
            AddBullets(m_pWeapon->iClipSize[0], 0);
    }
    else
    {
        m_iBulletsInClip[0] = m_pWeapon->iClipSize[0]; // Picked weapon has full clip.
        m_bWeaponPresent = true;
    }
}


//----------------------------------------------------------------------------------------------------------------
void CWeaponWithAmmo::Holster( CWeaponWithAmmo* pSwitchFrom, CWeaponWithAmmo& cSwitchTo )
{
    if ( pSwitchFrom )
    {
        //GoodAssert( !pSwitchFrom->m_bShooting ); // Happends when weapon is out of bullets and switched automatically.

        pSwitchFrom->m_bChanging = pSwitchFrom->m_bHolding = pSwitchFrom->m_bShooting = pSwitchFrom->m_bReloading = pSwitchFrom->m_bUsingZoom = false;
        pSwitchFrom->m_fEndTime = CBotrixPlugin::fTime + cSwitchTo.m_pWeapon->fReloadTime[0]; // Save time for flag FWeaponBackgroundReload.
    }

    if ( FLAG_SOME_SET(FWeaponBackgroundReload, cSwitchTo.m_pWeapon->iFlags[0]) && !cSwitchTo.HasAmmoInClip(0) && cSwitchTo.HasAmmoExtra(0) &&
         (CBotrixPlugin::fTime >= cSwitchTo.m_fEndTime) )
    {
        cSwitchTo.m_bReloading = true;
        cSwitchTo.EndReload();
    }

    cSwitchTo.m_bChanging = true;
    cSwitchTo.m_fEndTime = CBotrixPlugin::fTime + cSwitchTo.m_pWeapon->fHolsterTime;
}


//----------------------------------------------------------------------------------------------------------------
void CWeaponWithAmmo::EndReload()
{
    GoodAssert( m_bReloading && (m_iBulletsInClip[m_iSecondary] < m_pWeapon->iClipSize[m_iSecondary]) );

    int iClipSize = m_pWeapon->iClipSize[m_iSecondary];
    int iReloadBy = MIN2( m_pWeapon->iReloadBy[m_iSecondary], m_iBulletsExtra[m_iSecondary]);
    int iLeft = iClipSize - m_iBulletsInClip[m_iSecondary];
    if ( iReloadBy > iLeft )
        iReloadBy = iLeft;

    m_iBulletsInClip[m_iSecondary] += iReloadBy;
    m_iBulletsExtra[m_iSecondary]  -= iReloadBy;
    if ( (m_iBulletsInClip[m_iSecondary] < iClipSize) && m_iBulletsExtra[m_iSecondary] )
    {
        WEAPON_TRACE( "%.5f end partial reload by %d.", CBotrixPlugin::fTime, iReloadBy );
        m_fEndTime = CBotrixPlugin::fTime + m_pWeapon->fReloadTime[m_iSecondary]; // Do next reload.
    }
    else
    {
        WEAPON_TRACE( "%.5f end reload by %d.", CBotrixPlugin::fTime, iReloadBy );
        m_bReloading = false;
    }
}


//----------------------------------------------------------------------------------------------------------------
void CWeaponWithAmmo::GetLook( const Vector& vFrom, const CPlayer* pTo, TBotIntelligence iIntelligence, int iSecondary, Vector& vResult ) const
{
    // Assume we can see enemy head.
    switch ( m_pWeapon->iAim[iSecondary] )
    {
    case EWeaponAimFoot:
        pTo->GetFoot(vResult);
        break;
    case EWeaponAimBody:
        // Rifle preference is body.
        if ( CMod::bHeadShotDoesMoreDamage && (iIntelligence >= EBotNormal) && (m_pWeapon->iType != EWeaponRifle) )
            vResult = pTo->GetHead();
        else
        {
            pTo->GetCenter(vResult);
            if ( !CUtil::IsVisible( vFrom, vResult, EVisibilityBots ) )
                vResult = pTo->GetHead();
        }
        break;
    case EWeaponAimHead:
        vResult = pTo->GetHead();
        break;
    default:
        GoodAssert(false);
    }
}

//----------------------------------------------------------------------------------------------------------------
bool CWeaponWithAmmo::GetLook( const Vector& vFrom, const CPlayer* pTo, float fDistanceSqr,
                               TBotIntelligence iBotIntelligence, int iSecondary, Vector& vResult ) const
{
    GoodAssert( IsDistanceSafe(fDistanceSqr, iSecondary) );

    float fParabolicDistance45 = m_pWeapon->iParabolicDistance45[iSecondary];
    if ( fParabolicDistance45 && (fDistanceSqr > SQR(fParabolicDistance45)) ) // Can't reach enemy.
        return false;

    Vector vTo(CUtil::vZero);
    GetLook(vFrom, pTo, iBotIntelligence, iSecondary, vTo);

    float fParabolicDistance0 = m_pWeapon->iParabolicDistance0[iSecondary];
    if ( fParabolicDistance0 || fParabolicDistance45 )
    {
        vTo -= vFrom; // Make vectors relative to player's eyes.

        float fGradeDist;
        float fDist = FastSqrt(fDistanceSqr);
        float fGradesInc;

        if ( fParabolicDistance0 <= fDist )
        {
            // Distance increment that 1 grade does looking up.
            fGradeDist = (fParabolicDistance45 - fParabolicDistance0) / 45.0f;
            fGradesInc = (fDist - fParabolicDistance0) / fGradeDist; // Grade we need to aim up.
        }
        else // Distance increment that 1 grade does looking down.
        {
            fGradeDist = fParabolicDistance0 / -45.0f;
            fGradesInc = fDist / fGradeDist; // Grade we need to aim down.
        }

        fGradesInc = fDist * FastCos(fGradesInc); // z = fDist*cos(angle)
        vTo.z += fGradesInc; // Aim more up/down.

        vTo += vFrom; // Make vectors relative to player's eyes.
    }

#if 0 // BOTRIX_BOT_AIM_ERROR
    static const float fMaxErrorDistance = CUtil::iHalfMaxMapSize; // Don't error after that.

    // Aim errors (pitch/yaw): max error for a distance = 1000, and max error for a distance = 0.
    static const float aAimErrors[EBotIntelligenceTotal][2] =
    {
        // Pitch(when distance is 0) Yaw(when distance is 0). When distance is 1000 both pitch and yaw is 1.
        { 40, 40, }, // I.e. fool bot at distance 0 will have error in 45 degrees for pitch and 30 for yaw.
        { 30, 30, }, // At distance 1000 error should not be greater than 1 degree.
        { 20, 20, },
        { 10, 10, },
        { 0,  0,  },
    };

    int iRangePitch = 1, iRangeYaw = 1;
    if ( fDistance < fMaxErrorDistance )
    {
        float fPercentage = fDistance / fMaxErrorDistance;
        iRangePitch = 2 * (int)(aAimErrors[iBotIntelligence][0] * fPercentage);
        iRangeYaw = 2 * (int)(aAimErrors[iBotIntelligence][1] * fPercentage);
    }

    int iRandPitch = (rand() % iRangePitch) - (iRangePitch >> 1); // 45 degrees, errors = 22.5 up and 22.5 down
    int iRandYaw = (rand() % iRangeYaw) - (iRangeYaw >> 1);

    angResult.x += iRandPitch;
    angResult.y += iRandYaw;
#endif // BOTRIX_BOT_AIM_ERROR

    vResult = vTo;
    return true;
}


//----------------------------------------------------------------------------------------------------------------
void CWeapons::GetRespawnWeapons( good::vector<CWeaponWithAmmo>& aWeapons, TTeam iTeam, TClass iClass )
{
    aWeapons.clear();
    aWeapons.reserve( 8 );

    int iTeamFlag = 1 << iTeam;
    int iClassFlag = 1 << iClass;
    for ( int i=0; i < m_aWeapons.size(); ++i )
    {
        if ( FLAG_SOME_SET(iTeamFlag, m_aWeapons[i].GetBaseWeapon()->iTeam) &&
             FLAG_SOME_SET(iClassFlag, m_aWeapons[i].GetBaseWeapon()->iClass) )
            aWeapons.push_back( m_aWeapons[i] );
    }
}


//----------------------------------------------------------------------------------------------------------------
bool CWeapons::AddAmmo( const CItemClass* pAmmoClass, good::vector<CWeaponWithAmmo>& aWeapons )
{
    bool bResult = false;
    for ( int i=0; i < aWeapons.size(); ++i )
    {
        const good::vector<const CItemClass*>* aAmmos = aWeapons[i].GetBaseWeapon()->aAmmos;
        const good::vector<int>* aAmmoBullets = aWeapons[i].GetBaseWeapon()->aAmmoBullets;
        for ( int iSec=CWeapon::PRIMARY; iSec <= CWeapon::SECONDARY; ++iSec )
            for ( int j=0; j < aAmmos[iSec].size(); ++j )
                if ( aAmmos[iSec][j] == pAmmoClass )
                {
                    int iAmmoCount = aAmmoBullets[iSec][j];
                    aWeapons[i].AddBullets(iAmmoCount, iSec);
                    bResult = true;
                }
    }
    return bResult;
}


//----------------------------------------------------------------------------------------------------------------
TWeaponId CWeapons::GetBestWeapon( const good::vector<CWeaponWithAmmo>& aWeapons )
{
    // Choose best weapon.
    /*
    bool bCanKill = false, bOneBullet = false;
    float fDamagePerSec = 0.0f, fDamage = 0.0f;
    */
    TWeaponId iIdx = EWeaponIdInvalid;
    TBotIntelligence iPreference = -1;

    for ( TWeaponId i = 0; i < aWeapons.size(); ++i )
    {
        const CWeaponWithAmmo& cWeapon = aWeapons[i];
        const CWeapon* pWeapon = cWeapon.GetBaseWeapon();

        if ( !pWeapon->bForbidden && cWeapon.IsPresent() &&
             (iPreference < pWeapon->iBotPreference) &&
             ( ( cWeapon.HasAmmo(CWeapon::PRIMARY) && (cWeapon.Damage(CWeapon::PRIMARY) > 0.0f) ) ||
               ( cWeapon.HasAmmo(CWeapon::SECONDARY) && (cWeapon.Damage(CWeapon::SECONDARY) > 0.0f) ) ) )
        {
            iIdx = i;
            iPreference = cWeapon.IsMelee() ? -1 : pWeapon->iBotPreference; // Prefer non melee weapons.
        }

        /*
        if ( cWeapon.GetBaseWeapon()->bForbidden || !cWeapon.IsPresent() ||
            !cWeapon.IsRanged() || !cWeapon.HasAmmo() ) // Skip all melees, grenades and physics or without ammo.
            continue;

        float fDamage0 = cWeapon.Damage(0);
        float fDamage1 = cWeapon.Damage(1);
        bool bOneBullet0 = (fDamage0 >= CMod::GetVar( EModVarPlayerMaxHealth ));
        bool bOneBullet1 = (fDamage1 >= CMod::GetVar( EModVarPlayerMaxHealth ));

        // Check if can kill with one bullet first.
        if ( bOneBullet0 && cWeapon.HasAmmoInClip(0) && (!bOneBullet || (fDamage < fDamage0)) )
        {
            iIdx = i;
            bOneBullet = bCanKill = true;
            fDamage = fDamage0;
            continue;
        }
        if ( bOneBullet1 && cWeapon.HasAmmoInClip(1) && (!bOneBullet || (fDamage < fDamage1))  )
        {
            iIdx = i;
            bOneBullet = bCanKill = true;
            fDamage = fDamage1;
            continue;
        }

        if ( bOneBullet ) // We have founded previously weapon that can kill with one bullet.
            continue;

        // Check if weapon has sufficient bullets to kill.
        fDamage0 = cWeapon.TotalDamage();
        bool bCanKill0 = (fDamage0 >= CMod::GetVar( EModVarPlayerMaxHealth )); // Has enough bullets to kill?
        float fDamagePerSec0 = cWeapon.DamagePerSecond(); // How fast this weapon kills.

        if ( bCanKill0 && (!bCanKill || (fDamagePerSec < fDamagePerSec0)) )
        {
            iIdx = i;
            bCanKill = true;
            fDamagePerSec = fDamagePerSec0;
            continue;
        }

        if ( bCanKill ) // We have founded previously weapon that has enough bullets to kill.
            continue;

        // Check total damage.
        if ( fDamage < fDamage0 ) // Previous weapon does less damage.
        {
            iIdx = i;
            fDamage = fDamage0;
        }
        */
    }
    return iIdx;
}


//----------------------------------------------------------------------------------------------------------------
TWeaponId CWeapons::GetRandomWeapon( TBotIntelligence iIntelligence, const good::bitset& cSkipWeapons )
{
    int iSize = MIN2( cSkipWeapons.size(), Size() );

    TWeaponId iIdx = rand() % iSize;
    for ( TWeaponId i = iIdx+1; i < iSize; ++i )
    {
        CWeaponWithAmmo& cWeapon = m_aWeapons[i];
        const CWeapon* pWeapon = cWeapon.GetBaseWeapon();
        if ( !pWeapon->bForbidden && cWeapon.IsRanged() && (iIntelligence <= pWeapon->iBotPreference) &&
             CItems::ExistsOnMap(pWeapon->pWeaponClass) && !cSkipWeapons.test(i) )
            return i;
    }
    for ( TWeaponId i = iIdx; i >= 0; --i )
    {
        CWeaponWithAmmo& cWeapon = m_aWeapons[i];
        const CWeapon* pWeapon = cWeapon.GetBaseWeapon();
        if ( !pWeapon->bForbidden && cWeapon.IsRanged() && (iIntelligence <= pWeapon->iBotPreference) &&
             CItems::ExistsOnMap(pWeapon->pWeaponClass) && !cSkipWeapons.test(i) )
            return i;
    }

    return -1;
}
