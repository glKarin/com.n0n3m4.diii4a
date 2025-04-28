#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"

#include "ViewLight.h"
#include "Player.h"

#define VIEW_LIGHT_DEFAULT_RADIUS_STR "1280 640 640"
#define VIEW_LIGHT_DEFAULT_OFFSET_STR "0 0 0"
#define VIEW_LIGHT_DEFAULT_COLOR_STR "1 1 1"

#define VIEW_LIGHT_DEFAULT_RADIUS_VALUE 1280, 640, 640
#define VIEW_LIGHT_DEFAULT_OFFSET_VALUE 0, 0, 0
#define VIEW_LIGHT_DEFAULT_COLOR_VALUE 1, 1, 1

#define VIEW_LIGHT_DEFAULT_RADIUS idVec3(VIEW_LIGHT_DEFAULT_RADIUS_VALUE)
#define VIEW_LIGHT_DEFAULT_OFFSET idVec3(VIEW_LIGHT_DEFAULT_OFFSET_VALUE)
#define VIEW_LIGHT_DEFAULT_COLOR idVec3(VIEW_LIGHT_DEFAULT_COLOR_VALUE)

/***********************************************************************

  idViewLight

***********************************************************************/

/***********************************************************************

	init

***********************************************************************/

/*
================
idViewLight::idViewLight()
================
*/
idViewLight::idViewLight() {
    owner				= NULL;
    on                  = false;
	lightDefHandle		= -1;
    lightOffset.Set(VIEW_LIGHT_DEFAULT_OFFSET_VALUE);
	memset(&renderLight, 0, sizeof(renderLight));
}

/*
================
idViewLight::~idViewLight()
================
*/
idViewLight::~idViewLight() {
    //gameLocal.Printf("Player view flash light removed.\n");
	if (lightDefHandle != -1) {
		gameRenderWorld->FreeLightDef(lightDefHandle);
	}
}

/*
================
idViewLight::Spawn
================
*/
void idViewLight::Spawn( const idDict &args ) {
	bool start_off;

    this->spawnArgs = args;

    Respawn();

    spawnArgs.GetBool("start_off", "0", start_off);
	if (start_off) {
		Off();
	}

    on = !start_off;
}

void idViewLight::Respawn(void)
{
    /*idVec3 lightRadius;
    if(spawnArgs.GetVector("view_radius", VIEW_LIGHT_DEFAULT_RADIUS_STR, lightRadius))
    {
        spawnArgs.Set("light_up", va("0 0 %f", lightRadius[2]));
        spawnArgs.Set("light_right", va("0 %f 0", lightRadius[1]));
        spawnArgs.Set("light_target", va("%f 0 0", lightRadius[0]));
    }*/

    // do the parsing the same way dmap and the editor do
	ParseSpawnArgsToRenderLight();

    // game specific functionality, not mirrored in
    // editor or dmap light parsing

    // also put the light texture on the model, so light flares
    // can get the current intensity of the light
    // lightDefHandle = -1;		// no static version yet

    // see if an optimized shadow volume exists
    // the renderer will ignore this value after a light has been moved,
    // but there may still be a chance to get it wrong if the game moves
    // a light before the first present, and doesn't clear the prelight

    spawnArgs.GetVector("view_offset", VIEW_LIGHT_DEFAULT_OFFSET_STR, lightOffset);

    PresentLightDefChange();
}

/*
================
idViewLight::Init
================
*/
void idViewLight::Init( idPlayer* _owner )
{
    owner = _owner;

	SetupRenderLight();
}

/*
================
idViewLight::PresentLight
================
*/
void idViewLight::PresentLight( bool showViewLight ) {
	if(!owner)
		return;

	if(!on || !showViewLight)
	{
		FreeLightDef();
        return;
	}

    // Dont do anything with the weapon while its stale
    // if ( fl.networkStale ) {
        // return;
    // }
	
// RAVEN BEGIN
// rjohnson: cinematics should never be done from the player's perspective, so don't think the weapon ( and their sounds! )
    if ( gameLocal.inCinematic ) {
        return;
    }
// RAVEN END

    // present the light
	SetupRenderLight();
	Present();
}

