#ifndef __GAME_VIEWLIGHT_H__
#define __GAME_VIEWLIGHT_H__

/**
 * Show player's view flash light

cvar harm_ui_showViewLight, default 0 = hide view flash light
cvar harm_ui_viewLightShader, default lights/flashlight5 = view flash light material texture/entityDef name
cvar harm_ui_viewLightRadius, default 1280 640 640 = view flash light radius
cvar harm_ui_viewLightOffset, default 0 0 0 = view flash light origin offset
cvar harm_ui_viewLightType, default 0 = player view flashlight type
cvar harm_ui_viewLightOnWeapon, default 0 = player view flashlight follow weapon position
*/

#include "State.h"

class idPlayer;
class idViewLight {
public:

    // CLASS_PROTOTYPE( idViewLight );

    idViewLight( void );
    virtual					~idViewLight( void );

    // Init
    void					Spawn						( const idDict &args );					// unarchives object from save game file

    // State control/player interface
	void					Present                     ( void );

    // Visual presentation
    void					PresentLight				( bool showViewLight );

    void				    Init						( idPlayer* _owner );
    idPlayer *              GetOwner                    ( void );
	void					EnterCinematic              ( void );
	void					ExitCinematic               ( void );
    void					Toggle				        ( void );
    bool					IsOn				        ( void ) const;
    void					SetOn				        ( bool open );
    void			        SetRadius(float x, float y, float z);
    void			        SetRadius(const idVec3 &radius);
    void	                SetColor(float red, float green, float blue);
    void	                SetColor(const idVec3 &color);
    void	                SetOffset(float forward, float right, float up);
    void	                SetOffset(const idVec3 &offset);
    bool			        SetShader(const char *shadername);
    bool			        SetEntity(const char *entityname);
    bool			        SetLight(const char *name);
    void			        SetLightParm(int parmnum, float value);
    void	                SetParm(const char *name, const char *value);
    void			        SetLightType(int type);
    void			        SetOnWeapon(bool wp);

protected:
	void					SetupRenderLight( void );
	void					On(void);
	void					Off(void);
	void					FreeLightDef(void);
	void					PresentLightDefChange(void);
    void	                UpdateLight(bool full);
    void					Respawn(void);
    void					Open				        ( bool open = true );
	void					ParseSpawnArgsToRenderLight(void);
	void					CorrectWeaponLightAxis(const idWeapon *wp);

private:
    idDict                  spawnArgs;
    qhandle_t				lightDefHandle;				// handle to renderer light def
    renderLight_t			renderLight;				// light presented to the renderer
    idPlayer *              owner;
    bool                    on;
    idVec3                  lightOffset;
	bool					onWeapon;

	int					    currentWeapon;
	bool					correctWeaponAxis;
	idMat3					correctWeaponAxisMat;

    friend class idPlayer;

    CLASS_STATES_PROTOTYPE ( idViewLight );
};

ID_INLINE idPlayer * idViewLight::GetOwner( void )
{
    return owner;
}

ID_INLINE bool idViewLight::IsOn( void ) const
{
    return on;
}

#endif
