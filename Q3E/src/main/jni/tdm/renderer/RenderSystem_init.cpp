/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#include "precompiled.h"
#pragma hdrstop

#include "renderer/tr_local.h"
#include "renderer/backend/FrameBuffer.h"
#include "renderer/backend/GLSLProgramManager.h"
#include "renderer/backend/RenderBackend.h"
#include "renderer/backend/stages/AmbientOcclusionStage.h"
#include "renderer/backend/stages/BloomStage.h"
#include "renderer/backend/stages/VolumetricStage.h"
#include "renderer/backend/FrameBufferManager.h"
#include "sys/sys_padinput.h"

// Vista OpenGL wrapper check
#ifdef _WIN32
#include "sys/win32/win_local.h"
#endif

// functions that are not called every frame

glconfig_t	glConfig;

idCVar r_glDriver( "r_glDriver", "", CVAR_RENDERER, "\"opengl32.dll\", etc." );
idCVar r_glDebugOutput( "r_glDebugOutput", "0", CVAR_RENDERER | CVAR_INTEGER | CVAR_ARCHIVE, "Enables GL debug messages and displays them on the console. Using a debug context may provide additional insight. 2 - enables synchronous processing (slower)" );
idCVar r_glDebugContext( "r_glDebugContext", "0", CVAR_RENDERER | CVAR_BOOL | CVAR_ARCHIVE, "If enabled, create a GL debug context." );
idCVar r_multiSamples( "r_multiSamples", "0", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, "number of antialiasing samples" );
idCVar r_displayRefresh( "r_displayRefresh", "0", CVAR_RENDERER | CVAR_INTEGER | CVAR_NOCHEAT, "optional display refresh rate option for vid mode", 0.0f, 200.0f );
idCVar r_fullscreen( "r_fullscreen", "2", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, "0 = windowed, 1 = full screen, 2 = fullscreen windowed" );
idCVar r_singleTriangle( "r_singleTriangle", "0", CVAR_RENDERER | CVAR_BOOL, "only draw a single triangle per primitive" );
idCVar r_checkBounds( "r_checkBounds", "0", CVAR_RENDERER | CVAR_BOOL, "compare all surface bounds with precalculated ones" );

idCVar r_useConstantMaterials( "r_useConstantMaterials", "1", CVAR_RENDERER | CVAR_BOOL, "use pre-calculated material registers if possible" );
idCVar r_useSilRemap( "r_useSilRemap", "1", CVAR_RENDERER | CVAR_BOOL, "consider verts with the same XYZ, but different ST the same for shadows" );
idCVar r_useNodeCommonChildren( "r_useNodeCommonChildren", "1", CVAR_RENDERER | CVAR_BOOL, "stop pushing reference bounds early when possible" );
idCVar r_useShadowProjectedCull( "r_useShadowProjectedCull", "1", CVAR_RENDERER | CVAR_BOOL, "discard triangles outside light volume before shadowing" );
idCVar r_useShadowSurfaceScissor( "r_useShadowSurfaceScissor", "1", CVAR_RENDERER | CVAR_BOOL, "scissor shadows by the scissor rect of the interaction surfaces" );
idCVar r_useInteractionTable( "r_useInteractionTable", "2", CVAR_RENDERER | CVAR_INTEGER, "which implementation to use for table of existing interactions: 0 = none, 1 = single full matrix, 2 = single hash table" );
idCVar r_useTurboShadow( "r_useTurboShadow", "1", CVAR_RENDERER | CVAR_BOOL, "use the infinite projection with W technique for dynamic shadows" );
idCVar r_useDeferredTangents( "r_useDeferredTangents", "1", CVAR_RENDERER | CVAR_BOOL, "defer tangents calculations after deform" );
idCVar r_useCachedDynamicModels( "r_useCachedDynamicModels", "1", CVAR_RENDERER | CVAR_BOOL, "cache snapshots of dynamic models" );

//duzenko & stgatilov:
idCVar r_softShadowsQuality( "r_softShadowsQuality", "0", CVAR_RENDERER | CVAR_INTEGER | CVAR_ARCHIVE, "Number of samples in soft shadows blur. 0 = hard shadows, 6 = low-quality, 24 = good, 96 = perfect" );
idCVar r_softShadowsRadius( "r_softShadowsRadius", "1.0", CVAR_RENDERER | CVAR_FLOAT | CVAR_ARCHIVE, "Radius of light source for soft shadows. Decreasing it makes soft shadows less blurry." );

//stgatilov #4825: see also
//  http://forums.thedarkmod.com/topic/19139-nonsmooth-graphics-due-to-bumpmapping/
idCVar r_useBumpmapLightTogglingFix(
	"r_useBumpmapLightTogglingFix", "1", CVAR_RENDERER | CVAR_BOOL,
	"Reduce light toggling due to difference between bumpmapped normal and interpolated normal. "
	"Only single-sided surfaces are affected. "
);

idCVar r_useStateCaching( "r_useStateCaching", "1", CVAR_RENDERER | CVAR_BOOL, "avoid redundant state changes in GL_*() calls" );

idCVar r_znear( "r_znear", "3", CVAR_RENDERER | CVAR_FLOAT, "near Z clip plane distance", 0.001f, 200.0f );

idCVar r_ignoreGLErrors( "r_ignoreGLErrors", 
#ifdef _DEBUG 
	"0" 
#else 
	"1"
#endif
	, CVAR_RENDERER | CVAR_BOOL, "ignore GL errors" );
idCVar r_finish( "r_finish", "0", CVAR_RENDERER | CVAR_BOOL, "force a call to glFinish() every frame" );
idCVarInt r_swapInterval( "r_swapInterval", "0", CVAR_RENDERER | CVAR_ARCHIVE, "changes wglSwapIntarval" );

idCVar r_ambientMinLevel( "r_ambientMinLevel", "0", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_FLOAT, "specifies minimal level of ambient light brightness, making linear change in ambient color", 0.0f, 1.0f);
idCVar r_ambientGamma( "r_ambientGamma", "1", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_FLOAT, "specifies power of gamma correction applied solely to ambient light", 0.1f, 3.0f);

idCVar r_jitter( "r_jitter", "0", CVAR_RENDERER | CVAR_BOOL, "randomly subpixel jitter the projection matrix" );

idCVar r_skipSuppress( "r_skipSuppress", "0", CVAR_RENDERER | CVAR_BOOL, "ignore the per-view suppressions" );
idCVar r_skipPostProcess( "r_skipPostProcess", "0", CVAR_RENDERER | CVAR_BOOL, "skip all post-process renderings" );
idCVar r_skipInteractions( "r_skipInteractions", "0", CVAR_RENDERER | CVAR_INTEGER, "skip all light/surface interaction drawing" );
idCVar r_skipDynamicTextures( "r_skipDynamicTextures", "0", CVAR_RENDERER | CVAR_BOOL, "don't dynamically create textures" );
idCVar r_skipEntities( "r_skipEntities", "0", CVAR_RENDERER | CVAR_BOOL, "draw only world geometry, skip all entity models rendering" );
idCVar r_skipCopyTexture( "r_skipCopyTexture", "0", CVAR_RENDERER | CVAR_BOOL, "do all rendering, but don't actually copyTexSubImage2D" );
idCVar r_skipBackEnd( "r_skipBackEnd", "0", CVAR_RENDERER | CVAR_BOOL, "don't draw anything" );
idCVar r_skipRender( "r_skipRender", "0", CVAR_RENDERER | CVAR_INTEGER, "skip 3D rendering, but pass 2D" );
idCVar r_skipRenderContext( "r_skipRenderContext", "0", CVAR_RENDERER | CVAR_BOOL, "NULL the rendering context during backend 3D rendering" );
idCVar r_skipTranslucent( "r_skipTranslucent", "0", CVAR_RENDERER | CVAR_BOOL, "skip the translucent interaction rendering" );
idCVar r_skipAmbient( "r_skipAmbient", "0", CVAR_RENDERER | CVAR_INTEGER, "1 = bypasses all non-interaction drawing, 2 = skips ambient light interactions, 3 = both" );
idCVarInt r_skipNewAmbient( "r_skipNewAmbient", "0", CVAR_RENDERER, "bypasses non-standard ambient drawing, 1 - per-material, 2 - soft particles, 3 - both" );
idCVar r_skipBlendLights( "r_skipBlendLights", "0", CVAR_RENDERER | CVAR_BOOL, "skip all blend lights" );
idCVarInt r_skipFogLights( "r_skipFogLights", "0", CVAR_RENDERER, "skip fog lights: bitmask 1 - solid, 2 - translucent, 4 - bounding box" );
idCVar r_skipDeforms( "r_skipDeforms", "0", CVAR_RENDERER | CVAR_BOOL, "leave all deform materials in their original state" );
idCVar r_skipFrontEnd( "r_skipFrontEnd", "0", CVAR_RENDERER | CVAR_BOOL, "bypasses all front end work, but 2D gui rendering still draws" );
idCVar r_skipUpdates( "r_skipUpdates", "0", CVAR_RENDERER | CVAR_BOOL, "1 = don't accept any entity or light updates, making everything static" );
idCVar r_skipOverlays( "r_skipOverlays", "0", CVAR_RENDERER | CVAR_BOOL, "skip overlay surfaces" );
idCVar r_skipSpecular( "r_skipSpecular", "0", CVAR_RENDERER | CVAR_BOOL, "use black for specular1" );
idCVar r_skipBump( "r_skipBump", "0", CVAR_RENDERER | CVAR_BOOL, "uses a flat surface instead of the bump map" );
idCVar r_skipDiffuse( "r_skipDiffuse", "0", CVAR_RENDERER | CVAR_BOOL, "use black for diffuse" );
idCVar r_skipROQ( "r_skipROQ", "0", CVAR_RENDERER | CVAR_BOOL, "skip ROQ decoding" );
idCVar r_skipDepthCapture( "r_skipDepthCapture", "0", CVAR_RENDERER | CVAR_BOOL, "skip depth capture" ); // #3877 #4418
idCVar r_useSoftParticles( "r_useSoftParticles", "1", CVAR_RENDERER | CVAR_BOOL, "soften particle transitions when player walks through them or they cross solid geometry" ); // #3878 #4418

