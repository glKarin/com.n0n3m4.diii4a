#include "SDL.h"

#include "../wl_def.h"
#include "../wl_play.h"
#include "../c_cvars.h"

extern bool	ingame;
extern  bool menusAreFaded;
extern int inConversation;

bool inConfirm = false;

extern void Sys_SyncState();
extern void Sys_Analog(int &side, int &forward, const int delta);

int PortableInMenu(void){
    //return (ingame)?0:1;
    return (!menusAreFaded) || !ingame || inConfirm || inConversation;
}

extern "C" {
int hasHardwareKeyboard()
{
    return 0;
}
}

//NOTE this is cpp
void pollAndroidControls()
{
    TicCmd_t &cmd = control[ConsolePlayer];
    const int delta = (!alwaysrun && cmd.buttonstate[bt_run]) || (alwaysrun && !cmd.buttonstate[bt_run]) ? RUNMOVE : BASEMOVE;
    Sys_Analog(cmd.controlstrafe, cmd.controly, delta);
#if 0
    control[ConsolePlayer].controly  -= forwardmove     * (alwaysrun ? RUNMOVE:BASEMOVE);
    control[ConsolePlayer].controlstrafe  += sidemove   * (alwaysrun ? RUNMOVE:BASEMOVE);


    switch(look_yaw_mode)
    {
        case LOOK_MODE_MOUSE:
            control[ConsolePlayer].controlx += -look_yaw_mouse * 8000;
            look_yaw_mouse = 0;
            break;
        case LOOK_MODE_JOYSTICK:
            control[ConsolePlayer].controlx += -look_yaw_joy * 80;
            break;
    }

    for (int n=0;n<NUMBUTTONS;n++)
    {
        if (my_buttonstate[n])
            control[ConsolePlayer].buttonstate[n] = 1;
    }
#endif
}

void PostSDLInit(SDL_Window *Screen)
{
#if 0
    SDL_AddEventWatch(Android_EventWatch, NULL);

    SDL_DisplayMode mode;
    SDL_GetWindowDisplayMode(Screen, &mode);
    Android_SetScreenSize(mode.w, mode.h);
#endif
}

void frameControls()
{
    Sys_SyncState();
#if 0
    //LOGI("frameControls\n");

    int inMenuNew = PortableInMenu();
    if (inMenuLast != inMenuNew)
    {
        inMenuLast = inMenuNew;
        if (!inMenuNew)
        {
            tcGameMain->setEnabled(true);
            if (enableWeaponWheel)
                tcWeaponWheel->setEnabled(true);
            tcMenuMain->setEnabled(false);
        }
        else
        {
            tcGameMain->setEnabled(false);
            tcGameWeapons->setEnabled(false);
            tcWeaponWheel->setEnabled(false);
            tcMenuMain->setEnabled(true);
        }
    }


    weaponCycle(showWeaponCycle);
    setHideSticks(!showSticks);
    controlsContainer.draw();

    // glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    // glClear(GL_COLOR_BUFFER_BIT);
#endif
}