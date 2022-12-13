#ifndef __HH_REACTION_H
#define __HH_REACTION_H


//
// hhReactionTarget
//
class hhReactionTarget
{
public:
	hhReactionTarget() {
		weight	= 1.0f;
		ent		= NULL;
	}

	hhReactionTarget(idEntity *e, float w = 1.0f) {
		ent		= e;
		weight	= w;
	}

	idEntityPtr<idEntity>	ent;			// The entity that is effected
	float					weight;			// The degree ent is effected
};

//
// hhReactionVolume
//
class hhReactionVolume : public idEntity {
public:
	CLASS_PROTOTYPE( hhReactionVolume );
	
	enum {
		flagPlayers		= (1<<0),
		flagMonsters	= (1<<1),
		flagActors		= (flagPlayers|flagMonsters),
	};

	void		Spawn(void); 
	
	// Gets all of the entities (filtered by validFlags) that are inside this volume
	void		GetValidEntitiesInside(idList<idEntity*> &outValidEnts);
	bool		IsValidEntity( idEntity *ent );

	void		Save(idSaveGame *savefile) const;
	void		Restore(idRestoreGame *savefile);

protected:
	int	flags;		// Flags for this volume
};

//
// hhReactionDesc
//
// Holds the unchanging properties of a reaction
//
class hhReactionDesc
{
public:
	friend class hhReactionHandler;
	hhReactionDesc() {
		name				= idStr("<undefined>");
		effect				= Effect_Invalid;
		cause				= Cause_Invalid;
		flags				= 0;
		effectRadius		= -1.0f;
		effectMinRadius		= -1.0f;
		effectVolumes.SetGranularity(1);
		key					= idStr("");
		keyVal				= idStr("");
		anim				= idStr("");
		listenerRadius		= -1.0f;
		listenerMinRadius	= -1.0f;
		listenerVolumes.SetGranularity(1);
	}

	enum Cause {
		Cause_Invalid = -1,
		Cause_Damage,					// AI can damage this to get reaction
		Cause_Touch,					// AI can touch this to get reaction
		Cause_Use,						// AI can 'use' this to get reaction	
		Cause_None,						// AI does nothing, and the effect is always felt
		Cause_PlayAnim,					// AI must play an anim to get the desired effect
		Cause_Telepathic_Trigger,		// AI must use their telepathy to trigger this entity
		Cause_Telepathic_Throw,			// AI must use their telepathy to physically throw this entity

		Cause_Total
	};

	enum Effect {
		Effect_Invalid = -1,
		Effect_Damage,					// Causes damage (reduces health)
		Effect_DamageEnemy,				// Causes damage (reduces health)
		Effect_VehicleDock,				// Causes damage (reduces health)
		Effect_Vehicle,					// Causes damage (reduces health)
		Effect_Heal,					// Heals entity (increases health)
		Effect_HaveFun,					// Something 'fun' for this entity to do
		Effect_ProvideCover,			// Indicates this entity can possibily provide 'cover' for a monster
		Effect_BroadcastTargetMsgs,		// This entity will have its target's msg's sent
		Effect_Climb,					// This entity will let the monster climb it.
		Effect_Passageway,				// This entity is a harvester passageway
		Effect_CallBackup,				// Calls for help

		Effect_Total
	};
	
	enum {
		flag_EffectAllPlayers	= (1<<0),		// This entity automatically targets ALL entities of type hhPlayer
		flag_EffectAllMonsters	= (1<<1),		// This entity automatically targets ALL entities of typehhAI				
		flag_Exclusive			= (1<<2),		// This msg is exclusive, and once a monster has committed to it - NO one else should listen to it
		flag_EffectListener		= (1<<3),		// The effect of this reaction is applied to the listener		
		flag_AnimFaceCauseDir	= (1<<4),		// When playing an anim for this reaction, face the direction of our cause
		flag_AnimTriggerCause	= (1<<5),		// When playing an anim for this reaction, trigger the cause entity
		flag_SnapToPoint		= (1<<6),		// This msg is only for monsters that are telepathic		
		flagReq_MeleeAttack		= (1<<8),		// This msg is for monsters that can melee attack
		flagReq_RangeAttack		= (1<<9),		// This msg is for monsters that can range attack
		flagReq_Anim			= (1<<10),		// This msg is for monsters that have a given anim (anim)
		flagReq_KeyValue		= (1<<11),		// This msg is for monsters that have a given key set (key, keyVal)
		flagReq_NoVehicle		= (1<<12),		// This msg is for monsters that are NOT in vehicles
		flagReq_CanSee			= (1<<13),		// This msg is only for monsters that can see the cause entity
		flagReq_Telepathic		= (1<<14),		// This msg is only for monsters that are telepathic
	};

	
	static	idStr		CauseToStr(Cause c);			// Converts activate enum into readable form
	static	idStr		EffectToStr(Effect e);			// Converts reaction enum to readable form
	static  Cause		StrToCause(const char* str);	// Converts string to caused by enum or Caused_Invalid if not found
	static  Effect		StrToEffect(const char* str);	// Converts string to effect enum or Effect_Invalid if not found
	void				Set(const idDict &keys);		// Sets this description to the given key values
	bool				CauseRequiresPathfinding(void)	const	{return cause == Cause_Touch || cause == Cause_Use || cause == Cause_PlayAnim;}
	void				Save( idSaveGame *savefile ) const;
	void				Restore( idRestoreGame *savefile );
	
