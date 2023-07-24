// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __DECLPLAYERTASK_H__
#define __DECLPLAYERTASK_H__

enum playerTaskType_t {
	PTT_MISSION,
	PTT_TASK,
	PTT_OBJECTIVE,
};

class sdDeclPlayerTask : public idDecl {
public:
									sdDeclPlayerTask( void );
	virtual							~sdDeclPlayerTask( void );

	virtual const char*				DefaultDefinition( void ) const;
	virtual bool					Parse( const char *text, const int textLength );
	virtual void					FreeData( void );

	void							ParseFromDict( const idDict& dict );

	static void						CacheFromDict( const idDict& dict );

	int								GetPriority( void ) const { return priority; }
	const sdRequirementContainer&	GetEligibility( void ) const { return eligibility; }
	const sdDeclLocStr*				GetTitle( void ) const { return title; }
	const sdDeclLocStr*				GetFriendlyTitle( void ) const { return friendlyTitle; }
	const sdDeclLocStr*				GetCompletedTitle( void ) const { return completedTitle; }
	const sdDeclLocStr*				GetCompletedFriendlyTitle( void ) const { return completedFriendlyTitle; }
	int								GetTimeLimit( void ) const { return timeLimit; }
	int								GetNumWayPoints( void ) const { return waypointData.Num(); }
	const idDict&					GetWayPointData( int index ) const { return waypointData[ index ]; }
									
									// this can return NULL if no icon key was specified
	const idMaterial*				GetWaypointIcon( int index ) const { return waypointIcons[ index ]; }
	bool							IsMission( void ) const { return type == PTT_MISSION; }
	bool							IsObjective( void ) const { return type == PTT_OBJECTIVE; }
	bool							IsTask( void ) const { return type == PTT_TASK; }
	const char*						GetScriptObject( void ) const { return scriptObject; }
	const idDict&					GetData( void ) const { return info; }
	sdTeamInfo*						GetTeam( void ) const { return team; }
	bool							HasEligibleWayPoint( void ) const { return showEligibleWayPoints; }
	float							GetXPBonus( void ) const { return xpBonus; }
	const wchar_t*					GetXPString( void ) const { return xpString.c_str(); }
	bool							NoOcclusion( void ) const { return noOcclusion; }
	int								GetBotTaskType( void ) const { return botTaskType; }

private:
	const sdDeclLocStr*					title;
	const sdDeclLocStr*					friendlyTitle;
	const sdDeclLocStr*					completedTitle;
	const sdDeclLocStr*					completedFriendlyTitle;

	idStr								scriptObject;
	playerTaskType_t					type;
	int									timeLimit;
	int									priority;
	sdRequirementContainer				eligibility;
	bool								showEligibleWayPoints;
	float								xpBonus;
	idWStr								xpString;
	bool								noOcclusion;

	idListGranularityOne< idDict >		waypointData;
	idListGranularityOne< const idMaterial* >waypointIcons;

	int									botTaskType;

	sdTeamInfo*							team;
	idDict								info;
};

#endif // __DECLPLAYERTASK_H__
