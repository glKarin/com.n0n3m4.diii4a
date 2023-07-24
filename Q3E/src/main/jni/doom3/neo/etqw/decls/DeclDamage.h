// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __DECLDAMAGE_H__
#define __DECLDAMAGE_H__

class sdDeclDamageFilter;
class idEntity;
class sdPlayerStatEntry;

class sdDeclDamage : public idDecl {
public:
	struct stats_t {
		sdPlayerStatEntry*	damage;
		sdPlayerStatEntry*	shotsHit;
		sdPlayerStatEntry*	shotsHitTorso;
		sdPlayerStatEntry*	shotsHitHead;
		sdPlayerStatEntry*	headshotKills;
		sdPlayerStatEntry*	xp;

		// players
		sdPlayerStatEntry*	kills;
		sdPlayerStatEntry*	deaths;
		sdPlayerStatEntry*	teamKills;

		sdPlayerStatEntry*	totalKills;
		sdPlayerStatEntry*	totalHeadshotKills;
		sdPlayerStatEntry*	totalDeaths;
		sdPlayerStatEntry*	totalTeamKills;
		sdPlayerStatEntry*	totalDamage;

		idStr				name;
	};

							sdDeclDamage( void );
	virtual					~sdDeclDamage( void );

	virtual const char*		DefaultDefinition( void ) const;
	virtual bool			Parse( const char *text, const int textLength );
	virtual void			FreeData( void );

	float					GetDamage( idEntity* entity, bool& noScale ) const;
	float					GetPush( void ) const { return push; }
	float					GetRadius( void ) const { return radius; }
	float					GetKickTime( void ) const { return kickTime; }
	float					GetKickAmplitude( void ) const { return kickAmplitude; }
	float					GetKnockback( void ) const { return knockback; }
	float					GetDamageKnockback( void ) const { return damageKnockback; }

	float					GetBlobTime( void ) const { return blobTime; }
	float					GetBlobX( void ) const { return blobRect[ 0 ]; }
	float					GetBlobY( void ) const { return blobRect[ 1 ]; }
	float					GetBlobWidth( void ) const { return blobRect[ 2 ]; }
	float					GetBlobHeight( void ) const { return blobRect[ 3 ]; }
	const char*				GetBlobMaterial( void ) const { return blobMaterial; }

	static void				CacheFromDict( const idDict& dict );

	const char*				GetSound( const char* key ) const;

	const idVec3&			GetKickDir( void ) const { return kickDir; }

	bool					GetNoGod( void ) const { return flags.noGod; }
	bool					GetNoArmor( void ) const { return flags.noArmor; }
	bool					GetGib( void ) const { return flags.gib; }
	bool					GetNoAir( void ) const { return flags.noAir; }
	bool					GetNoTeam( void ) const { return flags.noTeam; }
	bool					GetNoTrace( void ) const { return flags.noTrace; }
	bool					GetNoPain( void ) const { return flags.noPain; }
	bool					GetForcePassengerKill( void ) const { return flags.forcePassengerKill; }
	bool					GetCanHeadshot( void ) const { return flags.canHeadShot; }
	bool					GetMelee( void ) const { return flags.melee; }
	bool					GetAllowComplaint( void ) const { return !flags.noComplaint; }
	bool					GetRecordHitStats( void ) const { return flags.recordHitStats; }
	bool					GetNoDirection( void ) const { return flags.noDirection; }

	bool					IsTeamDamage( void ) const { return flags.isTeamDamage; }

	float					GetSelfDamageScale( void ) const { return selfDamageScale; }

	static const sdDeclDamage* DamageForName( const char* name, bool makeDefault );

	const sdDeclToolTip*	GetObituary( void ) const { return obituary; }
	const sdDeclToolTip*	GetSelfObituary( void ) const { return selfObituary; }
	const sdDeclToolTip*	GetTeamKillObituary( void ) const { return teamKillObituary; }
	const sdDeclToolTip*	GetUnknownObituary( void ) const { return unknownObituary; }
	const sdDeclToolTip*	GetUnknownFriendlyObituary( void ) const { return unknownFriendlyObituary; }

	const sdDeclProficiencyItem*	GetDamageBonus( void ) const { return damageBonus; }

	idCVar*					GetTeamKillCheckCvar( void ) const { return teamKillCVar; }

	const stats_t&			GetStats( void ) const { return stats; }

protected:
	const sdDeclDamageFilter*	damage;

	float					push;
	float					radius;
	float					knockback;
	float					damageKnockback;
	float					kickTime;
	float					kickAmplitude;
	float					selfDamageScale;

	idDict					sounds;

	float					blobTime;
	idVec4					blobRect;
	idStr					blobMaterial;

	const sdDeclToolTip*	obituary;
	const sdDeclToolTip*	selfObituary;
	const sdDeclToolTip*	teamKillObituary;
	const sdDeclToolTip*	unknownObituary;
	const sdDeclToolTip*	unknownFriendlyObituary;

	idCVar*					teamKillCVar;

	const sdDeclProficiencyItem*	damageBonus;

	stats_t stats;

	idVec3					kickDir;

	typedef struct damageFlags_s {
		bool				noGod				: 1;
		bool				noArmor				: 1;
		bool				gib					: 1;
		bool				noAir				: 1;
		bool				noTeam				: 1;
		bool				noTrace				: 1;
		bool				noPain				: 1;
		bool				forcePassengerKill	: 1;
		bool				canHeadShot			: 1;
		bool				melee				: 1;
		bool				noComplaint			: 1;
		bool				recordHitStats		: 1;
		bool				isTeamDamage		: 1;
		bool				noDirection			: 1;
	} damageFlags_t;

	damageFlags_t			flags;
};

#define DAMAGE_FOR_NAME( name ) sdDeclDamage::DamageForName( name, true )
#define DAMAGE_FOR_NAME_UNSAFE( name ) sdDeclDamage::DamageForName( name, false )

#endif // __DECLDAMAGE_H__