/*
================
idWeapon::EnterCinematic
================
*/
void idViewLight::EnterCinematic(void)
{
	Open(false);
}

/*
================
idWeapon::ExitCinematic
================
*/
void idViewLight::ExitCinematic(void)
{
	Open(true);
}

void idViewLight::Open( bool open )
{
    if(open)
    {
        On();
    }
    else
    {
        Off();
    }
}

void idViewLight::SetOn( bool open )
{
    on = open;
    Open(on);
}

void idViewLight::Toggle( void )
{
    on = !on;
    Open(on);
}

void idViewLight::SetupRenderLight( void )
{
	if(pm_thirdPerson.GetBool())
	{
		renderLight.allowLightInViewID = 0;
		renderLight.lightId = LIGHTID_VIEW_MUZZLE_FLASH + owner->entityNumber;
	}
	else
	{
		renderLight.allowLightInViewID = owner->entityNumber+1;
		renderLight.lightId = LIGHTID_VIEW_MUZZLE_FLASH + owner->entityNumber;
	}
}

/*
================
idViewLight::PresentLightDefChange
================
*/
void idViewLight::PresentLightDefChange(void)
{
	// let the renderer apply it to the world
	if ((lightDefHandle != -1)) {
		gameRenderWorld->UpdateLightDef(lightDefHandle, &renderLight);
	} else {
		lightDefHandle = gameRenderWorld->AddLightDef(&renderLight);
	}
}

/*
================
idViewLight::FreeLightDef
================
*/
void idViewLight::FreeLightDef(void)
{
	if (lightDefHandle != -1) {
		gameRenderWorld->FreeLightDef(lightDefHandle);
		lightDefHandle = -1;
	}
}

/*
================
idViewLight::On
================
*/
void idViewLight::On(void)
{
	// offset the start time of the shader to sync it to the game time
	renderLight.shaderParms[ SHADERPARM_TIMEOFFSET ] = -MS2SEC(gameLocal.time);

	PresentLightDefChange();
}

/*
================
idViewLight::Off
================
*/
void idViewLight::Off(void)
{
	FreeLightDef();
}

/*
================
idViewLight::Present
================
*/
void idViewLight::Present(void)
{
	// current transformation
#ifdef _MOD_FULL_BODY_AWARENESS
	if(!harm_pm_fullBodyAwareness.GetBool() || pm_thirdPerson.GetBool() || owner->focusUI)
    renderLight.origin  = owner->firstPersonViewOrigin + lightOffset * owner->firstPersonViewAxis;
	else
#endif
    renderLight.origin  = owner->firstPersonViewOrigin_viewWeaponOrigin + lightOffset * owner->firstPersonViewAxis;
	renderLight.axis	= owner->firstPersonViewAxis;

	// update the renderLight and renderEntity to render the light and flare
	PresentLightDefChange();
}

void idViewLight::SetParm(const char *name, const char *value)
{
    spawnArgs.Set(name, value);
    UpdateLight(true);
}

void idViewLight::UpdateLight(bool full)
{
    if(full)
		ParseSpawnArgsToRenderLight();
    if(on)
        PresentLightDefChange();
}

void idViewLight::ParseSpawnArgsToRenderLight(void)
{
	gameEdit->ParseSpawnArgsToRenderLight(&spawnArgs, &renderLight);
	if(renderLight.pointLight)
	{
		renderLight.target.Set(renderLight.lightRadius[0], 0.0f, 0.0f);
		renderLight.right.Set(0.0f, renderLight.lightRadius[1], 0.0f);
		renderLight.up.Set(0.0f, 0.0f, renderLight.lightRadius[2]);
	}
	else
	{
		idBounds bv;
		bv.Clear();
		bv.AddPoint(renderLight.target);
		bv.AddPoint(renderLight.right);
		bv.AddPoint(renderLight.up);
		renderLight.lightRadius = bv[1] - bv[0];
	}
}

