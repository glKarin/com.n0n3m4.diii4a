// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __GAME_INTERFACES_USABLEINTERFACE_H__
#define __GAME_INTERFACES_USABLEINTERFACE_H__

class sdWeaponLockInfo;
class sdEntityDisplayIconInfo;


class sdUsableInterface {
public:
	virtual bool						OnExit( idPlayer* player, bool force = false ) = 0;
	virtual void						SwapViewMode( idPlayer* player ) = 0;

	virtual void						BecomeActiveViewProxy( idPlayer* viewPlayer ) = 0;
	virtual void						StopActiveViewProxy( void ) = 0;
	virtual void						UpdateProxyView( idPlayer* viewPlayer ) = 0;

	virtual bool						GetHideHud( idPlayer* player ) const = 0;
	virtual bool						GetSensitivity( idPlayer* player, float& sensX, float& sensY ) const = 0;
	virtual bool						GetShowPlayer( idPlayer* player ) const = 0;
	virtual bool						GetShowPlayerShadow( idPlayer* player ) const = 0;
	virtual float						GetFov( idPlayer* player ) const = 0;
	virtual void						ClampViewAngles( idPlayer* player, const idAngles& oldViewAngles ) const = 0;
	virtual bool						GetShowCrosshair( idPlayer* player ) const = 0;
	virtual bool						GetHideDecoyInfo( idPlayer* player ) const = 0;
	virtual bool						GetShowTargetingInfo( idPlayer* player ) const = 0;
	virtual playerStance_t				GetPlayerStance( idPlayer* player ) const = 0;
	virtual jointHandle_t				GetPlayerIconJoint( idPlayer* player ) const = 0;
	
	virtual float						GetDamageScale( idPlayer* player ) const = 0;

	virtual void						UpdateHud( idPlayer* player, guiHandle_t handle ) = 0;
	virtual void						UpdateViewAngles( idPlayer* player ) = 0;
	virtual void						UpdateViewPos( idPlayer* player, idVec3& origin, idMat3& axis, bool fullUpdate ) = 0;
	virtual void						CalculateRenderView( idPlayer* player, renderView_t& renderView ) = 0;
	virtual const idAngles				GetRequiredViewAngles( idPlayer* player, idVec3& target ) = 0;

	virtual int							GetDestructionEndTime( void ) const = 0;
	virtual bool						GetDirectionWarning( void ) const = 0;
	virtual int							GetRouteKickDistance( void ) const = 0;

	virtual const sdDeclGUI*			GetOverlayGUI( void ) const = 0;

	virtual bool						HasAbility( qhandle_t handle, const idPlayer* player ) const = 0;

	virtual void						ForcePlacement( idPlayer* other, int index, int oldIndex, bool keepCamera ) = 0;
	virtual void						FindPositionForPlayer( idPlayer* player ) = 0;
	virtual int							GetNumPositions() const = 0;
	virtual idPlayer*					GetPlayerAtPosition( int i ) const = 0;
	virtual const sdDeclLocStr*			GetPositionTitle( int i ) const = 0;

	virtual idPlayer*					GetXPSharer( float& shareFactor ) = 0;

	virtual void						SwapPosition( idPlayer* player ) = 0;
	virtual void						SelectWeapon( idPlayer* player, int index ) = 0;	

	virtual const sdWeaponLockInfo*		GetWeaponLockInfo( idPlayer* player ) const = 0;	

	virtual void						NextWeapon( idPlayer* player ) const = 0;
	virtual void						PrevWeapon( idPlayer* player ) const = 0;

	virtual bool						GetAllowPlayerMove( idPlayer* player ) const = 0;
	virtual bool						GetAllowPlayerDamage( idPlayer* player ) const = 0;
	virtual bool						GetAllowPlayerWeapon( idPlayer* player ) const = 0;
	virtual bool						GetAllowAdjustBodyAngles( idPlayer* player ) const = 0;
	virtual const sdDeclLocStr*			GetWeaponName( const idPlayer* player ) const = 0;
	virtual const char*					GetWeaponLookupName( const idPlayer* player ) const = 0;

	virtual void						OnPostThink( void ) = 0;
};

class sdInteractiveInterface {
public:
	virtual bool						OnActivate( idPlayer* player, float distance ) = 0;
	virtual bool						OnActivateHeld( idPlayer* player, float distance ) = 0;
	virtual bool						OnUsed( idPlayer* player, float distance ) = 0;
};

class sdEntityDisplayIconInterface {
public:
	virtual bool						HasIcon( idPlayer* viewer, sdWorldToScreenConverter& converter ) = 0;
	virtual bool						GetEntityDisplayIconInfo( idPlayer* viewer, sdWorldToScreenConverter& converter, sdEntityDisplayIconInfo& iconInfo ) = 0;
};

#endif // __GAME_INTERFACES_USABLEINTERFACE_H__
