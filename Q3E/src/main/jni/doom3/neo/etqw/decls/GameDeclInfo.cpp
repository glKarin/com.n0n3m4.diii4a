// Copyright (C) 2007 Id Software, Inc.
//


#include "precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "../vehicles/Transport.h"
#include "../guis/UserInterfaceLocal.h"
#include "../guis/UserInterfaceManagerLocal.h"

#include "../anim/Anim.h"

#include "../decls/declVehicleScript.h"

#include "GameDeclIdentifiers.h"

sdDeclInfo declModelDefInfo(			declModelDefIdentifier,				DIF_ALLOW_TEMPLATES | DIF_WRITE_BINARY, idDeclModelDef::CacheFromDict );
sdDeclInfo declExportDefInfo(			declExportDefIdentifier,			DIF_SKIP_PARSING | DIF_SKIP_CHECKSUM );
sdDeclInfo declVehicleScriptDefInfo(	declVehicleScriptDefIdentifier,		DIF_ALLOW_TEMPLATES | DIF_WRITE_BINARY, sdDeclVehicleScript::CacheFromDict,	sdTransport::ReloadVehicleScripts );
sdDeclInfo declAmmoTypeInfo(			declAmmoTypeIdentifier,			 	DIF_ALLOW_TEMPLATES | DIF_NOT_PRECACHED );
sdDeclInfo declInvSlotInfo(				declInvSlotIdentifier,			 	DIF_ALLOW_TEMPLATES | DIF_WRITE_BINARY | DIF_NOT_PRECACHED );
sdDeclInfo declInvItemTypeInfo(			declInvItemTypeIdentifier,		 	DIF_ALLOW_TEMPLATES | DIF_WRITE_BINARY | DIF_NOT_PRECACHED );
sdDeclInfo declInvItemInfo(				declInvItemIdentifier,			 	DIF_ALLOW_TEMPLATES | DIF_WRITE_BINARY );
sdDeclInfo declItemPackageInfo(			declItemPackageIdentifier,		 	DIF_ALLOW_TEMPLATES | DIF_WRITE_BINARY, sdDeclItemPackage::CacheFromDict );
sdDeclInfo declStringMapInfo(			declStringMapIdentifier,			DIF_ALLOW_TEMPLATES | DIF_WRITE_BINARY | DIF_NOT_PRECACHED, sdDeclStringMap::CacheFromDict );
sdDeclInfo declDamageInfo(				declDamageIdentifier,				DIF_ALLOW_TEMPLATES | DIF_WRITE_BINARY | DIF_NOT_PRECACHED, sdDeclDamage::CacheFromDict );
sdDeclInfo declDamageFilterInfo(		declDamageFilterIdentifier,		 	DIF_ALLOW_TEMPLATES | DIF_WRITE_BINARY | DIF_NOT_PRECACHED );
sdDeclInfo declCampaignInfo(			declCampaignIdentifier			 	);
sdDeclInfo declQuickChatInfo(			declQuickChatIdentifier			 	);
sdDeclInfo declMapInfoInfo(				declMapInfoIdentifier				);
sdDeclInfo declToolTipInfo(				declToolTipIdentifier,			 	DIF_ALLOW_TEMPLATES | DIF_NOT_PRECACHED, sdDeclToolTip::CacheFromDict );
sdDeclInfo declTargetInfoInfo(			declTargetInfoIdentifier,			DIF_ALLOW_TEMPLATES | DIF_WRITE_BINARY | DIF_NOT_PRECACHED, sdDeclTargetInfo::CacheFromDict );
sdDeclInfo declProficiencyTypeInfo(		declProficiencyTypeIdentifier,		DIF_ALLOW_TEMPLATES | DIF_WRITE_BINARY | DIF_NOT_PRECACHED );
sdDeclInfo declProficiencyItemInfo(		declProficiencyItemIdentifier,		DIF_ALLOW_TEMPLATES | DIF_WRITE_BINARY | DIF_NOT_PRECACHED );
sdDeclInfo declRankInfo(				declRankIdentifier,					DIF_ALLOW_TEMPLATES | DIF_WRITE_BINARY | DIF_NOT_PRECACHED );
sdDeclInfo declDeployableObjectInfo(	declDeployableObjectIdentifier,		DIF_ALLOW_TEMPLATES, sdDeclDeployableObject::CacheFromDict );
sdDeclInfo declDeployableZoneInfo(		declDeployableZoneIdentifier,		DIF_ALLOW_TEMPLATES | DIF_WRITE_BINARY, sdDeclDeployableZone::CacheFromDict );
sdDeclInfo declPlayerClassInfo(			declPlayerClassIdentifier,			DIF_ALLOW_TEMPLATES | DIF_WRITE_BINARY, sdDeclPlayerClass::CacheFromDict );
sdDeclInfo declGUIInfo(					declGUIIdentifier,					DIF_ALLOW_TEMPLATES  | DIF_WRITE_BINARY | DIF_ALWAYS_GENERATE_BINARY, sdDeclGUI::CacheFromDict, sdUserInterfaceManagerLocal::OnReloadGUI );
sdDeclInfo declGUIThemeInfo(			declGUIThemeIdentifier,				DIF_ALLOW_TEMPLATES, sdDeclGUITheme::CacheFromDict, sdDeclGUITheme::OnReloadGUITheme );
sdDeclInfo declTeamInfoInfo(			declTeamInfoIdentifier,				DIF_ALLOW_TEMPLATES | DIF_WRITE_BINARY | DIF_NOT_PRECACHED );
sdDeclInfo declPlayerTaskInfo(			declPlayerTaskIdentifier,			DIF_ALLOW_TEMPLATES | DIF_WRITE_BINARY | DIF_NOT_PRECACHED, sdDeclPlayerTask::CacheFromDict );
sdDeclInfo declRequirementInfo(			declRequirementIdentifier,			DIF_ALLOW_TEMPLATES | DIF_WRITE_BINARY | DIF_NOT_PRECACHED );
sdDeclInfo declVehiclePathInfo(			declVehiclePathIdentifier			);
sdDeclInfo declKeyBindingInfo(			declKeyBindingIdentifier,			DIF_NOT_PRECACHED );
sdDeclInfo declRadialMenuInfo(			declRadialMenuIdentifier,			DIF_ALLOW_TEMPLATES | DIF_WRITE_BINARY | DIF_SKIP_CHECKSUM | DIF_NOT_PRECACHED );
sdDeclInfo declAreaOfRelevanceInfo(		declAreaOfRelevanceIdentifier,		DIF_ALLOW_TEMPLATES | DIF_WRITE_BINARY | DIF_NOT_PRECACHED );
sdDeclInfo declRatingInfo(				declRatingIdentifier,				DIF_ALLOW_TEMPLATES | DIF_WRITE_BINARY | DIF_NOT_PRECACHED );
sdDeclInfo declHeightMapInfo(			declHeightMapIdentifier,			DIF_ALLOW_TEMPLATES | DIF_WRITE_BINARY, sdDeclHeightMap::CacheFromDict );
sdDeclInfo declDeployMaskInfo(			declDeployMaskIdentifier,			DIF_ALLOW_TEMPLATES | DIF_WRITE_BINARY, sdDeclDeployMask::CacheFromDict );