/*
================
idViewLight::SetRadius
================
*/
void idViewLight::SetRadius(float x, float y, float z)
{
    renderLight.lightRadius[0] = x;
    renderLight.lightRadius[1] = y;
    renderLight.lightRadius[2] = z;

    renderLight.target.Set(x, 0.0f, 0.0f);
    renderLight.right.Set(0.0f, y, 0.0f);
    renderLight.up.Set(0.0f, 0.0f, z);
    renderLight.end = renderLight.target;

    spawnArgs.Set("light_target", va("%f 0 0", x));
    spawnArgs.Set("light_right", va("0 %f 0", y));
    spawnArgs.Set("light_up", va("0 0 %f", z));
	spawnArgs.Set("light_radius", va("%f %f %f", x, y, z));
    UpdateLight(false);
}

/*
================
idViewLight::SetRadius
================
*/
void idViewLight::SetRadius(const idVec3 &radius)
{
    SetRadius(radius[0], radius[1], radius[2]);
}

/*
================
idLight::SetColor
================
*/
void idViewLight::SetColor(float red, float green, float blue)
{
    renderLight.shaderParms[ SHADERPARM_RED ]		= red;
    renderLight.shaderParms[ SHADERPARM_GREEN ]		= green;
    renderLight.shaderParms[ SHADERPARM_BLUE ]		= blue;
    spawnArgs.Set("_color", va("%f %f %f", red, green, blue));
    UpdateLight(false);
}

/*
================
idLight::SetColor
================
*/
void idViewLight::SetColor(const idVec3 &color)
{
    SetColor(color[0], color[1], color[2]);
}

void idViewLight::SetOffset(float forward, float right, float up)
{
    lightOffset[0] = forward;
    lightOffset[1] = right;
    lightOffset[2] = up;
    spawnArgs.Set("view_offset", va("%f %f %f", forward, right, up));
    UpdateLight(false);
}

void idViewLight::SetOffset(const idVec3 &offset)
{
    SetOffset(offset[0], offset[1], offset[2]);
}

/*
================
idViewLight::SetShader
================
*/
bool idViewLight::SetShader(const char *shadername)
{
    const idMaterial *shader = declManager->FindMaterial(shadername, false);
    if(!shader)
        return false;
    spawnArgs.Set("texture", shadername);
    renderLight.shader = shader;
    UpdateLight(false);
    return true;
}

/*
================
idViewLight::SetLightParm
================
*/
void idViewLight::SetLightParm(int parmnum, float value)
{
    if ((parmnum < 0) || (parmnum >= MAX_ENTITY_SHADER_PARMS)) {
        gameLocal.Warning("shader parm index (%d) out of range", parmnum);
        return;
    }

    renderLight.shaderParms[ parmnum ] = value;
    UpdateLight(false);
}

/*
================
idViewLight::SetEntity
================
*/
bool idViewLight::SetEntity(const char *entityname)
{
    const idDecl *decl = declManager->FindType(DECL_ENTITYDEF, entityname, false);
    if(!decl)
        return false;
    const idDeclEntityDef *def = static_cast<const idDeclEntityDef *>(decl);
    Spawn(def->dict);
    UpdateLight(false);
    return true;
}

bool idViewLight::SetLight(const char *name)
{
    if(SetEntity(name))
        return true;
    else
        return SetShader(name);
}

void idViewLight::SetLightType(int type)
{
    switch(type)
    {
        case 1:
            renderLight.pointLight = true;
            renderLight.parallel = false;
            break;
        case 0:
        default:
            renderLight.parallel = false;
            renderLight.pointLight = false;
            break;
    }

    UpdateLight(false);
}
