#ifndef __BSE_FX_H__
#define __BSE_FX_H__

/*
===============================================================================

  Special effects.

===============================================================================
*/

typedef struct {
	renderLight_t			renderLight;			// light presented to the renderer
	qhandle_t				lightDefHandle;			// handle to renderer light def
	renderEntity_t			renderEntity;			// used to present a model to the renderer
	int						modelDefHandle;			// handle to static renderer model
	float					delay;
	int						particleSystem;
	int						start;
	bool					soundStarted;
	bool					shakeStarted;
	bool					decalDropped;
	bool					launched;
} idFXLocalAction;

class rvBSE
{
	public:
		rvBSE();
		virtual					~rvBSE();

		void					Spawn(void);

		virtual void			Think();
		void					Setup(const char *fx);
		void					Run(int time);
		void					Start(int time);
		void					Stop(void);
		const int				Duration(void);
		const char 			*EffectName(void);
		const char 			*Joint(void);
		const bool				Done();
		void Init(const rvDeclEffect* declEffect, renderEffect_s* parms, idRenderWorld *world, int time);
		void Update(renderEffect_s* parms, int time);
		void Event_Remove(void);
		ID_INLINE void Destroy(void) {
			Event_Remove();
		}

	protected:
		void ProjectDecal(const idVec3 &origin, const idVec3 &dir, float depth, bool parallel, float size, const char *material, float angle = 0.0f);
		void ProjectDecal(const idVec3 &origin, const idMat3 &axis, float depth, bool parallel, float size, const char *material, float angle = 0.0f);
		bool					StartSoundShader(const idSoundShader *shader, const s_channelType channel, int soundShaderFlags, bool broadcast, int *length);
		void Setup(const rvDeclEffect *fx);
		void Sync(renderEffect_s* parms);

	protected:
		void					CleanUp(void);
		void					CleanUpSingleAction(const rvFXSingleAction &fxaction, idFXLocalAction &laction);
		void					ApplyFade(const rvFXSingleAction &fxaction, idFXLocalAction &laction, const int time, const int actualStart);
		void SetReferenceSound(int handle);
		void UpdateSound(void);

		int						started;
		int						nextTriggerTime;
		const rvDeclEffect 		*fxEffect;				// GetFX() should be called before using fxEffect as a pointer
		idList<idFXLocalAction>	actions;
		idStr					systemName;
		int time;

	private:
		renderEffect_t parms;
		idRenderWorld *gameRenderWorld;
};

#endif /* !__BSE_FX_H__ */

