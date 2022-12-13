#ifndef __PREY_GAME_H
#define __PREY_GAME_H

class hhReactionHandler;
class hhSunCorona; // CJR
class hhHand;
class hhAIInspector;

#ifdef GAME_DLL
extern idCVar com_forceGenericSIMD;
#endif

//HUMANHEAD: aob - needed for networking to send the least amount of bits
extern const int DECL_MAX_TYPES_NUM_BITS; 
//HUMANHEAD END

class hhGameLocal : public idGameLocal {
public:
	virtual void			Init( void );
	virtual void			Shutdown( void );
	void					UnregisterEntity( idEntity *ent );

	virtual void			MapShutdown( void );	
	virtual void			InitFromNewMap( const char *mapName, idRenderWorld *renderWorld, idSoundWorld *soundWorld, bool isServer, bool isClient, int randseed );
	virtual void			RadiusDamage( const idVec3 &origin, idEntity *inflictor, idEntity *attacker, idEntity *ignoreDamage, idEntity *ignorePush, const char *damageDefName, float dmgPower = 1.0f );// jrm
	virtual void			RadiusPush( const idVec3 &origin, const float radius, const float push, const idEntity *inflictor, const idEntity *ignore, float inflictorScale, const bool quake );
	//HUMANHEAD rww
	virtual void			LogitechLCDUpdate(void);
	//HUMANHEAD END
	virtual gameReturn_t	RunFrame( const usercmd_t *clientCmds );
	virtual void			CacheDictionaryMedia( const idDict *dict );
	virtual bool			Draw( int clientNum );

	// added functionality functions
	void					RegisterTalonTarget( idEntity *ent );
	idEntity*				SpawnClientObject( const char* objectName, idDict* additionalArgs ); //rww
	idEntity*				SpawnObject( const char* objectName, idDict* additionalArgs = NULL );	

	void					GetTip(const char *binding, idStr &keyMaterialString, idStr &keyString, bool &isWide);
	bool					SetTip(idUserInterface* gui, const char *binding, const char *tip, const char *topMaterial = NULL, const char *overrideMaterial = NULL, const char *prefix = NULL);
	void					SetSunCorona( hhSunCorona *ent ) { sunCorona = ent; } // CJR
	hhSunCorona *			GetSunCorona( void ) { return sunCorona; } // CJR
	idEntity *				FindEntityOfType(const idTypeInfo &type, idEntity *last);

	// nla
	idDict &				GetSpawnArgs()	{ return( spawnArgs ); };	
	hhReactionHandler*		GetReactionHandler()	{ return reactionHandler; }
	float					GetTimeScale() const;
	const idVec3&			GetGravityNormal() { gravityNormal = gravity; gravityNormal.Normalize(); return gravityNormal; }	
	int						SimpleMonstersWithinRadius( const idVec3 org, float radius, idAI **monstList, int maxCount = MAX_GENTITIES ) const;

	void					SendMessageAI( const idEntity* entity, const idVec3& origin, float radius, const idEventDef& message );

	const char*				MatterTypeToMatterName( surfTypes_t type ) const;
	const char*				MatterTypeToMatterKey( const char* prefix, surfTypes_t type ) const;
	surfTypes_t				MatterNameToMatterType( const char* name ) const;
	surfTypes_t				GetMatterType( const trace_t& trace, const char* descriptor = "impact" ) const;
	surfTypes_t				GetMatterType( const idEntity *ent, const idMaterial *material, const char* descriptor = "impact" ) const;

	class hhDDAManager*		GetDDA() { return ddaManager; }

	bool					IsLOTA() { return bIsLOTA; }
	void					AlertAI( idEntity *ent );
	void					AlertAI( idEntity *ent, float radius );

	// Special case, these must be virtual -mdl
	virtual void			Save( idSaveGame *savefile ) const;
	virtual void			Restore( idRestoreGame *savefile );

	float					GetDDAValue( void );

	void					ClearStaticData( void );

	float					TimeBasedRandomFloat(void); //HUMANHEAD rww

#if _HH_INLINED_PROC_CLIPMODELS
	virtual void			CreateInlinedProcClip(idEntity *clipOwner); //HUMANHEAD rww
#endif

	virtual bool			PlayerIsDeathwalking( void );

	idList<idEntity*>		talonTargets;
	float					lastAIAlertRadius;

	idList< idEntityPtr< hhHand > > hands;

	idEntityPtr<hhAIInspector>		inspector;

protected:
#if DEATHWALK_AUTOLOAD
	virtual void			SpawnAppendedMapEntities();
#endif
	virtual bool			InhibitEntitySpawn( idDict &spawnArgs ); // HUMANHEAD mdl

protected:
	virtual	void			SpawnMapEntities( void ); // JRM
	idClipModel *			dwWorldClipModel;
#if _HH_INLINED_PROC_CLIPMODELS
	idList<idClipModel *>	inlinedProcClipModels; //HUMANHEAD rww - chopped geometry grabbed from proc and turned into clipmodel data
#endif

	idVec3					gravityNormal;

	hhSunCorona				*sunCorona; // CJR

	//HUMANHEAD rww - moved this down, it is using mixed memory inside and outside the managed heap as is, very messy.
	//class hhDDAManager*	ddaManager;
	//HUMANHEAD END

	bool					bIsLOTA; // CJR:  If true, this map is a LOTA map

	idList<idClipModel *>	staticClipModels; // HUMANHEAD mdl:  For inlined static clip models
	idList<renderEntity_t *> staticRenderEntities; // HUMANHEAD mdl:  For inlined static models
};

#endif
