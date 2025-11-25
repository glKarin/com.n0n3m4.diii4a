#ifndef _MODEL_MD5EXT_H
#define _MODEL_MD5EXT_H

#ifdef _EXTRAS_TOOLS
void R_MD5Edit_AddCommand(void);

void R_ModelTest_AddCommand(void);
void R_ModelLight_AddCommand(void);
#endif

#if defined(_MODEL_PSK) || defined(_MODEL_IQM) || defined(_MODEL_SMD) || defined(_MODEL_GLTF) || defined(_MODEL_FBX)
#define _MODEL_MD5_EXT 1
#endif

#ifdef _MODEL_MD5_EXT
enum {
    CCP_NONE = 0,
    CCP_MESH = 1,
    CCP_SCALE = 1 << 1,
    CCP_FLAGS = 1 << 2,
    CCP_OFFSET = 1 << 3,
    CCP_ROTATION = 1 << 4,
    CCP_ANIMATIONS = 1 << 5,
    CCP_SAVEPATH = 1 << 6,
};
#define CONVERT_TO_MD5MESH_USAGE(x) "Usage: %s <" #x " file> [-scale <scale> -addOrigin -renameOrigin -offset <\"x-axis y-axis z-axis\"> -rotation <\"pitch yaw roll\"> -savePath <output path>]\n"
#define CONVERT_TO_MD5_USAGE(x, y) "Usage: %s <" #x " file> [-scale <scale> -addOrigin -renameOrigin -offset <\"x-axis y-axis z-axis\"> -rotation <\"pitch yaw roll\"> -savePath <output path> [-animation] <" #y ">......]\n"
#define CONVERT_TO_MD5ANIM_USAGE(x, y) CONVERT_TO_MD5_USAGE(x, y)
#define CONVERT_TO_MD5_ALL_USAGE(x) "Usage: %s <" #x " file> [-scale <scale> -addOrigin -renameOrigin -offset <\"x-axis y-axis z-axis\"> -rotation <\"pitch yaw roll\"> -savePath <output path> [ [-animation] <animation name or index>......] ]\n"
#define CONVERT_TO_MD5ANIM_ALL_USAGE(x) CONVERT_TO_MD5_ALL_USAGE(x)

//#define ETW_PSK 1
#define WEIGHTS_SUM_NOT_EQUALS_ONE(w) ((w) < (1.0f - idMath::FLT_EPSILON))
int R_Model_ParseMd5ConvertCmdLine(const idCmdArgs &args, idStr *mesh, int *flags, float *scale, idVec3 *offset, idMat3 *rotation, idStrList *anims, idStr *savePath);
idStr R_Model_MakeOutputPath(const char *originPath, const char *extName = NULL, const char *savePath = NULL);

bool R_Model_ConvertToMd5(const char *fileName);

void R_Md5Convert_AddCommand(void);
#endif

#ifdef _MODEL_PSK
void R_ActorX_AddCommand(void);
#endif

#ifdef _MODEL_IQM
void R_IQM_AddCommand(void);
#endif

#ifdef _MODEL_SMD
void R_SMD_AddCommand(void);
#endif

#ifdef _MODEL_GLTF
void R_GLTF_AddCommand(void);
#endif

#ifdef _MODEL_FBX
void R_Fbx_AddCommand(void);
#endif

void R_Md5v6_AddCommand(void);

ID_INLINE void R_Model_AddCommand(void)
{
#ifdef _MODEL_PSK
    R_ActorX_AddCommand();
#endif
#ifdef _MODEL_IQM
    R_IQM_AddCommand();
#endif
#ifdef _MODEL_SMD
    R_SMD_AddCommand();
#endif
#ifdef _MODEL_GLTF
    R_GLTF_AddCommand();
#endif
#ifdef _MODEL_FBX
    R_Fbx_AddCommand();
#endif
#ifdef _MODEL_MD5_EXT
    R_Md5Convert_AddCommand();
#endif
    R_Md5v6_AddCommand();
}

#endif /* !_MODEL_MD5EXT_H */
