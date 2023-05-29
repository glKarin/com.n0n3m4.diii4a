#ifndef __HH_SOUND_H
#define __HH_SOUND_H

extern const idEventDef EV_Sound;
extern const idEventDef EV_ResetTargetHandles;

class hhSound : public idSound {
	CLASS_PROTOTYPE( hhSound );

	public:
						hhSound();
		void			Spawn();
		virtual bool	StartSoundShader( const idSoundShader *shader, const s_channelType channel, int soundShaderFlags, bool broadcast, int* length );
		virtual void	StopSound( const s_channelType channel, bool broadcast );
		void			StartDelayedSoundShader( const idSoundShader *shader, const s_channelType channel, int soundShaderFlags, bool broadcast );
		void			StopDelayedSound( const s_channelType channel, bool broadcast = false );
		float			GetCurrentAmplitude(const s_channelType channel);

		void			Save( idSaveGame *savefile ) const;
		void			Restore( idRestoreGame *savefile );

	protected:
		virtual bool	GetPhysicsToSoundTransform( idVec3 &origin, idMat3 &axis );

		float			RandomRange( const float min, const float max );

		bool			DetermineVolume( float& volume );
		idVec3			DeterminePositionOffset();

	protected:
		void			Event_SetTargetHandles();
		void			Event_StartDelayedSoundShader( const char *shader, const s_channelType channel, int soundShaderFlags = 0, bool broadcast = false );
		void			Event_SubtitleOff( void );
	protected:
		idVec3			positionOffset;
};

#endif