idCVar r_ignore( "r_ignore", "0", CVAR_RENDERER, "used for random debugging without defining new vars" );
idCVar r_ignore2( "r_ignore2", "0", CVAR_RENDERER, "used for random debugging without defining new vars" );
idCVar r_usePreciseTriangleInteractions( "r_usePreciseTriangleInteractions", "0", CVAR_RENDERER | CVAR_BOOL, "1 = do winding clipping to determine if each ambiguous tri should be lit" );
idCVar r_useCulling( "r_useCulling", "2", CVAR_RENDERER | CVAR_INTEGER, "0 = none, 1 = sphere, 2 = sphere + box", 0, 2, idCmdSystem::ArgCompletion_Integer<0, 2> );
idCVar r_useLightScissors( "r_useLightScissors", "1", CVAR_RENDERER | CVAR_BOOL, "1 = use custom scissor rectangle for each light" );
//anon begin
idCVar r_useLightPortalCulling( "r_useLightPortalCulling", "1", CVAR_RENDERER | CVAR_INTEGER, "0 = none, 1 = cull frustum corners to plane, 2 = exact clip the frustum faces", 0, 2, idCmdSystem::ArgCompletion_Integer<0, 2> );
idCVar r_useEntityPortalCulling( "r_useEntityPortalCulling", "1", CVAR_RENDERER | CVAR_INTEGER, "0 = none, 1 = cull frustum corners to plane, 2 = exact clip the frustum faces", 0, 2, idCmdSystem::ArgCompletion_Integer<0, 2> );
//anon end
idCVar r_useClippedLightScissors( "r_useClippedLightScissors", "1", CVAR_RENDERER | CVAR_INTEGER, "0 = full screen when near clipped, 1 = exact when near clipped, 2 = exact always", 0, 2, idCmdSystem::ArgCompletion_Integer<0, 2> );
idCVar r_useEntityCulling( "r_useEntityCulling", "1", CVAR_RENDERER | CVAR_BOOL, "0 = none, 1 = box" );
idCVar r_useEntityScissors( "r_useEntityScissors", "1", CVAR_RENDERER | CVAR_BOOL, "1 = use custom scissor rectangle for each entity" );
idCVar r_useInteractionCulling( "r_useInteractionCulling", "1", CVAR_RENDERER | CVAR_BOOL, "1 = cull interactions" );
idCVar r_useInteractionScissors( "r_useInteractionScissors", "2", CVAR_RENDERER | CVAR_INTEGER, "1 = use a custom scissor rectangle for each shadow interaction, 2 = also crop using portal scissors", -2, 2, idCmdSystem::ArgCompletion_Integer < -2, 2 > );
idCVar r_useShadowCulling( "r_useShadowCulling", "1", CVAR_RENDERER | CVAR_BOOL, "try to cull shadows from partially visible lights" );
idCVar r_useFrustumFarDistance( "r_useFrustumFarDistance", "0", CVAR_RENDERER | CVAR_FLOAT, "if != 0 force the view frustum far distance to this distance" );
idCVar r_logFile( "r_logFile", "0", CVAR_RENDERER | CVAR_INTEGER, "number of frames to emit GL logs" );
idCVar r_clear( "r_clear", "2", CVAR_RENDERER,
	"force screen clear every frame, 1 = purple, 2 = black, 'r g b' = custom\n"
	"Note: TDM requires black color to work properly!"	// e.g. skybox and "attack when ready" screen
);
idCVar r_offsetFactor( "r_offsetfactor", "-2", CVAR_RENDERER | CVAR_FLOAT | CVAR_ARCHIVE, "polygon offset parameter" ); // #4079
idCVar r_offsetUnits( "r_offsetunits", "-0.1", CVAR_RENDERER | CVAR_FLOAT | CVAR_ARCHIVE, "polygon offset parameter" ); // #4079
idCVar r_shadowPolygonOffset( "r_shadowPolygonOffset", "-1", CVAR_RENDERER | CVAR_FLOAT | CVAR_ARCHIVE, "bias value added to depth test for stencil shadow drawing" );
idCVar r_shadowPolygonFactor( "r_shadowPolygonFactor", "0", CVAR_RENDERER | CVAR_FLOAT | CVAR_ARCHIVE, "scale value for stencil shadow drawing" );
idCVar r_frontBuffer( "r_frontBuffer", "0", CVAR_RENDERER | CVAR_BOOL, "draw to front buffer for debugging" );
idCVarBool r_skipSubviews( "r_skipSubviews", "0", CVAR_RENDERER, "1 = don't render mirrors, portals, etc" );
idCVar r_skipGuiShaders( "r_skipGuiShaders", "0", CVAR_RENDERER | CVAR_INTEGER, "1 = skip all gui elements on surfaces, 2 = skip drawing but still handle events, 3 = draw but skip events", 0, 3, idCmdSystem::ArgCompletion_Integer<0, 3> );
idCVar r_skipParticles( "r_skipParticles", "0", CVAR_RENDERER | CVAR_INTEGER, "1 = skip all particle systems", 0, 1, idCmdSystem::ArgCompletion_Integer<0, 1> );
idCVar r_subviewOnly( "r_subviewOnly", "0", CVAR_RENDERER | CVAR_BOOL, "1 = don't render main view, allowing subviews to be debugged" );
idCVar r_shadows( "r_shadows", "1", CVAR_RENDERER | CVAR_INTEGER  | CVAR_ARCHIVE, "1 = stencil shadows, 2 = shadow maps" );
idCVar r_testGamma( "r_testGamma", "0", CVAR_RENDERER | CVAR_FLOAT, "if > 0 draw a grid pattern to test gamma levels", 0, 195 );
idCVar r_testGammaBias( "r_testGammaBias", "0", CVAR_RENDERER | CVAR_FLOAT, "if > 0 draw a grid pattern to test gamma levels" );
idCVar r_testStepGamma( "r_testStepGamma", "0", CVAR_RENDERER | CVAR_FLOAT, "if > 0 draw a grid pattern to test gamma levels" );
idCVar r_lightScale( "r_lightScale", "2", CVAR_RENDERER | CVAR_FLOAT, "all light intensities are multiplied by this" );
idCVar r_lightSourceRadius( "r_lightSourceRadius", "0", CVAR_RENDERER | CVAR_FLOAT | CVAR_ARCHIVE, "screenshot soft-shadows" );
idCVar r_flareSize( "r_flareSize", "1", CVAR_RENDERER | CVAR_FLOAT, "scale the flare deforms from the material def" );

idCVar r_useExternalShadows( "r_useExternalShadows", "1", CVAR_RENDERER | CVAR_INTEGER, "1 = skip drawing caps when outside the light volume", 0, 1, idCmdSystem::ArgCompletion_Integer<0, 1> );
idCVar r_useOptimizedShadows( "r_useOptimizedShadows", "1", CVAR_RENDERER | CVAR_BOOL, "use the dmap generated static shadow volumes" );
idCVar r_useScissor( "r_useScissor", "1", CVAR_RENDERER | CVAR_BOOL, "scissor clip as portals and lights are processed" );
idCVar r_useDepthBoundsTest( "r_useDepthBoundsTest", "1", CVAR_RENDERER | CVAR_BOOL, "use depth bounds test to reduce shadow fill" );

idCVar r_screenFraction( "r_screenFraction", "100", CVAR_RENDERER | CVAR_INTEGER, "for testing fill rate, the resolution of the entire screen can be changed" );
idCVar r_demonstrateBug( "r_demonstrateBug", "0", CVAR_RENDERER | CVAR_BOOL, "used during development to show IHV's their problems" );
idCVar r_usePortals( "r_usePortals", "1", CVAR_RENDERER | CVAR_BOOL, " 1 = use portals to perform area culling, otherwise draw everything" );
idCVar r_singleLight( "r_singleLight", "-1", CVAR_RENDERER | CVAR_INTEGER, "suppress all but one light" );
idCVar r_singleEntity( "r_singleEntity", "-1", CVAR_RENDERER | CVAR_INTEGER, "suppress all but one entity" );
idCVar r_singleSurface( "r_singleSurface", "-1", CVAR_RENDERER | CVAR_INTEGER, "suppress all but one surface on each entity" );
idCVar r_singleArea( "r_singleArea", "0", CVAR_RENDERER | CVAR_BOOL, "only draw the portal area the view is actually in" );
idCVar r_forceLoadImages( "r_forceLoadImages", "0", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_BOOL, "draw all images to screen after registration" );
idCVar r_orderIndexes( "r_orderIndexes", "1", CVAR_RENDERER | CVAR_BOOL, "perform index reorganization to optimize vertex use" );
idCVar r_lightAllBackFaces( "r_lightAllBackFaces", "0", CVAR_RENDERER | CVAR_BOOL, "light all the back faces, even when they would be shadowed" );
idCVar r_skipModels( "r_skipModels", "0", CVAR_RENDERER | CVAR_INTEGER, "0 - draw all, 1 - static only, 2 - dynamic only" );

