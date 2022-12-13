#ifndef __HH_PROJECTILE_RIFLE_SNIPER_H
#define __HH_PROJECTILE_RIFLE_SNIPER_H

/***********************************************************************

  hhProjectileRifleSniper
	
***********************************************************************/
extern const idEventDef EV_ApplyExitWound;
extern const idEventDef EV_AttemptFinalExitEventDef;

class hhProjectileRifleSniper : public hhProjectile {
	CLASS_PROTOTYPE( hhProjectileRifleSniper );

	public:
		void					Spawn();
		virtual void			Think();

		void					Save( idSaveGame *savefile ) const;
		void					Restore( idRestoreGame *savefile );

	protected:
		virtual float			DetermineDamageScale( const trace_t* collision ) const;
		virtual bool			DamageIsValid( const trace_t* collision, float& damageScale );

		virtual const idEventDef* GetInvalidDamageEventDef() const { return &EV_Fizzle; }
		void					DamageEntityHit( const trace_t* collision, const idVec3& velocity, idEntity* entHit );

	protected:
		void					Event_AttemptFinalExitEventDef();

		void					Event_AllowCollision_PassThru( const trace_t* collision );

	protected:
		//Used floats so we can divide without casting
		float					numPassThroughs;
		float					maxPassThroughs;
		idEntityPtr<idEntity>	lastDamagedEntity;
};

#endif