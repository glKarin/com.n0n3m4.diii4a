// Unuse C++11 raw string literals for Traditional C++98

#include "doom3/BPR.inc.h"

#include "doom3/default.vert.h"
#include "doom3/default.frag.h"

#include "doom3/shadow.vert.h"
#include "doom3/shadow.frag.h"

#include "doom3/heatHaze.vert.h"
#include "doom3/heatHaze.frag.h"

#include "doom3/heatHazeWithMask.vert.h"
#include "doom3/heatHazeWithMask.frag.h"

#include "doom3/heatHazeWithMaskAndVertex.vert.h"
#include "doom3/heatHazeWithMaskAndVertex.frag.h"

#include "doom3/colorProcess.vert.h"
#include "doom3/colorProcess.frag.h"

#include "doom3/interaction.vert.h"
#include "doom3/interaction.frag.h"

#include "doom3/zfill.vert.h"
#include "doom3/zfill.frag.h"

#include "doom3/cubemap.vert.h"
#include "doom3/cubemap.frag.h"

#include "doom3/environment.vert.h"
#include "doom3/environment.frag.h"

#include "doom3/bumpyEnvironment.vert.h"
#include "doom3/bumpyEnvironment.frag.h"

#include "doom3/fog.vert.h"
#include "doom3/fog.frag.h"

#include "doom3/blendLight.vert.h"

#include "doom3/zfillClip.vert.h"
#include "doom3/zfillClip.frag.h"

#include "doom3/diffuseCubemap.vert.h"

#include "doom3/texgen.vert.h"
#include "doom3/texgen.frag.h"

#include "doom3/megaTexture.vert.h"
#include "doom3/megaTexture.frag.h"

#ifdef _SHADOW_MAPPING

#include "doom3/shadowMapping.inc.h"
#include "doom3/stencilShadow.inc.h"

#include "doom3/depthShadowMapping.vert.h"
#include "doom3/depthShadowMapping.frag.h"

#include "doom3/interactionShadowMapping.vert.h"
#include "doom3/interactionShadowMapping.frag.h"

#include "doom3/depthPerforated.vert.h"
#include "doom3/depthPerforated.frag.h"

#endif

#ifdef _STENCIL_SHADOW_IMPROVE

#include "doom3/interactionStencilShadow.vert.h"
#include "doom3/interactionStencilShadow.frag.h"

#endif

#ifdef _GLOBAL_ILLUMINATION

#include "doom3/globalIllumination.vert.h"
#include "doom3/globalIllumination.frag.h"

#endif

#include "doom3/plain.vert.h"
#include "doom3/plain.frag.h"

#ifdef _SPLASHDAMAGE

#include "doom3/occlusionTest.vert.h"
#include "doom3/occlusionTest.frag.h"

#endif