const char* gameDeclIdentifierList[ DECLTYPE_GAME_NUM_TYPES ] = {
	declModelDefIdentifier,
	declExportDefIdentifier,
	declVehicleScriptDefIdentifier,
	declAmmoTypeIdentifier,
	declInvSlotIdentifier,
	declInvItemTypeIdentifier,
	declInvItemIdentifier,
	declItemPackageIdentifier,
	declStringMapIdentifier,
	declDamageIdentifier,
	declDamageFilterIdentifier,
	declCampaignIdentifier,
	declQuickChatIdentifier,
	declMapInfoIdentifier,
	declToolTipIdentifier,
	declTargetInfoIdentifier,
	declProficiencyTypeIdentifier,
	declProficiencyItemIdentifier,
	declRankIdentifier,
	declDeployableObjectIdentifier,
	declDeployableZoneIdentifier,
	declPlayerClassIdentifier,
	declGUIIdentifier,
	declTeamInfoIdentifier,
	declPlayerTaskIdentifier,
	declRequirementIdentifier,
	declGUIThemeIdentifier,
	declVehiclePathIdentifier,
	declKeyBindingIdentifier,
	declRadialMenuIdentifier,
	declAreaOfRelevanceIdentifier,
	declRatingIdentifier,
	declHeightMapIdentifier,
	declDeployMaskIdentifier,
};
