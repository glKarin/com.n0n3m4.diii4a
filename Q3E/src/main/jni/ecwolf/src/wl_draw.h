#ifndef __WL_DRAW_H__
#define __WL_DRAW_H__

#include "tmemory.h"

/*
=============================================================================

							WL_DRAW DEFINITIONS

=============================================================================
*/

//
// math tables
//
extern  TUniquePtr<short[]> pixelangle;
extern  fixed finetangent[FINEANGLES/2 + ANG180];
extern	fixed finesine[FINEANGLES+FINEANGLES/4];
extern	fixed* finecosine;
extern  TUniquePtr<int[]> wallheight;
extern  word horizwall[],vertwall[];
extern  int32_t    frameon;
extern	int r_extralight;

extern  unsigned screenloc[3];

extern  bool fizzlein, fpscounter;

extern  fixed   viewx,viewy;                    // the focal point
extern  fixed   viewsin,viewcos;

void    ThreeDRefresh (void);

typedef struct
{
	word leftpix,rightpix;
	word dataofs[64];
// table data after dataofs[rightpix-leftpix+1]
} t_compshape;

extern bool UseWolf4SDL3DSpriteScaler;

#endif
