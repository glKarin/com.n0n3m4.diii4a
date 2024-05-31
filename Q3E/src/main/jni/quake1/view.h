#ifndef VIEW_H
#define VIEW_H

#include "qtypes.h"
#include "matrixlib.h"
struct cmd_state_s;

void V_Init (void);
void V_UpdateBlends (void);
void V_ParseDamage (void);
void V_DriftPitch(void);
void V_FadeViewFlashs(void);
void V_CalcViewBlend(void);
void V_CalcRefdefUsing (const matrix4x4_t *entrendermatrix, const vec3_t clviewangles, qbool teleported, qbool clonground, qbool clcmdjump, float clstatsviewheight, qbool cldead, const vec3_t clvelocity);
void V_CalcRefdef(void);
void V_MakeViewIsometric(void);
void V_StartPitchDrift(void);
void V_StopPitchDrift (void);
void V_StartPitchDrift_f(struct cmd_state_s *cmd);

#endif