// visual debugging info
idCVarInt r_showPortals( "r_showPortals", "0", CVAR_RENDERER, "draw portal outlines in color: green = player sees through portal; yellow = not seen through but visleaf is open through another portal; red = portal and visleaf the other side are closed." );
idCVar r_showUnsmoothedTangents( "r_showUnsmoothedTangents", "0", CVAR_RENDERER | CVAR_BOOL, "if 1, put all nvidia register combiner programming in display lists" );
idCVar r_showSilhouette( "r_showSilhouette", "0", CVAR_RENDERER | CVAR_BOOL, "highlight edges that are casting shadow planes" );
idCVar r_showVertexColor( "r_showVertexColor", "0", CVAR_RENDERER | CVAR_BOOL, "draws all triangles with the solid vertex color" );
idCVar r_showUpdates( "r_showUpdates", "0", CVAR_RENDERER | CVAR_BOOL, "report entity and light updates and ref counts" );
idCVar r_showDemo( "r_showDemo", "0", CVAR_RENDERER | CVAR_BOOL, "report reads and writes to the demo file" );
idCVar r_showDynamic( "r_showDynamic", "0", CVAR_RENDERER | CVAR_BOOL, "report stats on dynamic surface generation" );
idCVar r_showDefs( "r_showDefs", "0", CVAR_RENDERER | CVAR_BOOL, "report the number of modeDefs and lightDefs in view" );
idCVar r_showTrace( "r_showTrace", "0", CVAR_RENDERER | CVAR_INTEGER, "show the intersection of an eye trace with the world", idCmdSystem::ArgCompletion_Integer<0, 2> );
idCVar r_showIntensity( "r_showIntensity", "0", CVAR_RENDERER | CVAR_BOOL, "draw the screen colors based on intensity, red = 0, green = 128, blue = 255" );
idCVar r_showImages( "r_showImages", "0", CVAR_RENDERER | CVAR_INTEGER, "1 = show all images instead of rendering, 2 = show in proportional size", 0, 2, idCmdSystem::ArgCompletion_Integer<0, 2> );
idCVar com_smp( "com_smp", "1", CVAR_RENDERER | CVAR_BOOL | CVAR_ARCHIVE, "run game modeling and renderer frontend in second thread, parallel to renderer backend" );
idCVar r_showSmp( "r_showSmp", "0", CVAR_RENDERER | CVAR_BOOL, "show which end (front or back) is blocking" );
idCVarInt r_showLights( "r_showLights", "0", CVAR_RENDERER, "bitmask: 1 = print volumes numbers, highlighting ones covering the view, 2 = draw planes of each volume, 4 = draw edges of each volume, 8 = draw edges of BFG frustum" );
idCVar r_showShadows( "r_showShadows", "0", CVAR_RENDERER | CVAR_INTEGER, "1 = visualize the stencil shadow volumes, 2 = draw filled in, 3 = lines with depth test", -1, 3, idCmdSystem::ArgCompletion_Integer < -1, 3 > );
idCVar r_showShadowCount( "r_showShadowCount", "0", CVAR_RENDERER | CVAR_INTEGER, "colors screen based on shadow volume depth complexity, >= 2 = print overdraw count based on stencil index values, 3 = only show turboshadows, 4 = only show static shadows", 0, 4, idCmdSystem::ArgCompletion_Integer<0, 4> );
idCVar r_showLightScissors( "r_showLightScissors", "0", CVAR_RENDERER | CVAR_INTEGER, "show light scissor rectangles" );
idCVar r_showEntityScissors( "r_showEntityScissors", "0", CVAR_RENDERER | CVAR_BOOL, "show entity scissor rectangles" );
idCVar r_showInteractionFrustums( "r_showInteractionFrustums", "0", CVAR_RENDERER | CVAR_INTEGER, "1 = show a frustum for each interaction, 2 = also draw lines to light origin, 3 = also draw entity bbox", 0, 3, idCmdSystem::ArgCompletion_Integer<0, 3> );
idCVar r_showInteractionScissors( "r_showInteractionScissors", "0", CVAR_RENDERER | CVAR_INTEGER, "1 = show screen rectangle which contains the interaction frustum, 2 = also draw construction lines", 0, 2, idCmdSystem::ArgCompletion_Integer<0, 2> );
idCVar r_showLightCount( "r_showLightCount", "0", CVAR_RENDERER | CVAR_INTEGER, "1 = colors surfaces based on light count, 2 = also count everything through walls, 3 = also print overdraw", 0, 3, idCmdSystem::ArgCompletion_Integer<0, 3> );
idCVar r_showViewEntitys( "r_showViewEntitys", "0", CVAR_RENDERER | CVAR_INTEGER, "1 = displays the bounding boxes of all view models, 2 = print index numbers, 3 = index number and render model name" );
idCVarInt r_showEntityDraws( "r_showEntityDraws", "0", CVAR_RENDERER, "show effective draw calls per model (bitmask 2=grouped 3=vertex)" );
idCVar r_showTris( "r_showTris", "0", CVAR_RENDERER | CVAR_INTEGER, "enables wireframe rendering of the world, 1 = only draw visible ones, 2 = draw all front facing, 3 = draw all", 0, 4, idCmdSystem::ArgCompletion_Integer<0, 4> );
idCVar r_showSurfaceInfo( "r_showSurfaceInfo", "0", CVAR_RENDERER | CVAR_INTEGER, "show surface material name under crosshair" );
idCVar r_showNormals( "r_showNormals", "0", CVAR_RENDERER | CVAR_FLOAT, "draws wireframe normals" );
idCVar r_showMemory( "r_showMemory", "0", CVAR_RENDERER | CVAR_BOOL, "print frame memory utilization" );
idCVar r_showCull( "r_showCull", "0", CVAR_RENDERER | CVAR_BOOL, "report sphere and box culling stats" );
idCVar r_showInteractions( "r_showInteractions", "0", CVAR_RENDERER | CVAR_BOOL, "report interaction generation activity" );
idCVar r_showDepth( "r_showDepth", "0", CVAR_RENDERER | CVAR_BOOL, "display the contents of the depth buffer and the depth range" );
idCVar r_showSurfaces( "r_showSurfaces", "0", CVAR_RENDERER | CVAR_BOOL, "report surface/light/shadow counts" );
idCVar r_showPrimitives( "r_showPrimitives", "0", CVAR_RENDERER | CVAR_INTEGER, "report drawsurf/index/vertex counts" );
idCVar r_showEdges( "r_showEdges", "0", CVAR_RENDERER | CVAR_BOOL, "draw the sil edges" );
idCVar r_showTexturePolarity( "r_showTexturePolarity", "0", CVAR_RENDERER | CVAR_BOOL, "shade triangles by texture area polarity" );
idCVar r_showTangentSpace( "r_showTangentSpace", "0", CVAR_RENDERER | CVAR_INTEGER, "shade triangles by tangent space, 1 = use 1st tangent vector, 2 = use 2nd tangent vector, 3 = use normal vector", 0, 3, idCmdSystem::ArgCompletion_Integer<0, 3> );
idCVar r_showDominantTri( "r_showDominantTri", "0", CVAR_RENDERER | CVAR_BOOL, "draw lines from vertexes to center of dominant triangles" );
idCVar r_showAlloc( "r_showAlloc", "0", CVAR_RENDERER | CVAR_BOOL, "report alloc/free counts" );
idCVar r_showTextureVectors( "r_showTextureVectors", "0", CVAR_RENDERER | CVAR_FLOAT, " if > 0 draw each triangles texture (tangent) vectors" );
idCVar r_showOverDraw( "r_showOverDraw", "0", CVAR_RENDERER | CVAR_INTEGER, "1 = geometry overdraw, 2 = light interaction overdraw, 3 = geometry and light interaction overdraw", 0, 3, idCmdSystem::ArgCompletion_Integer<0, 3> );

idCVar r_lockSurfaces( "r_lockSurfaces", "0", CVAR_RENDERER | CVAR_BOOL, "allow moving the view point without changing the composition of the scene, including culling" );
idCVar r_useEntityCallbacks( "r_useEntityCallbacks", "1", CVAR_RENDERER | CVAR_BOOL, "if 0, issue the callback immediately at update time, rather than defering" );

idCVar r_showSkel( "r_showSkel", "0", CVAR_RENDERER | CVAR_INTEGER, "draw the skeleton when model animates, 1 = draw model with skeleton, 2 = draw skeleton only", 0, 2, idCmdSystem::ArgCompletion_Integer<0, 2> );
idCVar r_jointNameScale( "r_jointNameScale", "0.02", CVAR_RENDERER | CVAR_FLOAT, "size of joint names when r_showskel is set to 1" );
idCVar r_jointNameOffset( "r_jointNameOffset", "0.5", CVAR_RENDERER | CVAR_FLOAT, "offset of joint names when r_showskel is set to 1" );

idCVar r_debugLineDepthTest( "r_debugLineDepthTest", "0", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_BOOL, "perform depth test on debug lines" );
idCVar r_debugLineWidth( "r_debugLineWidth", "1", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_BOOL, "width of debug lines" );
idCVar r_debugArrowStep( "r_debugArrowStep", "120", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, "step size of arrow cone line rotation in degrees", 0, 120 );
idCVar r_debugPolygonFilled( "r_debugPolygonFilled", "1", CVAR_RENDERER | CVAR_BOOL, "draw a filled polygon" );

idCVar r_materialOverride( "r_materialOverride", "", CVAR_RENDERER, "overrides all materials", idCmdSystem::ArgCompletion_Decl<DECL_MATERIAL> );

idCVar r_showRenderToTexture( "r_showRenderToTexture", "0", CVAR_RENDERER | CVAR_INTEGER, "" );

// greebo: screenshot format CVAR, by default convert the generated TGA to JPG
idCVar r_screenshot_format(	"r_screenshot_format", "jpg",   CVAR_RENDERER | CVAR_ARCHIVE, "Image format used to store ingame screenshots: png/tga/jpg/bmp." );

// rebb: toggle for dedicated ambient light shader use, mainly for performance testing
idCVar r_dedicatedAmbient( "r_dedicatedAmbient", "1", CVAR_RENDERER | CVAR_BOOL, "enable dedicated ambientLight shader" );

// 2016-2018 additions by duzenko
idCVar r_useAnonreclaimer( "r_useAnonreclaimer", "0", CVAR_RENDERER | CVAR_BOOL | CVAR_ARCHIVE, "test anonreclaimer patch" );
//stgatilov: temporary cvar, to be removed when ARB->GLSL migration is complete and settled
idCVar r_glCoreProfile( "r_glCoreProfile", "2", CVAR_RENDERER | CVAR_ARCHIVE,
	"Which profile of OpenGL to use:\n"
	"  0: compatibility profile\n"
	"  1: core profile\n"
	"  2: forward-compatible core profile\n"
	"Note: restarting TDM is required after change!"
);

// FBO
idCVar r_showFBO( "r_showFBO", "0", CVAR_RENDERER | CVAR_INTEGER, "0-5 individual fbo attachments" );
idCVar r_fboColorBits(
	"r_fboColorBits", "64", CVAR_RENDERER | CVAR_INTEGER | CVAR_ARCHIVE,
	"Total number of color bits in every pixel of rendered image.\n"
	"Must be one of: 16, 32, 64"
);
idCVarBool r_fboSRGB( "r_fboSRGB", "0", CVAR_RENDERER | CVAR_ARCHIVE, "Use framebuffer-level gamma correction" );
idCVar r_fboDepthBits( "r_fboDepthBits", "24", CVAR_RENDERER | CVAR_INTEGER | CVAR_ARCHIVE, "16, 24, 32" );
idCVarInt r_shadowMapSize( "r_shadowMapSize", "1024", CVAR_RENDERER | CVAR_INTEGER | CVAR_ARCHIVE, "Shadow map texture resolution" );

// relocate stgatilov ROQ options
idCVar r_cinematic_legacyRoq( "r_cinematic_legacyRoq", "0", CVAR_RENDERER | CVAR_INTEGER | CVAR_ARCHIVE,
                              "Play cinematics with original Doom3 code or with FFmpeg libraries. "
                              "0 - always use FFmpeg libraries, 1 - use original Doom3 code for ROQ and FFmpeg for other videos, 2 - never use FFmpeg" );

namespace {
	std::map<int, int> glDebugMessageIdLastSeenInFrame;
	const int SUPPRESS_FOR_NUM_FRAMES = 300;
}

static void APIENTRY R_OpenGLDebugMessageCallback( GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam ) {
	if ( type == GL_DEBUG_TYPE_PUSH_GROUP || type == GL_DEBUG_TYPE_POP_GROUP ) {
		// this is gl[Push|Pop]DebugGroup that we called ourselves
		return;
	}
	if ( severity == GL_DEBUG_SEVERITY_NOTIFICATION ) {
		if ( type == GL_DEBUG_TYPE_OTHER && strstr(message, "Buffer detailed info") ) {
			// We don't want to see this from NVIDIA driver:
			//   Buffer detailed info: Buffer object 2 (bound to GL_ARRAY_BUFFER_ARB, usage hint is GL_DYNAMIC_DRAW) will use SYSTEM HEAP memory as the source for buffer object operations.
			return;
		}
	}

	int msgHash = idStr::Hash( message );
	if( glDebugMessageIdLastSeenInFrame.find( msgHash ) == glDebugMessageIdLastSeenInFrame.end() ||
			tr.frameCount - glDebugMessageIdLastSeenInFrame[msgHash] > SUPPRESS_FOR_NUM_FRAMES ) {
		common->Printf( "GL: %s\n", message );
		glDebugMessageIdLastSeenInFrame[msgHash] = tr.frameCount;
	}
}

