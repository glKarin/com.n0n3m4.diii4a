#ifndef __HH_WEAPON_HIDER_H
#define __HH_WEAPON_HIDER_H


//=============================================================================
//
// hhHiderWeaponAltFireController
//
//=============================================================================
class hhHiderWeaponAltFireController : public hhWeaponFireController {
	CLASS_PROTOTYPE(hhHiderWeaponAltFireController);
public:
	ID_INLINE int				AmmoRequired() const;
	virtual	const idDict*		GetProjectileDict() const;
};

ID_INLINE int hhHiderWeaponAltFireController::AmmoRequired() const {
	return self->AmmoInClip();
}


class hhWeaponHider : public hhWeapon {
	CLASS_PROTOTYPE( hhWeaponHider );

	public:
									hhWeaponHider();
		float						nextPredictionAttack; //rww - does not need to be saved/restored, used only in client code.
		float						lastPredictionAttack; //rww - does not need to be saved/restored, used only in client code.
		int							nextPredictionTimeSkip; //rww - does not need to be saved/restored, used only in client code.

	protected:
		ID_INLINE virtual		hhWeaponFireController* CreateAltFireController();
		virtual void			ClientPredictionThink( void ); //rww
		virtual void			WriteToSnapshot( idBitMsgDelta &msg ) const; //rww
		virtual void			ReadFromSnapshot( const idBitMsgDelta &msg ); //rww
};

ID_INLINE hhWeaponFireController*	hhWeaponHider::CreateAltFireController() {
	return new hhHiderWeaponAltFireController;
}

#endif