#ifndef __HH_HEALTH_SPORE_H
#define __HH_HEALTH_SPORE_H

class hhHealthSpore : public idEntity {
	CLASS_PROTOTYPE( hhHealthSpore );

public:
	void			Spawn();
	void			Save( idSaveGame *savefile ) const;
	void			Restore( idRestoreGame *savefile );

	//rww - netcode
	void			WriteToSnapshot( idBitMsgDelta &msg ) const;
	void			ReadFromSnapshot( const idBitMsgDelta &msg );

protected:
	void			ApplyEffect( idActor* pActor );

	void			Event_Touch( idEntity* pOther, trace_t* pTraceInfo );
	virtual void	Event_RespawnSpore();
};

#endif