/*
==================
R_InitOpenGL

This function is responsible for initializing a valid OpenGL subsystem
for rendering.  This is done by calling the system specific GLimp_Init,
which gives us a working OGL subsystem, then setting all necessary openGL
state, including images, vertex programs, and display lists.

Changes to the vertex cache size or smp state require a vid_restart.

If glConfig.isInitialized is false, no rendering can take place, but
all renderSystem functions will still operate properly, notably the material
and model information functions.
==================
*/
void R_InitOpenGL( void ) {
	GLint			temp;
	glimpParms_t	parms;
	int				i;

	common->Printf( "----- Initializing OpenGL -----\n" );

	if ( glConfig.isInitialized ) {
		common->FatalError( "R_InitOpenGL called while active" );
	}

	// in case we had an error while doing a tiled rendering
	tr.viewportOffset[0] = 0;
	tr.viewportOffset[1] = 0;

	//
	// initialize OS specific portions of the renderSystem
	//
	for ( i = 0 ; i < 2 ; i++ ) {
		// set the parameters we are trying
		if ( r_customWidth.GetInteger() <= 0 || r_customHeight.GetInteger() <= 0 ) {
			bool ok = Sys_GetCurrentMonitorResolution( glConfig.vidWidth, glConfig.vidHeight );
			if (!ok) {
				glConfig.vidWidth = 800;
				glConfig.vidHeight = 600;
			}
			r_customWidth.SetInteger( glConfig.vidWidth );
			r_customHeight.SetInteger( glConfig.vidHeight );
		} else {
			glConfig.vidWidth = r_customWidth.GetInteger();
			glConfig.vidHeight = r_customHeight.GetInteger();
		}

		parms.width = glConfig.vidWidth;
		parms.height = glConfig.vidHeight;
		parms.fullScreen = r_fullscreen.GetBool();
		parms.displayHz = r_displayRefresh.GetInteger();
		parms.stereo = false;
		parms.multiSamples = 0;

		if ( GLimp_Init( parms ) ) {
			// it worked
			InitOpenGLTracing();
			break;
		}

		if ( i == 1 ) {
			common->FatalError( "Unable to initialize OpenGL" );
		} else {
			common->Printf( "Retrying OpenGL initialization in safe mode\n" );
		}

		// if we failed, set everything back to "safe mode" and try again
		r_fullscreen.SetInteger( 0 );
		r_displayRefresh.SetInteger( 0 );
		r_multiSamples.SetInteger( 0 );
		r_customWidth.SetInteger( 800 );
		r_customHeight.SetInteger( 600 );
	}

	// input and sound systems need to be tied to the new window
	Sys_InitInput();
	Sys_InitPadInput();
	soundSystem->InitHW();

	if ( glConfig.srgb = r_fboSRGB )
		qglEnable( GL_FRAMEBUFFER_SRGB );

	// get our config strings
	glConfig.vendor_string = (const char *)qglGetString(GL_VENDOR);
	glConfig.renderer_string = (const char *)qglGetString(GL_RENDERER);
	glConfig.version_string = (const char *)qglGetString(GL_VERSION);

	if ( strcmp( glConfig.vendor_string, "Intel" ) == 0 ) { 
		glConfig.vendor = glvIntel; 
	}

	if ( strcmp( glConfig.vendor_string, "ATI Technologies Inc." ) == 0 ) { 
		glConfig.vendor = glvAMD; 
	}

	if ( strncmp( glConfig.vendor_string, "NVIDIA", 6 ) == 0 || 
		 strncmp( glConfig.vendor_string, "Nvidia", 6 ) == 0 || 
		 strncmp( glConfig.vendor_string, "nvidia", 6 ) == 0 ) {
		 glConfig.vendor = glvNVIDIA;
	}

	// OpenGL driver constants
	qglGetIntegerv( GL_MAX_TEXTURE_SIZE, &temp );
	glConfig.maxTextureSize = temp;

	// stubbed or broken drivers may have reported 0...
	if ( glConfig.maxTextureSize <= 0 ) {
		glConfig.maxTextureSize = 256;
	}
	glConfig.isInitialized = true;

	common->Printf( "OpenGL vendor: %s\n", glConfig.vendor_string );
	common->Printf( "OpenGL renderer: %s\n", glConfig.renderer_string );
	common->Printf( "OpenGL version: %s %s\n", glConfig.version_string, GLAD_GL_ARB_compatibility ? "compatibility" : "core" );

	// recheck all the extensions
	GLimp_CheckRequiredFeatures();


	if( GLAD_GL_KHR_debug ) {
		qglDebugMessageCallback( R_OpenGLDebugMessageCallback, nullptr );
		if( r_glDebugOutput.GetBool() ) {
			qglEnable( GL_DEBUG_OUTPUT );
		} else {
			qglDisable( GL_DEBUG_OUTPUT );
		}
		if( r_glDebugOutput.GetInteger() == 2) {
			qglEnable( GL_DEBUG_OUTPUT_SYNCHRONOUS );
		} else {
			qglDisable( GL_DEBUG_OUTPUT_SYNCHRONOUS );
		}
	}

	cmdSystem->AddCommand( "reloadGLSLprograms", R_ReloadGLSLPrograms_f, CMD_FL_RENDERER, "reloads GLSL programs" );

	R_ReloadGLSLPrograms_f( idCmdArgs() );

	// allocate the vertex array range or vertex objects
	vertexCache.Init();

	// allocate the frame data, which may be more if smp is enabled
	R_InitFrameData();

	renderBackend->Init();

	// Reset our gamma
	R_SetColorMappings();

#ifdef _WIN32
	static bool glCheck = false;
	if ( !glCheck && win32.osversion.dwMajorVersion >= 6 ) {
		glCheck = true;
		if ( !idStr::Icmp( glConfig.vendor_string, "Microsoft" ) && idStr::FindText( glConfig.renderer_string, "OpenGL-D3D" ) != -1 ) {
			if ( cvarSystem->GetCVarBool( "r_fullscreen" ) ) {
				cmdSystem->BufferCommandText( CMD_EXEC_NOW, "vid_restart partial windowed\n" );
				Sys_GrabMouseCursor( false );
			}
			int ret = MessageBox( NULL, "Please install OpenGL drivers from your graphics hardware vendor to run " GAME_NAME ".\nYour OpenGL functionality is limited.",
										"Insufficient OpenGL capabilities", MB_OKCANCEL | MB_ICONWARNING | MB_TASKMODAL );
			if ( ret == IDCANCEL ) {
				cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "quit\n" );
				cmdSystem->ExecuteCommandBuffer();
			}
			if ( cvarSystem->GetCVarBool( "r_fullscreen" ) ) {
				cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "vid_restart\n" );
			}
		}
	}
#endif
}

/*
==================
GL_CheckErrors
==================
*/
void GL_CheckErrors( void ) {
	if ( r_ignoreGLErrors.GetBool() ) {
		return;
	}
	int		err;
	char	s[64];
	int		i;
	// check for up to 10 errors pending
	for ( i = 0 ; i < 10 ; i++ ) {
		err = qglGetError();
		if ( err == GL_NO_ERROR ) { 
			return; 
		}
		switch ( err ) {
		case GL_INVALID_ENUM:
			strcpy( s, "GL_INVALID_ENUM" );
			break;
		case GL_INVALID_VALUE:
			strcpy( s, "GL_INVALID_VALUE" );
			break;
		case GL_INVALID_OPERATION:
			strcpy( s, "GL_INVALID_OPERATION" );
			break;
		case GL_STACK_OVERFLOW:
			strcpy( s, "GL_STACK_OVERFLOW" );
			break;
		case GL_STACK_UNDERFLOW:
			strcpy( s, "GL_STACK_UNDERFLOW" );
			break;
		case GL_OUT_OF_MEMORY:
			strcpy( s, "GL_OUT_OF_MEMORY" );
			break;
		case GL_INVALID_FRAMEBUFFER_OPERATION:
			strcpy( s, "GL_INVALID_FRAMEBUFFER_OPERATION" );
			break;
		default:
			idStr::snPrintf( s, sizeof( s ), "%i", err );
			break;
		}
		common->Printf( "GL_CheckErrors: %s\n", s );
	}
}

/*
=====================
R_ReloadSurface_f

Reload the material displayed by r_showSurfaceInfo
=====================
*/
static void R_ReloadSurface_f( const idCmdArgs &args ) {
	// Skip if the current render is the lightgem render (default RENDERTOOLS_SKIP_ID)
	if ( tr.primaryView->IsLightGem() )	{
		return;
	}
	modelTrace_t mt;

	// start far enough away that we don't hit the player model
	const idVec3 start = tr.primaryView->renderView.vieworg + tr.primaryView->renderView.viewaxis[0] * 16;
	const idVec3 end = start + tr.primaryView->renderView.viewaxis[0] * 1000.0f;
	if ( !tr.primaryWorld->Trace( mt, start, end, 0.0f, false, true ) ) {
		return;
	}
	common->Printf( "Reloading %s\n", mt.material->GetName() );

	// reload the decl
	mt.material->base->Reload();

	// reload any images used by the decl
	mt.material->ReloadImages( false );
}

/*
=====================
R_OverrideSurfaceMaterial_f

Change the material on surface under cursor (as displayed by r_showSurfaceInfo)
=====================
*/
static void R_OverrideSurfaceMaterial_f( const idCmdArgs &args ) {
	// Skip if the current render is the lightgem render (default RENDERTOOLS_SKIP_ID)
	if ( tr.primaryView->IsLightGem() )	{
		return;
	}

	const char *materialName = args.Argv(1);
	if ( materialName[0] == 0 ) {
		common->Printf( "Write material name as parameter\n" );
		return;
	}
	const idMaterial *material = declManager->FindMaterial( materialName, false );
	if ( !material ) {
		common->Printf( "Could not find material with specified name\n" );
		return;
	}

	// start far enough away that we don't hit the player model
	const idVec3 start = tr.primaryView->renderView.vieworg + tr.primaryView->renderView.viewaxis[0] * 16;
	const idVec3 end = start + tr.primaryView->renderView.viewaxis[0] * 1000.0f;
	modelTrace_t mt;
	if ( !tr.primaryWorld->Trace( mt, start, end, 0.0f, false, true ) ) {
		return;
	}
	common->Printf( "Overriding material %s with %s at surface %s : %d\n", mt.material->GetName(), material->GetName(), mt.model->Name(), mt.surfIdx );

	const modelSurface_t *surf = mt.model->Surface( mt.surfIdx );
	// change the material
	// note: this is very dirty and should never be done in ordinary code!
	// but this is only a debug tool, and I won't regret if it suddenly stops working or breaks something else after usage =)
	const_cast<modelSurface_t*>(surf)->material = material;

	// refresh renderer like in "reloadModels" command
	R_ReCreateWorldReferences();
}

/*
=============
TestVideoClean
=============
*/
static void TestVideoClean() {
	if ( tr.testVideo ) {
		tr.testVideo->Close();
		tr.testVideo = NULL;
	}
	tr.testVideoFrame = NULL;
}

/*
=============
R_TestImage_f

Display the given image centered on the screen.
testimage <filename>
testimage cubemap <filename>
testimage index <number>
=============
*/
void R_TestImage_f( const idCmdArgs &args ) {
	TestVideoClean();
	tr.testImage = NULL;

	bool cubemap = false;
	const char *filename = nullptr;
	int imageNum = -1;

	if ( args.Argc() == 3 && !idStr::Icmp( args.Argv( 1 ), "cubemap" ) ) {
		cubemap = true;
		filename = args.Argv( 2 );
	}
	else if ( args.Argc() == 3 && !idStr::Icmp( args.Argv( 1 ), "index" ) && idStr::IsNumeric( args.Argv( 2 ) ) ) {
		imageNum = atoi( args.Argv( 2 ) );
		if ( !( imageNum >= 0 && imageNum < globalImages->images.Num() ) ) {
			common->Printf("Image index out of range: %d not in [0..%d)\n", imageNum, globalImages->images.Num() );
			return;
		}
		if ( globalImages->images[imageNum]->GetType() != IT_ASSET ) {
			common->Printf("Image %d is not asset\n", imageNum );
			return;
		}
	}
	else if ( args.Argc() == 2 ) {
		filename = args.Argv( 1 );
	}
	else {
		common->Printf(
			"Usage syntax (note: enclose image programs in doublequotes):\n"
			"  testImage <imagename>\n"
			"  testImage cubemap <imagename>\n"
			"  testImage index <imageindex>\n"
		);
		return;
	}

	if ( filename ) {
		tr.testImage = globalImages->ImageFromFile( filename, TF_DEFAULT, false, TR_REPEAT, TD_DEFAULT, cubemap ? CF_NATIVE : CF_2D );
	} else {
		tr.testImage = globalImages->images[imageNum]->AsAsset();
	}
	tr.testImageIsCubemap = cubemap;
}

