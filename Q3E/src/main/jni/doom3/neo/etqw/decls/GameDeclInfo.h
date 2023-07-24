// Copyright (C) 2007 Id Software, Inc.
//


#ifndef __GAME_DECLINFO_H__
#define __GAME_DECLINFO_H__

extern sdDeclInfo declModelDefInfo;
extern sdDeclInfo declExportDefInfo;
extern sdDeclInfo declVehicleScriptDefInfo;
extern sdDeclInfo declAmmoTypeInfo;
extern sdDeclInfo declInvSlotInfo;
extern sdDeclInfo declInvItemTypeInfo;
extern sdDeclInfo declInvItemInfo;
extern sdDeclInfo declItemPackageInfo;
extern sdDeclInfo declStringMapInfo;
extern sdDeclInfo declDamageInfo;
extern sdDeclInfo declDamageFilterInfo;
extern sdDeclInfo declCampaignInfo;
extern sdDeclInfo declQuickChatInfo;
extern sdDeclInfo declMapInfoInfo;
extern sdDeclInfo declToolTipInfo;
extern sdDeclInfo declTargetInfoInfo;
extern sdDeclInfo declProficiencyTypeInfo;
extern sdDeclInfo declProficiencyItemInfo;
extern sdDeclInfo declRankInfo;
extern sdDeclInfo declDeployableObjectInfo;
extern sdDeclInfo declDeployableZoneInfo;
extern sdDeclInfo declPlayerClassInfo;
extern sdDeclInfo declGUIInfo;
extern sdDeclInfo declGUIThemeInfo;
extern sdDeclInfo declTeamInfoInfo;
extern sdDeclInfo declPlayerTaskInfo;
extern sdDeclInfo declRequirementInfo;
extern sdDeclInfo declVehiclePathInfo;
extern sdDeclInfo declKeyBindingInfo;
extern sdDeclInfo declRadialMenuInfo;
extern sdDeclInfo declAreaOfRelevanceInfo;
extern sdDeclInfo declRatingInfo;
extern sdDeclInfo declHeightMapInfo;
extern sdDeclInfo declDeployMaskInfo;

typedef enum gameDeclIdentifierType_e {
	DECLTYPE_MODEL,
	DECLTYPE_EXPORT,
	DECLTYPE_VEHICLESCRIPT,
	DECLTYPE_AMMOTYPE,
	DECLTYPE_INVSLOT,
	DECLTYPE_INVITEMTYPE,
	DECLTYPE_INVITEM,
	DECLTYPE_ITEMPACKAGE,
	DECLTYPE_STRINGMAP,
	DECLTYPE_DAMAGE,
	DECLTYPE_DAMAGEFILTER,
	DECLTYPE_CAMPAIGN,
	DECLTYPE_QUICKCHAT,
	DECLTYPE_MAPINFO,
	DECLTYPE_TOOLTIP,
	DECLTYPE_TARGETINFO,
	DECLTYPE_PROFICIENCYTYPE,
	DECLTYPE_PROFICIENCYITEM,
	DECLTYPE_RANKINFO,
	DECLTYPE_DEPLOYOBJECT,
	DECLTYPE_DEPLOYZONE,
	DECLTYPE_PLAYERCLASS,
	DECLTYPE_GUI,
	DECLTYPE_TEAMINFO,
	DECLTYPE_PLAYERTASK,
	DECLTYPE_REQUIREMENT,
	DECLTYPE_GUITHEME,
	DECLTYPE_VEHICLEPATH,
	DECLTYPE_KEYBINDING,
	DECLTYPE_RADIALMENU,
	DECLTYPE_AOR,
	DECLTYPE_RATING,
	DECLTYPE_HEIGHTMAP,
	DECLTYPE_DEPLOYMASK,
	DECLTYPE_GAME_NUM_TYPES,
} gameDeclIdentifierType_t;

extern const char* gameDeclIdentifierList[ DECLTYPE_GAME_NUM_TYPES ];


template< gameDeclIdentifierType_t INDEX >
void idListGameDecls_f( const idCmdArgs &args ) {
	declManager->ListType( args, gameDeclIdentifierList[ INDEX ] );
}

template< gameDeclIdentifierType_t INDEX >
void idPrintGameDecls_f( const idCmdArgs &args ) {
	declManager->PrintType( args, gameDeclIdentifierList[ INDEX ] );
}

template< gameDeclIdentifierType_t INDEX >
void idArgCompletionGameDecl_f( const idCmdArgs &args, argCompletionCallback_t callback ) {
	cmdSystem->ArgCompletion_DeclName( args, callback, gameDeclIdentifierList[ INDEX ] );
}

#endif // __GAME_DECLINFO_H__
