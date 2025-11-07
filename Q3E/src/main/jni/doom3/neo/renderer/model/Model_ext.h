#ifndef _MODEL_MD5EXT_H
#define _MODEL_MD5EXT_H

#ifdef _EXTRAS_TOOLS
void R_MD5Edit_AddCommand(void);

void R_ModelTest_AddCommand(void);
void R_ModelLight_AddCommand(void);
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

#if defined(_MODEL_PSK) || defined(_MODEL_IQM) || defined(_MODEL_GLTF)
#define _MODEL_MD5_EXT 1
#endif

#ifdef _MODEL_MD5_EXT
bool R_Model_ConvertToMd5(const char *fileName);

void R_Md5Convert_AddCommand(void);
#endif

#endif /* !_MODEL_MD5EXT_H */
