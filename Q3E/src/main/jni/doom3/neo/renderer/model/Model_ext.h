#ifndef _MODEL_MD5EXT_H
#define _MODEL_MD5EXT_H

#ifdef _EXTRAS_TOOLS
void MD5Edit_AddCommand(void);

void ModelTest_AddCommand(void);
void ModelLight_AddCommand(void);
#endif

#ifdef _MODEL_PSK
void Unreal_AddCommand(void);
#endif

#ifdef _MODEL_IQM
void IQM_AddCommand(void);
#endif

#if defined(_MODEL_PSK) || defined(_MODEL_IQM)
#define _MODEL_MD5_EXT 1
#endif

#ifdef _MODEL_MD5_EXT
bool R_Model_ConvertToMd5(const char *fileName);

void Md5Model_AddCommand(void);
#endif

#endif /* !_MODEL_MD5EXT_H */
