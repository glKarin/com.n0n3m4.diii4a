#ifndef _MODEL_MD5CONVERT_H
#define _MODEL_MD5CONVERT_H

typedef struct md5ConvertDef_s
{
    const idDeclEntityDef *def;
    idStr type;
    float scale;
	idVec3 offset;
	idMat3 rotation;
    bool addOrigin;
    idStr mesh;
    idStrList animNames;
    idStrList anims;
    idStr savePath;
} md5ConvertDef_t;

typedef bool (* md5ConvertDef_f)(const md5ConvertDef_t &def);

bool R_Model_ConvertToMd5(const char *fileName);

#ifdef _MODEL_PSK
bool R_Model_HandlePskPsa(const md5ConvertDef_t &convert);
#endif
#ifdef _MODEL_IQM
bool R_Model_HandleIqm(const md5ConvertDef_t &convert);
#endif
#ifdef _MODEL_SMD
bool R_Model_HandleSmd(const md5ConvertDef_t &convert);
#endif
#ifdef _MODEL_GLTF
bool R_Model_HandleGLTF(const md5ConvertDef_t &convert);
#endif

bool R_Model_HandleMd5V6(const md5ConvertDef_t &convert);

#endif
