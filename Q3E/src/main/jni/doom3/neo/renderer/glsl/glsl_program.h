#if !defined(GLSL_PROGRAM_PROC)
#error "you must define GLSL_PROGRAM_PROC before including this file"
#endif

// base
GLSL_PROGRAM_PROC shaderProgram_t interactionShader; // PHONG lighting model interaction shader
GLSL_PROGRAM_PROC shaderProgram_t shadowShader; // stencil shadow shader
GLSL_PROGRAM_PROC shaderProgram_t defaultShader; // default texture and color shader
GLSL_PROGRAM_PROC shaderProgram_t depthFillShader; // z-fill shader
GLSL_PROGRAM_PROC shaderProgram_t depthFillClipShader; //k: z-fill shader(clipped)
GLSL_PROGRAM_PROC shaderProgram_t cubemapShader; //k: skybox shader
GLSL_PROGRAM_PROC shaderProgram_t environmentShader; //k: reflection shader(environment)
GLSL_PROGRAM_PROC shaderProgram_t bumpyEnvironmentShader; //k: reflection shader(Bumpy environment)
GLSL_PROGRAM_PROC shaderProgram_t fogShader; //k: fog shader
GLSL_PROGRAM_PROC shaderProgram_t blendLightShader; //k: blend light shader
GLSL_PROGRAM_PROC shaderProgram_t interactionPBRShader; //k: PBR lighting model interaction shader
GLSL_PROGRAM_PROC shaderProgram_t interactionBlinnPhongShader; //k: BLINN-PHONG lighting model interaction shader
GLSL_PROGRAM_PROC shaderProgram_t ambientLightingShader; //k: Ambient lighting model interaction shader
GLSL_PROGRAM_PROC shaderProgram_t diffuseCubemapShader; //k: diffuse cubemap shader
GLSL_PROGRAM_PROC shaderProgram_t texgenShader; //k: texgen shader

// new stage
GLSL_PROGRAM_PROC shaderProgram_t heatHazeShader; //k: heatHaze shader
GLSL_PROGRAM_PROC shaderProgram_t heatHazeWithMaskShader; //k: heatHaze with mask shader
GLSL_PROGRAM_PROC shaderProgram_t heatHazeWithMaskAndVertexShader; //k: heatHaze with mask and vertex shader
GLSL_PROGRAM_PROC shaderProgram_t colorProcessShader; //k: color process shader
GLSL_PROGRAM_PROC shaderProgram_t megaTextureShader; //k: megatexture shader

#ifdef _HUMANHEAD //karin: newstage
GLSL_PROGRAM_PROC shaderProgram_t screeneffectShader; //k: screen effect shader spiritview
GLSL_PROGRAM_PROC shaderProgram_t radialblurShader; //k: screen effect shader deathview
GLSL_PROGRAM_PROC shaderProgram_t liquidShader; //k: liquid shader
//GLSL_PROGRAM_PROC shaderProgram_t interactionLiquidShader; //k: interaction liquid shader: TODO: using liquidShader as alias
GLSL_PROGRAM_PROC shaderProgram_t screenprocessShader; //k: screen process shader
#endif

// shadow mapping
#ifdef _SHADOW_MAPPING
GLSL_PROGRAM_PROC shaderProgram_t depthShader; //k: depth shader
GLSL_PROGRAM_PROC shaderProgram_t depthShader_color; //k: depth shader(if not support depth texture(2D / cubemap), only ES2.0)

GLSL_PROGRAM_PROC shaderProgram_t interactionShadowMappingShader_pointLight; //k: PHONG interaction with shadow mapping(point light)
GLSL_PROGRAM_PROC shaderProgram_t interactionShadowMappingPBRShader_pointLight; //k: PBR interaction with shadow mapping(point light)
GLSL_PROGRAM_PROC shaderProgram_t interactionShadowMappingBlinnPhongShader_pointLight; //k: BLINN-PHONG interaction with shadow mapping(point light)
GLSL_PROGRAM_PROC shaderProgram_t ambientLightingShadowMappingShader_pointLight; //k: Ambient interaction with shadow mapping(point light)

GLSL_PROGRAM_PROC shaderProgram_t interactionShadowMappingShader_parallelLight; //k: PHONG interaction with shadow mapping(parallel light)
GLSL_PROGRAM_PROC shaderProgram_t interactionShadowMappingPBRShader_parallelLight; //k: PBR interaction with shadow mapping(parallel light)
GLSL_PROGRAM_PROC shaderProgram_t interactionShadowMappingBlinnPhongShader_parallelLight; //k: BLINN-PHONG interaction with shadow mapping(parallel light)
GLSL_PROGRAM_PROC shaderProgram_t ambientLightingShadowMappingShader_parallelLight; //k: Ambient interaction with shadow mapping(parallel light)

GLSL_PROGRAM_PROC shaderProgram_t interactionShadowMappingShader_spotLight; //k: PHONG interaction with shadow mapping(spot light)
GLSL_PROGRAM_PROC shaderProgram_t interactionShadowMappingPBRShader_spotLight; //k: PBR interaction with shadow mapping(spot light)
GLSL_PROGRAM_PROC shaderProgram_t interactionShadowMappingBlinnPhongShader_spotLight; //k: BLINN-PHONG interaction with shadow mapping(spot light)
GLSL_PROGRAM_PROC shaderProgram_t ambientLightingShadowMappingShader_spotLight; //k: Ambient interaction with shadow mapping(spot light)

GLSL_PROGRAM_PROC shaderProgram_t depthPerforatedShader; //k: depth perforated shader
GLSL_PROGRAM_PROC shaderProgram_t depthPerforatedShader_color; //k: depth perforated shader(if not support depth texture(2D / cubemap), only ES2.0)
#endif

// Augmented stencil shadow
#ifdef _STENCIL_SHADOW_IMPROVE
GLSL_PROGRAM_PROC shaderProgram_t interactionTranslucentShader; //k: PHONG lighting model interaction shader(translucent stencil shadow)
GLSL_PROGRAM_PROC shaderProgram_t interactionPBRTranslucentShader; //k: PBR lighting model interaction shader(translucent stencil shadow)
GLSL_PROGRAM_PROC shaderProgram_t interactionBlinnPhongTranslucentShader; //k: BLINN-PHONG lighting model interaction shader(translucent stencil shadow)
GLSL_PROGRAM_PROC shaderProgram_t ambientLightingTranslucentShader; //k: Ambient lighting model interaction shader(translucent stencil shadow)

#ifdef _SOFT_STENCIL_SHADOW
GLSL_PROGRAM_PROC shaderProgram_t interactionSoftShader; //k: PHONG lighting model interaction shader(soft stencil shadow)
GLSL_PROGRAM_PROC shaderProgram_t interactionPBRSoftShader; //k: PBR lighting model interaction shader(soft stencil shadow)
GLSL_PROGRAM_PROC shaderProgram_t interactionBlinnPhongSoftShader; //k: BLINN-PHONG lighting model interaction shader(soft stencil shadow)
GLSL_PROGRAM_PROC shaderProgram_t ambientLightingSoftShader; //k: Ambient lighting model interaction shader(soft stencil shadow)
#endif

#endif