/*
=============
R_TestVideo_f

Plays the cinematic file in a testImage
=============
*/
void R_TestVideo_f( const idCmdArgs &args ) {
	TestVideoClean();
	if ( args.Argc() < 2 ) {
		return;
	}
	//stgatilov #4847: support testing FFmpeg videos with audio stream
	bool withAudio = args.Argc() >= 3 && strcmp( args.Argv( 2 ), "withAudio" ) == 0;

	tr.testVideoFrame = globalImages->GetImage( "_scratch" )->AsScratch();
	tr.testVideo = idCinematic::Alloc( args.Argv( 1 ) );
	tr.testVideo->InitFromFile( args.Argv( 1 ), false, withAudio );
	tr.testVideoStartTime = tr.primaryRenderView.time * 0.001;

	cinData_t cin;
	cin = tr.testVideo->ImageForTime( 0 );
	if ( !cin.image ) {
		common->Warning( "Failed to get first frame from video file" );
		return TestVideoClean();
	}
	common->Printf( "%i x %i images\n", cin.imageWidth, cin.imageHeight );
	int	len = tr.testVideo->AnimationLength();
	common->Printf( "%5.1f seconds of video\n", len * 0.001 );

	if ( withAudio ) {
		//stgatilov #4847: check that audio stream is peekable
		float buff[4096] = { 0 };
		int cnt = 1024;
		bool ok = tr.testVideo->SoundForTimeInterval( 0, &cnt, buff );
		if ( !ok ) {
			common->Warning( "Failed to get first few sound samples from video file" );
			return TestVideoClean();
		}
		common->Printf( "Sound stream opened\n" );

		//create implicit sound shader with special name
		char soundName[256];
		idStr::snPrintf( soundName, sizeof( soundName ), "__testvideo:%p__", tr.testVideo );
		session->sw->PlayShaderDirectly( soundName );
	} else {
		// try to play the matching wav file
		idStr	wavString = args.Argv( ( args.Argc() == 2 ) ? 1 : 2 );
		wavString.StripFileExtension();
		wavString = wavString + ".wav";
		session->sw->PlayShaderDirectly( wavString.c_str() );
	}
}

static int R_QsortSurfaceAreas( const void *a, const void *b ) {
	const idMaterial	*ea, *eb;
	int	ac, bc;

	ea = *( idMaterial ** )a;

	if ( !ea->EverReferenced() ) {
		ac = 0;
	} else {
		ac = ea->GetSurfaceArea();
	}
	eb = *( idMaterial ** )b;

	if ( !eb->EverReferenced() ) {
		bc = 0;
	} else {
		bc = eb->GetSurfaceArea();
	}

	if ( ac < bc ) {
		return -1;
	}

	if ( ac > bc ) {
		return 1;
	}
	return idStr::Icmp( ea->GetName(), eb->GetName() );
}


/*
===================
R_ReportSurfaceAreas_f

Prints a list of the materials sorted by surface area
===================
*/
void R_ReportSurfaceAreas_f( const idCmdArgs &args ) {
	int	i, count;
	idMaterial	**list;

	count = declManager->GetNumDecls( DECL_MATERIAL );
	list = ( idMaterial ** )_alloca( count * sizeof( *list ) );

	for ( i = 0 ; i < count ; i++ ) {
		list[i] = ( idMaterial * )declManager->DeclByIndex( DECL_MATERIAL, i, false );
	}
	qsort( list, count, sizeof( list[0] ), R_QsortSurfaceAreas );

	// skip over ones with 0 area
	for ( i = 0 ; i < count ; i++ ) {
		if ( list[i]->GetSurfaceArea() > 0 ) {
			break;
		}
	}

	for ( /**/; i < count ; i++ ) {
		// report size in "editor blocks"
		int	blocks = list[i]->GetSurfaceArea() / 4096.0;
		common->Printf( "%7i %s\n", blocks, list[i]->GetName() );
	}
}

/*
==============================================================================

						THROUGHPUT BENCHMARKING

==============================================================================
*/

/*
================
R_RenderingFPS
================
*/
static float R_RenderingFPS( const renderView_t &renderView ) {
	qglFinish();

	int		start = Sys_Milliseconds();
	static const int SAMPLE_MSEC = 1000;
	int		end;
	int		count = 0;

	while ( 1 ) {
		// render
		renderSystem->BeginFrame( glConfig.vidWidth, glConfig.vidHeight );
		tr.primaryWorld->RenderScene( renderView );
		renderSystem->EndFrame( NULL, NULL );
		qglFinish();
		count++;
		end = Sys_Milliseconds();
		if ( end - start > SAMPLE_MSEC ) {
			break;
		}
	}
	float fps = count * 1000.0 / ( end - start );

	return fps;
}

/*
================
R_Benchmark_f
================
*/
void R_Benchmark_f( const idCmdArgs &args ) {
	float	fps, msec;
	renderView_t	view;

	if ( !tr.primaryView ) {
		common->Printf( "No primaryView for benchmarking\n" );
		return;
	}
	view = tr.primaryRenderView;

	for ( int size = 100 ; size >= 10 ; size -= 10 ) {
		r_screenFraction.SetInteger( size );
		fps = R_RenderingFPS( view );
		int	kpix = glConfig.vidWidth * glConfig.vidHeight * ( size * 0.01 ) * ( size * 0.01 ) * 0.001;
		msec = 1000.0 / fps;
		common->Printf( "kpix: %4i  msec:%5.1f fps:%5.1f\n", kpix, msec, fps );
	}

	// enable r_singleTriangle 1 while r_screenFraction is still at 10
	r_singleTriangle.SetBool( 1 );
	fps = R_RenderingFPS( view );
	msec = 1000.0 / fps;
	common->Printf( "single tri  msec:%5.1f fps:%5.1f\n", msec, fps );
	r_singleTriangle.SetBool( 0 );
	r_screenFraction.SetInteger( 100 );

	// enable r_skipRenderContext 1
	r_skipRenderContext.SetBool( true );
	fps = R_RenderingFPS( view );
	msec = 1000.0 / fps;
	common->Printf( "no context  msec:%5.1f fps:%5.1f\n", msec, fps );
	r_skipRenderContext.SetBool( false );
}


/*
==============================================================================

						SCREEN SHOTS

==============================================================================
*/

/*
====================
R_ReadTiledPixels

Allows the rendering of an image larger than the actual window by
tiling it into window-sized chunks and rendering each chunk separately

If ref isn't specified, the full session UpdateScreen will be done.
====================
*/
void R_ReadTiledPixels( int width, int height, byte *buffer, renderView_t *ref = NULL ) {
	// include extra space for OpenGL padding to word boundaries
#ifdef __ANDROID__ //karin: RGBA
	byte *temp = ( byte * )R_StaticAlloc( glConfig.vidWidth * glConfig.vidHeight * 4 );
#else
	byte *temp = ( byte * )R_StaticAlloc( glConfig.vidWidth * glConfig.vidHeight * 3 );
#endif

	const int oldWidth = glConfig.vidWidth;
	const int oldHeight = glConfig.vidHeight;

	tr.tiledViewport[0] = width;
	tr.tiledViewport[1] = height;

	for ( int xo = 0 ; xo < width ; xo += oldWidth ) {
		for ( int yo = 0 ; yo < height ; yo += oldHeight ) {
			tr.viewportOffset[0] = -xo;
			tr.viewportOffset[1] = -yo;
			int w = ( xo + oldWidth > width ) ? ( width - xo ) : oldWidth;
			int h = ( yo + oldHeight > height ) ? ( height - yo ) : oldHeight;

			if ( ref ) {
				tr.BeginFrame( oldWidth, oldHeight );
				tr.primaryWorld->RenderScene( *ref );
				copyRenderCommand_t &cmd = *( copyRenderCommand_t * )R_GetCommandBuffer( sizeof( cmd ) );
				cmd.commandId = RC_COPY_RENDER;
				cmd.buffer = temp;
				cmd.usePBO = false;
				cmd.image = NULL;
				cmd.x = 0;
				cmd.y = 0;
				cmd.imageWidth = oldWidth;
				cmd.imageHeight = oldHeight;
				tr.EndFrame( NULL, NULL );
				tr.BeginFrame( oldWidth, oldHeight );
				tr.EndFrame( NULL, NULL );
			} else {
				session->UpdateScreen( false );
#ifdef _OPENGLES3 //karin: OpenGLES3.0
				qglReadBuffer( GL_FRONT );
#endif
				qglPixelStorei( GL_PACK_ALIGNMENT, 1 );	// otherwise small rows get padded to 32 bits
#ifdef __ANDROID__ //karin: RGBA
				qglReadPixels( 0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, temp );
#else
				qglReadPixels( 0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, temp );
#endif
			}

			for ( int y = 0 ; y < h ; y++ ) {
#ifdef __ANDROID__ //karin: RGBA
				memcpy( buffer + ( ( yo + y )* width + xo ) * 4, temp + y * oldWidth * 4, w * 4 );
#else
                memcpy( buffer + ( ( yo + y )* width + xo ) * 3, temp + y * oldWidth * 3, w * 3 );
#endif
			}
		}
	}

	tr.viewportOffset[0] = 0;
	tr.viewportOffset[1] = 0;
	tr.tiledViewport[0] = 0;
	tr.tiledViewport[1] = 0;

	R_StaticFree( temp );

	glConfig.vidWidth = oldWidth;
	glConfig.vidHeight = oldHeight;
}

/*
====================
Screenshot_ChangeFilename
====================
*/
void Screenshot_ChangeFilename( idStr& filename, const char *extension ) {
	idStr mapname( cvarSystem->GetCVarString( "fs_currentfm" ) );
	char thetime[MAX_IMAGE_NAME / 2];

	if ( !mapname || mapname[0] == '\0' ) {
		mapname = "noFm";
	}

	time_t tt;
	time( &tt );
	struct tm * ltime = localtime( &tt );
	strftime( thetime, sizeof( thetime ), "%Y-%m-%d %H-%M-%S", ltime );
	
	// Obsttorte: Add players view location to screenshot filename (#5819)
	idVec3 origin;
	idMat3 axis;
	gameLocal.GetViewPos_Cmd(origin, axis);
	idStr playerViewOriginStr(origin.ToString());

	idStr fileOnly = mapname + " (" + thetime + ") (" + playerViewOriginStr + ")." + extension;

	filename = "screenshots/";
	filename.AppendPath( fileOnly );
}

