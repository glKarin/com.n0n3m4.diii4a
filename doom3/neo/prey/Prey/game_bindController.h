
#ifndef __HH_GAME_BINDCONTROLLER_H__
#define __HH_GAME_BINDCONTROLLER_H__


/*
===================================================================================

	hhBindController

	Entity used to control an entity's position in space.
	Used for rail rides, tractor beams, crane, etc.

===================================================================================
*/

extern const idEventDef EV_BindAttach;
extern const idEventDef EV_BindDetach;

class hhBindController : public idEntity {

public:
	CLASS_PROTOTYPE( hhBindController );

	void					Spawn();
	virtual					~hhBindController();
	virtual void			Think();
	virtual void			Attach(idEntity *ent, bool loose=false, int bodyID=0, idVec3 &point=vec3_origin);
	virtual void			Detach();
	void					SetTension(float tension);
	void					SetSlack(float slack);
	void					SetRiderParameters(const char *animname, const char *handname, float yaw, float pitch);
	idVec3					GetRiderBindPoint() const;
	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	bool					OrientPlayer() const { return bOrientPlayer; }
	idEntity *				GetRider() const;
	ID_INLINE bool			IsLoose() const	{ return bLooseBind; }
	ID_INLINE int			GetHangID() const { return hangID; }
	ID_INLINE void			SetShuttle(bool shuttle) { force.SetShuttle(shuttle); }

protected:
	bool					ValidRider(idEntity *ent) const;
	void					CreateHand(hhPlayer *player);
	void					ReleaseHand();

	void					Event_Attach(idEntity *rider, bool loose);
	void					Event_AttachBody(idEntity *rider, bool loose, int bodyID);
	void					Event_Detach();

protected:
	idEntityPtr<idEntity>	boundRider;			// Pointer to rider for true bind-based riders
												// Loose bindings store rider in the force
	hhHand *				hand;
	bool					bLooseBind;			// True if the bind is through a force
	bool					bOrientPlayer;		// Orient the player to my orientation
	hhForce_Converge		force;				// Force used for "loose" binding

	// Apply these to any entities bound
	idStr					animationName;		// Animation to play on riders
	idStr					handName;			// Hand to apply to player riders
	float					yawLimit;			// Yaw restriction for player riders
	float					pitchLimit;			//rww - used for slabs and other things to restrict pitch of rider
	int						hangID;				// Body id to attach to for hanging
};

#endif 