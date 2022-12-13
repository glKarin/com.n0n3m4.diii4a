#ifndef __HH_BONE_CONTROLLER_H
#define __HH_BONE_CONTROLLER_H

/***********************************************************************

	hhBoneController

***********************************************************************/

class hhBoneController {
	public:
							hhBoneController();

		void				SetTurnRate( idAngles& Rate ) { m_TurnRate = Rate; }
		const idAngles&		GetTurnRate() const { return m_TurnRate; }

		void				SetRotationFactor( idAngles& RotationFactor );
		const idAngles&		GetRotationFactor() const { return m_Factor; }

		void				Setup( idEntity *pOwner, const char *pJointname, idAngles &MinAngles, idAngles &MaxAngles, idAngles& Rate, idAngles& Factor );
		void				Setup( idEntity *pOwner, jointHandle_t Joint, idAngles &MinAngles, idAngles &MaxAngles, idAngles& Rate, idAngles& Factor );
		void				Update( int iCurrentTime );
		bool				TurnTo( idAngles &Target );
		bool				AimAt( idVec3 &Target );
		bool				IsFinishedMoving( int iAxis );
		bool				IsFinishedMoving();

		void				AdjustScanRateToLinearizeBonePath( float fLinearScanRate );

		bool				Add( idAngles &Ang ) { return TurnTo( Ang + m_IdealAng ); };
		void				Clear( void ) { m_IdealAng.Zero(); };
		const idAngles		&CurrentAngles() const { return m_CurrentAng; };

		void				Save( idSaveGame *savefile ) const;
		void				Restore( idRestoreGame *savefile );

	protected:
		bool				ClampAngles( void );

	public:
		idAngles			m_Fraction;

	protected:
		idAngles			m_MaxDelta;
		
		jointHandle_t		m_JointHandle;
		idAngles			m_IdealAng;
		idAngles			m_CurrentAng;
		idAngles			m_MinAngles;
		idAngles			m_MaxAngles;

		idAngles			m_Factor;
		idAngles			m_TurnRate;
		idEntity			*m_pOwner;
};

#endif