/*
==================
TakeScreenshot

Move to tr_imagefiles.c...

Will automatically tile render large screen shots if necessary
Downsample is the number of steps to mipmap the image before saving it
If ref == NULL, session->updateScreen will be used
==================
*/
void idRenderSystemLocal::TakeScreenshot( int width, int height, const char *fileName, int blends, renderView_t *ref, bool envshot ) {
	
	takingScreenshot = true;

	int	pix = width * height;
#ifdef __ANDROID__
	byte *buffer = ( byte * )R_StaticAlloc( pix * 4 );
#else
    byte *buffer = ( byte * )R_StaticAlloc( pix * 3 );
#endif

	if ( blends <= 1 ) {
		R_ReadTiledPixels( width, height, buffer, ref );
#ifdef __ANDROID__
        byte *buffer3 = ( byte * )R_StaticAlloc( pix * 3 );
        for ( int i = 0 ; i < pix ; i++ ) {
            buffer3[i * 3] = buffer[i * 4];
            buffer3[i * 3 + 1] = buffer[i * 4 + 1];
            buffer3[i * 3 + 2] = buffer[i * 4 + 2];
        }
	    R_StaticFree( buffer );
        buffer = buffer3;
#endif
	} else {
        unsigned short *shortBuffer = ( unsigned short * )R_StaticAlloc( pix * 2 * 3 );
		memset( shortBuffer, 0, pix * 2 * 3 );

		// enable anti-aliasing jitter
		r_jitter.SetBool( true );

		for ( int i = 0 ; i < blends ; i++ ) {
			R_ReadTiledPixels( width, height, buffer, ref );

#ifdef __ANDROID__
			for ( int j = 0 ; j < pix ; j++ ) {
				shortBuffer[j * 3] += buffer[j * 4];
				shortBuffer[j * 3 + 1] += buffer[j * 4 + 1];
				shortBuffer[j * 3 + 2] += buffer[j * 4 + 2];
			}
#else
            for ( int j = 0 ; j < pix * 3 ; j++ ) {
                shortBuffer[j] += buffer[j];
            }
#endif
		}

		// divide back to bytes
#ifdef __ANDROID__
        byte *buffer3 = ( byte * )R_StaticAlloc( pix * 3 );
		for ( int i = 0 ; i < pix * 3 ; i++ ) {
			buffer3[i] = shortBuffer[i] / blends;
		}
	    R_StaticFree( buffer );
        buffer = buffer3;
#else
        for ( int i = 0 ; i < pix * 3 ; i++ ) {
            buffer[i] = shortBuffer[i] / blends;
        }
#endif
		R_StaticFree( shortBuffer );
		r_jitter.SetBool( false );
	}

	idStr changedPath = fileName;
	idStr extension;
	if ( envshot ) {
		extension = "tga";
	} else {
		// find the preferred image format
		extension = r_screenshot_format.GetString();
		// change extension and index of screenshot file
		Screenshot_ChangeFilename( changedPath, extension );
	}

	idImageWriter writer;
	writer.Source( buffer, width, height, 3 );
	writer.Dest( fileSystem->OpenFileWrite( changedPath.c_str(), "fs_savepath", "" ) );
	writer.Flip();
	writer.WriteExtension( extension.c_str() );
	common->Printf( "Wrote %s\n", changedPath.c_str() );

	R_StaticFree( buffer );

	takingScreenshot = false;
}

/*
==================
screenshot_viewpos
==================
*/
void R_ScreenShotWithViewpos_f( const idCmdArgs &args ) {
	// Daft Mugi #6331: Show viewpos on player HUD during screenshot
	// NOTE: The font color for viewpos is set to cyan for best legibility,
	//       so devs and mappers have an easier time reading the viewpos
	//       during beta testing and troubleshooting.
	int cyanColor = 2;
	int setColor = cv_show_viewpos.GetInteger();
	idStr ambientGamma;
	idStr setAmbientGamma = cvarSystem->GetCVarString("r_ambientGamma");
	idStr cmdString;

	switch (args.Argc()) {
	case 1:
		ambientGamma = setAmbientGamma;
		break;
	case 2:
		ambientGamma = args.Argv(1);
		break;
	default:
		common->Printf( "usage: screenshot_viewpos\n       screenshot_viewpos <gamma>\n" );
		return;
	}

	// NOTE: Use multiple "wait" commands to ensure that the viewpos
	// is displayed during the "screenshot" command.
	sprintf(
		cmdString,
		"tdm_show_viewpos %i; r_ambientGamma %s;"
		"wait; wait; wait; screenshot;"
		"tdm_show_viewpos %i; r_ambientGamma %s;",
		cyanColor, ambientGamma.c_str(),
		setColor, setAmbientGamma.c_str()
	);

	// hide console
	console->Close();

	cmdSystem->AppendCommandText(cmdString);
}

/*
==================
R_BlendedScreenShot

screenshot
screenshot [filename]
screenshot [width] [height]
screenshot [width] [height] [samples]
==================
*/
#define	MAX_BLENDS	256	// to keep the accumulation in shorts
void R_ScreenShot_f( const idCmdArgs &args ) {
	idStr checkname;

	int width = glConfig.vidWidth;
	int height = glConfig.vidHeight;
	int	blends = 0;

	switch ( args.Argc() ) {
	case 1:
		width = glConfig.vidWidth;
		height = glConfig.vidHeight;
		blends = 1;
		break;
	case 2:
		width = glConfig.vidWidth;
		height = glConfig.vidHeight;
		blends = 1;
		checkname = args.Argv( 1 );
		break;
	case 3:
		width = atoi( args.Argv( 1 ) );
		height = atoi( args.Argv( 2 ) );
		blends = 1;
		break;
	case 4:
		width = atoi( args.Argv( 1 ) );
		height = atoi( args.Argv( 2 ) );
		blends = atoi( args.Argv( 3 ) );
		if ( blends < 1 ) {
			blends = 1;
		}
		if ( blends > MAX_BLENDS ) {
			blends = MAX_BLENDS;
		}
		break;
	default:
		common->Printf( "usage: screenshot\n       screenshot <filename>\n       screenshot <width> <height>\n       screenshot <width> <height> <blends>\n" );
		return;
	}

	// put the console away
	console->Close();

	tr.TakeScreenshot( width, height, checkname, blends, NULL );


}

/*
===============
R_StencilShot
Save out a screenshot showing the stencil buffer expanded by 16x range
===============
*/
void R_StencilShot( void ) {
	byte		*buffer;

	const int	width = tr.GetScreenWidth();
	const int	height = tr.GetScreenHeight();
	const int	pix = width * height;
	const int	flen = pix * 3 + 18;

	buffer = ( byte * )Mem_Alloc( flen );
	memset( buffer, 0, 18 );

	byte *byteBuffer = ( byte * )Mem_Alloc( pix );

	qglPixelStorei( GL_PACK_ALIGNMENT, 1 );	// otherwise small rows get padded to 32 bits
	qglReadPixels( 0, 0, width, height, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, byteBuffer );

	for ( int i = 0 ; i < pix ; i++ ) {
		buffer[18 + i * 3] =
		    buffer[18 + i * 3 + 1] =
		        buffer[18 + i * 3 + 2] = byteBuffer[i];
	}

	// fill in the header (this is vertically flipped, which qglReadPixels emits)
	buffer[ 2] = 2;		// uncompressed type
	buffer[12] = width & 255;
	buffer[13] = width >> 8;
	buffer[14] = height & 255;
	buffer[15] = height >> 8;
	buffer[16] = 24;	// pixel size

	fileSystem->WriteFile( "screenshots/stencilShot.tga", buffer, flen, "fs_savepath", "" );

	Mem_Free( buffer );
	Mem_Free( byteBuffer );
}

// nbohr1more #4041: add envshotGL for cubicLight
/*
==================
R_EnvShotGL_f

envshotGL <basename>

(OpenGL orientation) Saves out env/<basename>_ft.tga, etc
==================
*/
void R_EnvShotGL_f( const idCmdArgs &args ) {
	idStr		fullname;
	const char	*baseName;
	idMat3		axis[6];
	renderView_t	ref;
	viewDef_t	primary;
	int			blends, size;

	if ( !tr.primaryView ) {
		common->Printf( "No primary view.\n" );
		return;
	} else if ( args.Argc() != 2 && args.Argc() != 3 && args.Argc() != 4 ) {
		common->Printf( "USAGE: envshotGL <basename> [size] [blends]\n" );
		return;
	}
	primary = *tr.primaryView;
	baseName = args.Argv( 1 );
	blends = 1;

	if ( args.Argc() == 4 ) {
		size = atoi( args.Argv( 2 ) );
		blends = atoi( args.Argv( 3 ) );
	} else if ( args.Argc() == 3 ) {
		size = atoi( args.Argv( 2 ) );
		blends = 1;
	} else {
		size = 256;
		blends = 1;
	}
	memset( &axis, 0, sizeof( axis ) );
	axis[0][0][0] = 1;
	axis[0][1][2] = 1;
	axis[0][2][1] = 1;

	axis[1][0][0] = -1;
	axis[1][1][2] = -1;
	axis[1][2][1] = 1;

	axis[2][0][1] = 1;
	axis[2][1][0] = -1;
	axis[2][2][2] = -1;

	axis[3][0][1] = -1;
	axis[3][1][0] = -1;
	axis[3][2][2] = 1;

	axis[4][0][2] = 1;
	axis[4][1][0] = -1;
	axis[4][2][1] = 1;

	axis[5][0][2] = -1;
	axis[5][1][0] = 1;
	axis[5][2][1] = 1;

	for ( int i = 0 ; i < 6 ; i++ ) {
		ref = primary.renderView;
		ref.x = ref.y = 0;
		ref.fov_x = ref.fov_y = 90;
		ref.width = SCREEN_WIDTH;// glConfig.vidWidth;
		ref.height = SCREEN_HEIGHT; //glConfig.vidHeight;
		ref.viewaxis = axis[i];
		sprintf( fullname, "env/%s%s", baseName, cubemapFaceNamesNative[i] );
		tr.TakeScreenshot( size, size, fullname, blends, &ref, true );
	}
	common->Printf( "Wrote %s, etc\n", fullname.c_str() );
}

//============================================================================

/*
==================
R_EnvShot_f

envshot <basename>

Saves out env/<basename>_ft.tga, etc
==================
*/
void R_EnvShot_f( const idCmdArgs &args ) {
	idStr fullname;
	const char *baseName;
	idMat3 axis[6];
	renderView_t ref;
	viewDef_t primary;
	int	blends, size;
	int playerView = 0;

	if ( !tr.primaryView ) {
		common->Printf( "No primary view.\n" );
		return;
	} else if ( args.Argc() != 2 && args.Argc() != 3 && args.Argc() != 4 ) {
		common->Printf( "USAGE: envshot <basename> [size] [blends|playerView|playerBackView]\n" );
		return;
	}
	primary = *tr.primaryView;
	baseName = args.Argv( 1 );

	blends = 1;
	if ( args.Argc() == 4 ) {
		size = atoi( args.Argv( 2 ) );
		if ( !idStr::Icmp( args.Argv( 3 ), "playerView" ) )
			playerView = 1;
		else if ( !idStr::Icmp( args.Argv( 3 ), "playerBackView" ) )
			playerView = -1;
		else
			blends = atoi( args.Argv( 3 ) );
	} else if ( args.Argc() == 3 ) {
		size = atoi( args.Argv( 2 ) );
		blends = 1;
	} else {
		size = 256;
		blends = 1;
	}
	memset( &axis, 0, sizeof( axis ) );

	// SteveL #4041: these axes were wrong, causing some of the images to be flipped and rotated.
	// forward = east (positive x-axis in DR)
	axis[0][0][0] = 1;
	axis[0][1][1] = 1;
	axis[0][2][2] = 1;
	// back = west
	axis[1][0][0] = -1;
	axis[1][1][1] = -1;
	axis[1][2][2] = 1;
	// left = north
	axis[2][0][1] = 1;
	axis[2][1][0] = -1;
	axis[2][2][2] = 1;
	// right = south
	axis[3][0][1] = -1;
	axis[3][1][0] = 1;
	axis[3][2][2] = 1;
	// up, while facing forward
	axis[4][0][2] = 1;
	axis[4][1][1] = 1;
	axis[4][2][0] = -1;
	// down, while facing forward
	axis[5][0][2] = -1;
	axis[5][1][1] = 1;
	axis[5][2][0] = 1;

	for ( int i = 0; i < 6; i++ ) {
		ref = primary.renderView;
		ref.x = ref.y = 0;
		ref.fov_x = ref.fov_y = 90;
		ref.width = SCREEN_WIDTH;
		ref.height = SCREEN_HEIGHT;
		ref.viewaxis = axis[i];
		if ( playerView ) {
			if ( playerView < 0 ) {
				ref.viewaxis *= axis[1];
			}
			ref.viewaxis = ref.viewaxis * gameLocal.GetLocalPlayer()->renderView->viewaxis;
		}
		sprintf( fullname, "env/%s%s", baseName, cubemapFaceNamesCamera[i] );
		tr.TakeScreenshot( size, size, fullname, blends, &ref, true );
	}
	common->Printf( "Wrote %s, etc. Reloading the texture...\n", fullname.c_str() );
	sprintf( fullname, "env/%s", baseName );
	if ( idImage* image = globalImages->GetImage(fullname) ) {
		image->AsAsset()->Reload( false, false );
	}
}

