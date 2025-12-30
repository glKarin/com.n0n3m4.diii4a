#include "port_act_defs.h"



#ifdef __cplusplus
extern "C"
{
#endif
int PortableKeyEvent(int state, int code ,int unitcode);
void PortableAction(int state, int action);

void PortableMove(float fwd, float strafe);
void PortableMoveFwd(float fwd);
void PortableMoveSide(float strafe);
void PortableLookPitch(int mode, float pitch);
void PortableLookYaw(int mode, float pitch);
void PortableCommand(const char * cmd);

void PortableMouse(float dx,float dy);

void PortableMouseAbs(float x,float y);

void PortableInit(int argc,const char ** argv);
void PortableFrame(void);

int PortableInMenu(void);
int PortableShowKeyboard(void);
int PortableInAutomap(void);


#ifdef __cplusplus
}
#endif
