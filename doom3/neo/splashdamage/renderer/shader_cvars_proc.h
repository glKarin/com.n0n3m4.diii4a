
#ifndef QSHADER_CVAR_PROC
#error "you must define QSHADER_CVAR_PROC before including this file"
#endif

#define SHADER_CVARS(defname) defname[] = { \
    QSHADER_CVAR_PROC(r_shaderQuality), \
    QSHADER_CVAR_PROC(r_megaDrawMethod), \
    QSHADER_CVAR_PROC(r_normalizeNormalMaps), \
    QSHADER_CVAR_PROC(r_dxnNormalMaps), \
    QSHADER_CVAR_PROC(r_32ByteVtx), \
    QSHADER_CVAR_PROC(r_useDitherMask), \
    QSHADER_CVAR_PROC(r_shaderSkipSpecCubeMaps), \
    QSHADER_CVAR_PROC(alphatest_kill), \
    QSHADER_CVAR_PROC(r_detailTexture), \
    QSHADER_CVAR_PROC(r_megaMultiply), \
    QSHADER_CVAR_PROC(r_useARBPositionInvariant), \
    QSHADER_CVAR_PROC(r_skipDiffuse), \
    QSHADER_CVAR_PROC(r_skipBump), \
};

//#undef QSHADER_CVAR_PROC
