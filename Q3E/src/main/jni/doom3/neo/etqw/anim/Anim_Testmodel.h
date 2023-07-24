// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __ANIM_TESTMODEL_H__
#define __ANIM_TESTMODEL_H__

#include "../AnimatedEntity.h"
#include "../physics/Physics_Parametric.h"
#include "../Actor.h"

/*
==============================================================================================

	idTestModel

==============================================================================================
*/

typedef struct {
	jointModTransform_t		mod;
	jointHandle_t			from;
	jointHandle_t			to;
} copyJoints_t;

class idTestModel : public idAnimatedEntity {
public:
	CLASS_PROTOTYPE( idTestModel );

							idTestModel();
							~idTestModel();


	void					Spawn( void );

	virtual bool			ShouldConstructScriptObjectAtSpawn( void ) const;

	void					NextAnim( const idCmdArgs &args );
	void					PrevAnim( const idCmdArgs &args );
	void					NextFrame( const idCmdArgs &args );
	void					PrevFrame( const idCmdArgs &args );
	void					TestAnim( const idCmdArgs &args );
	void					BlendAnim( const idCmdArgs &args );
	void					HideSurfaceID( const idCmdArgs &args );
	void					ShowSurfaceID( const idCmdArgs &args );
	void					ResetSurfaceID( const idCmdArgs &args );

	static void 			KeepTestModel_f( const idCmdArgs &args );
	static void 			TestModel_f( const idCmdArgs &args );
	static void				ArgCompletion_TestModel( const idCmdArgs &args, void(*callback)( const char *s ) );
	static void 			TestSkin_f( const idCmdArgs &args );
	static void 			TestShaderParm_f( const idCmdArgs &args );
	static void 			TestParticleStopTime_f( const idCmdArgs &args );
	static void 			TestAnim_f( const idCmdArgs &args );
	static void				ArgCompletion_TestAnim( const idCmdArgs &args, void(*callback)( const char *s ) );
	static void 			TestBlend_f( const idCmdArgs &args );
	static void				TestModelHideSurfaceID_f( const idCmdArgs &args );
	static void				TestModelShowSurfaceID_f( const idCmdArgs &args );
	static void				TestModelResetSurfaceID_f( const idCmdArgs &args );
	static void 			TestModelNextAnim_f( const idCmdArgs &args );
	static void 			TestModelPrevAnim_f( const idCmdArgs &args );
	static void 			TestModelNextFrame_f( const idCmdArgs &args );
	static void 			TestModelPrevFrame_f( const idCmdArgs &args );

private:
	idEntityPtr<idEntity>	head;
	idAnimator				*headAnimator;
	idAnim					customAnim;
	idPhysics_Parametric	physicsObj;
	idStr					animname;
	int						anim;
	int						headAnim;
	int						mode;
	int						frame;
	int						starttime;
	int						animtime;

	idList<copyJoints_t>	copyJoints;

	virtual void			Think( void );

	void					Event_Footstep( void );
};

#endif /* !__ANIM_TESTMODEL_H__*/
