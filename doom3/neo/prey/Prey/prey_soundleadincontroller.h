#ifndef __HH_SOUND_LEADIN_CONTROLLER_H
#define __HH_SOUND_LEADIN_CONTROLLER_H

class hhSoundLeadInController : public idClass {
	CLASS_PROTOTYPE( hhSoundLeadInController );

	public:
					hhSoundLeadInController();

		void		SetOwner( idEntity* ent );

		//rww - network code
		virtual void	WriteToSnapshot( idBitMsgDelta &msg ) const;
		virtual void	ReadFromSnapshot( const idBitMsgDelta &msg );

		void		SetLeadIn( const char* soundname );
		void		SetLoop( const char* soundname );
		void		SetLeadOut( const char* soundname );

		int			StartSound( const s_channelType leadChannel, const s_channelType loopChannel, int soundShaderFlags = 0, bool broadcast = false );
		void		StopSound( const s_channelType leadChannel, const s_channelType loopChannel, bool broadcast = false );

		void		Save( idSaveGame *savefile ) const;
		void		Restore( idRestoreGame *savefile );

		void		SetLoopOnlyOnLocal(int loopOnlyOnLocal);

	protected:
		float		CalculateScale( const float value, const float min, const float max );
		void		StartFade( const idSoundShader*	shader, const s_channelType channel, int start, int end, int finaldBVolume, int duration );

	protected:
		void		Event_StartSoundShaderEx( idSoundShader* shader, const s_channelType channel, int soundShaderFlags, bool broadcast );

	protected:
		const idSoundShader*	leadInShader;
		const idSoundShader*	loopShader;
		const idSoundShader*	leadOutShader;

		int			startTime;
		int			endTime;

		idEntityPtr<idEntity>	owner;

		//rww - everything below here is for networking, no need to save/restore
		s_channelType			lastLeadChannel;
		s_channelType			lastLoopChannel;
	public:
		bool					bPlaying;
		int						iLoopOnlyOnLocal;
};

#endif