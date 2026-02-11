
#ifndef __PREY_PHYSICS_PARAMETRIC_H__
#define __PREY_PHYSICS_PARAMETRIC_H__

class hhWeapon;
class hhPlayer;

class hhPhysics_StaticWeapon : public idPhysics_Static {
	CLASS_PROTOTYPE( hhPhysics_StaticWeapon );

	public:
						hhPhysics_StaticWeapon();

		virtual void	SetSelfOwner( idActor* a );
		virtual bool	Evaluate( int timeStepMSec, int endTimeMSec );

		void			SetLocalAxis( const idMat3& newLocalAxis );
		void			SetLocalOrigin( const idVec3& newLocalOrigin );	

		void			Save( idSaveGame *savefile ) const;
		void			Restore( idRestoreGame *savefile );

	protected:
		hhWeapon*		castSelf;
		hhPlayer*		selfOwner;
};

class hhPhysics_StaticForceField : public idPhysics_Static {
	CLASS_PROTOTYPE( hhPhysics_StaticForceField );

	public:
								hhPhysics_StaticForceField();

		virtual bool					Evaluate( int timeStepMSec, int endTimeMSec );

		virtual bool					EvaluateContacts( void );
		virtual int						GetNumContacts( void ) const;
		virtual const contactInfo_t &	GetContact( int num ) const;
		virtual void					ClearContacts( void );
		virtual void					AddContactEntitiesForContacts( void );
		virtual void					AddContactEntity( idEntity *e );
		virtual void					RemoveContactEntity( idEntity *e );

		virtual void					ActivateContactEntities( void );

		void			Save( idSaveGame *savefile ) const;
		void			Restore( idRestoreGame *savefile );

	protected:
		idList<contactInfo_t>			contacts;				// contacts
		idList<contactEntity_t>			contactEntities;
};

#endif /* __PREY_PHYSICS_PARAMETRIC_H__ */