	idStr 						name;				// unique name for each reaction desc
	Effect						effect;				// pdm: for reaction system
	Cause						cause;				// pdm: for reaction system	
	unsigned int				flags;				// jrm: reaction flags for reaction system	
	float						effectRadius;		// jrm: If > 0.0f then ONLY target entities that are at most this far away
	float						effectMinRadius;	// jrm: if > 0.0f then ONLY target entities that are at least this far away
	idList<hhReactionVolume*>	effectVolumes;		// jrm: if valid, then only entities that are inside one of these volumes are valid 'effect' entities
	idStr						key;				// jrm: the key required for a monster to be eligable for this reaction msg (ReactionFlag_KeyReq must be set)
	idStr						keyVal;				// jrm: the value of the reaction key required
	idStr						anim;				// jrm: the value of the reaction key required
	float						listenerRadius;		// jrm: listeners farther away than this distance will not receive this reaction
	float						listenerMinRadius;	// jrm: listeners closer than this dist will not receive this reaction
	idList<hhReactionVolume*>	listenerVolumes;	// jrm: only listeners that are inside one of these volumes will receive this reaction
	idDict						touchOffsets;		// Example: Use "monster_hunter" as key to get touchOffset values
	idStr						touchDir;			// Indicates how a monster should approach the cause entity

	idStr						finish_key;			// mdc: used to set a key on monster after finishing a reaction
	idStr						finish_val;			// mdc: used to set a key on monster after finishing a reaction
};

//
// hhReaction
//
// Describes a reaction ready to be processed by the AI. Holds a description of the reaction, as well as the cause/effect entities.
// Do the cause describe by <desc> to <causeEntity> to get the effect in <desc> applied to <effectEntity>
//
class hhReaction
{
public:
	hhReaction()
	{
		desc			= NULL;
		causeEntity		= NULL;		
		active			= TRUE;
		exclusiveOwner	= NULL;
		exclusiveUntil	= -1;
		effectEntity.Clear();
	};
	hhReaction(const hhReactionDesc *reactDesc, idEntity *cause, const hhReactionTarget &effect)
	{
		active			= TRUE;
		desc			= reactDesc;
		causeEntity		= cause;
		exclusiveUntil	= -1;
		exclusiveOwner	= NULL;
		effectEntity.Append(effect);		
	}

	bool			IsActive()		const	{return active;}
	bool			IsActiveForListener(idEntity *listener) const {
		if(!active) {
			return FALSE;
		}

		// If someone has claimed this, and their claim is still valid... Only active for that particular AI
		if(exclusiveOwner.GetEntity() && gameLocal.GetTime() < exclusiveUntil) {
			return exclusiveOwner.GetEntity() == listener;
		}

		// No one has claimed us, or time has expired, so anyone can claim us now
		return TRUE;
	};

	bool			ClaimExclusivity(idEntity *claimer, int ms=1000) {

		// Cannot claim, someone else already has it
		if(exclusiveOwner.GetEntity() && gameLocal.GetTime() < exclusiveUntil) {
			return FALSE;
		}

		// They claimed it now....
		exclusiveOwner = claimer;
		exclusiveUntil = gameLocal.GetTime() + ms;
		return TRUE;
	}

	void			Save(idSaveGame *savefile) const;	
	void			Restore(idRestoreGame *savefile);

	bool							active;				// TRUE to be considered for sending
	idEntityPtr<idEntity>			exclusiveOwner;		// The AI that has 'claimed' this reaction
	int								exclusiveUntil;		// Until gameLocal.time has reached this value, this reaction does not send msgs (except for exclusive reactions)
	const hhReactionDesc*			desc;				// Info about the kind of reaction this is
	idEntityPtr<idEntity>			causeEntity;		// The entity to apply the cause to	
	idList<hhReactionTarget>		effectEntity;		// The effect of doing the cause is felt by these entities
};


//
// hhReactionHandler
//
// Manages all of the reaction descs currently loaded
//
class hhReactionHandler
{
public:
	hhReactionHandler();
	virtual ~hhReactionHandler();

	void Save( idSaveGame *savefile ) const;
	void Restore( idRestoreGame *savefile );

	// Loads the given reaction def name. If uniqueDescKeys are specified, a unique hhReactionDesc* is guarenteed to be returned.
	// Otherwise, if no override keys are specified and a reaction of the given name is found, it is returned without creating a new one
	// The returned desc is ONLY a desc, it is not meant to be modified in any way
	const hhReactionDesc*		LoadReactionDesc(const char* defName, const idDict &uniqueDescKeys) {
		if(uniqueDescKeys.GetNumKeyVals() <= 0) {
			return LoadReactionDesc(defName, NULL);
		} else {
			return LoadReactionDesc(defName, &uniqueDescKeys);
		}
	}
	const hhReactionDesc*		LoadReactionDesc(const char* defName)								{return LoadReactionDesc(defName, NULL);}
	
protected:
	friend class hhReaction;

	// Creates a new reaction desc with the given name and keys to describe the reaction
	hhReactionDesc*				CreateReactionDesc(const char *name, const idDict &keys);
	idStr						GetUniqueReactionDescName(const char *name);
	const hhReactionDesc*		FindReactionDesc(const char *name);
	void						StripReactionPrefix(idDict &dict);	// Removes any "reaction_" or "reaction1_" prefix from the given dict
	const hhReactionDesc*		LoadReactionDesc(const char* defName, const idDict *uniqueDescKeys);
	
	
	idList<hhReactionDesc*>	reactionDescs;
};

#endif