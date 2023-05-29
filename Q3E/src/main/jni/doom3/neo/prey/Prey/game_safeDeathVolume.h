#ifndef __HH_SAFE_DEATH_VOLUME_H
#define __HH_SAFE_DEATH_VOLUME_H

//-----------------------------------------------------------------------
//
// hhSafeResurrectionVolume
//
//-----------------------------------------------------------------------
class hhSafeResurrectionVolume : public idEntity {
	CLASS_PROTOTYPE( hhSafeResurrectionVolume );
	
	public:
		void			Spawn();

		void			PickRandomPoint( idVec3& origin, idMat3& axis );

	protected:
		virtual int		DetermineContents() const;

		void			Event_Enable();
		void			Event_Disable();
};

//-----------------------------------------------------------------------
//
// hhSafeResurrectionVolume
//
//-----------------------------------------------------------------------
class hhSafeDeathVolume : public hhSafeResurrectionVolume {
	CLASS_PROTOTYPE( hhSafeDeathVolume );

	public:
		void			Spawn();

	protected:
		virtual bool	IsValid( const hhPlayer* player );
		virtual int		DetermineContents() const;
		
	protected:
		void			Event_Touch( idEntity *other, trace_t *trace );
};

#endif