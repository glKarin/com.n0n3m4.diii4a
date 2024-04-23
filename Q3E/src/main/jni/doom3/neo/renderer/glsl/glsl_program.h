#if !defined(GLSL_PROGRAM_PROC)
#error "you must define GLSL_PROGRAM_PROC before including this file"
#endif

// base
GLSL_PROGRAM_PROC shaderProgram_t	interactionShader;
GLSL_PROGRAM_PROC shaderProgram_t	shadowShader;
GLSL_PROGRAM_PROC shaderProgram_t	defaultShader;
GLSL_PROGRAM_PROC shaderProgram_t	depthFillShader;
GLSL_PROGRAM_PROC shaderProgram_t	depthFillClipShader; //k: z-fill clipped shader
GLSL_PROGRAM_PROC shaderProgram_t cubemapShader; //k: skybox shader
GLSL_PROGRAM_PROC shaderProgram_t reflectionCubemapShader; //k: reflection shader
GLSL_PROGRAM_PROC shaderProgram_t	fogShader; //k: fog shader
GLSL_PROGRAM_PROC shaderProgram_t	blendLightShader; //k: blend light shader
GLSL_PROGRAM_PROC shaderProgram_t	interactionBlinnPhongShader; //k: BLINN-PHONG lighting model interaction shader
GLSL_PROGRAM_PROC shaderProgram_t diffuseCubemapShader; //k: diffuse cubemap shader
GLSL_PROGRAM_PROC shaderProgram_t texgenShader; //k: texgen shader

// new stage
GLSL_PROGRAM_PROC shaderProgram_t heatHazeShader; //k: heatHaze shader
GLSL_PROGRAM_PROC shaderProgram_t heatHazeWithMaskShader; //k: heatHaze with mask shader
GLSL_PROGRAM_PROC shaderProgram_t heatHazeWithMaskAndVertexShader; //k: heatHaze with mask and vertex shader
GLSL_PROGRAM_PROC shaderProgram_t colorProcessShader; //k: color process shader

#ifdef _SHADOW_MAPPING
GLSL_PROGRAM_PROC shaderProgram_t   depthShader; //k: depth shader
GLSL_PROGRAM_PROC shaderProgram_t   depthShader_color; //k: depth shader(if not support depth texture(2D / cubemap), only ES2.0)

GLSL_PROGRAM_PROC shaderProgram_t	interactionShadowMappingShader_pointLight; //k: interaction with shadow mapping(point light)
GLSL_PROGRAM_PROC shaderProgram_t	interactionShadowMappingBlinnPhongShader_pointLight; //k: interaction with shadow mapping(point light)

GLSL_PROGRAM_PROC shaderProgram_t	interactionShadowMappingShader_parallelLight; //k: interaction with shadow mapping(parallel)
GLSL_PROGRAM_PROC shaderProgram_t	interactionShadowMappingBlinnPhongShader_parallelLight; //k: interaction with shadow mapping(parallel)

GLSL_PROGRAM_PROC shaderProgram_t	interactionShadowMappingShader_spotLight; //k: interaction with shadow mapping
GLSL_PROGRAM_PROC shaderProgram_t	interactionShadowMappingBlinnPhongShader_spotLight; //k: interaction with shadow mapping

GLSL_PROGRAM_PROC shaderProgram_t depthPerforatedShader; //k: depth perforated shader
GLSL_PROGRAM_PROC shaderProgram_t depthPerforatedShader_color; //k: depth perforated shader(if not support depth texture(2D / cubemap), only ES2.0)
#endif

#ifdef _TRANSLUCENT_STENCIL_SHADOW
GLSL_PROGRAM_PROC shaderProgram_t	interactionTranslucentShader; //k: PHONG lighting model interaction shader(translucent stencil shadow)
GLSL_PROGRAM_PROC shaderProgram_t	interactionBlinnPhongTranslucentShader; //k: BLINN-PHONG lighting model interaction shader(translucent stencil shadow)
#endif