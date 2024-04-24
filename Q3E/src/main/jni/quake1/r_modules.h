
#ifndef R_MODULES_H
#define R_MODULES_H

void R_Modules_Init(void);
void R_RegisterModule(const char *name, void(*start)(void), void(*shutdown)(void), void(*newmap)(void), void(*devicelost)(void), void(*devicerestored)(void));
void R_Modules_Start(void);
void R_Modules_Shutdown(void);
void R_Modules_NewMap(void);
void R_Modules_Restart_f(struct cmd_state_s *cmd);
void R_Modules_DeviceLost(void);
void R_Modules_DeviceRestored(void);

#endif

