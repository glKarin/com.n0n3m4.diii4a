/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#ifndef __GAME_CAMERA_H__
#define __GAME_CAMERA_H__

#include "idlib/math/Quat.h"

#include "Entity.h"

/*
===============================================================================

Camera providing an alternative view of the level.

===============================================================================
*/

class idCamera : public idEntity {
public:
	ABSTRACT_PROTOTYPE( idCamera );

	void					Spawn( void );
	virtual void			GetViewParms( renderView_t *view ) = 0;
	virtual renderView_t *	GetRenderView();
	virtual void			Stop( void ){} ;
};

/*
===============================================================================

idCameraView

===============================================================================
*/

class idCameraView : public idCamera {
public:
	CLASS_PROTOTYPE( idCameraView );
							idCameraView();

	// save games
	void					Save( idSaveGame *savefile ) const; // blendo eric: savegame pass 1
	void					Restore( idRestoreGame *savefile );				// unarchives object from save game file

	void					Spawn( );
	virtual void			GetViewParms( renderView_t *view );
	virtual void			Stop( void );

	//bc
	void					SetActive(bool value);
	void					SetPath(idEntity *pathEnt, float movetime, int angleRotate, float accelerationTime, float decelerationTime);

	void					SetFOVLerp(float start, float end, int timeMS, int lerpType);
	float					GetCurrentFOV() { return fov; };

protected:
	
	void					Event_Activate(idEntity *activator);
	void					Event_SetAttachments();
	void					Event_SetOverrideRoll(float roll);
	void					Event_SetRollLerp(float startAngle, float endAngle, int timeMS, int lerpType);
	void					Event_GetCameraAngles(void);
	idMat3					CalcViewAxis(void);
	void					SetAttachment( idEntity **e, const char *p );
	void					Think(void);	
	void					StartDollyZoom(idEntity* target, float targetWidth);
	float					fov;
	float					fov_v;
	idEntity				*attachedTo;
	idEntity				*attachedView;

	float					fovLerpStart;
	float					fovLerpEnd;
	int						fovLerpStartTime;
	int						fovLerpEndTime;
	int						fovLerpType;
	enum					{	LERPTYPE_LINEAR,			//0
								LERPTYPE_CUBIC_EASE_IN,		//1
								LERPTYPE_CUBIC_EASE_OUT,	//2
								LERPTYPE_CUBIC_EASE_INOUT	//3
							}; //lerpType interpolation types

	bool					dollyZoomActive;
	idEntity*				dollyZoomTarget;
	float					dollyZoomWidth;

	float					overrideRoll; // Set a custom roll value for cameras that are using attachedView (they will pitch and yaw to face their target)
	float					rollLerpStart;
	float					rollLerpEnd;
	int						rollLerpStartTime;
	int						rollLerpEndTime;
	int						rollLerpType;
};



/*
===============================================================================

A camera which follows a path defined by an animation.

===============================================================================
*/

typedef struct {
	idCQuat				q;
	idVec3				t;
	float				fov;
} cameraFrame_t;

class idCameraAnim : public idCamera {
public:
	CLASS_PROTOTYPE( idCameraAnim );

							idCameraAnim();
							~idCameraAnim();

	// save games
	void					Save( idSaveGame *savefile ) const; // blendo eric: savegame pass 1
	void					Restore( idRestoreGame *savefile );				// unarchives object from save game file

	void					Spawn( void );
	virtual void			GetViewParms( renderView_t *view );

private:
	int						threadNum;
	idVec3					offset;
	int						frameRate;
	int						starttime;
	int						cycle;
	idList<int>				cameraCuts;
	idList<cameraFrame_t>	camera;
	idEntityPtr<idEntity>	activator;

	void					Start( void );
	void					Stop( void );
	void					Think( void );

	void					LoadAnim( void );
	void					Event_Start( void );
	void					Event_Stop( void );
	void					Event_SetCallback( void );
	void					Event_Activate( idEntity *activator );
};

#endif /* !__GAME_CAMERA_H__ */