//============================================================================


/*
==================
R_MakeAmbientMap_f
==================
*/
void R_MakeAmbientMap_f( const idCmdArgs &args ) {
	if ( args.Argc() < 2 ) {
		common->Printf( "USAGE: makeAmbientMap <basename> (multiplier)\n" );
		return;
	}
	const char *baseName = args.Argv(1);

	float multiplier = 1.0f;
	if ( args.Argc() >= 3 ) {
		sscanf( args.Argv(2), "%f", &multiplier);
	}

	byte *picDiffuse[6] = {nullptr};
	int size = 0;
	idStr paddedBasename = idStr("env/") + baseName;
	if ( !R_LoadCubeImages( paddedBasename.c_str(), CF_CAMERA, picDiffuse, &size, nullptr ) ) {
		common->Printf( "Failed to load cubemap %s\n", baseName );
		return;
	}

	// clone cubemap, since R_BakeAmbient destroys input
	byte *picSpecular[6] = {nullptr};
	for ( int f = 0; f < 6; f++ ) {
		picSpecular[f] = (byte *) Mem_Alloc( size * size * 4 );
		memcpy( picSpecular[f], picDiffuse[f], size * size * 4 );
	}
	int sizeDiffuse = size;
	int sizeSpecular = size;

	R_BakeAmbient( picDiffuse, &sizeDiffuse, multiplier, false, nullptr );
	R_BakeAmbient( picSpecular, &sizeSpecular, multiplier, true, nullptr );

	static const char *typeSuffix[2] = {"_diff", "_spec"};
	for ( int specular = 0; specular < 2; specular++ ) {
		for ( int f = 0; f < 6; f++ ) {
			char fullname[1024];
			idStr::snPrintf( fullname, sizeof(fullname), "%s%s%s", paddedBasename.c_str(), typeSuffix[specular], cubemapFaceNamesNative[f] );
			int sz = (specular ? sizeSpecular : sizeDiffuse);
			R_WriteTGA( fullname, (specular ? picSpecular : picDiffuse)[f], sz, sz );
		}
	}
	for ( int f = 0; f < 6; f++ ) {
		Mem_Free( picDiffuse[f] );
		Mem_Free( picSpecular[f] );
	}
}

//============================================================================

/*
===============
R_SetColorMappings
===============
*/
void R_SetColorMappings( void ) {
	//stgatilov: brightness and gamma adjustments are done in final shader pass
	return;
#if 0
	int		j;
	float	g, b;
	int		inf;

	b = r_brightness.GetFloat();
	g = r_gamma.GetFloat();

	for ( int i = 0; i < 256; i++ ) {
		j = i * b;

		if ( j > 255 ) {
			j = 255;
		}

		if ( g == 1 ) {
			inf = ( j << 8 ) | j;
		} else {
			inf = 0xffff * pow( j / 255.0f, 1.0f / g ) + 0.5f;
		}

		if ( inf < 0 ) {
			inf = 0;
		}

		if ( inf > 0xffff ) {
			inf = 0xffff;
		}
		tr.gammaTable[i] = inf;
	}
	GLimp_SetGamma( tr.gammaTable, tr.gammaTable, tr.gammaTable );
#endif
}


/*
================
GfxInfo_f
================
*/
static void GfxInfo_f( const idCmdArgs &args ) {
	const char *fsstrings[] = {
		"windowed",
		"fullscreen",
		"borderless"
	};

	common->Printf( "\nGL_VENDOR: %s\n", glConfig.vendor_string );
	common->Printf( "GL_RENDERER: %s\n", glConfig.renderer_string );
	common->Printf( "GL_VERSION: %s\n", glConfig.version_string );

	//common->Printf( "GL_EXTENSIONS: %s\n", glConfig.extensions_string );
	common->Printf( "GL_EXTENSIONS: " );
	GLint n = 0;
	qglGetIntegerv( GL_NUM_EXTENSIONS, &n );
	for ( GLint i = 0; i < n; i++ ) {
		const char* extension =
			(const char*)qglGetStringi( GL_EXTENSIONS, i );
		common->Printf( "%s ", extension );
	}
	common->Printf( "\n" );

	if ( glConfig.wgl_extensions_string ) {
		common->Printf( "WGL_EXTENSIONS: %s\n", glConfig.wgl_extensions_string );
	}
	common->Printf( "GL_MAX_TEXTURE_SIZE: %d\n", glConfig.maxTextureSize );
	common->Printf( "GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS: %d\n", glConfig.maxTextures );
	//common->Printf( "\nPIXELFORMAT: color(%d-bits) Z(%d-bit) stencil(%d-bits)\n", glConfig.colorBits, glConfig.depthBits, glConfig.stencilBits );
	common->Printf( "MODE: %d x %d %s hz:", glConfig.vidWidth, glConfig.vidHeight, fsstrings[r_fullscreen.GetInteger()] );

	if ( glConfig.displayFrequency ) {
		common->Printf( "%d\n", glConfig.displayFrequency );
	} else {
		common->Printf( "N/A\n" );
	}
	common->Printf( "CPU: %s\n", Sys_GetProcessorString() );

	//=============================

	if ( r_finish.GetBool() ) {
		common->Printf( "Forcing glFinish\n" );
	} else {
		common->Printf( "glFinish not forced\n" );
	}

#ifdef _WIN32
	if ( r_swapInterval && qwglSwapIntervalEXT ) {
		common->Printf( "swapInterval forced (%i)\n", r_swapInterval.GetInteger() );
	} else {
		common->Printf( "swapInterval not forced\n" );
	}
#endif
}

void R_PurgeImages_f( const idCmdArgs& args ) {
	globalImages->PurgeAllImages();
}

void R_TuneDown_f( const idCmdArgs& args ) {
	bool undo = false;
	for ( int i = 1; i < args.Argc(); i++ ) {
		if ( idStr::Icmp( args.Argv( i ), "undo" ) == 0 ) {
			undo = true;
			continue;
		}
	}
	r_skipInteractions.SetInteger( undo ? 0 : 2 );
	r_skipPostProcess.SetInteger( !undo );
	r_shadows.SetInteger( undo );
	r_tonemap.SetInteger( undo );
	r_skipSubviews.SetInteger( !undo );
	r_skipParticles.SetInteger( !undo );
	r_ambientMinLevel.SetFloat( undo ? 0 : 0.5 );
	common->Printf( "%s shadows and most lights, subviews, particles, postprocessing\n", undo ? "Enabled" : "Disabled" );
}

/*
=================
R_VidRestart_f
=================
*/
void R_VidRestart_f( const idCmdArgs &args ) {

	// if OpenGL isn't started, do nothing
	if ( !glConfig.isInitialized ) {
		return;
	}
	bool full = true;
	bool forceWindow = false;

	for ( int i = 1 ; i < args.Argc() ; i++ ) {
		if ( idStr::Icmp( args.Argv( i ), "partial" ) == 0 ) {
			full = false;
			continue;
		}
		if ( idStr::Icmp( args.Argv( i ), "windowed" ) == 0 ) {
			forceWindow = true;
			continue;
		}
	}

	// this could take a while, so give them the cursor back ASAP
	Sys_GrabMouseCursor( false );

	// dump ambient caches
	renderModelManager->FreeModelVertexCaches();

	// free any current world interaction surfaces and vertex caches
	R_FreeDerivedData();

	// make sure the defered frees are actually freed
	R_ToggleSmpFrame();
	R_ToggleSmpFrame();

	// free the vertex caches so they will be regenerated again
	vertexCache.PurgeAll();

	// sound and input are tied to the window we are about to destroy
	if ( full ) {
		// free all of our texture numbers
		soundSystem->ShutdownHW();
		Sys_ShutdownInput();
		frameBuffers->PurgeAll();
		globalImages->PurgeAllImages();
		// free the context and close the window
		session->TerminateFrontendThread();
		vertexCache.Shutdown();
		renderBackend->Shutdown();
		GLimp_Shutdown();
		glConfig.isInitialized = false;

		// create the new context and vertex cache
		int latch = cvarSystem->GetCVarInteger( "r_fullscreen" );
		if ( forceWindow ) {
			cvarSystem->SetCVarInteger( "r_fullscreen", 0 );
		}
		R_InitOpenGL();
		cvarSystem->SetCVarInteger( "r_fullscreen", latch );

		// regenerate all images
		globalImages->ReloadAllImages();
		session->StartFrontendThread();
	} else {
		glimpParms_t	parms;
		parms.width = glConfig.vidWidth = r_customWidth.GetInteger();
		parms.height = glConfig.vidHeight = r_customHeight.GetInteger();
		parms.fullScreen = ( forceWindow ) ? false : r_fullscreen.GetBool();
		parms.displayHz = r_displayRefresh.GetInteger();
		parms.multiSamples = 0;
		parms.stereo = false;
		GLimp_SetScreenParms( parms );
	}

	// make sure the regeneration doesn't use anything no longer valid
	tr.viewCount++;
	tr.viewDef = NULL;

	// regenerate all necessary interactions
	R_RegenerateWorld_f( idCmdArgs() );

	// check for problems
	// use the builtin function instead revelator.
	GL_CheckErrors();

	// start sound playing again
	soundSystem->SetMute( false );

	if ( game != NULL ) {
		game->OnVidRestart();
	}
}


/*
=================
R_InitMaterials
=================
*/
void R_InitMaterials( void ) {
	tr.defaultMaterial = declManager->FindMaterial( "_default", false );
	if ( !tr.defaultMaterial ) {
		common->FatalError( "_default material not found" );
	}
	declManager->FindMaterial( "_default", false );

	tr.defaultShaderPoint = declManager->FindMaterial( "lights/defaultPointLight" );
	tr.defaultShaderProj  = declManager->FindMaterial( "lights/defaultProjectedLight" );
}


/*
=================
R_SizeUp_f

Keybinding command
=================
*/
static void R_SizeUp_f( const idCmdArgs &args ) {
	if ( r_screenFraction.GetInteger() + 10 > 100 ) {
		r_screenFraction.SetInteger( 100 );
	} else {
		r_screenFraction.SetInteger( r_screenFraction.GetInteger() + 10 );
	}
}


