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

#ifndef __GAME_SECURITYCAMERA_H__
#define __GAME_SECURITYCAMERA_H__

#include "Entity.h"

const int CAMERA_SPOKECOUNT = 16;

/*
===================================================================================

	Security camera

===================================================================================
*/


class idSecurityCamera : public idEntity {
public:
	CLASS_PROTOTYPE( idSecurityCamera );

							idSecurityCamera(void);
	virtual					~idSecurityCamera(void);

	void					Spawn( void );

	void					Save( idSaveGame *savefile ) const; // blendo eric: savegame pass 1
	void					Restore( idRestoreGame *savefile );

	virtual void			Think( void );

	virtual renderView_t *	GetRenderView();
	virtual void			Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );
	virtual bool			Pain( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );
	virtual void			Present( void );
	virtual void			Damage(idEntity *inflictor, idEntity *attacker, const idVec3 &dir, const char *damageDefName, const float damageScale, const int location, const int materialType = SURFTYPE_NONE);

	//BC
	void					HotReload();
	bool					IsAlerted();
	bool					IsSpliced;
	int						cameraIndex;

	void					SetTeam(int value);

	void					SetElectricalActive(bool value);

	virtual void			DoHack(); //for the hackgrenade.

	//blendo eric: prevent hackgrenade (and any other attachments) from disabling physics
	//virtual bool			IsAtRest( void ) const{ return false; }

private:

	enum					{ PAUSED, SWEEPING, SUSPICIOUS, ALERTED, DEAD, COOLDOWN, DAMAGEWARNING, ELECTRICAL_DISABLED };
	int						state;
	int						sweepAngle1;
	int						sweepAngle2;
	int						lerpStartAngle;
	int						lerpEndAngle;
	int						sweepTimerEnd;
	int						sweepTimerInterruptionTime;
	int						sweeptimeMax;
	
	float					sweepAngle;
	int						modelAxis;
	bool					flipAxis;
	float					scanDist;
	float					scanFov;
	float					frustumScale;	// if using spotlight, scale frustum by this to line up w/ spokes
	
	bool					negativeSweep;	//to know when to rotate clockwise/counterclockwise
	float					scanFovCos;

	idVec3					viewOffset;

	int						pvsArea;
	idPhysics_RigidBody		physicsObj;
	idTraceModel			trm;

	bool					CanSeePlayer( void );
	void					SetAlertModeVis( int status );
	void					DrawFov( void );
	const idVec3			GetAxis( void ) const;
	float					GetSweepTime( void ) const;

	void					Event_AddLight( void );

	//BC
	int						pauseTimer;
	void					BecomeSuspicious();
	void					BecomeAlerted(idEntity *targetEnt);
	int						cameraCheckTimer;
	int						stateTimer;
	int						suspicionPips;
	void					ResumeSweep();
	void					DrawBounds(void);
	bool					GetLOSCheck(idEntity *ent, idVec3 offset, float fovAmount);
	int						GetSightTimeMS() const;
	int						GetSightDamageWarnTimeMS() const;

	idBeam*					boundingBeam[CAMERA_SPOKECOUNT];
	idBeam*					boundingBeamTarget[CAMERA_SPOKECOUNT];

	void					AdjustStartEndLerpAngles();

	idLight					*spotLight;

	float					videoFOV;

	void					CanSeeEnemy(void);
	int						enemyCheckTimer;

	float					enemycheckFovCos;

	float					spotlightScale;
	

	renderLight_t			headlight;
	qhandle_t				headlightHandle;

	int						damageSparkTimer;

	static int				cameraCount;
	int						cameraID;

	//private end
};

#endif /* !__GAME_SECURITYCAMERA_H__ */