/*
=================
R_SizeDown_f

Keybinding command
=================
*/
static void R_SizeDown_f( const idCmdArgs &args ) {
	if ( r_screenFraction.GetInteger() - 10 < 10 ) {
		r_screenFraction.SetInteger( 10 );
	} else {
		r_screenFraction.SetInteger( r_screenFraction.GetInteger() - 10 );
	}
}

/*
===============
TouchGui_f

  this is called from the main thread
===============
*/
void R_TouchGui_f( const idCmdArgs &args ) {
	const char	*gui = args.Argv( 1 );

	if ( !gui[0] ) {
		common->Printf( "USAGE: touchGui <guiName>\n" );
		return;
	}
	common->Printf( "touchGui %s\n", gui );
	session->UpdateScreen();
	uiManager->Touch( gui );
}

/*
=================
R_InitCvars
=================
*/
void R_InitCvars( void ) {
	// update latched cvars here

}

/*
=================
R_InitCommands
=================
*/
void R_InitCommands( void ) {
	cmdSystem->AddCommand( "sizeUp", R_SizeUp_f, CMD_FL_RENDERER, "makes the rendered view larger" );
	cmdSystem->AddCommand( "sizeDown", R_SizeDown_f, CMD_FL_RENDERER, "makes the rendered view smaller" );
	cmdSystem->AddCommand( "reloadGuis", R_ReloadGuis_f, CMD_FL_RENDERER, "reloads guis" );
	cmdSystem->AddCommand( "listGuis", R_ListGuis_f, CMD_FL_RENDERER, "lists guis" );
	cmdSystem->AddCommand( "touchGui", R_TouchGui_f, CMD_FL_RENDERER, "touches a gui" );
	cmdSystem->AddCommand( "screenshot", R_ScreenShot_f, CMD_FL_RENDERER, "takes a screenshot" );
	cmdSystem->AddCommand( "screenshot_viewpos", R_ScreenShotWithViewpos_f, CMD_FL_RENDERER, "takes a screenshot with viewpos on player HUD" );
	cmdSystem->AddCommand( "envshot", R_EnvShot_f, CMD_FL_RENDERER, "takes an environment shot" );
	cmdSystem->AddCommand( "envshotGL", R_EnvShotGL_f, CMD_FL_RENDERER, "takes an environment shot in opengl orientation" ); // nbohr1more #4041: add envshotGL for cubicLight
	cmdSystem->AddCommand( "makeAmbientMap", R_MakeAmbientMap_f, CMD_FL_RENDERER | CMD_FL_CHEAT, "makes an ambient map" );
	cmdSystem->AddCommand( "benchmark", R_Benchmark_f, CMD_FL_RENDERER, "benchmark" );
	cmdSystem->AddCommand( "gfxInfo", GfxInfo_f, CMD_FL_RENDERER, "show graphics info" );
	cmdSystem->AddCommand( "modulateLights", R_ModulateLights_f, CMD_FL_RENDERER | CMD_FL_CHEAT, "modifies shader parms on all lights" );
	cmdSystem->AddCommand( "testImage", R_TestImage_f, CMD_FL_RENDERER | CMD_FL_CHEAT, "displays the given image centered on screen", idCmdSystem::ArgCompletion_ImageName );
	cmdSystem->AddCommand( "testVideo", R_TestVideo_f, CMD_FL_RENDERER | CMD_FL_CHEAT, "displays the given cinematic", idCmdSystem::ArgCompletion_VideoName );
	cmdSystem->AddCommand( "reportSurfaceAreas", R_ReportSurfaceAreas_f, CMD_FL_RENDERER, "lists all used materials sorted by surface area" );
	cmdSystem->AddCommand( "regenerateWorld", R_RegenerateWorld_f, CMD_FL_RENDERER, "regenerates all interactions" );
	cmdSystem->AddCommand( "showInteractionMemory", R_ShowInteractionMemory_f, CMD_FL_RENDERER, "shows memory used by interactions" );
	cmdSystem->AddCommand( "showTriSurfMemory", R_ShowTriSurfMemory_f, CMD_FL_RENDERER, "shows memory used by triangle surfaces" );
	cmdSystem->AddCommand( "vid_restart", R_VidRestart_f, CMD_FL_RENDERER, "restarts renderSystem" );
	cmdSystem->AddCommand( "listRenderEntityDefs", R_ListRenderEntityDefs_f, CMD_FL_RENDERER, "lists the entity defs" );
	cmdSystem->AddCommand( "listRenderLightDefs", R_ListRenderLightDefs_f, CMD_FL_RENDERER, "lists the light defs" );
	cmdSystem->AddCommand( "reloadSurface", R_ReloadSurface_f, CMD_FL_RENDERER, "reloads the decl and images for selected surface" );
	cmdSystem->AddCommand( "overrideSurfaceMaterial", R_OverrideSurfaceMaterial_f, CMD_FL_RENDERER, "changes the material of the surface currently under cursor", idCmdSystem::ArgCompletion_Decl<DECL_MATERIAL> );
	cmdSystem->AddCommand( "purgeImages", R_PurgeImages_f, CMD_FL_RENDERER, "deletes all currently loaded images" );
	cmdSystem->AddCommand( "tuneDown", R_TuneDown_f, CMD_FL_RENDERER, "removes all beauty" );
}

/*
===============
idRenderSystemLocal::Clear
===============
*/
void idRenderSystemLocal::Clear( void ) {
	frameCount = 0;
	viewCount = 0;
	staticAllocCount = 0;
	frameShaderTime = 0.0f;
	viewportOffset[0] = 0;
	viewportOffset[1] = 0;
	tiledViewport[0] = 0;
	tiledViewport[1] = 0;
	ambientLightVector.Zero();
	sortOffset = 0;
	worlds.Clear();
	primaryWorld = NULL;
	memset( &primaryRenderView, 0, sizeof( primaryRenderView ) );
	primaryView = NULL;
	defaultMaterial = NULL;
	testImage = NULL;
	viewDef = NULL;
	memset( &pc, 0, sizeof( pc ) );
	memset( &identitySpace, 0, sizeof( identitySpace ) );
	logFile = NULL;
	memset( renderCrops, 0, sizeof( renderCrops ) );
	currentRenderCrop = 0;
	guiRecursionLevel = 0;
	guiModel = NULL;
	demoGuiModel = NULL;
	memset( gammaTable, 0, sizeof( gammaTable ) );
	takingScreenshot = false;
	frontEndJobList = NULL;
}

/*
===============
idRenderSystemLocal::Init
===============
*/
void idRenderSystemLocal::Init( void ) {
	r_swapInterval.SetModified();

	// clear all our internal state
	viewCount = 1;		// so cleared structures never match viewCount
						// we used to memset tr, but now that it is a class, we can't, so
						// there may be other state we need to reset

	ambientLightVector[0] = 0.5f;
	ambientLightVector[1] = 0.5f - 0.385f;
	ambientLightVector[2] = 0.8925f;
	ambientLightVector[3] = 1.0f;

	memset( &backEnd, 0, sizeof( backEnd ) );

	R_InitCvars();

	R_InitCommands();

	guiModel = new idGuiModel;
	guiModel->Clear();

	demoGuiModel = new idGuiModel;
	demoGuiModel->Clear();

	R_InitTriSurfData();

	globalImages->Init();
	frameBuffers->Init();
	programManager->Init();

	idCinematic::InitCinematic( );

	// build brightness translation tables
	R_SetColorMappings();

	R_InitMaterials();

	renderModelManager->Init();

	// set the identity space
	identitySpace.modelMatrix[0 * 4 + 0] = 1.0f;
	identitySpace.modelMatrix[1 * 4 + 1] = 1.0f;
	identitySpace.modelMatrix[2 * 4 + 2] = 1.0f;

	frontEndJobList = parallelJobManager->AllocJobList( JOBLIST_RENDERER_FRONTEND, JOBLIST_PRIORITY_MEDIUM, 8192, 0, NULL );
}

/*
===============
idRenderSystemLocal::Shutdown
===============
*/
void idRenderSystemLocal::Shutdown( void ) {
	common->Printf( "idRenderSystem::Shutdown()\n" );
	R_DoneFreeType( );

	ambientOcclusion->Shutdown();
	bloom->Shutdown();
	volumetric->Shutdown();

	if ( glConfig.isInitialized ) {
		globalImages->PurgeAllImages();
	}
	renderModelManager->Shutdown();

	idCinematic::ShutdownCinematic( );

	frameBuffers->Shutdown();
	globalImages->Shutdown();
	programManager->Shutdown();

	// close the r_logFile
	if ( logFile ) {
		fprintf( logFile, "*** CLOSING LOG ***\n" );
		fclose( logFile );
		logFile = 0;
	}

	// free frame memory
	R_ShutdownFrameData();

	// free the vertex cache, which should have nothing allocated now
	vertexCache.Shutdown();

	R_ShutdownTriSurfData();

	RB_ShutdownDebugTools();

	delete guiModel;
	delete demoGuiModel;

	parallelJobManager->FreeJobList( frontEndJobList );

	Clear();

	ShutdownOpenGL();
}

/*
========================
idRenderSystemLocal::BeginLevelLoad
========================
*/
void idRenderSystemLocal::BeginLevelLoad( void ) {
	renderModelManager->BeginLevelLoad();
	globalImages->BeginLevelLoad();
}

/*
========================
idRenderSystemLocal::EndLevelLoad
========================
*/
void idRenderSystemLocal::EndLevelLoad( void ) {
	renderModelManager->EndLevelLoad();
	globalImages->EndLevelLoad();
	programManager->ReloadAllPrograms();
	if ( r_forceLoadImages.GetBool() ) {
		RB_ShowImages();
	}
	common->Printf( "----------------------------------------\n" );
}

/*
========================
idRenderSystemLocal::InitOpenGL
========================
*/
void idRenderSystemLocal::InitOpenGL( void ) {
	// if OpenGL isn't started, start it now
	if ( !glConfig.isInitialized ) {
		R_InitOpenGL();
		globalImages->ReloadAllImages();
		GL_CheckErrors(); // use the existing internal function instead: revelator.
	}
}

/*
========================
idRenderSystemLocal::ShutdownOpenGL
========================
*/
void idRenderSystemLocal::ShutdownOpenGL( void ) {
	// free the context and close the window
	R_ShutdownFrameData();
	GLimp_Shutdown();
	glConfig.isInitialized = false;
}

/*
========================
idRenderSystemLocal::IsOpenGLRunning
========================
*/
bool idRenderSystemLocal::IsOpenGLRunning( void ) const {
	if ( !glConfig.isInitialized ) {
		return false;
	}
	return true;
}

/*
========================
idRenderSystemLocal::IsFullScreen
========================
*/
bool idRenderSystemLocal::IsFullScreen( void ) const {
	return glConfig.isFullscreen;
}

/*
========================
idRenderSystemLocal::GetScreenWidth
========================
*/
int idRenderSystemLocal::GetScreenWidth( void ) const {
	return glConfig.vidWidth;
}

/*
========================
idRenderSystemLocal::GetScreenHeight
========================
*/
int idRenderSystemLocal::GetScreenHeight( void ) const {
	return glConfig.vidHeight;
}
